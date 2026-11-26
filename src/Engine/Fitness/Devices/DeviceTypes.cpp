#include "DeviceTypes.h"

namespace Struktur::Fitness
{
const char* ToString(DeviceRole role)
{
	switch (role)
	{
		case DeviceRole::TRAINER:
			return "trainer";
		case DeviceRole::HEART_RATE:
			return "heartRate";
		case DeviceRole::POWER_METER:
			return "powerMeter";
	}
	return "unknown";
}

const char* ToString(ConnectionState state)
{
	switch (state)
	{
		case ConnectionState::DISCONNECTED:
			return "disconnected";
		case ConnectionState::CONNECTING:
			return "connecting";
		case ConnectionState::CONNECTED:
			return "connected";
		case ConnectionState::RECONNECTING:
			return "reconnecting";
		case ConnectionState::FAILED:
			return "failed";
		case ConnectionState::STOPPED:
			return "stopped";
	}
	return "unknown";
}

const char* ToString(DeviceEvent::Type type)
{
	switch (type)
	{
		case DeviceEvent::Type::CONNECTED:
			return "connected";
		case DeviceEvent::Type::DISCONNECTED:
			return "disconnected";
		case DeviceEvent::Type::RECONNECTING:
			return "reconnecting";
		case DeviceEvent::Type::CONNECTION_FAILED:
			return "connectionFailed";
		case DeviceEvent::Type::SAMPLE:
			return "sample";
		case DeviceEvent::Type::CONTROL_GRANTED:
			return "controlGranted";
		case DeviceEvent::Type::CONTROL_DENIED:
			return "controlDenied";
		case DeviceEvent::Type::CONTROL_LOST:
			return "controlLost";
		case DeviceEvent::Type::TRAINER_UNRESPONSIVE:
			return "trainerUnresponsive";
		case DeviceEvent::Type::COMMAND_SUCCEEDED:
			return "commandSucceeded";
		case DeviceEvent::Type::COMMAND_FAILED:
			return "commandFailed";
		case DeviceEvent::Type::COMMAND_UNSUPPORTED:
			return "commandUnsupported";
		case DeviceEvent::Type::TARGET_CLAMPED:
			return "targetClamped";
	}
	return "unknown";
}

const char* ToString(ControlState state)
{
	switch (state)
	{
		case ControlState::NONE:
			return "none";
		case ControlState::REQUESTING:
			return "requesting";
		case ControlState::GRANTED:
			return "granted";
		case ControlState::DENIED:
			return "denied";
		case ControlState::UNRESPONSIVE:
			return "unresponsive";
	}
	return "unknown";
}

const char* ToString(TrainerMode mode)
{
	switch (mode)
	{
		case TrainerMode::FREE_RIDE:
			return "freeRide";
		case TrainerMode::ERG:
			return "erg";
		case TrainerMode::SIMULATION:
			return "simulation";
		case TrainerMode::RESISTANCE:
			return "resistance";
		case TrainerMode::MANUAL:
			return "manual";
	}
	return "unknown";
}

const char* ToString(CommandResult::Status status)
{
	switch (status)
	{
		case CommandResult::Status::QUEUED:
			return "queued";
		case CommandResult::Status::CLAMPED:
			return "clamped";
		case CommandResult::Status::DEFERRED:
			return "deferred";
		case CommandResult::Status::UNSUPPORTED:
			return "unsupported";
		case CommandResult::Status::CONTROL_DENIED:
			return "controlDenied";
	}
	return "unknown";
}
}  // namespace Struktur::Fitness
