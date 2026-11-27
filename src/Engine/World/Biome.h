#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Struktur::World
{
struct Color
{
	float red   = 1.0f;
	float green = 1.0f;
	float blue  = 1.0f;

	static Color Lerp(const Color& from, const Color& to, float weight);
	uint32_t ToAbgr(float alpha = 1.0f) const;
};

enum class TerrainMaterial : uint8_t
{
	GRASS = 0,
	ROCK  = 1,
	SNOW  = 2,
	SAND  = 3,
	WATER = 4,
};

const char* ToString(TerrainMaterial material);

// One palette rule: the first rule whose limits contain the vertex wins.
struct PaletteEntry
{
	TerrainMaterial material = TerrainMaterial::GRASS;
	Color color;
	double minHeight = -1e9;  // relative to the road
	double maxHeight = 1e9;
	double minSlope  = 0.0;   // 0 = flat, 1 = 45 degrees
	double maxSlope  = 1e9;
};

enum class PropPlacement : uint8_t
{
	SCATTER,  // random positions in a lateral band, `density` per 1000 m^2
	EDGE,     // every `spacing` metres at a fixed lateral offset (posts, guardrails, lamps)
};

enum class PropSide : uint8_t
{
	BOTH,
	LEFT,
	RIGHT,
	OUTER,  // the outside of a bend (EDGE props such as guardrails)
};

struct PropRule
{
	std::string role;  // manifest role, e.g. "tree_pine"
	PropPlacement placement = PropPlacement::SCATTER;
	PropSide side           = PropSide::BOTH;
	double density          = 1.0;   // SCATTER: per 1000 m^2
	double spacing          = 40.0;  // EDGE: metres between props
	double minLateral       = 8.0;   // metres from the road centre
	double maxLateral       = 120.0;
	double minHeightMeters  = 4.0;   // world size range
	double maxHeightMeters  = 8.0;
	double maxDistance      = 900.0;  // not drawn further than this ahead
	double minCurvature     = 0.0;    // EDGE: only where the road bends at least this much (1/m)
	bool alignToRoad        = false;  // a quad facing along the road instead of a camera-facing billboard
};

struct BiomeDefinition
{
	std::string id;
	std::string name;
	double weight          = 1.0;  // relative chance in the seeded sequence
	double minLengthMeters = 3000.0;
	double maxLengthMeters = 6000.0;

	Color skyZenith{0.25f, 0.45f, 0.80f};
	Color skyHorizon{0.70f, 0.82f, 0.95f};
	Color skyGround{0.45f, 0.50f, 0.45f};
	Color sunColor{1.0f, 0.95f, 0.85f};
	double timeOfDay      = 14.0;   // hours; sun height follows (night: < 6 or > 19)
	double sunAzimuthDeg  = 200.0;  // relative to the route's mean direction
	double sunSize        = 1.0;
	Color ambient{0.45f, 0.48f, 0.52f};
	double starIntensity  = 0.0;
	double moonIntensity  = 0.0;
	Color fogColor{0.70f, 0.80f, 0.90f};
	double fogDensity     = 0.0018;  // exponential, per metre
	double fogStartMeters = 60.0;
	Color mountainFar{0.55f, 0.62f, 0.75f};
	Color mountainNear{0.40f, 0.48f, 0.55f};
	double mountainHeight = 0.10;   // fraction of the screen above the horizon
	double cloudAmount    = 0.5;

	// Terrain (heights relative to the road).
	double terrainAmplitude = 18.0;
	double terrainScale     = 220.0;  // noise wavelength, metres
	double corridorMeters   = 2.0;    // flat strip beyond the road edge
	double blendMeters      = 45.0;   // corridor -> full noise
	double leftOffset       = 0.0;    // added to the far terrain on each side (e.g. a coast falling away)
	double rightOffset      = 0.0;
	std::vector<PaletteEntry> palette;

	Color roadAsphalt{0.22f, 0.22f, 0.24f};
	Color roadShoulder{0.36f, 0.34f, 0.30f};
	Color roadLine{0.92f, 0.92f, 0.88f};

	bool waterEnabled    = false;
	double waterLevel    = -3.0;  // relative to the road
	PropSide waterSide   = PropSide::BOTH;
	Color waterColor{0.15f, 0.40f, 0.55f};

	std::string weather  = "none";  // none | rain | snow
	double weatherIntensity = 0.0;  // 0 - 1

	double gradeAmplitudePercent = 4.0;    // ROUTE grade source / Free Ride ERG
	double gradeWavelengthMeters = 900.0;

	std::vector<PropRule> props;
};

// Parses one biome file. Unknown keys are ignored, bad values fall back to defaults; returns false (and `error`) only
// when the text isn't a JSON object or has no id.
bool ParseBiome(const std::string& json, BiomeDefinition& out, std::string& error);

using BiomeList = std::shared_ptr<const std::vector<BiomeDefinition>>;

// Where the road is in the biome sequence: biome `from` until `weight` starts rising towards `to`.
struct BiomeBlend
{
	int from      = 0;
	int to        = 0;
	float weight  = 0.0f;  // 0 = all `from`, 1 = all `to`
};

// Interpolated look at a point - sky, fog, light.
struct BiomeLook
{
	Color skyZenith, skyHorizon, skyGround, sunColor, ambient, fogColor, mountainFar, mountainNear, waterColor;
	double timeOfDay      = 14.0;
	double sunAzimuthDeg  = 200.0;
	double sunSize        = 1.0;
	double starIntensity  = 0.0;
	double moonIntensity  = 0.0;
	double fogDensity     = 0.0018;
	double fogStartMeters = 60.0;
	double mountainHeight = 0.1;
	double cloudAmount    = 0.5;
	double rain           = 0.0;
	double snow           = 0.0;
};

BiomeLook LookFor(const std::vector<BiomeDefinition>& biomes, const BiomeBlend& blend);

// The seeded biome sequence along the road: regions of seeded length, each a weighted pick (never the same biome twice
// in a row), with a `transitionMeters` blend into the next. `forcedBiome` (an id) pins the whole road to one biome.
class BiomeSequence
{
public:
	BiomeSequence(BiomeList biomes, uint64_t seed, const std::string& forcedBiome, double transitionMeters = 500.0);

	BiomeBlend BlendAt(double distance);
	const std::vector<BiomeDefinition>& GetBiomes() const;
	const BiomeList& GetBiomeList() const;
	// Start distance of the region containing `distance` (for tests / the debug overlay).
	double RegionStart(double distance);

private:
	void ExtendTo(double distance);
	int PickBiome(int64_t region, int previous) const;

	BiomeList m_biomes;
	uint64_t m_seed;
	int m_forced;  // -1 = none
	double m_transition;
	std::vector<double> m_regionStarts;  // m_regionStarts[i] = start of region i (region 0 starts at 0)
	std::vector<int> m_regionBiomes;
};
}  // namespace Struktur::World
