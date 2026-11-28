#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Struktur::Ride
{
struct TextCue
{
	double offsetSeconds   = 0.0;  // from the start of its step
	double durationSeconds = 10.0;
	std::string message;
};

// One block of a workout. Targets are fractions of FTP, so the same workout scales to any rider.
struct WorkoutStep
{
	enum class Kind : uint8_t
	{
		STEADY,      // constant target (startFraction == endFraction)
		RAMP,        // linear from startFraction to endFraction
		FREE_RIDE,   // no target - ride as you like (trainer goes to simulation mode)
		MAX_EFFORT,  // no target - all out
	};

	Kind kind              = Kind::STEADY;
	double durationSeconds = 0.0;
	double startFraction   = 0.0;
	double endFraction     = 0.0;
	std::optional<double> cadenceRpm;
	std::string label;  // "Warmup", "Interval 2/5", ...
	std::vector<TextCue> cues;

	bool HasTarget() const
	{
		return kind == Kind::STEADY || kind == Kind::RAMP;
	}
	// Target fraction `secondsIntoStep` in (clamped to the step), or nullopt for a step with no target.
	std::optional<double> TargetFractionAt(double secondsIntoStep) const;
};

struct Workout
{
	std::string name;
	std::string author;
	std::string description;
	std::string sourcePath;
	std::vector<WorkoutStep> steps;

	double TotalDurationSeconds() const;
	// Start time of each step (same size as steps).
	std::vector<double> StepStartTimes() const;
};

// Safety limits for workout files - they come from the internet and are treated as untrusted.
struct WorkoutLimits
{
	size_t maxFileBytes     = 1024 * 1024;
	size_t maxSteps         = 1000;  // after expanding IntervalsT repeats
	int maxRepeat           = 200;
	double maxStepSeconds   = 6.0 * 3600.0;
	double maxTotalSeconds  = 12.0 * 3600.0;
	double maxPowerFraction = 4.0;  // 400 % FTP
	size_t maxCuesPerStep   = 64;
	size_t maxTextLength    = 512;
};

struct WorkoutLoadResult
{
	std::optional<Workout> workout;
	std::string error;  // human-readable reason when workout is empty

	bool IsOk() const
	{
		return workout.has_value();
	}
};

// Zwift-style .zwo XML: Warmup, Cooldown, SteadyState, Ramp, IntervalsT, FreeRide, MaxEffort, textevent.
WorkoutLoadResult ParseZwo(const std::string& text, const WorkoutLimits& limits = WorkoutLimits());
// .mrc ("MINUTES PERCENT" course data).
WorkoutLoadResult ParseMrc(const std::string& text, const WorkoutLimits& limits = WorkoutLimits());
// Picks the parser by extension (.zwo / .mrc, case-insensitive) after checking the size cap.
WorkoutLoadResult ParseWorkout(const std::string& fileName, const std::string& text,
                               const WorkoutLimits& limits = WorkoutLimits());
// Reads a file from disk (size-capped before reading) and parses it.
WorkoutLoadResult LoadWorkoutFile(const std::string& path, const WorkoutLimits& limits = WorkoutLimits());
}  // namespace Struktur::Ride
