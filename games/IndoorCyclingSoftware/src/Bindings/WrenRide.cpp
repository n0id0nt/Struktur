// "ride" module - the Wren face of the ride screens (RideIntegration::RideService / Ride::RideSession), plus the
// UIGraph element in the "ui" module. Everything on class Ride is static. Getters build fresh snapshots (maps/lists);
// nothing calls back into Wren - changes arrive as ride* engine events (see RideService).
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

#include "Debug/Assertions.h"
#include "Engine/Callback/Variant.h"
#include "Engine/GameContext.h"
#include "Ride/Zones.h"
#include "RideContext.h"
#include "RideIntegration/RideService.h"
#include "Engine/Scripting/CallbackHelperFunctions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/UI/UIGraph.h"
#include "Bindings/RideBindings.h"
#include "WrenBindings/WrenMath.h"
#include "WrenBindings/WrenUI.h"
#include "wren.hpp"

#ifdef DEBUG
	#include <Trace/wren_trace.h>
#endif

using Struktur::Callback::Variant;
using Struktur::Callback::VariantList;
using Struktur::Callback::VariantMap;
namespace Ride = Struktur::Ride;
namespace World = Struktur::World;

static Struktur::GameContext& GetContext(WrenVM* vm)
{
	return *static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
}

static Struktur::RideIntegration::RideService& GetService(WrenVM* vm)
{
	return Struktur::RideContext::From(GetContext(vm)).GetRideService();
}

static double GetNow(WrenVM* vm)
{
	return GetContext(vm).GetTimeSystem().unscaledTime;
}

static void ReturnVariant(WrenVM* vm, const Variant& value)
{
	Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, 0, value);
}

static Variant OptionalNumber(const std::optional<double>& value)
{
	return value ? Variant(*value) : Variant(nullptr);
}

static VariantList ColorList(const Ride::ZoneColor& color)
{
	VariantList list;
	list.items = {Variant((double)color.red), Variant((double)color.green), Variant((double)color.blue)};
	return list;
}

// A number argument clamped to [minimum, maximum], or `fallback` when the slot isn't a number.
static double ClampedNumber(WrenVM* vm, int slot, double fallback, double minimum, double maximum)
{
	return wrenGetSlotType(vm, slot) == WREN_TYPE_NUM ? std::clamp(wrenGetSlotDouble(vm, slot), minimum, maximum)
	                                                  : fallback;
}

// Rider from five consecutive slots starting at `slot`: name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm,
// weightKg. (Separate arguments rather than a map: the Wren C API can't read map contents back out.)
static Ride::RiderInfo RiderFromSlots(WrenVM* vm, int slot)
{
	Ride::RiderInfo rider;
	if (wrenGetSlotType(vm, slot) == WREN_TYPE_STRING)
	{
		rider.name = wrenGetSlotString(vm, slot);
	}
	rider.ftpWatts              = ClampedNumber(vm, slot + 1, rider.ftpWatts, 50.0, 600.0);
	rider.maxHeartRateBpm       = ClampedNumber(vm, slot + 2, rider.maxHeartRateBpm, 100.0, 240.0);
	rider.thresholdHeartRateBpm = ClampedNumber(vm, slot + 3, rider.thresholdHeartRateBpm, 80.0, 230.0);
	rider.weightKg              = ClampedNumber(vm, slot + 4, rider.weightKg, 30.0, 200.0);
	return rider;
}

static VariantMap WorkoutSummary(const Ride::Workout& workout, double ftpWatts)
{
	VariantMap summary;
	summary.items["name"]            = workout.name;
	summary.items["author"]          = workout.author;
	summary.items["description"]     = workout.description;
	summary.items["durationSeconds"] = workout.TotalDurationSeconds();
	VariantList steps;
	double start = 0.0;
	for (const Ride::WorkoutStep& step : workout.steps)
	{
		VariantMap entry;
		entry.items["label"]     = step.label;
		entry.items["start"]     = start;
		entry.items["duration"]  = step.durationSeconds;
		entry.items["low"]       = step.startFraction;
		entry.items["high"]      = step.endFraction;
		entry.items["hasTarget"] = step.HasTarget();
		entry.items["zone"]      = Ride::PowerZone(0.5 * (step.startFraction + step.endFraction) * ftpWatts, ftpWatts);
		steps.items.push_back(entry);
		start += step.durationSeconds;
	}
	summary.items["steps"] = steps;
	return summary;
}

// ============================================================================
// Settings & library
// ============================================================================

