#include "FrameScheduler.h"

#include <chrono>

namespace Struktur::Jobs
{
FrameScheduler::FrameScheduler(JobSystem& jobs, double mainThreadBudgetSeconds)
    : m_jobs(jobs),
      m_budgetSeconds(mainThreadBudgetSeconds)
{
}

FrameScheduler::~FrameScheduler()
{
	m_jobs.Wait(m_postFrameDone);
}

void FrameScheduler::BeginFrame()
{
	const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	m_jobs.Wait(m_postFrameDone);
	m_postFrameDone        = JobHandle();
	m_lastSyncMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	m_lastMainThreadRun    = m_jobs.RunMainThreadWork(m_budgetSeconds);
}

void FrameScheduler::EndFrame()
{
	++m_frame;
	if (m_postFrame.empty())
	{
		return;
	}
	std::vector<JobHandle> handles;
	handles.reserve(m_postFrame.size());
	for (std::unique_ptr<JobFunction>& work : m_postFrame)
	{
		handles.push_back(m_jobs.Schedule([function = std::move(work)] { (*function)(); }, Lane::POST_FRAME));
	}
	m_postFrame.clear();
	m_postFrameDone = m_jobs.WhenAll(handles, Lane::POST_FRAME);
}
}  // namespace Struktur::Jobs
