#include "FtmsTrainer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Debug/Assertions.h"

namespace Struktur::Fitness
{
// FTMS v1.0 Set Target Resistance Level is uint8 at 0.1 resolution.
static constexpr double WIRE_RESISTANCE_MAXIMUM = 25.5;
// Upper bound used before the trainer's Supported Power Range is known.
static constexpr int DEFAULT_MAXIMUM_POWER_WATTS = 2000;

static const BleUuid FTMS_SERVICE = BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE);
static const BleCharacteristicRef FEATURE_CHARACTERISTIC{FTMS_SERVICE, BleUuid::FromShort(Uuids::FTMS_FEATURE)};
static const BleCharacteristicRef POWER_RANGE_CHARACTERISTIC{FTMS_SERVICE,
                                                             BleUuid::FromShort(Uuids::FTMS_SUPPORTED_POWER)};
static const BleCharacteristicRef RESISTANCE_RANGE_CHARACTERISTIC{FTMS_SERVICE,
                                                                  BleUuid::FromShort(Uuids::FTMS_SUPPORTED_RESISTANCE)};
static const BleCharacteristicRef INDOOR_BIKE_DATA_CHARACTERISTIC{FTMS_SERVICE,
                                                                  BleUuid::FromShort(Uuids::FTMS_INDOOR_BIKE_DATA)};
static const BleCharacteristicRef CONTROL_POINT_CHARACTERISTIC{FTMS_SERVICE,
                                                               BleUuid::FromShort(Uuids::FTMS_CONTROL_POINT)};
static const BleCharacteristicRef MACHINE_STATUS_CHARACTERISTIC{FTMS_SERVICE,
                                                                BleUuid::FromShort(Uuids::FTMS_MACHINE_STATUS)};

static std::string FormatValue(const char* format, double value)
{
	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), format, value);
	return std::string(buffer);
}

FtmsTrainer::FtmsTrainer(IBleCentral& central, BleAddress address, std::string name, FtmsTrainerSettings settings)
    : m_link(central, MakeLinkConfig(address, settings.reconnect)),
      m_name(std::move(name)),
      m_settings(settings),
      m_controlPoint(settings.controlPointTimeoutSeconds),
      m_sample(),
      m_capabilities(),
      m_controlState(ControlState::NONE),
      m_target(),
      m_requestControlAttempts(0),
      m_controlLostRestarts(0),
      m_consecutiveTimeouts(0),
      m_reportedUnresponsive(false),
      m_controlPointWriteId(0),
      m_controlPointRequestId(0),
      m_stopping(false),
      m_stopped(false),
      m_stopDeadlineSeconds(0.0)
{
}

BleLinkConfig FtmsTrainer::MakeLinkConfig(BleAddress address, const ReconnectPolicy& reconnect)
{
	BleLinkConfig config;
	config.address   = address;
	config.reconnect = reconnect;
	config.setup     = {
        {LinkSetupStep::Kind::READ, FEATURE_CHARACTERISTIC, SubscriptionKind::NOTIFY, true},
        {LinkSetupStep::Kind::READ, POWER_RANGE_CHARACTERISTIC, SubscriptionKind::NOTIFY, false},
        {LinkSetupStep::Kind::READ, RESISTANCE_RANGE_CHARACTERISTIC, SubscriptionKind::NOTIFY, false},
        {LinkSetupStep::Kind::SUBSCRIBE, INDOOR_BIKE_DATA_CHARACTERISTIC, SubscriptionKind::NOTIFY, true},
        {LinkSetupStep::Kind::SUBSCRIBE, CONTROL_POINT_CHARACTERISTIC, SubscriptionKind::INDICATE, true},
        {LinkSetupStep::Kind::SUBSCRIBE, MACHINE_STATUS_CHARACTERISTIC, SubscriptionKind::NOTIFY, false},
    };
	return config;
}

std::string FtmsTrainer::GetIdentifier() const
{
	return FormatAddress(m_link.GetAddress());
}

const std::string& FtmsTrainer::GetName() const
{
	return m_name;
}

ConnectionState FtmsTrainer::GetConnectionState() const
{
	return m_stopped ? ConnectionState::STOPPED : m_link.GetConnectionState();
}

