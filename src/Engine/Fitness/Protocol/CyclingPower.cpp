#include "CyclingPower.h"

#include "Engine/Fitness/Protocol/Bytes.h"

namespace Struktur::Fitness::CyclingPower
{
std::optional<Measurement> ParseMeasurement(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	Measurement measurement;
	if (!reader.ReadU16(measurement.flags) || !reader.ReadS16(measurement.powerWatts))
	{
		return std::nullopt;
	}
	const uint16_t flags = measurement.flags;

	if (flags & MeasurementFlags::PEDAL_POWER_BALANCE_PRESENT)
	{
		uint8_t raw = 0;
		if (!reader.ReadU8(raw))
		{
			return std::nullopt;
		}
		measurement.pedalPowerBalanceRaw = raw;
	}
	if (flags & MeasurementFlags::ACCUMULATED_TORQUE_PRESENT)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		measurement.accumulatedTorqueRaw = raw;
	}
	if (flags & MeasurementFlags::WHEEL_REVOLUTION_PRESENT)
	{
		WheelRevolutions wheel;
		if (!reader.ReadU32(wheel.cumulativeRevolutions) || !reader.ReadU16(wheel.lastEventTime))
		{
			return std::nullopt;
		}
		measurement.wheel = wheel;
	}
	if (flags & MeasurementFlags::CRANK_REVOLUTION_PRESENT)
	{
		CrankRevolutions crank;
		if (!reader.ReadU16(crank.cumulativeRevolutions) || !reader.ReadU16(crank.lastEventTime))
		{
			return std::nullopt;
		}
		measurement.crank = crank;
	}
	if (flags & MeasurementFlags::EXTREME_FORCE_PRESENT)
	{
		int16_t maximum = 0;
		int16_t minimum = 0;
		if (!reader.ReadS16(maximum) || !reader.ReadS16(minimum))
		{
			return std::nullopt;
		}
		measurement.maximumForceNewtons = maximum;
		measurement.minimumForceNewtons = minimum;
	}
	if (flags & MeasurementFlags::EXTREME_TORQUE_PRESENT)
	{
		int16_t maximum = 0;
		int16_t minimum = 0;
		if (!reader.ReadS16(maximum) || !reader.ReadS16(minimum))
		{
			return std::nullopt;
		}
		measurement.maximumTorqueRaw = maximum;
		measurement.minimumTorqueRaw = minimum;
	}
	if (flags & MeasurementFlags::EXTREME_ANGLES_PRESENT)
	{
		// One uint24: maximum angle in the low 12 bits, minimum angle in the high 12 bits.
		uint32_t packed = 0;
		if (!reader.ReadU24(packed))
		{
			return std::nullopt;
		}
		measurement.maximumAngleDegrees = static_cast<uint16_t>(packed & 0x0FFF);
		measurement.minimumAngleDegrees = static_cast<uint16_t>((packed >> 12) & 0x0FFF);
	}
	if (flags & MeasurementFlags::TOP_DEAD_SPOT_PRESENT)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		measurement.topDeadSpotDegrees = raw;
	}
	if (flags & MeasurementFlags::BOTTOM_DEAD_SPOT_PRESENT)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		measurement.bottomDeadSpotDegrees = raw;
	}
	if (flags & MeasurementFlags::ACCUMULATED_ENERGY_PRESENT)
	{
		uint16_t raw = 0;
		if (!reader.ReadU16(raw))
		{
			return std::nullopt;
		}
		measurement.accumulatedEnergyKilojoules = raw;
	}
	return measurement;
}
}  // namespace Struktur::Fitness::CyclingPower
