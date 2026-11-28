// Device-layer tests against FakeBleCentral and the FakeFtmsTrainer mock: connection lifecycle, the control point
// procedure rules, capability-driven behaviour, reconnect/backoff and clean shutdown.
#include <algorithm>
#include <functional>

#include "Engine/Fitness/Ble/FakeBleCentral.h"
#include "Engine/Fitness/Ble/FakeDevices.h"
#include "Engine/Fitness/Core/Backoff.h"
#include "Engine/Fitness/Devices/BleSensors.h"
#include "Engine/Fitness/Devices/ControlPointQueue.h"
#include "Engine/Fitness/Devices/DeviceDiscovery.h"
#include "Engine/Fitness/Devices/DeviceRegistry.h"
#include "Engine/Fitness/Devices/FtmsTrainer.h"
#include "Engine/Fitness/FitnessManager.h"
#include "doctest.h"

using namespace Struktur::Fitness;
using Struktur::Fitness::Ftms::OpCode;
using Struktur::Fitness::Ftms::ResultCode;

// Drives a device and the fake central with a simulated clock. Each Step runs the central's deferred callbacks
// and one device Update, twice, so a request and its reply both land within one step.
struct Harness
{
	FakeBleCentral central;
	double now = 0.0;
	std::vector<DeviceEvent> events;

	void Step(Device& device, double deltaSeconds = 0.05)
	{
		now += deltaSeconds;
		for (int pass = 0; pass < 2; ++pass)
		{
			central.Pump();
			device.Update(now);
			for (DeviceEvent& event : device.TakeEvents())
			{
				events.push_back(std::move(event));
			}
		}
	}

	void Run(Device& device, double seconds, double deltaSeconds = 0.05)
	{
		const double end = now + seconds;
		while (now < end)
		{
			Step(device, deltaSeconds);
		}
	}

	bool RunUntil(Device& device, const std::function<bool()>& condition, double maxSeconds = 30.0)
	{
		const double end = now + maxSeconds;
		while (now < end)
		{
			if (condition())
			{
				return true;
			}
			Step(device);
		}
		return condition();
	}

	size_t Count(DeviceEvent::Type type) const
	{
		return static_cast<size_t>(std::count_if(events.begin(), events.end(),
		                                         [type](const DeviceEvent& event) { return event.type == type; }));
	}

	bool Has(DeviceEvent::Type type) const
	{
		return Count(type) > 0;
	}
};

static size_t CountOpCode(const FakeFtmsTrainer& trainer, OpCode opCode)
{
	const std::vector<OpCode> opCodes = trainer.GetReceivedOpCodes();
	return static_cast<size_t>(std::count(opCodes.begin(), opCodes.end(), opCode));
}

static FtmsTrainerSettings FastSettings()
{
	FtmsTrainerSettings settings;
	settings.reconnect.initialDelaySeconds = 1.0;
	settings.reconnect.maximumDelaySeconds = 8.0;
	settings.controlPointTimeoutSeconds    = 1.0;
	settings.shutdownTimeoutSeconds        = 1.0;
	return settings;
}

static bool IsConnectedWithControl(const FtmsTrainer& trainer)
{
	return trainer.GetConnectionState() == ConnectionState::CONNECTED &&
	       trainer.GetControlState() == ControlState::GRANTED;
}

// ---------------------------------------------------------------------------------------------------------------
// Small pure pieces
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Addresses format and parse")
{
	CHECK(FormatAddress(0xC0FFEE123456ull) == "C0:FF:EE:12:34:56");
	BleAddress parsed = 0;
	REQUIRE(ParseAddress("c0:ff:ee:12:34:56", parsed));
	CHECK(parsed == 0xC0FFEE123456ull);
	CHECK_FALSE(ParseAddress("C0:FF:EE:12:34", parsed));
	CHECK_FALSE(ParseAddress("C0:FF:EE:12:34:5G", parsed));
}