void FtmsTrainer::Start(double now)
{
	m_stopped  = false;
	m_stopping = false;
	m_link.Start(now);
}

void FtmsTrainer::Update(double now)
{
	if (m_stopped)
	{
		return;
	}

	std::vector<LinkEvent> events = m_link.Update(now);
	for (LinkEvent& event : events)
	{
		HandleLinkEvent(event, now);
	}
	PumpControlPoint(now);

	if (m_stopping && (m_controlPoint.IsIdle() || now >= m_stopDeadlineSeconds || !m_link.IsReady()))
	{
		m_link.Stop();
		m_controlPoint.CancelAll();
		m_controlPoint.TakeOutcomes();
		m_controlState = ControlState::NONE;
		m_stopping     = false;
		m_stopped      = true;
	}
}

void FtmsTrainer::Stop(double now)
{
	if (m_stopped || m_stopping)
	{
		return;
	}
	if (m_link.IsReady() && m_controlState == ControlState::GRANTED)
	{
		// Reset releases control and drops whatever target load is active, so the trainer is not left in ERG at
		// the game's last wattage for the next app. Wait for its response (bounded), then disconnect.
		m_controlPoint.CancelQueued();
		m_controlPoint.Enqueue(Ftms::OpCode::RESET, Ftms::BuildReset(), false);
		m_stopping            = true;
		m_stopDeadlineSeconds = now + m_settings.shutdownTimeoutSeconds;
		PumpControlPoint(now);
		return;
	}
	m_link.Stop();
	m_controlPoint.CancelAll();
	m_controlPoint.TakeOutcomes();
	m_controlState = ControlState::NONE;
	m_stopped      = true;
}

bool FtmsTrainer::IsStopped() const
{
	return m_stopped;
}

const TrainerSample& FtmsTrainer::GetLatestSample() const
{
	return m_sample;
}

const TrainerCapabilities& FtmsTrainer::GetCapabilities() const
{
	return m_capabilities;
}

ControlState FtmsTrainer::GetControlState() const
{
	return m_controlState;
}

TrainerMode FtmsTrainer::GetMode() const
{
	return m_target.mode;
}

CommandResult FtmsTrainer::SetTargetPower(int watts)
{
	const Target previous = m_target;
	m_target.mode         = TrainerMode::ERG;
	m_target.powerWatts   = watts;
	CommandResult result  = ApplyTarget(watts);
	if (!result.IsAccepted())
	{
		m_target = previous;
	}
	return result;
}

CommandResult FtmsTrainer::SetSimulation(const SimulationTarget& target)
{
	const Target previous = m_target;
	m_target.mode         = TrainerMode::SIMULATION;
	m_target.simulation   = target;
	CommandResult result  = ApplyTarget(target.gradePercent);
	if (!result.IsAccepted())
	{
		m_target = previous;
	}
	return result;
}

CommandResult FtmsTrainer::SetResistanceLevel(double level)
{
	const Target previous    = m_target;
	m_target.mode            = TrainerMode::RESISTANCE;
	m_target.resistanceLevel = level;
	CommandResult result     = ApplyTarget(level);
	if (!result.IsAccepted())
	{
		m_target = previous;
	}
	return result;
}

CommandResult FtmsTrainer::SetManualResistance(double fraction)
{
	const Target previous   = m_target;
	m_target.mode           = TrainerMode::MANUAL;
	m_target.manualFraction = std::clamp(fraction, 0.0, 1.0);
	CommandResult result    = ApplyTarget(fraction);
	if (!result.IsAccepted())
	{
		m_target = previous;
	}
	return result;
}

CommandResult FtmsTrainer::ClearTarget()
{
	const Target previous = m_target;
	m_target.mode         = TrainerMode::FREE_RIDE;
	CommandResult result  = ApplyTarget(0.0);
	if (!result.IsAccepted())
	{
		m_target = previous;
	}
	return result;
}

