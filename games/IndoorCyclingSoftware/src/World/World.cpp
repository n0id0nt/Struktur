#include "World.h"

#include <algorithm>
#include <cmath>

#include "World/Noise.h"

namespace Struktur::World
{
static constexpr double PI                     = 3.14159265358979323846;
static constexpr double CHUNKS_BEHIND_METERS   = 64.0;
static constexpr double HISTORY_METERS         = 200.0;
static constexpr double PROFILE_MARGIN_METERS  = 128.0;
static constexpr double MIN_LOOKAHEAD_SPEED    = 4.0;  // m/s - converts distance ahead into workout time ahead

double GradeForIntensity(double intensity)
{
	if (std::isnan(intensity))
	{
		return 0.0;
	}
	return std::clamp((intensity - 0.65) * 18.0, -3.0, 9.0);
}

World::World()
{
	Unload();
}

World::~World()
{
	Unload();
}

void World::SetAssets(WorldAssets assets)
{
	m_assets = std::move(assets);
}

const WorldAssets& World::GetAssets() const
{
	return m_assets;
}

void World::Load(const WorldSettings& settings, Jobs::JobSystem* jobs)
{
	Unload();
	m_settings = settings;
	m_settings.Validate();
	m_biomes    = m_assets.biomes;
	m_propTypes = PropTypeTable::FromBiomes(m_biomes);
	m_streamer = std::make_unique<ChunkStreamer>(jobs);
	Rebuild();
	// A standing frame, so the look and first chunk requests are valid before the first real one.
	Update(0.0, Input::TrainerInput());
}

void World::Unload()
{
	m_streamer.reset();  // waits for the builds that write into its meshes
	m_sequence.reset();
	m_route.reset();
	m_propTypes = PropTypeTable();
	m_biomes.clear();
	m_targetGrades.clear();
	m_profile           = ElevationProfile();
	m_look              = BiomeLook();
	m_blend             = BiomeBlend();
	m_distance          = 0.0;
	m_time              = 0.0;
}

bool World::IsLoaded() const
{
	return m_streamer != nullptr;
}

ChunkSettings World::MakeChunkSettings() const
{
	const QualityParameters quality = m_settings.Resolved();
	ChunkSettings settings;
	settings.seed          = m_settings.seed;
	settings.lateralExtent = quality.lateralExtentMeters;
	settings.roadHalfWidth = m_settings.roadHalfWidthMeters;
	settings.shoulder      = m_settings.shoulderMeters;
	settings.propDensity   = quality.propDensity;
	settings.flatShading   = m_settings.flatShading;
	return settings;
}

void World::Rebuild()
{
	RouteSettings routeSettings;
	routeSettings.seed                 = Hash64(m_settings.seed);
	routeSettings.minCurveRadiusMeters = m_settings.minCurveRadiusMeters;
	m_route    = std::make_unique<Route>(routeSettings);
	m_sequence = std::make_unique<BiomeSequence>(m_biomes, Hash64(m_settings.seed ^ 0xB10Bu), m_settings.biome);
	m_streamer->Configure(MakeChunkSettings(), m_biomes, m_propTypes);

	ElevationLimits limits;
	limits.maxGradePercent        = m_settings.maxGradePercent;
	limits.maxGradeChangePerMeter = m_settings.maxGradeChangePerMeter;
	const double grade            = m_profile.GradeAt(m_distance);
	m_profile.SetLimits(limits);
	m_profile.Reset(m_distance, grade);
	// Route generation always starts at 0; catch up to the rider (cheap, and pruned as it goes).
	for (double distance = 0.0; distance < m_distance; distance += 5000.0)
	{
		m_route->EnsureGenerated(distance + 5000.0);
		m_route->PruneBefore(std::min(distance + 5000.0, m_distance) - HISTORY_METERS);
	}
	++m_roadRevision;
}

void World::SetSettings(const WorldSettings& settings)
{
	WorldSettings validated = settings;
	validated.Validate();
	const bool routeChanged = validated.seed != m_settings.seed ||
	                          validated.minCurveRadiusMeters != m_settings.minCurveRadiusMeters ||
	                          validated.biome != m_settings.biome;
	if (!IsLoaded())
	{
		m_settings = validated;
		return;
	}
	const WorldSettings previous = m_settings;
	m_settings                   = validated;
	if (routeChanged)
	{
		Rebuild();
		return;
	}
	const ChunkSettings before = m_streamer->GetSettings();
	const ChunkSettings after  = MakeChunkSettings();
	if (before.lateralExtent != after.lateralExtent || before.roadHalfWidth != after.roadHalfWidth ||
	    before.shoulder != after.shoulder || before.propDensity != after.propDensity ||
	    before.flatShading != after.flatShading)
	{
		m_streamer->Configure(after, m_biomes, m_propTypes);
	}
	if (previous.maxGradePercent != m_settings.maxGradePercent ||
	    previous.maxGradeChangePerMeter != m_settings.maxGradeChangePerMeter)
	{
		ElevationLimits limits    = m_profile.GetLimits();
		limits.maxGradePercent        = m_settings.maxGradePercent;
		limits.maxGradeChangePerMeter = m_settings.maxGradeChangePerMeter;
		m_profile.SetLimits(limits);
	}
}

const WorldSettings& World::GetSettings() const
{
	return m_settings;
}

double World::RouteGradeAt(double distance)
{
	const BiomeBlend blend = m_sequence->BlendAt(distance);
	const BiomeList& biomes = m_biomes;
	double amplitude  = 3.0;
	double wavelength = 900.0;
	if (!biomes.empty())
	{
		const BiomeDefinition& from = biomes[static_cast<size_t>(blend.from)];
		const BiomeDefinition& to   = biomes[static_cast<size_t>(blend.to)];
		amplitude  = from.gradeAmplitudePercent + (to.gradeAmplitudePercent - from.gradeAmplitudePercent) * blend.weight;
		wavelength = from.gradeWavelengthMeters + (to.gradeWavelengthMeters - from.gradeWavelengthMeters) * blend.weight;
	}
	return amplitude * 1.6 * Fbm1D(Hash64(m_settings.seed ^ 0x6EADu), distance / wavelength, 2);
}

void World::FillTargetGrades(const Input::TrainerInput& trainer)
{
	const double spacing = m_profile.GetLimits().spacingMeters;
	const double ahead   = m_settings.Resolved().drawDistanceMeters + PROFILE_MARGIN_METERS;
	const size_t count   = static_cast<size_t>(ahead / spacing) + 1;
	m_targetGrades.resize(count);
	const bool workout = !trainer.intensityAhead.empty();
	const double speed = std::max(MIN_LOOKAHEAD_SPEED, trainer.speedMetersPerSecond);
	for (size_t index = 0; index < count; ++index)
	{
		const double distance = m_distance + static_cast<double>(index) * spacing;
		double grade          = 0.0;
		if (m_settings.gradeSource == GradeSource::ROUTE)
		{
			grade = RouteGradeAt(distance);
		}
		else if (!trainer.ergMode)
		{
			grade = trainer.chosenGradePercent;
		}
		else if (workout)
		{
			const size_t second = std::min(static_cast<size_t>((distance - m_distance) / speed), trainer.intensityAhead.size() - 1);
			const double intensity = trainer.intensityAhead[second];
			grade = std::isnan(intensity) ? std::clamp(RouteGradeAt(distance) * 0.4, -2.5, 2.5) : GradeForIntensity(intensity);
		}
		else
		{
			grade = std::clamp(RouteGradeAt(distance) * 0.5, -3.0, 3.0);
		}
		m_targetGrades[index] = grade;
	}
}

void World::Update(double deltaSeconds, const Input::TrainerInput& trainer)
{
	const double delta = std::clamp(deltaSeconds, 0.0, 0.25);
	m_time += delta;
	m_distance += std::max(0.0, trainer.speedMetersPerSecond) * delta;

	const QualityParameters quality = m_settings.Resolved();
	m_route->EnsureGenerated(m_distance + quality.drawDistanceMeters + PROFILE_MARGIN_METERS);
	m_route->PruneBefore(m_distance - HISTORY_METERS);

	FillTargetGrades(trainer);
	m_profile.Update(m_distance, m_targetGrades, HISTORY_METERS);

	m_streamer->Update(*m_route, *m_sequence, m_distance, CHUNKS_BEHIND_METERS, quality.drawDistanceMeters);

	m_blend = m_sequence->BlendAt(m_distance);
	m_look  = LookFor(m_biomes, m_blend);

}

double World::GetDistance() const
{
	return m_distance;
}

double World::GetRoadGradePercent() const
{
	return m_profile.GradeAt(m_distance);
}

double World::GetHeightOrigin() const
{
	return m_profile.HeightAt(m_distance);
}

const Route& World::GetRoute() const
{
	return *m_route;
}

const ElevationProfile& World::GetProfile() const
{
	return m_profile;
}

ChunkStreamer& World::GetStreamer()
{
	return *m_streamer;
}

const std::vector<ChunkMesh*>& World::GetChunks() const
{
	return m_streamer->GetReady();
}

const PropTypeTable& World::GetPropTypes() const
{
	return m_propTypes;
}

BiomeSequence& World::GetBiomeSequence()
{
	return *m_sequence;
}

const std::vector<BiomeDefinition>& World::GetBiomes() const
{
	return m_biomes;
}

const BiomeLook& World::GetLook() const
{
	return m_look;
}

BiomeBlend World::GetBlend() const
{
	return m_blend;
}

uint32_t World::GetRoadRevision() const
{
	return m_roadRevision;
}

double World::GetTimeSeconds() const
{
	return m_time;
}

void World::JumpTo(double distance)
{
	m_distance = std::max(0.0, distance);
	Rebuild();
}

void World::WaitForChunks()
{
	const QualityParameters quality = m_settings.Resolved();
	for (int attempt = 0; attempt < 200; ++attempt)
	{
		m_streamer->WaitIdle();
		m_streamer->Update(*m_route, *m_sequence, m_distance, CHUNKS_BEHIND_METERS, quality.drawDistanceMeters);
		const StreamerStats stats = m_streamer->GetStats();
		const int wanted = static_cast<int>(std::floor((m_distance + quality.drawDistanceMeters) / 64.0)) -
		                   static_cast<int>(std::floor(std::max(0.0, m_distance - CHUNKS_BEHIND_METERS) / 64.0)) + 1;
		if (stats.pendingJobs == 0 && stats.readyChunks >= wanted)
		{
			return;
		}
	}
}
}  // namespace Struktur::World
