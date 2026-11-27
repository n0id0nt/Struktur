#include "JobSystem.h"

#include <chrono>

namespace Struktur::Jobs
{
static constexpr int MAX_WORKERS = 64;
// How long a waiter sleeps between checks when it has nothing to help with (it is also woken by completions).
static constexpr std::chrono::microseconds WAIT_SLEEP{200};

// Which worker of which system this thread is (-1 = not a worker).
static thread_local const JobSystem* s_workerOwner = nullptr;

const char* ToString(Lane lane)
{
	switch (lane)
	{
		case Lane::POST_FRAME:
			return "postFrame";
		case Lane::BACKGROUND:
			return "background";
		default:
			return "frame";
	}
}

// --- JobHandle ---------------------------------------------------------------------------------------------------

JobHandle::JobHandle(std::shared_ptr<JobState> state)
    : m_state(std::move(state))
{
}

bool JobHandle::IsValid() const
{
	return m_state != nullptr;
}

bool JobHandle::IsComplete() const
{
	return m_state == nullptr || m_state->done.load(std::memory_order_acquire);
}

bool JobHandle::WasDropped() const
{
	return m_state != nullptr && m_state->dropped.load(std::memory_order_acquire);
}

Lane JobHandle::GetLane() const
{
	return m_state != nullptr ? m_state->lane : Lane::FRAME;
}

// --- JobSystem ---------------------------------------------------------------------------------------------------

bool JobSystem::ThreadsAvailable()
{
#if STRUKTUR_JOBS_THREADS
	return true;
#else
	return false;
#endif
}

int JobSystem::ResolveWorkerCount(int requested)
{
	if (!ThreadsAvailable())
	{
		return 0;
	}
	if (requested >= 0)
	{
		return std::min(requested, MAX_WORKERS);
	}
#if STRUKTUR_JOBS_THREADS
	const int cores = static_cast<int>(std::thread::hardware_concurrency());
	return std::clamp(cores - 1, 0, MAX_WORKERS);
#else
	return 0;
#endif
}

JobSystem::JobSystem(const JobSystemConfig& config)
    : m_workerCount(ResolveWorkerCount(config.workerCount)),
      m_stopping(false),
      m_shutDown(false),
      m_draining(false)
{
#if STRUKTUR_JOBS_THREADS
	m_workers.reserve(static_cast<size_t>(m_workerCount));
	for (int index = 0; index < m_workerCount; ++index)
	{
		m_workers.emplace_back(&JobSystem::WorkerLoop, this, index);
	}
#endif
}

JobSystem::~JobSystem()
{
	Shutdown();
}

int JobSystem::GetWorkerCount() const
{
	return m_workerCount;
}

bool JobSystem::IsWorkerThread() const
{
	return s_workerOwner == this;
}

JobHandle JobSystem::Schedule(std::function<void()> work, Lane lane)
{
	return Schedule(std::move(work), lane, std::vector<JobHandle>());
}

JobHandle JobSystem::Schedule(std::function<void()> work, Lane lane, const JobHandle& dependency)
{
	return Schedule(std::move(work), lane, std::vector<JobHandle>{dependency});
}

JobHandle JobSystem::Schedule(std::function<void()> work, Lane lane, const std::vector<JobHandle>& dependencies)
{
	std::shared_ptr<JobState> job = std::make_shared<JobState>();
	job->work                     = std::move(work);
	job->lane                     = lane;
	m_jobsScheduled.fetch_add(1, std::memory_order_relaxed);
	for (const JobHandle& dependency : dependencies)
	{
		JobState* state = dependency.m_state.get();
		if (state == nullptr)
		{
			continue;
		}
		std::lock_guard<std::mutex> lock(state->mutex);
		if (!state->done.load(std::memory_order_acquire))
		{
			job->pendingDependencies.fetch_add(1, std::memory_order_relaxed);
			state->dependents.push_back(job);
		}
	}
	// Release the set-up guard; if every dependency was already done the job is ready now. A job with no work (WhenAll)
	// that is already ready is simply complete - it never needs to be queued.
	if (job->pendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		if (job->work)
		{
			Enqueue(job);
		}
		else
		{
			m_jobsRun.fetch_add(1, std::memory_order_relaxed);
			Finish(job);
		}
	}
	return JobHandle(job);
}

JobHandle JobSystem::WhenAll(const std::vector<JobHandle>& handles, Lane lane)
{
	return Schedule(std::function<void()>(), lane, handles);
}

void JobSystem::Enqueue(std::shared_ptr<JobState> job)
{
	bool dropNow = false;
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_queues[static_cast<int>(job->lane)].push_back(std::move(job));
		dropNow = m_shutDown;
	}
	if (dropNow)
	{
		DrainDropped();
		return;
	}
	if (m_workerCount > 0)
	{
		m_workAvailable.notify_one();
	}
}

bool JobSystem::PopJob(Lane lowestLane, std::shared_ptr<JobState>& job)
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	for (int lane = 0; lane <= static_cast<int>(lowestLane); ++lane)
	{
		std::deque<std::shared_ptr<JobState>>& queue = m_queues[lane];
		if (!queue.empty())
		{
			job = std::move(queue.front());
			queue.pop_front();
			return true;
		}
	}
	return false;
}

void JobSystem::Execute(const std::shared_ptr<JobState>& job, bool helper)
{
	if (job->work)
	{
		job->work();
		job->work = nullptr;  // release whatever the job captured as soon as it has run
	}
	m_jobsRun.fetch_add(1, std::memory_order_relaxed);
	if (helper)
	{
		m_jobsRunByHelper.fetch_add(1, std::memory_order_relaxed);
	}
	Finish(job);
}

