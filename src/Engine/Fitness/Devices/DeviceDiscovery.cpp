#include "DeviceDiscovery.h"

#include <algorithm>

namespace Struktur::Fitness
{
static bool Advertises(const std::vector<BleUuid>& services, uint16_t shortUuid)
{
	const BleUuid uuid = BleUuid::FromShort(shortUuid);
	return std::find(services.begin(), services.end(), uuid) != services.end();
}

std::vector<DeviceRole> GetCandidateRoles(const std::vector<BleUuid>& advertisedServices)
{
	std::vector<DeviceRole> roles;
	if (Advertises(advertisedServices, Uuids::FITNESS_MACHINE_SERVICE))
	{
		roles.push_back(DeviceRole::TRAINER);
	}
	if (Advertises(advertisedServices, Uuids::HEART_RATE_SERVICE))
	{
		roles.push_back(DeviceRole::HEART_RATE);
	}
	if (Advertises(advertisedServices, Uuids::CYCLING_POWER_SERVICE))
	{
		roles.push_back(DeviceRole::POWER_METER);
	}
	return roles;
}

std::optional<DeviceRole> GetPrimaryRole(const std::vector<BleUuid>& advertisedServices)
{
	const std::vector<DeviceRole> roles = GetCandidateRoles(advertisedServices);
	return roles.empty() ? std::nullopt : std::optional<DeviceRole>(roles.front());
}

std::vector<BleUuid> GetFitnessScanServices()
{
	return {BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE), BleUuid::FromShort(Uuids::HEART_RATE_SERVICE),
	        BleUuid::FromShort(Uuids::CYCLING_POWER_SERVICE)};
}

bool DiscoveredDevice::HasRole(DeviceRole role) const
{
	return std::find(roles.begin(), roles.end(), role) != roles.end();
}

DeviceScanner::DeviceScanner(IBleCentral& central)
    : m_central(&central),
      m_inbox(std::make_shared<Inbox>(256)),
      m_devices(),
      m_scanning(false)
{
}

DeviceScanner::~DeviceScanner()
{
	Stop();
}

void DeviceScanner::Start()
{
	if (m_scanning)
	{
		return;
	}
	m_scanning = true;
	// Fresh inbox per scan so adverts still in flight from a previous scan are discarded.
	m_inbox                      = std::make_shared<Inbox>(256);
	std::shared_ptr<Inbox> inbox = m_inbox;
	m_central->StartScan(GetFitnessScanServices(),
	                     [inbox](const BleAdvertisement& advertisement) { inbox->Push(advertisement); });
}

void DeviceScanner::Stop()
{
	if (!m_scanning)
	{
		return;
	}
	m_scanning = false;
	m_central->StopScan();
}

bool DeviceScanner::IsScanning() const
{
	return m_scanning;
}

void DeviceScanner::Clear()
{
	m_devices.clear();
}

void DeviceScanner::Update(double now)
{
	if (!m_scanning)
	{
		return;
	}
	for (BleAdvertisement& advertisement : m_inbox->Drain())
	{
		std::vector<DeviceRole> roles = GetCandidateRoles(advertisement.serviceUuids);
		if (roles.empty())
		{
			continue;
		}
		auto existing =
		    std::find_if(m_devices.begin(), m_devices.end(), [&advertisement](const DiscoveredDevice& device)
		                 { return device.address == advertisement.address; });
		if (existing == m_devices.end())
		{
			m_devices.push_back(DiscoveredDevice{});
			existing          = m_devices.end() - 1;
			existing->address = advertisement.address;
		}
		if (!advertisement.name.empty())
		{
			existing->name = advertisement.name;
		}
		existing->rssi            = advertisement.rssi;
		existing->roles           = std::move(roles);
		existing->lastSeenSeconds = now;
	}
}

const std::vector<DiscoveredDevice>& DeviceScanner::GetDevices() const
{
	return m_devices;
}

const DiscoveredDevice* DeviceScanner::FindDevice(BleAddress address) const
{
	for (const DiscoveredDevice& device : m_devices)
	{
		if (device.address == address)
		{
			return &device;
		}
	}
	return nullptr;
}
}  // namespace Struktur::Fitness
