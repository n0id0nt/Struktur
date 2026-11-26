#pragma once

#include <chrono>
#include <memory>

#include "Engine/Fitness/Ble/FakeBleCentral.h"
#include "Engine/Fitness/Ble/FakeDevices.h"

namespace Struktur::Fitness
{
// A self-running FakeBleCentral with a fake FTMS trainer, heart rate strap and power meter that advertise while a
// scan is running and stream data on their own. For working on fitness UI with no hardware: the game uses it when
// the STRUKTUR_FITNESS_SIMULATE environment variable is set (see GameContext). Single-threaded like its base - all
// work happens in Poll(), which FitnessManager calls from the game thread every frame.
class SimulatedBleCentral : public FakeBleCentral
{
public:
	SimulatedBleCentral();

	void Poll() override;

private:
	double SecondsSinceStart() const;

	std::chrono::steady_clock::time_point m_start;
	std::unique_ptr<FakeFtmsTrainer> m_trainer;
	BleAddress m_heartRateAddress;
	BleAddress m_powerMeterAddress;
	double m_nextAdvertiseSeconds;
	double m_nextDataSeconds;
	uint16_t m_crankRevolutions;
	uint16_t m_crankEventTime;
};
}  // namespace Struktur::Fitness
