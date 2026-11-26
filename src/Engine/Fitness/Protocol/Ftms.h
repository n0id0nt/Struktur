#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "Engine/Fitness/Protocol/Bytes.h"

// Fitness Machine Service (FTMS v1.0, Bluetooth SIG) - pure parsers and command builders. Nothing in here touches
// BLE; every function maps bytes <-> structs so it can be unit-tested against golden byte arrays.
namespace Struktur::Fitness::Ftms
{
// ---------------------------------------------------------------------------------------------------------------
// Indoor Bike Data (0x2AD2)
// ---------------------------------------------------------------------------------------------------------------

// Flag bits of the uint16 flags field. Note bit 0 is inverted: MORE_DATA == 0 means Instantaneous Speed IS present.
namespace IndoorBikeDataFlags
{
constexpr uint16_t MORE_DATA             = 1 << 0;
constexpr uint16_t AVERAGE_SPEED         = 1 << 1;
constexpr uint16_t INSTANTANEOUS_CADENCE = 1 << 2;
constexpr uint16_t AVERAGE_CADENCE       = 1 << 3;
constexpr uint16_t TOTAL_DISTANCE        = 1 << 4;
constexpr uint16_t RESISTANCE_LEVEL      = 1 << 5;
constexpr uint16_t INSTANTANEOUS_POWER   = 1 << 6;
constexpr uint16_t AVERAGE_POWER         = 1 << 7;
constexpr uint16_t EXPENDED_ENERGY       = 1 << 8;
constexpr uint16_t HEART_RATE            = 1 << 9;
constexpr uint16_t METABOLIC_EQUIVALENT  = 1 << 10;
constexpr uint16_t ELAPSED_TIME          = 1 << 11;
constexpr uint16_t REMAINING_TIME        = 1 << 12;
}  // namespace IndoorBikeDataFlags

// One Indoor Bike Data notification. A machine may split one logical record over several notifications (all but
// the last carry MORE_DATA), so every field is optional - consumers merge whatever each packet carries.
struct IndoorBikeData
{
	uint16_t flags = 0;
	std::optional<double> instantaneousSpeedKmh;    // uint16, 0.01 km/h
	std::optional<double> averageSpeedKmh;          // uint16, 0.01 km/h
	std::optional<double> instantaneousCadenceRpm;  // uint16, 0.5 rpm
	std::optional<double> averageCadenceRpm;        // uint16, 0.5 rpm
	std::optional<uint32_t> totalDistanceMeters;    // uint24, 1 m
	std::optional<int16_t> resistanceLevel;         // sint16, unitless, raw as sent
	std::optional<int16_t> instantaneousPowerWatts;
	std::optional<int16_t> averagePowerWatts;
	std::optional<uint16_t> totalEnergyKcal;     // 0xFFFF = not available
	std::optional<uint16_t> energyPerHourKcal;   // 0xFFFF = not available
	std::optional<uint8_t> energyPerMinuteKcal;  // 0xFF = not available
	std::optional<uint8_t> heartRateBpm;
	std::optional<double> metabolicEquivalent;  // uint8, 0.1
	std::optional<uint16_t> elapsedTimeSeconds;
	std::optional<uint16_t> remainingTimeSeconds;

	bool HasMoreData() const
	{
		return (flags & IndoorBikeDataFlags::MORE_DATA) != 0;
	}
};

// nullopt when the payload is shorter than its own flags say it should be.
std::optional<IndoorBikeData> ParseIndoorBikeData(std::span<const uint8_t> data);

// ---------------------------------------------------------------------------------------------------------------
// Fitness Machine Feature (0x2ACC)
// ---------------------------------------------------------------------------------------------------------------

namespace MachineFeature
{
constexpr uint32_t AVERAGE_SPEED           = 1u << 0;
constexpr uint32_t CADENCE                 = 1u << 1;
constexpr uint32_t TOTAL_DISTANCE          = 1u << 2;
constexpr uint32_t INCLINATION             = 1u << 3;
constexpr uint32_t ELEVATION_GAIN          = 1u << 4;
constexpr uint32_t PACE                    = 1u << 5;
constexpr uint32_t STEP_COUNT              = 1u << 6;
constexpr uint32_t RESISTANCE_LEVEL        = 1u << 7;
constexpr uint32_t STRIDE_COUNT            = 1u << 8;
constexpr uint32_t EXPENDED_ENERGY         = 1u << 9;
constexpr uint32_t HEART_RATE_MEASUREMENT  = 1u << 10;
constexpr uint32_t METABOLIC_EQUIVALENT    = 1u << 11;
constexpr uint32_t ELAPSED_TIME            = 1u << 12;
constexpr uint32_t REMAINING_TIME          = 1u << 13;
constexpr uint32_t POWER_MEASUREMENT       = 1u << 14;
constexpr uint32_t FORCE_ON_BELT_AND_POWER = 1u << 15;
constexpr uint32_t USER_DATA_RETENTION     = 1u << 16;
}  // namespace MachineFeature

namespace TargetSettingFeature
{
constexpr uint32_t SPEED                  = 1u << 0;
constexpr uint32_t INCLINATION            = 1u << 1;
constexpr uint32_t RESISTANCE             = 1u << 2;
constexpr uint32_t POWER                  = 1u << 3;
constexpr uint32_t HEART_RATE             = 1u << 4;
constexpr uint32_t EXPENDED_ENERGY        = 1u << 5;
constexpr uint32_t STEP_NUMBER            = 1u << 6;
constexpr uint32_t STRIDE_NUMBER          = 1u << 7;
constexpr uint32_t DISTANCE               = 1u << 8;
constexpr uint32_t TRAINING_TIME          = 1u << 9;
constexpr uint32_t TIME_IN_TWO_HR_ZONES   = 1u << 10;
constexpr uint32_t TIME_IN_THREE_HR_ZONES = 1u << 11;
constexpr uint32_t TIME_IN_FIVE_HR_ZONES  = 1u << 12;
constexpr uint32_t INDOOR_BIKE_SIMULATION = 1u << 13;
constexpr uint32_t WHEEL_CIRCUMFERENCE    = 1u << 14;
constexpr uint32_t SPIN_DOWN_CONTROL      = 1u << 15;
constexpr uint32_t CADENCE                = 1u << 16;
}  // namespace TargetSettingFeature

struct Features
{
	uint32_t machineFeatures       = 0;
	uint32_t targetSettingFeatures = 0;

