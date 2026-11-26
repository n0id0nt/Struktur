#include "SimulatedBleCentral.h"

#include <algorithm>
#include <cmath>

namespace Struktur::Fitness
{
static constexpr double ADVERTISE_INTERVAL_SECONDS = 1.0;
static constexpr double DATA_INTERVAL_SECONDS      = 0.5;
// The synthetic rider's steady effort on a flat road in simulation mode.
static constexpr double SIMULATED_EFFORT_WATTS = 185.0;

SimulatedBleCentral::SimulatedBleCentral()
    : m_start(std::chrono::steady_clock::now()),
      m_trainer(),
      m_heartRateAddress(0x5EED00000002ull),
      m_powerMeterAddress(0x5EED00000003ull),
      m_nextAdvertiseSeconds(0.0),
      m_nextDataSeconds(0.0),
      m_crankRevolutions(0),
      m_crankEventTime(0),
      m_lastRiderSeconds(0.0),
      m_riderPower(0.0),
      m_riderCadence(0.0),
      m_riderHeartRate(72.0),
      m_crankAccumulator(0.0)
{
	FakeFtmsTrainer::Config config;
	config.address               = 0x5EED00000001ull;
	config.name                  = "SIM Smart Trainer";
	config.resistanceRangeTenths = std::array<int16_t, 3>{0, 200, 10};
	m_trainer                    = std::make_unique<FakeFtmsTrainer>(*this, config);
	AddFakeHeartRateMonitor(*this, m_heartRateAddress, "SIM Heart Rate");
	AddFakePowerMeter(*this, m_powerMeterAddress, "SIM Power Pedals");
}

void SimulatedBleCentral::Poll()
{
	const double now = SecondsSinceStart();
	if (IsScanning() && now >= m_nextAdvertiseSeconds)
	{
		AdvertiseAll();
		m_nextAdvertiseSeconds = now + ADVERTISE_INTERVAL_SECONDS;
	}
	StepRider(now, std::clamp(now - m_lastRiderSeconds, 0.0, 0.5));
	m_lastRiderSeconds = now;
	if (now >= m_nextDataSeconds)
	{
		// Notify() only reaches devices the game has subscribed to, so unpaired ones stay silent.
		m_trainer->SetRiderOutput((int)std::lround(m_riderPower), m_riderCadence);
		m_trainer->Tick(now);
		SendFakeHeartRate(*this, m_heartRateAddress, (uint8_t)std::lround(m_riderHeartRate));
		SendFakePower(*this, m_powerMeterAddress, (int16_t)std::lround(m_riderPower), m_crankRevolutions,
		              m_crankEventTime);
		m_nextDataSeconds = now + DATA_INTERVAL_SECONDS;
	}
	FakeBleCentral::Poll();
}

void SimulatedBleCentral::StepRider(double now, double deltaSeconds)
{
	if (deltaSeconds <= 0.0)
	{
		return;
	}
	// A rider who isn't paired to anything yet sits still.
	const BleCharacteristicRef indoorBikeData{BleUuid::FromShort(Uuids::FITNESS_MACHINE_SERVICE),
	                                          BleUuid::FromShort(Uuids::FTMS_INDOOR_BIKE_DATA)};
	if (!IsSubscribed(m_trainer->GetAddress(), indoorBikeData))
	{
		m_riderPower   = 0.0;
		m_riderCadence = 0.0;
		m_riderHeartRate += (72.0 - m_riderHeartRate) * (1.0 - std::exp(-deltaSeconds / 25.0));
		return;
	}
	const double wobble = std::sin(now * 0.9) * 0.025 + std::sin(now * 2.3) * 0.015;
	double desired      = 0.0;
	double cadence      = 0.0;
	if (const std::optional<int> target = m_trainer->GetTargetPower())
	{
		desired = *target;
		cadence = *target > 300 ? 96.0 : 89.0;
	}
	else
	{
		const double grade = m_trainer->GetTargetGradePercent().value_or(0.0);
		desired            = SIMULATED_EFFORT_WATTS * std::clamp(1.0 + 0.07 * grade, 0.35, 1.9);
		cadence            = std::clamp(90.0 - 1.8 * grade, 62.0, 100.0);
		if (grade < -4.5)
		{
			// Steep descent: freewheel.
			desired = 0.0;
			cadence = 0.0;
		}
	}
	// Every 80 s, a 10 s high-cadence spin-up (~118 rpm).
	if (cadence > 0.0 && std::fmod(now, 80.0) > 70.0)
	{
		cadence = 118.0;
	}
	desired *= 1.0 + wobble;
	m_riderPower += (desired - m_riderPower) * (1.0 - std::exp(-deltaSeconds / 1.1));
	m_riderCadence += (cadence + std::sin(now * 1.7) * 2.0 - m_riderCadence) * (1.0 - std::exp(-deltaSeconds / 1.5));
	const double heartTarget = std::clamp(62.0 + 0.47 * m_riderPower, 60.0, 192.0);
	m_riderHeartRate += (heartTarget - m_riderHeartRate) * (1.0 - std::exp(-deltaSeconds / 25.0));
	// Crank revolutions for the power meter's cadence.
	m_crankAccumulator += m_riderCadence / 60.0 * deltaSeconds;
	while (m_crankAccumulator >= 1.0)
	{
		m_crankAccumulator -= 1.0;
		m_crankRevolutions = (uint16_t)(m_crankRevolutions + 1);
		m_crankEventTime   = (uint16_t)std::llround(now * 1024.0);
	}
}

double SimulatedBleCentral::SecondsSinceStart() const
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
}
}  // namespace Struktur::Fitness
