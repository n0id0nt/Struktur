// Tasks, stepped jobs and main-thread follow-ups (phase 2). Like the job system tests, every scenario that doesn't need
// a second thread runs with zero workers too.
#include <doctest.h>

#include <array>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "Engine/Jobs/Task.h"
#include "TestHelpers.h"

using namespace Struktur::Jobs;

TEST_CASE("a task returns its result and reports success")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		Task<int> answer = jobs.Run([] { return 42; });
		Task<std::string> named = jobs.Run([](TaskContext&) { return std::string("streamed"); }, Lane::FRAME);
		std::atomic<int> sideEffect{0};
		Task<void> nothing = jobs.Run([&sideEffect] { sideEffect = 7; });
		CHECK(answer.IsValid());
		jobs.Wait(answer);
		jobs.Wait(named);
		jobs.Wait(nothing);
		CHECK(answer.Succeeded());
		CHECK(answer.IsComplete());
		CHECK(answer.GetResult() == 42);
		CHECK(*answer.TryGetResult() == 42);
		CHECK(answer.GetProgress() == 1.0f);
		CHECK(named.GetResult() == "streamed");
		CHECK(named.TakeResult() == "streamed");
		CHECK(nothing.Succeeded());
		CHECK(sideEffect.load() == 7);
		CHECK(std::string(ToString(answer.GetStatus())) == "succeeded");

		Task<int> invalid;
		CHECK_FALSE(invalid.IsValid());
		CHECK(invalid.IsComplete());
		CHECK(invalid.TryGetResult() == nullptr);
		invalid.Cancel();  // harmless
	}
}

TEST_CASE("the update loop can poll a task until it completes")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		Task<int> load = jobs.Run(
		    []
		    {
			    BusyMilliseconds(3.0);
			    return 5;
		    });
		if (workers == 0)
		{
			// Nothing runs on its own without workers.
			CHECK(load.GetStatus() == TaskStatus::PENDING);
		}
		const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		while (!load.IsComplete() && MillisecondsSince(start) < 2000.0)
		{
			jobs.RunMainThreadWork(0.001);  // what the frame loop will call (phase 3)
			std::this_thread::yield();
		}
		CHECK(load.Succeeded());
		CHECK(load.GetResult() == 5);
	}
}

TEST_CASE("a task waits for its dependencies and can read their results")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		Task<int> first = jobs.Run(
		    []
		    {
			    BusyMilliseconds(1.0);
			    return 20;
		    });
		Task<int> second = jobs.Run([first] { return first.GetResult() * 2 + 2; }, Lane::BACKGROUND, first.GetHandle());
		std::vector<Task<int>> parts;
		std::vector<JobHandle> handles;
		for (int index = 0; index < 100; ++index)
		{
			parts.push_back(jobs.Run([index] { return index; }, Lane::FRAME));
			handles.push_back(parts.back().GetHandle());
		}
		Task<int> sum = jobs.Run(
		    [parts]
		    {
			    int total = 0;
			    for (const Task<int>& part : parts)
			    {
				    total += part.GetResult();
			    }
			    return total;
		    },
		    Lane::FRAME, handles);
		jobs.Wait(second);
		jobs.Wait(sum);
		CHECK(second.GetResult() == 42);
		CHECK(sum.GetResult() == 4950);
	}
}

TEST_CASE("a task cancelled before it starts never runs its work")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<bool> release{false};
		JobHandle gate = jobs.Schedule(
		    [&release]
		    {
			    while (!release.load())
			    {
				    std::this_thread::yield();
			    }
		    });
		std::atomic<bool> ran{false};
		Task<int> task = jobs.Run(
		    [&ran]
		    {
			    ran = true;
			    return 1;
		    },
		    Lane::BACKGROUND, gate);
		task.Cancel();
		release = true;
		jobs.Wait(task);
		CHECK(task.WasCancelled());
		CHECK(task.IsComplete());
		CHECK_FALSE(ran.load());
		CHECK(task.TryGetResult() == nullptr);
		CHECK_FALSE(task.GetHandle().WasDropped());  // the job itself completed normally
	}
}

TEST_CASE("a running task stops when it sees the cancel request")
{
	if (!JobSystem::ThreadsAvailable())
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{2});
	std::atomic<bool> started{false};
	Task<int> task = jobs.Run(
	    [&started](TaskContext& context)
	    {
		    started = true;
		    while (!context.IsCancelled())
		    {
			    std::this_thread::yield();
		    }
		    return 99;
	    },
	    Lane::FRAME);
	while (!started.load())
	{
		std::this_thread::yield();
	}
	CHECK(task.GetStatus() == TaskStatus::RUNNING);
	task.Cancel();
	jobs.Wait(task);
	CHECK(task.WasCancelled());
	CHECK(task.TryGetResult() == nullptr);  // whatever the work returned after cancelling is discarded
}

