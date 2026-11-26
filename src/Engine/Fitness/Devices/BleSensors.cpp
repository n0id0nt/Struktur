#include "BleSensors.h"

#include <cstdio>
#include <optional>

#include "Debug/Assertions.h"
#include "Engine/Fitness/Protocol/CyclingPower.h"
#include "Engine/Fitness/Protocol/HeartRate.h"

namespace Struktur::Fitness
{
static const BleCharacteristicRef HEART_RATE_MEASUREMENT_CHARACTERISTIC{BleUuid::FromShort(Uuids::HEART_RATE_SERVICE),
                                                                        BleUuid::FromShort(
                                                                            Uuids::HEART_RATE_MEASUREMENT)};
static const BleCharacteristicRef CYCLING_POWER_MEASUREMENT_CHARACTERISTIC{
    BleUuid::FromShort(Uuids::CYCLING_POWER_SERVICE), BleUuid::FromShort(Uuids::CYCLING_POWER_MEASUREMENT)};

static BleLinkConfig MakeSingleSubscriptionConfig(BleAddress address, const BleCharacteristicRef& characteristic,
                                                  const ReconnectPolicy& reconnect)
{
	BleLinkConfig config;
	config.address   = address;
	config.reconnect = reconnect;
	config.setup     = {{LinkSetupStep::Kind::SUBSCRIBE, characteristic, SubscriptionKind::NOTIFY, true}};
	return config;
}

// Maps link lifecycle events onto device events. Returns the device event type, or nullopt for data events.
static std::optional<DeviceEvent::Type> LifecycleEventType(const LinkEvent& event)
{
	switch (event.type)
	{
		case LinkEvent::Type::READY:
			return DeviceEvent::Type::CONNECTED;
		case LinkEvent::Type::DISCONNECTED:
			return DeviceEvent::Type::DISCONNECTED;
		case LinkEvent::Type::RETRY_SCHEDULED:
			return DeviceEvent::Type::RECONNECTING;
		case LinkEvent::Type::FAILED:
			return DeviceEvent::Type::CONNECTION_FAILED;
		default:
			return std::nullopt;
	}
}

static std::string LifecycleDetail(const LinkEvent& event, const std::string& name)
{
	if (event.type == LinkEvent::Type::READY)
	{
		return name;
	}
	if (event.type == LinkEvent::Type::RETRY_SCHEDULED)
	{
		char buffer[48];
		std::snprintf(buffer, sizeof(buffer), " (retry in %.1f s)", event.retryDelaySeconds);
		return event.detail + buffer;
	}
	return event.detail;
}

// ---------------------------------------------------------------------------------------------------------------
// BleHeartRateSensor
// ---------------------------------------------------------------------------------------------------------------

BleHeartRateSensor::BleHeartRateSensor(IBleCentral& central, BleAddress address, std::string name,
                                       const ReconnectPolicy& reconnect)
    : m_link(central, MakeSingleSubscriptionConfig(address, HEART_RATE_MEASUREMENT_CHARACTERISTIC, reconnect)),
      m_name(std::move(name)),
      m_sample(),
      m_stopped(false)
{
}

std::string BleHeartRateSensor::GetIdentifier() const
{
	return FormatAddress(m_link.GetAddress());
}

const std::string& BleHeartRateSensor::GetName() const
{
	return m_name;
}

ConnectionState BleHeartRateSensor::GetConnectionState() const
{
	return m_link.GetConnectionState();
}

void BleHeartRateSensor::Start(double now)
{
	m_stopped = false;
	m_link.Start(now);
}

void BleHeartRateSensor::Update(double now)
{
	if (m_stopped)
	{
		return;
	}
	for (const LinkEvent& event : m_link.Update(now))
	{
		if (const std::optional<DeviceEvent::Type> lifecycle = LifecycleEventType(event))
		{
			Emit(*lifecycle, LifecycleDetail(event, m_name));
			continue;
		}
		if (event.type != LinkEvent::Type::NOTIFICATION ||
		    event.characteristic != HEART_RATE_MEASUREMENT_CHARACTERISTIC)
		{
			continue;
		}
		const std::optional<HeartRate::Measurement> measurement = HeartRate::ParseMeasurement(event.value);
		if (!measurement)
		{
			DEBUG_WARNING("Malformed Heart Rate Measurement (%zu bytes)", event.value.size());
			continue;
		}
		m_sample.timestampSeconds = now;
		m_sample.bpm              = measurement->bpm;
		m_sample.contact          = static_cast<HeartRateContact>(measurement->contact);
		m_sample.rrIntervalsSeconds.clear();
		for (uint16_t interval : measurement->rrIntervals)
		{
			m_sample.rrIntervalsSeconds.push_back(HeartRate::RrIntervalToSeconds(interval));
		}
		Emit(DeviceEvent::Type::SAMPLE);
	}
}

void BleHeartRateSensor::Stop(double)
{
	m_link.Stop();
	m_stopped = true;
}

bool BleHeartRateSensor::IsStopped() const
{
	return m_stopped;
}

const HeartRateSample& BleHeartRateSensor::GetLatestSample() const
{
	return m_sample;
}

// ---------------------------------------------------------------------------------------------------------------
// BlePowerMeter
// ---------------------------------------------------------------------------------------------------------------

BlePowerMeter::BlePowerMeter(IBleCentral& central, BleAddress address, std::string name,
                             const ReconnectPolicy& reconnect)
    : m_link(central, MakeSingleSubscriptionConfig(address, CYCLING_POWER_MEASUREMENT_CHARACTERISTIC, reconnect)),
      m_name(std::move(name)),
      m_sample(),
      m_cadence(),
      m_stopped(false)
{
}

std::string BlePowerMeter::GetIdentifier() const
{
	return FormatAddress(m_link.GetAddress());
}

const std::string& BlePowerMeter::GetName() const
{
	return m_name;
}

ConnectionState BlePowerMeter::GetConnectionState() const
{
	return m_link.GetConnectionState();
}

void BlePowerMeter::Start(double now)
{
	m_stopped = false;
	m_cadence.Reset();
	m_link.Start(now);
}

void BlePowerMeter::Update(double now)
{
	if (m_stopped)
	{
		return;
	}
	for (const LinkEvent& event : m_link.Update(now))
	{
		if (const std::optional<DeviceEvent::Type> lifecycle = LifecycleEventType(event))
		{
			if (event.type != LinkEvent::Type::READY)
			{
				// Crank counters restart from an unknown point after a reconnect.
				m_cadence.Reset();
			}
			Emit(*lifecycle, LifecycleDetail(event, m_name));
			continue;
		}
		if (event.type != LinkEvent::Type::NOTIFICATION ||
		    event.characteristic != CYCLING_POWER_MEASUREMENT_CHARACTERISTIC)
		{
			continue;
		}
		const std::optional<CyclingPower::Measurement> measurement = CyclingPower::ParseMeasurement(event.value);
		if (!measurement)
		{
			DEBUG_WARNING("Malformed Cycling Power Measurement (%zu bytes)", event.value.size());
			continue;
		}
		m_sample.timestampSeconds    = now;
		m_sample.powerWatts          = measurement->powerWatts;
		m_sample.pedalBalancePercent = measurement->GetPedalPowerBalancePercent();
		if (measurement->crank)
		{
			m_cadence.AddSample(measurement->crank->cumulativeRevolutions, measurement->crank->lastEventTime, now);
		}
		m_sample.cadenceRpm = m_cadence.GetCadence(now);
		Emit(DeviceEvent::Type::SAMPLE);
	}
	// Let cadence decay to zero even when the meter stops notifying.
	if (m_sample.cadenceRpm)
	{
		m_sample.cadenceRpm = m_cadence.GetCadence(now);
	}
}

void BlePowerMeter::Stop(double)
{
	m_link.Stop();
	m_stopped = true;
}

bool BlePowerMeter::IsStopped() const
{
	return m_stopped;
}

const PowerSample& BlePowerMeter::GetLatestSample() const
{
	return m_sample;
}
}  // namespace Struktur::Fitness
