#pragma once

#include <string>

#include "Engine/Fitness/Ble/IBleCentral.h"
#include "Engine/Fitness/Devices/BleLink.h"
#include "Engine/Fitness/Devices/DeviceTypes.h"
#include "Engine/Fitness/Protocol/CadenceCalculator.h"

namespace Struktur::Fitness
{
// Any BLE heart rate monitor: subscribes to Heart Rate Measurement (0x2A37) on service 0x180D.
class BleHeartRateSensor : public HeartRateSensor
{
public:
	BleHeartRateSensor(IBleCentral& central, BleAddress address, std::string name,
	                   const ReconnectPolicy& reconnect = ReconnectPolicy());

	std::string GetIdentifier() const override;
	const std::string& GetName() const override;
	ConnectionState GetConnectionState() const override;
	void Start(double now) override;
	void Update(double now) override;
	void Stop(double now) override;
	bool IsStopped() const override;
	const HeartRateSample& GetLatestSample() const override;

private:
	BleLink m_link;
	std::string m_name;
	HeartRateSample m_sample;
	bool m_stopped;
};

// Any BLE power meter (Cycling Power Service 0x1818): power, pedal balance, and cadence derived from crank
// revolution deltas. Cadence decays to 0 after a few seconds without a new crank event.
class BlePowerMeter : public PowerMeter
{
public:
	BlePowerMeter(IBleCentral& central, BleAddress address, std::string name,
	              const ReconnectPolicy& reconnect = ReconnectPolicy());

	std::string GetIdentifier() const override;
	const std::string& GetName() const override;
	ConnectionState GetConnectionState() const override;
	void Start(double now) override;
	void Update(double now) override;
	void Stop(double now) override;
	bool IsStopped() const override;
	const PowerSample& GetLatestSample() const override;

private:
	BleLink m_link;
	std::string m_name;
	PowerSample m_sample;
	CadenceCalculator m_cadence;
	bool m_stopped;
};
}  // namespace Struktur::Fitness
