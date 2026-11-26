#include "Ftms.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Struktur::Fitness::Ftms
{
// Rounds a real value onto an integer wire field, clamping to that field's range instead of wrapping.
template <typename Integer>
static Integer ToWire(double value, double resolution)
{
	const double scaled = std::round(value / resolution);
	const double low    = static_cast<double>(std::numeric_limits<Integer>::min());
	const double high   = static_cast<double>(std::numeric_limits<Integer>::max());
	return static_cast<Integer>(std::clamp(scaled, low, high));
}

std::optional<IndoorBikeData> ParseIndoorBikeData(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	IndoorBikeData result;
	if (!reader.ReadU16(result.flags))
	{
		return std::nullopt;
	}
	const uint16_t flags = result.flags;

	// Every field below appears in this fixed order, and only when flagged.
	if ((flags & IndoorBikeDataFlags::MORE_DATA) == 0)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		result.instantaneousSpeedKmh = raw * 0.01;
	}
	if (flags & IndoorBikeDataFlags::AVERAGE_SPEED)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		result.averageSpeedKmh = raw * 0.01;
	}
	if (flags & IndoorBikeDataFlags::INSTANTANEOUS_CADENCE)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		result.instantaneousCadenceRpm = raw * 0.5;
	}
	if (flags & IndoorBikeDataFlags::AVERAGE_CADENCE)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		result.averageCadenceRpm = raw * 0.5;
	}
	if (flags & IndoorBikeDataFlags::TOTAL_DISTANCE)
	{
		uint32_t raw = 0;
		if (!reader.ReadU24(raw))
		{
			return std::nullopt;
		}
		result.totalDistanceMeters = raw;
	}
	if (flags & IndoorBikeDataFlags::RESISTANCE_LEVEL)
	{
		int16_t raw = 0;
		if (!reader.ReadS16(raw))
		{
			return std::nullopt;
		}
		result.resistanceLevel = raw;
	}
	if (flags & IndoorBikeDataFlags::INSTANTANEOUS_POWER)
	{
		int16_t raw = 0;
		if (!reader.ReadS16(raw))
		{
			return std::nullopt;
		}
		result.instantaneousPowerWatts = raw;
	}
	if (flags & IndoorBikeDataFlags::AVERAGE_POWER)
	{
		int16_t raw = 0;
		if (!reader.ReadS16(raw))
		{
			return std::nullopt;
		}
		result.averagePowerWatts = raw;
	}
	if (flags & IndoorBikeDataFlags::EXPENDED_ENERGY)
	{
		uint16_t total    = 0;
		uint16_t perHour  = 0;
		uint8_t perMinute = 0;
		if (!reader.ReadU16(total) || !reader.ReadU16(perHour) || !reader.ReadU8(perMinute))
		{
			return std::nullopt;
		}
		result.totalEnergyKcal     = total;
		result.energyPerHourKcal   = perHour;
		result.energyPerMinuteKcal = perMinute;
	}
	if (flags & IndoorBikeDataFlags::HEART_RATE)
	{
		uint8_t raw = 0;
		if (!reader.ReadU8(raw))
		{
			return std::nullopt;
		}
		result.heartRateBpm = raw;
	}
	if (flags & IndoorBikeDataFlags::METABOLIC_EQUIVALENT)
	{
		uint8_t raw = 0;
		if (!reader.ReadU8(raw))
		{
			return std::nullopt;
		}
		result.metabolicEquivalent = raw * 0.1;
	}
	if (flags & IndoorBikeDataFlags::ELAPSED_TIME)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		result.elapsedTimeSeconds = raw;
	}
	if (flags & IndoorBikeDataFlags::REMAINING_TIME)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		result.remainingTimeSeconds = raw;
	}
	return result;
}

std::optional<Features> ParseFeatures(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	Features features;
	if (!reader.ReadU32(features.machineFeatures) || !reader.ReadU32(features.targetSettingFeatures))
	{
		return std::nullopt;
	}
	return features;
}

std::optional<PowerRange> ParsePowerRange(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	PowerRange range;
	if (!reader.ReadS16(range.minimumWatts) || !reader.ReadS16(range.maximumWatts) ||
	    !reader.ReadU16(range.incrementWatts))
	{
		return std::nullopt;
	}
	return range;
}

std::optional<ResistanceRange> ParseResistanceRange(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	int16_t minimum    = 0;
	int16_t maximum    = 0;
	uint16_t increment = 0;
	if (!reader.ReadS16(minimum) || !reader.ReadS16(maximum) || !reader.ReadU16(increment))
	{
		return std::nullopt;
	}
	return ResistanceRange{minimum * 0.1, maximum * 0.1, increment * 0.1};
}