	bool HasMachineFeature(uint32_t bit) const
	{
		return (machineFeatures & bit) != 0;
	}
	bool HasTargetSetting(uint32_t bit) const
	{
		return (targetSettingFeatures & bit) != 0;
	}
};

std::optional<Features> ParseFeatures(std::span<const uint8_t> data);

// ---------------------------------------------------------------------------------------------------------------
// Supported Power Range (0x2AD8) / Supported Resistance Level Range (0x2AD6)
// ---------------------------------------------------------------------------------------------------------------

struct PowerRange
{
	int16_t minimumWatts    = 0;
	int16_t maximumWatts    = 0;
	uint16_t incrementWatts = 0;
};

std::optional<PowerRange> ParsePowerRange(std::span<const uint8_t> data);

// Resistance values travel as sint16/uint16 with 0.1 resolution; stored here already scaled.
struct ResistanceRange
{
	double minimum   = 0.0;
	double maximum   = 0.0;
	double increment = 0.0;
};

std::optional<ResistanceRange> ParseResistanceRange(std::span<const uint8_t> data);

// ---------------------------------------------------------------------------------------------------------------
// Fitness Machine Control Point (0x2AD9)
// ---------------------------------------------------------------------------------------------------------------

enum class OpCode : uint8_t
{
	REQUEST_CONTROL                       = 0x00,
	RESET                                 = 0x01,
	SET_TARGET_SPEED                      = 0x02,
	SET_TARGET_INCLINATION                = 0x03,
	SET_TARGET_RESISTANCE_LEVEL           = 0x04,
	SET_TARGET_POWER                      = 0x05,
	SET_TARGET_HEART_RATE                 = 0x06,
	START_OR_RESUME                       = 0x07,
	STOP_OR_PAUSE                         = 0x08,
	SET_INDOOR_BIKE_SIMULATION_PARAMETERS = 0x11,
	RESPONSE_CODE                         = 0x80,
};

enum class ResultCode : uint8_t
{
	SUCCESS               = 0x01,
	OP_CODE_NOT_SUPPORTED = 0x02,
	INVALID_PARAMETER     = 0x03,
	OPERATION_FAILED      = 0x04,
	CONTROL_NOT_PERMITTED = 0x05,
};

const char* ToString(OpCode opCode);
const char* ToString(ResultCode resultCode);

// Inputs in real units; the builder converts to the wire resolutions (wind 0.001 m/s, grade 0.01 %, Crr 0.0001,
// Cw 0.01 kg/m) and clamps each to its field's range. Defaults are typical road-bike values.
struct SimulationParameters
{
	double windSpeedMetersPerSecond     = 0.0;
	double gradePercent                 = 0.0;
	double rollingResistanceCoefficient = 0.004;
	double windResistanceCoefficient    = 0.51;
};

Bytes BuildRequestControl();
Bytes BuildReset();
Bytes BuildStartOrResume();
Bytes BuildStopOrPause(bool pause);
Bytes BuildSetTargetPower(int16_t watts);
// FTMS v1.0 defines this parameter as uint8 with 0.1 resolution, so the level clamps to 0.0 - 25.5.
Bytes BuildSetTargetResistanceLevel(double level);
Bytes BuildSetIndoorBikeSimulation(const SimulationParameters& parameters);

// An indication on the control point: 0x80, request op code, result code, then optional response parameters.
struct ControlPointResponse
{
	uint8_t requestOpCode = 0;
	ResultCode result     = ResultCode::OPERATION_FAILED;
	Bytes parameters;
};

// nullopt for anything that is not a well-formed Response Code indication.
std::optional<ControlPointResponse> ParseControlPointResponse(std::span<const uint8_t> data);

// ---------------------------------------------------------------------------------------------------------------
// Fitness Machine Status (0x2ADA) - only what the trainer layer reacts to.
// ---------------------------------------------------------------------------------------------------------------

namespace MachineStatusOpCode
{
constexpr uint8_t RESET                         = 0x01;
constexpr uint8_t STOPPED_OR_PAUSED_BY_USER     = 0x02;
constexpr uint8_t STOPPED_BY_SAFETY_KEY         = 0x03;
constexpr uint8_t STARTED_OR_RESUMED_BY_USER    = 0x04;
constexpr uint8_t TARGET_RESISTANCE_CHANGED     = 0x07;
constexpr uint8_t TARGET_POWER_CHANGED          = 0x08;
constexpr uint8_t SIMULATION_PARAMETERS_CHANGED = 0x12;
constexpr uint8_t CONTROL_PERMISSION_LOST       = 0xFF;
}  // namespace MachineStatusOpCode

struct MachineStatus
{
	uint8_t opCode = 0;
	Bytes parameters;
};

std::optional<MachineStatus> ParseMachineStatus(std::span<const uint8_t> data);
}  // namespace Struktur::Fitness::Ftms
