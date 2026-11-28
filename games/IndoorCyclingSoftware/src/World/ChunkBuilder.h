#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "World/Biome.h"
#include "World/Route.h"

namespace Struktur::World
{
// GPU vertex for terrain, road and water (40 bytes). Positions are relative to the chunk origin; y is the height
// relative to the ROAD at that distance - the road's own elevation h(s) is added in the vertex shader from a height
// texture, so the profile ahead can change after the chunk is built.
struct WorldVertex
{
	float x, y, z;
	float normalX, normalY, normalZ;
	uint32_t abgr;
	float distance;  // metres along the road from the chunk start
	float lateral;   // t, metres left of the centre line
	float material;  // TerrainMaterial
};

// One prop instance (32 bytes = two vec4 of instance data).
struct PropInstance
{
	float x, y, z;   // relative to the chunk origin; y relative to the road height
	float distance;  // metres along the road from the chunk start (for the height lookup)
	float heightMeters;
	float angle;     // radians, for road-aligned props
	float variant;   // 0 - 1, picks a frame of the prop's sheet
	float tint;      // 0 - 1 brightness variation
};

// Every prop role any biome uses, in a fixed order (one instanced draw per type per chunk).
struct PropTypeTable
{
	std::vector<std::string> roles;
	std::vector<double> maxDistance;
	std::vector<bool> alignToRoad;

	static PropTypeTable FromBiomes(const std::vector<BiomeDefinition>& biomes);
	int IndexOf(const std::string& role) const;
};

struct ChunkSettings
{
	uint64_t seed            = 1;
	double chunkLength       = 64.0;
	double rowSpacing        = 2.0;
	double lateralExtent     = 320.0;
	double roadHalfWidth     = 3.5;
	double shoulder          = 1.2;
	double propDensity       = 0.7;
	bool flatShading         = true;
	// Inside a bend the terrain is cut off at this fraction of the local radius so it can't fold over itself.
	double innerRadiusFraction = 0.7;
};

// Everything a worker thread needs to build one chunk - copies only, never the live route.
struct ChunkJob
{
	int64_t chunkIndex = 0;
	double start       = 0.0;  // distance along the road
	ChunkSettings settings;
	std::vector<RouteSample> samples;  // route samples [firstSample, firstSample + count)
	int64_t firstSample   = 0;
	double sampleSpacing  = 1.0;
	std::vector<BiomeBlend> rowBlends;  // one per terrain row
	// Owned by the World, which waits for every build before it releases them (World::Unload).
	const BiomeList* biomes          = nullptr;
	const PropTypeTable* propTypes   = nullptr;
};

struct ChunkMesh
{
	int64_t chunkIndex = -1;
	double start       = 0.0;
	double originX     = 0.0;
	double originZ     = 0.0;
	int terrainRows    = 0;
	int terrainColumns = 0;
	bool flat          = false;  // terrain vertices are per-triangle (flat shading) instead of a grid
	std::vector<WorldVertex> terrain;
	std::vector<uint16_t> terrainIndices;
	std::vector<WorldVertex> road;
	std::vector<uint16_t> roadIndices;
	std::vector<WorldVertex> water;
	std::vector<uint16_t> waterIndices;
	std::vector<std::vector<PropInstance>> props;  // indexed like PropTypeTable::roles
	double buildMilliseconds = 0.0;
	// The grid positions (x, z relative to the origin) before flat-shading expansion - for the self-intersection test.
	std::vector<float> gridXZ;
};

// Terrain height relative to the road at a world point (x, z) that lies `lateral` metres from the centre line.
double TerrainHeight(const BiomeDefinition& biome, uint64_t seed, double worldX, double worldZ, double lateral,
                     double roadEdge);

// Builds terrain, road ribbon, water and props for one chunk. Pure: same job, same mesh. Reuses `out`'s buffers.
void BuildChunk(const ChunkJob& job, ChunkMesh& out);

// The lateral offsets of the terrain columns on one side (from the road edge outwards), shared by the tests.
std::vector<double> TerrainColumnOffsets(double lateralExtent);
}  // namespace Struktur::World
