#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Fitness/Ble/IBleCentral.h"
#include "Engine/Fitness/Core/ThreadSafeQueue.h"
#include "Engine/Fitness/Devices/DeviceTypes.h"

namespace Struktur::Fitness
{
// Role assignment by advertised service UUID:
//   0x1826 Fitness Machine  -> TRAINER
//   0x180D Heart Rate       -> HEART_RATE
//   0x1818 Cycling Power    -> POWER_METER
// Returned in priority order: a trainer that also advertises Cycling Power (both test trainers do) is primarily a
// TRAINER but can still be picked as a POWER_METER.
std::vector<DeviceRole> GetCandidateRoles(const std::vector<BleUuid>& advertisedServices);
std::optional<DeviceRole> GetPrimaryRole(const std::vector<BleUuid>& advertisedServices);
// The service UUIDs to scan for.
std::vector<BleUuid> GetFitnessScanServices();

struct DiscoveredDevice
{
	BleAddress address = 0;
	std::string name;
	int16_t rssi = 0;
	std::vector<DeviceRole> roles;
	double lastSeenSeconds = 0.0;

	bool HasRole(DeviceRole role) const;
};

// Scans for fitness devices and keeps a de-duplicated list (one entry per address, refreshed by every advert).
class DeviceScanner
{
public:
	explicit DeviceScanner(IBleCentral& central);
	~DeviceScanner();

	DeviceScanner(const DeviceScanner&)            = delete;
	DeviceScanner& operator=(const DeviceScanner&) = delete;

	void Start();
	void Stop();
	bool IsScanning() const;
	void Clear();

	void Update(double now);

	const std::vector<DiscoveredDevice>& GetDevices() const;
	const DiscoveredDevice* FindDevice(BleAddress address) const;

private:
	using Inbox = ThreadSafeQueue<BleAdvertisement>;

	IBleCentral* m_central;
	std::shared_ptr<Inbox> m_inbox;
	std::vector<DiscoveredDevice> m_devices;
	bool m_scanning;
};
}  // namespace Struktur::Fitness