TEST_CASE("Backoff doubles up to the cap and can be exhausted")
{
	ReconnectPolicy policy;
	policy.initialDelaySeconds = 1.0;
	policy.maximumDelaySeconds = 10.0;
	policy.maximumAttempts     = 5;
	Backoff backoff(policy);
	CHECK(backoff.NextDelay() == doctest::Approx(1.0));
	CHECK(backoff.NextDelay() == doctest::Approx(2.0));
	CHECK(backoff.NextDelay() == doctest::Approx(4.0));
	CHECK(backoff.NextDelay() == doctest::Approx(8.0));
	CHECK_FALSE(backoff.IsExhausted());
	CHECK(backoff.NextDelay() == doctest::Approx(10.0));
	CHECK(backoff.IsExhausted());
	backoff.Reset();
	CHECK(backoff.NextDelay() == doctest::Approx(1.0));
}

TEST_CASE("ControlPointQueue: one outstanding procedure at a time")
{
	ControlPointQueue queue(2.0);
	const uint32_t first  = queue.Enqueue(OpCode::REQUEST_CONTROL, Ftms::BuildRequestControl(), false);
	const uint32_t second = queue.Enqueue(OpCode::START_OR_RESUME, Ftms::BuildStartOrResume(), false);

	auto send = queue.TakeNextToSend(0.0);
	REQUIRE(send.has_value());
	CHECK(send->requestId == first);
	CHECK_FALSE(queue.TakeNextToSend(0.1).has_value());  // still waiting for the indication

	// An indication for a different op code does not complete the in-flight request.
	CHECK_FALSE(queue.OnIndication(Bytes{0x80, 0x07, 0x01}));
	CHECK(queue.HasInFlight());

	CHECK(queue.OnIndication(Bytes{0x80, 0x00, 0x01}));
	auto outcomes = queue.TakeOutcomes();
	REQUIRE(outcomes.size() == 1);
	CHECK(outcomes[0].requestId == first);
	CHECK(outcomes[0].IsSuccess());

	send = queue.TakeNextToSend(0.2);
	REQUIRE(send.has_value());
	CHECK(send->requestId == second);
}

TEST_CASE("ControlPointQueue: timeout, write failure and late answers")
{
	ControlPointQueue queue(1.0);
	queue.Enqueue(OpCode::SET_TARGET_POWER, Ftms::BuildSetTargetPower(100), true);
	queue.TakeNextToSend(10.0);
	queue.Update(10.9);
	CHECK(queue.TakeOutcomes().empty());
	queue.Update(11.1);
	auto outcomes = queue.TakeOutcomes();
	REQUIRE(outcomes.size() == 1);
	CHECK(outcomes[0].kind == ControlPointOutcome::Kind::TIMEOUT);
	// The answer arriving after the timeout is ignored.
	CHECK_FALSE(queue.OnIndication(Bytes{0x80, 0x05, 0x01}));

	const uint32_t id = queue.Enqueue(OpCode::SET_TARGET_POWER, Ftms::BuildSetTargetPower(120), true);
	queue.TakeNextToSend(12.0);
	queue.OnWriteFailed(id + 99);  // unrelated id: ignored
	CHECK(queue.HasInFlight());
	queue.OnWriteFailed(id);
	outcomes = queue.TakeOutcomes();
	REQUIRE(outcomes.size() == 1);
	CHECK(outcomes[0].kind == ControlPointOutcome::Kind::WRITE_FAILED);
	CHECK(queue.IsIdle());
}

TEST_CASE("ControlPointQueue: newer targets supersede queued ones of the same op code")
{
	ControlPointQueue queue;
	queue.Enqueue(OpCode::REQUEST_CONTROL, Ftms::BuildRequestControl(), false);
	queue.TakeNextToSend(0.0);  // in flight - never superseded
	queue.Enqueue(OpCode::SET_TARGET_POWER, Ftms::BuildSetTargetPower(100), true);
	queue.Enqueue(OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS, Ftms::BuildSetIndoorBikeSimulation({}), true);
	queue.Enqueue(OpCode::SET_TARGET_POWER, Ftms::BuildSetTargetPower(150), true);
	CHECK(queue.GetQueuedCount() == 2);
	auto outcomes = queue.TakeOutcomes();
	REQUIRE(outcomes.size() == 1);
	CHECK(outcomes[0].kind == ControlPointOutcome::Kind::SUPERSEDED);

	queue.OnIndication(Bytes{0x80, 0x00, 0x01});
	auto send = queue.TakeNextToSend(0.1);
	REQUIRE(send.has_value());
	CHECK(send->payload[0] == static_cast<uint8_t>(OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS));
	queue.OnIndication(Bytes{0x80, 0x11, 0x01});
	send = queue.TakeNextToSend(0.2);
	REQUIRE(send.has_value());
	CHECK(send->payload == Ftms::BuildSetTargetPower(150));
}

