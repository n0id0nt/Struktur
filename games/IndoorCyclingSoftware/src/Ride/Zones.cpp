#include "Zones.h"

#include <cstring>

namespace Struktur::Ride
{
static constexpr ZoneColor ZONE_PALETTE[] = {
    {235, 235, 235},  // 0 unknown
    {150, 150, 150},  // 1 grey
    {60, 140, 235},   // 2 blue
    {75, 200, 95},    // 3 green
    {245, 205, 50},   // 4 yellow
    {245, 135, 40},   // 5 orange
    {230, 60, 50},    // 6 red
    {165, 85, 220},   // 7 purple
};

const char* ToString(HeartRateZoneModel model)
{
	return model == HeartRateZoneModel::PERCENT_LTHR ? "percentLthr" : "percentMax";
}

std::optional<HeartRateZoneModel> HeartRateZoneModelFromString(const char* text)
{
	if (text == nullptr)
	{
		return std::nullopt;
	}
	if (std::strcmp(text, "percentMax") == 0)
	{
		return HeartRateZoneModel::PERCENT_MAX;
	}
	if (std::strcmp(text, "percentLthr") == 0)
	{
		return HeartRateZoneModel::PERCENT_LTHR;
	}
	return std::nullopt;
}

int PowerZone(double watts, double ftpWatts)
{
	if (ftpWatts <= 0.0)
	{
		return 0;
	}
	// Upper bounds of Z1..Z6 as fractions of FTP; anything above is Z7.
	static constexpr double UPPER_BOUNDS[] = {0.555, 0.755, 0.905, 1.055, 1.205, 1.505};
	const double fraction                  = watts / ftpWatts;
	for (int i = 0; i < 6; ++i)
	{
		if (fraction < UPPER_BOUNDS[i])
		{
			return i + 1;
		}
	}
	return 7;
}

int HeartRateZoneCount(HeartRateZoneModel model)
{
	return model == HeartRateZoneModel::PERCENT_LTHR ? 7 : 5;
}

int HeartRateZone(double bpm, HeartRateZoneModel model, double maxHeartRateBpm, double thresholdHeartRateBpm)
{
	if (model == HeartRateZoneModel::PERCENT_MAX)
	{
		if (maxHeartRateBpm <= 0.0)
		{
			return 0;
		}
		static constexpr double UPPER_BOUNDS[] = {0.60, 0.70, 0.80, 0.90};
		const double fraction                  = bpm / maxHeartRateBpm;
		for (int i = 0; i < 4; ++i)
		{
			if (fraction < UPPER_BOUNDS[i])
			{
				return i + 1;
			}
		}
		return 5;
	}

	if (thresholdHeartRateBpm <= 0.0)
	{
		return 0;
	}
	// Upper bounds of Z1..Z5b as fractions of LTHR (whole-percent bands, so x.5 boundaries).
	static constexpr double UPPER_BOUNDS[] = {0.805, 0.895, 0.935, 0.995, 1.025, 1.065};
	const double fraction                  = bpm / thresholdHeartRateBpm;
	for (int i = 0; i < 6; ++i)
	{
		if (fraction < UPPER_BOUNDS[i])
		{
			return i + 1;
		}
	}
	return 7;
}

ZoneColor PowerZoneColor(int zone)
{
	return (zone >= 1 && zone <= 7) ? ZONE_PALETTE[zone] : ZONE_PALETTE[0];
}

ZoneColor HeartRateZoneColor(int zone, HeartRateZoneModel model)
{
	if (zone < 1)
	{
		return ZONE_PALETTE[0];
	}
	if (model == HeartRateZoneModel::PERCENT_LTHR)
	{
		return PowerZoneColor(zone);
	}
	// 5 zones onto grey, blue, green, orange, red - the colours a rider associates with easy -> hard.
	static constexpr int FIVE_ZONE_PALETTE[] = {1, 2, 3, 5, 6};
	return ZONE_PALETTE[FIVE_ZONE_PALETTE[(zone > 5 ? 5 : zone) - 1]];
}
}  // namespace Struktur::Ride
