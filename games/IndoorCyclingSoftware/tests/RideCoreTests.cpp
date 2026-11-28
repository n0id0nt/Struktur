// Ride core tests: zones, the speed model, ZWO/MRC parsing (sample, malformed and hostile files) and the workout
// runner's timing.
#include <cmath>
#include <fstream>
#include <sstream>

#include "Ride/SpeedModel.h"
#include "Ride/Workout.h"
#include "Ride/WorkoutRunner.h"
#include "Ride/Zones.h"
#include "doctest.h"

using namespace Struktur::Ride;

static std::string DataPath(const std::string& name)
{
	return std::string(STRUKTUR_TEST_DATA_DIR) + "/Workouts/" + name;
}

static std::string SamplePath(const std::string& name)
{
	return std::string(STRUKTUR_WORKOUTS_DIR) + "/" + name;
}

// ---------------------------------------------------------------------------------------------------------------
// Zones
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Power zones follow Coggan's bands")
{
	const double ftp = 250.0;
	CHECK(PowerZone(100, ftp) == 1);  // 40 %
	CHECK(PowerZone(138, ftp) == 1);  // 55 %
	CHECK(PowerZone(140, ftp) == 2);  // 56 %
	CHECK(PowerZone(188, ftp) == 2);  // 75 %
	CHECK(PowerZone(200, ftp) == 3);  // 80 %
	CHECK(PowerZone(250, ftp) == 4);  // 100 %
	CHECK(PowerZone(280, ftp) == 5);  // 112 %
	CHECK(PowerZone(350, ftp) == 6);  // 140 %
	CHECK(PowerZone(400, ftp) == 7);  // 160 %
	CHECK(PowerZone(200, 0.0) == 0);
}

