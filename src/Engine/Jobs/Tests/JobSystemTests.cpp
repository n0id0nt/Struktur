// Job system correctness. Every scenario runs with zero workers (inline mode), one worker and several, and must
// behave the same apart from timing.
#include <doctest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "Engine/Jobs/JobSystem.h"
#include "TestHelpers.h"

using namespace Struktur::Jobs;

TEST_CASE("worker count resolves to cores minus one, or what was asked")
{
	CHECK(JobSystem::ResolveWorkerCount(0) == 0);
	if (JobSystem::ThreadsAvailable())
	{
		CHECK(JobSystem::ResolveWorkerCount(3) == 3);
		const int cores = static_cast<int>(std::thread::hardware_concurrency());
		CHECK(JobSystem::ResolveWorkerCount(-1) == std::max(0, cores - 1));
	}
	else
	{
		CHECK(JobSystem::ResolveWorkerCount(8) == 0);
	}
	JobSystem jobs(JobSystemConfig{2});
	CHECK(jobs.GetWorkerCount() == JobSystem::ResolveWorkerCount(2));
}

TEST_CASE("every job runs exactly once")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		const int count = 20000;
		std::vector<std::atomic<int>> runs(count);
		std::vector<JobHandle> handles;
		handles.reserve(count);
		for (int index = 0; index < count; ++index)
		{
			handles.push_back(jobs.Schedule([&runs, index] { runs[index].fetch_add(1); }));
		}
		jobs.Wait(jobs.WhenAll(handles));
		int wrong = 0;
		for (const std::atomic<int>& run : runs)
		{
			wrong += run.load() == 1 ? 0 : 1;
		}
		CHECK(wrong == 0);
		for (const JobHandle& handle : handles)
		{
			REQUIRE(handle.IsComplete());
		}
		const JobSystemStats stats = jobs.GetStats();
		CHECK(stats.jobsScheduled == count + 1);  // + the WhenAll
		CHECK(stats.jobsRun == count + 1);
		if (workers == 0)
		{
			CHECK(stats.jobsRunByHelper == count);  // every job ran on the waiting thread; the WhenAll completed itself
		}
	}
}

TEST_CASE("a dependency chain runs in order")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::vector<int> order;
		std::mutex mutex;
		JobHandle previous;
		for (int index = 0; index < 2000; ++index)
		{
			previous = jobs.Schedule(
			    [&order, &mutex, index]
			    {
				    std::lock_guard<std::mutex> lock(mutex);
				    order.push_back(index);
			    },
			    Lane::FRAME, previous);
		}
		jobs.Wait(previous);
		REQUIRE(order.size() == 2000);
		bool sorted = true;
		for (size_t index = 0; index < order.size(); ++index)
		{
			sorted = sorted && order[index] == static_cast<int>(index);
		}
		CHECK(sorted);
	}
}

TEST_CASE("a random dependency graph never starts a job before its dependencies finish")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		const int count = 20000;
		std::atomic<int64_t> ticket{0};
		std::vector<int64_t> started(count, -1);
		std::vector<int64_t> finished(count, -1);
		std::vector<std::vector<int>> dependencies(count);
		std::vector<JobHandle> handles(count);
		std::mt19937 random(1234 + workers);
		for (int index = 0; index < count; ++index)
		{
			std::vector<JobHandle> waitFor;
			const int dependencyCount = index == 0 ? 0 : static_cast<int>(random() % 4);
			for (int dependency = 0; dependency < dependencyCount; ++dependency)
			{
				const int other = static_cast<int>(random() % static_cast<uint32_t>(index));
				dependencies[index].push_back(other);
				waitFor.push_back(handles[other]);
			}
			const Lane lane = static_cast<Lane>(random() % LANE_COUNT);
			handles[index]  = jobs.Schedule(
                [&ticket, &started, &finished, index]
                {
                    started[index]  = ticket.fetch_add(1);
                    finished[index] = ticket.fetch_add(1);
                },
                lane, waitFor);
		}
		jobs.WaitAll(handles);
		int violations = 0;
		int notRun     = 0;
		for (int index = 0; index < count; ++index)
		{
			notRun += started[index] < 0 ? 1 : 0;
			for (int dependency : dependencies[index])
			{
				violations += finished[dependency] < started[index] ? 0 : 1;
			}
		}
		CHECK(notRun == 0);
		CHECK(violations == 0);
	}
}