// Ride.settings() -> map of the app-wide ride settings
static void wren_RideSettings(WrenVM* vm)
{
	const Ride::RideSettings& settings = GetService(vm).GetSettings();
	VariantMap map;
	map.items["heartRateZoneModel"] = std::string(Ride::ToString(settings.heartRateZoneModel));
	map.items["graphWindowSeconds"] = settings.graphWindowSeconds;
	map.items["powerSource"]        = std::string(Ride::ToString(settings.powerSource));
	map.items["autoPauseSeconds"]   = settings.autoPauseSeconds;
	map.items["trimMode"] = std::string(settings.control.trimMode == Ride::TrimMode::PERCENT ? "percent" : "watts");
	map.items["trimStepWatts"]     = settings.control.trimStepWatts;
	map.items["gradeStepPercent"]  = settings.control.gradeStepPercent;
	map.items["cadenceProtection"] = settings.control.cadenceProtection;
	map.items["bikeMassKg"]        = settings.speed.bikeMassKg;
	map.items["dragArea"]          = settings.speed.dragAreaSquareMeters;
	map.items["rollingResistance"] = settings.speed.rollingResistance;
	map.items["targetBandPercent"] = settings.targetBandPercent;
	map.items["hudLayout"]         = settings.hudLayout;
	// Procedural world ("world" section of ride_settings.json).
	const World::WorldSettings& world = settings.world;
	map.items["worldEnabled"]        = world.enabled;
	map.items["worldSeed"]           = (double)world.seed;
	map.items["worldQuality"]        = std::string(World::ToString(world.quality));
	map.items["worldGradeSource"]    = std::string(World::ToString(world.gradeSource));
	map.items["worldBiome"]          = world.biome;
	map.items["worldComfortMode"]    = world.comfortMode;
	map.items["worldCadenceBob"]     = world.cadenceBob;
	map.items["worldSpeedFov"]       = world.speedFov;
	map.items["worldNearestUpscale"] = world.nearestUpscale;
	map.items["worldFlatShading"]    = world.flatShading;
	map.items["worldBandedFog"]      = world.bandedFog;
	map.items["worldDither"]         = world.dither;
	map.items["worldScanlines"]      = world.scanlines;
	map.items["worldDebugOverlay"]   = world.debugOverlay;
	map.items["worldCameraHeight"]   = world.cameraHeightMeters;
	map.items["worldMaxGrade"]       = world.maxGradePercent;
	map.items["worldLowResHeight"]   = world.lowResHeight;
	ReturnVariant(vm, map);
}

// world* keys of Ride.setSetting. Returns false for an unknown key / wrong type.
static bool SetWorldSetting(World::WorldSettings& world, const std::string& key, WrenVM* vm, WrenType type)
{
	if (type == WREN_TYPE_STRING)
	{
		const std::string value = wrenGetSlotString(vm, 2);
		if (key == "worldQuality")
		{
			world.quality = World::QualityFromString(value, world.quality);
			return true;
		}
		if (key == "worldGradeSource")
		{
			world.gradeSource = World::GradeSourceFromString(value, world.gradeSource);
			return true;
		}
		if (key == "worldBiome")
		{
			world.biome = value;
			return true;
		}
		return false;
	}
	if (type == WREN_TYPE_NUM)
	{
		const double value = wrenGetSlotDouble(vm, 2);
		if (key == "worldSeed")
		{
			world.seed = (uint32_t)std::clamp(value, 0.0, 4294967295.0);
			return true;
		}
		if (key == "worldCameraHeight")
		{
			world.cameraHeightMeters = value;
			return true;
		}
		if (key == "worldMaxGrade")
		{
			world.maxGradePercent = value;
			return true;
		}
		if (key == "worldLowResHeight")
		{
			world.lowResHeight = (int)value;
			return true;
		}
		return false;
	}
	if (type == WREN_TYPE_BOOL)
	{
		const bool value = wrenGetSlotBool(vm, 2);
		bool* const flags[] = {&world.enabled,        &world.comfortMode, &world.cadenceBob, &world.speedFov,
		                       &world.nearestUpscale, &world.flatShading, &world.bandedFog,  &world.dither,
		                       &world.scanlines,      &world.debugOverlay, &world.demoPacer};
		const char* const names[] = {"worldEnabled",        "worldComfortMode", "worldCadenceBob", "worldSpeedFov",
		                             "worldNearestUpscale", "worldFlatShading", "worldBandedFog",  "worldDither",
		                             "worldScanlines",      "worldDebugOverlay", "worldDemoPacer"};
		for (size_t index = 0; index < sizeof(names) / sizeof(names[0]); ++index)
		{
			if (key == names[index])
			{
				*flags[index] = value;
				return true;
			}
		}
	}
	return false;
}

