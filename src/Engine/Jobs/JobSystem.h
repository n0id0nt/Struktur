#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#if STRUKTUR_JOBS_THREADS
	#include <thread>
#endif

// The engine's job system: a pool of worker threads running small jobs, with dependencies between them.
//
//  - Schedule() queues a job and returns a JobHandle; a job can depend on other jobs and only starts once they are
//    all complete. JobHandle::IsComplete() can be polled from anywhere (e.g. the update loop).
//  - Lanes set priority: FRAME work (this frame's systems, parallel-for) first, then POST_FRAME (kicked after the
//    frame is submitted), then BACKGROUND (long work that may span many frames). Workers only run background jobs up
//    to a limit (all but two workers by default), so frame work always finds a free worker.
//  - Wait() never just blocks: the waiting thread runs queued jobs until the one it needs is done. The main thread
//    waiting on frame work only helps with frame-lane jobs, so it never picks up a long background job mid-frame.
//  - Run() gives a Task<T> (Task.h): a result, progress, cancellation and follow-ups that run on the main thread
//    inside RunMainThreadWork(). Work that returns StepResult is stepped: it runs again after each CONTINUE, giving
//    way to other work in between - how long background work stays out of frame work's way.
//  - Zero workers is a first-class mode (web builds without threads, debugging, STRUKTUR_THREADS=OFF): the same API,
//    jobs simply run on the calling thread inside Wait() / RunPending(). Nothing runs until someone waits or pumps.
//
// Internals (phase 1b): jobs live in a pooled slot array (no allocation per job; a job's work is stored inline when
// it fits in JobFunction::INLINE_SIZE bytes), and each worker has its own queue per lane - it takes its own newest
// job first and steals the oldest from other workers when it runs dry. Handles are (slot, generation) pairs, valid
// for as long as the JobSystem exists.
//
// Engine-agnostic (no SDL, bgfx or Wren) so it builds and tests standalone - see src/Engine/Jobs/CMakeLists.txt and
// docs/JobSystem.md.
namespace Struktur::Jobs
{
enum class Lane : uint8_t
{
	FRAME      = 0,
	POST_FRAME = 1,
	BACKGROUND = 2,
};

constexpr int LANE_COUNT = 3;

const char* ToString(Lane lane);

// What a stepped job returns: CONTINUE queues it again (at the back of its lane, so other work - and any higher-lane
// work - gets a turn first), DONE completes it. Long background work written as steps gives way between steps.
enum class StepResult : uint8_t
{
	CONTINUE,
	DONE,
};

template <typename T>
class Task;

// A callable stored inline when it is small enough (most lambdas), on the heap otherwise. Move-only. Calling it
// returns true when it is finished: always for ordinary work, and once it returns StepResult::DONE for stepped work.
class JobFunction
{
public:
	static constexpr size_t INLINE_SIZE = 64;

	JobFunction() = default;
	~JobFunction() { Reset(); }
	JobFunction(const JobFunction&)            = delete;
	JobFunction& operator=(const JobFunction&) = delete;

	template <typename Callable>
	void Set(Callable&& callable)
	{
		using Target = std::decay_t<Callable>;
		Reset();
		if constexpr (std::is_same_v<Target, std::function<void()>>)
		{
			if (!callable)
			{
				return;  // an empty std::function is "no work"
			}
		}
		if constexpr (sizeof(Target) <= INLINE_SIZE && alignof(Target) <= alignof(std::max_align_t))
		{
			m_target  = new (m_storage) Target(std::forward<Callable>(callable));
			m_destroy = &DestroyInline<Target>;
		}
		else
		{
			m_target  = new Target(std::forward<Callable>(callable));
			m_destroy = &DestroyHeap<Target>;
		}
		m_invoke = &Invoke<Target>;
	}

	bool IsSet() const { return m_invoke != nullptr; }
	bool IsInline() const { return m_target == static_cast<const void*>(m_storage); }
	bool operator()() { return m_invoke(m_target); }

	void Reset()
	{
		if (m_destroy != nullptr)
		{
			m_destroy(m_target);
		}
		m_target  = nullptr;
		m_invoke  = nullptr;
		m_destroy = nullptr;
	}

private:
	template <typename Target>
	static bool Invoke(void* target)
	{
		if constexpr (std::is_same_v<std::invoke_result_t<Target&>, StepResult>)
		{
			return (*static_cast<Target*>(target))() == StepResult::DONE;
		}
		else
		{
			(*static_cast<Target*>(target))();
			return true;
		}
	}
	template <typename Target>
	static void DestroyInline(void* target)
	{
		static_cast<Target*>(target)->~Target();
	}
	template <typename Target>
	static void DestroyHeap(void* target)
	{
		delete static_cast<Target*>(target);
	}