// ---------------------------------------------------------------------------------------------------------------
// FTMS trainer against the mock
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Trainer: connect, read capabilities, take control, apply a deferred target")
{
	Harness harness;
	FakeFtmsTrainer::Config config;
	config.resistanceRangeTenths = std::array<int16_t, 3>{0, 200, 10};
	FakeFtmsTrainer fake(harness.central, config);
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());

	// Asked before connecting: remembered, not sent.
	CHECK(trainer.SetTargetPower(200).status == CommandResult::Status::DEFERRED);

	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetPower() == std::optional<int>(200); }));

	CHECK(trainer.GetConnectionState() == ConnectionState::CONNECTED);
	CHECK(trainer.GetControlState() == ControlState::GRANTED);
	CHECK(trainer.GetMode() == TrainerMode::ERG);

	const TrainerCapabilities& capabilities = trainer.GetCapabilities();
	CHECK(capabilities.known);
	CHECK(capabilities.supportsTargetPower);
	CHECK(capabilities.supportsSimulation);
	CHECK_FALSE(capabilities.supportsResistanceLevel);
	CHECK(capabilities.maximumPowerWatts == std::optional<int>(2000));
	CHECK(capabilities.maximumResistance.has_value());

	// Request Control strictly first, then Start/Resume, then the target.
	const std::vector<OpCode> opCodes = fake.GetReceivedOpCodes();
	REQUIRE(opCodes.size() >= 3);
	CHECK(opCodes[0] == OpCode::REQUEST_CONTROL);
	CHECK(opCodes[1] == OpCode::START_OR_RESUME);
	CHECK(opCodes[2] == OpCode::SET_TARGET_POWER);

	CHECK(harness.Has(DeviceEvent::Type::CONNECTED));
	CHECK(harness.Has(DeviceEvent::Type::CONTROL_GRANTED));
	CHECK(harness.Has(DeviceEvent::Type::COMMAND_SUCCEEDED));
	CHECK(harness.central.IsSubscribed(fake.GetAddress(), {BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE),
	                                                       BleUuid::FromShort(Uuids::FTMS_CONTROL_POINT)}));
}

TEST_CASE("Trainer: never more than one control point write awaiting an indication")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainerSettings settings        = FastSettings();
	settings.requestControlRetries      = 0;
	settings.controlPointTimeoutSeconds = 2.0;
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", settings);
	fake.SetRespondToControlPoint(false);

	trainer.SetTargetPower(150);
	trainer.Start(harness.now);
	harness.Run(trainer, 1.5);
	// Request Control was written, nothing answered: Start/Resume and the target must still be waiting.
	CHECK(fake.GetReceivedCommands().size() == 1);
	CHECK(trainer.GetControlState() == ControlState::REQUESTING);

	harness.Run(trainer, 1.0);
	CHECK(trainer.GetControlState() == ControlState::UNRESPONSIVE);
	CHECK(harness.Has(DeviceEvent::Type::TRAINER_UNRESPONSIVE));
	CHECK(fake.GetReceivedCommands().size() == 1);
}