// Ride.setSetting(key, value) -> bool. Call saveSettings() to persist.
static void wren_RideSetSetting(WrenVM* vm)
{
	Ride::RideSettings& settings = GetService(vm).GetSettings();
	const std::string key        = wrenGetSlotType(vm, 1) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 1) : "";
	const WrenType type          = wrenGetSlotType(vm, 2);
	bool ok                      = true;
	if (key.rfind("world", 0) == 0)
	{
		ok = SetWorldSetting(settings.world, key, vm, type);
		settings.Validate();
		if (!ok)
		{
			DEBUG_WARNING("Ride.setSetting: unknown setting or wrong value type for '%s'", key.c_str());
		}
		wrenSetSlotBool(vm, 0, ok);
		return;
	}
	if (type == WREN_TYPE_STRING)
	{
		const std::string value = wrenGetSlotString(vm, 2);
		if (key == "heartRateZoneModel")
		{
			const std::optional<Ride::HeartRateZoneModel> model = Ride::HeartRateZoneModelFromString(value.c_str());
			ok                                                  = model.has_value();
			if (model)
			{
				settings.heartRateZoneModel = *model;
			}
		}
		else if (key == "powerSource")
		{
			settings.powerSource = value == "trainer"      ? Ride::PowerSource::TRAINER
			                       : value == "powerMeter" ? Ride::PowerSource::POWER_METER
			                                               : Ride::PowerSource::AUTO;
		}
		else if (key == "trimMode")
		{
			settings.control.trimMode = value == "percent" ? Ride::TrimMode::PERCENT : Ride::TrimMode::WATTS;
		}
		else if (key == "hudLayout")
		{
			settings.hudLayout = value;
		}
		else
		{
			ok = false;
		}
	}
	else if (type == WREN_TYPE_NUM)
	{
		const double value = wrenGetSlotDouble(vm, 2);
		if (key == "graphWindowSeconds")
		{
			settings.graphWindowSeconds = (int)value;
		}
		else if (key == "autoPauseSeconds")
		{
			settings.autoPauseSeconds = value;
		}
		else if (key == "trimStepWatts")
		{
			settings.control.trimStepWatts = (int)value;
		}
		else if (key == "gradeStepPercent")
		{
			settings.control.gradeStepPercent = value;
		}
		else if (key == "bikeMassKg")
		{
			settings.speed.bikeMassKg = value;
		}
		else if (key == "dragArea")
		{
			settings.speed.dragAreaSquareMeters = value;
		}
		else if (key == "rollingResistance")
		{
			settings.speed.rollingResistance = value;
		}
		else if (key == "targetBandPercent")
		{
			settings.targetBandPercent = value;
		}
		else
		{
			ok = false;
		}
	}
	else if (type == WREN_TYPE_BOOL && key == "cadenceProtection")
	{
		settings.control.cadenceProtection = wrenGetSlotBool(vm, 2);
	}
	else
	{
		ok = false;
	}
	if (!ok)
	{
		DEBUG_WARNING("Ride.setSetting: unknown setting or wrong value type for '%s'", key.c_str());
	}
	settings.Validate();
	wrenSetSlotBool(vm, 0, ok);
}

// Ride.worldSettings() -> String: the "world" section of the settings, for the world module
static void wren_RideWorldSettings(WrenVM* vm)
{
	wrenSetSlotString(vm, 0, GetService(vm).GetSettings().world.ToJson().dump().c_str());
}

// Ride.saveSettings() -> bool
static void wren_RideSaveSettings(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetService(vm).SaveSettings());
}

// Ride.workouts() -> [{"id", "name", "source", "durationSeconds", "error"}]
static void wren_RideWorkouts(WrenVM* vm)
{
	VariantList list;
	for (const Struktur::RideIntegration::WorkoutListing& listing : GetService(vm).ListWorkouts())
	{
		VariantMap entry;
		entry.items["id"]              = listing.id;
		entry.items["name"]            = listing.name;
		entry.items["source"]          = listing.source;
		entry.items["durationSeconds"] = listing.durationSeconds;
		entry.items["error"]           = listing.error;
		list.items.push_back(entry);
	}
	ReturnVariant(vm, list);
}

// Ride.loadWorkout(id, ftpWatts) -> {"ok", "error", "id", "name", "author", "description", "durationSeconds", "steps"}
static void wren_RideLoadWorkout(WrenVM* vm)
{
	const std::string id                 = wrenGetSlotType(vm, 1) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 1) : "";
	const double ftp                     = wrenGetSlotType(vm, 2) == WREN_TYPE_NUM ? wrenGetSlotDouble(vm, 2) : 200.0;
	const Ride::WorkoutLoadResult loaded = GetService(vm).LoadWorkout(id);
	VariantMap result                    = loaded.workout ? WorkoutSummary(*loaded.workout, ftp) : VariantMap();
	result.items["ok"]                   = loaded.IsOk();
	result.items["error"]                = loaded.error;
	result.items["id"]                   = id;
	ReturnVariant(vm, result);
}

// Ride.userWorkoutsFolder() -> String (OS path)
static void wren_RideUserWorkoutsFolder(WrenVM* vm)
{
	wrenSetSlotString(vm, 0, GetService(vm).GetUserWorkoutsFolder().c_str());
}