	alignas(std::max_align_t) unsigned char m_storage[INLINE_SIZE];
	void* m_target              = nullptr;
	bool (*m_invoke)(void*)     = nullptr;
	void (*m_destroy)(void*)    = nullptr;
};

class JobSystem;

class JobHandle
{
public:
	JobHandle() = default;

	bool IsValid() const;
	// True once the job has run (or was dropped by Shutdown). An invalid handle counts as complete.
	bool IsComplete() const;
	// True when the job never ran because the job system shut down first.
	bool WasDropped() const;
	Lane GetLane() const;

private:
	friend class JobSystem;
	JobHandle(JobSystem* system, uint32_t index, uint32_t generation, Lane lane);

	JobSystem* m_system   = nullptr;
	uint32_t m_index      = 0;
	uint32_t m_generation = 0;
	Lane m_lane           = Lane::FRAME;
};

struct JobSystemConfig
{
	// -1 = one per logical core, minus one for the main thread; 0 = no worker threads (inline mode).
	int workerCount = -1;
	// How many workers may run background jobs at once. -1 = automatic: all but two (all but one with 2-3 workers,
	// all with one), so frame work always has a free worker. Threads helping inside Wait() / RunPending() don't count.
	int backgroundWorkerLimit = -1;
};

struct JobSystemStats
{
	int workerCount           = 0;
	int backgroundWorkerLimit = 0;
	uint64_t jobsScheduled    = 0;
	uint64_t jobsRun          = 0;  // by any thread; a stepped job counts once, when it is done
	uint64_t stepsRequeued    = 0;  // steps that returned CONTINUE and went back in the queue
	uint64_t mainThreadRun    = 0;  // follow-ups run by RunMainThreadWork()
	uint64_t jobsRunByHelper  = 0;  // by a thread inside Wait() / RunPending() rather than a worker's own loop
	uint64_t jobsStolen       = 0;  // taken from another worker's queue
	uint64_t jobsDropped      = 0;
	uint64_t heapWork         = 0;  // jobs whose work was too big to store inline
	uint32_t poolSlots        = 0;  // job slots allocated so far (high-water mark)
};

class JobSystem
{
public:
	explicit JobSystem(const JobSystemConfig& config = JobSystemConfig());
	~JobSystem();

	JobSystem(const JobSystem&)            = delete;
	JobSystem& operator=(const JobSystem&) = delete;

	// Whether this build can start worker threads at all (STRUKTUR_THREADS).
	static bool ThreadsAvailable();
	// The worker count a config resolves to on this machine.
	static int ResolveWorkerCount(int requested);
	// The background limit a config resolves to for `workers` workers.
	static int ResolveBackgroundLimit(int requested, int workers);

	int GetWorkerCount() const;
	int GetBackgroundWorkerLimit() const;
	bool IsWorkerThread() const;

	template <typename Work>
	JobHandle Schedule(Work&& work, Lane lane = Lane::FRAME)
	{
		return ScheduleWith(std::forward<Work>(work), lane, nullptr, 0);
	}
	template <typename Work>
	JobHandle Schedule(Work&& work, Lane lane, const JobHandle& dependency)
	{
		return ScheduleWith(std::forward<Work>(work), lane, &dependency, 1);
	}
	template <typename Work>
	JobHandle Schedule(Work&& work, Lane lane, const std::vector<JobHandle>& dependencies)
	{
		return ScheduleWith(std::forward<Work>(work), lane, dependencies.data(), dependencies.size());
	}
	// A job that completes when every handle has; already complete for an empty list.
	JobHandle WhenAll(const std::vector<JobHandle>& handles, Lane lane = Lane::FRAME);

	// Splits [begin, end) into pieces of `grain` items and runs body(first, last) for each piece as its own job.
	// Returns a handle that completes when every piece has.
	template <typename Body>
	JobHandle ParallelFor(size_t begin, size_t end, size_t grain, Body body, Lane lane = Lane::FRAME);