TEST_CASE("Heart rate zones from max HR and from LTHR")
{
	CHECK(HeartRateZone(100, HeartRateZoneModel::PERCENT_MAX, 200, 0) == 1);  // 50 %
	CHECK(HeartRateZone(130, HeartRateZoneModel::PERCENT_MAX, 200, 0) == 2);  // 65 %
	CHECK(HeartRateZone(150, HeartRateZoneModel::PERCENT_MAX, 200, 0) == 3);  // 75 %
	CHECK(HeartRateZone(170, HeartRateZoneModel::PERCENT_MAX, 200, 0) == 4);  // 85 %
	CHECK(HeartRateZone(185, HeartRateZoneModel::PERCENT_MAX, 200, 0) == 5);  // 92.5 %
	CHECK(HeartRateZone(150, HeartRateZoneModel::PERCENT_MAX, 0, 170) == 0);

	const double lthr = 170;
	CHECK(HeartRateZone(130, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 1);  // 76 %
	CHECK(HeartRateZone(145, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 2);  // 85 %
	CHECK(HeartRateZone(155, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 3);  // 91 %
	CHECK(HeartRateZone(165, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 4);  // 97 %
	CHECK(HeartRateZone(172, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 5);  // 101 %
	CHECK(HeartRateZone(178, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 6);  // 105 %
	CHECK(HeartRateZone(185, HeartRateZoneModel::PERCENT_LTHR, 0, lthr) == 7);  // 109 %
	CHECK(HeartRateZoneCount(HeartRateZoneModel::PERCENT_LTHR) == 7);
	CHECK(HeartRateZoneModelFromString("percentLthr") ==
	      std::optional<HeartRateZoneModel>(HeartRateZoneModel::PERCENT_LTHR));
	CHECK_FALSE(HeartRateZoneModelFromString("bogus").has_value());
}

// ---------------------------------------------------------------------------------------------------------------
// Speed model
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("Speed model settles at the analytic steady-state speed")
{
	SpeedModelParameters parameters;  // 75 + 8 kg, Crr 0.004, CdA 0.32
	const double expected = SpeedModel::SteadyStateSpeed(parameters, 200.0, 0.0);
	// 200 W on the flat with these parameters is a little over 33 km/h.
	CHECK(expected * 3.6 == doctest::Approx(33.6).epsilon(0.03));

	SpeedModel model(parameters);
	for (int i = 0; i < 600; ++i)
	{
		model.Step(200.0, 0.0, 1.0);
	}
	CHECK(model.GetSpeedMetersPerSecond() == doctest::Approx(expected).epsilon(0.005));
	CHECK(model.GetElevationGainMeters() == doctest::Approx(0.0));
}

TEST_CASE("Speed model: climbs are slower, descents faster, and speed carries momentum")
{
	SpeedModelParameters parameters;
	const double flat  = SpeedModel::SteadyStateSpeed(parameters, 250.0, 0.0);
	const double climb = SpeedModel::SteadyStateSpeed(parameters, 250.0, 6.0);
	const double down  = SpeedModel::SteadyStateSpeed(parameters, 250.0, -4.0);
	CHECK(climb < flat * 0.5);
	CHECK(down > flat);

	SpeedModel model(parameters);
	for (int i = 0; i < 300; ++i)
	{
		model.Step(250.0, 0.0, 1.0);
	}
	const double cruising = model.GetSpeedMetersPerSecond();
	// Hitting a 6 % wall: speed bleeds off over several seconds, not instantly.
	model.Step(250.0, 6.0, 1.0);
	CHECK(model.GetSpeedMetersPerSecond() > cruising * 0.85);
	for (int i = 0; i < 120; ++i)
	{
		model.Step(250.0, 6.0, 1.0);
	}
	CHECK(model.GetSpeedMetersPerSecond() == doctest::Approx(climb).epsilon(0.01));
	CHECK(model.GetElevationGainMeters() > 30.0);
	const double gained = model.GetElevationGainMeters();

	// Coasting downhill: speeds up with no power, altitude falls, gain doesn't change.
	for (int i = 0; i < 60; ++i)
	{
		model.Step(0.0, -5.0, 1.0);
	}
	CHECK(model.GetSpeedMetersPerSecond() > climb);
	CHECK(model.GetElevationGainMeters() == doctest::Approx(gained));
	CHECK(model.GetAltitudeMeters() < gained);
}

TEST_CASE("Speed model: standing start, stopping, and no rolling backwards")
{
	SpeedModel model;
	model.Step(0.0, 0.0, 5.0);
	CHECK(model.GetSpeedMetersPerSecond() == doctest::Approx(0.0));
	model.Step(300.0, 0.0, 2.0);
	CHECK(model.GetSpeedMetersPerSecond() > 2.0);
	for (int i = 0; i < 600; ++i)
	{
		model.Step(0.0, 2.0, 1.0);
	}
	CHECK(model.GetSpeedMetersPerSecond() == doctest::Approx(0.0));
	CHECK(model.GetDistanceMeters() > 0.0);
}

TEST_CASE("Equivalent grade inverts the model")
{
	SpeedModelParameters parameters;
	const double speed = SpeedModel::SteadyStateSpeed(parameters, 220.0, 3.0);
	CHECK(SpeedModel::EquivalentGrade(parameters, 220.0, speed) == doctest::Approx(3.0).epsilon(0.01));
	CHECK(SpeedModel::EquivalentGrade(parameters, 220.0, 0.0) == doctest::Approx(0.0));
}

// ---------------------------------------------------------------------------------------------------------------
// ZWO / MRC parsing
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("ZWO: every element type")
{
	const std::string zwo          = R"(<workout_file>
  <author>Test</author><name>All Steps</name><description>d</description><sportType>bike</sportType>
  <workout>
    <Warmup Duration="300" PowerLow="0.4" PowerHigh="0.7"><textevent timeoffset="5" message="hi"/></Warmup>
    <SteadyState Duration="120" Power="0.9" Cadence="95"/>
    <IntervalsT Repeat="2" OnDuration="30" OffDuration="60" OnPower="1.2" OffPower="0.5" CadenceResting="80">
      <textevent timeoffset="40" message="recover"/>
    </IntervalsT>
    <Ramp Duration="60" PowerLow="0.5" PowerHigh="1.0"/>
    <FreeRide Duration="90" FlatRoad="1"/>
    <MaxEffort Duration="15"/>
    <Cooldown Duration="200" PowerLow="0.3" PowerHigh="0.6"/>
    <SomethingNew Duration="10"/>
  </workout>
</workout_file>)";
	const WorkoutLoadResult result = ParseZwo(zwo);
	REQUIRE_MESSAGE(result.IsOk(), result.error);
	const Workout& workout = *result.workout;
	CHECK(workout.name == "All Steps");
	CHECK(workout.author == "Test");
	REQUIRE(workout.steps.size() == 10);  // warmup, steady, 2x(on, off), ramp, free, max, cooldown

	CHECK(workout.steps[0].kind == WorkoutStep::Kind::RAMP);
	CHECK(workout.steps[0].startFraction == doctest::Approx(0.4));
	CHECK(workout.steps[0].endFraction == doctest::Approx(0.7));
	REQUIRE(workout.steps[0].cues.size() == 1);
	CHECK(workout.steps[0].cues[0].message == "hi");

	CHECK(workout.steps[1].kind == WorkoutStep::Kind::STEADY);
	CHECK(workout.steps[1].cadenceRpm == std::optional<double>(95.0));

	CHECK(workout.steps[2].label == "Interval 1/2");
	CHECK(workout.steps[2].durationSeconds == doctest::Approx(30));
	CHECK(workout.steps[2].startFraction == doctest::Approx(1.2));
	CHECK(workout.steps[3].label == "Recovery 1/2");
	CHECK(workout.steps[3].cadenceRpm == std::optional<double>(80.0));
	// The block-level cue at 40 s lands 10 s into the first recovery.
	REQUIRE(workout.steps[3].cues.size() == 1);
	CHECK(workout.steps[3].cues[0].offsetSeconds == doctest::Approx(10.0));

	CHECK(workout.steps[6].kind == WorkoutStep::Kind::RAMP);
	CHECK(workout.steps[7].kind == WorkoutStep::Kind::FREE_RIDE);
	CHECK_FALSE(workout.steps[7].HasTarget());
	CHECK(workout.steps[8].kind == WorkoutStep::Kind::MAX_EFFORT);
	// Cooldown always ramps down whichever attribute holds the higher value.
	CHECK(workout.steps[9].startFraction == doctest::Approx(0.6));
	CHECK(workout.steps[9].endFraction == doctest::Approx(0.3));
	CHECK(workout.TotalDurationSeconds() == doctest::Approx(300 + 120 + 2 * 90 + 60 + 90 + 15 + 200));
}

TEST_CASE("ZWO: the bundled sample workouts load")
{
	for (const char* name : {"Sweet Spot 3x10.zwo", "VO2 Max 5x3.zwo", "Demo Quick Tour.zwo"})
	{
		const WorkoutLoadResult result = LoadWorkoutFile(SamplePath(name));
		CHECK_MESSAGE(result.IsOk(), name, ": ", result.error);
	}
	const WorkoutLoadResult sweetSpot = LoadWorkoutFile(SamplePath("Sweet Spot 3x10.zwo"));
	REQUIRE(sweetSpot.IsOk());
	CHECK(sweetSpot.workout->TotalDurationSeconds() == doctest::Approx(600 + 3 * 600 + 2 * 300 + 480));
}

TEST_CASE("ZWO: malformed files fail with a clear message")
{
	struct Case
	{
		const char* file;
		const char* expectedFragment;
	};
	const Case cases[] = {
	    {"bad_not_xml.zwo", "not valid XML"},          {"bad_missing_duration.zwo", "missing Duration"},
	    {"bad_power_text.zwo", "not a number"},        {"bad_huge_repeat.zwo", "out of range"},
	    {"bad_wrong_root.zwo", "no <workout_file>"},   {"bad_no_steps.zwo", "no steps"},
	    {"bad_negative_duration.zwo", "out of range"},
	};
	for (const Case& testCase : cases)
	{
		const WorkoutLoadResult result = LoadWorkoutFile(DataPath(testCase.file));
		CHECK_FALSE_MESSAGE(result.IsOk(), testCase.file);
		CHECK_MESSAGE(result.error.find(testCase.expectedFragment) != std::string::npos, testCase.file, " -> ",
		              result.error);
	}
}

TEST_CASE("ZWO: DTD entities are never expanded or fetched")
{
	const WorkoutLoadResult result = LoadWorkoutFile(DataPath("entity_attack.zwo"));
	// pugixml skips the DOCTYPE; undeclared entity references stay literal text, so the name is short and contains
	// nothing from the "external" file.
	if (result.IsOk())
	{
		CHECK(result.workout->name.size() < 64);
		CHECK(result.workout->name.find("fonts") == std::string::npos);
	}
	else
	{
		CHECK_FALSE(result.error.empty());
	}
}

TEST_CASE("ZWO: size and step caps")
{
	WorkoutLimits limits;
	limits.maxFileBytes = 64;
	CHECK(ParseZwo(std::string(100, ' '), limits).error.find("larger than") != std::string::npos);

	WorkoutLimits fewSteps;
	fewSteps.maxSteps = 4;
	const std::string zwo =
	    R"(<workout_file><workout><IntervalsT Repeat="3" OnDuration="10" OffDuration="10" OnPower="1" OffPower="0.5"/></workout></workout_file>)";
	CHECK(ParseZwo(zwo, fewSteps).error.find("more than 4 steps") != std::string::npos);
	CHECK(ParseWorkout("x.erg", "").error.find("unsupported file type") != std::string::npos);
}

TEST_CASE("MRC: minutes/percent course with jumps and ramps")
{
	const WorkoutLoadResult result = LoadWorkoutFile(SamplePath("Endurance Builder.mrc"));
	REQUIRE_MESSAGE(result.IsOk(), result.error);
	const Workout& workout = *result.workout;
	CHECK(workout.name == "Endurance Builder");
	REQUIRE(workout.steps.size() == 4);
	CHECK(workout.steps[0].kind == WorkoutStep::Kind::RAMP);
	CHECK(workout.steps[0].durationSeconds == doctest::Approx(600));
	CHECK(workout.steps[0].startFraction == doctest::Approx(0.45));
	CHECK(workout.steps[0].endFraction == doctest::Approx(0.65));
	CHECK(workout.steps[1].kind == WorkoutStep::Kind::STEADY);
	CHECK(workout.steps[1].durationSeconds == doctest::Approx(1200));
	CHECK(workout.steps[2].startFraction == doctest::Approx(0.72));
	CHECK(workout.TotalDurationSeconds() == doctest::Approx(55 * 60));
}

TEST_CASE("MRC: malformed courses fail clearly")
{
	CHECK(LoadWorkoutFile(DataPath("bad_watts.mrc")).error.find("MINUTES WATTS") != std::string::npos);
	CHECK(LoadWorkoutFile(DataPath("bad_backwards.mrc")).error.find("back in time") != std::string::npos);
	CHECK(LoadWorkoutFile(DataPath("bad_no_header.mrc")).error.find("COURSE HEADER") != std::string::npos);
	CHECK(LoadWorkoutFile(DataPath("does_not_exist.zwo")).error.find("can't read") != std::string::npos);
}

// ---------------------------------------------------------------------------------------------------------------
// Workout runner
// ---------------------------------------------------------------------------------------------------------------

static Workout ThreeSteps()
{
	Workout workout;
	workout.name = "Three";
	WorkoutStep first;
	first.kind            = WorkoutStep::Kind::STEADY;
	first.durationSeconds = 60;
	first.startFraction = first.endFraction = 0.5;
	first.cues.push_back(TextCue{10.0, 5.0, "ten seconds in"});
	WorkoutStep second;
	second.kind            = WorkoutStep::Kind::STEADY;
	second.durationSeconds = 30;
	second.startFraction = second.endFraction = 1.0;
	WorkoutStep third;
	third.kind            = WorkoutStep::Kind::RAMP;
	third.durationSeconds = 100;
	third.startFraction   = 0.6;
	third.endFraction     = 0.4;
	workout.steps         = {first, second, third};
	return workout;
}

TEST_CASE("Runner: steps advance on the clock, with cues and events")
{
	WorkoutRunner runner(ThreeSteps(), 4.0);
	runner.Start(1000.0);
	CHECK(runner.GetStepIndex() == 0);
	CHECK(*runner.GetTargetFraction() == doctest::Approx(0.5));
	runner.Update(1011.0);
	std::vector<RunnerEvent> events = runner.TakeEvents();
	REQUIRE(events.size() == 2);
	CHECK(events[0].type == RunnerEvent::Type::STEP_STARTED);
	CHECK(events[1].type == RunnerEvent::Type::CUE);
	CHECK(events[1].message == "ten seconds in");

	runner.Update(1060.0);
	CHECK(runner.GetStepIndex() == 1);
	CHECK(runner.GetStepRemaining() == doctest::Approx(30.0));
	CHECK(runner.GetNextStep() == &runner.GetWorkout().steps[2]);
	runner.Update(1250.0);
	CHECK(runner.IsFinished());
	events = runner.TakeEvents();
	CHECK(events.back().type == RunnerEvent::Type::FINISHED);
}

TEST_CASE("Runner: lead-in ramps the target between steps, and ramps interpolate")
{
	WorkoutRunner runner(ThreeSteps(), 4.0);
	runner.Start(0.0);
	runner.Update(60.0);  // step 2 starts: 0.5 -> 1.0 over 4 s
	CHECK(runner.IsInLeadIn());
	CHECK(*runner.GetTargetFraction() == doctest::Approx(0.5));
	runner.Update(62.0);
	CHECK(*runner.GetTargetFraction() == doctest::Approx(0.75));
	runner.Update(64.5);
	CHECK_FALSE(runner.IsInLeadIn());
	CHECK(*runner.GetTargetFraction() == doctest::Approx(1.0));
	runner.Update(90.0 + 50.0);  // halfway down the 0.6 -> 0.4 ramp
	CHECK(*runner.GetTargetFraction() == doctest::Approx(0.5));
}

TEST_CASE("Runner: pause freezes the clock; skip and extend change only the current step")
{
	WorkoutRunner runner(ThreeSteps(), 4.0);
	runner.Start(0.0);
	runner.Update(20.0);
	runner.Pause(20.0);
	runner.Update(500.0);
	CHECK(runner.GetElapsed() == doctest::Approx(20.0));
	runner.Resume(500.0);
	runner.Update(510.0);
	CHECK(runner.GetElapsed() == doctest::Approx(30.0));
	CHECK(runner.GetStepIndex() == 0);

	runner.ExtendStep(15.0);
	CHECK(runner.GetStepDuration() == doctest::Approx(75.0));
	runner.Update(560.0);  // 80 s of workout: past 60 but inside the extended 75? no - 80 > 75
	CHECK(runner.GetStepIndex() == 1);
	CHECK(runner.GetPlanPosition() == doctest::Approx(60.0 + 5.0));

	runner.SkipStep(561.0);
	CHECK(runner.GetStepIndex() == 2);
	CHECK(runner.GetStepElapsed() == doctest::Approx(0.0).epsilon(0.001));
	// Plan position jumps to the start of step 3 in the original plan.
	CHECK(runner.GetPlanPosition() == doctest::Approx(90.0));
}

TEST_CASE("Runner: free-ride steps have no target and no lead-in")
{
	Workout workout       = ThreeSteps();
	workout.steps[1].kind = WorkoutStep::Kind::FREE_RIDE;
	WorkoutRunner runner(workout, 4.0);
	runner.Start(0.0);
	runner.Update(61.0);
	CHECK_FALSE(runner.GetTargetFraction().has_value());
	runner.Update(91.0);  // into the ramp after a free ride: no blend from "nothing"
	CHECK_FALSE(runner.IsInLeadIn());
	CHECK(*runner.GetTargetFraction() == doctest::Approx(0.598).epsilon(0.01));
}
