#include "LocalizationManager.h"

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "nlohmann/json.hpp"

namespace
{
// allow_exceptions=false + is_discarded() instead of a bare parse() call - malformed content should log and
// fail gracefully, not throw a native exception across whatever call boundary (Wren included) sits above this
// - same hardened-parse fix InputConfigLoader already applies to its own JSON loading.
nlohmann::json ParseJsonFile(const std::string& path, const char* what)
{
	auto result = Struktur::FileSystem::ReadString(path);
	if (!result.success)
	{
		DEBUG_ERROR("LOCALIZATION: Failed to read %s: %s - %s", what, path.c_str(), result.errorMessage.c_str());
		return nlohmann::json();
	}

	nlohmann::json json = nlohmann::json::parse(result.value, nullptr, false);
	if (json.is_discarded())
	{
		DEBUG_ERROR("LOCALIZATION: Failed to parse %s (malformed JSON): %s", what, path.c_str());
		return nlohmann::json();
	}
	return json;
}
}  // namespace

namespace Struktur::Localization
{
bool LocalizationManager::LoadFromManifest(const std::string& manifestPath)
{
	nlohmann::json manifest = ParseJsonFile(manifestPath, "localization manifest");
	if (manifest.is_null())
	{
		return false;
	}

	if (!manifest.contains("languages") || !manifest["languages"].is_array())
	{
		DEBUG_ERROR("LOCALIZATION: Manifest missing 'languages' array: %s", manifestPath.c_str());
		return false;
	}

	m_lastManifestPath = manifestPath;

	if (manifest.contains("defaultLanguage") && manifest["defaultLanguage"].is_string())
	{
		m_defaultLanguageCode = manifest["defaultLanguage"].get<std::string>();
	}

	bool anyLoaded = false;
	for (const auto& entry : manifest["languages"])
	{
		if (!entry.contains("code") || !entry.contains("file"))
		{
			DEBUG_WARNING("LOCALIZATION: Manifest entry missing 'code' or 'file' - skipped");
			continue;
		}

		std::string code = entry["code"].get<std::string>();
		std::string file  = entry["file"].get<std::string>();
		anyLoaded |= LoadLanguage(code, file);
	}

	// Only switch the active language once loading is done, and only if that particular language actually
	// loaded - a manifest listing a default language whose file failed to load should leave GetString() still
	// resolving against whatever was active before (or the "en"/en member default if this is the first load),
	// not silently switch to a language with zero strings in it.
	if (m_languages.count(m_defaultLanguageCode))
	{
		m_activeLanguageCode = m_defaultLanguageCode;
	}

	return anyLoaded;
}

bool LocalizationManager::Reload()
{
	if (m_lastManifestPath.empty())
	{
		DEBUG_WARNING("LOCALIZATION: Reload() called before any LoadFromManifest succeeded - ignored");
		return false;
	}

	// Clear first, unlike a plain LoadFromManifest call - a key removed from a JSON file on disk since the last
	// load should disappear here too, not linger from LoadLanguage's own "merge into whatever's already there"
	// semantics.
	m_languages.clear();
	m_warnedMissingKeys.clear();
	return LoadFromManifest(m_lastManifestPath);
}

bool LocalizationManager::LoadLanguage(const std::string& languageCode, const std::string& jsonPath)
{
	nlohmann::json json = ParseJsonFile(jsonPath, "localization file");
	if (json.is_null())
	{
		return false;
	}

	if (!json.contains("strings") || !json["strings"].is_object())
	{
		DEBUG_ERROR("LOCALIZATION: '%s' missing 'strings' object: %s", languageCode.c_str(), jsonPath.c_str());
		return false;
	}

	auto& strings = m_languages[languageCode];
	for (const auto& [key, value] : json["strings"].items())
	{
		if (!value.is_string())
		{
			DEBUG_WARNING("LOCALIZATION: '%s' key '%s' is not a string - skipped", languageCode.c_str(), key.c_str());
			continue;
		}
		strings[key] = value.get<std::string>();
	}

	DEBUG_INFO("LOCALIZATION: Loaded %zu strings for '%s' from %s", strings.size(), languageCode.c_str(),
	          jsonPath.c_str());
	return true;
}

void LocalizationManager::SetActiveLanguage(const std::string& languageCode)
{
	if (!m_languages.count(languageCode))
	{
		DEBUG_WARNING("LOCALIZATION: SetActiveLanguage('%s') - not loaded, keeping '%s' active", languageCode.c_str(),
		             m_activeLanguageCode.c_str());
		return;
	}
	m_activeLanguageCode = languageCode;
}

std::vector<std::string> LocalizationManager::GetAvailableLanguages() const
{
	std::vector<std::string> codes;
	codes.reserve(m_languages.size());
	for (const auto& [code, strings] : m_languages)
	{
		codes.push_back(code);
	}
	return codes;
}

std::vector<std::string> LocalizationManager::GetAllKeys() const
{
	std::set<std::string> uniqueKeys;
	for (const auto& [code, strings] : m_languages)
	{
		for (const auto& [key, value] : strings)
		{
			uniqueKeys.insert(key);
		}
	}
	return std::vector<std::string>(uniqueKeys.begin(), uniqueKeys.end());
}

bool LocalizationManager::TryGetRawString(const std::string& languageCode, const std::string& key,
                                          std::string& outValue) const
{
	auto languageIt = m_languages.find(languageCode);
	if (languageIt == m_languages.end())
	{
		return false;
	}
	auto stringIt = languageIt->second.find(key);
	if (stringIt == languageIt->second.end())
	{
		return false;
	}
	outValue = stringIt->second;
	return true;
}

std::string LocalizationManager::GetString(const std::string& key) const
{
	auto activeIt = m_languages.find(m_activeLanguageCode);
	if (activeIt != m_languages.end())
	{
		auto stringIt = activeIt->second.find(key);
		if (stringIt != activeIt->second.end())
		{
			return stringIt->second;
		}
	}

	// Fall back to the default language before giving up entirely - e.g. a key that's only been translated in
	// English so far still shows real text in every other language, rather than the "[key]" marker.
	if (m_activeLanguageCode != m_defaultLanguageCode)
	{
		auto defaultIt = m_languages.find(m_defaultLanguageCode);
		if (defaultIt != m_languages.end())
		{
			auto stringIt = defaultIt->second.find(key);
			if (stringIt != defaultIt->second.end())
			{
				return stringIt->second;
			}
		}
	}

	if (m_warnedMissingKeys.insert(key).second)
	{
		DEBUG_ERROR("LOCALIZATION: Missing key '%s' (active='%s', default='%s')", key.c_str(),
		           m_activeLanguageCode.c_str(), m_defaultLanguageCode.c_str());
	}
	return "[" + key + "]";
}

bool LocalizationManager::HasString(const std::string& key) const
{
	auto activeIt = m_languages.find(m_activeLanguageCode);
	if (activeIt != m_languages.end() && activeIt->second.count(key))
	{
		return true;
	}
	auto defaultIt = m_languages.find(m_defaultLanguageCode);
	return defaultIt != m_languages.end() && defaultIt->second.count(key) > 0;
}
}  // namespace Struktur::Localization
