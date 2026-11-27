#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#if STRUKTUR_JOBS_THREADS
	#include <thread>
#endif

// The engine's job system: a pool of worker threads running small jobs, with dependencies between them.
//
//  - Schedule() queues a job and returns a JobHandle; a job can depend on other jobs and only starts once they are
//    all complete. JobHandle::IsComplete() can be polled from anywhere (e.g. the update loop).
//  - Lanes set priority: FRAME work (this frame's systems, parallel-for) first, then POST_FRAME (kicked after the
//    frame is submitted), then BACKGROUND (long work that may span many frames).
//  - Wait() never just blocks: the waiting thread runs queued jobs until the one it needs is done. The main thread
//    waiting on frame work only helps with frame-lane jobs, so it never picks up a long background job mid-frame.
//  - Zero workers is a first-class mode (web builds without threads, debugging, STRUKTUR_THREADS=OFF): the same API,
//    jobs simply run on the calling thread inside Wait() / RunPending(). Nothing runs until someone waits or pumps.
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

class JobSystem;

// Shared by a job and every handle to it.
struct JobState
{
	std::function<void()> work;
	Lane lane = Lane::FRAME;
	// Dependencies still running, plus one guard held while the job is being set up.
	std::atomic<int> pendingDependencies{1};
	std::atomic<bool> done{false};
	std::atomic<bool> dropped{false};  // finished without running (system shut down first)
	std::mutex mutex;                  // guards `dependents` against the job finishing while one is added
	std::vector<std::shared_ptr<JobState>> dependents;
};

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
	explicit JobHandle(std::shared_ptr<JobState> state);

	std::shared_ptr<JobState> m_state;
};

struct JobSystemConfig
{
	// -1 = one per logical core, minus one for the main thread; 0 = no worker threads (inline mode).
	int workerCount = -1;
};

struct JobSystemStats
{
	int workerCount          = 0;
	uint64_t jobsScheduled   = 0;
	uint64_t jobsRun         = 0;  // by any thread
	uint64_t jobsRunByHelper = 0;  // by a thread inside Wait() / RunPending() rather than a worker's own loop
	uint64_t jobsDropped     = 0;
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

	int GetWorkerCount() const;
	bool IsWorkerThread() const;

	JobHandle Schedule(std::function<void()> work, Lane lane = Lane::FRAME);
	JobHandle Schedule(std::function<void()> work, Lane lane, const JobHandle& dependency);
	JobHandle Schedule(std::function<void()> work, Lane lane, const std::vector<JobHandle>& dependencies);
	// A job that completes when every handle has; already complete for an empty list.
	JobHandle WhenAll(const std::vector<JobHandle>& handles, Lane lane = Lane::FRAME);

	// Splits [begin, end) into pieces of `grain` items and runs body(first, last) for each piece as its own job.
	// Returns a handle that completes when every piece has.
	template <typename Body>
	JobHandle ParallelFor(size_t begin, size_t end, size_t grain, Body body, Lane lane = Lane::FRAME);

	// Runs queued jobs on this thread until `handle` is complete.
	void Wait(const JobHandle& handle);
	void WaitAll(const std::vector<JobHandle>& handles);

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
	template <typename Body>
	struct ParallelForPart
	{
		std::shared_ptr<Body> body;
		size_t first;
		size_t last;
		void operator()() const { (*body)(first, last); }
	};

	void Enqueue(std::shared_ptr<JobState> job);
	bool PopJob(Lane lowestLane, std::shared_ptr<JobState>& job);
	void Execute(const std::shared_ptr<JobState>& job, bool helper);
	void Finish(const std::shared_ptr<JobState>& job);
	void WorkerLoop(int workerIndex);
	bool AnyQueuedLocked() const;  // m_queueMutex held
	void DrainDropped();

	int m_workerCount;
	mutable std::mutex m_queueMutex;
	std::condition_variable m_workAvailable;
	std::deque<std::shared_ptr<JobState>> m_queues[LANE_COUNT];
	bool m_stopping;
	bool m_shutDown;
	std::atomic<bool> m_draining;
	std::atomic<int> m_waiters{0};  // threads sleeping in Wait() - completions only notify when there are any
#if STRUKTUR_JOBS_THREADS
	std::vector<std::thread> m_workers;
#endif
	std::mutex m_completionMutex;
	std::condition_variable m_jobCompleted;

	std::atomic<uint64_t> m_jobsScheduled{0};
	std::atomic<uint64_t> m_jobsRun{0};
	std::atomic<uint64_t> m_jobsRunByHelper{0};
	std::atomic<uint64_t> m_jobsDropped{0};
};

template <typename Body>
JobHandle JobSystem::ParallelFor(size_t begin, size_t end, size_t grain, Body body, Lane lane)
{
	if (end <= begin)
	{
		return WhenAll({}, lane);
	}
	grain                       = std::max<size_t>(1, grain);
	std::shared_ptr<Body> shared = std::make_shared<Body>(std::move(body));
	std::vector<JobHandle> parts;
	parts.reserve((end - begin + grain - 1) / grain);
	for (size_t first = begin; first < end; first += grain)
	{
		parts.push_back(Schedule(ParallelForPart<Body>{shared, first, std::min(end, first + grain)}, lane));
	}
	return WhenAll(parts, lane);
}
}  // namespace Struktur::Jobs
