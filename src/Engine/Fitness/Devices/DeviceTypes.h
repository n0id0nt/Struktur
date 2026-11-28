#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Transport-neutral device vocabulary. Nothing here mentions BLE or FTMS, so an ANT+ FE-C trainer (or ANT+ heart
// rate strap) can implement the same Trainer / HeartRateSensor / PowerMeter interfaces later without game changes.
namespace Struktur::Fitness
{
enum class DeviceRole : uint8_t
{
	TRAINER,
	HEART_RATE,
	POWER_METER,
};

const char* ToString(DeviceRole role);

enum class ConnectionState : uint8_t
{
	DISCONNECTED,  // not started
	CONNECTING,    // connecting, discovering, reading capabilities, subscribing
	CONNECTED,     // ready: data flowing, commands accepted
	RECONNECTING,  // link lost or attempt failed; waiting out the backoff delay
	FAILED,        // reconnect attempts exhausted
	STOPPED,       // stopped by the game; link released
};

const char* ToString(ConnectionState state);

struct DeviceEvent
{
	enum class Type : uint8_t
	{
		CONNECTED,
		DISCONNECTED,       // an established link dropped (reconnect follows automatically)
		RECONNECTING,       // a retry is scheduled; detail says why
		CONNECTION_FAILED,  // retries exhausted
		SAMPLE,             // a new latest-sample snapshot is available
		CONTROL_GRANTED,
		CONTROL_DENIED,        // another app holds control, or the trainer refused
		CONTROL_LOST,          // trainer revoked control (re-request is automatic)
		TRAINER_UNRESPONSIVE,  // control point stopped answering
		COMMAND_SUCCEEDED,
		COMMAND_FAILED,
		COMMAND_UNSUPPORTED,  // trainer rejected the op code; capability is now treated as absent
		TARGET_CLAMPED,       // requested target adjusted to the trainer's range or the safety cap
	};

	DeviceRole role = DeviceRole::TRAINER;
	Type type       = Type::SAMPLE;
	std::string detail;
};

const char* ToString(DeviceEvent::Type type);

// ---------------------------------------------------------------------------------------------------------------
// Samples - the latest-value snapshots each device exposes. Fields a device has never reported stay nullopt.
// ---------------------------------------------------------------------------------------------------------------

struct TrainerSample
{
	double timestampSeconds = 0.0;  // game-clock time of the last update
	std::optional<int> powerWatts;
	std::optional<double> cadenceRpm;
	std::optional<double> speedKmh;
	std::optional<int> heartRateBpm;
	std::optional<int> resistanceLevel;
	std::optional<uint32_t> distanceMeters;
	std::optional<uint16_t> elapsedSeconds;
};

enum class HeartRateContact : uint8_t
{
	NOT_SUPPORTED,
	NOT_DETECTED,
	DETECTED,
};

struct HeartRateSample
{
	double timestampSeconds = 0.0;
	std::optional<int> bpm;
	HeartRateContact contact = HeartRateContact::NOT_SUPPORTED;
	std::vector<double> rrIntervalsSeconds;  // intervals carried by the LAST notification only
};

struct PowerSample
{
	double timestampSeconds = 0.0;
	std::optional<int> powerWatts;
	std::optional<double> cadenceRpm;  // derived from crank revolutions when the meter reports them
	std::optional<double> pedalBalancePercent;
};

// ---------------------------------------------------------------------------------------------------------------
// Trainer control
// ---------------------------------------------------------------------------------------------------------------

struct TrainerCapabilities
{
	bool known                   = false;  // false until the trainer's feature set has been read
	bool supportsTargetPower     = false;
	bool supportsResistanceLevel = false;
	bool supportsSimulation      = false;
	bool reportsPower            = false;
	bool reportsCadence          = false;
	bool reportsHeartRate        = false;
	std::optional<int> minimumPowerWatts;
	std::optional<int> maximumPowerWatts;
	int powerIncrementWatts = 1;
	std::optional<double> minimumResistance;
	std::optional<double> maximumResistance;
	std::optional<double> resistanceIncrement;
	std::optional<double> minimumGradePercent;  // Supported Inclination Range, when the trainer reports one
	std::optional<double> maximumGradePercent;
};

enum class ControlState : uint8_t
{
	NONE,
	REQUESTING,
	GRANTED,
	DENIED,
	UNRESPONSIVE,
};

const char* ToString(ControlState state);

enum class TrainerMode : uint8_t
{
	FREE_RIDE,   // no target
	ERG,         // target power
	SIMULATION,  // grade / wind / rolling resistance
	RESISTANCE,  // raw resistance level
	MANUAL,      // 0..1 "gear" - simulation grade when available, else resistance level
};

const char* ToString(TrainerMode mode);

struct SimulationTarget
{
	double gradePercent                 = 0.0;
	double windSpeedMetersPerSecond     = 0.0;
	double rollingResistanceCoefficient = 0.004;
	double windResistanceCoefficient    = 0.51;  // kg/m
};

struct CommandResult
{
	enum class Status : uint8_t
	{
		QUEUED,          // sent to the trainer's command queue as requested
		CLAMPED,         // queued, but adjusted to the trainer's range or the safety cap
		DEFERRED,        // not connected yet - remembered and applied once connected
		UNSUPPORTED,     // trainer does not support this kind of target
		CONTROL_DENIED,  // trainer refused control; nothing can be sent
	};

