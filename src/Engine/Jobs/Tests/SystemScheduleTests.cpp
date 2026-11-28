// ECS systems as jobs (phase 4): access declarations, the per-phase schedule and running it. Engine-free - the
// systems here are stand-ins that record what ran, where and alongside what.
#include <doctest.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "Engine/ECS/SystemSchedule.h"
#include "TestHelpers.h"

using namespace Struktur;
using namespace Struktur::System;

namespace ScheduleTestTypes
{
struct Position
{
};
struct Velocity
{
};
struct Sprite
{
};
struct Animation
{
};
struct Clock
{
};
}  // namespace ScheduleTestTypes

using namespace ScheduleTestTypes;

TEST_CASE("access declarations clash only on writes")
{
	const SystemAccess readsPosition = SystemAccess::Worker().Read<Position, Clock>();
	const SystemAccess alsoReadsPosition = SystemAccess::Worker().Read<Position>().Write<Velocity>();
	const SystemAccess writesPosition = SystemAccess::Worker().Write<Position>();
	const SystemAccess animates = SystemAccess::Worker().Read<Animation, Clock>().Write<Sprite>();
	const SystemAccess mainThread = SystemAccess::MainThread();

	CHECK_FALSE(readsPosition.ConflictsWith(alsoReadsPosition));  // two readers
	CHECK(readsPosition.ConflictsWith(writesPosition));           // read / write
	CHECK(writesPosition.ConflictsWith(readsPosition));           // either way round
	CHECK(writesPosition.ConflictsWith(writesPosition));          // write / write
	CHECK_FALSE(animates.ConflictsWith(writesPosition));          // disjoint
	CHECK(mainThread.ConflictsWith(animates));                    // main thread clashes with everything
	CHECK(animates.ConflictsWith(mainThread));
	CHECK(SystemAccess::Worker().Read<Clock>().Read<Clock>().GetReads().size() == 1);  // no duplicates
}

TEST_CASE("the schedule batches consecutive worker systems and keeps main-thread ones in place")
{
	// 0 main, 1 worker(writes Sprite), 2 worker(writes Position), 3 worker(reads Position), 4 main, 5 worker
	const std::vector<SystemAccess> accesses = {
	    SystemAccess::MainThread(),
	    SystemAccess::Worker().Read<Animation>().Write<Sprite>(),
	    SystemAccess::Worker().Write<Position>(),
	    SystemAccess::Worker().Read<Position>().Write<Velocity>(),
	    SystemAccess::MainThread(),
	    SystemAccess::Worker().Write<Position>(),
	};
	const std::vector<ScheduleStep> steps = BuildSchedule(accesses);
	REQUIRE(steps.size() == 4);
	CHECK(steps[0].mainThread);
	CHECK(steps[0].systems == std::vector<size_t>{0});
	REQUIRE_FALSE(steps[1].mainThread);
	CHECK(steps[1].systems == std::vector<size_t>{1, 2, 3});
	CHECK(steps[1].dependencies[0].empty());
	CHECK(steps[1].dependencies[1].empty());                     // Sprite and Position don't clash
	CHECK(steps[1].dependencies[2] == std::vector<size_t>{1});   // reads what system 2 writes: waits for it
	CHECK(steps[2].mainThread);
	CHECK(steps[2].systems == std::vector<size_t>{4});
	CHECK(steps[3].systems == std::vector<size_t>{5});           // a new batch after the main-thread system

	// Nothing declared (every system main-thread): one step each, same order as before.
	const std::vector<ScheduleStep> legacy = BuildSchedule(std::vector<SystemAccess>(5, SystemAccess::MainThread()));
	REQUIRE(legacy.size() == 5);
	for (size_t index = 0; index < legacy.size(); ++index)
	{
		CHECK(legacy[index].mainThread);
		CHECK(legacy[index].systems == std::vector<size_t>{index});
	}
}