// Ride.commandLineWorkout() -> path or null (only the first call returns it)
static void wren_RideCommandLineWorkout(WrenVM* vm)
{
	const std::optional<std::string> path = GetService(vm).TakeCommandLineWorkout();
	if (path)
	{
		wrenSetSlotString(vm, 0, path->c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// Ride.isDemo() -> bool
static void wren_RideIsDemo(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetService(vm).IsDemo());
}

// ============================================================================
// Session
// ============================================================================

// Ride.startFreeRide(name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm, weightKg) -> bool
static void wren_RideStartFreeRide(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetService(vm).StartFreeRide(RiderFromSlots(vm, 1), GetNow(vm)));
}

// Ride.startWorkout(id, name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm, weightKg) -> {"ok", "error"}
static void wren_RideStartWorkout(WrenVM* vm)
{
	const std::string id = wrenGetSlotType(vm, 1) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 1) : "";
	std::string error;
	const bool ok = GetService(vm).StartWorkout(id, RiderFromSlots(vm, 2), GetNow(vm), error);
	VariantMap result;
	result.items["ok"]    = ok;
	result.items["error"] = error;
	ReturnVariant(vm, result);
}

// Ride.isActive() -> bool
static void wren_RideIsActive(WrenVM* vm)
{
	wrenSetSlotBool(vm, 0, GetService(vm).IsActive());
}

static const char* ConnectionText(bool assigned, bool connected)
{
	return !assigned ? "none" : (connected ? "connected" : "lost");
}

// Ride.state() -> snapshot map of everything the HUD shows (null without a ride)
static void wren_RideState(WrenVM* vm)
{
	Ride::RideSession* session = GetService(vm).GetSession();
	if (session == nullptr)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	const Ride::TrainerController& controller = session->GetController();
	const Ride::RideSettings& settings        = session->GetSettings();
	VariantMap state;
	state.items["mode"]          = std::string(session->GetMode() == Ride::RideMode::WORKOUT ? "workout" : "freeRide");
	state.items["waiting"]       = session->IsWaitingToStart();
	state.items["paused"]        = session->IsPaused();
	state.items["autoPaused"]    = session->IsAutoPaused();
	state.items["finished"]      = session->IsFinished();
	state.items["timer"]         = session->GetTimerSeconds();
	state.items["elapsed"]       = session->GetElapsedSeconds();
	state.items["lap"]           = session->GetLapNumber();
	state.items["lapSeconds"]    = session->GetLapSeconds();
	const Struktur::Input::TrainerInput& trainer = GetContext(vm).GetInput().GetTrainer();
	state.items["power"]         = OptionalNumber(trainer.smoothedPowerWatts);
	state.items["powerZone"]     = session->GetPowerZone();
	state.items["powerColor"]    = ColorList(Ride::PowerZoneColor(session->GetPowerZone()));
	state.items["cadence"]       = OptionalNumber(trainer.cadenceRpm);
	state.items["heartRate"]     = OptionalNumber(trainer.heartRateBpm);
	state.items["heartRateZone"] = session->GetHeartRateZone();
	state.items["heartRateColor"] =
	    ColorList(Ride::HeartRateZoneColor(session->GetHeartRateZone(), settings.heartRateZoneModel));
	state.items["speed"]         = session->GetSpeedMetersPerSecond();
	state.items["averageSpeed"]  = session->GetAverageSpeedMetersPerSecond();
	state.items["averagePower"]  = OptionalNumber(session->GetAveragePower());
	state.items["distance"]      = session->GetDistanceMeters();
	state.items["elevationGain"] = session->GetElevationGainMeters();
	state.items["grade"]         = session->GetGradePercent();
	state.items["userGrade"]     = controller.GetGradePercent();
	state.items["minimumGrade"]  = controller.GetMinimumGrade();
	state.items["maximumGrade"]  = controller.GetMaximumGrade();
	state.items["erg"]           = controller.IsErgEnabled();
	state.items["controlMode"] =
	    std::string(controller.GetMode() == Ride::TrainerController::Mode::ERG ? "erg" : "sim");
	state.items["target"]      = OptionalNumber(session->GetTargetWatts());
	state.items["stepTarget"]  = OptionalNumber(session->GetStepTargetWatts());
	state.items["trimWatts"]   = controller.GetTrimWatts();
	state.items["trimPercent"] = controller.GetTrimPercent();
	state.items["trimMode"]    = std::string(controller.GetTrimMode() == Ride::TrimMode::PERCENT ? "percent" : "watts");
	state.items["freeRideTarget"]        = session->GetFreeRideTarget();
	state.items["cadenceProtection"]     = controller.IsCadenceProtectionActive();
	state.items["inTargetSeconds"]       = session->GetInTargetSeconds();
	state.items["targetSeconds"]         = session->GetTargetSeconds();
	state.items["ftpWatts"]              = session->GetRider().ftpWatts;
	VariantMap sensors;
	sensors.items["trainer"]   = std::string(ConnectionText(trainer.trainerAssigned, trainer.trainerConnected));
	sensors.items["heartRate"] = std::string(ConnectionText(trainer.heartRateAssigned, trainer.heartRateConnected));
	sensors.items["powerMeter"] =
	    std::string(ConnectionText(trainer.powerMeterAssigned, trainer.powerMeterConnected));
	state.items["sensors"] = sensors;

	if (const Ride::WorkoutRunner* runner = session->GetRunner())
	{
		const Ride::Workout& workout  = runner->GetWorkout();
		const Ride::WorkoutStep& step = runner->GetCurrentStep();
		const double ftp              = session->GetRider().ftpWatts;
		VariantMap info;
		info.items["name"]          = workout.name;
		info.items["stepIndex"]     = (int)runner->GetStepIndex();
		info.items["stepCount"]     = (int)workout.steps.size();
		info.items["stepLabel"]     = step.label;
		info.items["stepKind"]      = (int)step.kind;
		info.items["stepRemaining"] = runner->GetStepRemaining();
		info.items["stepDuration"]  = runner->GetStepDuration();
		info.items["stepLow"]       = step.startFraction * ftp;
		info.items["stepHigh"]      = step.endFraction * ftp;
		info.items["stepHasTarget"] = step.HasTarget();
		info.items["stepCadence"]   = OptionalNumber(step.cadenceRpm);
		info.items["planPosition"]  = runner->GetPlanPosition();
		info.items["totalDuration"] = workout.TotalDurationSeconds();
		info.items["finished"]      = runner->IsFinished();
		if (const Ride::WorkoutStep* next = runner->GetNextStep())
		{
			info.items["nextLabel"]     = next->label;
			info.items["nextDuration"]  = next->durationSeconds;
			info.items["nextLow"]       = next->startFraction * ftp;
			info.items["nextHigh"]      = next->endFraction * ftp;
			info.items["nextHasTarget"] = next->HasTarget();
		}
		state.items["workout"] = info;
	}
	ReturnVariant(vm, state);
}

