#include "RideSession.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Ride/FitWriter.h"
#include "Ride/Zones.h"
#include "nlohmann/json.hpp"

namespace Struktur::Ride
{
static constexpr double POWER_SMOOTHING_SECONDS   = 1.5;  // EMA time constant (~3 s average)
static constexpr double MAX_FRAME_SECONDS         = 1.0;  // a hitch longer than this isn't counted as riding
static constexpr double FREE_RIDE_TARGET_FRACTION = 0.65;
static constexpr int FREE_RIDE_NUDGE_WATTS        = 5;

const char* ToString(SessionEvent::Type type)
{
	switch (type)
	{
		case SessionEvent::Type::STARTED:
			return "started";
		case SessionEvent::Type::STEP_STARTED:
			return "stepStarted";
		case SessionEvent::Type::CUE:
			return "cue";
		case SessionEvent::Type::AUTO_PAUSED:
			return "autoPaused";
		case SessionEvent::Type::PAUSED:
			return "paused";
		case SessionEvent::Type::RESUMED:
			return "resumed";
		case SessionEvent::Type::LAP:
			return "lap";
		case SessionEvent::Type::WORKOUT_FINISHED:
			return "workoutFinished";
		case SessionEvent::Type::SAVED:
			return "saved";
		case SessionEvent::Type::SAVE_FAILED:
			return "saveFailed";
	}
	return "unknown";
}

RideSession::RideSession(RideMode mode, RiderInfo rider, RideSettings settings, std::optional<Workout> workout,
                         std::string outputBasePath, int64_t unixStartTime, int32_t utcOffsetSeconds)
    : m_mode(mode),
      m_rider(std::move(rider)),
      m_settings(std::move(settings)),
      m_runner(),
      m_outputBasePath(std::move(outputBasePath)),
      m_unixStart(unixStartTime),
      m_utcOffsetSeconds(utcOffsetSeconds),
      m_speedModel(),
      m_controller(),
      m_log(),
      m_readings(),
      m_started(false),
      m_waiting(true),
      m_paused(false),
      m_autoPaused(false),
      m_finished(false),
      m_startNow(0.0),
      m_lastNow(0.0),
      m_timer(0.0),
      m_nextSampleTimer(1.0),
      m_zeroPowerSince(),
      m_smoothedPower(),
      m_freeRideTarget(0.0),
      m_lapStartTimer(0.0),
      m_lapNumber(1),
      m_lastSaveNow(0.0),
      m_lastSaveError(),
      m_powerSum(0.0),
      m_sampleCount(0),
      m_targetSeconds(0),
      m_inTargetSeconds(0),
      m_events()
{
	m_settings.Validate();
	m_settings.speed.riderMassKg = std::clamp(m_rider.weightKg, 30.0, 200.0);
	m_speedModel.SetParameters(m_settings.speed);
	m_controller.SetSettings(m_settings.control);
	m_controller.SetSpeedParameters(m_settings.speed);
	if (m_mode == RideMode::WORKOUT && workout)
	{
		m_runner = std::make_unique<WorkoutRunner>(std::move(*workout), m_settings.leadInSeconds);
	}
	else
	{
		m_mode = RideMode::FREE_RIDE;
	}
	const double start = m_settings.freeRideStartTargetWatts > 0 ? m_settings.freeRideStartTargetWatts
	                                                             : m_rider.ftpWatts * FREE_RIDE_TARGET_FRACTION;
	m_freeRideTarget   = std::round(start / FREE_RIDE_NUDGE_WATTS) * FREE_RIDE_NUDGE_WATTS;
	// A workout starts in ERG (that's what workouts are for); Free Ride starts in simulation on a flat road.
	m_controller.SetErgEnabled(m_mode == RideMode::WORKOUT);
}

void RideSession::Start(double now)
{
	if (m_started)
	{
		return;
	}
	m_started     = true;
	m_waiting     = true;
	m_startNow    = now;
	m_lastNow     = now;
	m_lastSaveNow = now;
	const std::string firstLap =
	    m_runner && !m_runner->GetWorkout().steps.empty() ? m_runner->GetWorkout().steps[0].label : "Lap 1";
	m_log.Start(m_unixStart, firstLap);
}

int64_t RideSession::WallClock(double now) const
{
	// Rounded, not floored: frame-time drift would otherwise land two consecutive samples on the same second.
	return m_unixStart + (int64_t)std::llround(std::max(0.0, now - m_startNow));
}

std::optional<double> RideSession::CurrentStepTarget() const
{
	if (!m_runner || !m_runner->IsStarted() || m_runner->IsFinished())
	{
		return std::nullopt;
	}
	const std::optional<double> fraction = m_runner->GetTargetFraction();
	if (!fraction)
	{
		return std::nullopt;
	}
	return *fraction * m_rider.ftpWatts;
}

bool RideSession::IsInTarget(double power, double target) const
{
	const double band = std::max(target * m_settings.targetBandPercent / 100.0, m_settings.targetBandMinimumWatts);
	return std::abs(power - target) <= band;
}

void RideSession::Update(double now, const SensorReadings& readings, Fitness::Trainer* trainer)
{
	if (!m_started || m_finished)
	{
		return;
	}
	const double deltaSeconds = std::clamp(now - m_lastNow, 0.0, MAX_FRAME_SECONDS);
	m_lastNow                 = now;
	m_readings                = readings;

	// Power smoothing (exponential, ~3 s window).
	if (readings.powerWatts)
	{
		const double blend = 1.0 - std::exp(-deltaSeconds / POWER_SMOOTHING_SECONDS);
		m_smoothedPower    = m_smoothedPower ? *m_smoothedPower + (*readings.powerWatts - *m_smoothedPower) * blend
		                                     : *readings.powerWatts;
	}
	else
	{
		m_smoothedPower.reset();
	}

	// Start / auto-pause / auto-resume on power.
	const bool pedalling = readings.powerWatts && *readings.powerWatts > 0.5;
	if (pedalling)
	{
		m_zeroPowerSince.reset();
		if (m_waiting)
		{
			m_waiting = false;
			m_log.AddTimerEvent(WallClock(now), true);
			if (m_runner)
			{
				m_runner->Start(now);
			}
			m_events.push_back(SessionEvent{SessionEvent::Type::STARTED, 0, {}, 0.0});
		}
		else if (m_autoPaused)
		{
			Resume(now);
		}
	}
	else if (!m_waiting && !m_paused)
	{
		if (!m_zeroPowerSince)
		{
			m_zeroPowerSince = now;
		}
		if (m_settings.autoPauseSeconds > 0.0 && now - *m_zeroPowerSince >= m_settings.autoPauseSeconds)
		{
			Pause(now, true);
		}
	}

	const bool running = !m_waiting && !m_paused;
	if (running)
	{
		m_timer += deltaSeconds;
		m_speedModel.Step(readings.powerWatts.value_or(0.0), m_controller.GetEffectiveGradePercent(), deltaSeconds);
	}
	else
	{
		// Stopped rider: let the virtual bike coast to a halt rather than freeze mid-air.
		m_speedModel.Step(0.0, m_controller.GetEffectiveGradePercent(), deltaSeconds);
	}

	// Workout progress.
	if (m_runner)
	{
		m_runner->Update(now);
		for (const RunnerEvent& event : m_runner->TakeEvents())
		{
			if (event.type == RunnerEvent::Type::STEP_STARTED)
			{
				if (event.stepIndex > 0)
				{
					m_log.NewLap(m_runner->GetWorkout().steps[event.stepIndex].label, WallClock(now), m_timer);
					m_lapStartTimer = m_timer;
					++m_lapNumber;
				}
				m_events.push_back(SessionEvent{SessionEvent::Type::STEP_STARTED, event.stepIndex,
				                                m_runner->GetWorkout().steps[event.stepIndex].label, 0.0});
			}
			else if (event.type == RunnerEvent::Type::CUE)
			{
				m_events.push_back(
				    SessionEvent{SessionEvent::Type::CUE, event.stepIndex, event.message, event.durationSeconds});
			}
			else
			{
				m_events.push_back(SessionEvent{SessionEvent::Type::WORKOUT_FINISHED, event.stepIndex, {}, 0.0});
			}
		}
	}

	// Trainer control.
	m_controller.SetBaseTarget(m_mode == RideMode::WORKOUT ? CurrentStepTarget()
	                                                       : std::optional<double>(m_freeRideTarget));
	if (trainer != nullptr)
	{
		m_controller.SetCapabilities(trainer->GetCapabilities());
	}
	ControlInputs inputs;
	inputs.now                  = now;
	inputs.powerWatts           = m_smoothedPower;
	inputs.cadenceRpm           = readings.cadenceRpm;
	inputs.speedMetersPerSecond = m_speedModel.GetSpeedMetersPerSecond();
	m_controller.Update(inputs, trainer);

	// 1 Hz samples of moving time.
	// The epsilon absorbs floating-point drift from summing frame times (20 x 0.05 s is a hair under 1 s).
	while (running && m_timer + 1e-6 >= m_nextSampleTimer)
	{
		RecordSample(now);
		m_nextSampleTimer += 1.0;
	}

	CollectBackgroundSave(false);
	if (now - m_lastSaveNow >= m_settings.autosaveSeconds && !m_log.GetSamples().empty() && !IsSaving())
	{
		if (m_jobs != nullptr)
		{
			StartBackgroundSave();
		}
		else
		{
			SaveNow();
		}
		m_lastSaveNow = now;
	}
}

void RideSession::SetBackgroundJobs(Jobs::JobSystem* jobs)
{
	CollectBackgroundSave(true);
	m_jobs = jobs;
}

bool RideSession::IsSaving() const
{
	return m_pendingSave.IsValid() && !m_pendingSave.IsComplete();
}

// Everything a save needs, copied so the files can be written without touching the session. Callable, so a background
// task can own it outright.
struct RideSaveJob
{
	RideLog snapshot;
	FitOptions options;
	std::string fitPath;
	std::string jsonPath;
	std::string jsonText;

