#include "RideSettings.h"

#include <algorithm>

#include "nlohmann/json.hpp"

namespace Struktur::Ride
{
static constexpr int SETTINGS_VERSION = 1;

const char* ToString(PowerSource source)
{
	switch (source)
	{
		case PowerSource::TRAINER:
			return "trainer";
		case PowerSource::POWER_METER:
			return "powerMeter";
		case PowerSource::AUTO:
			break;
	}
	return "auto";
}

static PowerSource PowerSourceFromString(const std::string& text)
{
	if (text == "trainer")
	{
		return PowerSource::TRAINER;
	}
	if (text == "powerMeter")
	{
		return PowerSource::POWER_METER;
	}
	return PowerSource::AUTO;
}

template <typename Value>
static void ReadValue(const nlohmann::json& object, const char* key, Value& out)
{
	if (!object.is_object() || !object.contains(key))
	{
		return;
	}
	const nlohmann::json& value = object[key];
	if constexpr (std::is_same_v<Value, bool>)
	{
		if (value.is_boolean())
		{
			out = value.get<bool>();
		}
	}
	else if constexpr (std::is_arithmetic_v<Value>)
	{
		if (value.is_number())
		{
			out = value.get<Value>();
		}
	}
}

void RideSettings::Validate()
{
	graphWindowSeconds                 = std::clamp(graphWindowSeconds, 60, 3600);
	autoPauseSeconds                   = autoPauseSeconds <= 0.0 ? 0.0 : std::clamp(autoPauseSeconds, 2.0, 120.0);
	autosaveSeconds                    = std::clamp(autosaveSeconds, 10.0, 600.0);
	targetBandPercent                  = std::clamp(targetBandPercent, 1.0, 25.0);
	targetBandMinimumWatts             = std::clamp(targetBandMinimumWatts, 0.0, 50.0);
	leadInSeconds                      = std::clamp(leadInSeconds, 0.0, 5.0);
	freeRideStartTargetWatts           = std::clamp(freeRideStartTargetWatts, 0, 1500);
	world.Validate();
	speed.bikeMassKg                   = std::clamp(speed.bikeMassKg, 3.0, 40.0);
	speed.rollingResistance            = std::clamp(speed.rollingResistance, 0.001, 0.02);
	speed.dragAreaSquareMeters         = std::clamp(speed.dragAreaSquareMeters, 0.15, 0.8);
	speed.airDensityKgPerCubicM        = std::clamp(speed.airDensityKgPerCubicM, 0.8, 1.4);
	speed.drivetrainEfficiency         = std::clamp(speed.drivetrainEfficiency, 0.85, 1.0);
	control.rampSeconds                = std::clamp(control.rampSeconds, 0.0, 5.0);
	control.minCommandIntervalSeconds  = std::clamp(control.minCommandIntervalSeconds, 1.0, 5.0);
	control.trimStepWatts              = std::clamp(control.trimStepWatts, 1, 50);
	control.trimStepPercent            = std::clamp(control.trimStepPercent, 0.5, 10.0);
	control.gradeStepPercent           = std::clamp(control.gradeStepPercent, 0.1, 5.0);
	control.defaultMinimumGradePercent = std::clamp(control.defaultMinimumGradePercent, -30.0, 0.0);
	control.defaultMaximumGradePercent = std::clamp(control.defaultMaximumGradePercent, 0.0, 30.0);
	control.defaultMinimumPowerWatts   = std::clamp(control.defaultMinimumPowerWatts, 0, 200);
	control.defaultMaximumPowerWatts   = std::clamp(control.defaultMaximumPowerWatts, 200, 3000);
	control.lowCadenceRpm              = std::clamp(control.lowCadenceRpm, 20.0, 80.0);
	control.recoveryCadenceRpm         = std::clamp(control.recoveryCadenceRpm, control.lowCadenceRpm, 100.0);
	control.protectionMinimumFraction  = std::clamp(control.protectionMinimumFraction, 0.2, 1.0);
}

RideSettings RideSettings::FromJson(const std::string& json, std::string& warning)
{
	RideSettings settings;
	const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
	if (root.is_discarded() || !root.is_object())
	{
		warning = "ride_settings.json is not valid JSON - using defaults";
		return settings;
	}
	if (root.contains("heartRateZoneModel") && root["heartRateZoneModel"].is_string())
	{
		const std::string model = root["heartRateZoneModel"].get<std::string>();
		if (const std::optional<HeartRateZoneModel> parsed = HeartRateZoneModelFromString(model.c_str()))
		{
			settings.heartRateZoneModel = *parsed;
		}
		else
		{
			warning = "unknown heartRateZoneModel '" + model + "'";
		}
	}
	if (root.contains("powerSource") && root["powerSource"].is_string())
	{
		settings.powerSource = PowerSourceFromString(root["powerSource"].get<std::string>());
	}
	ReadValue(root, "graphWindowSeconds", settings.graphWindowSeconds);
	ReadValue(root, "autoPauseSeconds", settings.autoPauseSeconds);
	ReadValue(root, "autosaveSeconds", settings.autosaveSeconds);
	ReadValue(root, "targetBandPercent", settings.targetBandPercent);
	ReadValue(root, "targetBandMinimumWatts", settings.targetBandMinimumWatts);
	ReadValue(root, "leadInSeconds", settings.leadInSeconds);
	ReadValue(root, "freeRideStartTargetWatts", settings.freeRideStartTargetWatts);

	if (root.contains("speed"))
	{
		const nlohmann::json& speed = root["speed"];
		ReadValue(speed, "bikeMassKg", settings.speed.bikeMassKg);
		ReadValue(speed, "rollingResistance", settings.speed.rollingResistance);
		ReadValue(speed, "dragArea", settings.speed.dragAreaSquareMeters);
		ReadValue(speed, "airDensity", settings.speed.airDensityKgPerCubicM);
		ReadValue(speed, "drivetrainEfficiency", settings.speed.drivetrainEfficiency);
	}
	if (root.contains("control"))
	{
		const nlohmann::json& control = root["control"];
		ReadValue(control, "rampSeconds", settings.control.rampSeconds);
		ReadValue(control, "minCommandIntervalSeconds", settings.control.minCommandIntervalSeconds);
		ReadValue(control, "trimStepWatts", settings.control.trimStepWatts);
		ReadValue(control, "trimStepPercent", settings.control.trimStepPercent);
		ReadValue(control, "gradeStepPercent", settings.control.gradeStepPercent);
		ReadValue(control, "defaultMinimumGradePercent", settings.control.defaultMinimumGradePercent);
		ReadValue(control, "defaultMaximumGradePercent", settings.control.defaultMaximumGradePercent);
		ReadValue(control, "defaultMinimumPowerWatts", settings.control.defaultMinimumPowerWatts);
		ReadValue(control, "defaultMaximumPowerWatts", settings.control.defaultMaximumPowerWatts);
		ReadValue(control, "cadenceProtection", settings.control.cadenceProtection);
		ReadValue(control, "lowCadenceRpm", settings.control.lowCadenceRpm);
		ReadValue(control, "recoveryCadenceRpm", settings.control.recoveryCadenceRpm);
		ReadValue(control, "protectionMinimumFraction", settings.control.protectionMinimumFraction);
		if (control.contains("trimMode") && control["trimMode"].is_string())
		{
			settings.control.trimMode =
			    control["trimMode"].get<std::string>() == "percent" ? TrimMode::PERCENT : TrimMode::WATTS;
		}
	}
	if (root.contains("world"))
	{
		std::string worldWarning;
		settings.world = RideWorld::WorldSettings::FromJson(root["world"], worldWarning);
		if (!worldWarning.empty())
		{
			warning = worldWarning;
		}
	}
	settings.Validate();
	return settings;
}

std::string RideSettings::ToJson() const
{
	nlohmann::json root;
	root["version"]                  = SETTINGS_VERSION;
	root["heartRateZoneModel"]       = ToString(heartRateZoneModel);
	root["graphWindowSeconds"]       = graphWindowSeconds;
	root["powerSource"]              = ToString(powerSource);
	root["autoPauseSeconds"]         = autoPauseSeconds;
	root["autosaveSeconds"]          = autosaveSeconds;
	root["targetBandPercent"]        = targetBandPercent;
	root["targetBandMinimumWatts"]   = targetBandMinimumWatts;
	root["leadInSeconds"]            = leadInSeconds;
	root["freeRideStartTargetWatts"] = freeRideStartTargetWatts;
	root["speed"]                    = {{"bikeMassKg", speed.bikeMassKg},
	                                    {"rollingResistance", speed.rollingResistance},
	                                    {"dragArea", speed.dragAreaSquareMeters},
	                                    {"airDensity", speed.airDensityKgPerCubicM},
	                                    {"drivetrainEfficiency", speed.drivetrainEfficiency}};
	root["control"]                  = {{"rampSeconds", control.rampSeconds},
	                                    {"minCommandIntervalSeconds", control.minCommandIntervalSeconds},
	                                    {"trimMode", control.trimMode == TrimMode::PERCENT ? "percent" : "watts"},
	                                    {"trimStepWatts", control.trimStepWatts},
	                                    {"trimStepPercent", control.trimStepPercent},
	                                    {"gradeStepPercent", control.gradeStepPercent},
	                                    {"defaultMinimumGradePercent", control.defaultMinimumGradePercent},
	                                    {"defaultMaximumGradePercent", control.defaultMaximumGradePercent},
	                                    {"defaultMinimumPowerWatts", control.defaultMinimumPowerWatts},
	                                    {"defaultMaximumPowerWatts", control.defaultMaximumPowerWatts},
	                                    {"cadenceProtection", control.cadenceProtection},
	                                    {"lowCadenceRpm", control.lowCadenceRpm},
	                                    {"recoveryCadenceRpm", control.recoveryCadenceRpm},
	                                    {"protectionMinimumFraction", control.protectionMinimumFraction}};
	root["world"] = world.ToJson();
	return root.dump(2);
}
}  // namespace Struktur::Ride
