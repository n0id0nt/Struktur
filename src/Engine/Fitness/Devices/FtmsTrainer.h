#pragma once

#include <optional>
#include <string>

#include "Engine/Fitness/Ble/IBleCentral.h"
#include "Engine/Fitness/Devices/BleLink.h"
#include "Engine/Fitness/Devices/ControlPointQueue.h"
#include "Engine/Fitness/Devices/DeviceTypes.h"
#include "Engine/Fitness/Protocol/Ftms.h"

namespace Struktur::Fitness
{
struct FtmsTrainerSettings
{
	ReconnectPolicy reconnect;
	double controlPointTimeoutSeconds = 5.0;
	// Safety cap applied on top of the trainer's own Supported Power Range. nullopt = trainer range only.
	std::optional<int> maximumTargetPowerWatts;
	// SetManualResistance(0..1) maps onto this grade span when the trainer supports simulation.
	double manualGradeMinimumPercent = 0.0;
	double manualGradeMaximumPercent = 10.0;
	// Request Control is retried this many times on timeout before the trainer is reported unresponsive.
	int requestControlRetries = 2;
	// Consecutive target-command timeouts before TRAINER_UNRESPONSIVE is raised.
	int unresponsiveAfterTimeouts = 3;
	// Upper bound on the graceful-stop handshake (Reset) before the link is dropped anyway.
	double shutdownTimeoutSeconds = 2.0;
};

// FTMS (Fitness Machine Service, 0x1826) smart trainer over BLE - Elite Direto, Wahoo KICKR (fw >= 3.5.1) etc.
//
// Lifecycle on every (re)connect:
//   read Feature (required) + Supported Power/Resistance Range (optional) -> subscribe Indoor Bike Data (notify),
//   Control Point (indicate), Machine Status (notify, optional) -> Request Control -> Start/Resume -> re-apply the
//   last target the game asked for.
//
// Targets are clamped to the trainer's advertised ranges and the safety cap, and any op code the trainer answers
// "not supported" is treated as unsupported from then on (the game sees COMMAND_UNSUPPORTED once, then
// UNSUPPORTED results). Stop() sends Reset - which releases control and the target load - before disconnecting.
class FtmsTrainer : public Trainer
{
public:
	FtmsTrainer(IBleCentral& central, BleAddress address, std::string name, FtmsTrainerSettings settings = {});

	std::string GetIdentifier() const override;
	const std::string& GetName() const override;
	ConnectionState GetConnectionState() const override;

	void Start(double now) override;
	void Update(double now) override;
	void Stop(double now) override;
	bool IsStopped() const override;

	const TrainerSample& GetLatestSample() const override;
	const TrainerCapabilities& GetCapabilities() const override;
	ControlState GetControlState() const override;
	TrainerMode GetMode() const override;

	CommandResult SetTargetPower(int watts) override;
	CommandResult SetSimulation(const SimulationTarget& target) override;
	CommandResult SetResistanceLevel(double level) override;
	CommandResult SetManualResistance(double fraction) override;
	CommandResult ClearTarget() override;

private:
	struct Target
	{
		TrainerMode mode = TrainerMode::FREE_RIDE;
		int powerWatts   = 0;
		SimulationTarget simulation;
		double resistanceLevel = 0.0;
		double manualFraction  = 0.0;
	};

	static BleLinkConfig MakeLinkConfig(BleAddress address, const ReconnectPolicy& reconnect);

	void HandleLinkEvent(LinkEvent& event, double now);
	void HandleNotification(const LinkEvent& event, double now);
	void HandleOutcome(const ControlPointOutcome& outcome);
	void ApplyFeatures(const Ftms::Features& features);
	void BeginControlSequence();
	void HandleControlLost(const std::string& reason);
	void PumpControlPoint(double now);

	// Validates the stored target against capabilities and queues its command. Returns the result for the caller.
	CommandResult ApplyTarget(double requestedValue);
	int ClampPower(int watts) const;
	double ClampResistance(double level) const;

	BleLink m_link;
	std::string m_name;
	FtmsTrainerSettings m_settings;
	ControlPointQueue m_controlPoint;
	TrainerSample m_sample;
	TrainerCapabilities m_capabilities;
	ControlState m_controlState;
	Target m_target;
	int m_requestControlAttempts;
	int m_controlLostRestarts;
	int m_consecutiveTimeouts;
	bool m_reportedUnresponsive;
	uint32_t m_controlPointWriteId;
	uint32_t m_controlPointRequestId;
	bool m_stopping;
	bool m_stopped;
	double m_stopDeadlineSeconds;
};
}  // namespace Struktur::Fitness
