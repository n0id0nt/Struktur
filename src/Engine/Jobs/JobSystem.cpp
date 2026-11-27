#include "JobSystem.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace Struktur::Jobs
{
static constexpr int MAX_WORKERS = 64;
// How long a waiter sleeps between checks when it has nothing to help with (it is also woken by completions).
static constexpr std::chrono::microseconds WAIT_SLEEP{200};
// An idle worker yields this many times before it goes to sleep, and sleeps at most this long between checks.
static constexpr int IDLE_SPINS = 64;
static constexpr std::chrono::milliseconds IDLE_SLEEP{2};
// Slots move between the shared pool and a thread's cache in batches of this many.
static constexpr size_t CACHE_BATCH = 256;
static constexpr int SPIN_BEFORE_YIELD = 64;

// Which worker of which system this thread is.
static thread_local const JobSystem* s_workerOwner = nullptr;
static thread_local int s_workerIndex              = -1;

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

void JobSystem::SpinLock::lock()
{
	int spins = 0;
	while (m_flag.test_and_set(std::memory_order_acquire))
	{
		if (++spins >= SPIN_BEFORE_YIELD)
		{
#if STRUKTUR_JOBS_THREADS
			std::this_thread::yield();
#endif
			spins = 0;
		}
	}
}

// --- JobHandle ---------------------------------------------------------------------------------------------------

JobHandle::JobHandle(JobSystem* system, uint32_t index, uint32_t generation, Lane lane)
    : m_system(system),
      m_index(index),
      m_generation(generation),
      m_lane(lane)
{
}

bool JobHandle::IsValid() const
{
	return m_system != nullptr;
}

bool JobHandle::IsComplete() const
{
	if (m_system == nullptr)
	{
		return true;
	}
	const uint64_t state = m_system->Slot(m_index).state.load(std::memory_order_acquire);
	// A newer generation means the slot was recycled - which only happens after this job finished.
	return (state >> JobSystem::GENERATION_SHIFT) != m_generation || (state & JobSystem::DONE_BIT) != 0;
}

bool JobHandle::WasDropped() const
{
	if (m_system == nullptr)
	{
		return false;
	}
	const uint64_t state = m_system->Slot(m_index).state.load(std::memory_order_acquire);
	return (state >> JobSystem::GENERATION_SHIFT) == m_generation && (state & JobSystem::DROPPED_BIT) != 0;
}

Lane JobHandle::GetLane() const
{
	return m_lane;
}

// --- set-up ------------------------------------------------------------------------------------------------------

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

int JobSystem::ResolveBackgroundLimit(int requested, int workers)
{
	if (workers <= 0)
	{
		return 0;
	}
	if (requested >= 0)
	{
		return std::min(requested, workers);
	}
	if (workers == 1)
	{
		return 1;
	}
	return workers <= 3 ? workers - 1 : workers - 2;
}

JobSystem::JobSystem(const JobSystemConfig& config)
    : m_workerCount(ResolveWorkerCount(config.workerCount)),
      m_backgroundLimit(ResolveBackgroundLimit(config.backgroundWorkerLimit, m_workerCount))
{
	for (std::atomic<int>& queued : m_queued)
	{
		queued.store(0, std::memory_order_relaxed);
	}
	for (int index = 0; index <= m_workerCount; ++index)
	{
		m_caches.push_back(std::make_unique<SlotCache>());
	}
	for (int index = 0; index < QueueCount(); ++index)
	{
		m_queues.push_back(std::make_unique<WorkerQueue>());
	}
	{
		std::lock_guard<std::mutex> lock(m_poolMutex);
		GrowPool();
	}
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

int JobSystem::GetBackgroundWorkerLimit() const
{
	return m_backgroundLimit;
}

bool JobSystem::IsWorkerThread() const
{
	return s_workerOwner == this;
}

int JobSystem::CurrentWorker() const
{
	return s_workerOwner == this ? s_workerIndex : -1;
}

int JobSystem::QueueCount() const
{
	return std::max(1, m_workerCount);
}

// --- pool --------------------------------------------------------------------------------------------------------

JobSystem::JobSlot& JobSystem::Slot(uint32_t index) const
{
	return m_chunks[index >> CHUNK_BITS][index & (CHUNK_SIZE - 1)];
}

void JobSystem::GrowPool()
{
	const uint32_t chunk = m_chunkCount.load(std::memory_order_relaxed);
	if (chunk >= MAX_CHUNKS)
	{
		std::fprintf(stderr, "JobSystem: more than %u jobs alive at once\n", MAX_CHUNKS * CHUNK_SIZE);
		std::abort();
	}
	m_chunks[chunk] = std::make_unique<JobSlot[]>(CHUNK_SIZE);
	m_globalFree.reserve(m_globalFree.size() + CHUNK_SIZE);
	for (uint32_t offset = CHUNK_SIZE; offset > 0; --offset)
	{
		m_globalFree.push_back((chunk << CHUNK_BITS) | (offset - 1));
	}
	m_chunkCount.store(chunk + 1, std::memory_order_release);
}

uint32_t JobSystem::AllocateSlot()
{
	const int self   = CurrentWorker();
	SlotCache& cache = self >= 0 ? *m_caches[static_cast<size_t>(self)] : *m_caches.back();
	std::unique_lock<std::mutex> cacheLock(cache.mutex, std::defer_lock);
	if (self < 0)
	{
		cacheLock.lock();
	}
	if (cache.free.empty())
	{
		std::lock_guard<std::mutex> lock(m_poolMutex);
		if (m_globalFree.empty())
		{
			GrowPool();
		}
		const size_t take = std::min(CACHE_BATCH, m_globalFree.size());
		cache.free.insert(cache.free.end(), m_globalFree.end() - static_cast<ptrdiff_t>(take), m_globalFree.end());
		m_globalFree.resize(m_globalFree.size() - take);
	}
	const uint32_t index = cache.free.back();
	cache.free.pop_back();
	JobSlot& slot = Slot(index);
	{
		std::lock_guard<SpinLock> lock(slot.lock);
		const uint64_t generation = (slot.state.load(std::memory_order_relaxed) >> GENERATION_SHIFT) + 1;
		slot.state.store(generation << GENERATION_SHIFT, std::memory_order_release);
	}
	return index;
}

void JobSystem::FreeSlot(uint32_t index)
{
	if (m_shutDown.load(std::memory_order_acquire))
	{
		return;  // after shutdown slots stay put, so dropped handles keep answering WasDropped()
	}
	const int self   = CurrentWorker();
	SlotCache& cache = self >= 0 ? *m_caches[static_cast<size_t>(self)] : *m_caches.back();
	std::unique_lock<std::mutex> cacheLock(cache.mutex, std::defer_lock);
	if (self < 0)
	{
		cacheLock.lock();
	}
	cache.free.push_back(index);
	if (cache.free.size() >= CACHE_BATCH * 2)
	{
		std::lock_guard<std::mutex> lock(m_poolMutex);
		m_globalFree.insert(m_globalFree.end(), cache.free.end() - static_cast<ptrdiff_t>(CACHE_BATCH), cache.free.end());
		cache.free.resize(cache.free.size() - CACHE_BATCH);
	}
}

// --- scheduling --------------------------------------------------------------------------------------------------

JobHandle JobSystem::WhenAll(const std::vector<JobHandle>& handles, Lane lane)
{
	return Submit(AllocateSlot(), lane, handles.data(), handles.size());
}

JobHandle JobSystem::Submit(uint32_t index, Lane lane, const JobHandle* dependencies, size_t count)
{
	JobSlot& slot              = Slot(index);
	slot.lane                  = lane;
	const uint32_t generation  = static_cast<uint32_t>(slot.state.load(std::memory_order_acquire) >> GENERATION_SHIFT);
	slot.pendingDependencies.store(1, std::memory_order_relaxed);  // set-up guard
	m_jobsScheduled.fetch_add(1, std::memory_order_relaxed);
	for (size_t position = 0; position < count; ++position)
	{
		const JobHandle& dependency = dependencies[position];
		if (dependency.m_system != this)
		{
			continue;  // invalid (or another system's) handle: nothing to wait for
		}
		JobSlot& other = Slot(dependency.m_index);
		std::lock_guard<SpinLock> lock(other.lock);
		const uint64_t state = other.state.load(std::memory_order_acquire);
		if ((state >> GENERATION_SHIFT) == dependency.m_generation && (state & DONE_BIT) == 0)
		{
			slot.pendingDependencies.fetch_add(1, std::memory_order_relaxed);
			if (other.dependentCount < INLINE_DEPENDENTS)
			{
				other.dependents[other.dependentCount++] = index;
			}
			else
			{
				other.moreDependents.push_back(index);
			}
		}
	}
	const JobHandle handle(this, index, generation, lane);
	if (slot.pendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		if (slot.work.IsSet())
		{
			Enqueue(index);
		}
		else
		{
			// No work (WhenAll) and nothing to wait for: complete now, no need to queue it.
			m_jobsRun.fetch_add(1, std::memory_order_relaxed);
			Finish(index, false);
		}
	}
	return handle;
}

void JobSystem::Enqueue(uint32_t index, bool requeue)
{
	const int lane = static_cast<int>(Slot(index).lane);
	if (m_shutDown.load(std::memory_order_acquire))
	{
		{
			std::lock_guard<SpinLock> lock(m_queues[0]->lock);
			m_queues[0]->lanes[lane].push_back(index);
		}
		m_queued[lane].fetch_add(1, std::memory_order_seq_cst);
		DrainDropped();
		return;
	}
	const int self  = CurrentWorker();
	const int queue = self >= 0 ? self : static_cast<int>(m_nextQueue.fetch_add(1, std::memory_order_relaxed) % QueueCount());
	{
		std::lock_guard<SpinLock> lock(m_queues[static_cast<size_t>(queue)]->lock);
		std::deque<uint32_t>& jobs = m_queues[static_cast<size_t>(queue)]->lanes[lane];
		// A requeued step goes behind the other work: on a worker's own queue that is the oldest end (the owner takes
		// its newest first), elsewhere the newest end (other threads take the oldest first).
		if (requeue && self >= 0)
		{
			jobs.push_front(index);
		}
		else
		{
			jobs.push_back(index);
		}
	}
	m_queued[lane].fetch_add(1, std::memory_order_seq_cst);
	WakeOne();
}

void JobSystem::WakeOne()
{
	if (m_workerCount > 0 && m_sleepers.load(std::memory_order_seq_cst) > 0)
	{
		std::lock_guard<std::mutex> lock(m_wakeMutex);
		m_wake.notify_one();
	}
}

bool JobSystem::PopFromQueue(int queue, int lane, bool newest, uint32_t& index)
{
	WorkerQueue& target = *m_queues[static_cast<size_t>(queue)];
	{
		std::lock_guard<SpinLock> lock(target.lock);
		std::deque<uint32_t>& jobs = target.lanes[lane];
		if (jobs.empty())
		{
			return false;
		}
		if (newest)
		{
			index = jobs.back();
			jobs.pop_back();
		}
		else
		{
			index = jobs.front();
			jobs.pop_front();
		}
	}
	m_queued[lane].fetch_sub(1, std::memory_order_relaxed);
	return true;
}

bool JobSystem::PopJob(int self, Lane lowestLane, bool allowBackground, uint32_t& index)
{
	const int queues = QueueCount();
	for (int lane = 0; lane <= static_cast<int>(lowestLane); ++lane)
	{
		if (lane == static_cast<int>(Lane::BACKGROUND) && !allowBackground)
		{
			continue;
		}
		if (m_queued[lane].load(std::memory_order_acquire) <= 0)
		{
			continue;  // a hint: a job pushed this instant is found on the next look
		}
		// Own queue first, newest job (still warm in cache); then steal the oldest from the others.
		if (self >= 0 && PopFromQueue(self, lane, true, index))
		{
			return true;
		}
		const int start = self >= 0 ? self + 1 : static_cast<int>(m_nextQueue.load(std::memory_order_relaxed) % queues);
		for (int offset = 0; offset < queues; ++offset)
		{
			const int queue = (start + offset) % queues;
			if (queue == self)
			{
				continue;
			}
			if (PopFromQueue(queue, lane, false, index))
			{
				if (self >= 0)
				{
					m_jobsStolen.fetch_add(1, std::memory_order_relaxed);
				}
				return true;
			}
		}
	}
	return false;
}

void JobSystem::Execute(uint32_t index, bool helper)
{
	JobSlot& slot = Slot(index);
	if (slot.work.IsSet())
	{
		if (!slot.work())
		{
			// A stepped job that wants another turn: same slot and handle, back in the queue.
			m_stepsRequeued.fetch_add(1, std::memory_order_relaxed);
			Enqueue(index, true);
			return;
		}
		slot.work.Reset();  // release whatever the job captured as soon as it has run
	}
	m_jobsRun.fetch_add(1, std::memory_order_relaxed);
	if (helper)
	{
		m_jobsRunByHelper.fetch_add(1, std::memory_order_relaxed);
	}
	Finish(index, false);
}

void JobSystem::Finish(uint32_t index, bool dropped)
{
	JobSlot& slot = Slot(index);
	uint32_t local[INLINE_DEPENDENTS];
	uint8_t localCount = 0;
	std::vector<uint32_t> extra;
	{
		std::lock_guard<SpinLock> lock(slot.lock);
		slot.state.fetch_or(DONE_BIT | (dropped ? DROPPED_BIT : 0), std::memory_order_acq_rel);
		localCount = slot.dependentCount;
		for (uint8_t position = 0; position < localCount; ++position)
		{
			local[position] = slot.dependents[position];
		}
		slot.dependentCount = 0;
		extra.swap(slot.moreDependents);
	}
	for (size_t position = 0; position < localCount + extra.size(); ++position)
	{
		const uint32_t dependent = position < localCount ? local[position] : extra[position - localCount];
		JobSlot& waiting         = Slot(dependent);
		if (waiting.pendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			if (waiting.work.IsSet() || m_shutDown.load(std::memory_order_acquire))
			{
				Enqueue(dependent);
			}
			else
			{
				m_jobsRun.fetch_add(1, std::memory_order_relaxed);
				Finish(dependent, false);  // a WhenAll: nothing to run
			}
		}
	}
	if (m_waiters.load(std::memory_order_acquire) > 0)
	{
		std::lock_guard<std::mutex> lock(m_completionMutex);
		m_jobCompleted.notify_all();
	}
	FreeSlot(index);
}

// --- running -----------------------------------------------------------------------------------------------------

bool JobSystem::TryRunOne(Lane lowestLane)
{
	uint32_t index = 0;
	if (!PopJob(CurrentWorker(), lowestLane, true, index))
	{
		return false;
	}
	Execute(index, true);
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
	if (handle.m_system != this)
	{
		return;
	}
	// Help with work of the waited job's priority or higher. Worker threads (and the no-worker case, where nobody
	// else will) may also take lower lanes, which is where the waited job's dependencies can be.
	const Lane helpLane     = handle.m_lane;
	const bool mayHelpLower = IsWorkerThread() || m_workerCount == 0;
	while (!handle.IsComplete())
	{
		if (TryRunOne(helpLane))
		{
			continue;
		}
		if (mayHelpLower && TryRunOne(Lane::BACKGROUND))
		{
			continue;
		}
		if (handle.IsComplete())
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
			if (!handle.IsComplete())
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

bool JobSystem::WorkerHasWork() const
{
	return m_queued[0].load(std::memory_order_seq_cst) > 0 || m_queued[1].load(std::memory_order_seq_cst) > 0 ||
	       (m_queued[2].load(std::memory_order_seq_cst) > 0 &&
	        m_backgroundRunning.load(std::memory_order_seq_cst) < m_backgroundLimit);
}

void JobSystem::WorkerLoop(int workerIndex)
{
	s_workerOwner = this;
	s_workerIndex = workerIndex;
	int idle      = 0;
	while (!m_stopping.load(std::memory_order_acquire))
	{
		uint32_t index = 0;
		if (PopJob(workerIndex, Lane::POST_FRAME, false, index))
		{
			Execute(index, false);
			idle = 0;
			continue;
		}
		// Background work, only while fewer than the limit of workers are on it.
		if (m_queued[2].load(std::memory_order_acquire) > 0)
		{
			if (m_backgroundRunning.fetch_add(1, std::memory_order_acq_rel) < m_backgroundLimit)
			{
				if (PopJob(workerIndex, Lane::BACKGROUND, true, index))
				{
					const bool background = Slot(index).lane == Lane::BACKGROUND;
					if (!background)
					{
						m_backgroundRunning.fetch_sub(1, std::memory_order_acq_rel);
					}
					Execute(index, false);
					if (background)
					{
						m_backgroundRunning.fetch_sub(1, std::memory_order_acq_rel);
					}
					idle = 0;
					continue;
				}
			}
			m_backgroundRunning.fetch_sub(1, std::memory_order_acq_rel);
		}
		if (++idle < IDLE_SPINS)
		{
#if STRUKTUR_JOBS_THREADS
			std::this_thread::yield();
#endif
			continue;
		}
		m_sleepers.fetch_add(1, std::memory_order_seq_cst);
		{
			std::unique_lock<std::mutex> lock(m_wakeMutex);
			if (!m_stopping.load(std::memory_order_acquire) && !WorkerHasWork())
			{
				m_wake.wait_for(lock, IDLE_SLEEP);
			}
		}
		m_sleepers.fetch_sub(1, std::memory_order_seq_cst);
		idle = IDLE_SPINS / 2;
	}
}

// --- main thread ---------------------------------------------------------------------------------------------------

static double SecondsSince(std::chrono::steady_clock::time_point start)
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

void JobSystem::PostToMainThread(std::unique_ptr<JobFunction> work)
{
	std::lock_guard<std::mutex> lock(m_mainThreadMutex);
	m_mainThreadQueue.push_back(std::move(work));
}

size_t JobSystem::RunFollowUps(std::chrono::steady_clock::time_point start, double budgetSeconds)
{
	size_t ran = 0;
	for (;;)
	{
		std::unique_ptr<JobFunction> work;
		{
			std::lock_guard<std::mutex> lock(m_mainThreadMutex);
			if (m_mainThreadQueue.empty())
			{
				break;
			}
			work = std::move(m_mainThreadQueue.front());
			m_mainThreadQueue.pop_front();
		}
		if (work->IsSet())
		{
			(*work)();
		}
		++ran;
		m_mainThreadRun.fetch_add(1, std::memory_order_relaxed);
		if (SecondsSince(start) >= budgetSeconds)
		{
			break;
		}
	}
	return ran;
}

size_t JobSystem::RunMainThreadWork(double budgetSeconds)
{
	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	size_t ran                                         = RunFollowUps(start, budgetSeconds);
	if (m_workerCount == 0 && SecondsSince(start) < budgetSeconds)
	{
		ran += RunPending(budgetSeconds - SecondsSince(start), Lane::BACKGROUND);
		if (SecondsSince(start) < budgetSeconds)
		{
			ran += RunFollowUps(start, budgetSeconds);
		}
	}
	return ran;
}

size_t JobSystem::GetMainThreadQueuedCount() const
{
	std::lock_guard<std::mutex> lock(m_mainThreadMutex);
	return m_mainThreadQueue.size();
}

// --- queries and shutdown ----------------------------------------------------------------------------------------

size_t JobSystem::GetQueuedCount() const
{
	size_t count = 0;
	for (const std::unique_ptr<WorkerQueue>& queue : m_queues)
	{
		std::lock_guard<SpinLock> lock(queue->lock);
		for (const std::deque<uint32_t>& jobs : queue->lanes)
		{
			count += jobs.size();
		}
	}
	return count;
}

JobSystemStats JobSystem::GetStats() const
{
	JobSystemStats stats;
	stats.workerCount           = m_workerCount;
	stats.backgroundWorkerLimit = m_backgroundLimit;
	stats.jobsScheduled         = m_jobsScheduled.load(std::memory_order_relaxed);
	stats.jobsRun               = m_jobsRun.load(std::memory_order_relaxed);
	stats.stepsRequeued         = m_stepsRequeued.load(std::memory_order_relaxed);
	stats.mainThreadRun         = m_mainThreadRun.load(std::memory_order_relaxed);
	stats.jobsRunByHelper       = m_jobsRunByHelper.load(std::memory_order_relaxed);
	stats.jobsStolen            = m_jobsStolen.load(std::memory_order_relaxed);
	stats.jobsDropped           = m_jobsDropped.load(std::memory_order_relaxed);
	stats.heapWork              = m_heapWork.load(std::memory_order_relaxed);
	stats.poolSlots             = m_chunkCount.load(std::memory_order_relaxed) * CHUNK_SIZE;
	return stats;
}

void JobSystem::DrainDropped()
{
	// Iterative, so a long dependency chain doesn't recurse: dropping a job enqueues its dependents, which this loop
	// then drops too. Only one thread drains at a time; the loop re-checks so a job enqueued meanwhile isn't missed.
	for (;;)
	{
		if (m_draining.exchange(true, std::memory_order_acq_rel))
		{
			return;
		}
		// Straight from the queues (not PopJob, whose queued-count hints can briefly lag a push).
		bool found = true;
		while (found)
		{
			found = false;
			for (int queue = 0; queue < QueueCount(); ++queue)
			{
				for (int lane = 0; lane < LANE_COUNT; ++lane)
				{
					uint32_t index = 0;
					while (PopFromQueue(queue, lane, false, index))
					{
						found = true;
						Slot(index).work.Reset();
						m_jobsDropped.fetch_add(1, std::memory_order_relaxed);
						Finish(index, true);
					}
				}
			}
		}
		m_draining.store(false, std::memory_order_release);
		if (GetQueuedCount() == 0)
		{
			return;
		}
	}
}

void JobSystem::Shutdown()
{
	if (m_shutDown.exchange(true, std::memory_order_acq_rel))
	{
		return;
	}
	m_stopping.store(true, std::memory_order_release);
	{
		std::lock_guard<std::mutex> lock(m_wakeMutex);
		m_wake.notify_all();
	}
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
