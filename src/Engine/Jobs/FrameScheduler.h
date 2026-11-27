#pragma once

#include <memory>
#include <vector>

#include "Engine/Jobs/JobSystem.h"

// Ties the job system to the frame loop (phase 3). The engine calls BeginFrame() at the top of every frame and
// EndFrame() once the frame has been submitted:
//
//   BeginFrame()  1. sync point: waits for the previous frame's post-frame jobs (helping run them meanwhile)
//                 2. runs main-thread follow-ups (Task::ThenOnMainThread, JobSystem::RunOnMainThread) within the
//                    main-thread budget; with zero workers this is also where queued jobs make progress
//   ...frame...   systems run; anything may call PostFrame(work)
//   EndFrame()    schedules the collected post-frame work on the POST_FRAME lane, to run while the next frame's
//                 input/update starts - it must be done by the next BeginFrame()
//
// Work that may take longer than a frame doesn't belong here: run it as a background Task instead.
namespace Struktur::Jobs
{
class FrameScheduler
{
public:
	explicit FrameScheduler(JobSystem& jobs, double mainThreadBudgetSeconds = 0.002);
	~FrameScheduler();

	FrameScheduler(const FrameScheduler&)            = delete;
	FrameScheduler& operator=(const FrameScheduler&) = delete;

	void BeginFrame();
	void EndFrame();

	// Queues work for after this frame (autosaves, chunk builds, stats). It starts at EndFrame() and must finish by
	// the next BeginFrame(), which waits for it.
	template <typename Work>
	void PostFrame(Work&& work)
	{
		std::unique_ptr<JobFunction> function = std::make_unique<JobFunction>();
		function->Set(std::forward<Work>(work));
		m_postFrame.push_back(std::move(function));
	}

	JobSystem& GetJobSystem() { return m_jobs; }
	double GetMainThreadBudget() const { return m_budgetSeconds; }
	void SetMainThreadBudget(double seconds) { m_budgetSeconds = seconds; }

	// Last BeginFrame(): how long it waited on post-frame work, and how many follow-ups/jobs it ran.
	double GetLastSyncMilliseconds() const { return m_lastSyncMilliseconds; }
	size_t GetLastMainThreadRun() const { return m_lastMainThreadRun; }
	uint64_t GetFrameIndex() const { return m_frame; }

private:
	JobSystem& m_jobs;
	double m_budgetSeconds;
	std::vector<std::unique_ptr<JobFunction>> m_postFrame;
	JobHandle m_postFrameDone;
	double m_lastSyncMilliseconds = 0.0;
	size_t m_lastMainThreadRun    = 0;
	uint64_t m_frame              = 0;
};
}  // namespace Struktur::Jobs