TEST_CASE("progress reported by the work is visible from the main thread")
{
	if (JobSystem::ThreadsAvailable())
	{
		JobSystem jobs(JobSystemConfig{2});
		std::atomic<bool> release{false};
		Task<void> task = jobs.Run(
		    [&release](TaskContext& context)
		    {
			    context.SetProgress(0.5f);
			    while (!release.load())
			    {
				    std::this_thread::yield();
			    }
			    context.SetProgress(2.0f);  // clamped
		    },
		    Lane::FRAME);
		while (task.GetProgress() < 0.5f)
		{
			std::this_thread::yield();
		}
		CHECK(task.GetProgress() == 0.5f);
		CHECK_FALSE(task.IsComplete());
		release = true;
		jobs.Wait(task);
		CHECK(task.GetProgress() == 1.0f);
	}
	// Without workers, a stepped task advances one step per job run.
	JobSystem inlineJobs(JobSystemConfig{0});
	int step       = 0;
	Task<void> steps = inlineJobs.Run(
	    [&step](TaskContext& context)
	    {
		    ++step;
		    context.SetProgress(step / 10.0f);
		    return step == 10 ? StepResult::DONE : StepResult::CONTINUE;
	    });
	CHECK(steps.GetProgress() == 0.0f);
	CHECK(inlineJobs.TryRunOne());
	CHECK(steps.GetProgress() == doctest::Approx(0.1f));
	CHECK(steps.GetStatus() == TaskStatus::RUNNING);
	inlineJobs.Wait(steps);
	CHECK(step == 10);
	CHECK(steps.Succeeded());
}

TEST_CASE("follow-ups run on the main thread, only inside RunMainThreadWork")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		const std::thread::id mainThread = std::this_thread::get_id();
		std::atomic<std::thread::id> workThread{};
		Task<int> task = jobs.Run(
		    [&workThread]
		    {
			    workThread = std::this_thread::get_id();
			    return 3;
		    });
		int seenResult = 0;
		std::thread::id followUpThread;
		task.ThenOnMainThread(
		    [&seenResult, &followUpThread](Task<int>& done)
		    {
			    followUpThread = std::this_thread::get_id();
			    seenResult     = done.Succeeded() ? done.TakeResult() : -1;
		    });
		if (workers > 0)
		{
			while (!task.IsComplete())
			{
				std::this_thread::yield();  // poll rather than Wait(), which could run the task on this thread
			}
		}
		else
		{
			jobs.Wait(task);
		}
		CHECK(task.Succeeded());
		CHECK(seenResult == 0);  // not yet: follow-ups wait for the main thread's turn
		CHECK(jobs.GetMainThreadQueuedCount() == 1);
		CHECK(jobs.RunMainThreadWork(1.0) >= 1);
		CHECK(seenResult == 3);
		CHECK(followUpThread == mainThread);
		if (workers > 0)
		{
			CHECK(workThread.load() != mainThread);
		}

		// Added after the task completed: queued straight away.
		bool late = false;
		task.ThenOnMainThread([&late](Task<int>&) { late = true; });
		CHECK_FALSE(late);
		jobs.RunMainThreadWork(1.0);
		CHECK(late);

		// A cancelled task still gets its follow-up, and can tell it was cancelled.
		std::atomic<bool> release{false};
		JobHandle gate = jobs.Schedule(
		    [&release]
		    {
			    while (!release.load())
			    {
				    std::this_thread::yield();
			    }
		    });
		Task<int> cancelled = jobs.Run([] { return 1; }, Lane::FRAME, gate);
		TaskStatus seenStatus = TaskStatus::PENDING;
		cancelled.ThenOnMainThread([&seenStatus](Task<int>& done) { seenStatus = done.GetStatus(); });
		cancelled.Cancel();
		release = true;
		jobs.Wait(cancelled);
		jobs.RunMainThreadWork(1.0);
		CHECK(seenStatus == TaskStatus::CANCELLED);
		CHECK(jobs.GetStats().mainThreadRun == 3);
	}
}

