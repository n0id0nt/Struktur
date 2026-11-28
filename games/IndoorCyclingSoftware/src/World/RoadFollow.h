#pragma once

#include <cstdint>

#include "World/Camera.h"
#include "World/WorldSettings.h"

namespace Struktur::World
{
class World;

// How the world camera follows the rider along the road: behind and above, looking ahead, with a smoothed look
// direction, a field of view that widens with speed and an optional cadence bob. The camera entity's component.
struct RoadFollow
{
	double heightMeters         = 2.2;
	double distanceBehindMeters = 7.0;
	double lookAheadMeters      = 22.0;
	double fieldOfViewDegrees   = 62.0;
	bool comfortMode            = false;  // no camera motion effects
	bool cadenceBob             = false;
	bool speedFov               = true;

	// Smoothing state.
	glm::dvec3 smoothedLook{1.0, 0.0, 0.0};
	double smoothedFov    = 62.0;
	uint32_t roadRevision = 0;  // the road it last followed: a rebuilt road snaps the camera instead of gliding

	void ApplySettings(const WorldSettings& settings);
};

// Moves `camera` behind the rider of a loaded world. The crank phase (revolutions) drives the bob while `pedalling`.
void FollowRoad(RoadFollow& follow, const World& world, double deltaSeconds, double speedMetersPerSecond,
                double crankPhase, bool pedalling, Camera& camera);
}  // namespace Struktur::World