TEST_CASE("Trainer: ERG that never answers (reported Direto X behaviour) is surfaced, not assumed to work")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
	harness.Run(trainer, 0.3);  // let Start/Resume finish so only the ERG commands go unanswered

	fake.SetRespondToControlPoint(false);
	for (int watts : {150, 160, 170})
	{
		CHECK(trainer.SetTargetPower(watts).status == CommandResult::Status::QUEUED);
		harness.Run(trainer, 1.3);
	}
	CHECK(harness.Count(DeviceEvent::Type::COMMAND_FAILED) == 3);
	CHECK(harness.Count(DeviceEvent::Type::TRAINER_UNRESPONSIVE) == 1);
	CHECK_FALSE(harness.Has(DeviceEvent::Type::COMMAND_SUCCEEDED));
}

TEST_CASE("Trainer: targets are clamped to the trainer range and the safety cap")
{
	Harness harness;
	FakeFtmsTrainer::Config config;
	config.powerRange = Ftms::PowerRange{25, 1000, 5};
	FakeFtmsTrainer fake(harness.central, config);
	FtmsTrainerSettings settings     = FastSettings();
	settings.maximumTargetPowerWatts = 100;
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", settings);
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));

	CommandResult result = trainer.SetTargetPower(300);
	CHECK(result.status == CommandResult::Status::CLAMPED);
	CHECK(result.appliedValue == doctest::Approx(100));
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetPower() == std::optional<int>(100); }));
	CHECK(harness.Has(DeviceEvent::Type::TARGET_CLAMPED));

	result = trainer.SetTargetPower(10);  // below the trainer minimum
	CHECK(result.appliedValue == doctest::Approx(25));

	result = trainer.SetTargetPower(63);  // snapped to the 5 W increment from the 25 W minimum
	CHECK(result.appliedValue == doctest::Approx(65));
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetPower() == std::optional<int>(65); }));
}

TEST_CASE("Trainer: capability flags gate commands, and a 'not supported' answer disables the capability")
{
	SUBCASE("feature bit absent")
	{
		Harness harness;
		FakeFtmsTrainer::Config config;
		config.targetFeatures = Ftms::TargetSettingFeature::INDOOR_BIKE_SIMULATION;
		FakeFtmsTrainer fake(harness.central, config);
		FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
		trainer.Start(harness.now);
		REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));

		CHECK(trainer.SetTargetPower(150).status == CommandResult::Status::UNSUPPORTED);
		CHECK(trainer.SetResistanceLevel(5.0).status == CommandResult::Status::UNSUPPORTED);
		harness.Run(trainer, 0.5);
		CHECK(CountOpCode(fake, OpCode::SET_TARGET_POWER) == 0);
		CHECK(trainer.GetMode() == TrainerMode::FREE_RIDE);
	}
	SUBCASE("feature bit present but the trainer refuses the op code")
	{
		Harness harness;
		FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
		fake.SetResultOverride(OpCode::SET_TARGET_POWER, ResultCode::OP_CODE_NOT_SUPPORTED);
		FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
		trainer.Start(harness.now);
		REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));

		CHECK(trainer.SetTargetPower(150).status == CommandResult::Status::QUEUED);
		REQUIRE(harness.RunUntil(trainer, [&]() { return harness.Has(DeviceEvent::Type::COMMAND_UNSUPPORTED); }));
		CHECK_FALSE(trainer.GetCapabilities().supportsTargetPower);
		CHECK(trainer.SetTargetPower(160).status == CommandResult::Status::UNSUPPORTED);
	}
}