TEST_CASE("main-thread work runs in order and respects the budget")
{
	JobSystem jobs(JobSystemConfig{0});
	std::vector<int> order;
	for (int index = 0; index < 10; ++index)
	{
		jobs.RunOnMainThread(
		    [&order, index]
		    {
			    BusyMilliseconds(1.0);
			    order.push_back(index);
		    });
	}
	const size_t ran = jobs.RunMainThreadWork(0.0025);
	CHECK(ran == 3);  // checked between follow-ups: 1 ms, 2 ms, 3 ms > 2.5 ms
	CHECK(jobs.GetMainThreadQueuedCount() == 7);
	jobs.RunMainThreadWork(1.0);
	REQUIRE(order.size() == 10);
	for (int index = 0; index < 10; ++index)
	{
		CHECK(order[static_cast<size_t>(index)] == index);
	}
}

TEST_CASE("a stepped task runs every step once and completes")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<int> steps{0};
		Task<void> task = jobs.Run([&steps] { return steps.fetch_add(1) + 1 == 500 ? StepResult::DONE : StepResult::CONTINUE; });
		// A plain stepped job through Schedule() works the same way.
		std::atomic<int> plainSteps{0};
		JobHandle plain = jobs.Schedule(
		    [&plainSteps] { return plainSteps.fetch_add(1) + 1 == 50 ? StepResult::DONE : StepResult::CONTINUE; },
		    Lane::BACKGROUND);
		jobs.Wait(task);
		jobs.Wait(plain);
		CHECK(task.Succeeded());
		CHECK(steps.load() == 500);
		CHECK(plainSteps.load() == 50);
		const JobSystemStats stats = jobs.GetStats();
		CHECK(stats.stepsRequeued == 499 + 49);
		CHECK(stats.jobsRun == stats.jobsScheduled);
	}
}

TEST_CASE("a stepped task can be cancelled between steps")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<int> steps{0};
		Task<void> task = jobs.Run(
		    [&steps]
		    {
			    steps.fetch_add(1);
			    BusyMilliseconds(0.05);
			    return StepResult::CONTINUE;  // never finishes on its own
		    });
		while (steps.load() < 5)
		{
			jobs.RunMainThreadWork(0.001);
		}
		task.Cancel();
		jobs.Wait(task);
		CHECK(task.WasCancelled());
		const int after = steps.load();
		jobs.RunMainThreadWork(0.005);
		CHECK(steps.load() == after);  // no step ran after it completed
	}
}

TEST_CASE("stepped background work gives way to frame work on a single worker")
{
	if (!JobSystem::ThreadsAvailable())
	{
		return;
	}
	JobSystem jobs(JobSystemConfig{1});
	std::atomic<bool> started{false};
	Task<void> streaming = jobs.Run(
	    [&started](TaskContext& context)
	    {
		    started = true;
		    BusyMilliseconds(1.0);
		    return context.IsCancelled() ? StepResult::DONE : StepResult::CONTINUE;
	    });
	while (!started.load())
	{
		std::this_thread::yield();
	}
	// The only worker is busy streaming; a frame job still starts within about one step instead of waiting it out.
	double worstMs = 0.0;
	for (int attempt = 0; attempt < 20; ++attempt)
	{
		const std::chrono::steady_clock::time_point scheduled = std::chrono::steady_clock::now();
		std::atomic<double> startedAfterMs{-1.0};
		JobHandle frame = jobs.Schedule([&startedAfterMs, scheduled] { startedAfterMs = MillisecondsSince(scheduled); });
		while (!frame.IsComplete())
		{
			std::this_thread::yield();
		}
		worstMs = std::max(worstMs, startedAfterMs.load());
	}
	CAPTURE(worstMs);
	CHECK(worstMs < 5.0);
	CHECK(streaming.GetStatus() == TaskStatus::RUNNING);
	streaming.Cancel();
	jobs.Wait(streaming);
	CHECK(streaming.WasCancelled());
}

TEST_CASE("without workers, streaming spans frames and each frame keeps to its budget")
{
	JobSystem jobs(JobSystemConfig{0});
	int steps       = 0;
	Task<int> chunk = jobs.Run([] { return 7; });  // an ordinary task among the steps
	Task<void> streaming = jobs.Run(
	    [&steps](TaskContext& context)
	    {
		    BusyMilliseconds(0.25);
		    ++steps;
		    context.SetProgress(steps / 40.0f);
		    return steps == 40 ? StepResult::DONE : StepResult::CONTINUE;
	    });
	bool followedUp = false;
	streaming.ThenOnMainThread([&followedUp](Task<void>& done) { followedUp = done.Succeeded(); });
	int frames        = 0;
	double worstFrame = 0.0;
	while (!followedUp && frames < 1000)
	{
		const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		jobs.RunMainThreadWork(0.001);
		worstFrame = std::max(worstFrame, MillisecondsSince(start));
		++frames;
	}
	CAPTURE(frames);
	CAPTURE(worstFrame);
	CHECK(followedUp);
	CHECK(chunk.GetResult() == 7);
	CHECK(frames >= 5);          // 10 ms of steps can't fit in a 1 ms frame budget
	CHECK(worstFrame < 3.0);     // budget + at most one step (0.25 ms), with room for timer noise
}