	std::string operator()() const;  // writes both files; the error, or empty
};

static std::string WriteRideFiles(const RideSaveJob& job)
{
	std::string error;
	if (!WriteFileAtomically(job.fitPath, EncodeFitActivity(job.snapshot, job.options), error))
	{
		return error.empty() ? "could not write " + job.fitPath : error;
	}
	if (!WriteFileAtomically(job.jsonPath, job.jsonText, error))
	{
		return error.empty() ? "could not write " + job.jsonPath : error;
	}
	return "";
}

std::string RideSaveJob::operator()() const
{
	return WriteRideFiles(*this);
}

void RideSession::StartBackgroundSave()
{
	if (m_outputBasePath.empty())
	{
		return;
	}
	RideSaveJob job;
	job.snapshot = m_log;
	// An autosave is a complete, valid file: close the copy as if the ride ended now.
	job.snapshot.Close(job.snapshot.GetEndUnix(), m_log.GetSamples().empty() ? 0.0 : m_log.GetSamples().back().timerSeconds);
	job.options.utcOffsetSeconds = m_utcOffsetSeconds;
	job.options.serialNumber     = (uint32_t)(m_unixStart & 0x7FFFFFFF);
	job.fitPath                  = GetFitPath();
	job.jsonPath                 = GetJsonPath();
	job.jsonText                 = EncodeJson();
	m_pendingSave = m_jobs->Run(std::move(job), Jobs::Lane::BACKGROUND);
}

void RideSession::CollectBackgroundSave(bool wait)
{
	if (!m_pendingSave.IsValid())
	{
		return;
	}
	if (wait)
	{
		m_jobs->Wait(m_pendingSave);
	}
	if (!m_pendingSave.IsComplete())
	{
		return;
	}
	if (m_pendingSave.Succeeded())
	{
		ReportSave(GetFitPath(), m_pendingSave.GetResult());
	}
	// Dropped (the job system shut down first): nothing was written; Finish() writes the final files anyway.
	m_pendingSave = Jobs::Task<std::string>();
}

void RideSession::ReportSave(const std::string& path, const std::string& error)
{
	m_lastSaveError = error;
	m_events.push_back(SessionEvent{error.empty() ? SessionEvent::Type::SAVED : SessionEvent::Type::SAVE_FAILED, 0,
	                                error.empty() ? path : error, 0.0});
}

void RideSession::RecordSample(double now)
{
	RideSample sample;
	sample.unixTime     = WallClock(now);
	sample.timerSeconds = std::floor(m_timer + 1e-6);
	if (m_readings.powerWatts)
	{
		sample.powerWatts = (int)std::lround(*m_readings.powerWatts);
	}
	if (m_readings.heartRateBpm)
	{
		sample.heartRateBpm = (int)std::lround(*m_readings.heartRateBpm);
	}
	if (m_readings.cadenceRpm)
	{
		sample.cadenceRpm = (int)std::lround(*m_readings.cadenceRpm);
	}
	sample.speedMetersPerSecond        = m_speedModel.GetSpeedMetersPerSecond();
	sample.distanceMeters              = m_speedModel.GetDistanceMeters();
	sample.altitudeMeters              = m_speedModel.GetAltitudeMeters();
	sample.gradePercent                = m_controller.GetEffectiveGradePercent();
	sample.ergEnabled                  = m_controller.IsErgEnabled();
	const std::optional<double> target = GetTargetWatts();
	if (target)
	{
		sample.targetWatts = (int)std::lround(*target);
		// The lead-in ramp is a transition, not something to be scored against.
		const bool scorable = !(m_runner && m_runner->IsInLeadIn());
		sample.inTarget     = scorable && sample.powerWatts && IsInTarget(*sample.powerWatts, *target);
		if (!scorable)
		{
			sample.targetWatts.reset();
		}
	}
	m_log.AddSample(sample);
	m_powerSum += sample.powerWatts.value_or(0);
	++m_sampleCount;
	if (sample.targetWatts)
	{
		++m_targetSeconds;
		m_inTargetSeconds += sample.inTarget ? 1 : 0;
	}
}

void RideSession::Pause(double now, bool automatic)
{
	if (m_paused || m_waiting)
	{
		return;
	}
	m_paused     = true;
	m_autoPaused = automatic;
	m_log.AddTimerEvent(WallClock(now), false);
	if (m_runner)
	{
		m_runner->Pause(now);
	}
	m_events.push_back(
	    SessionEvent{automatic ? SessionEvent::Type::AUTO_PAUSED : SessionEvent::Type::PAUSED, 0, {}, 0.0});
}

void RideSession::Resume(double now)
{
	if (!m_paused)
	{
		return;
	}
	m_paused     = false;
	m_autoPaused = false;
	m_zeroPowerSince.reset();
	m_log.AddTimerEvent(WallClock(now), true);
	if (m_runner)
	{
		m_runner->Resume(now);
	}
	m_events.push_back(SessionEvent{SessionEvent::Type::RESUMED, 0, {}, 0.0});
}

void RideSession::SetErgEnabled(bool enabled)
{
	m_controller.SetErgEnabled(enabled);
}

void RideSession::ToggleErg()
{
	m_controller.SetErgEnabled(!m_controller.IsErgEnabled());
}

void RideSession::AdjustResistance(int direction)
{
	m_controller.AdjustResistance(direction);
}

void RideSession::SetTrimMode(TrimMode mode)
{
	m_controller.SetTrimMode(mode);
}

void RideSession::SetFreeRideTarget(double watts)
{
	const double maximum = m_settings.control.defaultMaximumPowerWatts;
	m_freeRideTarget     = std::clamp(std::round(watts), 0.0, maximum);
}

void RideSession::NudgeFreeRideTarget(int direction)
{
	SetFreeRideTarget(m_freeRideTarget + (direction > 0 ? FREE_RIDE_NUDGE_WATTS : -FREE_RIDE_NUDGE_WATTS));
}

void RideSession::TogglePause(double now)
{
	if (m_waiting || m_finished)
	{
		return;
	}
	if (m_paused)
	{
		Resume(now);
	}
	else
	{
		Pause(now, false);
	}
}

void RideSession::SkipStep(double now)
{
	if (m_runner)
	{
		m_runner->SkipStep(now);
	}
}

void RideSession::ExtendStep(double seconds)
{
	if (m_runner)
	{
		m_runner->ExtendStep(seconds);
	}
}

void RideSession::Lap(double now)
{
	if (m_mode != RideMode::FREE_RIDE || m_waiting || m_finished)
	{
		return;
	}
	++m_lapNumber;
	m_lapStartTimer = m_timer;
	m_log.NewLap("Lap " + std::to_string(m_lapNumber), WallClock(now), m_timer);
	m_events.push_back(SessionEvent{SessionEvent::Type::LAP, (size_t)m_lapNumber, {}, 0.0});
}

bool RideSession::Finish(double now)
{
	if (m_finished)
	{
		return m_lastSaveError.empty();
	}
	if (!m_paused && !m_waiting)
	{
		m_log.AddTimerEvent(WallClock(now), false);
	}
	m_log.Close(WallClock(now), m_timer);
	m_finished = true;
	// An autosave still writing would otherwise finish after - and overwrite - the final files.
	CollectBackgroundSave(true);
	return SaveNow();
}

bool RideSession::SaveNow()
{
	if (m_outputBasePath.empty())
	{
		return true;
	}
	RideLog snapshot = m_log;
	if (!m_finished)
	{
		// An autosave is a complete, valid file: close the copy as if the ride ended now.
		snapshot.Close(snapshot.GetEndUnix(),
		               m_log.GetSamples().empty() ? 0.0 : m_log.GetSamples().back().timerSeconds);
	}
	CollectBackgroundSave(true);
	RideSaveJob job;
	job.snapshot                 = std::move(snapshot);
	job.options.utcOffsetSeconds = m_utcOffsetSeconds;
	job.options.serialNumber     = (uint32_t)(m_unixStart & 0x7FFFFFFF);
	job.fitPath                  = GetFitPath();
	job.jsonPath                 = GetJsonPath();
	job.jsonText                 = EncodeJson();
	const std::string error      = WriteRideFiles(job);
	ReportSave(job.fitPath, error);
	return error.empty();
}

std::string RideSession::EncodeJson() const
{
	nlohmann::json root;
	root["format"]           = "struktur-ride";
	root["version"]          = 1;
	root["mode"]             = m_mode == RideMode::WORKOUT ? "workout" : "freeRide";
	root["startUnix"]        = m_unixStart;
	root["utcOffsetSeconds"] = m_utcOffsetSeconds;
	root["rider"]            = {{"name", m_rider.name},
	                            {"ftpWatts", m_rider.ftpWatts},
	                            {"maxHeartRateBpm", m_rider.maxHeartRateBpm},
	                            {"thresholdHeartRateBpm", m_rider.thresholdHeartRateBpm},
	                            {"weightKg", m_rider.weightKg}};
	if (m_runner)
	{
		root["workout"] = {{"name", m_runner->GetWorkout().name}, {"source", m_runner->GetWorkout().sourcePath}};
	}
	nlohmann::json samples = nlohmann::json::array();
	for (const RideSample& sample : m_log.GetSamples())
	{
		nlohmann::json entry;
		entry["t"]        = sample.unixTime;
		entry["timer"]    = sample.timerSeconds;
		entry["power"]    = sample.powerWatts ? nlohmann::json(*sample.powerWatts) : nlohmann::json(nullptr);
		entry["hr"]       = sample.heartRateBpm ? nlohmann::json(*sample.heartRateBpm) : nlohmann::json(nullptr);
		entry["cadence"]  = sample.cadenceRpm ? nlohmann::json(*sample.cadenceRpm) : nlohmann::json(nullptr);
		entry["speed"]    = std::round(sample.speedMetersPerSecond * 1000.0) / 1000.0;
		entry["distance"] = std::round(sample.distanceMeters * 100.0) / 100.0;
		entry["altitude"] = std::round(sample.altitudeMeters * 100.0) / 100.0;
		entry["target"]   = sample.targetWatts ? nlohmann::json(*sample.targetWatts) : nlohmann::json(nullptr);
		entry["grade"]    = std::round(sample.gradePercent * 100.0) / 100.0;
		entry["erg"]      = sample.ergEnabled;
		samples.push_back(std::move(entry));
	}
	root["samples"]     = std::move(samples);
	nlohmann::json laps = nlohmann::json::array();
	for (const RideLap& lap : m_log.GetLaps())
	{
		const RideSummary summary = m_log.SummarizeLap(lap);
		laps.push_back(
		    {{"label", lap.label},
		     {"startUnix", lap.startUnix},
		     {"endUnix", lap.endUnix},
		     {"timerSeconds", summary.timerSeconds},
		     {"distanceMeters", summary.distanceMeters},
		     {"averagePower", summary.averagePower ? nlohmann::json(*summary.averagePower) : nlohmann::json(nullptr)}});
	}
	root["laps"]           = std::move(laps);
	const RideSummary ride = m_log.SummarizeRide();
	root["summary"]        = {
        {"timerSeconds", ride.timerSeconds},
        {"elapsedSeconds", ride.elapsedSeconds},
        {"distanceMeters", ride.distanceMeters},
        {"ascentMeters", ride.ascentMeters},
        {"averagePower", ride.averagePower ? nlohmann::json(*ride.averagePower) : nlohmann::json(nullptr)},
        {"maximumPower", ride.maximumPower ? nlohmann::json(*ride.maximumPower) : nlohmann::json(nullptr)},
        {"averageHeartRate", ride.averageHeartRate ? nlohmann::json(*ride.averageHeartRate) : nlohmann::json(nullptr)},
        {"targetSeconds", ride.targetSeconds},
        {"inTargetSeconds", ride.inTargetSeconds}};
	return root.dump(1);
}

// --- getters -------------------------------------------------------------------------------------------------------

RideMode RideSession::GetMode() const
{
	return m_mode;
}
const RiderInfo& RideSession::GetRider() const
{
	return m_rider;
}
const RideSettings& RideSession::GetSettings() const
{
	return m_settings;
}
bool RideSession::IsWaitingToStart() const
{
	return m_waiting;
}
bool RideSession::IsPaused() const
{
	return m_paused;
}
bool RideSession::IsAutoPaused() const
{
	return m_autoPaused;
}
bool RideSession::IsFinished() const
{
	return m_finished;
}
double RideSession::GetTimerSeconds() const
{
	return m_timer;
}
double RideSession::GetElapsedSeconds() const
{
	return std::max(0.0, m_lastNow - m_startNow);
}
std::optional<double> RideSession::GetPower() const
{
	return m_smoothedPower;
}
std::optional<double> RideSession::GetInstantPower() const
{
	return m_readings.powerWatts;
}
std::optional<double> RideSession::GetCadence() const
{
	return m_readings.cadenceRpm;
}
std::optional<double> RideSession::GetHeartRate() const
{
	return m_readings.heartRateBpm;
}
double RideSession::GetSpeedMetersPerSecond() const
{
	return m_speedModel.GetSpeedMetersPerSecond();
}
double RideSession::GetDistanceMeters() const
{
	return m_speedModel.GetDistanceMeters();
}
double RideSession::GetElevationGainMeters() const
{
	return m_speedModel.GetElevationGainMeters();
}
void RideSession::SetRoadGrade(std::optional<double> gradePercent)
{
	m_controller.SetRoadGrade(gradePercent);
}

double RideSession::GetChosenGradePercent() const
{
	return m_controller.GetGradePercent();
}

double RideSession::GetGradePercent() const
{
	return m_controller.GetEffectiveGradePercent();
}
int RideSession::GetLapNumber() const
{
	return m_lapNumber;
}
double RideSession::GetLapSeconds() const
{
	return std::max(0.0, m_timer - m_lapStartTimer);
}
double RideSession::GetFreeRideTarget() const
{
	return m_freeRideTarget;
}
const SensorReadings& RideSession::GetReadings() const
{
	return m_readings;
}
const TrainerController& RideSession::GetController() const
{
	return m_controller;
}
const WorkoutRunner* RideSession::GetRunner() const
{
	return m_runner.get();
}
const RideLog& RideSession::GetLog() const
{
	return m_log;
}
std::string RideSession::GetFitPath() const
{
	return m_outputBasePath + ".fit";
}
std::string RideSession::GetJsonPath() const
{
	return m_outputBasePath + ".json";
}
const std::string& RideSession::GetLastSaveError() const
{
	return m_lastSaveError;
}

double RideSession::GetAverageSpeedMetersPerSecond() const
{
	return m_timer > 0.0 ? m_speedModel.GetDistanceMeters() / m_timer : 0.0;
}

std::optional<double> RideSession::GetAveragePower() const
{
	return m_sampleCount > 0 ? std::optional<double>(m_powerSum / m_sampleCount) : std::nullopt;
}

int RideSession::GetPowerZone() const
{
	return m_smoothedPower ? PowerZone(*m_smoothedPower, m_rider.ftpWatts) : 0;
}

int RideSession::GetHeartRateZone() const
{
	return m_readings.heartRateBpm ? HeartRateZone(*m_readings.heartRateBpm, m_settings.heartRateZoneModel,
	                                               m_rider.maxHeartRateBpm, m_rider.thresholdHeartRateBpm)
	                               : 0;
}

std::optional<double> RideSession::GetStepTargetWatts() const
{
	return CurrentStepTarget();
}

std::optional<double> RideSession::GetTargetWatts() const
{
	if (m_mode == RideMode::WORKOUT)
	{
		// ERG on: what the trainer holds (step + trim). ERG off: the step's own target, shown and scored only.
		return m_controller.IsErgEnabled() && m_controller.GetErgTargetWatts() ? m_controller.GetErgTargetWatts()
		                                                                       : CurrentStepTarget();
	}
	return m_controller.IsErgEnabled() ? m_controller.GetErgTargetWatts() : std::nullopt;
}

int RideSession::GetInTargetSeconds() const
{
	return m_inTargetSeconds;
}

int RideSession::GetTargetSeconds() const
{
	return m_targetSeconds;
}

std::vector<SessionEvent> RideSession::TakeEvents()
{
	return std::exchange(m_events, {});
}
}  // namespace Struktur::Ride
