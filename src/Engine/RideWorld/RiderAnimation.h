#pragma once

#include <cstdint>
#include <optional>

namespace Struktur::RideWorld
{
enum class RiderPose : uint8_t
{
	PEDALLING,
	COASTING,       // cadence 0: legs frozen in the coasting frame
	IDLE,           // no cadence sensor: the idle pose
	OUT_OF_SADDLE,  // pedalling, power well above FTP
};

const char* ToString(RiderPose pose);

// floor(phase * framesPerRevolution) % framesPerRevolution, with phase in revolutions (any value; negative wraps).
int CrankFrame(double phase, int framesPerRevolution);

// The smallest frames-per-revolution that shows every crank position within `maxDegreesPerFrame` of rotation. At any
// realistic cadence the per-display-frame advance is far below half a revolution (that would need > 30 x fps rpm),
// so the legs can't strobe backwards; the frame count only decides smoothness. 8 frames = 45 degree steps (minimum
// that still reads as pedalling), 12 = 30 degree steps (recommended).
int MinimumFramesPerRevolution(double maxDegreesPerFrame);

// True when a wheel with `spokes` evenly spaced spokes, turning at `revolutionsPerSecond`, advances more than half a
// spoke gap per display frame - the point where it starts to look stationary or backwards (wagon-wheel effect) and
// the renderer should switch to a blurred wheel.
bool WheelAliases(double revolutionsPerSecond, int spokes, double framesPerSecond);

struct RiderAnimationSettings
{
	double cadenceSmoothingSeconds = 0.35;  // exponential smoothing of the cadence input
	double wheelRadiusMeters       = 0.34;  // 700c x 25 mm
	double outOfSaddlePowerFraction = 1.25;  // power / FTP above which the rider stands (with hysteresis)
	double leanSmoothingSeconds    = 0.4;
	double maxLeanDegrees          = 18.0;
};

// Drives the rider sprite: crank phase from smoothed cadence, wheel angle from speed, pose, and lean in bends.
class RiderAnimation
{
public:
	explicit RiderAnimation(const RiderAnimationSettings& settings = RiderAnimationSettings());

	// cadence: rpm, nullopt when there is no cadence source. powerFraction: power / FTP (nullopt = unknown).
	// curvature: road curvature at the rider (1/m, positive = left).
	void Update(double deltaSeconds, std::optional<double> cadenceRpm, double speedMetersPerSecond,
	            std::optional<double> powerFraction, double curvature);

	double GetSmoothedCadence() const;
	double GetCrankPhase() const;  // revolutions, [0, 1)
	double GetWheelAngle() const;  // radians, [0, 2 pi)
	double GetWheelRevolutionsPerSecond() const;
	RiderPose GetPose() const;
	double GetLeanDegrees() const;  // positive = leaning left
	// The frame to draw for a sheet with `framesPerRevolution` frames (the coasting frame when coasting).
	int GetFrame(int framesPerRevolution, int coastFrame) const;

private:
	RiderAnimationSettings m_settings;
	double m_smoothedCadence;
	double m_phase;
	double m_wheelAngle;
	double m_wheelRps;
	RiderPose m_pose;
	double m_lean;
	bool m_standing;
};
}  // namespace Struktur::RideWorld
