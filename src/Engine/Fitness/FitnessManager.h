#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Engine/Fitness/Ble/IBleCentral.h"
#include "Engine/Fitness/Devices/DeviceDiscovery.h"
#include "Engine/Fitness/Devices/DeviceRegistry.h"
#include "Engine/Fitness/Devices/DeviceTypes.h"
#include "Engine/Fitness/Devices/FtmsTrainer.h"

namespace Struktur::Fitness
{
struct FitnessSettings
{
	FtmsTrainerSettings trainer;
	ReconnectPolicy sensorReconnect;
	// Where chosen devices are remembered. Empty = in memory only.
	std::string registryPath;
};

// Owns the BLE central, the scanner and up to one device per role. Game-thread only: call Update(now) every frame,
// then read snapshots from the devices and/or TakeEvents(). Nothing here starts on its own - the game decides when
// to scan or connect, so merely launching the game never touches Bluetooth.
class FitnessManager
{
public:
	explicit FitnessManager(std::unique_ptr<IBleCentral> central, FitnessSettings settings = {});
	// Calls Shutdown() - always releases the BLE links (a Direto accepts only one connection at a time).
	~FitnessManager();

	FitnessManager(const FitnessManager&)            = delete;
	FitnessManager& operator=(const FitnessManager&) = delete;

	bool IsBluetoothAvailable() const;

	void StartScan();
	void StopScan();
	bool IsScanning() const;
	const std::vector<DiscoveredDevice>& GetDiscoveredDevices() const;

	// Creates (replacing any previous device in that role), starts and remembers the device. Fails if the address
	// already serves another role - one BLE link per peripheral.
	bool AssignDevice(DeviceRole role, BleAddress address, const std::string& name, double now);
	void UnassignDevice(DeviceRole role, bool forget);
	// Starts every role that has a remembered device and no live one.
	void ConnectRememberedDevices(double now);

	// Where chosen devices are remembered (overrides FitnessSettings::registryPath). Empty = in memory only.
	void SetRegistryPath(const std::string& path);
	bool LoadRegistry();
	bool SaveRegistry() const;
	DeviceRegistry& GetRegistry();

	void Update(double now);
	std::vector<DeviceEvent> TakeEvents();

	Trainer* GetTrainer() const;
	HeartRateSensor* GetHeartRateSensor() const;
	PowerMeter* GetPowerMeter() const;
	Device* GetDevice(DeviceRole role) const;

	// Graceful stop of every device (the trainer sends Reset first), pumping with a real clock for at most
	// timeoutSeconds, then releases everything. Idempotent.
	void Shutdown(double timeoutSeconds = 2.5);

	IBleCentral& GetCentral();

private:
	std::unique_ptr<Device> CreateDevice(DeviceRole role, BleAddress address, const std::string& name);
	std::unique_ptr<Device>& Slot(DeviceRole role);
	void StopAndRelease(DeviceRole role);

	std::unique_ptr<IBleCentral> m_central;  // declared first: destroyed last, after every link that uses it
	FitnessSettings m_settings;
	DeviceScanner m_scanner;
	DeviceRegistry m_registry;
	std::unique_ptr<Device> m_trainer;
	std::unique_ptr<Device> m_heartRate;
	std::unique_ptr<Device> m_powerMeter;
	std::vector<DeviceEvent> m_events;
	double m_lastNowSeconds;  // last game time seen, so blocking stop loops continue the same clock
	bool m_shutDown;
};
}  // namespace Struktur::Fitness
