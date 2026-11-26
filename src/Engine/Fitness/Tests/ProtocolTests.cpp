// Golden-byte tests for the pure FTMS / Heart Rate / Cycling Power parsers and builders. Every byte array here is
// hand-assembled from the field tables in the Bluetooth SIG specs (little-endian, fields in spec order); the
// comment beside each one spells out what it encodes.
#include "Engine/Fitness/Protocol/BleUuid.h"
#include "Engine/Fitness/Protocol/CadenceCalculator.h"
#include "Engine/Fitness/Protocol/CyclingPower.h"
#include "Engine/Fitness/Protocol/Ftms.h"
#include "Engine/Fitness/Protocol/HeartRate.h"
#include "doctest.h"

using namespace Struktur::Fitness;

// ---------------------------------------------------------------------------------------------------------------
// UUIDs
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("BleUuid expands 16-bit UUIDs onto the Base UUID")
{
	const BleUuid heartRate = BleUuid::FromShort(Uuids::HEART_RATE_SERVICE);
	CHECK(heartRate.ToString() == "0000180d-0000-1000-8000-00805f9b34fb");
	CHECK(heartRate.GetShort() == std::optional<uint16_t>(0x180D));
	CHECK(BleUuid::Parse("0000180D-0000-1000-8000-00805F9B34FB") == heartRate);
	CHECK(BleUuid::Parse("180d") == heartRate);
	CHECK_FALSE(BleUuid::Parse("0000180d-0000-1000-8000-00805f9b34f").has_value());
	CHECK_FALSE(BleUuid::Parse("0000180d+0000-1000-8000-00805f9b34fb").has_value());

	// A vendor (non-base) UUID has no short form.
	const std::optional<BleUuid> vendor = BleUuid::Parse("a026ee01-0a7d-4ab3-97fa-f1500f9feb8b");
	REQUIRE(vendor.has_value());
	CHECK_FALSE(vendor->GetShort().has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// FTMS Indoor Bike Data
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Indoor Bike Data: speed is present when the More Data bit is CLEAR")
{
	// flags 0x0044 = cadence + power, bit 0 clear -> speed present
	// speed 3250 (32.50 km/h), cadence 180 (90 rpm), power 250 W
	const Bytes packet = {0x44, 0x00, 0xB2, 0x0C, 0xB4, 0x00, 0xFA, 0x00};
	const auto data    = Ftms::ParseIndoorBikeData(packet);
	REQUIRE(data.has_value());
	CHECK_FALSE(data->HasMoreData());
	REQUIRE(data->instantaneousSpeedKmh.has_value());
	CHECK(*data->instantaneousSpeedKmh == doctest::Approx(32.50));
	CHECK(*data->instantaneousCadenceRpm == doctest::Approx(90.0));
	CHECK(data->instantaneousPowerWatts == std::optional<int16_t>(250));
	CHECK_FALSE(data->averageSpeedKmh.has_value());
	CHECK_FALSE(data->heartRateBpm.has_value());
}

TEST_CASE("Indoor Bike Data: speed is absent when the More Data bit is SET")
{
	// flags 0x0041 = More Data + power -> no speed field, power 300 W follows the flags directly
	const Bytes packet = {0x41, 0x00, 0x2C, 0x01};
	const auto data    = Ftms::ParseIndoorBikeData(packet);
	REQUIRE(data.has_value());
	CHECK(data->HasMoreData());
	CHECK_FALSE(data->instantaneousSpeedKmh.has_value());
	CHECK(data->instantaneousPowerWatts == std::optional<int16_t>(300));
}

TEST_CASE("Indoor Bike Data: every field, in spec order")
{
	const Bytes packet = {
	    0xFE, 0x1F,        // flags: bits 1-12 set, bit 0 clear (speed present)
	    0xC4, 0x09,        // instantaneous speed 2500 -> 25.00 km/h
	    0x60, 0x09,        // average speed 2400 -> 24.00 km/h
	    0xAB, 0x00,        // instantaneous cadence 171 -> 85.5 rpm
	    0xA0, 0x00,        // average cadence 160 -> 80 rpm
	    0x39, 0x30, 0x00,  // total distance 12345 m (uint24)
	    0xFB, 0xFF,        // resistance level -5 (sint16)
	    0xC8, 0x00,        // instantaneous power 200 W
	    0xB4, 0x00,        // average power 180 W
	    0x96, 0x00,        // total energy 150 kcal
	    0x58, 0x02,        // energy per hour 600 kcal
	    0x0A,              // energy per minute 10 kcal
	    0x8E,              // heart rate 142 bpm
	    0x41,              // metabolic equivalent 65 -> 6.5
	    0x10, 0x0E,        // elapsed time 3600 s
	    0xB0, 0x04,        // remaining time 1200 s
	};
	const auto data = Ftms::ParseIndoorBikeData(packet);
	REQUIRE(data.has_value());
	CHECK(*data->instantaneousSpeedKmh == doctest::Approx(25.0));
	CHECK(*data->averageSpeedKmh == doctest::Approx(24.0));
	CHECK(*data->instantaneousCadenceRpm == doctest::Approx(85.5));
	CHECK(*data->averageCadenceRpm == doctest::Approx(80.0));
	CHECK(data->totalDistanceMeters == std::optional<uint32_t>(12345));
	CHECK(data->resistanceLevel == std::optional<int16_t>(-5));
	CHECK(data->instantaneousPowerWatts == std::optional<int16_t>(200));
	CHECK(data->averagePowerWatts == std::optional<int16_t>(180));
	CHECK(data->totalEnergyKcal == std::optional<uint16_t>(150));
	CHECK(data->energyPerHourKcal == std::optional<uint16_t>(600));
	CHECK(data->energyPerMinuteKcal == std::optional<uint8_t>(10));
	CHECK(data->heartRateBpm == std::optional<uint8_t>(142));
	CHECK(*data->metabolicEquivalent == doctest::Approx(6.5));
	CHECK(data->elapsedTimeSeconds == std::optional<uint16_t>(3600));
	CHECK(data->remainingTimeSeconds == std::optional<uint16_t>(1200));
}

TEST_CASE("Indoor Bike Data: truncated packets are rejected")
{
	CHECK_FALSE(Ftms::ParseIndoorBikeData(Bytes{}).has_value());
	CHECK_FALSE(Ftms::ParseIndoorBikeData(Bytes{0x44}).has_value());
	// Power flagged but only one of its two bytes present.
	CHECK_FALSE(Ftms::ParseIndoorBikeData(Bytes{0x44, 0x00, 0xB2, 0x0C, 0xB4, 0x00, 0xFA}).has_value());
	// More Data clear but no speed bytes at all.
	CHECK_FALSE(Ftms::ParseIndoorBikeData(Bytes{0x00, 0x00}).has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// FTMS Feature / ranges
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("FTMS Feature splits into machine and target-setting bitfields")
{
	// machine 0x00004082 = cadence | resistance level | power measurement
	// target  0x0000200C = resistance | power | indoor bike simulation
	const Bytes packet  = {0x82, 0x40, 0x00, 0x00, 0x0C, 0x20, 0x00, 0x00};
	const auto features = Ftms::ParseFeatures(packet);
	REQUIRE(features.has_value());
	CHECK(features->HasMachineFeature(Ftms::MachineFeature::CADENCE));
	CHECK(features->HasMachineFeature(Ftms::MachineFeature::POWER_MEASUREMENT));
	CHECK(features->HasMachineFeature(Ftms::MachineFeature::RESISTANCE_LEVEL));
	CHECK_FALSE(features->HasMachineFeature(Ftms::MachineFeature::HEART_RATE_MEASUREMENT));
	CHECK(features->HasTargetSetting(Ftms::TargetSettingFeature::POWER));
	CHECK(features->HasTargetSetting(Ftms::TargetSettingFeature::RESISTANCE));
	CHECK(features->HasTargetSetting(Ftms::TargetSettingFeature::INDOOR_BIKE_SIMULATION));
	CHECK_FALSE(features->HasTargetSetting(Ftms::TargetSettingFeature::INCLINATION));
	CHECK_FALSE(Ftms::ParseFeatures(Bytes{0x82, 0x40, 0x00, 0x00, 0x0C}).has_value());
}

TEST_CASE("Supported Power Range and Supported Resistance Range")
{
	// power: min 0 W, max 2000 W, increment 1 W
	const auto power = Ftms::ParsePowerRange(Bytes{0x00, 0x00, 0xD0, 0x07, 0x01, 0x00});
	REQUIRE(power.has_value());
	CHECK(power->minimumWatts == 0);
	CHECK(power->maximumWatts == 2000);
	CHECK(power->incrementWatts == 1);

	// resistance: min 0 (0.0), max 200 (20.0), increment 10 (1.0) - 0.1 resolution
	const auto resistance = Ftms::ParseResistanceRange(Bytes{0x00, 0x00, 0xC8, 0x00, 0x0A, 0x00});
	REQUIRE(resistance.has_value());
	CHECK(resistance->minimum == doctest::Approx(0.0));
	CHECK(resistance->maximum == doctest::Approx(20.0));
	CHECK(resistance->increment == doctest::Approx(1.0));

	CHECK_FALSE(Ftms::ParsePowerRange(Bytes{0x00, 0x00, 0xD0, 0x07, 0x01}).has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// FTMS Control Point
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Control Point command builders")
{
	CHECK(Ftms::BuildRequestControl() == Bytes{0x00});
	CHECK(Ftms::BuildReset() == Bytes{0x01});
	CHECK(Ftms::BuildStartOrResume() == Bytes{0x07});
	CHECK(Ftms::BuildStopOrPause(false) == Bytes{0x08, 0x01});
	CHECK(Ftms::BuildStopOrPause(true) == Bytes{0x08, 0x02});

	// Set Target Power: sint16 watts
	CHECK(Ftms::BuildSetTargetPower(150) == Bytes{0x05, 0x96, 0x00});
	CHECK(Ftms::BuildSetTargetPower(1000) == Bytes{0x05, 0xE8, 0x03});

	// Set Target Resistance Level: uint8, 0.1 resolution, clamped to the field
	CHECK(Ftms::BuildSetTargetResistanceLevel(12.5) == Bytes{0x04, 0x7D});
	CHECK(Ftms::BuildSetTargetResistanceLevel(30.0) == Bytes{0x04, 0xFF});
	CHECK(Ftms::BuildSetTargetResistanceLevel(-1.0) == Bytes{0x04, 0x00});
}

TEST_CASE("Set Indoor Bike Simulation Parameters encoding")
{
	Ftms::SimulationParameters climb;
	climb.gradePercent = 5.0;  // 500 = 0x01F4; Crr 0.004 -> 40 (0x28); Cw 0.51 -> 51 (0x33)
	CHECK(Ftms::BuildSetIndoorBikeSimulation(climb) == Bytes{0x11, 0x00, 0x00, 0xF4, 0x01, 0x28, 0x33});

	Ftms::SimulationParameters descent;
	descent.windSpeedMetersPerSecond = -1.5;   // -1500 = 0xFA24
	descent.gradePercent             = -3.25;  // -325 = 0xFEBB
	CHECK(Ftms::BuildSetIndoorBikeSimulation(descent) == Bytes{0x11, 0x24, 0xFA, 0xBB, 0xFE, 0x28, 0x33});

	Ftms::SimulationParameters extreme;
	extreme.gradePercent = 400.0;  // beyond sint16 at 0.01 % -> clamps to 32767
	CHECK(Ftms::BuildSetIndoorBikeSimulation(extreme) == Bytes{0x11, 0x00, 0x00, 0xFF, 0x7F, 0x28, 0x33});
}

TEST_CASE("Control Point response indications")
{
	const auto success = Ftms::ParseControlPointResponse(Bytes{0x80, 0x05, 0x01});
	REQUIRE(success.has_value());
	CHECK(success->requestOpCode == static_cast<uint8_t>(Ftms::OpCode::SET_TARGET_POWER));
	CHECK(success->result == Ftms::ResultCode::SUCCESS);
	CHECK(success->parameters.empty());

	const auto denied = Ftms::ParseControlPointResponse(Bytes{0x80, 0x00, 0x05});
	REQUIRE(denied.has_value());
	CHECK(denied->requestOpCode == static_cast<uint8_t>(Ftms::OpCode::REQUEST_CONTROL));
	CHECK(denied->result == Ftms::ResultCode::CONTROL_NOT_PERMITTED);

	const auto withParameters = Ftms::ParseControlPointResponse(Bytes{0x80, 0x11, 0x02, 0xAA});
	REQUIRE(withParameters.has_value());
	CHECK(withParameters->result == Ftms::ResultCode::OP_CODE_NOT_SUPPORTED);
	CHECK(withParameters->parameters == Bytes{0xAA});

	CHECK_FALSE(Ftms::ParseControlPointResponse(Bytes{0x05, 0x01}).has_value());
	CHECK_FALSE(Ftms::ParseControlPointResponse(Bytes{0x80, 0x05}).has_value());
	CHECK(std::string(Ftms::ToString(Ftms::ResultCode::INVALID_PARAMETER)) == "Invalid Parameter");
}

TEST_CASE("Machine Status")
{
	const auto lost = Ftms::ParseMachineStatus(Bytes{0xFF});
	REQUIRE(lost.has_value());
	CHECK(lost->opCode == Ftms::MachineStatusOpCode::CONTROL_PERMISSION_LOST);

	const auto powerChanged = Ftms::ParseMachineStatus(Bytes{0x08, 0xC8, 0x00});
	REQUIRE(powerChanged.has_value());
	CHECK(powerChanged->opCode == Ftms::MachineStatusOpCode::TARGET_POWER_CHANGED);
	CHECK(powerChanged->parameters == Bytes{0xC8, 0x00});

	CHECK_FALSE(Ftms::ParseMachineStatus(Bytes{}).has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// Heart Rate Measurement
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Heart Rate: 8-bit value, no contact support")
{
	const auto measurement = HeartRate::ParseMeasurement(Bytes{0x00, 0x48});
	REQUIRE(measurement.has_value());
	CHECK(measurement->bpm == 72);
	CHECK(measurement->contact == HeartRate::SensorContact::NOT_SUPPORTED);
	CHECK(measurement->rrIntervals.empty());
}

TEST_CASE("Heart Rate: sensor contact bits")
{
	// 0x06 = supported + detected; 0x04 = supported, not detected; 0x02 = detected bit without support -> ignored
	CHECK(HeartRate::ParseMeasurement(Bytes{0x06, 0x8C})->contact == HeartRate::SensorContact::DETECTED);
	CHECK(HeartRate::ParseMeasurement(Bytes{0x04, 0x8C})->contact == HeartRate::SensorContact::NOT_DETECTED);
	CHECK(HeartRate::ParseMeasurement(Bytes{0x02, 0x8C})->contact == HeartRate::SensorContact::NOT_SUPPORTED);
}

TEST_CASE("Heart Rate: 16-bit value")
{
	const auto measurement = HeartRate::ParseMeasurement(Bytes{0x01, 0xA0, 0x00});
	REQUIRE(measurement.has_value());
	CHECK(measurement->bpm == 160);
	CHECK_FALSE(HeartRate::ParseMeasurement(Bytes{0x01, 0xA0}).has_value());
}

TEST_CASE("Heart Rate: energy expended and RR intervals")
{
	// flags 0x1E: contact supported + detected, energy present, RR present
	// bpm 75, energy 291 kJ, RR 1024 (1.0 s) and 800 (0.78125 s)
	const auto measurement = HeartRate::ParseMeasurement(Bytes{0x1E, 0x4B, 0x23, 0x01, 0x00, 0x04, 0x20, 0x03});
	REQUIRE(measurement.has_value());
	CHECK(measurement->bpm == 75);
	CHECK(measurement->contact == HeartRate::SensorContact::DETECTED);
	CHECK(measurement->energyExpendedKilojoules == std::optional<uint16_t>(291));
	REQUIRE(measurement->rrIntervals.size() == 2);
	CHECK(measurement->rrIntervals[0] == 1024);
	CHECK(HeartRate::RrIntervalToSeconds(measurement->rrIntervals[1]) == doctest::Approx(0.78125));
}

TEST_CASE("Heart Rate: a stray trailing RR byte keeps the heart rate")
{
	const auto measurement = HeartRate::ParseMeasurement(Bytes{0x10, 0x4B, 0x00, 0x04, 0x20});
	REQUIRE(measurement.has_value());
	CHECK(measurement->bpm == 75);
	CHECK(measurement->rrIntervals == std::vector<uint16_t>{1024});
	CHECK_FALSE(HeartRate::ParseMeasurement(Bytes{}).has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// Cycling Power Measurement
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Cycling Power: power only")
{
	const auto measurement = CyclingPower::ParseMeasurement(Bytes{0x00, 0x00, 0xFA, 0x00});
	REQUIRE(measurement.has_value());
	CHECK(measurement->powerWatts == 250);
	CHECK_FALSE(measurement->crank.has_value());
	CHECK_FALSE(measurement->wheel.has_value());
}

TEST_CASE("Cycling Power: crank revolution data")
{
	// flags 0x0020 (crank present), power 200 W, crank revs 258, last crank event 1024 (1.0 s)
	const auto measurement = CyclingPower::ParseMeasurement(Bytes{0x20, 0x00, 0xC8, 0x00, 0x02, 0x01, 0x00, 0x04});
	REQUIRE(measurement.has_value());
	CHECK(measurement->powerWatts == 200);
	REQUIRE(measurement->crank.has_value());
	CHECK(measurement->crank->cumulativeRevolutions == 258);
	CHECK(measurement->crank->lastEventTime == 1024);
}

TEST_CASE("Cycling Power: every optional field, in fixed order")
{
	const Bytes packet = {
	    0xF7, 0x0F,              // flags 0x0FF7: bits 0,1,2,4,5,6,7,8,9,10,11
	    0x2C, 0x01,              // power 300 W
	    0x64,                    // pedal power balance 100 -> 50 %
	    0x34, 0x12,              // accumulated torque 0x1234
	    0x03, 0x02, 0x01, 0x00,  // cumulative wheel revolutions 0x00010203
	    0x00, 0x08,              // last wheel event time 0x0800
	    0x50, 0x00,              // cumulative crank revolutions 80
	    0x00, 0x20,              // last crank event time 0x2000
	    0xF4, 0x01, 0x9C, 0xFF,  // extreme force: max 500 N, min -100 N
	    0x80, 0x02, 0xE0, 0xFF,  // extreme torque: max 0x0280, min -32
	    0xB4, 0xA0, 0x05,        // extreme angles: max 180 (low 12 bits), min 90 (high 12 bits)
	    0x0A, 0x00,              // top dead spot 10 deg
	    0xBE, 0x00,              // bottom dead spot 190 deg
	    0x07, 0x00,              // accumulated energy 7 kJ
	};
	const auto measurement = CyclingPower::ParseMeasurement(packet);
	REQUIRE(measurement.has_value());
	CHECK(measurement->powerWatts == 300);
	CHECK(*measurement->GetPedalPowerBalancePercent() == doctest::Approx(50.0));
	CHECK(measurement->accumulatedTorqueRaw == std::optional<uint16_t>(0x1234));
	REQUIRE(measurement->wheel.has_value());
	CHECK(measurement->wheel->cumulativeRevolutions == 0x00010203u);
	CHECK(measurement->wheel->lastEventTime == 0x0800);
	REQUIRE(measurement->crank.has_value());
	CHECK(measurement->crank->cumulativeRevolutions == 80);
	CHECK(measurement->crank->lastEventTime == 0x2000);
	CHECK(measurement->maximumForceNewtons == std::optional<int16_t>(500));
	CHECK(measurement->minimumForceNewtons == std::optional<int16_t>(-100));
	CHECK(measurement->maximumTorqueRaw == std::optional<int16_t>(0x0280));
	CHECK(measurement->minimumTorqueRaw == std::optional<int16_t>(-32));
	CHECK(measurement->maximumAngleDegrees == std::optional<uint16_t>(180));
	CHECK(measurement->minimumAngleDegrees == std::optional<uint16_t>(90));
	CHECK(measurement->topDeadSpotDegrees == std::optional<uint16_t>(10));
	CHECK(measurement->bottomDeadSpotDegrees == std::optional<uint16_t>(190));
	CHECK(measurement->accumulatedEnergyKilojoules == std::optional<uint16_t>(7));
}

TEST_CASE("Cycling Power: truncated optional field is rejected")
{
	// crank flagged but the event time is missing
	CHECK_FALSE(CyclingPower::ParseMeasurement(Bytes{0x20, 0x00, 0xC8, 0x00, 0x02, 0x01, 0x00}).has_value());
	CHECK_FALSE(CyclingPower::ParseMeasurement(Bytes{0x00, 0x00, 0xFA}).has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// Cadence from crank revolution deltas
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Cadence: first sample is a baseline, then revolutions over event time")
{
	CadenceCalculator calculator;
	CHECK_FALSE(calculator.AddSample(100, 0, 0.0).has_value());
	// 1 revolution in 1024/1024 s -> 60 rpm
	CHECK(*calculator.AddSample(101, 1024, 1.0) == doctest::Approx(60.0));
	// 3 revolutions in 2048 ticks (2 s) -> 90 rpm
	CHECK(*calculator.AddSample(104, 3072, 3.0) == doctest::Approx(90.0));
}

TEST_CASE("Cadence: uint16 rollover of both revolutions and event time")
{
	CadenceCalculator calculator;
	calculator.AddSample(65535, 64512, 0.0);
	// revolutions 65535 -> 1 = +2; time 64512 -> 512 = +1536 ticks (1.5 s) -> 80 rpm
	CHECK(*calculator.AddSample(1, 512, 1.5) == doctest::Approx(80.0));
}

TEST_CASE("Cadence: repeated event holds, then decays to zero when stale")
{
	CadenceCalculator calculator(3.0);
	calculator.AddSample(10, 0, 0.0);
	calculator.AddSample(11, 1024, 1.0);  // 60 rpm
	CHECK(*calculator.AddSample(11, 1024, 2.0) == doctest::Approx(60.0));
	CHECK(*calculator.GetCadence(3.9) == doctest::Approx(60.0));
	CHECK(*calculator.GetCadence(4.1) == doctest::Approx(0.0));
}

TEST_CASE("Cadence: implausible deltas re-baseline without changing the value")
{
	CadenceCalculator calculator;
	calculator.AddSample(10, 0, 0.0);
	calculator.AddSample(11, 1024, 1.0);  // 60 rpm
	// 10 revolutions in 100 ticks would be 6144 rpm - rejected
	CHECK(*calculator.AddSample(21, 1124, 1.1) == doctest::Approx(60.0));
	// next sane delta is measured from the re-baselined point: 1 rev in 512 ticks -> 120 rpm
	CHECK(*calculator.AddSample(22, 1636, 1.6) == doctest::Approx(120.0));
}

TEST_CASE("Cadence: a long gap re-baselines instead of trusting a wrapped clock")
{
	CadenceCalculator calculator;
	calculator.AddSample(10, 0, 0.0);
	calculator.AddSample(11, 1024, 1.0);
	// 90 s later the event clock has wrapped at least once - treat as a fresh baseline at 0 rpm
	CHECK(*calculator.AddSample(12, 2048, 91.0) == doctest::Approx(0.0));
	CHECK(*calculator.AddSample(13, 3072, 92.0) == doctest::Approx(60.0));
}
