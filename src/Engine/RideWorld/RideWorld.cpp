#include "RideWorld.h"

#include <algorithm>
#include <cmath>

#include "Engine/RideWorld/Noise.h"

namespace Struktur::RideWorld
{
static constexpr double PI                     = 3.14159265358979323846;
static constexpr double CHUNKS_BEHIND_METERS   = 64.0;
static constexpr double HISTORY_METERS         = 200.0;
static constexpr double CAMERA_BEHIND_METERS   = 4.0;
static constexpr double LOOK_SMOOTHING_SECONDS = 0.35;
static constexpr double FOV_SMOOTHING_SECONDS  = 1.2;
static constexpr double BOB_AMPLITUDE_METERS   = 0.025;
static constexpr double MAX_SPEED_FOV_DEGREES  = 10.0;
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

DemoPacer::DemoPacer(double startAhead)
    : m_distance(0.0),
      m_phase(0.0),
      m_time(0.0),
      m_started(false),
      m_startAhead(startAhead)
{
}

void DemoPacer::Collect(double riderDistance, double deltaSeconds, std::vector<WorldActor>& out)
{
	if (!m_started)
	{
		m_distance = riderDistance + m_startAhead;
		m_started  = true;
	}
	m_time += deltaSeconds;
	// A steady 30 km/h with a slow surge, so the rider can catch it and drop it.
	const double speed = 8.3 + 1.2 * std::sin(m_time * 0.05);
	m_distance += speed * deltaSeconds;
	m_phase += 88.0 / 60.0 * deltaSeconds;
	m_phase -= std::floor(m_phase);
	// Far behind or far ahead: rejoin a little ahead so the demo always has someone to look at.
	if (m_distance < riderDistance - 80.0 || m_distance > riderDistance + 600.0)
	{
		m_distance = riderDistance + m_startAhead;
	}
	WorldActor actor;
	actor.distance   = m_distance;
	actor.lateral    = 1.4;
	actor.crankPhase = m_phase;
	actor.tint       = 0xFF60A0FFu;  // ABGR: orange jersey tint
	out.push_back(actor);
}

RideWorld::RideWorld(const WorldSettings& settings, BiomeList biomes, bool threaded)
    : m_settings(settings),
      m_biomes(std::move(biomes)),
      m_threaded(threaded),
      m_actorSource(nullptr),
      m_distance(0.0),
      m_time(0.0),
      m_powerZone(0),
      m_smoothedLookX(1.0),
      m_smoothedLookY(0.0),
      m_smoothedLookZ(0.0),
      m_smoothedFov(settings.fieldOfViewDegrees),
      m_cameraInitialised(false)
{
	if (!m_biomes)
	{
		m_biomes = std::make_shared<const std::vector<BiomeDefinition>>();
	}
	m_settings.Validate();
	Rebuild();
}

ChunkSettings RideWorld::MakeChunkSettings() const
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

void RideWorld::Rebuild()
{
	RouteSettings routeSettings;
	routeSettings.seed                 = Hash64(m_settings.seed);
	routeSettings.minCurveRadiusMeters = m_settings.minCurveRadiusMeters;
	m_route    = std::make_unique<Route>(routeSettings);
	m_sequence = std::make_unique<BiomeSequence>(m_biomes, Hash64(m_settings.seed ^ 0xB10Bu), m_settings.biome);
	m_propTypes = std::make_shared<const PropTypeTable>(PropTypeTable::FromBiomes(*m_biomes));
	if (!m_streamer)
	{
		m_streamer = std::make_unique<ChunkStreamer>(m_threaded);
	}
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
	m_cameraInitialised = false;
}

void RideWorld::SetSettings(const WorldSettings& settings)
{
	WorldSettings validated = settings;
	validated.Validate();
	const bool routeChanged = validated.seed != m_settings.seed ||
	                          validated.minCurveRadiusMeters != m_settings.minCurveRadiusMeters ||
	                          validated.biome != m_settings.biome;
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

const WorldSettings& RideWorld::GetSettings() const
{
	return m_settings;
}

void RideWorld::SetActorSource(IWorldActorSource* source)
{
	m_actorSource = source;
}

double RideWorld::RouteGradeAt(double distance)
{
	const BiomeBlend blend = m_sequence->BlendAt(distance);
	const std::vector<BiomeDefinition>& biomes = *m_biomes;
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

void RideWorld::FillTargetGrades(const RideWorldInputs& inputs)
{
	const double spacing = m_profile.GetLimits().spacingMeters;
	const double ahead   = m_settings.Resolved().drawDistanceMeters + PROFILE_MARGIN_METERS;
	const size_t count   = static_cast<size_t>(ahead / spacing) + 1;
	m_targetGrades.resize(count);
	const bool workout = !inputs.intensityAhead.empty();
	const double speed = std::max(MIN_LOOKAHEAD_SPEED, inputs.speedMetersPerSecond);
	for (size_t index = 0; index < count; ++index)
	{
		const double distance = m_distance + static_cast<double>(index) * spacing;
		double grade          = 0.0;
		if (m_settings.gradeSource == GradeSource::ROUTE)
		{
			grade = RouteGradeAt(distance);
		}
		else if (!inputs.ergMode)
		{
			grade = inputs.manualGradePercent;
		}
		else if (workout)
		{
			const size_t second = std::min(static_cast<size_t>((distance - m_distance) / speed), inputs.intensityAhead.size() - 1);
			const double intensity = inputs.intensityAhead[second];
			grade = std::isnan(intensity) ? std::clamp(RouteGradeAt(distance) * 0.4, -2.5, 2.5) : GradeForIntensity(intensity);
		}
		else
		{
			grade = std::clamp(RouteGradeAt(distance) * 0.5, -3.0, 3.0);
		}
		m_targetGrades[index] = grade;
	}
}

void RideWorld::Update(const RideWorldInputs& inputs)
{
	const double delta = std::clamp(inputs.deltaSeconds, 0.0, 0.25);
	m_time += delta;
	m_powerZone = inputs.powerZone;
	m_distance += std::max(0.0, inputs.speedMetersPerSecond) * delta;

	const QualityParameters quality = m_settings.Resolved();
	m_route->EnsureGenerated(m_distance + quality.drawDistanceMeters + PROFILE_MARGIN_METERS);
	m_route->PruneBefore(m_distance - HISTORY_METERS);

	FillTargetGrades(inputs);
	m_profile.Update(m_distance, m_targetGrades, HISTORY_METERS);

	m_streamer->Update(*m_route, *m_sequence, m_distance, CHUNKS_BEHIND_METERS, quality.drawDistanceMeters);

	m_blend = m_sequence->BlendAt(m_distance);
	m_look  = LookFor(*m_biomes, m_blend);

	const double curvature = m_route->SampleAt(m_distance).curvature;
	m_rider.Update(delta, inputs.cadenceRpm, inputs.speedMetersPerSecond, inputs.powerFraction, curvature);

	m_actors.clear();
	if (m_actorSource != nullptr)
	{
		m_actorSource->Collect(m_distance, delta, m_actors);
	}
	for (size_t index = 0; index < m_banners.size();)
	{
		if (m_banners[index] < m_distance - 20.0)
		{
			m_banners.erase(m_banners.begin() + static_cast<ptrdiff_t>(index));
		}
		else
		{
			++index;
		}
	}
	UpdateCamera(inputs);
}

void RideWorld::UpdateCamera(const RideWorldInputs& inputs)
{
	const double delta      = std::clamp(inputs.deltaSeconds, 0.0, 0.25);
	const double origin     = GetHeightOrigin();
	const double cameraAt   = std::max(m_route->GetStart(), m_distance - CAMERA_BEHIND_METERS);
	const RouteSample eye   = m_route->SampleAt(cameraAt);
	const double lookAt     = m_distance + m_settings.cameraLookAheadMeters;
	const RouteSample focus = m_route->SampleAt(lookAt);
	const bool motion       = !m_settings.comfortMode;

	double eyeY = m_profile.HeightAt(cameraAt) - origin + m_settings.cameraHeightMeters;
	if (motion && m_settings.cadenceBob && m_rider.GetPose() != RiderPose::IDLE)
	{
		eyeY += BOB_AMPLITUDE_METERS * std::sin(m_rider.GetCrankPhase() * 4.0 * PI);
	}
	const double focusY = m_profile.HeightAt(lookAt) - origin + m_settings.cameraHeightMeters * 0.55;

	double directionX   = focus.x - eye.x;
	double directionY   = focusY - eyeY;
	double directionZ   = focus.z - eye.z;
	const double length = std::sqrt(directionX * directionX + directionY * directionY + directionZ * directionZ);
	if (length > 1e-6)
	{
		directionX /= length;
		directionY /= length;
		directionZ /= length;
	}
	if (!m_cameraInitialised)
	{
		m_smoothedLookX     = directionX;
		m_smoothedLookY     = directionY;
		m_smoothedLookZ     = directionZ;
		m_smoothedFov       = m_settings.fieldOfViewDegrees;
		m_cameraInitialised = true;
	}
	const double blend = 1.0 - std::exp(-delta / LOOK_SMOOTHING_SECONDS);
	m_smoothedLookX += (directionX - m_smoothedLookX) * blend;
	m_smoothedLookY += (directionY - m_smoothedLookY) * blend;
	m_smoothedLookZ += (directionZ - m_smoothedLookZ) * blend;

	double wantedFov = m_settings.fieldOfViewDegrees;
	if (motion && m_settings.speedFov)
	{
		wantedFov += std::min(MAX_SPEED_FOV_DEGREES, std::max(0.0, inputs.speedMetersPerSecond - 5.0) * 0.8);
	}
	m_smoothedFov += (wantedFov - m_smoothedFov) * (1.0 - std::exp(-delta / FOV_SMOOTHING_SECONDS));

	m_camera.x                  = eye.x;
	m_camera.y                  = eyeY;
	m_camera.z                  = eye.z;
	m_camera.targetX            = eye.x + m_smoothedLookX * 10.0;
	m_camera.targetY            = eyeY + m_smoothedLookY * 10.0;
	m_camera.targetZ            = eye.z + m_smoothedLookZ * 10.0;
	m_camera.fieldOfViewDegrees = motion ? m_smoothedFov : m_settings.fieldOfViewDegrees;
}

double RideWorld::GetDistance() const
{
	return m_distance;
}

double RideWorld::GetRoadGradePercent() const
{
	return m_profile.GradeAt(m_distance);
}

double RideWorld::GetHeightOrigin() const
{
	return m_profile.HeightAt(m_distance);
}

const Route& RideWorld::GetRoute() const
{
	return *m_route;
}

const ElevationProfile& RideWorld::GetProfile() const
{
	return m_profile;
}

ChunkStreamer& RideWorld::GetStreamer()
{
	return *m_streamer;
}

const std::vector<ChunkMesh*>& RideWorld::GetChunks() const
{
	return m_streamer->GetReady();
}

std::shared_ptr<const PropTypeTable> RideWorld::GetPropTypes() const
{
	return m_propTypes;
}

BiomeSequence& RideWorld::GetBiomeSequence()
{
	return *m_sequence;
}

const BiomeLook& RideWorld::GetLook() const
{
	return m_look;
}

BiomeBlend RideWorld::GetBlend() const
{
	return m_blend;
}

const WorldCamera& RideWorld::GetCamera() const
{
	return m_camera;
}

const RiderAnimation& RideWorld::GetRiderAnimation() const
{
	return m_rider;
}

const std::vector<WorldActor>& RideWorld::GetActors() const
{
	return m_actors;
}

const std::vector<double>& RideWorld::GetBanners() const
{
	return m_banners;
}

int RideWorld::GetPowerZone() const
{
	return m_powerZone;
}

double RideWorld::GetTimeSeconds() const
{
	return m_time;
}

void RideWorld::PlaceBanner(double aheadMeters)
{
	m_banners.push_back(m_distance + aheadMeters);
}

void RideWorld::WaitForChunks()
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
}  // namespace Struktur::RideWorld