CommandResult FtmsTrainer::ApplyTarget(double requestedValue)
{
	CommandResult result;
	result.requestedValue = requestedValue;
	result.appliedValue   = requestedValue;

	if (m_stopping || m_stopped || m_controlState == ControlState::DENIED)
	{
		result.status = CommandResult::Status::CONTROL_DENIED;
		return result;
	}

	const bool capabilitiesKnown = m_capabilities.known;
	const bool ready             = m_link.IsReady() && capabilitiesKnown;

	switch (m_target.mode)
	{
		case TrainerMode::FREE_RIDE:
		{
			if (!ready)
			{
				result.status = CommandResult::Status::DEFERRED;
				return result;
			}
			if (m_capabilities.supportsSimulation)
			{
				// Flat road is the closest FTMS has to "no target".
				Ftms::SimulationParameters flat;
				flat.rollingResistanceCoefficient = m_target.simulation.rollingResistanceCoefficient;
				flat.windResistanceCoefficient    = m_target.simulation.windResistanceCoefficient;
				m_controlPoint.Enqueue(Ftms::OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS,
				                       Ftms::BuildSetIndoorBikeSimulation(flat), true);
			}
			else
			{
				// No simulation: leaving ERG needs Reset (which also drops control), then take control again.
				m_controlPoint.CancelQueued();
				m_controlPoint.Enqueue(Ftms::OpCode::RESET, Ftms::BuildReset(), false);
				m_controlState = ControlState::REQUESTING;
				m_controlPoint.Enqueue(Ftms::OpCode::REQUEST_CONTROL, Ftms::BuildRequestControl(), false);
				m_controlPoint.Enqueue(Ftms::OpCode::START_OR_RESUME, Ftms::BuildStartOrResume(), false);
			}
			result.status = CommandResult::Status::QUEUED;
			return result;
		}
		case TrainerMode::ERG:
		{
			if (capabilitiesKnown && !m_capabilities.supportsTargetPower)
			{
				result.status = CommandResult::Status::UNSUPPORTED;
				return result;
			}
			const int clamped   = ClampPower(m_target.powerWatts);
			result.appliedValue = clamped;
			if (clamped != m_target.powerWatts)
			{
				Emit(DeviceEvent::Type::TARGET_CLAMPED, "requested " + std::to_string(m_target.powerWatts) +
				                                            " W, applying " + std::to_string(clamped) + " W");
			}
			if (!ready)
			{
				result.status = CommandResult::Status::DEFERRED;
				return result;
			}
			m_controlPoint.Enqueue(Ftms::OpCode::SET_TARGET_POWER,
			                       Ftms::BuildSetTargetPower(static_cast<int16_t>(clamped)), true);
			result.status =
			    clamped != m_target.powerWatts ? CommandResult::Status::CLAMPED : CommandResult::Status::QUEUED;
			return result;
		}
		case TrainerMode::SIMULATION:
		{
			if (capabilitiesKnown && !m_capabilities.supportsSimulation)
			{
				result.status = CommandResult::Status::UNSUPPORTED;
				return result;
			}
			if (!ready)
			{
				result.status = CommandResult::Status::DEFERRED;
				return result;
			}
			Ftms::SimulationParameters parameters;
			parameters.gradePercent                 = m_target.simulation.gradePercent;
			parameters.windSpeedMetersPerSecond     = m_target.simulation.windSpeedMetersPerSecond;
			parameters.rollingResistanceCoefficient = m_target.simulation.rollingResistanceCoefficient;
			parameters.windResistanceCoefficient    = m_target.simulation.windResistanceCoefficient;
			m_controlPoint.Enqueue(Ftms::OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS,
			                       Ftms::BuildSetIndoorBikeSimulation(parameters), true);
			result.status = CommandResult::Status::QUEUED;
			return result;
		}
		case TrainerMode::RESISTANCE:
		{
			if (capabilitiesKnown && !m_capabilities.supportsResistanceLevel)
			{
				result.status = CommandResult::Status::UNSUPPORTED;
				return result;
			}
			const double clamped = ClampResistance(m_target.resistanceLevel);
			result.appliedValue  = clamped;
			if (!ready)
			{
				result.status = CommandResult::Status::DEFERRED;
				return result;
			}
			m_controlPoint.Enqueue(Ftms::OpCode::SET_TARGET_RESISTANCE_LEVEL,
			                       Ftms::BuildSetTargetResistanceLevel(clamped), true);
			result.status = std::abs(clamped - m_target.resistanceLevel) > 1e-9 ? CommandResult::Status::CLAMPED
			                                                                    : CommandResult::Status::QUEUED;
			return result;
		}
		case TrainerMode::MANUAL:
		{
			if (!ready)
			{
				result.status =
				    (capabilitiesKnown && !m_capabilities.supportsSimulation && !m_capabilities.supportsResistanceLevel)
				        ? CommandResult::Status::UNSUPPORTED
				        : CommandResult::Status::DEFERRED;
				return result;
			}
			const double fraction = m_target.manualFraction;
			if (m_capabilities.supportsSimulation)
			{
				Ftms::SimulationParameters parameters;
				parameters.gradePercent =
				    m_settings.manualGradeMinimumPercent +
				    fraction * (m_settings.manualGradeMaximumPercent - m_settings.manualGradeMinimumPercent);
				parameters.rollingResistanceCoefficient = m_target.simulation.rollingResistanceCoefficient;
				parameters.windResistanceCoefficient    = m_target.simulation.windResistanceCoefficient;
				m_controlPoint.Enqueue(Ftms::OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS,
				                       Ftms::BuildSetIndoorBikeSimulation(parameters), true);
				result.appliedValue = parameters.gradePercent;
				result.status       = CommandResult::Status::QUEUED;
				return result;
			}
			if (m_capabilities.supportsResistanceLevel)
			{
				const double lower  = m_capabilities.minimumResistance.value_or(0.0);
				const double upper  = m_capabilities.maximumResistance.value_or(WIRE_RESISTANCE_MAXIMUM);
				const double level  = ClampResistance(lower + fraction * (upper - lower));
				result.appliedValue = level;
				m_controlPoint.Enqueue(Ftms::OpCode::SET_TARGET_RESISTANCE_LEVEL,
				                       Ftms::BuildSetTargetResistanceLevel(level), true);
				result.status = CommandResult::Status::QUEUED;
				return result;
			}
			result.status = CommandResult::Status::UNSUPPORTED;
			return result;
		}
	}
	result.status = CommandResult::Status::UNSUPPORTED;
	return result;
}

