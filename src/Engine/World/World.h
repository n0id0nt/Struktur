#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "Engine/Input/TrainerInput.h"
#include "Engine/World/AssetManifest.h"
#include "Engine/World/Biome.h"
#include "Engine/World/ChunkStreamer.h"
#include "Engine/World/ElevationProfile.h"
#include "Engine/World/RiderAnimation.h"
#include "Engine/World/Route.h"
#include "Engine/World/SpriteDescriptor.h"
#include "Engine/World/WorldSettings.h"

namespace Struktur::World
{
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

// What a world is built from: the data under assets/World (see WorldAssetLoader.h).
struct WorldAssets
{
	BiomeList biomes;
	AssetManifest manifest;
	SpriteDescriptor spriteDescriptor = DefaultSpriteDescriptor();
};

// The procedural world: route, elevation, biomes, chunk streaming, camera and rider animation. It lives on the
// GameContext and is empty until Load(); the renderer reads its state and draws it. Engine-agnostic.
class World
{
public:
	World();
	~World();

	World(const World&)            = delete;
	World& operator=(const World&) = delete;

	// Set once at startup; a replacement applies from the next Load().
	void SetAssets(WorldAssets assets);
	const WorldAssets& GetAssets() const;

	// Builds the world for `settings` (replacing any loaded one) and requests the first chunks. `jobs` builds chunks in
	// the background; null builds them inline (tests).
	void Load(const WorldSettings& settings, Jobs::JobSystem* jobs);
	// Drops the road, chunks and actors, waiting for builds in flight. The assets stay.
	void Unload();
	// Everything below, apart from the assets and settings, needs a loaded world.
	bool IsLoaded() const;

	// Applies new settings; geometry is rebuilt only when something that shapes it changed.
	void SetSettings(const WorldSettings& settings);
	const WorldSettings& GetSettings() const;
	void SetActorSource(std::unique_ptr<IWorldActorSource> source);

	// Advances the road under the rider from what the trainer and the ride report this frame.
	void Update(double deltaSeconds, const Input::TrainerInput& trainer);

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
	void FillTargetGrades(const Input::TrainerInput& trainer);
	double RouteGradeAt(double distance);
	void UpdateCamera(double deltaSeconds, double speedMetersPerSecond);

	WorldAssets m_assets;
	WorldSettings m_settings;
	BiomeList m_biomes;  // the loaded world's
	std::unique_ptr<Route> m_route;
	std::unique_ptr<BiomeSequence> m_sequence;
	std::unique_ptr<ChunkStreamer> m_streamer;
	std::shared_ptr<const PropTypeTable> m_propTypes;
	ElevationProfile m_profile;
	RiderAnimation m_rider;
	std::unique_ptr<IWorldActorSource> m_actorSource;
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
}  // namespace Struktur::World
