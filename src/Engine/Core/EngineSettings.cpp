#include "EngineSettings.h"

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "nlohmann/json.hpp"

namespace
{
constexpr const char* kSettingsPath = "engine_settings.json";
}

Struktur::Core::EngineSettings Struktur::Core::EngineSettings::Load()
{
	EngineSettings settings;

	if (!FileSystem::Exists(kSettingsPath))
	{
		DEBUG_INFO("ENGINESETTINGS: No settings found at %s, using defaults", kSettingsPath);
		return settings;
	}

#ifdef EDITOR
	auto result = FileSystem::ReadString(kSettingsPath);
#else
	auto result = FileSystem::ReadEncrypted(kSettingsPath);
#endif

	if (!result)
	{
		DEBUG_ERROR("ENGINESETTINGS: Failed to load %s - %s", kSettingsPath, result.errorMessage.c_str());
		return settings;
	}

	// allow_exceptions=false + is_discarded() instead of a bare parse() call - a malformed file degrades to
	// "log and return defaults" rather than throwing, matching InputConfigLoader/StateWindow's own convention.
	nlohmann::json json = nlohmann::json::parse(result.value, nullptr, false);
	if (json.is_discarded())
	{
		DEBUG_ERROR("ENGINESETTINGS: Failed to parse %s (malformed JSON)", kSettingsPath);
		return settings;
	}

	if (json.contains("vsync") && json["vsync"].is_boolean())
	{
		settings.vsync = json["vsync"].get<bool>();
	}
	if (json.contains("targetFps") && json["targetFps"].is_number_integer())
	{
		settings.targetFps = json["targetFps"].get<int>();
	}

	DEBUG_INFO("ENGINESETTINGS: Loaded from %s", kSettingsPath);
	return settings;
}

void Struktur::Core::EngineSettings::Save(const EngineSettings& settings)
{
	nlohmann::json json;
	json["vsync"]     = settings.vsync;
	json["targetFps"] = settings.targetFps;

	std::string data = json.dump(4);

#ifdef EDITOR
	auto result = FileSystem::WriteString(kSettingsPath, data);
#else
	auto result = FileSystem::WriteEncrypted(kSettingsPath, data);
#endif

	if (!result)
	{
		DEBUG_ERROR("ENGINESETTINGS: Failed to save %s - %s", kSettingsPath, result.errorMessage.c_str());
		return;
	}

	FileSystem::SyncSaves();
	DEBUG_INFO("ENGINESETTINGS: Saved to %s", kSettingsPath);
}
