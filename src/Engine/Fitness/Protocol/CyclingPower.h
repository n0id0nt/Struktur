#pragma once

#include <cstdint>
#include <optional>
#include <span>

// Cycling Power Service (0x1818) - Cycling Power Measurement (0x2A63) parser. Pure, no BLE.
namespace Struktur::Fitness::CyclingPower
{
namespace MeasurementFlags
{
constexpr uint16_t PEDAL_POWER_BALANCE_PRESENT   = 1 << 0;
constexpr uint16_t PEDAL_POWER_BALANCE_REFERENCE = 1 << 1;  // 1 = balance is the LEFT pedal's share
constexpr uint16_t ACCUMULATED_TORQUE_PRESENT    = 1 << 2;
constexpr uint16_t ACCUMULATED_TORQUE_SOURCE     = 1 << 3;  // 0 = wheel based, 1 = crank based
constexpr uint16_t WHEEL_REVOLUTION_PRESENT      = 1 << 4;
constexpr uint16_t CRANK_REVOLUTION_PRESENT      = 1 << 5;
constexpr uint16_t EXTREME_FORCE_PRESENT         = 1 << 6;
constexpr uint16_t EXTREME_TORQUE_PRESENT        = 1 << 7;
constexpr uint16_t EXTREME_ANGLES_PRESENT        = 1 << 8;
constexpr uint16_t TOP_DEAD_SPOT_PRESENT         = 1 << 9;
constexpr uint16_t BOTTOM_DEAD_SPOT_PRESENT      = 1 << 10;
constexpr uint16_t ACCUMULATED_ENERGY_PRESENT    = 1 << 11;
constexpr uint16_t OFFSET_COMPENSATION_INDICATOR = 1 << 12;
}  // namespace MeasurementFlags

struct WheelRevolutions
{
	uint32_t cumulativeRevolutions = 0;
	uint16_t lastEventTime         = 0;  // 1/2048 s, rolls over
};

struct CrankRevolutions
{
	uint16_t cumulativeRevolutions = 0;  // rolls over
	uint16_t lastEventTime         = 0;  // 1/1024 s, rolls over
};

// Every optional field, in the fixed order the spec lays them out after the mandatory power value.
struct Measurement
{
	uint16_t flags     = 0;
	int16_t powerWatts = 0;
	std::optional<uint8_t> pedalPowerBalanceRaw;   // 1/2 %
	std::optional<uint16_t> accumulatedTorqueRaw;  // 1/32 Nm
	std::optional<WheelRevolutions> wheel;
	std::optional<CrankRevolutions> crank;
	std::optional<int16_t> maximumForceNewtons;
	std::optional<int16_t> minimumForceNewtons;
	std::optional<int16_t> maximumTorqueRaw;      // 1/32 Nm
	std::optional<int16_t> minimumTorqueRaw;      // 1/32 Nm
	std::optional<uint16_t> maximumAngleDegrees;  // 12 bits
	std::optional<uint16_t> minimumAngleDegrees;  // 12 bits
	std::optional<uint16_t> topDeadSpotDegrees;
	std::optional<uint16_t> bottomDeadSpotDegrees;
	std::optional<uint16_t> accumulatedEnergyKilojoules;

	std::optional<double> GetPedalPowerBalancePercent() const
	{
		return pedalPowerBalanceRaw ? std::optional<double>(*pedalPowerBalanceRaw * 0.5) : std::nullopt;
	}
};

std::optional<Measurement> ParseMeasurement(std::span<const uint8_t> data);
}  // namespace Struktur::Fitness::CyclingPower
