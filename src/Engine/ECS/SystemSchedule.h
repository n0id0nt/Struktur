#pragma once

#include <functional>
#include <vector>

#include "Engine/ECS/SystemAccess.h"
#include "Engine/Jobs/JobSystem.h"

// Turns one phase's ordered system list into steps (phase 4 of docs/JobSystem.md). Engine-free so it is unit tested
// with the job system's own tests.
//
//  - A main-thread system is a step of its own, run on the calling thread.
//  - A run of consecutive worker systems is one batch: each runs as a job that depends on the earlier systems of the
//    batch it clashes with (SystemAccess::ConflictsWith), so non-clashing ones run at the same time and clashing ones
//    keep their registration order. The batch ends before the next main-thread system starts.
//
// Registration order therefore still decides everything a declaration doesn't: systems that never declared access
// behave exactly as before.
namespace Struktur::System
{
struct ScheduleStep
{
	bool mainThread = true;
	std::vector<size_t> systems;                    // indices into the phase's list (one for a main-thread step)
	std::vector<std::vector<size_t>> dependencies;  // per system of a batch: positions within `systems` it waits for
};

inline std::vector<ScheduleStep> BuildSchedule(const std::vector<SystemAccess>& accesses)
{
	std::vector<ScheduleStep> steps;
	for (size_t index = 0; index < accesses.size(); ++index)
	{
		if (accesses[index].IsMainThread())
		{
			ScheduleStep step;
			step.mainThread = true;
			step.systems.push_back(index);
			step.dependencies.emplace_back();
			steps.push_back(std::move(step));
			continue;
		}
		if (steps.empty() || steps.back().mainThread)
		{
			ScheduleStep step;
			step.mainThread = false;
			steps.push_back(std::move(step));
		}
		ScheduleStep& batch = steps.back();
		std::vector<size_t> waitsFor;
		for (size_t position = 0; position < batch.systems.size(); ++position)
		{
			if (accesses[batch.systems[position]].ConflictsWith(accesses[index]))
			{
				waitsFor.push_back(position);
			}
		}
		batch.systems.push_back(index);
		batch.dependencies.push_back(std::move(waitsFor));
	}
	return steps;
}

// Runs the steps: `run(index)` runs one system, `afterBatch()` follows every batch (the command list is applied
// there). With `parallel` false - or zero workers - every system runs on the calling thread, in order.
inline void RunSchedule(const std::vector<ScheduleStep>& steps, Jobs::JobSystem& jobs, bool parallel,
                        const std::function<void(size_t)>& run, const std::function<void()>& afterBatch)
{
	for (const ScheduleStep& step : steps)
	{
		if (step.mainThread || !parallel || jobs.GetWorkerCount() == 0 || step.systems.size() == 1)
		{
			for (size_t index : step.systems)
			{
				run(index);
			}
			if (!step.mainThread)
			{
				afterBatch();
			}
			continue;
		}
		std::vector<Jobs::JobHandle> handles;
		handles.reserve(step.systems.size());
		for (size_t position = 0; position < step.systems.size(); ++position)
		{
			std::vector<Jobs::JobHandle> waitFor;
			for (size_t dependency : step.dependencies[position])
			{
				waitFor.push_back(handles[dependency]);
			}
			const size_t index = step.systems[position];
			handles.push_back(jobs.Schedule([&run, index] { run(index); }, Jobs::Lane::FRAME, waitFor));
		}
		jobs.Wait(jobs.WhenAll(handles));
		afterBatch();
	}
}
}  // namespace Struktur::System
