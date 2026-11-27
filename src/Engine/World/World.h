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
#include "Engine/World/Route.h"
#include "Engine/World/SpriteDescriptor.h"
#include "Engine/World/WorldSettings.h"

namespace Struktur::World
{
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

// The procedural world: route, elevation, biomes and chunk streaming. It lives on the GameContext and is empty until
// Load(); what moves in it (the rider, pacers, banners, the camera) are ECS entities (see WorldLifecycle.h), and the
// renderer draws both. Engine-agnostic.
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

	// Advances the road under the rider from what the trainer and the ride report this frame.
	void Update(double deltaSeconds, const Input::TrainerInput& trainer);

	double GetDistance() const;
	double GetRoadGradePercent() const;  // what SIM should send the trainer
	double GetHeightOrigin() const;      // h(rider): heights are rendered relative to this
	const Route& GetRoute() const;
	const ElevationProfile& GetProfile() const;
	ChunkStreamer& GetStreamer();
	const std::vector<ChunkMesh*>& GetChunks() const;
	const PropTypeTable& GetPropTypes() const;
	BiomeSequence& GetBiomeSequence();
	const std::vector<BiomeDefinition>& GetBiomes() const;
	const BiomeLook& GetLook() const;
	BiomeBlend GetBlend() const;
	// Goes up every time the road is rebuilt (a load, a jump, new road settings): followers snap rather than glide.
	uint32_t GetRoadRevision() const;
	double GetTimeSeconds() const;
	// Moves the rider to `distance` along the same seeded road (development: screenshots of far-away scenery).
	void JumpTo(double distance);
	// Builds every chunk around the rider now (screenshots, tests).
	void WaitForChunks();

private:
	void Rebuild();
	ChunkSettings MakeChunkSettings() const;
	void FillTargetGrades(const Input::TrainerInput& trainer);
	double RouteGradeAt(double distance);

	WorldAssets m_assets;
	WorldSettings m_settings;
	// The loaded world's own copy, fixed from Load() to Unload(): the sequence, the streamer and its builds point into
	// these two, so the streamer (which waits for builds) is always released first.
	BiomeList m_biomes;
	PropTypeTable m_propTypes;
	std::unique_ptr<Route> m_route;
	std::unique_ptr<BiomeSequence> m_sequence;
	std::unique_ptr<ChunkStreamer> m_streamer;
	ElevationProfile m_profile;
	std::vector<double> m_targetGrades;
	BiomeLook m_look;
	BiomeBlend m_blend;
	double m_distance;
	double m_time;
	uint32_t m_roadRevision = 0;
};
}  // namespace Struktur::World