TEST_CASE("waiting from inside a job helps instead of deadlocking")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<int> innerRuns{0};
		std::vector<JobHandle> outer;
		for (int index = 0; index < 64; ++index)
		{
			outer.push_back(jobs.Schedule(
			    [&jobs, &innerRuns]
			    {
				    std::vector<JobHandle> inner;
				    for (int part = 0; part < 16; ++part)
				    {
					    inner.push_back(jobs.Schedule([&innerRuns] { innerRuns.fetch_add(1); }));
				    }
				    jobs.Wait(jobs.WhenAll(inner));
			    }));
		}
		jobs.WaitAll(outer);
		CHECK(innerRuns.load() == 64 * 16);
	}
}

TEST_CASE("parallel-for matches a plain loop")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		const size_t count = 1000000;
		const size_t grain = 4096;
		std::vector<uint64_t> partial((count + grain - 1) / grain, 0);
		JobHandle handle = jobs.ParallelFor(0, count, grain,
		                                    [&partial, grain](size_t first, size_t last)
		                                    {
			                                    uint64_t sum = 0;
			                                    for (size_t value = first; value < last; ++value)
			                                    {
				                                    sum += static_cast<uint64_t>(value) * value;
			                                    }
			                                    partial[first / grain] = sum;
		                                    });
		jobs.Wait(handle);
		uint64_t total = 0;
		for (uint64_t sum : partial)
		{
			total += sum;
		}
		uint64_t expected = 0;
		for (size_t value = 0; value < count; ++value)
		{
			expected += static_cast<uint64_t>(value) * value;
		}
		CHECK(total == expected);
		// Empty and single-item ranges.
		CHECK(jobs.ParallelFor(5, 5, 16, [](size_t, size_t) {}).IsComplete());
		std::atomic<int> single{0};
		jobs.Wait(jobs.ParallelFor(7, 8, 16, [&single](size_t first, size_t last) { single += static_cast<int>(last - first); }));
		CHECK(single.load() == 1);
	}
}

TEST_CASE("inline mode: jobs run only when someone waits or pumps, within the budget")
{
	JobSystem jobs(JobSystemConfig{0});
	CHECK(jobs.GetWorkerCount() == 0);
	std::atomic<int> runs{0};
	JobHandle handle = jobs.Schedule([&runs] { runs.fetch_add(1); });
	CHECK_FALSE(handle.IsComplete());
	CHECK(jobs.GetQueuedCount() == 1);
	CHECK(jobs.RunPending(1.0) == 1);
	CHECK(handle.IsComplete());

	// A 10 ms budget over 2 ms jobs runs a handful, not all of them (checked between jobs, so it can overshoot one).
	std::vector<JobHandle> slow;
	for (int index = 0; index < 50; ++index)
	{
		slow.push_back(jobs.Schedule([] { BusyMilliseconds(2.0); }, Lane::BACKGROUND));
	}
	const size_t ran = jobs.RunPending(0.010);
	CHECK(ran >= 2);
	CHECK(ran <= 12);
	CHECK(jobs.GetQueuedCount() == 50 - ran);
	jobs.WaitAll(slow);
	CHECK(jobs.GetQueuedCount() == 0);
}

TEST_CASE("a very long chain doesn't overflow the stack in inline mode or on shutdown")
{
	{
		JobSystem jobs(JobSystemConfig{0});
		JobHandle previous;
		std::atomic<int> runs{0};
		for (int index = 0; index < 200000; ++index)
		{
			previous = jobs.Schedule([&runs] { runs.fetch_add(1); }, Lane::FRAME, previous);
		}
		jobs.Wait(previous);
		CHECK(runs.load() == 200000);
	}
	{
		JobSystem jobs(JobSystemConfig{0});
		JobHandle previous;
		for (int index = 0; index < 200000; ++index)
		{
			previous = jobs.Schedule([] {}, Lane::FRAME, previous);
		}
		jobs.Shutdown();
		CHECK(previous.IsComplete());
		CHECK(previous.WasDropped());
	}
}