	Status status         = Status::QUEUED;
	double requestedValue = 0.0;
	double appliedValue   = 0.0;

	bool IsAccepted() const
	{
		return status == Status::QUEUED || status == Status::CLAMPED || status == Status::DEFERRED;
	}
};

const char* ToString(CommandResult::Status status);

// ---------------------------------------------------------------------------------------------------------------
// Device interfaces
// ---------------------------------------------------------------------------------------------------------------

// Every device is driven from the game thread: Start(), then Update(now) once per frame. `now` is any monotonic
// clock in seconds (the game passes its unscaled time), which keeps every timeout testable.
class Device
{
public:
	virtual ~Device() = default;

	virtual DeviceRole GetRole() const                 = 0;
	virtual std::string GetIdentifier() const          = 0;  // transport address, e.g. "AA:BB:CC:DD:EE:FF"
	virtual const std::string& GetName() const         = 0;
	virtual ConnectionState GetConnectionState() const = 0;

	virtual void Start(double now)  = 0;
	virtual void Update(double now) = 0;
	// Begins a graceful stop (a trainer first releases its target). Keep calling Update until IsStopped().
	virtual void Stop(double now)  = 0;
	virtual bool IsStopped() const = 0;

	// Events raised since the last call, oldest first.
	std::vector<DeviceEvent> TakeEvents()
	{
		return std::exchange(m_events, {});
	}

protected:
	void Emit(DeviceEvent::Type type, std::string detail = {})
	{
		m_events.push_back(DeviceEvent{GetRole(), type, std::move(detail)});
	}

private:
	std::vector<DeviceEvent> m_events;
};

class Trainer : public Device
{
public:
	DeviceRole GetRole() const override
	{
		return DeviceRole::TRAINER;
	}

	virtual const TrainerSample& GetLatestSample() const       = 0;
	virtual const TrainerCapabilities& GetCapabilities() const = 0;
	virtual ControlState GetControlState() const               = 0;
	virtual TrainerMode GetMode() const                        = 0;

	virtual CommandResult SetTargetPower(int watts)                     = 0;
	virtual CommandResult SetSimulation(const SimulationTarget& target) = 0;
	virtual CommandResult SetResistanceLevel(double level)              = 0;
	// 0 = easiest, 1 = hardest. Uses simulation grade when supported (never assumes resistance control exists).
	virtual CommandResult SetManualResistance(double fraction) = 0;
	virtual CommandResult ClearTarget()                        = 0;
};

class HeartRateSensor : public Device
{
public:
	DeviceRole GetRole() const override
	{
		return DeviceRole::HEART_RATE;
	}

	virtual const HeartRateSample& GetLatestSample() const = 0;
};

class PowerMeter : public Device
{
public:
	DeviceRole GetRole() const override
	{
		return DeviceRole::POWER_METER;
	}

	virtual const PowerSample& GetLatestSample() const = 0;
};
}  // namespace Struktur::Fitness