struct TaskTracker
{
	static inline std::atomic<int> alive{0};
	TaskTracker() { alive.fetch_add(1); }
	TaskTracker(const TaskTracker&) { alive.fetch_add(1); }
	TaskTracker(TaskTracker&&) noexcept { alive.fetch_add(1); }
	~TaskTracker() { alive.fetch_sub(1); }
};

TEST_CASE("shutdown drops tasks that haven't finished, and their follow-ups never run")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		{
			JobSystem jobs(JobSystemConfig{workers});
			// A stepped job that never finishes holds back everything that depends on it.
			JobHandle endless = jobs.Schedule(
			    []
			    {
				    BusyMilliseconds(0.1);
				    return StepResult::CONTINUE;
			    },
			    Lane::BACKGROUND);
			std::atomic<bool> ran{false};
			Task<int> blocked = jobs.Run(
			    [&ran, tracker = TaskTracker()]
			    {
				    ran = true;
				    return 1;
			    },
			    Lane::BACKGROUND, endless);
			std::array<char, 256> big{};
			Task<int> bigBlocked = jobs.Run([big, tracker = TaskTracker()] { return static_cast<int>(big[0]); }, Lane::BACKGROUND, endless);
			std::atomic<int> steps{0};
			Task<void> stepping = jobs.Run(
			    [&steps, tracker = TaskTracker()]
			    {
				    steps.fetch_add(1);
				    BusyMilliseconds(0.1);
				    return StepResult::CONTINUE;
			    },
			    Lane::FRAME);
			bool followUpRan = false;
			blocked.ThenOnMainThread([&followUpRan](Task<int>&) { followUpRan = true; });
			stepping.ThenOnMainThread([&followUpRan](Task<void>&) { followUpRan = true; });
			while (steps.load() < 3)
			{
				jobs.RunMainThreadWork(0.001);
			}
			CHECK(TaskTracker::alive.load() == 3);
			jobs.Shutdown();
			CHECK(endless.WasDropped());
			CHECK(blocked.WasDropped());
			CHECK(bigBlocked.WasDropped());
			CHECK(stepping.WasDropped());
			CHECK(blocked.IsComplete());
			CHECK_FALSE(ran.load());
			CHECK(blocked.TryGetResult() == nullptr);
			jobs.RunMainThreadWork(1.0);
			CHECK_FALSE(followUpRan);
			CHECK(jobs.GetMainThreadQueuedCount() == 0);
			CHECK(TaskTracker::alive.load() == 0);  // the work of every dropped task was destroyed exactly once

			// Running a task after shutdown gives an already-dropped task.
			Task<int> late = jobs.Run([] { return 1; });
			CHECK(late.WasDropped());
		}
		CHECK(TaskTracker::alive.load() == 0);
	}
}

TEST_CASE("many tasks from many threads all complete with the right results")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		const int threads   = JobSystem::ThreadsAvailable() ? 4 : 1;
		const int perThread = 2000;
		std::vector<std::vector<Task<int>>> made(static_cast<size_t>(threads));
		auto produce = [&jobs, &made, perThread](int thread)
		{
			for (int index = 0; index < perThread; ++index)
			{
				made[static_cast<size_t>(thread)].push_back(jobs.Run([thread, index] { return thread * 100000 + index; },
				                                                     index % 2 == 0 ? Lane::FRAME : Lane::BACKGROUND));
			}
		};
		std::vector<std::thread> producers;
		for (int thread = 1; thread < threads; ++thread)
		{
			producers.emplace_back(produce, thread);
		}
		produce(0);
		for (std::thread& producer : producers)
		{
			producer.join();
		}
		for (int thread = 0; thread < threads; ++thread)
		{
			for (int index = 0; index < perThread; ++index)
			{
				Task<int>& task = made[static_cast<size_t>(thread)][static_cast<size_t>(index)];
				jobs.Wait(task);
				REQUIRE(task.Succeeded());
				CHECK(task.GetResult() == thread * 100000 + index);
			}
		}
	}
}