// Ride.toggleErg()
static void wren_RideToggleErg(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->ToggleErg();
	}
}

// Ride.adjustResistance(direction)
static void wren_RideAdjustResistance(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->AdjustResistance((int)wrenGetSlotDouble(vm, 1));
	}
}

// Ride.setTrimMode("watts" | "percent")
static void wren_RideSetTrimMode(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		const std::string mode = wrenGetSlotType(vm, 1) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 1) : "watts";
		session->SetTrimMode(mode == "percent" ? Ride::TrimMode::PERCENT : Ride::TrimMode::WATTS);
	}
}

// Ride.setFreeRideTarget(watts) - validated and clamped by the session
static void wren_RideSetFreeRideTarget(WrenVM* vm)
{
	Ride::RideSession* session = GetService(vm).GetSession();
	if (session && wrenGetSlotType(vm, 1) == WREN_TYPE_NUM)
	{
		const double watts = wrenGetSlotDouble(vm, 1);
		if (std::isfinite(watts))
		{
			session->SetFreeRideTarget(watts);
		}
	}
}

// Ride.nudgeTarget(direction)
static void wren_RideNudgeTarget(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->NudgeFreeRideTarget((int)wrenGetSlotDouble(vm, 1));
	}
}

// Ride.togglePause()
static void wren_RideTogglePause(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->TogglePause(GetNow(vm));
	}
}

// Ride.skipStep()
static void wren_RideSkipStep(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->SkipStep(GetNow(vm));
	}
}

// Ride.extendStep(seconds)
static void wren_RideExtendStep(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->ExtendStep(wrenGetSlotDouble(vm, 1));
	}
}

// Ride.lap()
static void wren_RideLap(WrenVM* vm)
{
	if (Ride::RideSession* session = GetService(vm).GetSession())
	{
		session->Lap(GetNow(vm));
	}
}

// Ride.finish() -> {"ok", "fitPath", "jsonPath", "error", "timer", "distance", "averagePower", "elevationGain"}
static void wren_RideFinish(WrenVM* vm)
{
	Struktur::RideIntegration::RideService& service = GetService(vm);
	Ride::RideSession* session                      = service.GetSession();
	VariantMap result;
	if (session == nullptr)
	{
		result.items["ok"]    = false;
		result.items["error"] = std::string("no ride");
		ReturnVariant(vm, result);
		return;
	}
	const bool ok                 = service.Finish(GetNow(vm));
	const Ride::RideSummary ride  = session->GetLog().SummarizeRide();
	result.items["ok"]            = ok;
	result.items["error"]         = session->GetLastSaveError();
	result.items["fitPath"]       = session->GetFitPath();
	result.items["jsonPath"]      = session->GetJsonPath();
	result.items["timer"]         = ride.timerSeconds;
	result.items["distance"]      = ride.distanceMeters;
	result.items["elevationGain"] = ride.ascentMeters;
	result.items["averagePower"]  = ride.averagePower ? Variant((double)*ride.averagePower) : Variant(nullptr);
	ReturnVariant(vm, result);
}

