#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Ride/Workout.h"

namespace Struktur::Ride
{
struct RunnerEvent
{
	enum class Type
	{
		STEP_STARTED,  // stepIndex is the new step
		CUE,           // a text cue is due: message / durationSeconds
		FINISHED,      // the last step ended
	};

	Type type        = Type::STEP_STARTED;
	size_t stepIndex = 0;
	std::string message;
	double durationSeconds = 0.0;
};

// Plays a workout against a monotonic clock (seconds, any origin - the ride passes its unscaled time).
//
//  - Pause/Resume stop the workout clock; skipping a step jumps the clock to the step's end; extending a step adds
//    time to the current step only (the plan after it is unchanged).
//  - Targets are FTP fractions. For the first `leadInSeconds` of a step, the target blends linearly from where the
//    previous step ended to the new step's target, so ERG resistance never jumps (3-5 s recommended).
//  - Steps with no target (free ride, max effort) return nullopt; the lead-in never blends across them.
class WorkoutRunner
{
public:
	explicit WorkoutRunner(Workout workout, double leadInSeconds = 4.0);

	void Start(double now);
	void Pause(double now);
	void Resume(double now);
	void SkipStep(double now);
	void ExtendStep(double seconds);
	void Update(double now);

	bool IsStarted() const;
	bool IsPaused() const;
	bool IsFinished() const;

	const Workout& GetWorkout() const;
	size_t GetStepIndex() const;
	const WorkoutStep& GetCurrentStep() const;
	const WorkoutStep* GetNextStep() const;
	double GetStepDuration() const;  // including any extension
	double GetStepElapsed() const;
	double GetStepRemaining() const;
	double GetElapsed() const;  // workout clock (excludes pauses, includes skipped time)
	// Position in the ORIGINAL plan (for the profile marker): never runs past the current step's planned end.
	double GetPlanPosition() const;
	bool IsInLeadIn() const;
	std::optional<double> GetTargetFraction() const;

	std::vector<RunnerEvent> TakeEvents();

private:
	double ClockAt(double now) const;
	double DurationOf(size_t stepIndex) const;
	void Advance(double clock);

	Workout m_workout;
	std::vector<double> m_planStarts;
	std::vector<double> m_extensions;
	double m_leadInSeconds;
	bool m_started;
	bool m_paused;
	bool m_finished;
	double m_startTime;
	double m_pausedTotal;
	double m_pauseStart;
	double m_skipOffset;
	double m_clock;
	size_t m_stepIndex;
	double m_stepStartClock;
	size_t m_nextCue;
	std::vector<RunnerEvent> m_events;
};
}  // namespace Struktur::Ride
