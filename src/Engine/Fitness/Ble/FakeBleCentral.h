#pragma once

#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "Engine/Fitness/Ble/IBleCentral.h"

namespace Struktur::Fitness
{
// In-memory IBleCentral for unit tests and trainer_probe --simulate. Single-threaded: every callback is deferred
// into a pending list and only runs inside Pump(), which gives tests full control over ordering (and mirrors the
// real backends, where callbacks arrive "later" on another thread).
class FakeBleCentral : public IBleCentral
{
public:
	using WriteHandler = std::function<BleStatus(const BleCharacteristicRef&, const Bytes&)>;

	struct Peripheral
	{
		BleAdvertisement advertisement;
		std::vector<BleServiceInfo> services;
		std::map<BleCharacteristicRef, Bytes> values;
		// Runs (inside Pump) for every write; may call Notify() to answer. nullptr = accept every write.
		WriteHandler onWrite;
		bool inRange                 = true;
		int connectFailuresRemaining = 0;  // next N Connect() calls fail with UNREACHABLE
		int maxConnections           = 1;  // Direto = 1, KICKR = 3
		int otherCentralConnections  = 0;  // links held by "other apps" - counts against maxConnections
	};

	struct WriteRecord
	{
		BleAddress address = 0;
		BleCharacteristicRef characteristic;
		Bytes value;
		bool withResponse = true;
	};

	Peripheral& AddPeripheral(BleAddress address, const std::string& name, std::vector<BleUuid> advertisedServices);
	Peripheral* FindPeripheral(BleAddress address);
	void AddCharacteristic(BleAddress address, const BleCharacteristicRef& characteristic, uint32_t properties,
	                       Bytes initialValue = {});

	// Sends a notification/indication to the subscriber, if any, when Pump() reaches it.
	void Notify(BleAddress address, const BleCharacteristicRef& characteristic, Bytes value);
	// Simulates link loss: subscriptions vanish and onDisconnected fires.
	void DropConnection(BleAddress address);
	// Re-delivers advertisements for every in-range peripheral matching the current scan.
	void AdvertiseAll();
	void SetAvailable(bool available);

	// Runs deferred callbacks (including any they queue) until none are left. Returns how many ran.
	size_t Pump();

	bool IsConnected(BleAddress address) const;
	bool IsSubscribed(BleAddress address, const BleCharacteristicRef& characteristic) const;
	int GetConnectAttempts(BleAddress address) const;
	size_t GetActiveConnectionCount() const;
	bool IsScanning() const;
	const std::vector<WriteRecord>& GetWrites() const;

	// IBleCentral
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
	struct Link
	{
		DisconnectCallback onDisconnected;
		std::map<BleCharacteristicRef, ValueCallback> subscriptions;
	};

	const BleCharacteristicInfo* FindCharacteristic(BleAddress address,
	                                                const BleCharacteristicRef& characteristic) const;
	bool MatchesScanFilter(const Peripheral& peripheral) const;
	void Defer(std::function<void()> job);

	bool m_available = true;
	bool m_scanning  = false;
	std::vector<BleUuid> m_scanFilter;
	AdvertisementCallback m_onAdvertisement;
	std::map<BleAddress, Peripheral> m_peripherals;
	std::map<BleAddress, Link> m_links;
	std::map<BleAddress, int> m_connectAttempts;
	std::vector<WriteRecord> m_writes;
	std::deque<std::function<void()>> m_pending;
};
}  // namespace Struktur::Fitness