// Ride.discard() - drops the session (after finish, or to abandon a ride without saving)
static void wren_RideDiscard(WrenVM* vm)
{
	GetService(vm).Discard();
}

// ============================================================================
// Graphs
// ============================================================================

static const Struktur::Util::Color TARGET_COLOR{235, 240, 245, 225};
static const Struktur::Util::Color FTP_LINE_COLOR{255, 255, 255, 45};

static Struktur::UI::UIGraph* GraphFromSlot(WrenVM* vm, int slot)
{
	if (wrenGetSlotType(vm, slot) != WREN_TYPE_FOREIGN)
	{
		return nullptr;
	}
	WrenUIElement* element = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, slot));
	return element && element->element ? dynamic_cast<Struktur::UI::UIGraph*>(element->element) : nullptr;
}

static Struktur::Util::Color ToColor(const Ride::ZoneColor& color, uint8_t alpha = 255)
{
	return Struktur::Util::Color{color.red, color.green, color.blue, alpha};
}

// Ride.updateGraph(graph, windowSeconds) - power (zone-coloured line + fill), heart rate (zone-coloured line on its
// own scale) and the target as a step line behind them, over the last windowSeconds of ride time.
static void wren_RideUpdateGraph(WrenVM* vm)
{
	Struktur::UI::UIGraph* graph = GraphFromSlot(vm, 1);
	Ride::RideSession* session   = GetService(vm).GetSession();
	if (graph == nullptr || session == nullptr)
	{
		return;
	}
	const double window =
	    std::clamp(wrenGetSlotType(vm, 2) == WREN_TYPE_NUM ? wrenGetSlotDouble(vm, 2) : 600.0, 60.0, 3600.0);
	const std::vector<Ride::RideSample>& samples = session->GetLog().GetSamples();
	const double end                             = session->GetTimerSeconds();
	const double start                           = end - window;
	const Ride::RiderInfo& rider                 = session->GetRider();
	const Ride::HeartRateZoneModel hrModel       = session->GetSettings().heartRateZoneModel;

	Struktur::UI::UIGraph::Series target;
	target.thickness = 4.0f;
	target.zOffset   = -3;
	// A faint line at FTP gives the zones a reference.
	Struktur::UI::UIGraph::Series ftpLine;
	ftpLine.thickness = 1.0f;
	ftpLine.zOffset   = -4;
	Struktur::UI::UIGraph::Series power;
	power.thickness = 2.0f;
	power.fill      = true;
	power.fillAlpha = 70;
	Struktur::UI::UIGraph::Series heart;
	heart.thickness = 2.0f;
	heart.zOffset   = 1;

	double maximumPower = rider.ftpWatts * 1.5;
	size_t first        = samples.size();
	while (first > 0 && samples[first - 1].timerSeconds >= start)
	{
		--first;
	}
	for (size_t i = first; i < samples.size(); ++i)
	{
		const Ride::RideSample& sample = samples[i];
		const float x                  = (float)((sample.timerSeconds - start) / window);
		const double watts             = sample.powerWatts.value_or(0);
		maximumPower                   = std::max(maximumPower, watts * 1.1);
		power.points.push_back(
		    {x, (float)watts, ToColor(Ride::PowerZoneColor(Ride::PowerZone(watts, rider.ftpWatts)))});
		if (sample.heartRateBpm)
		{
			const int zone =
			    Ride::HeartRateZone(*sample.heartRateBpm, hrModel, rider.maxHeartRateBpm, rider.thresholdHeartRateBpm);
			heart.points.push_back({x, (float)*sample.heartRateBpm, ToColor(Ride::HeartRateZoneColor(zone, hrModel))});
		}
		if (sample.targetWatts)
		{
			target.points.push_back({x, (float)*sample.targetWatts, TARGET_COLOR});
			maximumPower = std::max(maximumPower, *sample.targetWatts * 1.1);
		}
	}
	// Extend the target ahead of "now" so the rider sees what's coming in the next few seconds of this step.
	if (const std::optional<double> current = session->GetTargetWatts())
	{
		target.points.push_back({1.0f, (float)*current, TARGET_COLOR});
	}
	ftpLine.points  = {{0.0f, (float)rider.ftpWatts, FTP_LINE_COLOR}, {1.0f, (float)rider.ftpWatts, FTP_LINE_COLOR}};
	ftpLine.maximum = (float)maximumPower;
	power.maximum   = (float)maximumPower;
	target.maximum  = (float)maximumPower;
	heart.minimum   = 40.0f;
	heart.maximum   = (float)std::max(rider.maxHeartRateBpm, rider.thresholdHeartRateBpm * 1.15) + 5.0f;
	graph->SetSeries({ftpLine, target, power, heart});
}