const char* ToString(OpCode opCode)
{
	switch (opCode)
	{
		case OpCode::REQUEST_CONTROL:
			return "Request Control";
		case OpCode::RESET:
			return "Reset";
		case OpCode::SET_TARGET_SPEED:
			return "Set Target Speed";
		case OpCode::SET_TARGET_INCLINATION:
			return "Set Target Inclination";
		case OpCode::SET_TARGET_RESISTANCE_LEVEL:
			return "Set Target Resistance Level";
		case OpCode::SET_TARGET_POWER:
			return "Set Target Power";
		case OpCode::SET_TARGET_HEART_RATE:
			return "Set Target Heart Rate";
		case OpCode::START_OR_RESUME:
			return "Start or Resume";
		case OpCode::STOP_OR_PAUSE:
			return "Stop or Pause";
		case OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS:
			return "Set Indoor Bike Simulation Parameters";
		case OpCode::RESPONSE_CODE:
			return "Response Code";
	}
	return "Unknown Op Code";
}

const char* ToString(ResultCode resultCode)
{
	switch (resultCode)
	{
		case ResultCode::SUCCESS:
			return "Success";
		case ResultCode::OP_CODE_NOT_SUPPORTED:
			return "Op Code Not Supported";
		case ResultCode::INVALID_PARAMETER:
			return "Invalid Parameter";
		case ResultCode::OPERATION_FAILED:
			return "Operation Failed";
		case ResultCode::CONTROL_NOT_PERMITTED:
			return "Control Not Permitted";
	}
	return "Unknown Result Code";
}

Bytes BuildRequestControl()
{
	return ByteWriter().U8(static_cast<uint8_t>(OpCode::REQUEST_CONTROL)).Take();
}

Bytes BuildReset()
{
	return ByteWriter().U8(static_cast<uint8_t>(OpCode::RESET)).Take();
}

Bytes BuildStartOrResume()
{
	return ByteWriter().U8(static_cast<uint8_t>(OpCode::START_OR_RESUME)).Take();
}

Bytes BuildStopOrPause(bool pause)
{
	// Parameter: 0x01 = Stop, 0x02 = Pause.
	return ByteWriter().U8(static_cast<uint8_t>(OpCode::STOP_OR_PAUSE)).U8(pause ? 0x02 : 0x01).Take();
}

Bytes BuildSetTargetPower(int16_t watts)
{
	return ByteWriter().U8(static_cast<uint8_t>(OpCode::SET_TARGET_POWER)).S16(watts).Take();
}

Bytes BuildSetTargetResistanceLevel(double level)
{
	return ByteWriter()
	    .U8(static_cast<uint8_t>(OpCode::SET_TARGET_RESISTANCE_LEVEL))
	    .U8(ToWire<uint8_t>(level, 0.1))
	    .Take();
}

Bytes BuildSetIndoorBikeSimulation(const SimulationParameters& parameters)
{
	return ByteWriter()
	    .U8(static_cast<uint8_t>(OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS))
	    .S16(ToWire<int16_t>(parameters.windSpeedMetersPerSecond, 0.001))
	    .S16(ToWire<int16_t>(parameters.gradePercent, 0.01))
	    .U8(ToWire<uint8_t>(parameters.rollingResistanceCoefficient, 0.0001))
	    .U8(ToWire<uint8_t>(parameters.windResistanceCoefficient, 0.01))
	    .Take();
}

std::optional<ControlPointResponse> ParseControlPointResponse(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	uint8_t responseOpCode = 0;
	ControlPointResponse response;
	uint8_t result = 0;
	if (!reader.ReadU8(responseOpCode) || responseOpCode != static_cast<uint8_t>(OpCode::RESPONSE_CODE) ||
	    !reader.ReadU8(response.requestOpCode) || !reader.ReadU8(result))
	{
		return std::nullopt;
	}
	response.result                     = static_cast<ResultCode>(result);
	const std::span<const uint8_t> rest = reader.Rest();
	response.parameters.assign(rest.begin(), rest.end());
	return response;
}

std::optional<MachineStatus> ParseMachineStatus(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	MachineStatus status;
	if (!reader.ReadU8(status.opCode))
	{
		return std::nullopt;
	}
	const std::span<const uint8_t> rest = reader.Rest();
	status.parameters.assign(rest.begin(), rest.end());
	return status;
}
}  // namespace Struktur::Fitness::Ftms
