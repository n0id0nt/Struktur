// FrameScheduler (phase 3): the sync points the engine's frame loop calls.
#include <doctest.h>

#include <atomic>
#include <thread>

#include "Engine/Jobs/FrameScheduler.h"
#include "TestHelpers.h"

using namespace Struktur::Jobs;

TEST_CASE("post-frame work starts at EndFrame and is finished by the next BeginFrame")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		FrameScheduler frames(jobs);
		for (int frame = 0; frame < 20; ++frame)
		{
			frames.BeginFrame();
			std::atomic<int> ran{0};
			for (int index = 0; index < 8; ++index)
			{
				frames.PostFrame(
				    [&ran]
				    {
					    BusyMilliseconds(0.2);
					    ran.fetch_add(1);
				    });
			}
			if (workers > 0)
			{
				SleepMilliseconds(1);
			}
			CHECK(ran.load() == 0);  // nothing starts before the frame ends
			frames.EndFrame();
			frames.BeginFrame();     // the sync point
			CHECK(ran.load() == 8);
			frames.EndFrame();
		}
		CHECK(frames.GetFrameIndex() == 40);
	}
}

TEST_CASE("BeginFrame runs main-thread follow-ups within its budget")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		FrameScheduler frames(jobs, 0.0025);
		int followUps = 0;
		for (int index = 0; index < 10; ++index)
		{
			jobs.RunOnMainThread(
			    [&followUps]
			    {
				    BusyMilliseconds(1.0);
				    ++followUps;
			    });
		}
		frames.BeginFrame();
		CHECK(followUps == 3);
		CHECK(frames.GetLastMainThreadRun() == 3);
		frames.EndFrame();
		int framesTaken = 1;
		while (followUps < 10 && framesTaken < 100)
		{
			frames.BeginFrame();
			frames.EndFrame();
			++framesTaken;
		}
		CHECK(followUps == 10);
		CHECK(framesTaken == 4);
	}
}

TEST_CASE("a task started in one frame hands its result back in a later frame")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		FrameScheduler frames(jobs);
		Task<int> load = jobs.Run(
		    []
		    {
			    BusyMilliseconds(5.0);
			    return 12;
		    });
		int delivered      = 0;
		uint64_t frameSeen = 0;
		load.ThenOnMainThread(
		    [&delivered, &frameSeen, &frames](Task<int>& done)
		    {
			    delivered = done.GetResult();
			    frameSeen = frames.GetFrameIndex();
		    });
		int frame = 0;
		while (delivered == 0 && frame < 10000)
		{
			frames.BeginFrame();
			BusyMilliseconds(0.5);  // the frame's own work
			frames.EndFrame();
			++frame;
		}
		CHECK(delivered == 12);
		CHECK(frameSeen > 0);  // not on the frame it was started in
	}
}

TEST_CASE("the scheduler waits for outstanding post-frame work when destroyed")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		JobSystem jobs(JobSystemConfig{workers});
		std::atomic<bool> ran{false};
		{
			FrameScheduler frames(jobs);
			frames.BeginFrame();
			frames.PostFrame(
			    [&ran]
			    {
				    BusyMilliseconds(2.0);
				    ran = true;
			    });
			frames.EndFrame();
		}
		CHECK(ran.load());
	}
}