// The workout as zone-coloured blocks: one fill point per step (ramps split into 12 slices so their colour follows the
// zone), free ride as a low grey block and max effort as a tall purple one.
static Struktur::UI::UIGraph::Series ProfileBlocks(const Ride::Workout& workout, double ftp)
{
	const double total = std::max(1.0, workout.TotalDurationSeconds());
	Struktur::UI::UIGraph::Series blocks;
	blocks.thickness = 0.0f;
	blocks.fill      = true;
	blocks.fillAlpha = 230;
	double highest   = 1.2;
	double time      = 0.0;
	for (const Ride::WorkoutStep& step : workout.steps)
	{
		const int pieces = step.kind == Ride::WorkoutStep::Kind::RAMP ? 12 : 1;
		for (int i = 0; i < pieces; ++i)
		{
			const double offset = step.durationSeconds * i / pieces;
			double fraction     = step.TargetFractionAt(offset + step.durationSeconds / pieces * 0.5).value_or(0.0);
			Struktur::Util::Color color = ToColor(Ride::PowerZoneColor(Ride::PowerZone(fraction * ftp, ftp)));
			if (step.kind == Ride::WorkoutStep::Kind::FREE_RIDE)
			{
				fraction = 0.45;
				color    = Struktur::Util::Color{120, 120, 120, 255};
			}
			else if (step.kind == Ride::WorkoutStep::Kind::MAX_EFFORT)
			{
				fraction = 1.6;
				color    = ToColor(Ride::PowerZoneColor(7));
			}
			highest = std::max(highest, fraction);
			blocks.points.push_back({(float)((time + offset) / total), (float)fraction, color});
		}
		time += step.durationSeconds;
	}
	blocks.points.push_back({1.0f, 0.0f, Struktur::Util::Color{0, 0, 0, 0}});
	blocks.maximum = (float)(highest * 1.1);
	return blocks;
}

// Ride.updateProfile(graph) - the whole workout as zone-coloured blocks with a marker at the current position.
static void wren_RideUpdateProfile(WrenVM* vm)
{
	Struktur::UI::UIGraph* graph = GraphFromSlot(vm, 1);
	Ride::RideSession* session   = GetService(vm).GetSession();
	if (graph == nullptr || session == nullptr || session->GetRunner() == nullptr)
	{
		return;
	}
	const Ride::WorkoutRunner& runner = *session->GetRunner();
	const Ride::Workout& workout      = runner.GetWorkout();
	const double total                = std::max(1.0, workout.TotalDurationSeconds());
	graph->SetSeries({ProfileBlocks(workout, session->GetRider().ftpWatts)});
	graph->SetMarkers({{(float)(runner.GetPlanPosition() / total), 3.0f, Struktur::Util::Color{255, 255, 255, 255}}});
}

// Ride.previewProfile(graph, id, ftpWatts) - a workout's blocks without a ride (the browser's preview).
static void wren_RidePreviewProfile(WrenVM* vm)
{
	Struktur::UI::UIGraph* graph = GraphFromSlot(vm, 1);
	if (graph == nullptr || wrenGetSlotType(vm, 2) != WREN_TYPE_STRING)
	{
		return;
	}
	const Ride::WorkoutLoadResult loaded = GetService(vm).LoadWorkout(wrenGetSlotString(vm, 2));
	const double ftp                     = wrenGetSlotType(vm, 3) == WREN_TYPE_NUM ? wrenGetSlotDouble(vm, 3) : 200.0;
	if (!loaded.workout)
	{
		graph->Clear();
		return;
	}
	graph->SetSeries({ProfileBlocks(*loaded.workout, ftp)});
	graph->SetMarkers({});
}

// ============================================================================
// UIGraph (module "ui")
// ============================================================================

static void wren_UIGraphAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenUIElement));
}

static void wren_UIGraphFinalize(void* data)
{
	WrenUIElement* uiElement = (WrenUIElement*)data;
	uiElement->~WrenUIElement();
}

// UIGraph.new(absolutePosition, relativePosition, absoluteSize, relativeSize)
static void wren_UIGraphNew(WrenVM* vm)
{
	WrenUIElement* uiElement     = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	WrenVec2* positionPixel      = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 1));
	WrenVec2* positionPercentage = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 2));
	WrenVec2* sizePixel          = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 3));
	WrenVec2* sizePercentage     = static_cast<WrenVec2*>(wrenGetSlotForeign(vm, 4));
	auto* graph = new Struktur::UI::UIGraph(positionPixel->value, positionPercentage->value, sizePixel->value,
	                                        sizePercentage->value);
	new (uiElement) WrenUIElement{graph};
