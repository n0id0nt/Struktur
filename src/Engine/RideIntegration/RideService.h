#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Ride/RideSession.h"
#include "Engine/Ride/RideSettings.h"
#include "Engine/Ride/Workout.h"
#include "Engine/RideWorld/AssetManifest.h"
#include "Engine/RideWorld/Biome.h"
#include "Engine/RideWorld/RideWorld.h"
#include "Engine/RideWorld/SpriteDescriptor.h"

namespace Struktur
{
class GameContext;

namespace Renderer
{
class RideWorldRenderer;
}

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
// feeds the session from FitnessManager every frame (RideSystem calls Update). Session events become engine events
// ("ride" + name, e.g. rideStepStarted) and a "rideTick" event fires ~4x a second while a ride is active, so the Wren
// HUD refreshes on events rather than polling.
class RideService
{
public:
	// Chunk builds and ride autosaves run on `jobs` (GameContext's job system, which outlives this service).
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

	// --- procedural world (docs/RideWorld.md) ---------------------------------------------------------------------
	// The world behind the active ride, or null (world disabled in settings, no biome data, or no ride).
	RideWorld::RideWorld* GetWorld();
	// Re-applies settings.world to the running world (creating or dropping it when "enabled" changed).
	void ApplyWorldSettings();
	// Draws the world (RideWorldRenderSystem, every frame). Creates the renderer's GPU resources on first use.
	void RenderWorld(GameContext& context);
	const std::vector<std::string>& GetBiomeIds() const;
	// Every manifest role whose real file is missing, as log lines / a Markdown table (--list-missing-assets).
	std::vector<std::string> GetMissingAssetLines() const;
	std::string GetMissingAssetsMarkdown() const;

private:
	std::string MakeOutputBasePath(const std::string& title) const;
	void LoadWorldData();
	void CreateWorld();
	void UpdateWorld(double now);

	std::string m_saveDirectory;
	Ride::RideSettings m_settings;
	std::unique_ptr<Ride::RideSession> m_session;
	bool m_commandLineWorkoutTaken = false;
	double m_lastTickNow           = 0.0;

	RideWorld::BiomeList m_biomes;
	std::vector<std::string> m_biomeIds;
	RideWorld::AssetManifest m_manifest;
	RideWorld::SpriteDescriptor m_spriteDescriptor;
	Jobs::JobSystem* m_jobs = nullptr;
	std::unique_ptr<RideWorld::RideWorld> m_world;
	std::unique_ptr<RideWorld::DemoPacer> m_pacer;
	std::unique_ptr<Renderer::RideWorldRenderer> m_worldRenderer;
	double m_lastWorldNow = 0.0;
};
}  // namespace RideIntegration
}  // namespace Struktur
