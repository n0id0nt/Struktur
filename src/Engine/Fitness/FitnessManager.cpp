#include "FitnessManager.h"

#include <chrono>
#include <thread>

#include "Debug/Assertions.h"
#include "Engine/Fitness/Devices/BleSensors.h"

namespace Struktur::Fitness
{
static constexpr DeviceRole ALL_ROLES[] = {DeviceRole::TRAINER, DeviceRole::HEART_RATE, DeviceRole::POWER_METER};

FitnessManager::FitnessManager(std::unique_ptr<IBleCentral> central, FitnessSettings settings)
    : m_central(std::move(central)),
      m_settings(std::move(settings)),
      m_scanner(*m_central),
      m_registry(),
      m_trainer(),
      m_heartRate(),
      m_powerMeter(),
      m_events(),
      m_lastNowSeconds(0.0),
      m_shutDown(false)
{
	ASSERT_MSG(m_central != nullptr, "FitnessManager needs a BLE central");
}

FitnessManager::~FitnessManager()
{
	Shutdown();
}

bool FitnessManager::IsBluetoothAvailable() const
{
	return m_central->IsAvailable();
}

void FitnessManager::StartScan()
{
	m_scanner.Start();
}

void FitnessManager::StopScan()
{
	m_scanner.Stop();
}

bool FitnessManager::IsScanning() const
{
	return m_scanner.IsScanning();
}

const std::vector<DiscoveredDevice>& FitnessManager::GetDiscoveredDevices() const
{
	return m_scanner.GetDevices();
}

bool FitnessManager::AssignDevice(DeviceRole role, BleAddress address, const std::string& name, double now)
{
	const std::string identifier = FormatAddress(address);
	for (DeviceRole other : ALL_ROLES)
	{
		Device* device = GetDevice(other);
		if (other != role && device && device->GetIdentifier() == identifier)
		{
			DEBUG_ERROR("%s is already connected as the %s - one role per device", identifier.c_str(), ToString(other));
			return false;
		}
	}

	m_lastNowSeconds = now;
	StopAndRelease(role);
	std::unique_ptr<Device>& slot = Slot(role);
	slot                          = CreateDevice(role, address, name);
	slot->Start(now);
	m_shutDown = false;

	m_registry.Remember(role, RememberedDevice{identifier, name, "ble"});
	SaveRegistry();
	return true;
}

void FitnessManager::UnassignDevice(DeviceRole role, bool forget)
{
	StopAndRelease(role);
	if (forget)
	{
		m_registry.Forget(role);
		SaveRegistry();
	}
}

void FitnessManager::StopAndRelease(DeviceRole role)
{
	std::unique_ptr<Device>& slot = Slot(role);
	if (!slot)
	{
		return;
	}
	// Bounded graceful stop that keeps the game's clock running forward (lets a trainer release its load first).
	const auto start  = std::chrono::steady_clock::now();
	const double base = m_lastNowSeconds;
	auto now          = [&start, base]()
	{ return base + std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };
	slot->Stop(now());
	while (!slot->IsStopped() && now() - base < m_settings.trainer.shutdownTimeoutSeconds + 0.5)
	{
		m_central->Poll();
		slot->Update(now());
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	for (DeviceEvent& event : slot->TakeEvents())
	{
		m_events.push_back(std::move(event));
	}
	slot.reset();
}

void FitnessManager::ConnectRememberedDevices(double now)
{
	for (const auto& [role, remembered] : m_registry.GetAll())
	{
		if (remembered.transport != "ble" || Slot(role))
		{
			continue;
		}
		BleAddress address = 0;
		if (!ParseAddress(remembered.identifier, address))
		{
			DEBUG_WARNING("Remembered %s has an invalid address '%s'", ToString(role), remembered.identifier.c_str());
			continue;
		}
		m_lastNowSeconds = now;
		Slot(role)       = CreateDevice(role, address, remembered.name);
		Slot(role)->Start(now);
		m_shutDown = false;
	}
}

bool FitnessManager::LoadRegistry()
{
	return !m_settings.registryPath.empty() && m_registry.LoadFromFile(m_settings.registryPath);
}

bool FitnessManager::SaveRegistry() const
{
	return !m_settings.registryPath.empty() && m_registry.SaveToFile(m_settings.registryPath);
}

DeviceRegistry& FitnessManager::GetRegistry()
{
	return m_registry;
}

void FitnessManager::Update(double now)
{
	m_lastNowSeconds = now;
	m_central->Poll();
	m_scanner.Update(now);
	for (DeviceRole role : ALL_ROLES)
	{
		std::unique_ptr<Device>& slot = Slot(role);
		if (!slot)
		{
			continue;
		}
		slot->Update(now);
		for (DeviceEvent& event : slot->TakeEvents())
		{
			m_events.push_back(std::move(event));
		}
	}
}

std::vector<DeviceEvent> FitnessManager::TakeEvents()
{
	return std::exchange(m_events, {});
}

Trainer* FitnessManager::GetTrainer() const
{
	return static_cast<Trainer*>(m_trainer.get());
}

HeartRateSensor* FitnessManager::GetHeartRateSensor() const
{
	return static_cast<HeartRateSensor*>(m_heartRate.get());
}

PowerMeter* FitnessManager::GetPowerMeter() const
{
	return static_cast<PowerMeter*>(m_powerMeter.get());
}

Device* FitnessManager::GetDevice(DeviceRole role) const
{
	switch (role)
	{
		case DeviceRole::TRAINER:
			return m_trainer.get();
		case DeviceRole::HEART_RATE:
			return m_heartRate.get();
		case DeviceRole::POWER_METER:
			return m_powerMeter.get();
	}
	return nullptr;
}

void FitnessManager::Shutdown(double timeoutSeconds)
{
	if (m_shutDown)
	{
		return;
	}
	m_shutDown = true;
	m_scanner.Stop();

	const auto start  = std::chrono::steady_clock::now();
	const double base = m_lastNowSeconds;
	auto now          = [&start, base]()
	{ return base + std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };
	for (DeviceRole role : ALL_ROLES)
	{
		if (Slot(role))
		{
			Slot(role)->Stop(now());
		}
	}
	while (now() - base < timeoutSeconds)
	{
		bool allStopped = true;
		m_central->Poll();
		for (DeviceRole role : ALL_ROLES)
		{
			std::unique_ptr<Device>& slot = Slot(role);
			if (slot && !slot->IsStopped())
			{
				slot->Update(now());
				allStopped = allStopped && slot->IsStopped();
			}
		}
		if (allStopped)
		{
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	// Destroying a device drops its link even if the graceful handshake did not finish in time.
	m_trainer.reset();
	m_heartRate.reset();
	m_powerMeter.reset();
}

IBleCentral& FitnessManager::GetCentral()
{
	return *m_central;
}

std::unique_ptr<Device> FitnessManager::CreateDevice(DeviceRole role, BleAddress address, const std::string& name)
{
	switch (role)
	{
		case DeviceRole::TRAINER:
			return std::make_unique<FtmsTrainer>(*m_central, address, name, m_settings.trainer);
		case DeviceRole::HEART_RATE:
			return std::make_unique<BleHeartRateSensor>(*m_central, address, name, m_settings.sensorReconnect);
		case DeviceRole::POWER_METER:
			return std::make_unique<BlePowerMeter>(*m_central, address, name, m_settings.sensorReconnect);
	}
	return nullptr;
}

std::unique_ptr<Device>& FitnessManager::Slot(DeviceRole role)
{
	switch (role)
	{
		case DeviceRole::TRAINER:
			return m_trainer;
		case DeviceRole::HEART_RATE:
			return m_heartRate;
		case DeviceRole::POWER_METER:
			break;
	}
	return m_powerMeter;
}
}  // namespace Struktur::Fitness
