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

namespace Struktur::RideIntegration
{
static constexpr const char* SETTINGS_FILE    = "ride_settings.json";
static constexpr const char* WORKOUTS_FOLDER  = "Workouts";
static constexpr const char* RIDES_FOLDER     = "Rides";
static constexpr double TICK_INTERVAL_SECONDS = 0.25;
// Sensor values older than this count as missing (a strap that stopped notifying, a sleeping power meter).
static constexpr double POWER_STALE_SECONDS      = 3.0;
static constexpr double HEART_RATE_STALE_SECONDS = 5.0;

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
	m_lastTickNow = now;
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
	m_lastTickNow = now;
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

	Event::EventManager& events = context.GetEventManager();
	for (const Ride::SessionEvent& event : m_session->TakeEvents())
	{
		Callback::VariantMap data;
		data.items["message"]   = event.message;
		data.items["stepIndex"] = (int)event.stepIndex;
		data.items["duration"]  = event.durationSeconds;
		events.AddEvent(EventName(event.type), data);
	}
	if (now - m_lastTickNow >= TICK_INTERVAL_SECONDS)
	{
		m_lastTickNow = now;
		events.AddEvent("rideTick", nullptr);
	}
}
}  // namespace Struktur::RideIntegration
