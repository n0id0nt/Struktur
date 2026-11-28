#include "DeviceRegistry.h"

#include <fstream>
#include <sstream>

#include "Debug/Assertions.h"
#include "nlohmann/json.hpp"

namespace Struktur::Fitness
{
static constexpr int REGISTRY_VERSION = 1;

static std::optional<DeviceRole> RoleFromString(const std::string& text)
{
	for (DeviceRole role : {DeviceRole::TRAINER, DeviceRole::HEART_RATE, DeviceRole::POWER_METER})
	{
		if (text == ToString(role))
		{
			return role;
		}
	}
	return std::nullopt;
}

void DeviceRegistry::Remember(DeviceRole role, RememberedDevice device)
{
	m_devices[role] = std::move(device);
}

void DeviceRegistry::Forget(DeviceRole role)
{
	m_devices.erase(role);
}

std::optional<RememberedDevice> DeviceRegistry::Get(DeviceRole role) const
{
	auto found = m_devices.find(role);
	return found == m_devices.end() ? std::nullopt : std::optional<RememberedDevice>(found->second);
}

const std::map<DeviceRole, RememberedDevice>& DeviceRegistry::GetAll() const
{
	return m_devices;
}

std::string DeviceRegistry::Serialize() const
{
	nlohmann::json root;
	root["version"] = REGISTRY_VERSION;
	root["devices"] = nlohmann::json::object();
	for (const auto& [role, device] : m_devices)
	{
		root["devices"][ToString(role)] = {{"transport", device.transport},
		                                   {"identifier", device.identifier},
		                                   {"name", device.name}};
	}
	return root.dump(2);
}

bool DeviceRegistry::Deserialize(const std::string& json)
{
	const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
	if (root.is_discarded() || !root.is_object() || !root.contains("devices") || !root["devices"].is_object())
	{
		DEBUG_WARNING("Fitness device registry is malformed - ignoring it");
		return false;
	}
	std::map<DeviceRole, RememberedDevice> devices;
	for (const auto& [key, value] : root["devices"].items())
	{
		const std::optional<DeviceRole> role = RoleFromString(key);
		if (!role || !value.is_object() || !value.contains("identifier") || !value["identifier"].is_string())
		{
			continue;
		}
		RememberedDevice device;
		device.identifier = value["identifier"].get<std::string>();
		device.name       = value.value("name", std::string());
		device.transport  = value.value("transport", std::string("ble"));
		devices[*role]    = std::move(device);
	}
	m_devices = std::move(devices);
	return true;
}

bool DeviceRegistry::LoadFromFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file)
	{
		return false;
	}
	std::stringstream contents;
	contents << file.rdbuf();
	return Deserialize(contents.str());
}

bool DeviceRegistry::SaveToFile(const std::string& path) const
{
	std::ofstream file(path, std::ios::trunc);
	if (!file)
	{
		DEBUG_ERROR("Could not write fitness device registry to '%s'", path.c_str());
		return false;
	}
	file << Serialize();
	return static_cast<bool>(file);
}
}  // namespace Struktur::Fitness