	// Tasks (Task.h): a job with a result, progress and cancellation. `work` is T() or T(TaskContext&); work that
	// returns StepResult is stepped and gives a Task<void>. Background lane by default.
	template <typename Work>
	auto Run(Work&& work, Lane lane = Lane::BACKGROUND);
	template <typename Work>
	auto Run(Work&& work, Lane lane, const JobHandle& dependency);
	template <typename Work>
	auto Run(Work&& work, Lane lane, const std::vector<JobHandle>& dependencies);

	// Queues `callable` for the main thread: it runs inside the next RunMainThreadWork(). Task::ThenOnMainThread()
	// uses this to hand results back to code that may only run on the main thread (Wren, entt, bgfx resources).
	template <typename Callable>
	void RunOnMainThread(Callable&& callable)
	{
		std::unique_ptr<JobFunction> work = std::make_unique<JobFunction>();
		work->Set(std::forward<Callable>(callable));
		PostToMainThread(std::move(work));
	}

	// Called once a frame by the main thread: runs queued main-thread follow-ups, oldest first, until none are left or
	// `budgetSeconds` has passed (checked between them). With zero workers it then also runs queued jobs (all lanes)
	// in what is left of the budget, and any follow-ups they made ready - so tasks keep progressing without threads.
	// Returns how many follow-ups and jobs ran.
	size_t RunMainThreadWork(double budgetSeconds);
	size_t GetMainThreadQueuedCount() const;

	// Runs queued jobs on this thread until `handle` is complete.
	void Wait(const JobHandle& handle);
	void WaitAll(const std::vector<JobHandle>& handles);
	template <typename T>
	void Wait(const Task<T>& task)
	{
		Wait(task.GetHandle());
	}

	// Runs queued jobs on the calling thread, highest lane first, down to `lowestLane`, until the queues are empty or
	// `budgetSeconds` has passed (checked between jobs). Returns how many ran. This is how jobs progress with zero
	// workers; with workers it lets the main thread lend a hand.
	size_t RunPending(double budgetSeconds, Lane lowestLane = Lane::BACKGROUND);
	// Runs one queued job from lanes up to `lowestLane`, if there is one.
	bool TryRunOne(Lane lowestLane = Lane::BACKGROUND);

	size_t GetQueuedCount() const;
	JobSystemStats GetStats() const;

	// Stops the workers (jobs already running finish), then drops every queued job: it completes without running and
	// reports WasDropped(), so nothing waiting on it hangs. Called by the destructor.
	void Shutdown();

private:
	friend class JobHandle;
	friend struct TaskStateBase;

	// A tiny lock for the short critical sections around one slot or one queue.
	class SpinLock
	{
	public:
		void lock();
		void unlock() { m_flag.clear(std::memory_order_release); }

	private:
		std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
	};

	static constexpr uint64_t DONE_BIT    = 1;
	static constexpr uint64_t DROPPED_BIT = 2;
	static constexpr int GENERATION_SHIFT = 2;
	static constexpr int INLINE_DEPENDENTS = 4;

	struct JobSlot
	{
		JobFunction work;
		// generation << 2 | dropped | done. The generation goes up each time the slot is reused, so a handle to an
		// older job sees a different generation and knows its job finished long ago.
		std::atomic<uint64_t> state{DONE_BIT};
		std::atomic<int> pendingDependencies{0};
		Lane lane = Lane::FRAME;
		SpinLock lock;  // guards the dependents and the done transition
		uint32_t dependents[INLINE_DEPENDENTS];
		uint8_t dependentCount = 0;
		std::vector<uint32_t> moreDependents;
	};

	struct WorkerQueue
	{
		SpinLock lock;
		std::deque<uint32_t> lanes[LANE_COUNT];
	};

	struct SlotCache
	{
		std::mutex mutex;  // only the shared external cache needs it; a worker's own cache is touched by that worker
		std::vector<uint32_t> free;
	};

	// One piece of a parallel-for. The body belongs to the join job, which runs after every piece.
	template <typename Body>
	struct ParallelForPart
	{
		Body* body;
		size_t first;
		size_t last;
		void operator()() const { (*body)(first, last); }
	};

	// Runs once every piece has, releasing the body (as it does if the job is dropped at shutdown).
	template <typename Body>
	struct ParallelForJoin
	{
		std::unique_ptr<Body> body;
		void operator()() { body.reset(); }
	};

	template <typename Work>
	JobHandle ScheduleWith(Work&& work, Lane lane, const JobHandle* dependencies, size_t count)
	{
		const uint32_t index = AllocateSlot();
		JobSlot& slot        = Slot(index);
		slot.work.Set(std::forward<Work>(work));
		if (slot.work.IsSet() && !slot.work.IsInline())
		{
			m_heapWork.fetch_add(1, std::memory_order_relaxed);
		}
		return Submit(index, lane, dependencies, count);
	}

