#include "HeartRate.h"

#include "Engine/Fitness/Protocol/Bytes.h"

namespace Struktur::Fitness::HeartRate
{
std::optional<Measurement> ParseMeasurement(std::span<const uint8_t> data)
{
	ByteReader reader(data);
	uint8_t flags = 0;
	if (!reader.ReadU8(flags))
	{
		return std::nullopt;
	}

	Measurement measurement;
	if (flags & MeasurementFlags::VALUE_FORMAT_UINT16)
	{
		if (!reader.ReadU16(measurement.bpm))
		{
			return std::nullopt;
		}
	}
	else
	{
		uint8_t bpm = 0;
		if (!reader.ReadU8(bpm))
		{
			return std::nullopt;
		}
		measurement.bpm = bpm;
	}

	// Bit 1 (detected) is only meaningful when bit 2 (supported) is set.
	if (flags & MeasurementFlags::SENSOR_CONTACT_SUPPORTED)
	{
		measurement.contact =
		    (flags & MeasurementFlags::SENSOR_CONTACT_DETECTED) ? SensorContact::DETECTED : SensorContact::NOT_DETECTED;
	}

	if (flags & MeasurementFlags::ENERGY_EXPENDED_PRESENT)
	{
		uint16_t energy = 0;
		if (!reader.ReadU16(energy))
		{
			return std::nullopt;
		}
		measurement.energyExpendedKilojoules = energy;
	}

	if (flags & MeasurementFlags::RR_INTERVAL_PRESENT)
	{
		// RR intervals fill the rest of the packet. Lenient on purpose: a stray trailing byte (or a flag with no
		// intervals) drops only the RR data, never the heart rate itself.
		while (reader.Remaining() >= 2)
		{
			uint16_t interval = 0;
			reader.ReadU16(interval);
			measurement.rrIntervals.push_back(interval);
		}
	}
	return measurement;
}
}  // namespace Struktur::Fitness::HeartRate
