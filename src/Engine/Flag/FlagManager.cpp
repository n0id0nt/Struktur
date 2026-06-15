#include "FlagManager.h"

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "nlohmann/json.hpp"
#include "raylib.h"

namespace Struktur::Flag
{
FlagManager::FlagManager()
    : m_boolFlags(),
      m_intFlags(),
      m_floatFlags(),
      m_stringFlags()
{
}

FlagManager::~FlagManager() {}

//////////////////////////////////////////////////////////////////////////////////////////
// Boolean flags
//////////////////////////////////////////////////////////////////////////////////////////

bool FlagManager::GetFlag(const std::string& name) const
{
	auto it = m_boolFlags.find(name);
	return it != m_boolFlags.end() ? it->second : false;
}

void FlagManager::SetFlag(const std::string& name, bool value)
{
	m_boolFlags[name] = value;
}

bool FlagManager::HasFlag(const std::string& name) const
{
	return m_boolFlags.find(name) != m_boolFlags.end();
}

void FlagManager::RemoveFlag(const std::string& name)
{
	m_boolFlags.erase(name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Integer flags
//////////////////////////////////////////////////////////////////////////////////////////

int FlagManager::GetIntFlag(const std::string& name) const
{
	auto it = m_intFlags.find(name);
	return it != m_intFlags.end() ? it->second : 0;
}

void FlagManager::SetIntFlag(const std::string& name, int value)
{
	m_intFlags[name] = value;
}

bool FlagManager::HasIntFlag(const std::string& name) const
{
	return m_intFlags.find(name) != m_intFlags.end();
}

void FlagManager::RemoveIntFlag(const std::string& name)
{
	m_intFlags.erase(name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Float flags
//////////////////////////////////////////////////////////////////////////////////////////

float FlagManager::GetFloatFlag(const std::string& name) const
{
	auto it = m_floatFlags.find(name);
	return it != m_floatFlags.end() ? it->second : 0.0f;
}

void FlagManager::SetFloatFlag(const std::string& name, float value)
{
	m_floatFlags[name] = value;
}

bool FlagManager::HasFloatFlag(const std::string& name) const
{
	return m_floatFlags.find(name) != m_floatFlags.end();
}

void FlagManager::RemoveFloatFlag(const std::string& name)
{
	m_floatFlags.erase(name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// String flags
//////////////////////////////////////////////////////////////////////////////////////////

std::string FlagManager::GetStringFlag(const std::string& name) const
{
	auto it = m_stringFlags.find(name);
	return it != m_stringFlags.end() ? it->second : "";
}

void FlagManager::SetStringFlag(const std::string& name, const std::string& value)
{
	m_stringFlags[name] = value;
}

bool FlagManager::HasStringFlag(const std::string& name) const
{
	return m_stringFlags.find(name) != m_stringFlags.end();
}

void FlagManager::RemoveStringFlag(const std::string& name)
{
	m_stringFlags.erase(name);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Utility
//////////////////////////////////////////////////////////////////////////////////////////

void FlagManager::Clear()
{
	m_boolFlags.clear();
	m_intFlags.clear();
	m_floatFlags.clear();
	m_stringFlags.clear();
}

void FlagManager::ClearBoolFlags()
{
	m_boolFlags.clear();
}
void FlagManager::ClearIntFlags()
{
	m_intFlags.clear();
}
void FlagManager::ClearFloatFlags()
{
	m_floatFlags.clear();
}
void FlagManager::ClearStringFlags()
{
	m_stringFlags.clear();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Persistence
//////////////////////////////////////////////////////////////////////////////////////////

std::string FlagManager::Serialise() const
{
	nlohmann::json json;

	for (const auto& [key, value] : m_boolFlags)
	{
		json["bool"][key] = value;
	}
	for (const auto& [key, value] : m_intFlags)
	{
		json["int"][key] = value;
	}
	for (const auto& [key, value] : m_floatFlags)
	{
		json["float"][key] = value;
	}
	for (const auto& [key, value] : m_stringFlags)
	{
		json["string"][key] = value;
	}

	return json.dump(4);
}

bool FlagManager::Deserialise(const std::string& data)
{
	nlohmann::json json;
	try
	{
		json = nlohmann::json::parse(data);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		DEBUG_ERROR("FLAGMANAGER: JSON parse error - %s", e.what());
		return false;
	}

	if (json.contains("bool") && json["bool"].is_object())
	{
		for (const auto& [key, value] : json["bool"].items())
		{
			m_boolFlags[key] = value.get<bool>();
		}
	}

	if (json.contains("int") && json["int"].is_object())
	{
		for (const auto& [key, value] : json["int"].items())
		{
			m_intFlags[key] = value.get<int>();
		}
	}

	if (json.contains("float") && json["float"].is_object())
	{
		for (const auto& [key, value] : json["float"].items())
		{
			m_floatFlags[key] = value.get<float>();
		}
	}

	if (json.contains("string") && json["string"].is_object())
	{
		for (const auto& [key, value] : json["string"].items())
		{
			m_stringFlags[key] = value.get<std::string>();
		}
	}

	return true;
}

bool FlagManager::Save(const std::string& filePath) const
{
	std::string data = Serialise();

#ifdef EDITOR
	auto result = FileSystem::WriteString(filePath, data);
#else
	auto result = FileSystem::WriteEncrypted(filePath, data);
#endif

	if (!result)
	{
		DEBUG_ERROR("FLAGMANAGER: Failed to save flags to %s - %s", filePath.c_str(), result.errorMessage.c_str());
		return false;
	}

	FileSystem::SyncSaves();
	DEBUG_INFO("FLAGMANAGER: Saved flags to %s", filePath.c_str());
	return true;
}

bool FlagManager::Load(const std::string& filePath)
{
	if (!FileSystem::Exists(filePath))
	{
		DEBUG_INFO("FLAGMANAGER: No save found at %s, using defaults", filePath.c_str());
		return false;
	}

#ifdef EDITOR
	auto result = FileSystem::ReadString(filePath);
#else
	auto result = FileSystem::ReadEncrypted(filePath);
#endif

	if (!result)
	{
		DEBUG_ERROR("FLAGMANAGER: Failed to load flags from %s - %s", filePath.c_str(), result.errorMessage.c_str());
		return false;
	}

	if (!Deserialise(result.value))
	{
		DEBUG_ERROR("FLAGMANAGER: Failed to deserialise flags from %s", filePath.c_str());
		return false;
	}

	DEBUG_INFO("FLAGMANAGER: Loaded flags from %s", filePath.c_str());
	return true;
}
}  // namespace Struktur::Flag
