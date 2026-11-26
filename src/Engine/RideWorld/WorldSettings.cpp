#include "WorldSettings.h"

#include <algorithm>

#include "nlohmann/json.hpp"

namespace Struktur::RideWorld
{
const char* ToString(Quality quality)
{
	switch (quality)
	{
		case Quality::LOW:
			return "low";
		case Quality::HIGH:
			return "high";
		default:
			return "medium";
	}
}

Quality QualityFromString(const std::string& text, Quality fallback)
{
	if (text == "low")
	{
		return Quality::LOW;
	}
	if (text == "medium")
	{
		return Quality::MEDIUM;
	}
	if (text == "high")
	{
		return Quality::HIGH;
	}
	return fallback;
}

const char* ToString(GradeSource source)
{
	return source == GradeSource::ROUTE ? "route" : "manual";
}

GradeSource GradeSourceFromString(const std::string& text, GradeSource fallback)
{
	if (text == "manual")
	{
		return GradeSource::MANUAL;
	}
	if (text == "route")
	{
		return GradeSource::ROUTE;
	}
	return fallback;
}

QualityParameters ParametersFor(Quality quality)
{
	QualityParameters parameters;
	switch (quality)
	{
		case Quality::LOW:
			parameters.drawDistanceMeters  = 640.0;
			parameters.lateralExtentMeters = 220.0;
			parameters.propDensity         = 0.4;
			parameters.lowResHeight        = 270;
			parameters.weatherParticles    = 250;
			break;
		case Quality::HIGH:
			parameters.drawDistanceMeters  = 1536.0;
			parameters.lateralExtentMeters = 420.0;
			parameters.propDensity         = 1.0;
			parameters.lowResHeight        = 480;
			parameters.weatherParticles    = 1200;
			break;
		default:
			break;
	}
	return parameters;
}

WorldSettings WorldSettings::Defaults()
{
	WorldSettings settings;
#ifdef __EMSCRIPTEN__
	settings.quality = Quality::LOW;
#endif
	return settings;
}

template <typename T>
static void ReadValue(const nlohmann::json& object, const char* key, T& value)
{
	if (!object.contains(key))
	{
		return;
	}
	const nlohmann::json& entry = object[key];
	if constexpr (std::is_same_v<T, bool>)
	{
		if (entry.is_boolean())
		{
			value = entry.get<bool>();
		}
	}
	else if constexpr (std::is_same_v<T, std::string>)
	{
		if (entry.is_string())
		{
			value = entry.get<std::string>();
		}
	}
	else
	{
		if (entry.is_number())
		{
			value = entry.get<T>();
		}
	}
}

WorldSettings WorldSettings::FromJson(const nlohmann::json& section, std::string& warning)
{
	WorldSettings settings = Defaults();
	if (!section.is_object())
	{
		return settings;
	}
	ReadValue(section, "enabled", settings.enabled);
	if (section.contains("seed") && section["seed"].is_number_integer())
	{
		const int64_t seed = section["seed"].get<int64_t>();
		settings.seed      = static_cast<uint32_t>(std::clamp<int64_t>(seed, 0, 0xFFFFFFFFll));
	}
	if (section.contains("quality") && section["quality"].is_string())
	{
		const std::string text = section["quality"].get<std::string>();
		settings.quality       = QualityFromString(text, settings.quality);
		if (text != "low" && text != "medium" && text != "high")
		{
			warning = "unknown world.quality '" + text + "'";
		}
	}
	if (section.contains("gradeSource") && section["gradeSource"].is_string())
	{
		const std::string text = section["gradeSource"].get<std::string>();
		settings.gradeSource   = GradeSourceFromString(text, settings.gradeSource);
		if (text != "manual" && text != "route")
		{
			warning = "unknown world.gradeSource '" + text + "'";
		}
	}
	ReadValue(section, "lowResHeight", settings.lowResHeight);
	ReadValue(section, "biome", settings.biome);
	ReadValue(section, "maxGradePercent", settings.maxGradePercent);
	ReadValue(section, "maxGradeChangePerMeter", settings.maxGradeChangePerMeter);
	ReadValue(section, "minCurveRadius", settings.minCurveRadiusMeters);
	ReadValue(section, "roadHalfWidth", settings.roadHalfWidthMeters);
	ReadValue(section, "shoulder", settings.shoulderMeters);
	ReadValue(section, "cameraHeight", settings.cameraHeightMeters);
	ReadValue(section, "cameraLookAhead", settings.cameraLookAheadMeters);
	ReadValue(section, "comfortMode", settings.comfortMode);
	ReadValue(section, "cadenceBob", settings.cadenceBob);
	ReadValue(section, "speedFov", settings.speedFov);
	ReadValue(section, "fieldOfView", settings.fieldOfViewDegrees);
	ReadValue(section, "nearestUpscale", settings.nearestUpscale);
	ReadValue(section, "flatShading", settings.flatShading);
	ReadValue(section, "bandedFog", settings.bandedFog);
	ReadValue(section, "fogBands", settings.fogBands);
	ReadValue(section, "dither", settings.dither);
	ReadValue(section, "paletteLevels", settings.paletteLevels);
	ReadValue(section, "scanlines", settings.scanlines);
	ReadValue(section, "debugOverlay", settings.debugOverlay);
	ReadValue(section, "demoPacer", settings.demoPacer);
	settings.Validate();
	return settings;
}

nlohmann::json WorldSettings::ToJson() const
{
	nlohmann::json section;
	section["enabled"]                = enabled;
	section["seed"]                   = seed;
	section["quality"]                = ToString(quality);
	section["lowResHeight"]           = lowResHeight;
	section["gradeSource"]            = ToString(gradeSource);
	section["biome"]                  = biome;
	section["maxGradePercent"]        = maxGradePercent;
	section["maxGradeChangePerMeter"] = maxGradeChangePerMeter;
	section["minCurveRadius"]         = minCurveRadiusMeters;
	section["roadHalfWidth"]          = roadHalfWidthMeters;
	section["shoulder"]               = shoulderMeters;
	section["cameraHeight"]           = cameraHeightMeters;
	section["cameraLookAhead"]        = cameraLookAheadMeters;
	section["comfortMode"]            = comfortMode;
	section["cadenceBob"]             = cadenceBob;
	section["speedFov"]               = speedFov;
	section["fieldOfView"]            = fieldOfViewDegrees;
	section["nearestUpscale"]         = nearestUpscale;
	section["flatShading"]            = flatShading;
	section["bandedFog"]              = bandedFog;
	section["fogBands"]               = fogBands;
	section["dither"]                 = dither;
	section["paletteLevels"]          = paletteLevels;
	section["scanlines"]              = scanlines;
	section["debugOverlay"]           = debugOverlay;
	section["demoPacer"]              = demoPacer;
	return section;
}

void WorldSettings::Validate()
{
	if (lowResHeight != 0)
	{
		lowResHeight = std::clamp(lowResHeight, 144, 1080);
	}
	maxGradePercent        = std::clamp(maxGradePercent, 1.0, 25.0);
	maxGradeChangePerMeter = std::clamp(maxGradeChangePerMeter, 0.005, 0.5);
	minCurveRadiusMeters   = std::clamp(minCurveRadiusMeters, 40.0, 2000.0);
	roadHalfWidthMeters    = std::clamp(roadHalfWidthMeters, 2.0, 8.0);
	shoulderMeters         = std::clamp(shoulderMeters, 0.0, 4.0);
	cameraHeightMeters     = std::clamp(cameraHeightMeters, 1.5, 3.0);
	cameraLookAheadMeters  = std::clamp(cameraLookAheadMeters, 5.0, 80.0);
	fieldOfViewDegrees     = std::clamp(fieldOfViewDegrees, 40.0, 90.0);
	fogBands               = std::clamp(fogBands, 2, 32);
	paletteLevels          = std::clamp(paletteLevels, 2, 32);
	if (biome.empty() || biome.size() > 64)
	{
		biome = "auto";
	}
}

QualityParameters WorldSettings::Resolved() const
{
	QualityParameters parameters = ParametersFor(quality);
	if (lowResHeight != 0)
	{
		parameters.lowResHeight = lowResHeight;
	}
	return parameters;
}
}  // namespace Struktur::RideWorld