TEST_CASE("Trainer: manual resistance prefers simulation grade and never assumes resistance control")
{
	SUBCASE("simulation supported -> grade")
	{
		Harness harness;
		FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
		FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());  // grade span 0..10 %
		trainer.Start(harness.now);
		REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
		const CommandResult result = trainer.SetManualResistance(0.5);
		CHECK(result.status == CommandResult::Status::QUEUED);
		CHECK(result.appliedValue == doctest::Approx(5.0));
		REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetGradePercent().has_value(); }));
		CHECK(*fake.GetTargetGradePercent() == doctest::Approx(5.0));
		CHECK(CountOpCode(fake, OpCode::SET_TARGET_RESISTANCE_LEVEL) == 0);
	}
	SUBCASE("only resistance supported -> resistance level across the advertised range")
	{
		Harness harness;
		FakeFtmsTrainer::Config config;
		config.targetFeatures        = Ftms::TargetSettingFeature::RESISTANCE;
		config.resistanceRangeTenths = std::array<int16_t, 3>{0, 200, 10};
		FakeFtmsTrainer fake(harness.central, config);
		FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
		trainer.Start(harness.now);
		REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
		CHECK(trainer.SetManualResistance(0.5).appliedValue == doctest::Approx(10.0));
		REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetResistance().has_value(); }));
		CHECK(*fake.GetTargetResistance() == doctest::Approx(10.0));
	}
	SUBCASE("neither -> unsupported")
	{
		Harness harness;
		FakeFtmsTrainer::Config config;
		config.targetFeatures = Ftms::TargetSettingFeature::POWER;
		FakeFtmsTrainer fake(harness.central, config);
		FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
		trainer.Start(harness.now);
		REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
		CHECK(trainer.SetManualResistance(0.5).status == CommandResult::Status::UNSUPPORTED);
	}
}

TEST_CASE("Trainer: control refused by the trainer")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	fake.SetResultOverride(OpCode::REQUEST_CONTROL, ResultCode::CONTROL_NOT_PERMITTED);
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.SetTargetPower(150);
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return trainer.GetControlState() == ControlState::DENIED; }));
	CHECK(harness.Has(DeviceEvent::Type::CONTROL_DENIED));
	CHECK(trainer.SetTargetPower(160).status == CommandResult::Status::CONTROL_DENIED);
	harness.Run(trainer, 0.5);
	CHECK(CountOpCode(fake, OpCode::SET_TARGET_POWER) == 0);
}

TEST_CASE("Trainer: control revoked by the trainer is re-requested and the target re-applied")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
	trainer.SetTargetPower(180);
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetPower() == std::optional<int>(180); }));

	fake.RevokeControl();
	harness.Run(trainer, 0.5);
	CHECK(harness.Has(DeviceEvent::Type::CONTROL_LOST));
	CHECK(fake.HasControl());
	CHECK(CountOpCode(fake, OpCode::REQUEST_CONTROL) == 2);
	CHECK(CountOpCode(fake, OpCode::SET_TARGET_POWER) == 2);
}

TEST_CASE("Trainer: reconnects with backoff after link loss and restores control and target")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
	trainer.SetSimulation(SimulationTarget{4.0});
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetGradePercent().has_value(); }));

	harness.central.DropConnection(fake.GetAddress());
	harness.Step(trainer);
	CHECK(harness.Has(DeviceEvent::Type::DISCONNECTED));
	CHECK(harness.Has(DeviceEvent::Type::RECONNECTING));
	CHECK(trainer.GetConnectionState() == ConnectionState::RECONNECTING);
	CHECK(trainer.GetControlState() == ControlState::NONE);

	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }, 5.0));
	harness.Run(trainer, 0.3);
	CHECK(CountOpCode(fake, OpCode::SET_INDOOR_BIKE_SIMULATION_PARAMETERS) == 2);
	CHECK(harness.central.GetConnectAttempts(fake.GetAddress()) == 2);
}

TEST_CASE("Trainer: failed connection attempts back off exponentially")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	harness.central.FindPeripheral(fake.GetAddress())->connectFailuresRemaining = 3;
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);

	harness.Run(trainer, 0.5);  // attempt 1 fails at ~0, retry in 1 s
	CHECK(harness.central.GetConnectAttempts(fake.GetAddress()) == 1);
	harness.Run(trainer, 1.0);  // attempt 2 at ~1 s fails, retry in 2 s
	CHECK(harness.central.GetConnectAttempts(fake.GetAddress()) == 2);
	harness.Run(trainer, 1.0);  // ~2.5 s: still waiting
	CHECK(harness.central.GetConnectAttempts(fake.GetAddress()) == 2);
	harness.Run(trainer, 1.0);  // attempt 3 at ~3 s fails, retry in 4 s
	CHECK(harness.central.GetConnectAttempts(fake.GetAddress()) == 3);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }, 6.0));
	CHECK(harness.central.GetConnectAttempts(fake.GetAddress()) == 4);
	CHECK(harness.Count(DeviceEvent::Type::RECONNECTING) == 3);
}