TEST_CASE("frame work runs before post-frame and background work")
{
	for (int workers : WorkerCounts())
	{
		if (workers > 1)
		{
			continue;  // ordering is only observable with one runner
		}
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<bool> release{false};
		std::atomic<bool> gateRunning{false};
		JobHandle gate;
		if (workers == 1)
		{
			// Occupy the only worker while the queues fill.
			gate = jobs.Schedule(
			    [&release, &gateRunning]
			    {
				    gateRunning = true;
				    while (!release.load())
				    {
					    std::this_thread::yield();
				    }
			    },
			    Lane::FRAME);
			while (!gateRunning.load())
			{
				std::this_thread::yield();
			}
		}
		std::vector<Lane> order;
		std::mutex mutex;
		std::vector<JobHandle> handles;
		const Lane lanes[] = {Lane::BACKGROUND, Lane::POST_FRAME, Lane::FRAME};
		for (Lane lane : lanes)
		{
			for (int index = 0; index < 5; ++index)
			{
				handles.push_back(jobs.Schedule(
				    [&order, &mutex, lane]
				    {
					    std::lock_guard<std::mutex> lock(mutex);
					    order.push_back(lane);
				    },
				    lane));
			}
		}
		release = true;
		if (workers == 0)
		{
			jobs.RunPending(10.0);
		}
		// Poll rather than Wait(): a waiting thread helps run jobs, and two runners would interleave the lanes.
		for (const JobHandle& handle : handles)
		{
			while (!handle.IsComplete())
			{
				std::this_thread::yield();
			}
		}
		REQUIRE(order.size() == 15);
		for (size_t index = 0; index < order.size(); ++index)
		{
			const Lane expected = index < 5 ? Lane::FRAME : (index < 10 ? Lane::POST_FRAME : Lane::BACKGROUND);
			CHECK(order[index] == expected);
		}
	}
}

TEST_CASE("the main thread waiting on frame work never picks up background work")
{
	if (!JobSystem::ThreadsAvailable())
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{1});
	std::atomic<bool> release{false};
	std::atomic<bool> gateRunning{false};
	JobHandle gate = jobs.Schedule(
	    [&release, &gateRunning]
	    {
		    gateRunning = true;
		    while (!release.load())
		    {
			    std::this_thread::yield();
		    }
	    },
	    Lane::BACKGROUND);
	while (!gateRunning.load())
	{
		std::this_thread::yield();
	}
	const std::thread::id mainThread = std::this_thread::get_id();
	std::atomic<bool> backgroundOnMain{false};
	JobHandle background = jobs.Schedule(
	    [&backgroundOnMain, mainThread] { backgroundOnMain = std::this_thread::get_id() == mainThread; },
	    Lane::BACKGROUND);
	std::atomic<bool> frameOnMain{false};
	JobHandle frame = jobs.Schedule([&frameOnMain, mainThread] { frameOnMain = std::this_thread::get_id() == mainThread; },
	                                Lane::FRAME);
	jobs.Wait(frame);  // the worker is busy: the main thread runs the frame job itself
	CHECK(frameOnMain.load());
	CHECK_FALSE(background.IsComplete());
	release = true;
	// Don't Wait() on the background job here: a thread waiting on background work may run it itself, by design.
	while (!background.IsComplete())
	{
		std::this_thread::yield();
	}
	jobs.Wait(gate);
	CHECK_FALSE(backgroundOnMain.load());
}