int FtmsTrainer::ClampPower(int watts) const
{
	int lower = m_capabilities.minimumPowerWatts.value_or(0);
	int upper = m_capabilities.maximumPowerWatts.value_or(DEFAULT_MAXIMUM_POWER_WATTS);
	if (m_settings.maximumTargetPowerWatts)
	{
		upper = std::min(upper, *m_settings.maximumTargetPowerWatts);
	}
	lower     = std::min(lower, upper);  // the safety cap wins over a trainer minimum
	int value = std::clamp(watts, lower, upper);

	const int increment = m_capabilities.powerIncrementWatts;
	if (m_capabilities.minimumPowerWatts && increment > 1)
	{
		value = lower + ((value - lower + increment / 2) / increment) * increment;
		while (value > upper)
		{
			value -= increment;
		}
	}
	return value;
}

double FtmsTrainer::ClampResistance(double level) const
{
	const double lower = std::max(0.0, m_capabilities.minimumResistance.value_or(0.0));
	const double upper =
	    std::min(WIRE_RESISTANCE_MAXIMUM, m_capabilities.maximumResistance.value_or(WIRE_RESISTANCE_MAXIMUM));
	return std::clamp(level, lower, std::max(lower, upper));
}

void FtmsTrainer::HandleLinkEvent(LinkEvent& event, double now)
{
	switch (event.type)
	{
		case LinkEvent::Type::READ_VALUE:
		{
			if (event.characteristic == FEATURE_CHARACTERISTIC)
			{
				const std::optional<Ftms::Features> features = Ftms::ParseFeatures(event.value);
				if (features)
				{
					ApplyFeatures(*features);
				}
				else
				{
					DEBUG_WARNING("FTMS Feature characteristic malformed (%zu bytes)", event.value.size());
					m_capabilities = TrainerCapabilities();
				}
			}
			else if (event.characteristic == POWER_RANGE_CHARACTERISTIC)
			{
				const std::optional<Ftms::PowerRange> range = Ftms::ParsePowerRange(event.value);
				if (range && range->maximumWatts >= range->minimumWatts)
				{
					m_capabilities.minimumPowerWatts   = range->minimumWatts;
					m_capabilities.maximumPowerWatts   = range->maximumWatts;
					m_capabilities.powerIncrementWatts = std::max<int>(1, range->incrementWatts);
				}
			}
			else if (event.characteristic == RESISTANCE_RANGE_CHARACTERISTIC)
			{
				const std::optional<Ftms::ResistanceRange> range = Ftms::ParseResistanceRange(event.value);
				if (range && range->maximum >= range->minimum)
				{
					m_capabilities.minimumResistance   = range->minimum;
					m_capabilities.maximumResistance   = range->maximum;
					m_capabilities.resistanceIncrement = range->increment;
				}
			}
			return;
		}
		case LinkEvent::Type::READY:
		{
			m_capabilities.known     = true;
			m_requestControlAttempts = 0;
			m_controlLostRestarts    = 0;
			m_consecutiveTimeouts    = 0;
			m_reportedUnresponsive   = false;
			DEBUG_INFO("Trainer '%s' connected (ERG %s, simulation %s, resistance %s)", m_name.c_str(),
			           m_capabilities.supportsTargetPower ? "yes" : "no",
			           m_capabilities.supportsSimulation ? "yes" : "no",
			           m_capabilities.supportsResistanceLevel ? "yes" : "no");
			Emit(DeviceEvent::Type::CONNECTED, m_name);
			BeginControlSequence();
			return;
		}
		case LinkEvent::Type::NOTIFICATION:
			HandleNotification(event, now);
			return;
		case LinkEvent::Type::WRITE_COMPLETE:
			if (event.writeId == m_controlPointWriteId && event.status != BleStatus::SUCCESS)
			{
				DEBUG_WARNING("FTMS control point write failed: %s", ToString(event.status));
				m_controlPoint.OnWriteFailed(m_controlPointRequestId);
			}
			return;
		case LinkEvent::Type::DISCONNECTED:
			Emit(DeviceEvent::Type::DISCONNECTED, event.detail);
			break;
		case LinkEvent::Type::RETRY_SCHEDULED:
			Emit(DeviceEvent::Type::RECONNECTING,
			     event.detail + FormatValue(" (retry in %.1f s)", event.retryDelaySeconds));
			break;
		case LinkEvent::Type::FAILED:
			Emit(DeviceEvent::Type::CONNECTION_FAILED, event.detail);
			break;
	}

	// Link went away (DISCONNECTED / RETRY_SCHEDULED / FAILED): every pending procedure is void.
	m_controlPoint.CancelAll();
	m_controlPoint.TakeOutcomes();
	if (m_controlState != ControlState::DENIED)
	{
		m_controlState = ControlState::NONE;
	}
}

