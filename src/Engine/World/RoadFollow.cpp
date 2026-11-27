#include "RoadFollow.h"

#include <algorithm>
#include <cmath>

#include "Engine/World/World.h"

namespace Struktur::World
{
static constexpr double PI                     = 3.14159265358979323846;
static constexpr double LOOK_SMOOTHING_SECONDS = 0.35;
static constexpr double FOV_SMOOTHING_SECONDS  = 1.2;
static constexpr double BOB_AMPLITUDE_METERS   = 0.025;
static constexpr double MAX_SPEED_FOV_DEGREES  = 10.0;
static constexpr double FAR_PLANE_MARGIN       = 200.0;

void RoadFollow::ApplySettings(const WorldSettings& settings)
{
	heightMeters         = settings.cameraHeightMeters;
	distanceBehindMeters = settings.cameraDistanceMeters;
	lookAheadMeters      = settings.cameraLookAheadMeters;
	fieldOfViewDegrees   = settings.fieldOfViewDegrees;
	comfortMode          = settings.comfortMode;
	cadenceBob           = settings.cadenceBob;
	speedFov             = settings.speedFov;
}

void FollowRoad(RoadFollow& follow, const World& world, double deltaSeconds, double speedMetersPerSecond,
                double crankPhase, bool pedalling, Camera& camera)
{
	const double delta    = std::clamp(deltaSeconds, 0.0, 0.25);
	const Route& route    = world.GetRoute();
	const double distance = world.GetDistance();
	const double origin   = world.GetHeightOrigin();
	const double cameraAt = std::max(route.GetStart(), distance - follow.distanceBehindMeters);
	const RouteSample eye = route.SampleAt(cameraAt);
	const double lookAt   = distance + follow.lookAheadMeters;
	const RouteSample focus = route.SampleAt(lookAt);
	const bool motion       = !follow.comfortMode;

	double eyeY = world.GetProfile().HeightAt(cameraAt) - origin + follow.heightMeters;
	if (motion && follow.cadenceBob && pedalling)
	{
		eyeY += BOB_AMPLITUDE_METERS * std::sin(crankPhase * 4.0 * PI);
	}
	const double focusY = world.GetProfile().HeightAt(lookAt) - origin + follow.heightMeters * 0.55;

	glm::dvec3 direction(focus.x - eye.x, focusY - eyeY, focus.z - eye.z);
	const double length = glm::length(direction);
	if (length > 1e-6)
	{
		direction /= length;
	}
	if (follow.roadRevision != world.GetRoadRevision())
	{
		follow.smoothedLook = direction;
		follow.smoothedFov  = follow.fieldOfViewDegrees;
		follow.roadRevision = world.GetRoadRevision();
	}
	follow.smoothedLook += (direction - follow.smoothedLook) * (1.0 - std::exp(-delta / LOOK_SMOOTHING_SECONDS));

	double wantedFov = follow.fieldOfViewDegrees;
	if (motion && follow.speedFov)
	{
		wantedFov += std::min(MAX_SPEED_FOV_DEGREES, std::max(0.0, speedMetersPerSecond - 5.0) * 0.8);
	}
	follow.smoothedFov += (wantedFov - follow.smoothedFov) * (1.0 - std::exp(-delta / FOV_SMOOTHING_SECONDS));

	camera.position           = glm::dvec3(eye.x, eyeY, eye.z);
	camera.target             = camera.position + follow.smoothedLook * 10.0;
	camera.fieldOfViewDegrees = motion ? follow.smoothedFov : follow.fieldOfViewDegrees;
	camera.farPlane           = static_cast<float>(world.GetSettings().Resolved().drawDistanceMeters + FAR_PLANE_MARGIN);
}
}  // namespace Struktur::World
