#pragma once

#include <string>

#include "Engine/Ride/SpeedModel.h"
#include "Engine/Ride/TrainerController.h"
#include "Engine/Ride/Zones.h"
#include "Engine/RideWorld/WorldSettings.h"

namespace Struktur::Ride
{
enum class PowerSource : uint8_t
{
	AUTO,  // power meter when one is connected, else the trainer
	TRAINER,
	POWER_METER,
};

const char* ToString(PowerSource source);

// App-wide ride settings, persisted as ride_settings.json in the save folder. Rider-specific numbers (FTP, max HR,
// LTHR, weight, units) live on the rider profile instead, so they are never stored twice.
struct RideSettings
{
	HeartRateZoneModel heartRateZoneModel = HeartRateZoneModel::PERCENT_MAX;
	int graphWindowSeconds                = 600;
	PowerSource powerSource               = PowerSource::AUTO;
	double autoPauseSeconds               = 5.0;  // no power for this long pauses the ride; 0 = off
	double autosaveSeconds                = 60.0;
	double targetBandPercent              = 5.0;   // "in target" = within +/- this % of the target...
	double targetBandMinimumWatts         = 10.0;  // ...but never narrower than +/- this many watts
	double leadInSeconds                  = 4.0;   // workout step lead-in ramp
	int freeRideStartTargetWatts          = 0;     // 0 = 65 % of FTP
	SpeedModelParameters speed;                    // riderMassKg is replaced by the rider's weight at ride start
	TrainerControlSettings control;
	RideWorld::WorldSettings world = RideWorld::WorldSettings::Defaults();  // the "world" section

	// Missing or out-of-range values fall back to defaults; `warning` describes anything that was ignored.
	static RideSettings FromJson(const std::string& json, std::string& warning);
	std::string ToJson() const;
	// Clamps every field into its valid range.
	void Validate();
};
}  // namespace Struktur::Ride