TEST_CASE("running the schedule: clashing systems keep their order, others overlap, batches end before main-thread ones")
{
	for (int workers : WorkerCounts())
	{
		CAPTURE(workers);
		Jobs::JobSystem jobs(Jobs::JobSystemConfig{workers});
		// 0 main, 1..4 workers: 1 writes Sprite, 2 writes Position, 3 reads Position, 4 writes Sprite; 5 main.
		const std::vector<SystemAccess> accesses = {
		    SystemAccess::MainThread(),
		    SystemAccess::Worker().Write<Sprite>(),
		    SystemAccess::Worker().Write<Position>(),
		    SystemAccess::Worker().Read<Position>(),
		    SystemAccess::Worker().Write<Sprite>(),
		    SystemAccess::MainThread(),
		};
		const std::vector<ScheduleStep> steps = BuildSchedule(accesses);
		const std::thread::id mainThread      = std::this_thread::get_id();
		for (int frame = 0; frame < 50; ++frame)
		{
			std::mutex mutex;
			std::vector<size_t> finished;
			std::vector<std::thread::id> ranOn(accesses.size());
			std::atomic<int> running{0};
			std::atomic<int> mostAtOnce{0};
			int batchesApplied = 0;
			bool batchDoneBeforeLast = false;
			RunSchedule(
			    steps, jobs, true,
			    [&](size_t index)
			    {
				    const int now = running.fetch_add(1) + 1;
				    int seen      = mostAtOnce.load();
				    while (now > seen && !mostAtOnce.compare_exchange_weak(seen, now))
				    {
				    }
				    ranOn[index] = std::this_thread::get_id();
				    if (index == 5)
				    {
					    std::lock_guard<std::mutex> lock(mutex);
					    batchDoneBeforeLast = finished.size() == 5;
				    }
				    BusyMilliseconds(index >= 1 && index <= 4 ? 0.3 : 0.05);
				    running.fetch_sub(1);
				    std::lock_guard<std::mutex> lock(mutex);
				    finished.push_back(index);
			    },
			    [&] { ++batchesApplied; });
			REQUIRE(finished.size() == 6);
			CHECK(finished.front() == 0);
			CHECK(finished.back() == 5);
			CHECK(batchDoneBeforeLast);
			CHECK(batchesApplied == 1);
			CHECK(ranOn[0] == mainThread);
			CHECK(ranOn[5] == mainThread);
			// Clashing pairs keep registration order: 1 before 4 (both write Sprite), 2 before 3.
			auto position = [&finished](size_t index)
			{ return std::find(finished.begin(), finished.end(), index) - finished.begin(); };
			CHECK(position(1) < position(4));
			CHECK(position(2) < position(3));
			if (workers == 0)
			{
				CHECK(mostAtOnce.load() == 1);  // no workers: one at a time, in order
				CHECK(finished == std::vector<size_t>{0, 1, 2, 3, 4, 5});
			}
		}
	}
}

TEST_CASE("non-clashing worker systems really do run at the same time")
{
	if (!Jobs::JobSystem::ThreadsAvailable())
	{
		return;
	}
	Jobs::JobSystem jobs(Jobs::JobSystemConfig{4});
	const std::vector<SystemAccess> accesses = {
	    SystemAccess::Worker().Write<Sprite>(),
	    SystemAccess::Worker().Write<Position>(),
	    SystemAccess::Worker().Write<Velocity>(),
	    SystemAccess::Worker().Write<Animation>(),
	};
	const std::vector<ScheduleStep> steps = BuildSchedule(accesses);
	const auto start = std::chrono::steady_clock::now();
	RunSchedule(steps, jobs, true, [](size_t) { BusyMilliseconds(10.0); }, [] {});
	const double elapsed = MillisecondsSince(start);
	CAPTURE(elapsed);
	CHECK(elapsed < 30.0);  // 4 x 10 ms serially; side by side about 10 ms

	// The same systems with parallel off take the serial time.
	const auto serialStart = std::chrono::steady_clock::now();
	RunSchedule(steps, jobs, false, [](size_t) { BusyMilliseconds(10.0); }, [] {});
	CHECK(MillisecondsSince(serialStart) >= 39.0);
}
