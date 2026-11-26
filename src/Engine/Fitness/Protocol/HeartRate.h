#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

// Heart Rate Service (0x180D) - Heart Rate Measurement (0x2A37) parser. Pure, no BLE.
namespace Struktur::Fitness::HeartRate
{
namespace MeasurementFlags
{
constexpr uint8_t VALUE_FORMAT_UINT16      = 1 << 0;
constexpr uint8_t SENSOR_CONTACT_DETECTED  = 1 << 1;
constexpr uint8_t SENSOR_CONTACT_SUPPORTED = 1 << 2;
constexpr uint8_t ENERGY_EXPENDED_PRESENT  = 1 << 3;
constexpr uint8_t RR_INTERVAL_PRESENT      = 1 << 4;
}  // namespace MeasurementFlags

enum class SensorContact : uint8_t
{
	NOT_SUPPORTED,
	NOT_DETECTED,
	DETECTED,
};

struct Measurement
{
	uint16_t bpm          = 0;
	SensorContact contact = SensorContact::NOT_SUPPORTED;
	std::optional<uint16_t> energyExpendedKilojoules;
	// RR intervals in the wire unit of 1/1024 s, oldest first. Use RrIntervalToSeconds() to convert.
	std::vector<uint16_t> rrIntervals;
};

constexpr double RrIntervalToSeconds(uint16_t rrInterval)
{
	return rrInterval / 1024.0;
}

std::optional<Measurement> ParseMeasurement(std::span<const uint8_t> data);
}  // namespace Struktur::Fitness::HeartRate
