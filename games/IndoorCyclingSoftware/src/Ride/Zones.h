#pragma once

#include <cstdint>
#include <optional>

// Training zones. Pure functions - no engine dependencies.
namespace Struktur::Ride
{
enum class HeartRateZoneModel : uint8_t
{
	PERCENT_MAX,   // 5 zones from % of max heart rate
	PERCENT_LTHR,  // 7 zones from % of lactate threshold heart rate (Friel, cycling)
};

const char* ToString(HeartRateZoneModel model);
std::optional<HeartRateZoneModel> HeartRateZoneModelFromString(const char* text);

// Power: Coggan's 7 zones by % of FTP -
//   Z1 Active recovery < 56 %, Z2 Endurance 56-75 %, Z3 Tempo 76-90 %, Z4 Threshold 91-105 %,
//   Z5 VO2max 106-120 %, Z6 Anaerobic 121-150 %, Z7 Neuromuscular > 150 %.
constexpr int POWER_ZONE_COUNT = 7;
// 1-based zone, or 0 when it can't be classified (no FTP).
int PowerZone(double watts, double ftpWatts);

// Heart rate -
//   PERCENT_MAX:  Z1 < 60 %, Z2 60-70 %, Z3 70-80 %, Z4 80-90 %, Z5 >= 90 % of max HR.
//   PERCENT_LTHR: Z1 < 81 %, Z2 81-89 %, Z3 90-93 %, Z4 94-99 %, Z5a 100-102 %, Z5b 103-106 %, Z5c > 106 % of LTHR.
int HeartRateZoneCount(HeartRateZoneModel model);
// 1-based zone, or 0 when it can't be classified (missing max HR / LTHR).
int HeartRateZone(double bpm, HeartRateZoneModel model, double maxHeartRateBpm, double thresholdHeartRateBpm);

// One colour per zone, shared by power and heart rate so the same zone number always reads the same:
// grey, blue, green, yellow, orange, red, purple. Index 0 is "unknown" (neutral white).
struct ZoneColor
{
	uint8_t red   = 255;
	uint8_t green = 255;
	uint8_t blue  = 255;
};

ZoneColor PowerZoneColor(int zone);
// Heart-rate zones map onto the same palette, spread over the model's zone count.
ZoneColor HeartRateZoneColor(int zone, HeartRateZoneModel model);
}  // namespace Struktur::Ride
