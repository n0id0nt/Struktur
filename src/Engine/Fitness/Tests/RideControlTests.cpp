// Ride control tests: the ERG/SIM trainer controller (ramps, rate limit, trims, clamping, cadence protection), the ride
// session (start on pedalling, auto-pause, laps, time in target), settings persistence, and a FIT round trip through
// an independent minimal decoder written here from the FIT protocol description.
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <map>

#include "Engine/Ride/FitWriter.h"
#include "Engine/Ride/RideSession.h"
#include "Engine/Ride/RideSettings.h"
#include "Engine/Ride/TrainerController.h"
#include "doctest.h"

using namespace Struktur;
using namespace Struktur::Ride;

// A Trainer that records every command it is given.
class RecordingTrainer : public Fitness::Trainer
{
public:
	struct Command
	{
		double time;
		bool erg;
		double value;  // watts or grade
	};

	std::string GetIdentifier() const override
	{
		return "00:00:00:00:00:01";
	}
	const std::string& GetName() const override
	{
		return m_name;
	}
	Fitness::ConnectionState GetConnectionState() const override
	{
		return state;
	}
	void Start(double) override {}
	void Update(double) override {}
	void Stop(double) override {}
	bool IsStopped() const override
	{
		return false;
	}
	const Fitness::TrainerSample& GetLatestSample() const override
	{
		return m_sample;
	}
	const Fitness::TrainerCapabilities& GetCapabilities() const override
	{
		return capabilities;
	}
	Fitness::ControlState GetControlState() const override
	{
		return Fitness::ControlState::GRANTED;
	}
	Fitness::TrainerMode GetMode() const override
	{
		return Fitness::TrainerMode::FREE_RIDE;
	}

	Fitness::CommandResult SetTargetPower(int watts) override
	{
		commands.push_back(Command{now, true, (double)watts});
		return Fitness::CommandResult{};
	}
	Fitness::CommandResult SetSimulation(const Fitness::SimulationTarget& target) override
	{
		commands.push_back(Command{now, false, target.gradePercent});
		lastSimulation = target;
		return Fitness::CommandResult{};
	}
	Fitness::CommandResult SetResistanceLevel(double) override
	{
		return Fitness::CommandResult{};
	}
	Fitness::CommandResult SetManualResistance(double) override
	{
		return Fitness::CommandResult{};
	}
	Fitness::CommandResult ClearTarget() override
	{
		return Fitness::CommandResult{};
	}

	double now                     = 0.0;
	Fitness::ConnectionState state = Fitness::ConnectionState::CONNECTED;
	Fitness::TrainerCapabilities capabilities;
	std::vector<Command> commands;
	Fitness::SimulationTarget lastSimulation;

private:
	std::string m_name = "Recording";
	Fitness::TrainerSample m_sample;
};

static ControlInputs Inputs(double now, std::optional<double> power, std::optional<double> cadence, double speed = 9.0)
{
	ControlInputs inputs;
	inputs.now                  = now;
	inputs.powerWatts           = power;
	inputs.cadenceRpm           = cadence;
	inputs.speedMetersPerSecond = speed;
	return inputs;
}

