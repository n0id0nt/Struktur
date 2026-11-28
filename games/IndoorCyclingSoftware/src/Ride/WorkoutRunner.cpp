#include "WorkoutRunner.h"

#include <algorithm>
#include <utility>

namespace Struktur::Ride
{
WorkoutRunner::WorkoutRunner(Workout workout, double leadInSeconds)
    : m_workout(std::move(workout)),
      m_planStarts(),
      m_extensions(),
      m_leadInSeconds(std::max(0.0, leadInSeconds)),
      m_started(false),
      m_paused(false),
      m_finished(false),
      m_startTime(0.0),
      m_pausedTotal(0.0),
      m_pauseStart(0.0),
      m_skipOffset(0.0),
      m_clock(0.0),
      m_stepIndex(0),
      m_stepStartClock(0.0),
      m_nextCue(0),
      m_events()
{
	m_planStarts = m_workout.StepStartTimes();
	m_extensions.assign(m_workout.steps.size(), 0.0);
	m_finished = m_workout.steps.empty();
}

void WorkoutRunner::Start(double now)
{
	if (m_started || m_finished)
	{
		return;
	}
	m_started   = true;
	m_startTime = now;
	m_events.push_back(RunnerEvent{RunnerEvent::Type::STEP_STARTED, 0, {}, 0.0});
	Update(now);
}

void WorkoutRunner::Pause(double now)
{
	if (!m_started || m_paused || m_finished)
	{
		return;
	}
	Update(now);
	m_paused     = true;
	m_pauseStart = now;
}

void WorkoutRunner::Resume(double now)
{
	if (!m_paused)
	{
		return;
	}
	m_pausedTotal += now - m_pauseStart;
	m_paused = false;
	Update(now);
}

void WorkoutRunner::SkipStep(double now)
{
	if (!m_started || m_finished)
	{
		return;
	}
	Update(now);
	if (m_finished)
	{
		return;
	}
	const double stepEnd = m_stepStartClock + DurationOf(m_stepIndex);
	m_skipOffset += stepEnd - m_clock;
	if (m_paused)
	{
		// Skipping while paused still moves on; the clock stays frozen at the new step's start.
		m_clock = stepEnd;
		Advance(m_clock);
		return;
	}
	Update(now);
}

void WorkoutRunner::ExtendStep(double seconds)
{
	if (!m_started || m_finished || seconds <= 0.0)
	{
		return;
	}
	m_extensions[m_stepIndex] += seconds;
}

void WorkoutRunner::Update(double now)
{
	if (!m_started || m_paused || m_finished)
	{
		return;
	}
	m_clock = ClockAt(now);
	Advance(m_clock);
}

void WorkoutRunner::Advance(double clock)
{
	while (!m_finished && clock >= m_stepStartClock + DurationOf(m_stepIndex))
	{
		m_stepStartClock += DurationOf(m_stepIndex);
		++m_stepIndex;
		m_nextCue = 0;
		if (m_stepIndex >= m_workout.steps.size())
		{
			m_stepIndex = m_workout.steps.size() - 1;
			m_finished  = true;
			m_events.push_back(RunnerEvent{RunnerEvent::Type::FINISHED, m_stepIndex, {}, 0.0});
			return;
		}
		m_events.push_back(RunnerEvent{RunnerEvent::Type::STEP_STARTED, m_stepIndex, {}, 0.0});
	}
	const std::vector<TextCue>& cues = m_workout.steps[m_stepIndex].cues;
	const double stepElapsed         = clock - m_stepStartClock;
	while (m_nextCue < cues.size() && cues[m_nextCue].offsetSeconds <= stepElapsed)
	{
		const TextCue& cue = cues[m_nextCue];
		m_events.push_back(RunnerEvent{RunnerEvent::Type::CUE, m_stepIndex, cue.message, cue.durationSeconds});
		++m_nextCue;
	}
}

bool WorkoutRunner::IsStarted() const
{
	return m_started;
}

bool WorkoutRunner::IsPaused() const
{
	return m_paused;
}

bool WorkoutRunner::IsFinished() const
{
	return m_finished;
}

const Workout& WorkoutRunner::GetWorkout() const
{
	return m_workout;
}

size_t WorkoutRunner::GetStepIndex() const
{
	return m_stepIndex;
}

const WorkoutStep& WorkoutRunner::GetCurrentStep() const
{
	return m_workout.steps[m_stepIndex];
}

const WorkoutStep* WorkoutRunner::GetNextStep() const
{
	return m_stepIndex + 1 < m_workout.steps.size() ? &m_workout.steps[m_stepIndex + 1] : nullptr;
}

double WorkoutRunner::GetStepDuration() const
{
	return m_workout.steps.empty() ? 0.0 : DurationOf(m_stepIndex);
}

double WorkoutRunner::GetStepElapsed() const
{
	if (m_finished)
	{
		return GetStepDuration();
	}
	return std::max(0.0, m_clock - m_stepStartClock);
}

double WorkoutRunner::GetStepRemaining() const
{
	return std::max(0.0, GetStepDuration() - GetStepElapsed());
}

double WorkoutRunner::GetElapsed() const
{
	return m_clock;
}

double WorkoutRunner::GetPlanPosition() const
{
	if (m_workout.steps.empty())
	{
		return 0.0;
	}
	const double planned = m_workout.steps[m_stepIndex].durationSeconds;
	return m_planStarts[m_stepIndex] + std::min(GetStepElapsed(), planned);
}

bool WorkoutRunner::IsInLeadIn() const
{
	if (m_stepIndex == 0 || m_finished || !GetCurrentStep().HasTarget() ||
	    !m_workout.steps[m_stepIndex - 1].HasTarget())
	{
		return false;
	}
	return GetStepElapsed() < m_leadInSeconds;
}

std::optional<double> WorkoutRunner::GetTargetFraction() const
{
	if (!m_started || m_workout.steps.empty())
	{
		return std::nullopt;
	}
	const WorkoutStep& step = GetCurrentStep();
	// An extended step holds its final target for the extra time.
	const double elapsed               = std::min(GetStepElapsed(), step.durationSeconds);
	const std::optional<double> target = step.TargetFractionAt(elapsed);
	if (!target || !IsInLeadIn())
	{
		return target;
	}
	const WorkoutStep& previous = m_workout.steps[m_stepIndex - 1];
	const double from           = *previous.TargetFractionAt(previous.durationSeconds);
	const double blend          = m_leadInSeconds > 0.0 ? GetStepElapsed() / m_leadInSeconds : 1.0;
	return from + (*target - from) * blend;
}

std::vector<RunnerEvent> WorkoutRunner::TakeEvents()
{
	return std::exchange(m_events, {});
}

double WorkoutRunner::ClockAt(double now) const
{
	return now - m_startTime - m_pausedTotal + m_skipOffset;
}

double WorkoutRunner::DurationOf(size_t stepIndex) const
{
	return m_workout.steps[stepIndex].durationSeconds + m_extensions[stepIndex];
}
}  // namespace Struktur::Ride