TEST_CASE("Trainer: a single-connection trainer held by another app keeps retrying, never connects")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});  // maxConnections = 1, like a Direto
	harness.central.FindPeripheral(fake.GetAddress())->otherCentralConnections = 1;
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);
	harness.Run(trainer, 4.0);
	CHECK(trainer.GetConnectionState() == ConnectionState::RECONNECTING);
	CHECK(harness.central.GetActiveConnectionCount() == 0);

	harness.central.FindPeripheral(fake.GetAddress())->otherCentralConnections = 0;  // other app lets go
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }, 10.0));
}

TEST_CASE("Trainer: Stop sends Reset, waits for it, then releases the link")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
	trainer.SetTargetPower(200);
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetPower().has_value(); }));

	trainer.Stop(harness.now);
	CHECK_FALSE(trainer.IsStopped());
	REQUIRE(harness.RunUntil(trainer, [&]() { return trainer.IsStopped(); }, 2.0));
	CHECK(fake.GetReceivedOpCodes().back() == OpCode::RESET);
	CHECK_FALSE(fake.HasControl());
	CHECK_FALSE(fake.GetTargetPower().has_value());
	CHECK(harness.central.GetActiveConnectionCount() == 0);
	CHECK(trainer.GetConnectionState() == ConnectionState::STOPPED);
}

TEST_CASE("Trainer: Stop still releases the link when the trainer ignores Reset")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());  // 1 s shutdown timeout
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
	fake.SetRespondToControlPoint(false);
	trainer.Stop(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return trainer.IsStopped(); }, 1.5));
	CHECK(harness.central.GetActiveConnectionCount() == 0);
}

TEST_CASE("Trainer: Indoor Bike Data feeds the sample, merging records split with More Data")
{
	Harness harness;
	FakeFtmsTrainer fake(harness.central, FakeFtmsTrainer::Config{});
	FtmsTrainer trainer(harness.central, fake.GetAddress(), "Mock", FastSettings());
	trainer.Start(harness.now);
	REQUIRE(harness.RunUntil(trainer, [&]() { return IsConnectedWithControl(trainer); }));
	harness.events.clear();

	const BleCharacteristicRef indoorBikeData{BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE),
	                                          BleUuid::FromShort(Uuids::FTMS_INDOOR_BIKE_DATA)};
	// Part 1: More Data set, cadence 80 rpm only - no SAMPLE yet.
	harness.central.Notify(fake.GetAddress(), indoorBikeData, Bytes{0x05, 0x00, 0xA0, 0x00});
	harness.Step(trainer);
	CHECK_FALSE(harness.Has(DeviceEvent::Type::SAMPLE));
	CHECK(*trainer.GetLatestSample().cadenceRpm == doctest::Approx(80.0));
	// Part 2: final packet, speed 30.00 km/h + power 222 W.
	harness.central.Notify(fake.GetAddress(), indoorBikeData, Bytes{0x40, 0x00, 0xB8, 0x0B, 0xDE, 0x00});
	harness.Step(trainer);
	CHECK(harness.Count(DeviceEvent::Type::SAMPLE) == 1);
	const TrainerSample& sample = trainer.GetLatestSample();
	CHECK(sample.powerWatts == std::optional<int>(222));
	CHECK(*sample.speedKmh == doctest::Approx(30.0));
	CHECK(*sample.cadenceRpm == doctest::Approx(80.0));

	// The mock's own generator, in ERG.
	trainer.SetTargetPower(240);
	REQUIRE(harness.RunUntil(trainer, [&]() { return fake.GetTargetPower().has_value(); }));
	fake.Tick(harness.now);
	harness.Step(trainer);
	CHECK(trainer.GetLatestSample().powerWatts == std::optional<int>(240));
}

