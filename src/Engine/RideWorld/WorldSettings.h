#pragma once

#include <cstdint>
#include <string>

#include "nlohmann/json_fwd.hpp"

namespace Struktur::RideWorld
{
enum class Quality : uint8_t
{
	LOW,
	MEDIUM,
	HIGH,
};

const char* ToString(Quality quality);
Quality QualityFromString(const std::string& text, Quality fallback);

// Where the road's grade comes from.
//  - MANUAL: SIM mode - the road ahead blends towards the grade the rider sets with -/+, and the grade under the
//    rider is what the trainer is told. ERG mode - cosmetic, from the workout's step intensity (Free Ride: a gentle
//    procedural profile).
//  - ROUTE: the procedural route profile decides the grade (SIM: the trainer simulates it).
enum class GradeSource : uint8_t
{
	MANUAL,
	ROUTE,
};

const char* ToString(GradeSource source);
GradeSource GradeSourceFromString(const std::string& text, GradeSource fallback);

// What a quality preset expands to.
struct QualityParameters
{
	double drawDistanceMeters = 1000.0;
	double lateralExtentMeters = 320.0;  // terrain reaches this far either side of the road
	double propDensity        = 0.7;     // multiplier on every biome prop density
	int lowResHeight          = 360;     // scene framebuffer height (width follows the window's aspect)
	int weatherParticles      = 600;
};

QualityParameters ParametersFor(Quality quality);

// The procedural ride world's settings - the "world" section of ride_settings.json.
struct WorldSettings
{
	bool enabled            = true;
	uint32_t seed           = 1;
	Quality quality         = Quality::MEDIUM;  // web builds default to LOW (see Defaults())
	int lowResHeight        = 0;                // 0 = the preset's
	GradeSource gradeSource = GradeSource::MANUAL;
	std::string biome       = "auto";  // "auto" = seeded sequence, or a biome id to stay in one biome

	// Route limits.
	double maxGradePercent            = 12.0;
	double maxGradeChangePerMeter     = 0.05;  // % per metre: 1 % of grade change takes 20 m
	double minCurveRadiusMeters       = 90.0;
	double roadHalfWidthMeters        = 3.5;
	double shoulderMeters             = 1.2;

	// Camera.
	double cameraHeightMeters      = 2.2;  // 1.5 - 3
	double cameraLookAheadMeters   = 22.0;
	bool comfortMode               = false;  // disables every camera motion effect below
	bool cadenceBob                = false;
	bool speedFov                  = true;
	double fieldOfViewDegrees      = 62.0;

	// Retro look.
	bool nearestUpscale = true;
	bool flatShading    = true;
	bool bandedFog      = true;
	int fogBands        = 8;
	bool dither         = false;  // palette quantisation with ordered dithering
	int paletteLevels   = 6;      // levels per channel when dithering
	bool scanlines      = false;  // CRT scanline pass

	bool debugOverlay = false;
	bool demoPacer    = true;  // the pacer actor (only shown in --demo)

	static WorldSettings Defaults();
	static WorldSettings FromJson(const nlohmann::json& section, std::string& warning);
	nlohmann::json ToJson() const;
	void Validate();

	QualityParameters Resolved() const;  // preset + overrides
};
}  // namespace Struktur::RideWorld