TEST_CASE("WhenAll and handles")
{
	JobSystem jobs(JobSystemConfig{0});
	CHECK(jobs.WhenAll({}).IsComplete());
	JobHandle invalid;
	CHECK_FALSE(invalid.IsValid());
	CHECK(invalid.IsComplete());
	CHECK_FALSE(invalid.WasDropped());
	jobs.Wait(invalid);  // no-op
	JobHandle background = jobs.Schedule([] {}, Lane::BACKGROUND);
	CHECK(background.GetLane() == Lane::BACKGROUND);
	JobHandle all = jobs.WhenAll({background, invalid});
	jobs.Wait(all);
	CHECK(background.IsComplete());
	CHECK(all.IsComplete());
}

TEST_CASE("shutdown finishes running jobs and drops queued ones without hanging waiters")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<bool> firstRan{false};
		std::atomic<bool> started{false};
		JobHandle first = jobs.Schedule(
		    [&firstRan, &started]
		    {
			    started = true;
			    SleepMilliseconds(30);
			    firstRan = true;
		    });
		if (workers > 0)
		{
			while (!started.load())
			{
				std::this_thread::yield();
			}
		}
		std::atomic<int> dependentsRan{0};
		std::vector<JobHandle> dependents;
		for (int index = 0; index < 1000; ++index)
		{
			dependents.push_back(jobs.Schedule([&dependentsRan] { dependentsRan.fetch_add(1); }, Lane::FRAME, first));
		}
		jobs.Shutdown();
		CHECK(first.IsComplete());
		CHECK(firstRan.load() == (workers > 0));  // inline: nobody ran it before shutdown
		int dropped = 0;
		for (const JobHandle& dependent : dependents)
		{
			REQUIRE(dependent.IsComplete());
			dropped += dependent.WasDropped() ? 1 : 0;
		}
		CHECK(dependentsRan.load() == 0);
		CHECK(dropped == 1000);
		// Scheduling after shutdown completes (dropped) at once.
		JobHandle late = jobs.Schedule([&dependentsRan] { dependentsRan.fetch_add(1); });
		CHECK(late.IsComplete());
		CHECK(late.WasDropped());
		jobs.Wait(late);
		CHECK(dependentsRan.load() == 0);
	}
}

TEST_CASE("many threads scheduling at once")
{
	if (!JobSystem::ThreadsAvailable())
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{4});
	const int producers  = 8;
	const int perThread  = 10000;
	std::atomic<int> runs{0};
	std::vector<std::thread> threads;
	std::vector<std::vector<JobHandle>> handles(producers);
	for (int producer = 0; producer < producers; ++producer)
	{
		threads.emplace_back(
		    [&jobs, &runs, &handles, producer, perThread]
		    {
			    handles[producer].reserve(perThread);
			    for (int index = 0; index < perThread; ++index)
			    {
				    handles[producer].push_back(jobs.Schedule([&runs] { runs.fetch_add(1); }));
			    }
			    jobs.WaitAll(handles[producer]);
		    });
	}
	for (std::thread& thread : threads)
	{
		thread.join();
	}
	CHECK(runs.load() == producers * perThread);
}

// --- phase 1b: pooled slots, inline work, background limit, stealing ------------------------------------------------

TEST_CASE("recycled job slots never make an old handle look unfinished")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		JobHandle first = jobs.Schedule([] {});
		jobs.Wait(first);
		REQUIRE(first.IsComplete());
		// Churn through far more jobs than one cache holds, so the first job's slot is reused.
		for (int round = 0; round < 20; ++round)
		{
			std::vector<JobHandle> batch;
			for (int index = 0; index < 1000; ++index)
			{
				batch.push_back(jobs.Schedule([] {}));
			}
			jobs.WaitAll(batch);
			CHECK(first.IsComplete());
			CHECK_FALSE(first.WasDropped());
		}
		// A long-running job keeps its slot: its handle stays incomplete while newer jobs come and go.
		std::atomic<bool> release{false};
		std::atomic<bool> heldStarted{false};
		JobHandle held = jobs.Schedule([&release, &heldStarted] {
			heldStarted = true;
			while (!release.load())
			{
				std::this_thread::yield();
			}
		});
		if (workers > 0)
		{
			// Let a worker take it first, or this thread's own Wait() below could pick it up and spin forever.
			while (!heldStarted.load())
			{
				std::this_thread::yield();
			}
			for (int index = 0; index < 2000; ++index)
			{
				jobs.Wait(jobs.Schedule([] {}));
			}
			CHECK_FALSE(held.IsComplete());
		}
		release = true;
		jobs.Wait(held);
		CHECK(held.IsComplete());
	}
}