// Runs the controller at 20 Hz for `seconds`.
static void RunController(TrainerController& controller, RecordingTrainer& trainer, double& now, double seconds,
                          std::optional<double> power, std::optional<double> cadence = 90.0)
{
	const double end = now + seconds;
	while (now < end - 1e-9)
	{
		now += 0.05;
		trainer.now = now;
		controller.Update(Inputs(now, power, cadence), &trainer);
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Trainer controller
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Controller: at most one command per second, only when the value changes")
{
	TrainerController controller;
	RecordingTrainer trainer;
	double now = 0.0;
	controller.SetGradePercent(2.0);
	RunController(controller, trainer, now, 5.0, 150.0);
	REQUIRE(trainer.commands.size() == 1);  // first command immediately, then nothing changes
	CHECK_FALSE(trainer.commands[0].erg);
	CHECK(trainer.commands[0].value == doctest::Approx(2.0));

	for (int i = 0; i < 10; ++i)
	{
		controller.AdjustResistance(+1);  // ten presses within a second
	}
	RunController(controller, trainer, now, 3.0, 150.0);
	REQUIRE(trainer.commands.size() >= 2);
	for (size_t i = 1; i < trainer.commands.size(); ++i)
	{
		CHECK(trainer.commands[i].time - trainer.commands[i - 1].time >= 1.0 - 1e-9);
	}
	CHECK(trainer.commands.back().value == doctest::Approx(7.0));
	// FTMS Cw carries the speed model's 0.5 * rho * CdA.
	CHECK(trainer.lastSimulation.windResistanceCoefficient == doctest::Approx(0.5 * 1.225 * 0.32));
}

TEST_CASE("Controller: switching to ERG ramps from the rider's power - no step")
{
	TrainerController controller;
	RecordingTrainer trainer;
	double now = 0.0;
	RunController(controller, trainer, now, 2.0, 120.0);  // SIM, rider at 120 W
	controller.SetBaseTarget(250.0);
	controller.SetErgEnabled(true);
	trainer.commands.clear();
	RunController(controller, trainer, now, 4.0, 120.0);
	REQUIRE(trainer.commands.size() >= 3);
	CHECK(trainer.commands[0].erg);
	CHECK(trainer.commands[0].value < 140.0);  // starts near 120 W, not 250 W
	CHECK(trainer.commands.back().value == doctest::Approx(250.0));
	for (size_t i = 1; i < trainer.commands.size(); ++i)
	{
		CHECK(trainer.commands[i].value - trainer.commands[i - 1].value < 70.0);  // ~2.5 s ramp, 1 cmd/s
	}
}

TEST_CASE("Controller: switching to SIM starts at the grade that matches the rider's effort")
{
	TrainerController controller;
	RecordingTrainer trainer;
	double now = 0.0;
	controller.SetBaseTarget(300.0);
	controller.SetErgEnabled(true);
	RunController(controller, trainer, now, 5.0, 300.0);
	controller.SetGradePercent(0.0);
	controller.SetErgEnabled(false);
	trainer.commands.clear();
	RunController(controller, trainer, now, 0.05, 300.0);
	REQUIRE(trainer.commands.size() == 1);
	CHECK_FALSE(trainer.commands[0].erg);
	// 300 W at 9 m/s is a few percent uphill for this rider, so the handover starts there and eases to 0 %.
	CHECK(trainer.commands[0].value > 1.0);
	RunController(controller, trainer, now, 4.0, 300.0);
	CHECK(trainer.commands.back().value == doctest::Approx(0.0));
}

TEST_CASE("Controller: trims in watts or percent, clamped to the trainer's power range")
{
	TrainerController controller;
	controller.SetBaseTarget(200.0);
	controller.SetErgEnabled(true);
	controller.AdjustResistance(+1);
	controller.AdjustResistance(+1);
	CHECK(*controller.GetErgTargetWatts() == doctest::Approx(210.0));
	controller.SetTrimMode(TrimMode::PERCENT);
	controller.AdjustResistance(+1);
	CHECK(*controller.GetErgTargetWatts() == doctest::Approx(202.0));  // percent trim replaces the watt trim

	Fitness::TrainerCapabilities capabilities;
	capabilities.minimumPowerWatts = 0;
	capabilities.maximumPowerWatts = 180;
	controller.SetCapabilities(capabilities);
	CHECK(*controller.GetErgTargetWatts() == doctest::Approx(180.0));
}

TEST_CASE("Controller: grade clamps to the inclination range or the defaults")
{
	TrainerController controller;
	for (int i = 0; i < 100; ++i)
	{
		controller.AdjustResistance(+1);
	}
	CHECK(controller.GetGradePercent() == doctest::Approx(20.0));  // default maximum
	Fitness::TrainerCapabilities capabilities;
	capabilities.minimumGradePercent = -5.0;
	capabilities.maximumGradePercent = 12.0;
	controller.SetCapabilities(capabilities);
	CHECK(controller.GetGradePercent() == doctest::Approx(12.0));
	for (int i = 0; i < 100; ++i)
	{
		controller.AdjustResistance(-1);
	}
	CHECK(controller.GetGradePercent() == doctest::Approx(-5.0));
}

TEST_CASE("Controller: cadence collapse eases the ERG target down and recovery restores it")
{
	TrainerControlSettings settings;
	settings.rampSeconds = 0.0;  // isolate protection from the mode ramp
	TrainerController controller(settings);
	RecordingTrainer trainer;
	double now = 0.0;
	controller.SetBaseTarget(300.0);
	controller.SetErgEnabled(true);
	RunController(controller, trainer, now, 3.0, 300.0, 90.0);
	CHECK(*controller.GetErgTargetWatts() == doctest::Approx(300.0));

	RunController(controller, trainer, now, 2.0, 200.0, 40.0);  // under the 3 s grace period
	CHECK_FALSE(controller.IsCadenceProtectionActive());
	RunController(controller, trainer, now, 8.0, 150.0, 40.0);
	CHECK(controller.IsCadenceProtectionActive());
	CHECK(*controller.GetErgTargetWatts() == doctest::Approx(150.0));  // eased to 50 %

	RunController(controller, trainer, now, 15.0, 250.0, 85.0);
	CHECK_FALSE(controller.IsCadenceProtectionActive());
	CHECK(*controller.GetErgTargetWatts() == doctest::Approx(300.0));
}

TEST_CASE("Controller: nothing is sent to a disconnected trainer")
{
	TrainerController controller;
	RecordingTrainer trainer;
	trainer.state = Fitness::ConnectionState::RECONNECTING;
	double now    = 0.0;
	RunController(controller, trainer, now, 3.0, 150.0);
	CHECK(trainer.commands.empty());
	trainer.state = Fitness::ConnectionState::CONNECTED;
	RunController(controller, trainer, now, 0.1, 150.0);
	CHECK(trainer.commands.size() == 1);
}

// ---------------------------------------------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------------------------------------------

static SensorReadings Reading(std::optional<double> power, std::optional<double> heartRate = 140.0,
                              std::optional<double> cadence = 90.0)
{
	SensorReadings readings;
	readings.powerWatts         = power;
	readings.heartRateBpm       = heartRate;
	readings.cadenceRpm         = cadence;
	readings.trainerAssigned    = true;
	readings.trainerConnected   = true;
	readings.heartRateAssigned  = heartRate.has_value();
	readings.heartRateConnected = heartRate.has_value();
	return readings;
}

static void RunSession(RideSession& session, RecordingTrainer& trainer, double& now, double seconds,
                       std::optional<double> power, std::optional<double> heartRate = 140.0)
{
	const double end = now + seconds;
	while (now < end - 1e-9)
	{
		now += 0.05;
		trainer.now = now;
		session.Update(now, Reading(power, heartRate), &trainer);
	}
}

static Workout TwoStepWorkout()
{
	Workout workout;
	workout.name = "Two";
	WorkoutStep first;
	first.kind            = WorkoutStep::Kind::STEADY;
	first.durationSeconds = 30;
	first.startFraction = first.endFraction = 0.5;
	first.label                             = "Easy";
	WorkoutStep second;
	second.kind            = WorkoutStep::Kind::STEADY;
	second.durationSeconds = 30;
	second.startFraction = second.endFraction = 1.0;
	second.label                              = "Hard";
	workout.steps                             = {first, second};
	return workout;
}

TEST_CASE("Session: waits for the first pedal stroke, then auto-pauses and resumes on power")
{
	RideSettings settings;
	settings.autoPauseSeconds = 5.0;
	RiderInfo rider;
	RideSession session(RideMode::FREE_RIDE, rider, settings, std::nullopt, "", 1700000000, 0);
	RecordingTrainer trainer;
	double now = 100.0;
	session.Start(now);
	RunSession(session, trainer, now, 10.0, std::nullopt);
	CHECK(session.IsWaitingToStart());
	CHECK(session.GetTimerSeconds() == doctest::Approx(0.0));

	RunSession(session, trainer, now, 20.0, 200.0);
	CHECK_FALSE(session.IsWaitingToStart());
	CHECK(session.GetTimerSeconds() == doctest::Approx(20.0).epsilon(0.01));
	CHECK(session.GetLog().GetSamples().size() == 20);
	CHECK(session.GetSpeedMetersPerSecond() > 5.0);

	RunSession(session, trainer, now, 4.0, 0.0);  // under the auto-pause delay: still running
	CHECK_FALSE(session.IsPaused());
	RunSession(session, trainer, now, 10.0, 0.0);
	CHECK(session.IsAutoPaused());
	const double pausedTimer = session.GetTimerSeconds();
	RunSession(session, trainer, now, 10.0, std::nullopt);  // power sensor lost: stays paused, timer frozen
	CHECK(session.GetTimerSeconds() == doctest::Approx(pausedTimer));
	RunSession(session, trainer, now, 3.0, 180.0);
	CHECK_FALSE(session.IsPaused());

	bool sawAutoPause = false;
	bool sawResume    = false;
	for (const SessionEvent& event : session.TakeEvents())
	{
		sawAutoPause = sawAutoPause || event.type == SessionEvent::Type::AUTO_PAUSED;
		sawResume    = sawResume || event.type == SessionEvent::Type::RESUMED;
	}
	CHECK(sawAutoPause);
	CHECK(sawResume);
}

TEST_CASE("Session: workout laps per step, FTP-relative targets and time in target with ERG off")
{
	RideSettings settings;
	settings.leadInSeconds = 4.0;
	RiderInfo rider;
	rider.ftpWatts = 200.0;
	RideSession session(RideMode::WORKOUT, rider, settings, TwoStepWorkout(), "", 1700000000, 0);
	session.SetErgEnabled(false);  // targets shown and scored, but nothing sent as ERG
	RecordingTrainer trainer;
	double now = 0.0;
	session.Start(now);
	RunSession(session, trainer, now, 30.5, 100.0);  // step 1 target 100 W: in target throughout
	CHECK(*session.GetTargetWatts() == doctest::Approx(100.0 + 50.0 * (0.5 / 4.0)).epsilon(0.05));
	RunSession(session, trainer, now, 30.0, 150.0);  // step 2 target 200 W: 150 W is outside the band

	const RideLog& log = session.GetLog();
	REQUIRE(log.GetLaps().size() == 2);
	CHECK(log.GetLaps()[0].label == "Easy");
	CHECK(log.GetLaps()[1].label == "Hard");
	CHECK(session.GetTargetSeconds() >= 50);  // lead-in seconds aren't scored
	CHECK(session.GetInTargetSeconds() == doctest::Approx(30).epsilon(0.1));
	for (const RecordingTrainer::Command& command : trainer.commands)
	{
		CHECK_FALSE(command.erg);
	}
}

TEST_CASE("Session: Free Ride ERG target is typed, nudged and clamped; laps on demand")
{
	RideSettings settings;
	RiderInfo rider;
	rider.ftpWatts = 260.0;
	RideSession session(RideMode::FREE_RIDE, rider, settings, std::nullopt, "", 1700000000, 0);
	CHECK(session.GetFreeRideTarget() == doctest::Approx(170.0));  // 65 % of FTP, rounded to 5 W
	session.SetFreeRideTarget(5000.0);
	CHECK(session.GetFreeRideTarget() == doctest::Approx(1500.0));
	session.SetFreeRideTarget(212.4);
	session.NudgeFreeRideTarget(+1);
	CHECK(session.GetFreeRideTarget() == doctest::Approx(217.0));
	CHECK_FALSE(session.GetTargetWatts().has_value());  // ERG off: no target
	session.SetErgEnabled(true);

	RecordingTrainer trainer;
	double now = 0.0;
	session.Start(now);
	RunSession(session, trainer, now, 10.0, 200.0);
	CHECK(*session.GetTargetWatts() == doctest::Approx(217.0));
	CHECK(trainer.commands.back().erg);
	session.Lap(now);
	RunSession(session, trainer, now, 5.0, 200.0);
	CHECK(session.GetLapNumber() == 2);
	CHECK(session.GetLapSeconds() == doctest::Approx(5.0).epsilon(0.02));
	CHECK(session.GetLog().GetLaps().size() == 2);
}

// ---------------------------------------------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Settings round-trip, clamp and survive garbage")
{
	RideSettings settings;
	settings.heartRateZoneModel         = HeartRateZoneModel::PERCENT_LTHR;
	settings.graphWindowSeconds         = 1200;
	settings.powerSource                = PowerSource::POWER_METER;
	settings.control.trimMode           = TrimMode::PERCENT;
	settings.speed.dragAreaSquareMeters = 0.28;
	std::string warning;
	const RideSettings loaded = RideSettings::FromJson(settings.ToJson(), warning);
	CHECK(warning.empty());
	CHECK(loaded.heartRateZoneModel == HeartRateZoneModel::PERCENT_LTHR);
	CHECK(loaded.graphWindowSeconds == 1200);
	CHECK(loaded.powerSource == PowerSource::POWER_METER);
	CHECK(loaded.control.trimMode == TrimMode::PERCENT);
	CHECK(loaded.speed.dragAreaSquareMeters == doctest::Approx(0.28));

	const RideSettings clamped =
	    RideSettings::FromJson(R"({"graphWindowSeconds": 5, "autosaveSeconds": 1e9})", warning);
	CHECK(clamped.graphWindowSeconds == 60);
	CHECK(clamped.autosaveSeconds == doctest::Approx(600.0));

	const RideSettings garbage = RideSettings::FromJson("{nope", warning);
	CHECK_FALSE(warning.empty());
	CHECK(garbage.graphWindowSeconds == 600);
}

// ---------------------------------------------------------------------------------------------------------------
// FIT round trip
// ---------------------------------------------------------------------------------------------------------------

// A deliberately separate, minimal FIT reader: header + CRCs, definition/data messages, decoded field values.
struct DecodedFit
{
	bool headerCrcOk = false;
	bool fileCrcOk   = false;
	std::map<uint16_t, int> messageCounts;
	std::vector<std::map<uint8_t, uint64_t>> records;
	std::vector<std::map<uint8_t, uint64_t>> laps;
	std::vector<std::map<uint8_t, uint64_t>> sessions;
	std::vector<std::map<uint8_t, uint64_t>> fileIds;
};

static DecodedFit DecodeFit(const std::vector<uint8_t>& file)
{
	DecodedFit decoded;
	REQUIRE(file.size() >= 16);
	REQUIRE(file[0] == 14);
	REQUIRE(file[8] == '.');
	REQUIRE(file[9] == 'F');
	const uint32_t dataSize = file[4] | (file[5] << 8) | (file[6] << 16) | ((uint32_t)file[7] << 24);
	REQUIRE(file.size() == 14 + dataSize + 2);
	const uint16_t headerCrc = file[12] | (file[13] << 8);
	decoded.headerCrcOk      = FitCrc(file.data(), 12) == headerCrc;
	decoded.fileCrcOk        = FitCrc(file.data(), file.size()) == 0;  // CRC over everything incl. the CRC is 0

	struct Definition
	{
		uint16_t global = 0;
		std::vector<std::pair<uint8_t, uint8_t>> fields;  // number, size
	};
	std::map<uint8_t, Definition> definitions;
	size_t position  = 14;
	const size_t end = 14 + dataSize;
	while (position < end)
	{
		const uint8_t header = file[position++];
		REQUIRE((header & 0x80) == 0);  // no compressed timestamps in our files
		const uint8_t local = header & 0x0F;
		if (header & 0x40)
		{
			Definition definition;
			position += 2;  // reserved, architecture (checked little-endian by construction)
			definition.global = file[position] | (file[position + 1] << 8);
			position += 2;
			const uint8_t fieldCount = file[position++];
			for (int i = 0; i < fieldCount; ++i)
			{
				definition.fields.emplace_back(file[position], file[position + 1]);
				position += 3;
			}
			definitions[local] = definition;
			continue;
		}
		REQUIRE(definitions.count(local) == 1);
		const Definition& definition = definitions[local];
		std::map<uint8_t, uint64_t> values;
		for (const auto& [number, size] : definition.fields)
		{
			uint64_t value = 0;
			for (int i = 0; i < size; ++i)
			{
				value |= (uint64_t)file[position + i] << (8 * i);
			}
			position += size;
			values[number] = value;
		}
		++decoded.messageCounts[definition.global];
		if (definition.global == 20)
		{
			decoded.records.push_back(values);
		}
		else if (definition.global == 19)
		{
			decoded.laps.push_back(values);
		}
		else if (definition.global == 18)
		{
			decoded.sessions.push_back(values);
		}
		else if (definition.global == 0)
		{
			decoded.fileIds.push_back(values);
		}
	}
	CHECK(position == end);
	return decoded;
}

TEST_CASE("FIT: a recorded workout round-trips through an independent decoder")
{
	RideSettings settings;
	RiderInfo rider;
	rider.ftpWatts                     = 200.0;
	const std::filesystem::path folder = std::filesystem::temp_directory_path() / "struktur_fit_test";
	std::error_code cleanup;
	std::filesystem::remove_all(folder, cleanup);
	const std::string basePath = (folder / "ride").string();
	RideSession session(RideMode::WORKOUT, rider, settings, TwoStepWorkout(), basePath, 1700000000, 3600);
	// Autosaves go through the job system (as in the game); Finish() must still leave the complete final file.
	Struktur::Jobs::JobSystem jobs(Struktur::Jobs::JobSystemConfig{2});
	session.SetBackgroundJobs(&jobs);
	RecordingTrainer trainer;
	double now = 0.0;
	session.Start(now);
	RunSession(session, trainer, now, 40.0, 180.0, 150.0);
	session.TogglePause(now);
	RunSession(session, trainer, now, 5.0, 180.0, 150.0);  // manual pause: not recorded
	session.TogglePause(now);
	RunSession(session, trainer, now, 25.0, 210.0, std::nullopt);  // HR strap gone
	REQUIRE(session.Finish(now));

	std::vector<uint8_t> bytes;
	{
		std::ifstream file(session.GetFitPath(), std::ios::binary);
		REQUIRE(file);
		bytes.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	}
	const DecodedFit decoded = DecodeFit(bytes);
	CHECK(decoded.headerCrcOk);
	CHECK(decoded.fileCrcOk);
	CHECK(decoded.messageCounts.at(0) == 1);   // file_id
	CHECK(decoded.messageCounts.at(18) == 1);  // session
	CHECK(decoded.messageCounts.at(34) == 1);  // activity
	CHECK(decoded.messageCounts.at(21) >= 3);  // timer start, stop (pause), start, stop_all
	REQUIRE(decoded.fileIds.size() == 1);
	CHECK(decoded.fileIds[0].at(0) == 4);  // type activity

	const std::vector<RideSample>& samples = session.GetLog().GetSamples();
	REQUIRE(decoded.records.size() == samples.size());
	CHECK(samples.size() == 65);
	for (size_t i = 0; i < samples.size(); ++i)
	{
		const std::map<uint8_t, uint64_t>& record = decoded.records[i];
		CHECK(record.at(253) == ToFitTime(samples[i].unixTime));
		CHECK(record.at(7) == (uint64_t)*samples[i].powerWatts);
		CHECK(record.at(5) == (uint64_t)std::llround(samples[i].distanceMeters * 100.0));
		CHECK(record.at(6) == (uint64_t)std::llround(samples[i].speedMetersPerSecond * 1000.0));
		CHECK(record.at(2) == (uint64_t)std::llround((samples[i].altitudeMeters + 500.0) * 5.0));
		if (samples[i].heartRateBpm)
		{
			CHECK(record.at(3) == (uint64_t)*samples[i].heartRateBpm);
		}
		else
		{
			CHECK(record.at(3) == 0xFF);  // invalid marker
		}
	}
	// Timestamps keep moving through the pause, the timer doesn't.
	CHECK(samples.back().unixTime - samples.front().unixTime >= 69);

	REQUIRE(decoded.laps.size() == 2);
	CHECK(decoded.sessions[0].at(26) == 2);  // num_laps
	const RideSummary summary = session.GetLog().SummarizeRide();
	CHECK(decoded.sessions[0].at(8) == (uint64_t)std::llround(summary.timerSeconds * 1000.0));
	CHECK(decoded.sessions[0].at(20) == (uint64_t)*summary.averagePower);
	CHECK(decoded.sessions[0].at(5) == 2);  // sport cycling
	CHECK(decoded.sessions[0].at(6) == 6);  // indoor cycling

	// The JSON sidecar exists and carries every sample.
	{
		std::ifstream json(session.GetJsonPath());
		REQUIRE(json);
		const std::string text((std::istreambuf_iterator<char>(json)), std::istreambuf_iterator<char>());
		CHECK(text.find("\"struktur-ride\"") != std::string::npos);
	}
	std::error_code ignored;
	std::filesystem::remove_all(folder, ignored);
}

TEST_CASE("FIT: CRC matches the published check value")
{
	// FIT's CRC-16 (reflected 0x8005, init 0) of "123456789" is 0xBB3D.
	const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
	CHECK(FitCrc(data, sizeof(data)) == 0xBB3D);
}

// Writes a realistic sample ride for the independent Python validator (tools/validate_fit.py) when
// STRUKTUR_FIT_SAMPLE_OUT names a base path; otherwise does nothing.
TEST_CASE("FIT: write a sample ride for the external validator")
{
	const char* basePath = std::getenv("STRUKTUR_FIT_SAMPLE_OUT");
	if (basePath == nullptr || basePath[0] == '\0')
	{
		return;
	}
	RideSettings settings;
	RiderInfo rider;
	rider.name     = "Sample Rider";
	rider.ftpWatts = 250.0;
	RideSession session(RideMode::WORKOUT, rider, settings, TwoStepWorkout(), basePath, 1767225600, 0);
	RecordingTrainer trainer;
	double now = 0.0;
	session.Start(now);
	RunSession(session, trainer, now, 35.0, 150.0, 130.0);
	session.TogglePause(now);
	RunSession(session, trainer, now, 4.0, 150.0, 130.0);
	session.TogglePause(now);
	RunSession(session, trainer, now, 30.0, 260.0, 165.0);
	CHECK(session.Finish(now));
}

TEST_CASE("autosaves write their files in the background and report back")
{
	for (int workers : {2, 0})
	{
		CAPTURE(workers);
		RideSettings settings;
		settings.autosaveSeconds = 10.0;
		RiderInfo rider;
		const std::filesystem::path folder = std::filesystem::temp_directory_path() / "struktur_autosave_test";
		std::error_code cleanup;
		std::filesystem::remove_all(folder, cleanup);
		std::filesystem::create_directories(folder, cleanup);
		const std::string basePath = (folder / "ride").string();
		Struktur::Jobs::JobSystem jobs(Struktur::Jobs::JobSystemConfig{workers});
		RideSession session(RideMode::FREE_RIDE, rider, settings, std::nullopt, basePath, 1700000000, 0);
		session.SetBackgroundJobs(&jobs);
		RecordingTrainer trainer;
		double now = 0.0;
		session.Start(now);
		RunSession(session, trainer, now, 12.0, 180.0);  // past the first autosave
		bool saved = false;
		const auto waitStart = std::chrono::steady_clock::now();
		while (!saved && std::chrono::steady_clock::now() - waitStart < std::chrono::seconds(10))
		{
			if (workers == 0)
			{
				jobs.RunMainThreadWork(0.01);  // what the frame scheduler does without workers
			}
			RunSession(session, trainer, now, 0.05, 180.0);
			for (const SessionEvent& event : session.TakeEvents())
			{
				saved = saved || event.type == SessionEvent::Type::SAVED;
			}
			std::this_thread::yield();
		}
		CHECK(saved);
		CHECK(session.GetLastSaveError().empty());
		CHECK(std::filesystem::exists(session.GetFitPath()));
		CHECK(std::filesystem::exists(session.GetJsonPath()));
		REQUIRE(session.Finish(now));
		CHECK_FALSE(session.IsSaving());
		std::filesystem::remove_all(folder, cleanup);
	}
}
