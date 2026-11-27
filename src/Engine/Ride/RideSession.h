#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Fitness/Devices/DeviceTypes.h"
#include "Engine/Jobs/Task.h"
#include "Engine/Ride/RideLog.h"
#include "Engine/Ride/RideSettings.h"
#include "Engine/Ride/SpeedModel.h"
#include "Engine/Ride/TrainerController.h"
#include "Engine/Ride/Workout.h"
#include "Engine/Ride/WorkoutRunner.h"

namespace Struktur::Ride
{
enum class RideMode : uint8_t
{
	FREE_RIDE,
	WORKOUT,
};

struct RiderInfo
{
	std::string name;
	double ftpWatts              = 200.0;
	double maxHeartRateBpm       = 190.0;
	double thresholdHeartRateBpm = 165.0;
	double weightKg              = 75.0;
};

// What the sensors say this frame (chosen by the engine side from trainer / power meter / HR strap).
struct SensorReadings
{
	std::optional<double> powerWatts;
	std::optional<double> cadenceRpm;
	std::optional<double> heartRateBpm;
	bool trainerAssigned     = false;
	bool trainerConnected    = false;
	bool heartRateAssigned   = false;
	bool heartRateConnected  = false;
	bool powerMeterAssigned  = false;
	bool powerMeterConnected = false;
};

struct SessionEvent
{
	enum class Type : uint8_t
	{
		STARTED,  // first pedal stroke: the timer is running
		STEP_STARTED,
		CUE,
		AUTO_PAUSED,
		PAUSED,
		RESUMED,
		LAP,
		WORKOUT_FINISHED,  // last step done; the ride carries on as free riding until ended
		SAVED,
		SAVE_FAILED,
	};

	Type type        = Type::STARTED;
	size_t stepIndex = 0;
	std::string message;
	double durationSeconds = 0.0;
};

const char* ToString(SessionEvent::Type type);

// One ride, Free Ride or Workout. Game-thread only, engine-agnostic: the engine feeds sensor readings and the
// trainer every frame and reads snapshots back.
//
//  - The timer starts with the first pedal stroke and auto-pauses after `autoPauseSeconds` without power (manual
//    pause stays paused until resumed). The workout clock follows the ride timer.
//  - Speed, distance and virtual elevation come from SpeedModel on the effective grade.
//  - One sample per second of moving time goes into the RideLog; a workout opens a lap per step, Free Ride on Lap().
//  - FIT + JSON are written on Finish() and autosaved every `autosaveSeconds`. With SetBackgroundJobs(), an autosave
//    only snapshots the ride on the calling thread; encoding and writing the files run as a background task.
class RideSession
{
public:
	RideSession(RideMode mode, RiderInfo rider, RideSettings settings, std::optional<Workout> workout,
	            std::string outputBasePath, int64_t unixStartTime, int32_t utcOffsetSeconds);

	// Autosaves write their files on this job system instead of the calling thread (null = write in place). It must
	// outlive the session.
	void SetBackgroundJobs(Jobs::JobSystem* jobs);
	bool IsSaving() const;  // a background autosave is in flight

	void Start(double now);
	void Update(double now, const SensorReadings& readings, Fitness::Trainer* trainer);

	// --- controls ----------------------------------------------------------------------------------------------
	void SetErgEnabled(bool enabled);
	void ToggleErg();
	void AdjustResistance(int direction);
	void SetTrimMode(TrimMode mode);
	void SetFreeRideTarget(double watts);
	void NudgeFreeRideTarget(int direction);
	void TogglePause(double now);
	void SkipStep(double now);
	void ExtendStep(double seconds);
	void Lap(double now);
	// The procedural world's road grade under the rider (SIM simulates it); nullopt = no world.
	void SetRoadGrade(std::optional<double> gradePercent);
	// Stops the ride and writes the final FIT + JSON. Returns false if writing failed (see GetLastSaveError).
	bool Finish(double now);
	bool SaveNow();

	// --- state -------------------------------------------------------------------------------------------------
	RideMode GetMode() const;
	const RiderInfo& GetRider() const;
	const RideSettings& GetSettings() const;
	bool IsWaitingToStart() const;
	bool IsPaused() const;
	bool IsAutoPaused() const;
	bool IsFinished() const;
	double GetTimerSeconds() const;
	double GetElapsedSeconds() const;        // wall time since Start
	std::optional<double> GetPower() const;  // smoothed (3 s)
	std::optional<double> GetInstantPower() const;
	std::optional<double> GetCadence() const;
	std::optional<double> GetHeartRate() const;
	double GetSpeedMetersPerSecond() const;
	double GetAverageSpeedMetersPerSecond() const;
	std::optional<double> GetAveragePower() const;
	double GetDistanceMeters() const;
	double GetElevationGainMeters() const;
	double GetGradePercent() const;
	double GetChosenGradePercent() const;  // the rider's -/+ grade (the world blends its road towards it)
	int GetPowerZone() const;
	int GetHeartRateZone() const;
	// The target the rider should hold: the workout step's (FTP-relative, lead-in applied) or the Free Ride ERG target.
	std::optional<double> GetTargetWatts() const;
	std::optional<double> GetStepTargetWatts() const;  // workout only, before trim
	int GetLapNumber() const;
	double GetLapSeconds() const;
	int GetInTargetSeconds() const;
	int GetTargetSeconds() const;
	double GetFreeRideTarget() const;
	const SensorReadings& GetReadings() const;

	const TrainerController& GetController() const;
	const WorkoutRunner* GetRunner() const;
	const RideLog& GetLog() const;
	std::string GetFitPath() const;
	std::string GetJsonPath() const;
	const std::string& GetLastSaveError() const;

	std::vector<SessionEvent> TakeEvents();

private:
	void Pause(double now, bool automatic);
	void Resume(double now);
	void RecordSample(double now);
	int64_t WallClock(double now) const;
	std::optional<double> CurrentStepTarget() const;
	bool IsInTarget(double power, double target) const;
	std::string EncodeJson() const;
	void StartBackgroundSave();
	void CollectBackgroundSave(bool wait);
	void ReportSave(const std::string& path, const std::string& error);

	RideMode m_mode;
	RiderInfo m_rider;
	RideSettings m_settings;
	std::unique_ptr<WorkoutRunner> m_runner;
	std::string m_outputBasePath;
	int64_t m_unixStart;
	int32_t m_utcOffsetSeconds;

	SpeedModel m_speedModel;
	TrainerController m_controller;
	RideLog m_log;
	SensorReadings m_readings;

	bool m_started;
	bool m_waiting;
	bool m_paused;
	bool m_autoPaused;
	bool m_finished;
	double m_startNow;
	double m_lastNow;
	double m_timer;
	double m_nextSampleTimer;
	std::optional<double> m_zeroPowerSince;
	std::optional<double> m_smoothedPower;
	double m_freeRideTarget;
	double m_lapStartTimer;
	int m_lapNumber;
	double m_lastSaveNow;
	std::string m_lastSaveError;
	double m_powerSum;
	int m_sampleCount;
	int m_targetSeconds;
	int m_inTargetSeconds;
	std::vector<SessionEvent> m_events;
	Jobs::JobSystem* m_jobs = nullptr;
	Jobs::Task<std::string> m_pendingSave;  // result: the error, empty on success
};
}  // namespace Struktur::Ride
