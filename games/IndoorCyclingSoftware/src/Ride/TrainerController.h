#pragma once

#include <cstdint>
#include <optional>

#include "Engine/Fitness/Devices/DeviceTypes.h"
#include "Ride/SpeedModel.h"

namespace Struktur::Ride
{
enum class TrimMode : uint8_t
{
	WATTS,
	PERCENT,
};

struct TrainerControlSettings
{
	double rampSeconds               = 2.5;  // ERG <-> SIM handover and cadence-protection recovery
	double minCommandIntervalSeconds = 1.0;  // at most one trainer command per second
	TrimMode trimMode                = TrimMode::WATTS;
	int trimStepWatts                = 5;
	double trimStepPercent           = 1.0;
	double maxTrimWatts              = 300.0;
	double maxTrimPercent            = 50.0;
	double gradeStepPercent          = 0.5;
	// Used when the trainer doesn't report its own ranges.
	double defaultMinimumGradePercent = -10.0;
	double defaultMaximumGradePercent = 20.0;
	int defaultMinimumPowerWatts      = 0;
	int defaultMaximumPowerWatts      = 1500;
	// ERG "death spiral" protection: if cadence stays below lowCadenceRpm for lowCadenceSeconds, the target eases down
	// towards protectionMinimumFraction of itself; once cadence is back above recoveryCadenceRpm for
	// recoverySeconds it ramps back up.
	bool cadenceProtection           = true;
	double lowCadenceRpm             = 50.0;
	double lowCadenceSeconds         = 3.0;
	double recoveryCadenceRpm        = 65.0;
	double recoverySeconds           = 2.0;
	double protectionMinimumFraction = 0.5;
	double protectionEaseSeconds     = 5.0;
};

struct ControlInputs
{
	double now = 0.0;
	std::optional<double> powerWatts;  // smoothed rider power, used to start ramps from where the rider is
	std::optional<double> cadenceRpm;
	double speedMetersPerSecond = 0.0;  // simulated speed, for the ERG -> SIM handover grade
};

// Decides what the trainer is told, and when:
//
//  - ERG on with a base target (free-ride target or workout step): target power = (base + trim), or base * (1 + trim
//    %), clamped to the trainer's power range, times the cadence-protection factor.
//  - Otherwise SIM: the "resistance" is the simulated grade, clamped to the trainer's inclination range.
//  - Switching between the two never jumps: ERG starts from the rider's current power and SIM starts from the grade
//    that the rider's current power holds at the current speed, then both ramp to the real target over rampSeconds.
//  - Commands are rate-limited to one per minCommandIntervalSeconds and only sent when the value changes; they go
//    through Trainer::SetTargetPower / SetSimulation, i.e. the FTMS control-point queue.
class TrainerController
{
public:
	enum class Mode : uint8_t
	{
		NONE,
		ERG,
		SIM,
	};

	explicit TrainerController(const TrainerControlSettings& settings      = TrainerControlSettings(),
	                           const SpeedModelParameters& speedParameters = SpeedModelParameters());

	void SetSettings(const TrainerControlSettings& settings);
	void SetSpeedParameters(const SpeedModelParameters& speedParameters);

	void SetErgEnabled(bool enabled);
	bool IsErgEnabled() const;
	// The un-trimmed ERG target in watts; nullopt = nothing to hold (ERG then falls back to SIM).
	void SetBaseTarget(std::optional<double> watts);
	std::optional<double> GetBaseTarget() const;

	// +1 / -1: ERG on -> trim the target; ERG off -> change the grade.
	void AdjustResistance(int direction);
	void SetTrimMode(TrimMode mode);
	TrimMode GetTrimMode() const;
	double GetTrimWatts() const;
	double GetTrimPercent() const;
	double GetGradePercent() const;  // the rider's chosen grade (SIM resistance)
	void SetGradePercent(double gradePercent);
	// The procedural world's road grade under the rider. When set, SIM sends this instead of the chosen grade (the
	// world blends its road towards the chosen grade, so the rider still controls it with -/+). nullopt = no world.
	void SetRoadGrade(std::optional<double> gradePercent);
	std::optional<double> GetRoadGrade() const;

	// Applies capability ranges (call whenever the trainer's capabilities change).
	void SetCapabilities(const Fitness::TrainerCapabilities& capabilities);
	double GetMinimumGrade() const;
	double GetMaximumGrade() const;

	// Runs the control logic and sends at most one command. trainer may be null (no trainer: nothing is sent, the
	// state still advances so the UI stays truthful).
	void Update(const ControlInputs& inputs, Fitness::Trainer* trainer);

	Mode GetMode() const;
	// The ERG target after trim, clamping and protection (what the trainer is ramping towards), when in ERG.
	std::optional<double> GetErgTargetWatts() const;
	// The value last sent (watts in ERG, grade % in SIM), and when.
	std::optional<double> GetLastSentValue() const;
	// The grade currently being sent/ramped (SIM), for the speed model.
	double GetEffectiveGradePercent() const;
	bool IsRamping(double now) const;
	bool IsCadenceProtectionActive() const;
	double GetProtectionFactor() const;

private:
	double ClampPower(double watts) const;
	double ClampGrade(double grade) const;
	std::optional<double> DesiredErgTarget() const;
	void UpdateProtection(const ControlInputs& inputs, double deltaSeconds);

	TrainerControlSettings m_settings;
	SpeedModelParameters m_speedParameters;
	bool m_ergEnabled;
	std::optional<double> m_baseTarget;
	double m_trimWatts;
	double m_trimPercent;
	double m_gradePercent;
	std::optional<double> m_roadGrade;
	double m_minimumPower;
	double m_maximumPower;
	double m_minimumGrade;
	double m_maximumGrade;

	Mode m_mode;
	double m_rampFrom;
	double m_rampStart;
	double m_currentValue;  // output this tick (watts or grade)
	std::optional<double> m_lastSentValue;
	Mode m_lastSentMode;
	double m_lastSentTime;
	bool m_hasSent;

	double m_protectionFactor;
	std::optional<double> m_lowCadenceSince;
	std::optional<double> m_recoveredSince;
	double m_lastUpdate;
	bool m_hasUpdated;
};
}  // namespace Struktur::Ride