TEST_CASE("the job pool grows when many jobs are alive at once")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		const uint32_t initial = jobs.GetStats().poolSlots;
		std::atomic<bool> release{false};
		JobHandle gate = jobs.Schedule([&release] {
			while (!release.load())
			{
				std::this_thread::yield();
			}
		});
		std::atomic<int> runs{0};
		std::vector<JobHandle> blocked;
		for (int index = 0; index < 30000; ++index)
		{
			blocked.push_back(jobs.Schedule([&runs] { runs.fetch_add(1); }, Lane::FRAME, gate));
		}
		CHECK(jobs.GetStats().poolSlots > initial);
		release = true;
		jobs.WaitAll(blocked);
		CHECK(runs.load() == 30000);
	}
}

struct Tracker
{
	static std::atomic<int> alive;
	static std::atomic<int> calls;
	uint8_t padding[200];  // bigger than JobFunction::INLINE_SIZE: stored on the heap
	Tracker() { alive.fetch_add(1); }
	Tracker(const Tracker&) { alive.fetch_add(1); }
	Tracker(Tracker&&) noexcept { alive.fetch_add(1); }
	~Tracker() { alive.fetch_sub(1); }
	void operator()() const { calls.fetch_add(1); }
};
std::atomic<int> Tracker::alive{0};
std::atomic<int> Tracker::calls{0};

struct SmallTracker
{
	static std::atomic<int> alive;
	SmallTracker() { alive.fetch_add(1); }
	SmallTracker(const SmallTracker&) { alive.fetch_add(1); }
	SmallTracker(SmallTracker&&) noexcept { alive.fetch_add(1); }
	~SmallTracker() { alive.fetch_sub(1); }
	void operator()() const {}
};
std::atomic<int> SmallTracker::alive{0};

TEST_CASE("work of any size runs once and is destroyed once, run or dropped")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		Tracker::calls = 0;
		{
			JobSystem jobs(JobSystemConfig{workers});
			std::vector<JobHandle> handles;
			for (int index = 0; index < 500; ++index)
			{
				handles.push_back(jobs.Schedule(Tracker()));
				handles.push_back(jobs.Schedule(SmallTracker()));
			}
			jobs.WaitAll(handles);
			CHECK(Tracker::calls.load() == 500);
			CHECK(Tracker::alive.load() == 0);
			CHECK(SmallTracker::alive.load() == 0);
			const JobSystemStats stats = jobs.GetStats();
			CHECK(stats.heapWork == 500);  // only the big ones
			// Dropped work is destroyed too.
			std::atomic<bool> release{false};
			JobHandle gate = jobs.Schedule([&release] {
				while (!release.load())
				{
					std::this_thread::yield();
				}
			});
			for (int index = 0; index < 100; ++index)
			{
				jobs.Schedule(Tracker(), Lane::FRAME, gate);
			}
			CHECK(Tracker::alive.load() == 100);
			release = true;
			if (workers > 0)
			{
				jobs.Wait(gate);
			}
			jobs.Shutdown();
		}
		CHECK(Tracker::alive.load() == 0);
	}
}

