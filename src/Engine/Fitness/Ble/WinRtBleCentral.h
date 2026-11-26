#pragma once

#include <memory>

#include "Engine/Fitness/Ble/IBleCentral.h"

namespace Struktur::Fitness
{
// Windows backend over the WinRT Bluetooth LE APIs (Windows.Devices.Bluetooth*), via C++/WinRT from the Windows SDK.
// See docs/SmartTrainerLibraries.md for why this was chosen over a cross-platform wrapper.
//
// All GATT operations run one at a time on a private worker thread (blocking .get() on a multi-threaded apartment),
// which serialises GATT traffic per process and keeps the WinRT headers out of every other translation unit.
// Advertisement, value-changed and connection-status callbacks arrive on WinRT thread-pool threads.
class WinRtBleCentral : public IBleCentral
{
public:
	WinRtBleCentral();
	// Stops scanning, closes every link (unsubscribe, GattSession released, services/device closed) and joins the
	// worker. Windows tears the radio link down shortly after the last handle closes.
	~WinRtBleCentral() override;

	WinRtBleCentral(const WinRtBleCentral&)            = delete;
	WinRtBleCentral& operator=(const WinRtBleCentral&) = delete;

	bool IsAvailable() const override;
	void StartScan(std::vector<BleUuid> serviceFilter, AdvertisementCallback onAdvertisement) override;
	void StopScan() override;
	void Connect(BleAddress address, DisconnectCallback onDisconnected, StatusCallback onDone) override;
	void Disconnect(BleAddress address) override;
	void DiscoverServices(BleAddress address, DiscoverCallback onDone) override;
	void Read(BleAddress address, const BleCharacteristicRef& characteristic, ReadCallback onDone) override;
	void Subscribe(BleAddress address, const BleCharacteristicRef& characteristic, SubscriptionKind kind,
	               ValueCallback onValue, StatusCallback onDone) override;
	void Write(BleAddress address, const BleCharacteristicRef& characteristic, Bytes value, bool withResponse,
	           StatusCallback onDone) override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
}  // namespace Struktur::Fitness
