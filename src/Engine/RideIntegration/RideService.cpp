#include "RideService.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Debug/Assertions.h"
#include "Engine/Callback/Variant.h"
#include "Engine/Core/CommandLine.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/Fitness/FitnessManager.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/RideWorldRenderer.h"

namespace Struktur::RideIntegration
{
static constexpr const char* SETTINGS_FILE    = "ride_settings.json";
static constexpr const char* WORKOUTS_FOLDER  = "Workouts";
static constexpr const char* RIDES_FOLDER     = "Rides";
static constexpr double TICK_INTERVAL_SECONDS = 0.25;
// Sensor values older than this count as missing (a strap that stopped notifying, a sleeping power meter).
static constexpr double POWER_STALE_SECONDS      = 3.0;
static constexpr double HEART_RATE_STALE_SECONDS = 5.0;

static constexpr const char* WORLD_MANIFEST_FILE = "World/manifest.json";
static constexpr const char* WORLD_BIOMES_FOLDER = "World/Biomes";
static constexpr const char* WORLD_RIDER_FILE    = "World/Rider/rider.json";
static constexpr double WORKOUT_LOOKAHEAD_SECONDS = 300.0;  // how far ahead the world reads the workout plan
static constexpr double FINISH_BANNER_AHEAD_METERS = 90.0;
static constexpr size_t MAX_BIOME_FILE_BYTES      = 256 * 1024;

static std::filesystem::path PathFromUtf8(const std::string& text)
{
	return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

static std::string Utf8FromPath(const std::filesystem::path& path)
{
	const std::u8string text = path.u8string();
	return std::string(text.begin(), text.end());
}

static std::string ToLower(std::string text)
{
	for (char& character : text)
	{
		character = (char)std::tolower((unsigned char)character);
	}
	return text;
}

static bool IsWorkoutFile(const std::string& name)
{
	const std::string lower = ToLower(name);
	return lower.size() > 4 && (lower.ends_with(".zwo") || lower.ends_with(".mrc"));
}

RideService::RideService() = default;

// Out of line: RideWorldRenderer is only forward-declared in the header. Destroying it releases its GPU resources,
// which GameContext::Shutdown guarantees happens before bgfx shuts down.
RideService::~RideService() = default;

void RideService::Initialise(const std::string& saveDirectory)
{
	m_saveDirectory = saveDirectory;
	std::error_code error;
	std::filesystem::create_directories(PathFromUtf8(GetUserWorkoutsFolder()), error);
	std::filesystem::create_directories(PathFromUtf8(GetRidesFolder()), error);

	if (FileSystem::Exists(SETTINGS_FILE))
	{
		const auto text = FileSystem::ReadString(SETTINGS_FILE);
		if (text)
		{
			std::string warning;
			m_settings = Ride::RideSettings::FromJson(text.value, warning);
			if (!warning.empty())
			{
				DEBUG_WARNING("%s", warning.c_str());
			}
		}
	}
	LoadWorldData();
}

static bool EngineFileExists(const std::string& path)
{
	return FileSystem::Exists(path);
}

void RideService::LoadWorldData()
{
	// Biomes: every .json in World/Biomes, sorted by file name (adding a biome is a data-only change).
	auto biomes = std::make_shared<std::vector<RideWorld::BiomeDefinition>>();
	std::vector<std::string> files = FileSystem::ListDirectory(WORLD_BIOMES_FOLDER);
	std::sort(files.begin(), files.end());
	for (const std::string& file : files)
	{
		if (!ToLower(file).ends_with(".json"))
		{
			continue;
		}
		const std::string path = std::string(WORLD_BIOMES_FOLDER) + "/" + file;
		const FileResult<uint64_t> size = FileSystem::GetFileSize(path);
		if (!size || size.value > MAX_BIOME_FILE_BYTES)
		{
			DEBUG_WARNING("Ride world: skipping biome %s (unreadable or too large)", path.c_str());
			continue;
		}
		const auto text = FileSystem::ReadString(path);
		RideWorld::BiomeDefinition biome;
		std::string error;
		if (text && RideWorld::ParseBiome(text.value, biome, error))
		{
			m_biomeIds.push_back(biome.id);
			biomes->push_back(std::move(biome));
		}
		else
		{
			DEBUG_WARNING("Ride world: biome %s ignored: %s", path.c_str(), error.c_str());
		}
	}
	m_biomes = biomes;
	if (biomes->empty())
	{
		DEBUG_WARNING("Ride world: no biomes in %s - the world is disabled", WORLD_BIOMES_FOLDER);
	}

	m_spriteDescriptor = RideWorld::DefaultSpriteDescriptor();
	if (FileSystem::Exists(WORLD_RIDER_FILE))
	{
		const auto text = FileSystem::ReadString(WORLD_RIDER_FILE);
		std::string error;
		RideWorld::SpriteDescriptor descriptor;
		if (text && RideWorld::ParseSpriteDescriptor(text.value, descriptor, error))
		{
			m_spriteDescriptor = descriptor;
		}
		else
		{
			DEBUG_WARNING("Ride world: %s ignored (%s) - using the built-in rider", WORLD_RIDER_FILE, error.c_str());
		}
	}

	if (FileSystem::Exists(WORLD_MANIFEST_FILE))
	{
		const auto text = FileSystem::ReadString(WORLD_MANIFEST_FILE);
		std::string error;
		std::vector<std::string> warnings;
		if (!text || !RideWorld::ParseManifest(text.value, m_manifest, error, warnings))
		{
			DEBUG_WARNING("Ride world: %s unreadable (%s) - every role is a placeholder", WORLD_MANIFEST_FILE, error.c_str());
		}
		for (const std::string& warning : warnings)
		{
			DEBUG_WARNING("Ride world manifest: %s", warning.c_str());
		}
	}
	// Logged once at startup so missing art is obvious (also: --list-missing-assets).
	const std::vector<std::string> missing = GetMissingAssetLines();
	if (!missing.empty())
	{
		DEBUG_INFO("Ride world: %d of %d art roles use placeholders:", (int)missing.size(), (int)m_manifest.roles.size());
		for (const std::string& line : missing)
		{
			DEBUG_INFO("  missing %s", line.c_str());
		}
	}
}

std::vector<std::string> RideService::GetMissingAssetLines() const
{
	return RideWorld::FormatMissingAssetsLines(RideWorld::ResolveAssets(m_manifest, EngineFileExists));
}

std::string RideService::GetMissingAssetsMarkdown() const
{
	return RideWorld::FormatMissingAssetsMarkdown(RideWorld::ResolveAssets(m_manifest, EngineFileExists));
}

const std::vector<std::string>& RideService::GetBiomeIds() const
{
	return m_biomeIds;
}

RideWorld::RideWorld* RideService::GetWorld()
{
	return m_world.get();
}

void RideService::CreateWorld()
{
	m_world.reset();
	m_pacer.reset();
	if (m_worldRenderer)
	{
		m_worldRenderer->ResetChunks();
	}
	if (!m_settings.world.enabled || !m_biomes || m_biomes->empty())
	{
		return;
	}
	m_world = std::make_unique<RideWorld::RideWorld>(m_settings.world, m_biomes, true);
	if (IsDemo() && m_settings.world.demoPacer)
	{
		m_pacer = std::make_unique<RideWorld::DemoPacer>();
		m_world->SetActorSource(m_pacer.get());
	}
}

void RideService::ApplyWorldSettings()
{
	if (!m_session)
	{
		return;
	}
	if (m_settings.world.enabled != (m_world != nullptr))
	{
		CreateWorld();
		if (!m_world)
		{
			m_session->SetRoadGrade(std::nullopt);
		}
		return;
	}
	if (m_world)
	{
		const uint32_t seedBefore = m_world->GetSettings().seed;
		const std::string biomeBefore = m_world->GetSettings().biome;
		m_world->SetSettings(m_settings.world);
		if (m_worldRenderer && (seedBefore != m_settings.world.seed || biomeBefore != m_settings.world.biome))
		{
			m_worldRenderer->ResetChunks();
		}
	}
}

// Target intensity (fraction of FTP) for each of the next `seconds` seconds of the workout; NaN = no target.
static void FillIntensityAhead(const Ride::WorkoutRunner& runner, double seconds, std::vector<double>& out)
{
	out.clear();
	const Ride::Workout& workout = runner.GetWorkout();
	if (runner.IsFinished() || workout.steps.empty())
	{
		return;
	}
	size_t step          = runner.GetStepIndex();
	double intoStep      = runner.GetStepElapsed();
	double stepDuration  = runner.GetStepDuration();
	for (double second = 0.0; second < seconds; second += 1.0)
	{
		while (intoStep >= stepDuration && step + 1 < workout.steps.size())
		{
			intoStep -= stepDuration;
			++step;
			stepDuration = workout.steps[step].durationSeconds;
		}
		const std::optional<double> target = workout.steps[step].TargetFractionAt(intoStep);
		out.push_back(target ? *target : std::nan(""));
		intoStep += 1.0;
	}
}

void RideService::UpdateWorld(double now)
{
	if (!m_world || !m_session)
	{
		return;
	}
	const double delta = std::clamp(now - m_lastWorldNow, 0.0, 0.25);
	m_lastWorldNow     = now;
	RideWorld::RideWorldInputs inputs;
	inputs.deltaSeconds         = delta;
	inputs.speedMetersPerSecond = m_session->GetSpeedMetersPerSecond();
	inputs.manualGradePercent   = m_session->GetChosenGradePercent();
	inputs.ergMode              = m_session->GetController().GetMode() == Ride::TrainerController::Mode::ERG;
	inputs.powerZone            = m_session->GetPowerZone();
	inputs.cadenceRpm           = m_session->GetCadence();
	inputs.heartRateBpm         = m_session->GetHeartRate();
	const std::optional<double> power = m_session->GetPower();
	if (power && m_session->GetRider().ftpWatts > 0.0)
	{
		inputs.powerFraction = *power / m_session->GetRider().ftpWatts;
	}
	const std::optional<double> target = m_session->GetTargetWatts();
	if (power && target)
	{
		inputs.targetDeltaWatts = *power - *target;
	}
	if (const Ride::WorkoutRunner* runner = m_session->GetRunner())
	{
		const double total     = runner->GetWorkout().TotalDurationSeconds();
		inputs.workoutProgress = total > 0.0 ? std::clamp(runner->GetPlanPosition() / total, 0.0, 1.0) : 0.0;
		FillIntensityAhead(*runner, WORKOUT_LOOKAHEAD_SECONDS, inputs.intensityAhead);
	}
	m_world->Update(inputs);
	// SIM simulates the road under the rider (in ERG the controller holds watts and ignores it).
	m_session->SetRoadGrade(m_world->GetRoadGradePercent());
}

void RideService::RenderWorld(GameContext& context)
{
	if (!m_world)
	{
		return;
	}
	if (!m_worldRenderer)
	{
		m_worldRenderer = std::make_unique<Renderer::RideWorldRenderer>();
	}
	if (!m_worldRenderer->IsInitialised())
	{
		m_worldRenderer->Initialise(m_manifest, m_spriteDescriptor);
	}
	m_worldRenderer->Render(context, *m_world, context.GetTimeSystem().unscaledDelta);
}

Ride::RideSettings& RideService::GetSettings()
{
	return m_settings;
}

bool RideService::SaveSettings()
{
	m_settings.Validate();
	const auto result = FileSystem::WriteString(SETTINGS_FILE, m_settings.ToJson());
	if (!result)
	{
		DEBUG_ERROR("Could not save %s", SETTINGS_FILE);
		return false;
	}
	FileSystem::SyncSaves();
	return true;
}

std::string RideService::GetUserWorkoutsFolder() const
{
	return Utf8FromPath(PathFromUtf8(m_saveDirectory) / WORKOUTS_FOLDER);
}

std::string RideService::GetRidesFolder() const
{
	return Utf8FromPath(PathFromUtf8(m_saveDirectory) / RIDES_FOLDER);
}

bool RideService::IsDemo() const
{
	return Core::CommandLine::HasFlag("--demo");
}

static bool ByName(const WorkoutListing& left, const WorkoutListing& right)
{
	return ToLower(left.name) < ToLower(right.name);
}

std::vector<WorkoutListing> RideService::ListWorkouts() const
{
	std::vector<WorkoutListing> listings;
	// The save folder is mounted over the assets, so "Workouts" merges the built-in and the player's own files.
	for (const std::string& name : FileSystem::ListDirectory(WORKOUTS_FOLDER))
	{
		if (!IsWorkoutFile(name))
		{
			continue;
		}
		WorkoutListing listing;
		listing.id          = std::string(WORKOUTS_FOLDER) + "/" + name;
		const auto realPath = FileSystem::GetRealPath(listing.id);
		listing.source = (realPath && realPath.value.find(m_saveDirectory) != std::string::npos) ? "user" : "builtin";
		const Ride::WorkoutLoadResult loaded = LoadWorkout(listing.id);
		if (loaded.workout)
		{
			listing.name            = loaded.workout->name;
			listing.durationSeconds = loaded.workout->TotalDurationSeconds();
		}
		else
		{
			listing.name  = name;
			listing.error = loaded.error;
		}
		listings.push_back(std::move(listing));
	}
	std::sort(listings.begin(), listings.end(), ByName);
	return listings;
}

Ride::WorkoutLoadResult RideService::LoadWorkout(const std::string& id) const
{
	const Ride::WorkoutLimits limits;
	const std::filesystem::path path = PathFromUtf8(id);
	if (path.is_absolute())
	{
		return Ride::LoadWorkoutFile(id, limits);
	}
	// Engine file system (built-in or user workouts): check the size before reading the whole file.
	const auto size = FileSystem::GetFileSize(id);
	if (!size)
	{
		Ride::WorkoutLoadResult result;
		result.error = "can't read " + id;
		return result;
	}
	if (size.value > limits.maxFileBytes)
	{
		Ride::WorkoutLoadResult result;
		result.error = "the file is larger than " + std::to_string(limits.maxFileBytes / 1024) + " KB";
		return result;
	}
	const auto text = FileSystem::ReadString(id);
	if (!text)
	{
		Ride::WorkoutLoadResult result;
		result.error = "can't read " + id;
		return result;
	}
	return Ride::ParseWorkout(id, text.value, limits);
}

std::optional<std::string> RideService::TakeCommandLineWorkout()
{
	if (m_commandLineWorkoutTaken)
	{
		return std::nullopt;
	}
	m_commandLineWorkoutTaken       = true;
	std::optional<std::string> path = Core::CommandLine::FindFileArgument({".zwo", ".mrc"});
	if (path)
	{
		// Make relative paths absolute against the process's working directory.
		std::error_code error;
		const std::filesystem::path absolute = std::filesystem::absolute(PathFromUtf8(*path), error);
		if (!error)
		{
			path = Utf8FromPath(absolute);
		}
	}
	return path;
}

std::string RideService::MakeOutputBasePath(const std::string& title) const
{
	const std::time_t now = std::time(nullptr);
	std::tm local{};
#if defined(_WIN32)
	localtime_s(&local, &now);
#else
	localtime_r(&now, &local);
#endif
	char stamp[32];
	std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &local);
	std::string safeTitle;
	for (char character : title)
	{
		safeTitle += (std::isalnum((unsigned char)character) || character == '-') ? character : '_';
	}
	if (safeTitle.size() > 40)
	{
		safeTitle.resize(40);
	}
	return Utf8FromPath(PathFromUtf8(GetRidesFolder()) / (std::string(stamp) + "_" + safeTitle));
}

static int32_t LocalUtcOffsetSeconds()
{
	const std::time_t now = std::time(nullptr);
	std::tm local{};
	std::tm utc{};
#if defined(_WIN32)
	localtime_s(&local, &now);
	gmtime_s(&utc, &now);
#else
	localtime_r(&now, &local);
	gmtime_r(&now, &utc);
#endif
	utc.tm_isdst = local.tm_isdst;
	return (int32_t)std::difftime(std::mktime(&local), std::mktime(&utc));
}

bool RideService::StartFreeRide(const Ride::RiderInfo& rider, double now)
{
	m_session = std::make_unique<Ride::RideSession>(Ride::RideMode::FREE_RIDE, rider, m_settings, std::nullopt,
	                                                MakeOutputBasePath("Free_Ride"), (int64_t)std::time(nullptr),
	                                                LocalUtcOffsetSeconds());
	m_session->Start(now);
	m_lastTickNow  = now;
	m_lastWorldNow = now;
	CreateWorld();
	return true;
}

bool RideService::StartWorkout(const std::string& workoutId, const Ride::RiderInfo& rider, double now,
                               std::string& error)
{
	Ride::WorkoutLoadResult loaded = LoadWorkout(workoutId);
	if (!loaded.workout)
	{
		error = loaded.error;
		return false;
	}
	const std::string title = loaded.workout->name;
	m_session               = std::make_unique<Ride::RideSession>(Ride::RideMode::WORKOUT, rider, m_settings,
	                                                              std::move(loaded.workout), MakeOutputBasePath(title),
	                                                              (int64_t)std::time(nullptr), LocalUtcOffsetSeconds());
	m_session->Start(now);
	m_lastTickNow  = now;
	m_lastWorldNow = now;
	CreateWorld();
	return true;
}

Ride::RideSession* RideService::GetSession()
{
	return m_session.get();
}

bool RideService::IsActive() const
{
	return m_session != nullptr && !m_session->IsFinished();
}

bool RideService::Finish(double now)
{
	return m_session ? m_session->Finish(now) : false;
}

void RideService::Discard()
{
	m_session.reset();
	m_world.reset();
	m_pacer.reset();
	if (m_worldRenderer)
	{
		m_worldRenderer->ResetChunks();
	}
}

static std::string EventName(Ride::SessionEvent::Type type)
{
	std::string name = Ride::ToString(type);
	name[0]          = (char)std::toupper((unsigned char)name[0]);
	return "ride" + name;
}

void RideService::Update(GameContext& context, double now)
{
	if (!m_session || m_session->IsFinished())
	{
		return;
	}
	Fitness::FitnessManager& fitness = context.GetFitnessManager();
	Fitness::Trainer* trainer        = fitness.GetTrainer();
	Fitness::PowerMeter* meter       = fitness.GetPowerMeter();
	Fitness::HeartRateSensor* strap  = fitness.GetHeartRateSensor();

	Ride::SensorReadings readings;
	readings.trainerAssigned     = trainer != nullptr;
	readings.trainerConnected    = trainer && trainer->GetConnectionState() == Fitness::ConnectionState::CONNECTED;
	readings.powerMeterAssigned  = meter != nullptr;
	readings.powerMeterConnected = meter && meter->GetConnectionState() == Fitness::ConnectionState::CONNECTED;
	readings.heartRateAssigned   = strap != nullptr;
	readings.heartRateConnected  = strap && strap->GetConnectionState() == Fitness::ConnectionState::CONNECTED;

	const bool meterFresh = readings.powerMeterConnected && meter->GetLatestSample().powerWatts &&
	                        now - meter->GetLatestSample().timestampSeconds <= POWER_STALE_SECONDS;
	const bool trainerFresh = readings.trainerConnected && trainer->GetLatestSample().powerWatts &&
	                          now - trainer->GetLatestSample().timestampSeconds <= POWER_STALE_SECONDS;
	const bool useMeter = m_settings.powerSource == Ride::PowerSource::POWER_METER ||
	                      (m_settings.powerSource == Ride::PowerSource::AUTO && meterFresh);
	if (useMeter && meterFresh)
	{
		readings.powerWatts = *meter->GetLatestSample().powerWatts;
		readings.cadenceRpm = meter->GetLatestSample().cadenceRpm;
	}
	else if (!useMeter && trainerFresh)
	{
		readings.powerWatts = *trainer->GetLatestSample().powerWatts;
	}
	if (!readings.cadenceRpm && trainerFresh)
	{
		readings.cadenceRpm = trainer->GetLatestSample().cadenceRpm;
	}
	if (readings.heartRateConnected && strap->GetLatestSample().bpm &&
	    now - strap->GetLatestSample().timestampSeconds <= HEART_RATE_STALE_SECONDS)
	{
		readings.heartRateBpm = *strap->GetLatestSample().bpm;
	}
	else if (trainerFresh && trainer->GetLatestSample().heartRateBpm)
	{
		readings.heartRateBpm = *trainer->GetLatestSample().heartRateBpm;
	}

	m_session->Update(now, readings, trainer);
	UpdateWorld(now);

	Event::EventManager& events = context.GetEventManager();
	for (const Ride::SessionEvent& event : m_session->TakeEvents())
	{
		Callback::VariantMap data;
		data.items["message"]   = event.message;
		data.items["stepIndex"] = (int)event.stepIndex;
		data.items["duration"]  = event.durationSeconds;
		events.AddEvent(EventName(event.type), data);
		if (event.type == Ride::SessionEvent::Type::WORKOUT_FINISHED && m_world)
		{
			m_world->PlaceBanner(FINISH_BANNER_AHEAD_METERS);
		}
	}
	if (now - m_lastTickNow >= TICK_INTERVAL_SECONDS)
	{
		m_lastTickNow = now;
		events.AddEvent("rideTick", nullptr);
	}
}
}  // namespace Struktur::RideIntegration
