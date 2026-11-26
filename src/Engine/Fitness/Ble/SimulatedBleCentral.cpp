#include "SimulatedBleCentral.h"

#include <cmath>

namespace Struktur::Fitness
{
static constexpr double ADVERTISE_INTERVAL_SECONDS = 1.0;
static constexpr double DATA_INTERVAL_SECONDS      = 0.5;
static constexpr double SIMULATED_CADENCE_RPM      = 85.0;

SimulatedBleCentral::SimulatedBleCentral()
    : m_start(std::chrono::steady_clock::now()),
      m_trainer(),
      m_heartRateAddress(0x5EED00000002ull),
      m_powerMeterAddress(0x5EED00000003ull),
      m_nextAdvertiseSeconds(0.0),
      m_nextDataSeconds(0.0),
      m_crankRevolutions(0),
      m_crankEventTime(0)
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
	if (now >= m_nextDataSeconds)
	{
		// Notify() only reaches devices the game has subscribed to, so unpaired ones stay silent.
		m_trainer->Tick(now);
		const int heartRate = 118 + static_cast<int>(12.0 * std::sin(now / 20.0) + 12.0);
		SendFakeHeartRate(*this, m_heartRateAddress, static_cast<uint8_t>(heartRate));
		const double secondsPerRevolution = 60.0 / SIMULATED_CADENCE_RPM;
		const int revolutions             = static_cast<int>(now / secondsPerRevolution);
		m_crankRevolutions                = static_cast<uint16_t>(revolutions);
		m_crankEventTime = static_cast<uint16_t>(static_cast<long long>(revolutions * secondsPerRevolution * 1024.0));
		SendFakePower(*this, m_powerMeterAddress, 175, m_crankRevolutions, m_crankEventTime);
		m_nextDataSeconds = now + DATA_INTERVAL_SECONDS;
	}
	FakeBleCentral::Poll();
}

double SimulatedBleCentral::SecondsSinceStart() const
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
}
}  // namespace Struktur::Fitness