void FtmsTrainer::HandleNotification(const LinkEvent& event, double now)
{
	if (event.characteristic == INDOOR_BIKE_DATA_CHARACTERISTIC)
	{
		const std::optional<Ftms::IndoorBikeData> data = Ftms::ParseIndoorBikeData(event.value);
		if (!data)
		{
			DEBUG_WARNING("Malformed Indoor Bike Data notification (%zu bytes)", event.value.size());
			return;
		}
		// A record may span several notifications; merge whatever this one carries.
		if (data->instantaneousSpeedKmh)
		{
			m_sample.speedKmh = data->instantaneousSpeedKmh;
		}
		if (data->instantaneousCadenceRpm)
		{
			m_sample.cadenceRpm = data->instantaneousCadenceRpm;
		}
		if (data->instantaneousPowerWatts)
		{
			m_sample.powerWatts = *data->instantaneousPowerWatts;
		}
		if (data->heartRateBpm)
		{
			m_sample.heartRateBpm = *data->heartRateBpm;
		}
		if (data->resistanceLevel)
		{
			m_sample.resistanceLevel = *data->resistanceLevel;
		}
		if (data->totalDistanceMeters)
		{
			m_sample.distanceMeters = data->totalDistanceMeters;
		}
		if (data->elapsedTimeSeconds)
		{
			m_sample.elapsedSeconds = data->elapsedTimeSeconds;
		}
		m_sample.timestampSeconds = now;
		if (!data->HasMoreData())
		{
			Emit(DeviceEvent::Type::SAMPLE);
		}
		return;
	}

	if (event.characteristic == CONTROL_POINT_CHARACTERISTIC)
	{
		m_controlPoint.OnIndication(event.value);
		return;
	}

	if (event.characteristic == MACHINE_STATUS_CHARACTERISTIC)
	{
		const std::optional<Ftms::MachineStatus> status = Ftms::ParseMachineStatus(event.value);
		if (status && status->opCode == Ftms::MachineStatusOpCode::CONTROL_PERMISSION_LOST &&
		    (m_controlState == ControlState::GRANTED || m_controlState == ControlState::REQUESTING) && !m_stopping)
		{
			HandleControlLost("trainer revoked control");
		}
	}
}