void JobSystem::Finish(const std::shared_ptr<JobState>& job)
{
	std::vector<std::shared_ptr<JobState>> dependents;
	{
		std::lock_guard<std::mutex> lock(job->mutex);
		job->done.store(true, std::memory_order_release);
		dependents.swap(job->dependents);
	}
	for (std::shared_ptr<JobState>& dependent : dependents)
	{
		if (dependent->pendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			Enqueue(std::move(dependent));
		}
	}
	if (m_waiters.load(std::memory_order_acquire) > 0)
	{
		std::lock_guard<std::mutex> lock(m_completionMutex);
		m_jobCompleted.notify_all();
	}
}

bool JobSystem::TryRunOne(Lane lowestLane)
{
	std::shared_ptr<JobState> job;
	if (!PopJob(lowestLane, job))
	{
		return false;
	}
	Execute(job, true);
	return true;
}

size_t JobSystem::RunPending(double budgetSeconds, Lane lowestLane)
{
	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	size_t ran                                         = 0;
	while (TryRunOne(lowestLane))
	{
		++ran;
		const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
		if (elapsed >= budgetSeconds)
		{
			break;
		}
	}
	return ran;
}

void JobSystem::Wait(const JobHandle& handle)
{
	JobState* state = handle.m_state.get();
	if (state == nullptr)
	{
		return;
	}
	// Help with work of the waited job's priority or higher. Worker threads (and the no-worker case, where nobody
	// else will) may also take lower lanes, which is where the waited job's dependencies can be.
	const Lane helpLane     = state->lane;
	const bool mayHelpLower = IsWorkerThread() || m_workerCount == 0;
	while (!state->done.load(std::memory_order_acquire))
	{
		if (TryRunOne(helpLane))
		{
			continue;
		}
		if (mayHelpLower && TryRunOne(Lane::BACKGROUND))
		{
			continue;
		}
		if (state->done.load(std::memory_order_acquire))
		{
			break;
		}
		if (m_workerCount == 0)
		{
			// Nothing queued and nobody else to run it: it can't complete (a dependency that was never scheduled).
			break;
		}
		m_waiters.fetch_add(1, std::memory_order_acq_rel);
		{
			std::unique_lock<std::mutex> lock(m_completionMutex);
			if (!state->done.load(std::memory_order_acquire))
			{
				m_jobCompleted.wait_for(lock, WAIT_SLEEP);
			}
		}
		m_waiters.fetch_sub(1, std::memory_order_acq_rel);
	}
}

void JobSystem::WaitAll(const std::vector<JobHandle>& handles)
{
	for (const JobHandle& handle : handles)
	{
		Wait(handle);
	}
}

size_t JobSystem::GetQueuedCount() const
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	size_t count = 0;
	for (const std::deque<std::shared_ptr<JobState>>& queue : m_queues)
	{
		count += queue.size();
	}
	return count;
}

JobSystemStats JobSystem::GetStats() const
{
	JobSystemStats stats;
	stats.workerCount     = m_workerCount;
	stats.jobsScheduled   = m_jobsScheduled.load(std::memory_order_relaxed);
	stats.jobsRun         = m_jobsRun.load(std::memory_order_relaxed);
	stats.jobsRunByHelper = m_jobsRunByHelper.load(std::memory_order_relaxed);
	stats.jobsDropped     = m_jobsDropped.load(std::memory_order_relaxed);
	return stats;
}

bool JobSystem::AnyQueuedLocked() const
{
	for (const std::deque<std::shared_ptr<JobState>>& queue : m_queues)
	{
		if (!queue.empty())
		{
			return true;
		}
	}
	return false;
}

void JobSystem::WorkerLoop(int workerIndex)
{
	(void)workerIndex;
	s_workerOwner = this;
	for (;;)
	{
		std::shared_ptr<JobState> job;
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			while (!m_stopping && !AnyQueuedLocked())
			{
				m_workAvailable.wait(lock);
			}
			if (m_stopping)
			{
				return;
			}
			for (std::deque<std::shared_ptr<JobState>>& queue : m_queues)
			{
				if (!queue.empty())
				{
					job = std::move(queue.front());
					queue.pop_front();
					break;
				}
			}
		}
		Execute(job, false);
	}
}

void JobSystem::DrainDropped()
{
	// Iterative, so a long dependency chain doesn't recurse: dropping a job enqueues its dependents, which this loop
	// then drops too. Only one thread drains at a time; others just leave their job in the queue for it.
	for (;;)
	{
		if (m_draining.exchange(true, std::memory_order_acq_rel))
		{
			return;
		}
		std::shared_ptr<JobState> job;
		while (PopJob(Lane::BACKGROUND, job))
		{
			job->dropped.store(true, std::memory_order_release);
			job->work = nullptr;
			m_jobsDropped.fetch_add(1, std::memory_order_relaxed);
			Finish(job);
			job.reset();
		}
		m_draining.store(false, std::memory_order_release);
		// A job enqueued while this thread was finishing up would otherwise be left behind.
		if (GetQueuedCount() == 0)
		{
			return;
		}
	}
}

void JobSystem::Shutdown()
{
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		if (m_shutDown)
		{
			return;
		}
		m_stopping = true;
		m_shutDown = true;
	}
	m_workAvailable.notify_all();
#if STRUKTUR_JOBS_THREADS
	for (std::thread& worker : m_workers)
	{
		worker.join();
	}
	m_workers.clear();
#endif
	DrainDropped();
}
}  // namespace Struktur::Jobs
