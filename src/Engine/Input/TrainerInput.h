#pragma once

#include <optional>
#include <vector>

namespace Struktur::Input
{
// The smart trainer, heart rate strap and power meter as game input, plus the ride values derived from them. Written
// once per frame by System::RideSystem and read everywhere else (the world, the HUD, scripts) through
// Input::GetTrainer(). Engine-free, so the world library and its tests can fill one in by hand.
struct TrainerInput
{
	// --- devices (Fitness::FitnessManager): a value is empty while nothing fresh is reporting it ------------------
	std::optional<double> powerWatts;  // latest reading, unsmoothed
	std::optional<double> cadenceRpm;
	std::optional<double> heartRateBpm;
	bool trainerAssigned     = false;
	bool trainerConnected    = false;
	bool heartRateAssigned   = false;
	bool heartRateConnected  = false;
	bool powerMeterAssigned  = false;
	bool powerMeterConnected = false;

	// --- the ride in progress (Ride::RideSession): the defaults while none is running -----------------------------
	std::optional<double> smoothedPowerWatts;  // 3 s average
	double speedMetersPerSecond = 0.0;         // simulated
	double chosenGradePercent   = 0.0;         // the rider's -/+ grade (SIM resistance)
	bool ergMode                = false;
	int powerZone               = 0;
	std::optional<double> powerFraction;     // smoothed power / FTP
	std::optional<double> targetDeltaWatts;  // smoothed power - target
	double workoutProgress = 0.0;            // 0 - 1
	// Workout: target intensity (fraction of FTP) for each of the next N seconds; NaN = no target (free-ride step).
	// Empty = Free Ride.
	std::vector<double> intensityAhead;
};
}  // namespace Struktur::Input
