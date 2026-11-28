#include "ChunkBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "World/Noise.h"

namespace Struktur::World
{
static constexpr double ROAD_SURFACE_DROP     = 0.08;   // terrain under/beside the road sits this far below it
static constexpr double CURVATURE_WINDOW      = 12.0;   // metres either side when finding the local bend radius
static constexpr double DISABLED_WATER_LEVEL  = -60.0;  // a biome without water: its level sinks out of sight
static constexpr size_t MAX_PROPS_PER_TYPE    = 1024;
static constexpr double WATER_ROW_SPACING     = 8.0;

PropTypeTable PropTypeTable::FromBiomes(const std::vector<BiomeDefinition>& biomes)
{
	PropTypeTable table;
	for (const BiomeDefinition& biome : biomes)
	{
		for (const PropRule& rule : biome.props)
		{
			const int existing = table.IndexOf(rule.role);
			if (existing < 0)
			{
				table.roles.push_back(rule.role);
				table.maxDistance.push_back(rule.maxDistance);
				table.alignToRoad.push_back(rule.alignToRoad);
			}
			else
			{
				table.maxDistance[static_cast<size_t>(existing)] =
				    std::max(table.maxDistance[static_cast<size_t>(existing)], rule.maxDistance);
			}
		}
	}
	return table;
}

int PropTypeTable::IndexOf(const std::string& role) const
{
	for (size_t index = 0; index < roles.size(); ++index)
	{
		if (roles[index] == role)
		{
			return static_cast<int>(index);
		}
	}
	return -1;
}

std::vector<double> TerrainColumnOffsets(double lateralExtent)
{
	static constexpr double OFFSETS[] = {0.0,  1.5,   3.5,   6.0,   10.0,  15.0,  22.0,  31.0,  43.0,
	                                     58.0, 78.0,  104.0, 138.0, 182.0, 240.0, 315.0, 410.0, 530.0};
	std::vector<double> offsets;
	for (double offset : OFFSETS)
	{
		if (offset < lateralExtent)
		{
			offsets.push_back(offset);
		}
	}
	offsets.push_back(lateralExtent);
	return offsets;
}

static double SmoothStep01(double value)
{
	const double clamped = std::clamp(value, 0.0, 1.0);
	return clamped * clamped * (3.0 - 2.0 * clamped);
}

double TerrainHeight(const BiomeDefinition& biome, uint64_t seed, double worldX, double worldZ, double lateral,
                     double roadEdge)
{
	const double beyondEdge = std::abs(lateral) - roadEdge;
	if (beyondEdge <= biome.corridorMeters)
	{
		return -ROAD_SURFACE_DROP;
	}
	const double rise   = SmoothStep01((beyondEdge - biome.corridorMeters) / biome.blendMeters);
	const double noise  = Fbm2D(seed ^ 0x7E44u, worldX / biome.terrainScale, worldZ / biome.terrainScale, 4);
	// Bias upwards so hills dominate dips (dips below the road read as holes at low camera height).
	const double hills  = biome.terrainAmplitude * (0.35 + 0.65 * noise);
	const double offset = (lateral > 0.0 ? biome.leftOffset : biome.rightOffset) *
	                      SmoothStep01((beyondEdge - biome.corridorMeters) / (biome.blendMeters * 2.5));
	return -ROAD_SURFACE_DROP + rise * hills + offset;
}

static RouteSample RouteAt(const ChunkJob& job, double distance)
{
	RouteSample result;
	if (job.samples.empty())
	{
		return result;
	}
	const double position = std::clamp(distance / job.sampleSpacing - static_cast<double>(job.firstSample), 0.0,
	                                   static_cast<double>(job.samples.size() - 1));
	const size_t index    = static_cast<size_t>(std::floor(position));
	const size_t next     = std::min(index + 1, job.samples.size() - 1);
	const double weight   = position - static_cast<double>(index);
	const RouteSample& before = job.samples[index];
	const RouteSample& after  = job.samples[next];
	result.x              = before.x + (after.x - before.x) * weight;
	result.z              = before.z + (after.z - before.z) * weight;
	double tangentX       = before.tangentX + (after.tangentX - before.tangentX) * weight;
	double tangentZ       = before.tangentZ + (after.tangentZ - before.tangentZ) * weight;
	const double length   = std::hypot(tangentX, tangentZ);
	if (length > 0.0)
	{
		tangentX /= length;
		tangentZ /= length;
	}
	result.tangentX  = tangentX;
	result.tangentZ  = tangentZ;
	result.curvature = before.curvature + (after.curvature - before.curvature) * weight;
	return result;
}

// How far the terrain may reach on each side at `distance`: inside a bend, a fraction of the tightest radius nearby.
static void LateralLimits(const ChunkJob& job, double distance, double& leftLimit, double& rightLimit)
{
	double leftCurvature  = 0.0;
	double rightCurvature = 0.0;
	for (double offset = -CURVATURE_WINDOW; offset <= CURVATURE_WINDOW; offset += job.sampleSpacing)
	{
		const double curvature = RouteAt(job, distance + offset).curvature;
		leftCurvature          = std::max(leftCurvature, curvature);
		rightCurvature         = std::max(rightCurvature, -curvature);
	}
	const double fraction = job.settings.innerRadiusFraction;
	leftLimit             = leftCurvature > 1e-6 ? fraction / leftCurvature : 1e9;
	rightLimit            = rightCurvature > 1e-6 ? fraction / rightCurvature : 1e9;
}

static const BiomeDefinition& BiomeAt(const ChunkJob& job, int index)
{
	const std::vector<BiomeDefinition>& biomes = *job.biomes;
	return biomes[static_cast<size_t>(std::clamp(index, 0, static_cast<int>(biomes.size()) - 1))];
}

static double BlendedHeight(const ChunkJob& job, const BiomeBlend& blend, double worldX, double worldZ, double lateral)
{
	const double roadEdge = job.settings.roadHalfWidth + job.settings.shoulder;
	const double from     = TerrainHeight(BiomeAt(job, blend.from), job.settings.seed, worldX, worldZ, lateral, roadEdge);
	if (blend.weight <= 0.0f || blend.from == blend.to)
	{
		return from;
	}
	const double to = TerrainHeight(BiomeAt(job, blend.to), job.settings.seed, worldX, worldZ, lateral, roadEdge);
	return from + (to - from) * blend.weight;
}

static const PaletteEntry& PickPalette(const BiomeDefinition& biome, double height, double slope)
{
	for (const PaletteEntry& entry : biome.palette)
	{
		if (height >= entry.minHeight && height <= entry.maxHeight && slope >= entry.minSlope && slope <= entry.maxSlope)
		{
			return entry;
		}
	}
	return biome.palette.back();
}

static void Normalise(float& x, float& y, float& z)
{
	const float length = std::sqrt(x * x + y * y + z * z);
	if (length > 1e-6f)
	{
		x /= length;
		y /= length;
		z /= length;
	}
	else
	{
		x = 0.0f;
		y = 1.0f;
		z = 0.0f;
	}
}

static WorldVertex MakeVertex(float x, float y, float z, float normalX, float normalY, float normalZ, uint32_t abgr,
                              float distance, float lateral, TerrainMaterial material)
{
	WorldVertex vertex;
	vertex.x        = x;
	vertex.y        = y;
	vertex.z        = z;
	vertex.normalX  = normalX;
	vertex.normalY  = normalY;
	vertex.normalZ  = normalZ;
	vertex.abgr     = abgr;
	vertex.distance = distance;
	vertex.lateral  = lateral;
	vertex.material = static_cast<float>(material);
	return vertex;
}

static void BuildTerrain(const ChunkJob& job, ChunkMesh& out)
{
	const ChunkSettings& settings = job.settings;
	const double roadEdge         = settings.roadHalfWidth + settings.shoulder;
	const std::vector<double> offsets = TerrainColumnOffsets(settings.lateralExtent);
	const int sideColumns             = static_cast<int>(offsets.size());
	const int columns                 = sideColumns * 2 + 1;
	const int rows                    = static_cast<int>(std::lround(settings.chunkLength / settings.rowSpacing)) + 1;
	out.terrainRows                   = rows;
	out.terrainColumns                = columns;

	const size_t count = static_cast<size_t>(rows * columns);
	std::vector<float> heights(count);
	std::vector<float> lateral(count);
	out.gridXZ.resize(count * 2);

	for (int row = 0; row < rows; ++row)
	{
		const double distance = job.start + row * settings.rowSpacing;
		const RouteSample sample = RouteAt(job, distance);
		double leftLimit         = 0.0;
		double rightLimit        = 0.0;
		LateralLimits(job, distance, leftLimit, rightLimit);
		const BiomeBlend& blend = job.rowBlends[static_cast<size_t>(std::min(row, static_cast<int>(job.rowBlends.size()) - 1))];
		for (int column = 0; column < columns; ++column)
		{
			double offset;
			if (column < sideColumns)
			{
				// Right side, outermost first.
				offset = -std::min(roadEdge + offsets[static_cast<size_t>(sideColumns - 1 - column)], rightLimit);
			}
			else if (column == sideColumns)
			{
				offset = 0.0;
			}
			else
			{
				offset = std::min(roadEdge + offsets[static_cast<size_t>(column - sideColumns - 1)], leftLimit);
			}
			const double worldX = sample.x + sample.LeftX() * offset;
			const double worldZ = sample.z + sample.LeftZ() * offset;
			const size_t cell   = static_cast<size_t>(row * columns + column);
			heights[cell]       = static_cast<float>(BlendedHeight(job, blend, worldX, worldZ, offset));
			lateral[cell]       = static_cast<float>(offset);
			out.gridXZ[cell * 2]     = static_cast<float>(worldX - out.originX);
			out.gridXZ[cell * 2 + 1] = static_cast<float>(worldZ - out.originZ);
		}
	}

	// Smooth normals from the grid (central differences; one-sided at the borders).
	std::vector<float> normals(count * 3);
	for (int row = 0; row < rows; ++row)
	{
		for (int column = 0; column < columns; ++column)
		{
			const int rowA    = std::max(row - 1, 0);
			const int rowB    = std::min(row + 1, rows - 1);
			const int columnA = std::max(column - 1, 0);
			const int columnB = std::min(column + 1, columns - 1);
			const size_t behind = static_cast<size_t>(rowA * columns + column);
			const size_t ahead  = static_cast<size_t>(rowB * columns + column);
			const size_t right  = static_cast<size_t>(row * columns + columnA);
			const size_t left   = static_cast<size_t>(row * columns + columnB);
			// du: along the road, dv: towards the left.
			const float duX   = out.gridXZ[ahead * 2] - out.gridXZ[behind * 2];
			const float duY   = heights[ahead] - heights[behind];
			const float duZ   = out.gridXZ[ahead * 2 + 1] - out.gridXZ[behind * 2 + 1];
			const float dvX   = out.gridXZ[left * 2] - out.gridXZ[right * 2];
			const float dvY   = heights[left] - heights[right];
			const float dvZ   = out.gridXZ[left * 2 + 1] - out.gridXZ[right * 2 + 1];
			float normalX     = duY * dvZ - duZ * dvY;
			float normalY     = duZ * dvX - duX * dvZ;
			float normalZ     = duX * dvY - duY * dvX;
			if (normalY < 0.0f)
			{
				normalX = -normalX;
				normalY = -normalY;
				normalZ = -normalZ;
			}
			Normalise(normalX, normalY, normalZ);
			const size_t cell      = static_cast<size_t>(row * columns + column);
			normals[cell * 3]      = normalX;
			normals[cell * 3 + 1]  = normalY;
			normals[cell * 3 + 2]  = normalZ;
		}
	}

	// Colours and materials from the biome palettes.
	std::vector<uint32_t> colors(count);
	std::vector<float> materials(count);
	for (int row = 0; row < rows; ++row)
	{
		const BiomeBlend& blend = job.rowBlends[static_cast<size_t>(std::min(row, static_cast<int>(job.rowBlends.size()) - 1))];
		for (int column = 0; column < columns; ++column)
		{
			const size_t cell       = static_cast<size_t>(row * columns + column);
			const double normalY    = std::max(0.05, static_cast<double>(normals[cell * 3 + 1]));
			const double slope      = std::sqrt(std::max(0.0, 1.0 - normalY * normalY)) / normalY;
			const PaletteEntry& fromEntry = PickPalette(BiomeAt(job, blend.from), heights[cell], slope);
			const PaletteEntry& toEntry   = PickPalette(BiomeAt(job, blend.to), heights[cell], slope);
			Color color                   = Color::Lerp(fromEntry.color, toEntry.color, blend.weight);
			// Small per-vertex variation so flat areas aren't a single colour.
			const float variation =
			    0.92f + 0.16f * static_cast<float>(HashUnit(job.settings.seed ^ 0xC010u, job.chunkIndex * 4096 + row, column));
			color.red *= variation;
			color.green *= variation;
			color.blue *= variation;
			colors[cell]    = color.ToAbgr();
			materials[cell] = static_cast<float>(blend.weight < 0.5f ? fromEntry.material : toEntry.material);
		}
	}

	out.terrain.clear();
	out.terrainIndices.clear();
	out.flat = settings.flatShading;
	if (!settings.flatShading)
	{
		out.terrain.reserve(count);
		for (int row = 0; row < rows; ++row)
		{
			for (int column = 0; column < columns; ++column)
			{
				const size_t cell = static_cast<size_t>(row * columns + column);
				out.terrain.push_back(MakeVertex(out.gridXZ[cell * 2], heights[cell], out.gridXZ[cell * 2 + 1],
				                                 normals[cell * 3], normals[cell * 3 + 1], normals[cell * 3 + 2],
				                                 colors[cell], static_cast<float>(row * settings.rowSpacing),
				                                 lateral[cell], static_cast<TerrainMaterial>(static_cast<int>(materials[cell]))));
			}
		}
		for (int row = 0; row + 1 < rows; ++row)
		{
			for (int column = 0; column + 1 < columns; ++column)
			{
				const uint16_t corner0 = static_cast<uint16_t>(row * columns + column);
				const uint16_t corner1 = static_cast<uint16_t>((row + 1) * columns + column);
				const uint16_t corner2 = static_cast<uint16_t>((row + 1) * columns + column + 1);
				const uint16_t corner3 = static_cast<uint16_t>(row * columns + column + 1);
				out.terrainIndices.insert(out.terrainIndices.end(), {corner0, corner1, corner2, corner0, corner2, corner3});
			}
		}
		return;
	}

	// Flat shading: three vertices per triangle, the triangle's own normal and averaged colour.
	out.terrain.reserve(static_cast<size_t>((rows - 1) * (columns - 1) * 6));
	for (int row = 0; row + 1 < rows; ++row)
	{
		for (int column = 0; column + 1 < columns; ++column)
		{
			const size_t corners[2][3] = {
			    {static_cast<size_t>(row * columns + column), static_cast<size_t>((row + 1) * columns + column),
			     static_cast<size_t>((row + 1) * columns + column + 1)},
			    {static_cast<size_t>(row * columns + column), static_cast<size_t>((row + 1) * columns + column + 1),
			     static_cast<size_t>(row * columns + column + 1)}};
			for (const auto& triangle : corners)
			{
				const size_t p0 = triangle[0];
				const size_t p1 = triangle[1];
				const size_t p2 = triangle[2];
				const float e1X = out.gridXZ[p1 * 2] - out.gridXZ[p0 * 2];
				const float e1Y = heights[p1] - heights[p0];
				const float e1Z = out.gridXZ[p1 * 2 + 1] - out.gridXZ[p0 * 2 + 1];
				const float e2X = out.gridXZ[p2 * 2] - out.gridXZ[p0 * 2];
				const float e2Y = heights[p2] - heights[p0];
				const float e2Z = out.gridXZ[p2 * 2 + 1] - out.gridXZ[p0 * 2 + 1];
				float normalX   = e1Y * e2Z - e1Z * e2Y;
				float normalY   = e1Z * e2X - e1X * e2Z;
				float normalZ   = e1X * e2Y - e1Y * e2X;
				if (normalY < 0.0f)
				{
					normalX = -normalX;
					normalY = -normalY;
					normalZ = -normalZ;
				}
				Normalise(normalX, normalY, normalZ);
				// Average the three colours channel by channel.
				uint32_t channels[4] = {0, 0, 0, 0};
				for (size_t corner : triangle)
				{
					for (int channel = 0; channel < 4; ++channel)
					{
						channels[channel] += (colors[corner] >> (channel * 8)) & 0xFFu;
					}
				}
				const uint32_t averaged = (channels[0] / 3) | ((channels[1] / 3) << 8) | ((channels[2] / 3) << 16) |
				                          ((channels[3] / 3) << 24);
				for (size_t corner : triangle)
				{
					const int cornerRow = static_cast<int>(corner) / columns;
					out.terrain.push_back(MakeVertex(out.gridXZ[corner * 2], heights[corner], out.gridXZ[corner * 2 + 1],
					                                 normalX, normalY, normalZ, averaged,
					                                 static_cast<float>(cornerRow * settings.rowSpacing), lateral[corner],
					                                 static_cast<TerrainMaterial>(static_cast<int>(materials[p0]))));
				}
			}
		}
	}
	for (size_t index = 0; index < out.terrain.size(); ++index)
	{
		out.terrainIndices.push_back(static_cast<uint16_t>(index));
	}
}

static void BuildRoad(const ChunkJob& job, ChunkMesh& out)
{
	const ChunkSettings& settings = job.settings;
	const double roadEdge         = settings.roadHalfWidth + settings.shoulder;
	const int rows                = static_cast<int>(std::lround(settings.chunkLength / settings.rowSpacing)) + 1;
	out.road.clear();
	out.roadIndices.clear();
	for (int row = 0; row < rows; ++row)
	{
		const double distance    = job.start + row * settings.rowSpacing;
		const RouteSample sample = RouteAt(job, distance);
		const BiomeBlend& blend  = job.rowBlends[static_cast<size_t>(std::min(row, static_cast<int>(job.rowBlends.size()) - 1))];
		const Color asphalt =
		    Color::Lerp(BiomeAt(job, blend.from).roadAsphalt, BiomeAt(job, blend.to).roadAsphalt, blend.weight);
		for (int side = 0; side < 2; ++side)
		{
			const double offset = side == 0 ? -roadEdge : roadEdge;
			const double worldX = sample.x + sample.LeftX() * offset;
			const double worldZ = sample.z + sample.LeftZ() * offset;
			out.road.push_back(MakeVertex(static_cast<float>(worldX - out.originX), 0.0f,
			                              static_cast<float>(worldZ - out.originZ), 0.0f, 1.0f, 0.0f, asphalt.ToAbgr(),
			                              static_cast<float>(row * settings.rowSpacing), static_cast<float>(offset),
			                              TerrainMaterial::GRASS));
		}
	}
	for (int row = 0; row + 1 < rows; ++row)
	{
		const uint16_t corner0 = static_cast<uint16_t>(row * 2);
		const uint16_t corner1 = static_cast<uint16_t>(row * 2 + 1);
		const uint16_t corner2 = static_cast<uint16_t>(row * 2 + 3);
		const uint16_t corner3 = static_cast<uint16_t>(row * 2 + 2);
		out.roadIndices.insert(out.roadIndices.end(), {corner0, corner3, corner2, corner0, corner2, corner1});
	}
}

static double WaterLevel(const BiomeDefinition& biome)
{
	return biome.waterEnabled ? biome.waterLevel : DISABLED_WATER_LEVEL;
}

static bool WaterOnSide(const BiomeDefinition& biome, int side)
{
	if (!biome.waterEnabled)
	{
		return false;
	}
	return biome.waterSide == PropSide::BOTH || (side > 0 && biome.waterSide == PropSide::LEFT) ||
	       (side < 0 && biome.waterSide == PropSide::RIGHT);
}

static void BuildWater(const ChunkJob& job, ChunkMesh& out)
{
	out.water.clear();
	out.waterIndices.clear();
	const ChunkSettings& settings = job.settings;
	const double roadEdge         = settings.roadHalfWidth + settings.shoulder;
	const int rows                = static_cast<int>(std::lround(settings.chunkLength / WATER_ROW_SPACING)) + 1;
	for (int side = -1; side <= 1; side += 2)
	{
		bool any = false;
		for (const BiomeBlend& blend : job.rowBlends)
		{
			any = any || WaterOnSide(BiomeAt(job, blend.from), side) || WaterOnSide(BiomeAt(job, blend.to), side);
		}
		if (!any)
		{
			continue;
		}
		const uint16_t base = static_cast<uint16_t>(out.water.size());
		for (int row = 0; row < rows; ++row)
		{
			const double distance    = job.start + row * WATER_ROW_SPACING;
			const RouteSample sample = RouteAt(job, distance);
			const size_t blendRow    = std::min(static_cast<size_t>(row * WATER_ROW_SPACING / settings.rowSpacing),
			                                    job.rowBlends.size() - 1);
			const BiomeBlend& blend  = job.rowBlends[blendRow];
			const BiomeDefinition& fromBiome = BiomeAt(job, blend.from);
			const BiomeDefinition& toBiome   = BiomeAt(job, blend.to);
			const double levelA = WaterOnSide(fromBiome, side) ? WaterLevel(fromBiome) : DISABLED_WATER_LEVEL;
			const double levelB = WaterOnSide(toBiome, side) ? WaterLevel(toBiome) : DISABLED_WATER_LEVEL;
			const float level   = static_cast<float>(levelA + (levelB - levelA) * blend.weight);
			const Color color   = Color::Lerp(fromBiome.waterColor, toBiome.waterColor, blend.weight);
			double leftLimit         = 0.0;
			double rightLimit        = 0.0;
			LateralLimits(job, distance, leftLimit, rightLimit);
			const double limit = side > 0 ? leftLimit : rightLimit;
			const double inner = std::min(roadEdge + 1.0, limit);
			const double outer = std::min(settings.lateralExtent + roadEdge, limit);
			for (int edge = 0; edge < 2; ++edge)
			{
				const double offset = side * (edge == 0 ? inner : outer);
				const double worldX = sample.x + sample.LeftX() * offset;
				const double worldZ = sample.z + sample.LeftZ() * offset;
				out.water.push_back(MakeVertex(static_cast<float>(worldX - out.originX), level,
				                               static_cast<float>(worldZ - out.originZ), 0.0f, 1.0f, 0.0f,
				                               color.ToAbgr(), static_cast<float>(row * WATER_ROW_SPACING),
				                               static_cast<float>(offset), TerrainMaterial::WATER));
			}
		}
		for (int row = 0; row + 1 < rows; ++row)
		{
			const uint16_t corner0 = static_cast<uint16_t>(base + row * 2);
			const uint16_t corner1 = static_cast<uint16_t>(base + row * 2 + 1);
			const uint16_t corner2 = static_cast<uint16_t>(base + row * 2 + 3);
			const uint16_t corner3 = static_cast<uint16_t>(base + row * 2 + 2);
			out.waterIndices.insert(out.waterIndices.end(), {corner0, corner3, corner2, corner0, corner2, corner1});
		}
	}
}

static void AddProp(const ChunkJob& job, ChunkMesh& out, int typeIndex, const BiomeBlend& blend, double distance,
                    double offset, double heightMeters, double variant, double tint)
{
	std::vector<PropInstance>& list = out.props[static_cast<size_t>(typeIndex)];
	if (list.size() >= MAX_PROPS_PER_TYPE)
	{
		return;
	}
	const RouteSample sample = RouteAt(job, distance);
	const double worldX      = sample.x + sample.LeftX() * offset;
	const double worldZ      = sample.z + sample.LeftZ() * offset;
	PropInstance instance;
	instance.x            = static_cast<float>(worldX - out.originX);
	instance.y            = static_cast<float>(BlendedHeight(job, blend, worldX, worldZ, offset));
	instance.z            = static_cast<float>(worldZ - out.originZ);
	instance.distance     = static_cast<float>(distance - job.start);
	instance.heightMeters = static_cast<float>(heightMeters);
	instance.angle        = static_cast<float>(std::atan2(sample.tangentZ, sample.tangentX));
	instance.variant      = static_cast<float>(variant);
	instance.tint         = static_cast<float>(tint);
	list.push_back(instance);
}

static bool InsideLimits(const ChunkJob& job, double distance, double offset)
{
	double leftLimit  = 0.0;
	double rightLimit = 0.0;
	LateralLimits(job, distance, leftLimit, rightLimit);
	return offset > 0.0 ? offset < leftLimit : -offset < rightLimit;
}

static void BuildProps(const ChunkJob& job, ChunkMesh& out)
{
	const PropTypeTable& table = *job.propTypes;
	out.props.resize(table.roles.size());
	for (std::vector<PropInstance>& list : out.props)
	{
		list.clear();
	}
	const ChunkSettings& settings = job.settings;
	const double roadEdge         = settings.roadHalfWidth + settings.shoulder;
	const BiomeBlend& middle      = job.rowBlends[job.rowBlends.size() / 2];
	const int biomeIndices[2]     = {middle.from, middle.to};
	const double biomeWeights[2]  = {1.0 - middle.weight, middle.weight};
	for (int pick = 0; pick < 2; ++pick)
	{
		if (biomeWeights[pick] <= 0.0 || (pick == 1 && middle.from == middle.to))
		{
			continue;
		}
		const BiomeDefinition& biome = BiomeAt(job, biomeIndices[pick]);
		for (size_t ruleIndex = 0; ruleIndex < biome.props.size(); ++ruleIndex)
		{
			const PropRule& rule = biome.props[ruleIndex];
			const int typeIndex  = table.IndexOf(rule.role);
			if (typeIndex < 0)
			{
				continue;
			}
			Random random(Hash(settings.seed ^ 0x9209u, job.chunkIndex, biomeIndices[pick], static_cast<int64_t>(ruleIndex)));
			if (rule.placement == PropPlacement::SCATTER)
			{
				const int sides       = rule.side == PropSide::BOTH ? 2 : 1;
				const double area     = settings.chunkLength * (rule.maxLateral - rule.minLateral) * sides;
				const double expected = rule.density * area / 1000.0 * biomeWeights[pick] * settings.propDensity;
				int count             = static_cast<int>(std::floor(expected));
				if (random.NextUnit() < expected - count)
				{
					++count;
				}
				for (int index = 0; index < count; ++index)
				{
					const double distance = job.start + random.NextUnit() * settings.chunkLength;
					double sign           = rule.side == PropSide::LEFT ? 1.0 : -1.0;
					if (rule.side == PropSide::BOTH)
					{
						sign = random.NextUnit() < 0.5 ? 1.0 : -1.0;
					}
					const double lateral = std::max(rule.minLateral, roadEdge + 1.0) +
					                       random.NextUnit() * std::max(0.0, rule.maxLateral - std::max(rule.minLateral, roadEdge + 1.0));
					const double height  = random.Range(rule.minHeightMeters, rule.maxHeightMeters);
					const double variant = random.NextUnit();
					const double tint    = random.NextUnit();
					if (!InsideLimits(job, distance, sign * lateral))
					{
						continue;
					}
					AddProp(job, out, typeIndex, middle, distance, sign * lateral, height, variant, tint);
				}
			}
			else
			{
				const double first = std::ceil(job.start / rule.spacing) * rule.spacing;
				for (double distance = first; distance < job.start + settings.chunkLength; distance += rule.spacing)
				{
					const int64_t slot     = static_cast<int64_t>(std::llround(distance / rule.spacing));
					const double curvature = RouteAt(job, distance).curvature;
					if (std::abs(curvature) < rule.minCurvature)
					{
						continue;
					}
					double signs[2] = {1.0, -1.0};
					int signCount   = 2;
					if (rule.side == PropSide::LEFT)
					{
						signCount = 1;
					}
					else if (rule.side == PropSide::RIGHT)
					{
						signs[0]  = -1.0;
						signCount = 1;
					}
					else if (rule.side == PropSide::OUTER)
					{
						signs[0]  = curvature > 0.0 ? -1.0 : 1.0;  // bending left: outside is the right
						signCount = 1;
					}
					for (int signIndex = 0; signIndex < signCount; ++signIndex)
					{
						const double lateral = signs[signIndex] * std::max(rule.minLateral, roadEdge + 0.3);
						if (!InsideLimits(job, distance, lateral))
						{
							continue;
						}
						const double unit   = HashUnit(settings.seed ^ 0xED6Eu, slot, typeIndex, signIndex);
						const double height = rule.minHeightMeters + (rule.maxHeightMeters - rule.minHeightMeters) * unit;
						AddProp(job, out, typeIndex, middle, distance, lateral, height, unit, 0.5);
					}
				}
			}
		}
	}
}

void BuildChunk(const ChunkJob& job, ChunkMesh& out)
{
	const auto started = std::chrono::steady_clock::now();
	out.chunkIndex     = job.chunkIndex;
	out.start          = job.start;
	const RouteSample origin = RouteAt(job, job.start);
	out.originX              = origin.x;
	out.originZ              = origin.z;
	if (job.biomes && !job.biomes->empty() && !job.rowBlends.empty() && job.propTypes)
	{
		BuildTerrain(job, out);
		BuildRoad(job, out);
		BuildWater(job, out);
		BuildProps(job, out);
	}
	out.buildMilliseconds =
	    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
}
}  // namespace Struktur::World