void FtmsTrainer::HandleOutcome(const ControlPointOutcome& outcome)
{
	using Kind = ControlPointOutcome::Kind;
	if (outcome.kind == Kind::SUPERSEDED || outcome.kind == Kind::CANCELLED)
	{
		return;
	}
	if (outcome.kind == Kind::RESPONSE)
	{
		m_consecutiveTimeouts  = 0;
		m_reportedUnresponsive = false;
	}

	switch (outcome.opCode)
	{
		case Ftms::OpCode::REQUEST_CONTROL:
		{
			if (outcome.IsSuccess())
			{
				m_controlState           = ControlState::GRANTED;
				m_requestControlAttempts = 0;
				Emit(DeviceEvent::Type::CONTROL_GRANTED);
				return;
			}
			if (outcome.kind == Kind::RESPONSE)
			{
				m_controlState = ControlState::DENIED;
				m_controlPoint.CancelQueued();
				Emit(DeviceEvent::Type::CONTROL_DENIED, Ftms::ToString(outcome.result));
				return;
			}
			// Timeout or failed write: retry the whole sequence a couple of times, then give up.
			++m_requestControlAttempts;
			if (m_requestControlAttempts <= m_settings.requestControlRetries)
			{
				BeginControlSequence();
				return;
			}
			m_controlState = ControlState::UNRESPONSIVE;
			m_controlPoint.CancelQueued();
			Emit(DeviceEvent::Type::TRAINER_UNRESPONSIVE, "no response to Request Control");
			return;
		}
		case Ftms::OpCode::START_OR_RESUME:
		case Ftms::OpCode::RESET:
		{
			if (outcome.kind == Kind::RESPONSE && outcome.result == Ftms::ResultCode::CONTROL_NOT_PERMITTED &&
			    !m_stopping)
			{
				HandleControlLost(std::string(Ftms::ToString(outcome.opCode)) + " not permitted");
			}
			else if (!outcome.IsSuccess())
			{
				// Non-fatal: some trainers do not implement Start/Resume and still accept targets.
				DEBUG_WARNING("FTMS %s did not succeed (%s)", Ftms::ToString(outcome.opCode),
				              outcome.kind == Kind::RESPONSE ? Ftms::ToString(outcome.result) : "no response");
			}
			return;
		}
		case Ftms::OpCode::SET_TARGET_POWER:
		case Ftms::OpCode::SET_TARGET_RESISTANCE_LEVEL:
		case Ftms::OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS:
		{
			const std::string command = Ftms::ToString(outcome.opCode);
			if (outcome.IsSuccess())
			{
				m_controlLostRestarts = 0;
				Emit(DeviceEvent::Type::COMMAND_SUCCEEDED, command);
				return;
			}
			if (outcome.kind == Kind::RESPONSE)
			{
				if (outcome.result == Ftms::ResultCode::OP_CODE_NOT_SUPPORTED)
				{
					if (outcome.opCode == Ftms::OpCode::SET_TARGET_POWER)
					{
						m_capabilities.supportsTargetPower = false;
					}
					else if (outcome.opCode == Ftms::OpCode::SET_TARGET_RESISTANCE_LEVEL)
					{
						m_capabilities.supportsResistanceLevel = false;
					}
					else
					{
						m_capabilities.supportsSimulation = false;
					}
					Emit(DeviceEvent::Type::COMMAND_UNSUPPORTED, command);
					return;
				}
				if (outcome.result == Ftms::ResultCode::CONTROL_NOT_PERMITTED)
				{
					HandleControlLost(command + " not permitted");
					return;
				}
				Emit(DeviceEvent::Type::COMMAND_FAILED, command + ": " + Ftms::ToString(outcome.result));
				return;
			}
			if (outcome.kind == Kind::TIMEOUT)
			{
				++m_consecutiveTimeouts;
				Emit(DeviceEvent::Type::COMMAND_FAILED,
				     command + FormatValue(": no response within %.1f s", m_settings.controlPointTimeoutSeconds));
				if (m_consecutiveTimeouts >= m_settings.unresponsiveAfterTimeouts && !m_reportedUnresponsive)
				{
					m_reportedUnresponsive = true;
					Emit(DeviceEvent::Type::TRAINER_UNRESPONSIVE,
					     std::to_string(m_consecutiveTimeouts) + " control point commands timed out");
				}
				return;
			}
			Emit(DeviceEvent::Type::COMMAND_FAILED, command + ": write failed");
			return;
		}
		default:
			return;
	}
}

