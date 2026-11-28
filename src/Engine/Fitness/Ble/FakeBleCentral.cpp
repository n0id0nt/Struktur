#include "FakeBleCentral.h"

#include <algorithm>

namespace Struktur::Fitness
{
FakeBleCentral::Peripheral& FakeBleCentral::AddPeripheral(BleAddress address, const std::string& name,
                                                          std::vector<BleUuid> advertisedServices)
{
	Peripheral& peripheral                = m_peripherals[address];
	peripheral.advertisement.address      = address;
	peripheral.advertisement.name         = name;
	peripheral.advertisement.rssi         = -55;
	peripheral.advertisement.serviceUuids = std::move(advertisedServices);
	return peripheral;
}

FakeBleCentral::Peripheral* FakeBleCentral::FindPeripheral(BleAddress address)
{
	auto found = m_peripherals.find(address);
	return found == m_peripherals.end() ? nullptr : &found->second;
}

void FakeBleCentral::AddCharacteristic(BleAddress address, const BleCharacteristicRef& characteristic,
                                       uint32_t properties, Bytes initialValue)
{
	Peripheral* peripheral = FindPeripheral(address);
	if (!peripheral)
	{
		return;
	}
	auto service =
	    std::find_if(peripheral->services.begin(), peripheral->services.end(),
	                 [&characteristic](const BleServiceInfo& info) { return info.uuid == characteristic.service; });
	if (service == peripheral->services.end())
	{
		peripheral->services.push_back(BleServiceInfo{characteristic.service, {}});
		service = peripheral->services.end() - 1;
	}
	service->characteristics.push_back(BleCharacteristicInfo{characteristic.characteristic, properties});
	peripheral->values[characteristic] = std::move(initialValue);
}

void FakeBleCentral::Notify(BleAddress address, const BleCharacteristicRef& characteristic, Bytes value)
{
	Defer(
	    [this, address, characteristic, value = std::move(value)]()
	    {
		    auto link = m_links.find(address);
		    if (link == m_links.end())
		    {
			    return;
		    }
		    auto subscription = link->second.subscriptions.find(characteristic);
		    if (subscription != link->second.subscriptions.end())
		    {
			    subscription->second(value);
		    }
	    });
}

void FakeBleCentral::DropConnection(BleAddress address)
{
	auto link = m_links.find(address);
	if (link == m_links.end())
	{
		return;
	}
	DisconnectCallback onDisconnected = std::move(link->second.onDisconnected);
	m_links.erase(link);
	if (onDisconnected)
	{
		Defer(std::move(onDisconnected));
	}
}

void FakeBleCentral::AdvertiseAll()
{
	if (!m_scanning || !m_available)
	{
		return;
	}
	for (const auto& [address, peripheral] : m_peripherals)
	{
		if (peripheral.inRange && MatchesScanFilter(peripheral))
		{
			Defer(
			    [this, advertisement = peripheral.advertisement]()
			    {
				    if (m_scanning && m_onAdvertisement)
				    {
					    m_onAdvertisement(advertisement);
				    }
			    });
		}
	}
}

void FakeBleCentral::SetAvailable(bool available)
{
	m_available = available;
}

size_t FakeBleCentral::Pump()
{
	size_t ran = 0;
	while (!m_pending.empty())
	{
		std::function<void()> job = std::move(m_pending.front());
		m_pending.pop_front();
		job();
		++ran;
	}
	return ran;
}

bool FakeBleCentral::IsConnected(BleAddress address) const
{
	return m_links.count(address) != 0;
}

bool FakeBleCentral::IsSubscribed(BleAddress address, const BleCharacteristicRef& characteristic) const
{
	auto link = m_links.find(address);
	return link != m_links.end() && link->second.subscriptions.count(characteristic) != 0;
}

int FakeBleCentral::GetConnectAttempts(BleAddress address) const
{
	auto found = m_connectAttempts.find(address);
	return found == m_connectAttempts.end() ? 0 : found->second;
}

size_t FakeBleCentral::GetActiveConnectionCount() const
{
	return m_links.size();
}

bool FakeBleCentral::IsScanning() const
{
	return m_scanning;
}

const std::vector<FakeBleCentral::WriteRecord>& FakeBleCentral::GetWrites() const
{
	return m_writes;
}

bool FakeBleCentral::IsAvailable() const
{
	return m_available;
}

void FakeBleCentral::StartScan(std::vector<BleUuid> serviceFilter, AdvertisementCallback onAdvertisement)
{
	m_scanning        = true;
	m_scanFilter      = std::move(serviceFilter);
	m_onAdvertisement = std::move(onAdvertisement);
	AdvertiseAll();
}

void FakeBleCentral::StopScan()
{
	m_scanning        = false;
	m_onAdvertisement = nullptr;
}

void FakeBleCentral::Connect(BleAddress address, DisconnectCallback onDisconnected, StatusCallback onDone)
{
	++m_connectAttempts[address];
	BleStatus status       = BleStatus::SUCCESS;
	Peripheral* peripheral = FindPeripheral(address);
	if (!m_available)
	{
		status = BleStatus::NOT_AVAILABLE;
	}
	else if (!peripheral || !peripheral->inRange)
	{
		status = BleStatus::UNREACHABLE;
	}
	else if (peripheral->connectFailuresRemaining > 0)
	{
		--peripheral->connectFailuresRemaining;
		status = BleStatus::UNREACHABLE;
	}
	else if (!IsConnected(address) && peripheral->otherCentralConnections >= peripheral->maxConnections)
	{
		// The peripheral has no free connection slot (e.g. a Direto already held by another app).
		status = BleStatus::UNREACHABLE;
	}

	if (status == BleStatus::SUCCESS)
	{
		m_links[address].onDisconnected = std::move(onDisconnected);
	}
	Defer([onDone = std::move(onDone), status]() { onDone(status); });
}

void FakeBleCentral::Disconnect(BleAddress address)
{
	m_links.erase(address);
}

void FakeBleCentral::DiscoverServices(BleAddress address, DiscoverCallback onDone)
{
	if (!IsConnected(address))
	{
		Defer([onDone = std::move(onDone)]() { onDone(BleStatus::NOT_CONNECTED, {}); });
		return;
	}
	std::vector<BleServiceInfo> services = m_peripherals[address].services;
	Defer([onDone = std::move(onDone), services = std::move(services)]() { onDone(BleStatus::SUCCESS, services); });
}

void FakeBleCentral::Read(BleAddress address, const BleCharacteristicRef& characteristic, ReadCallback onDone)
{
	BleStatus status = BleStatus::SUCCESS;
	Bytes value;
	const BleCharacteristicInfo* info = FindCharacteristic(address, characteristic);
	if (!IsConnected(address))
	{
		status = BleStatus::NOT_CONNECTED;
	}
	else if (!info)
	{
		status = BleStatus::NOT_FOUND;
	}
	else if ((info->properties & CharacteristicProperty::READ) == 0)
	{
		status = BleStatus::PROTOCOL_ERROR;
	}
	else
	{
		value = m_peripherals[address].values[characteristic];
	}
	Defer([onDone = std::move(onDone), status, value = std::move(value)]() { onDone(status, value); });
}

void FakeBleCentral::Subscribe(BleAddress address, const BleCharacteristicRef& characteristic, SubscriptionKind kind,
                               ValueCallback onValue, StatusCallback onDone)
{
	BleStatus status                  = BleStatus::SUCCESS;
	const BleCharacteristicInfo* info = FindCharacteristic(address, characteristic);
	const uint32_t requiredProperty =
	    kind == SubscriptionKind::NOTIFY ? CharacteristicProperty::NOTIFY : CharacteristicProperty::INDICATE;
	if (!IsConnected(address))
	{
		status = BleStatus::NOT_CONNECTED;
	}
	else if (!info)
	{
		status = BleStatus::NOT_FOUND;
	}
	else if ((info->properties & requiredProperty) == 0)
	{
		status = BleStatus::PROTOCOL_ERROR;
	}
	else
	{
		m_links[address].subscriptions[characteristic] = std::move(onValue);
	}
	Defer([onDone = std::move(onDone), status]() { onDone(status); });
}

void FakeBleCentral::Write(BleAddress address, const BleCharacteristicRef& characteristic, Bytes value,
                           bool withResponse, StatusCallback onDone)
{
	const BleCharacteristicInfo* info = FindCharacteristic(address, characteristic);
	if (!IsConnected(address) || !info)
	{
		const BleStatus status = !IsConnected(address) ? BleStatus::NOT_CONNECTED : BleStatus::NOT_FOUND;
		Defer([onDone = std::move(onDone), status]() { onDone(status); });
		return;
	}
	m_writes.push_back(WriteRecord{address, characteristic, value, withResponse});

	// The handler runs when Pump() reaches this job, so any Notify() it makes lands AFTER the write response -
	// the order a real peripheral produces (write response, then indication).
	Defer(
	    [this, address, characteristic, value = std::move(value), onDone = std::move(onDone)]()
	    {
		    BleStatus status       = BleStatus::SUCCESS;
		    Peripheral* peripheral = FindPeripheral(address);
		    if (peripheral && peripheral->onWrite)
		    {
			    status = peripheral->onWrite(characteristic, value);
		    }
		    onDone(status);
	    });
}

const BleCharacteristicInfo* FakeBleCentral::FindCharacteristic(BleAddress address,
                                                                const BleCharacteristicRef& characteristic) const
{
	auto peripheral = m_peripherals.find(address);
	if (peripheral == m_peripherals.end())
	{
		return nullptr;
	}
	for (const BleServiceInfo& service : peripheral->second.services)
	{
		if (service.uuid != characteristic.service)
		{
			continue;
		}
		for (const BleCharacteristicInfo& info : service.characteristics)
		{
			if (info.uuid == characteristic.characteristic)
			{
				return &info;
			}
		}
	}
	return nullptr;
}

bool FakeBleCentral::MatchesScanFilter(const Peripheral& peripheral) const
{
	if (m_scanFilter.empty())
	{
		return true;
	}
	for (const BleUuid& uuid : peripheral.advertisement.serviceUuids)
	{
		if (std::find(m_scanFilter.begin(), m_scanFilter.end(), uuid) != m_scanFilter.end())
		{
			return true;
		}
	}
	return false;
}

void FakeBleCentral::Defer(std::function<void()> job)
{
	m_pending.push_back(std::move(job));
}
}  // namespace Struktur::Fitness
