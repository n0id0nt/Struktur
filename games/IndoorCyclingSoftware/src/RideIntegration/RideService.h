#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Input/TrainerInput.h"
#include "Ride/RideSession.h"
#include "Ride/RideSettings.h"
#include "Ride/Workout.h"

namespace Struktur
{
class GameContext;

namespace RideIntegration
{
struct WorkoutListing
{
	std::string id;  // "Workouts/<file>" (engine file system) or an absolute path (dropped / command line)
	std::string name;
	std::string source;  // "builtin" | "user" | "file"
	double durationSeconds = 0.0;
	std::string error;  // non-empty when the file doesn't load
};

// The engine side of the ride screens: owns the settings file, the workout library and the active RideSession, and
// feeds the session from FitnessManager every frame (RideSystem calls Update). It knows nothing about the world -
// RideSystem passes the road grade to the session and the finish banner to the world. Session events become engine events
// ("ride" + name, e.g. rideStepStarted) and a "rideTick" event fires ~4x a second while a ride is active, so the Wren
// HUD refreshes on events rather than polling.
class RideService
{
public:
	// Ride autosaves run on `jobs` (GameContext's job system, which outlives this service).
	explicit RideService(Jobs::JobSystem* jobs);
	~RideService();

	// saveDirectory: the OS path the engine mounts as its write dir. Loads ride_settings.json and makes sure the
	// Workouts/ and Rides/ folders exist there.
	void Initialise(const std::string& saveDirectory);

	Ride::RideSettings& GetSettings();
	bool SaveSettings();

	std::string GetUserWorkoutsFolder() const;
	std::string GetRidesFolder() const;
	bool IsDemo() const;

	// Built-in (assets/Workouts) and user (<save>/Workouts) workouts, sorted by name.
	std::vector<WorkoutListing> ListWorkouts() const;
	Ride::WorkoutLoadResult LoadWorkout(const std::string& id) const;
	// A workout path given on the command line, returned once.
	std::optional<std::string> TakeCommandLineWorkout();

	bool StartFreeRide(const Ride::RiderInfo& rider, double now);
	bool StartWorkout(const std::string& workoutId, const Ride::RiderInfo& rider, double now, std::string& error);
	Ride::RideSession* GetSession();
	bool IsActive() const;
	// Ends the ride and writes the files. The session stays readable (summary) until Discard().
	bool Finish(double now);
	void Discard();

	void Update(GameContext& context, double now);

	// True once after a workout's last step has finished.
	bool TakeWorkoutFinished();

private:
	std::string MakeOutputBasePath(const std::string& title) const;

	std::string m_saveDirectory;
	Ride::RideSettings m_settings;
	std::unique_ptr<Ride::RideSession> m_session;
	bool m_commandLineWorkoutTaken = false;
	double m_lastTickNow           = 0.0;

	Jobs::JobSystem* m_jobs  = nullptr;
	bool m_workoutFinished   = false;
};
}  // namespace RideIntegration
}  // namespace Struktur