TEST_CASE("Trainer: missing FTMS service fails setup and retries rather than half-working")
{
	Harness harness;
	AddFakeHeartRateMonitor(harness.central, 0x111111111111ull, "Not a trainer");
	FtmsTrainerSettings settings       = FastSettings();
	settings.reconnect.maximumAttempts = 1;
	FtmsTrainer trainer(harness.central, 0x111111111111ull, "Wrong", settings);
	trainer.Start(harness.now);
	harness.Run(trainer, 0.5);
	CHECK(trainer.GetConnectionState() == ConnectionState::RECONNECTING);
	harness.Run(trainer, 1.5);
	CHECK(trainer.GetConnectionState() == ConnectionState::FAILED);
	CHECK(harness.Has(DeviceEvent::Type::CONNECTION_FAILED));
	CHECK(harness.central.GetActiveConnectionCount() == 0);
}

// ---------------------------------------------------------------------------------------------------------------
// Sensors
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Heart rate sensor publishes bpm and contact")
{
	Harness harness;
	const BleAddress address = 0x222222222222ull;
	AddFakeHeartRateMonitor(harness.central, address, "Strap");
	BleHeartRateSensor sensor(harness.central, address, "Strap");
	sensor.Start(harness.now);
	REQUIRE(harness.RunUntil(sensor, [&]() { return sensor.GetConnectionState() == ConnectionState::CONNECTED; }));
	SendFakeHeartRate(harness.central, address, 133);
	harness.Step(sensor);
	CHECK(sensor.GetLatestSample().bpm == std::optional<int>(133));
	CHECK(sensor.GetLatestSample().contact == HeartRateContact::DETECTED);
	CHECK(harness.Has(DeviceEvent::Type::SAMPLE));
	sensor.Stop(harness.now);
	CHECK(harness.central.GetActiveConnectionCount() == 0);
}

TEST_CASE("Power meter derives cadence from crank revolutions and decays it when pedalling stops")
{
	Harness harness;
	const BleAddress address = 0x333333333333ull;
	AddFakePowerMeter(harness.central, address, "Pedals");
	BlePowerMeter meter(harness.central, address, "Pedals");
	meter.Start(harness.now);
	REQUIRE(harness.RunUntil(meter, [&]() { return meter.GetConnectionState() == ConnectionState::CONNECTED; }));

	SendFakePower(harness.central, address, 210, 65535, 64512);
	harness.Step(meter, 1.0);
	SendFakePower(harness.central, address, 215, 1, 512);  // +2 revs in 1.5 s across the rollover -> 80 rpm
	harness.Step(meter, 1.0);
	CHECK(meter.GetLatestSample().powerWatts == std::optional<int>(215));
	REQUIRE(meter.GetLatestSample().cadenceRpm.has_value());
	CHECK(*meter.GetLatestSample().cadenceRpm == doctest::Approx(80.0));

	harness.Run(meter, 4.0);  // no crank events
	CHECK(*meter.GetLatestSample().cadenceRpm == doctest::Approx(0.0));
}

// ---------------------------------------------------------------------------------------------------------------
// Discovery, registry, manager
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Roles are assigned from advertised services, trainer first")
{
	const BleUuid ftms = BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE);
	const BleUuid cps  = BleUuid::FromShort(Uuids::CYCLING_POWER_SERVICE);
	const BleUuid hr   = BleUuid::FromShort(Uuids::HEART_RATE_SERVICE);
	CHECK(GetCandidateRoles({cps, ftms}) == std::vector<DeviceRole>{DeviceRole::TRAINER, DeviceRole::POWER_METER});
	CHECK(GetPrimaryRole({hr}) == std::optional<DeviceRole>(DeviceRole::HEART_RATE));
	CHECK(GetPrimaryRole({cps}) == std::optional<DeviceRole>(DeviceRole::POWER_METER));
	CHECK_FALSE(GetPrimaryRole({BleUuid::FromShort(0x180F)}).has_value());
}

