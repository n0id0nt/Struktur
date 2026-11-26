#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "Engine/RideWorld/Biome.h"
#include "Engine/RideWorld/ChunkStreamer.h"
#include "Engine/RideWorld/ElevationProfile.h"
#include "Engine/RideWorld/RiderAnimation.h"
#include "Engine/RideWorld/Route.h"
#include "Engine/RideWorld/WorldSettings.h"

namespace Struktur::RideWorld
{
// What the ride screens feed the world each frame. Nothing here knows about trainers, so --demo and tests drive it
// the same way a real ride does.
struct RideWorldInputs
{
	double deltaSeconds          = 0.0;
	double speedMetersPerSecond  = 0.0;  // simulated speed
	double manualGradePercent    = 0.0;  // the grade the rider set (SIM resistance)
	bool ergMode                 = false;
	int powerZone                = 0;
	std::optional<double> cadenceRpm;  // nullopt = no cadence source
	std::optional<double> heartRateBpm;
	std::optional<double> powerFraction;  // power / FTP
	double workoutProgress       = 0.0;   // 0 - 1
	std::optional<double> targetDeltaWatts;  // power - target
	// Workout: target intensity (fraction of FTP) for each of the next N seconds; NaN = no target (free-ride step).
	// Empty = Free Ride.
	std::vector<double> intensityAhead;
};

// Something drawn on the road at a distance: a pacer or a ghost.
struct WorldActor
{
	double distance   = 0.0;
	double lateral    = 1.2;  // metres left of the centre line
	double crankPhase = 0.0;  // revolutions
	uint32_t tint     = 0xFFFFFFFFu;  // ABGR multiplier
};

// Supplies actors each frame (demo pacer today; ghost replays later).
class IWorldActorSource
{
public:
	virtual ~IWorldActorSource() = default;
	virtual void Collect(double riderDistance, double deltaSeconds, std::vector<WorldActor>& out) = 0;
};

// A rider that holds a steady pace around the player - for --demo only.
class DemoPacer : public IWorldActorSource
{
public:
	explicit DemoPacer(double startAhead = 25.0);
	void Collect(double riderDistance, double deltaSeconds, std::vector<WorldActor>& out) override;

private:
	double m_distance;
	double m_phase;
	double m_time;
	bool m_started;
	double m_startAhead;
};

struct WorldCamera
{
	double x = 0.0, y = 0.0, z = 0.0;  // absolute world position (y relative to the profile origin, see GetHeightOrigin)
	double targetX = 1.0, targetY = 0.0, targetZ = 0.0;
	double fieldOfViewDegrees = 62.0;
};

// Maps workout intensity (fraction of FTP) to a cosmetic road grade: recovery is flat or downhill, threshold and above
// climb. Free-ride steps (NaN) give 0.
double GradeForIntensity(double intensity);

// The procedural ride world: route, elevation, biomes, chunk streaming, camera and rider animation. Engine-agnostic -
// the renderer reads its state and draws it.
class RideWorld
{
public:
	RideWorld(const WorldSettings& settings, BiomeList biomes, bool threaded);

	// Applies new settings; geometry is rebuilt only when something that shapes it changed.
	void SetSettings(const WorldSettings& settings);
	const WorldSettings& GetSettings() const;
	void SetActorSource(IWorldActorSource* source);

	void Update(const RideWorldInputs& inputs);

	double GetDistance() const;
	double GetRoadGradePercent() const;  // what SIM should send the trainer
	double GetHeightOrigin() const;      // h(rider): heights are rendered relative to this
	const Route& GetRoute() const;
	const ElevationProfile& GetProfile() const;
	ChunkStreamer& GetStreamer();
	const std::vector<ChunkMesh*>& GetChunks() const;
	std::shared_ptr<const PropTypeTable> GetPropTypes() const;
	BiomeSequence& GetBiomeSequence();
	const std::vector<BiomeDefinition>& GetBiomes() const;
	const BiomeLook& GetLook() const;
	BiomeBlend GetBlend() const;
	const WorldCamera& GetCamera() const;
	const RiderAnimation& GetRiderAnimation() const;
	const std::vector<WorldActor>& GetActors() const;
	const std::vector<double>& GetBanners() const;  // distances of finish banners
	int GetPowerZone() const;
	double GetTimeSeconds() const;
	// Puts a finish banner `aheadMeters` in front of the rider (e.g. when a workout finishes).
	void PlaceBanner(double aheadMeters);
	// Moves the rider to `distance` along the same seeded road (development: screenshots of far-away scenery).
	void JumpTo(double distance);
	// Builds every chunk around the rider now (screenshots, tests).
	void WaitForChunks();

private:
	void Rebuild();
	ChunkSettings MakeChunkSettings() const;
	void FillTargetGrades(const RideWorldInputs& inputs);
	double RouteGradeAt(double distance);
	void UpdateCamera(const RideWorldInputs& inputs);

	WorldSettings m_settings;
	BiomeList m_biomes;
	bool m_threaded;
	std::unique_ptr<Route> m_route;
	std::unique_ptr<BiomeSequence> m_sequence;
	std::unique_ptr<ChunkStreamer> m_streamer;
	std::shared_ptr<const PropTypeTable> m_propTypes;
	ElevationProfile m_profile;
	RiderAnimation m_rider;
	IWorldActorSource* m_actorSource;
	std::vector<WorldActor> m_actors;
	std::vector<double> m_banners;
	std::vector<double> m_targetGrades;
	BiomeLook m_look;
	BiomeBlend m_blend;
	WorldCamera m_camera;
	double m_distance;
	double m_time;
	int m_powerZone;
	double m_smoothedLookX, m_smoothedLookY, m_smoothedLookZ;
	double m_smoothedFov;
	bool m_cameraInitialised;
};
}  // namespace Struktur::RideWorld
