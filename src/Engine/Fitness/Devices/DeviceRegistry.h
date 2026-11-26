#pragma once

#include <map>
#include <optional>
#include <string>

#include "Engine/Fitness/Devices/DeviceTypes.h"

namespace Struktur::Fitness
{
struct RememberedDevice
{
	std::string identifier;  // transport address, e.g. "AA:BB:CC:DD:EE:FF"
	std::string name;
	std::string transport = "ble";  // room for "ant" later
};

// Which physical device the player chose for each role, persisted as a small JSON file so the game reconnects to
// the same trainer / strap / power meter next session without scanning UI.
//
//   {"version": 1, "devices": {"trainer": {"transport": "ble", "identifier": "...", "name": "..."}, ...}}
class DeviceRegistry
{
public:
	void Remember(DeviceRole role, RememberedDevice device);
	void Forget(DeviceRole role);
	std::optional<RememberedDevice> Get(DeviceRole role) const;
	const std::map<DeviceRole, RememberedDevice>& GetAll() const;

	std::string Serialize() const;
	// Replaces the current contents. Returns false (and leaves contents untouched) on malformed input.
	bool Deserialize(const std::string& json);

	bool LoadFromFile(const std::string& path);
	bool SaveToFile(const std::string& path) const;

private:
	std::map<DeviceRole, RememberedDevice> m_devices;
};
}  // namespace Struktur::Fitness