TEST_CASE("Scanner de-duplicates and labels devices")
{
	FakeBleCentral central;
	FakeFtmsTrainer fake(central, FakeFtmsTrainer::Config{});
	AddFakeHeartRateMonitor(central, 0x444444444444ull, "HRM");
	central.AddPeripheral(0x555555555555ull, "Headphones", {BleUuid::FromShort(0x110B)});

	DeviceScanner scanner(central);
	scanner.Start();
	central.Pump();
	scanner.Update(1.0);
	central.AdvertiseAll();
	central.Pump();
	scanner.Update(2.0);

	REQUIRE(scanner.GetDevices().size() == 2);
	const DiscoveredDevice* trainer = scanner.FindDevice(fake.GetAddress());
	REQUIRE(trainer != nullptr);
	CHECK(trainer->HasRole(DeviceRole::TRAINER));
	CHECK(trainer->HasRole(DeviceRole::POWER_METER));
	CHECK(trainer->lastSeenSeconds == doctest::Approx(2.0));
	scanner.Stop();
	CHECK_FALSE(central.IsScanning());
}

TEST_CASE("Registry round-trips and rejects garbage")
{
	DeviceRegistry registry;
	registry.Remember(DeviceRole::TRAINER, RememberedDevice{"C0:FF:EE:00:00:01", "DIRETO XR 12345", "ble"});
	registry.Remember(DeviceRole::HEART_RATE, RememberedDevice{"AA:BB:CC:DD:EE:FF", "HRM-Pro", "ble"});

	DeviceRegistry loaded;
	REQUIRE(loaded.Deserialize(registry.Serialize()));
	REQUIRE(loaded.Get(DeviceRole::TRAINER).has_value());
	CHECK(loaded.Get(DeviceRole::TRAINER)->name == "DIRETO XR 12345");
	CHECK(loaded.Get(DeviceRole::HEART_RATE)->identifier == "AA:BB:CC:DD:EE:FF");
	CHECK_FALSE(loaded.Get(DeviceRole::POWER_METER).has_value());

	CHECK_FALSE(loaded.Deserialize("{not json"));
	CHECK(loaded.Get(DeviceRole::TRAINER).has_value());  // untouched by the failed load
}

TEST_CASE("FitnessManager: one role per device, remembered devices reconnect, shutdown releases every link")
{
	auto ownedCentral       = std::make_unique<FakeBleCentral>();
	FakeBleCentral& central = *ownedCentral;
	FakeFtmsTrainer fake(central, FakeFtmsTrainer::Config{});
	AddFakeHeartRateMonitor(central, 0x666666666666ull, "HRM");

	FitnessSettings settings;
	settings.trainer = FastSettings();
	FitnessManager manager(std::move(ownedCentral), settings);

	double now = 0.0;
	REQUIRE(manager.AssignDevice(DeviceRole::TRAINER, fake.GetAddress(), "Mock", now));
	CHECK_FALSE(manager.AssignDevice(DeviceRole::POWER_METER, fake.GetAddress(), "Mock", now));
	manager.GetRegistry().Remember(DeviceRole::HEART_RATE, RememberedDevice{"66:66:66:66:66:66", "HRM", "ble"});
	manager.ConnectRememberedDevices(now);
	REQUIRE(manager.GetHeartRateSensor() != nullptr);

	for (int i = 0; i < 40; ++i)
	{
		now += 0.05;
		manager.Update(now);
	}
	REQUIRE(manager.GetTrainer() != nullptr);
	CHECK(manager.GetTrainer()->GetControlState() == ControlState::GRANTED);
	CHECK(manager.GetHeartRateSensor()->GetConnectionState() == ConnectionState::CONNECTED);
	CHECK(central.GetActiveConnectionCount() == 2);

	const std::vector<DeviceEvent> events = manager.TakeEvents();
	CHECK(std::any_of(events.begin(), events.end(), [](const DeviceEvent& event)
	                  { return event.role == DeviceRole::HEART_RATE && event.type == DeviceEvent::Type::CONNECTED; }));

	manager.Shutdown();
	CHECK(central.GetActiveConnectionCount() == 0);
	CHECK(fake.GetReceivedOpCodes().back() == OpCode::RESET);
	CHECK(manager.GetTrainer() == nullptr);
}
