#pragma once

#include <chrono>
#include <memory>

#include "Engine/Fitness/Ble/FakeBleCentral.h"
#include "Engine/Fitness/Ble/FakeDevices.h"

namespace Struktur::Fitness
{
// A self-running FakeBleCentral with a fake FTMS trainer, heart rate strap and power meter that advertise while a
// scan is running and stream data on their own. A synthetic rider drives them: in ERG the rider's power follows the
// trainer's target with a ~1 s lag and a little noise; in simulation it rises and falls with the grade around a steady
// effort; cadence sags on climbs; heart rate drifts towards a level set by power over ~25 s.
// For working on the fitness screens with no hardware: the game uses it with --demo or STRUKTUR_FITNESS_SIMULATE=1
// (see GameContext). Single-threaded like its base - all work happens in Poll(), which FitnessManager calls from the
// game thread every frame.
class SimulatedBleCentral : public FakeBleCentral
{
public:
	SimulatedBleCentral();

	void Poll() override;

private:
	double SecondsSinceStart() const;
	void StepRider(double now, double deltaSeconds);

	std::chrono::steady_clock::time_point m_start;
	std::unique_ptr<FakeFtmsTrainer> m_trainer;
	BleAddress m_heartRateAddress;
	BleAddress m_powerMeterAddress;
	double m_nextAdvertiseSeconds;
	double m_nextDataSeconds;
	uint16_t m_crankRevolutions;
	uint16_t m_crankEventTime;
	double m_lastRiderSeconds;
	double m_riderPower;
	double m_riderCadence;
	double m_riderHeartRate;
	double m_crankAccumulator;
};
}  // namespace Struktur::Fitness