TEST_CASE("background work never takes more than its limit of workers")
{
	CHECK(JobSystem::ResolveBackgroundLimit(-1, 0) == 0);
	CHECK(JobSystem::ResolveBackgroundLimit(-1, 1) == 1);
	CHECK(JobSystem::ResolveBackgroundLimit(-1, 3) == 2);
	CHECK(JobSystem::ResolveBackgroundLimit(-1, 8) == 6);
	CHECK(JobSystem::ResolveBackgroundLimit(3, 8) == 3);
	CHECK(JobSystem::ResolveBackgroundLimit(20, 8) == 8);
	if (!JobSystem::ThreadsAvailable())
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{4});
	CHECK(jobs.GetBackgroundWorkerLimit() == 2);
	std::atomic<int> running{0};
	std::atomic<int> most{0};
	std::vector<JobHandle> background;
	for (int index = 0; index < 8; ++index)
	{
		background.push_back(jobs.Schedule(
		    [&running, &most]
		    {
			    const int now = running.fetch_add(1) + 1;
			    int seen      = most.load();
			    while (now > seen && !most.compare_exchange_weak(seen, now))
			    {
			    }
			    BusyMilliseconds(15.0);
			    running.fetch_sub(1);
		    },
		    Lane::BACKGROUND));
	}
	BusyMilliseconds(5.0);
	// While background jobs hold two workers, a frame job starts at once on a free one (no waiting on them).
	const std::chrono::steady_clock::time_point scheduled = std::chrono::steady_clock::now();
	std::atomic<double> startedAfterMs{-1.0};
	JobHandle frame = jobs.Schedule(
	    [&startedAfterMs, scheduled]
	    { startedAfterMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - scheduled).count(); },
	    Lane::FRAME);
	while (!frame.IsComplete())
	{
		std::this_thread::yield();
	}
	CHECK(startedAfterMs.load() < 5.0);
	for (const JobHandle& handle : background)
	{
		while (!handle.IsComplete())
		{
			std::this_thread::yield();
		}
	}
	CHECK(most.load() <= 2);
	CHECK(most.load() >= 1);
}

TEST_CASE("idle workers steal jobs another worker spawned")
{
	if (!JobSystem::ThreadsAvailable())
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{4});
	std::atomic<int> runs{0};
	JobHandle spawner = jobs.Schedule(
	    [&jobs, &runs]
	    {
		    // Spawned from a worker: they go to its own queue; the others have to steal them.
		    std::vector<JobHandle> children;
		    for (int index = 0; index < 400; ++index)
		    {
			    children.push_back(jobs.Schedule(
			        [&runs]
			        {
				        BusyMilliseconds(0.05);
				        runs.fetch_add(1);
			        }));
		    }
		    jobs.Wait(jobs.WhenAll(children));
	    });
	while (!spawner.IsComplete())
	{
		std::this_thread::yield();
	}
	CHECK(runs.load() == 400);
	CHECK(jobs.GetStats().jobsStolen > 0);
}

// A move-only parallel-for body that counts how many of it are alive.
struct OwnedBody
{
	static inline std::atomic<int> alive{0};
	std::unique_ptr<std::atomic<int>*> items;
	explicit OwnedBody(std::atomic<int>& counter) : items(std::make_unique<std::atomic<int>*>(&counter)) { alive.fetch_add(1); }
	OwnedBody(OwnedBody&& other) noexcept : items(std::move(other.items)) { alive.fetch_add(1); }
	~OwnedBody() { alive.fetch_sub(1); }
	void operator()(size_t first, size_t last) { (*items)->fetch_add(static_cast<int>(last - first)); }
};

TEST_CASE("the parallel-for join owns the body and releases it once, run or dropped")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		std::atomic<int> done{0};
		{
			JobSystem jobs(JobSystemConfig{workers});
			JobHandle loop = jobs.ParallelFor(0, 10000, 64, OwnedBody(done));
			jobs.Wait(loop);
			CHECK(done.load() == 10000);
			CHECK(OwnedBody::alive.load() == 0);  // released as soon as the loop is complete

			// Still waiting behind a job that never finishes: dropped at shutdown, released all the same.
			JobHandle endless = jobs.Schedule([] { return StepResult::CONTINUE; }, Lane::BACKGROUND);
			JobHandle queued  = jobs.ParallelFor(0, 100, 10, OwnedBody(done), Lane::BACKGROUND);
			(void)endless;
			(void)queued;
			jobs.Shutdown();
		}
		CHECK(OwnedBody::alive.load() == 0);
	}
}