	template <typename Work>
	auto RunWith(Work&& work, Lane lane, const JobHandle* dependencies, size_t count);

	void PostToMainThread(std::unique_ptr<JobFunction> work);
	size_t RunFollowUps(std::chrono::steady_clock::time_point start, double budgetSeconds);

	JobSlot& Slot(uint32_t index) const;
	uint32_t AllocateSlot();
	void FreeSlot(uint32_t index);
	void GrowPool();  // m_poolMutex held
	JobHandle Submit(uint32_t index, Lane lane, const JobHandle* dependencies, size_t count);
	void Enqueue(uint32_t index, bool requeue = false);
	bool PopJob(int self, Lane lowestLane, bool allowBackground, uint32_t& index);
	bool PopFromQueue(int queue, int lane, bool newest, uint32_t& index);
	void Execute(uint32_t index, bool helper);
	void Finish(uint32_t index, bool dropped);
	void WorkerLoop(int workerIndex);
	bool WorkerHasWork() const;
	void WakeOne();
	void DrainDropped();
	int CurrentWorker() const;  // -1 when the calling thread isn't one of this system's workers
	int QueueCount() const;

	int m_workerCount;
	int m_backgroundLimit;

	// Pool: chunks of slots, allocated as needed and kept until destruction.
	static constexpr uint32_t CHUNK_BITS  = 12;
	static constexpr uint32_t CHUNK_SIZE  = 1u << CHUNK_BITS;
	static constexpr uint32_t MAX_CHUNKS  = 2048;  // 8 M jobs alive at once
	std::unique_ptr<JobSlot[]> m_chunks[MAX_CHUNKS];
	std::atomic<uint32_t> m_chunkCount{0};
	std::mutex m_poolMutex;
	std::vector<uint32_t> m_globalFree;
	std::vector<std::unique_ptr<SlotCache>> m_caches;  // one per worker + one shared by every other thread

	std::vector<std::unique_ptr<WorkerQueue>> m_queues;  // one per worker (one in inline mode)
	std::atomic<uint32_t> m_nextQueue{0};
	std::atomic<int> m_queued[LANE_COUNT];
	std::atomic<int> m_backgroundRunning{0};

	std::atomic<bool> m_stopping{false};
	std::atomic<bool> m_shutDown{false};
	std::atomic<bool> m_draining{false};
#if STRUKTUR_JOBS_THREADS
	std::vector<std::thread> m_workers;
#endif
	std::mutex m_wakeMutex;
	std::condition_variable m_wake;
	std::atomic<int> m_sleepers{0};

	std::mutex m_completionMutex;
	std::condition_variable m_jobCompleted;
	std::atomic<int> m_waiters{0};  // threads sleeping in Wait() - completions only notify when there are any

	std::atomic<uint64_t> m_jobsScheduled{0};
	mutable std::mutex m_mainThreadMutex;
	std::deque<std::unique_ptr<JobFunction>> m_mainThreadQueue;

	std::atomic<uint64_t> m_jobsRun{0};
	std::atomic<uint64_t> m_stepsRequeued{0};
	std::atomic<uint64_t> m_mainThreadRun{0};
	std::atomic<uint64_t> m_jobsRunByHelper{0};
	std::atomic<uint64_t> m_jobsStolen{0};
	std::atomic<uint64_t> m_jobsDropped{0};
	std::atomic<uint64_t> m_heapWork{0};
};

template <typename Body>
JobHandle JobSystem::ParallelFor(size_t begin, size_t end, size_t grain, Body body, Lane lane)
{
	if (end <= begin)
	{
		return WhenAll({}, lane);
	}
	grain                       = std::max<size_t>(1, grain);
	std::unique_ptr<Body> owned = std::make_unique<Body>(std::move(body));
	std::vector<JobHandle> parts;
	parts.reserve((end - begin + grain - 1) / grain);
	for (size_t first = begin; first < end; first += grain)
	{
		parts.push_back(Schedule(ParallelForPart<Body>{owned.get(), first, std::min(end, first + grain)}, lane));
	}
	return Schedule(ParallelForJoin<Body>{std::move(owned)}, lane, parts);
}
}  // namespace Struktur::Jobs

#include "Engine/Jobs/Task.h"