#ifdef DEBUG
	char stackBuffer[4096];
	uiElement->callstack = wrenTraceGetCallStackString(vm, stackBuffer, sizeof(stackBuffer));
#endif
}

// UIGraph.clear()
static void wren_UIGraphClear(WrenVM* vm)
{
	WrenUIElement* uiElement = static_cast<WrenUIElement*>(wrenGetSlotForeign(vm, 0));
	if (auto* graph = uiElement->element ? dynamic_cast<Struktur::UI::UIGraph*>(uiElement->element) : nullptr)
	{
		graph->Clear();
	}
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

WREN_BINDING_MODULE(Ride)
{
	WREN_FOREIGN_CLASS(registry, "ui", "UIGraph", wren_UIGraphAllocate, wren_UIGraphFinalize,
	                   "Zone-coloured line/area graph drawn from quads (ride power/HR history, workout profiles)");
	WREN_CLASS_INHERITANCE(registry, "ui", "UIGraph", "UIElement");
	WREN_CONSTRUCTOR(registry, "ui", "UIGraph", "new(_,_,_,_)", wren_UIGraphNew,
	                 "Create UIGraph with absolutePosition, relativePosition, absoluteSize, relativeSize components");
	WREN_CLASS_METHOD(registry, "ui", "UIGraph", "clear()", wren_UIGraphClear, "Removes every series and marker");

	WREN_CLASS_STATIC(registry, "ride", "Ride", "settings()", wren_RideSettings,
	                  "The app-wide ride settings as a map.");
	WREN_CLASS_STATIC(
	    registry, "ride", "Ride", "setSetting(_,_)", wren_RideSetSetting,
	    "Changes one ride setting (validated); returns false for an unknown key. saveSettings() persists.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "saveSettings()", wren_RideSaveSettings, "Writes ride_settings.json.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "workouts()", wren_RideWorkouts,
	                  "Built-in and user workouts: list of maps (id, name, source, durationSeconds, error).");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "loadWorkout(_,_)", wren_RideLoadWorkout,
	                  "Loads a workout by id or path for a preview (id, ftpWatts) -> map with ok/error/steps.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "userWorkoutsFolder()", wren_RideUserWorkoutsFolder,
	                  "The folder players drop their own .zwo/.mrc files into.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "commandLineWorkout()", wren_RideCommandLineWorkout,
	                  "A workout path from the command line, returned once (else null).");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "isDemo()", wren_RideIsDemo, "True when started with --demo.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "worldSettings()", wren_RideWorldSettings,
	                  "The saved world settings as a string for World.load / World.applySettings.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "startFreeRide(_,_,_,_,_)", wren_RideStartFreeRide,
	                  "Starts a Free Ride (name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm, weightKg).");
	WREN_CLASS_STATIC(
	    registry, "ride", "Ride", "startWorkout(_,_,_,_,_,_)", wren_RideStartWorkout,
	    "Starts a workout (id, name, ftpWatts, maxHeartRateBpm, thresholdHeartRateBpm, weightKg) -> map.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "isActive()", wren_RideIsActive, "True while a ride is running.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "state()", wren_RideState,
	                  "Snapshot of the ride for the HUD, or null.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "toggleErg()", wren_RideToggleErg, "Switches ERG on/off.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "adjustResistance(_)", wren_RideAdjustResistance,
	                  "+1/-1: ERG on trims the target, ERG off changes the grade.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "setTrimMode(_)", wren_RideSetTrimMode, "\"watts\" or \"percent\".");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "setFreeRideTarget(_)", wren_RideSetFreeRideTarget,
	                  "Free Ride ERG target in watts (clamped).");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "nudgeTarget(_)", wren_RideNudgeTarget,
	                  "+1/-1: Free Ride target +/- 5 W.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "togglePause()", wren_RideTogglePause, "Pauses or resumes the ride.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "skipStep()", wren_RideSkipStep, "Skips the current workout step.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "extendStep(_)", wren_RideExtendStep,
	                  "Adds seconds to the current step.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "lap()", wren_RideLap, "Starts a new lap (Free Ride).");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "finish()", wren_RideFinish,
	                  "Ends the ride and writes FIT + JSON -> map with ok, paths and a summary.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "discard()", wren_RideDiscard, "Drops the ride session.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "updateGraph(_,_)", wren_RideUpdateGraph,
	                  "Fills a UIGraph with the last N seconds of power, heart rate and target.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "updateProfile(_)", wren_RideUpdateProfile,
	                  "Fills a UIGraph with the workout profile and position marker.");
	WREN_CLASS_STATIC(registry, "ride", "Ride", "previewProfile(_,_,_)", wren_RidePreviewProfile,
	                  "Fills a UIGraph with a workout's profile (graph, id, ftpWatts).");
}
