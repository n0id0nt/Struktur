#pragma once

#include "Engine/Fitness/Ble/IBleCentral.h"

namespace Struktur::Fitness
{
// Backend for platforms with no BLE implementation (web, Linux and macOS for now). Every operation completes
// immediately with NOT_AVAILABLE, so the device layer degrades to "no devices" without special cases.
class NullBleCentral : public IBleCentral
{
public:
	bool IsAvailable() const override
	{
		return false;
	}

	void StartScan(std::vector<BleUuid>, AdvertisementCallback) override {}
	void StopScan() override {}

	void Connect(BleAddress, DisconnectCallback, StatusCallback onDone) override
	{
		onDone(BleStatus::NOT_AVAILABLE);
	}
	void Disconnect(BleAddress) override {}

	void DiscoverServices(BleAddress, DiscoverCallback onDone) override
	{
		onDone(BleStatus::NOT_AVAILABLE, {});
	}
	void Read(BleAddress, const BleCharacteristicRef&, ReadCallback onDone) override
	{
		onDone(BleStatus::NOT_AVAILABLE, {});
	}
	void Subscribe(BleAddress, const BleCharacteristicRef&, SubscriptionKind, ValueCallback,
	               StatusCallback onDone) override
	{
		onDone(BleStatus::NOT_AVAILABLE);
	}
	void Write(BleAddress, const BleCharacteristicRef&, Bytes, bool, StatusCallback onDone) override
	{
		onDone(BleStatus::NOT_AVAILABLE);
	}
};
}  // namespace Struktur::Fitness