void FtmsTrainer::ApplyFeatures(const Ftms::Features& features)
{
	m_capabilities                         = TrainerCapabilities();
	m_capabilities.supportsTargetPower     = features.HasTargetSetting(Ftms::TargetSettingFeature::POWER);
	m_capabilities.supportsResistanceLevel = features.HasTargetSetting(Ftms::TargetSettingFeature::RESISTANCE);
	m_capabilities.supportsSimulation = features.HasTargetSetting(Ftms::TargetSettingFeature::INDOOR_BIKE_SIMULATION);
	m_capabilities.reportsPower       = features.HasMachineFeature(Ftms::MachineFeature::POWER_MEASUREMENT);
	m_capabilities.reportsCadence     = features.HasMachineFeature(Ftms::MachineFeature::CADENCE);
	m_capabilities.reportsHeartRate   = features.HasMachineFeature(Ftms::MachineFeature::HEART_RATE_MEASUREMENT);
}

void FtmsTrainer::BeginControlSequence()
{
	m_controlPoint.CancelQueued();
	m_controlState = ControlState::REQUESTING;
	m_controlPoint.Enqueue(Ftms::OpCode::REQUEST_CONTROL, Ftms::BuildRequestControl(), false);
	m_controlPoint.Enqueue(Ftms::OpCode::START_OR_RESUME, Ftms::BuildStartOrResume(), false);
	if (m_target.mode != TrainerMode::FREE_RIDE)
	{
		// Re-apply what the game last asked for (after a reconnect, or when control comes back).
		const CommandResult result = ApplyTarget(0.0);
		if (result.status == CommandResult::Status::UNSUPPORTED)
		{
			Emit(DeviceEvent::Type::COMMAND_UNSUPPORTED,
			     std::string("stored ") + ToString(m_target.mode) + " target is not supported by this trainer");
			m_target.mode = TrainerMode::FREE_RIDE;
		}
	}
}

void FtmsTrainer::HandleControlLost(const std::string& reason)
{
	Emit(DeviceEvent::Type::CONTROL_LOST, reason);
	++m_controlLostRestarts;
	if (m_controlLostRestarts > 2)
	{
		m_controlState = ControlState::DENIED;
		m_controlPoint.CancelQueued();
		Emit(DeviceEvent::Type::CONTROL_DENIED, "control repeatedly revoked");
		return;
	}
	BeginControlSequence();
}

void FtmsTrainer::PumpControlPoint(double now)
{
	if (!m_link.IsReady())
	{
		return;
	}
	m_controlPoint.Update(now);
	// Handling an outcome can produce more (a retry cancels what was queued), so drain until nothing is left.
	for (std::vector<ControlPointOutcome> outcomes = m_controlPoint.TakeOutcomes(); !outcomes.empty();
	     outcomes                                  = m_controlPoint.TakeOutcomes())
	{
		for (const ControlPointOutcome& outcome : outcomes)
		{
			HandleOutcome(outcome);
		}
	}
	const std::optional<ControlPointSend> send = m_controlPoint.TakeNextToSend(now);
	if (send)
	{
		m_controlPointRequestId = send->requestId;
		m_controlPointWriteId   = m_link.Write(CONTROL_POINT_CHARACTERISTIC, send->payload, true);
	}
}
}  // namespace Struktur::Fitness
