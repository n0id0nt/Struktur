#include "RiderAnimation.h"

#include <algorithm>
#include <cmath>

namespace Struktur::World
{
static constexpr double PI                   = 3.14159265358979323846;
static constexpr double GRAVITY              = 9.81;
static constexpr double COASTING_CADENCE_RPM = 1.0;  // below this the legs stop
static constexpr double STAND_HYSTERESIS     = 0.15;  // sit back down this far below the stand threshold

const char* ToString(RiderPose pose)
{
	switch (pose)
	{
		case RiderPose::COASTING:
			return "coasting";
		case RiderPose::IDLE:
			return "idle";
		case RiderPose::OUT_OF_SADDLE:
			return "outOfSaddle";
		default:
			return "pedalling";
	}
}

int CrankFrame(double phase, int framesPerRevolution)
{
	if (framesPerRevolution <= 0)
	{
		return 0;
	}
	const double wrapped = phase - std::floor(phase);
	const int frame      = static_cast<int>(std::floor(wrapped * framesPerRevolution));
	return ((frame % framesPerRevolution) + framesPerRevolution) % framesPerRevolution;
}

int MinimumFramesPerRevolution(double maxDegreesPerFrame)
{
	if (maxDegreesPerFrame <= 0.0)
	{
		return 1;
	}
	return static_cast<int>(std::ceil(360.0 / maxDegreesPerFrame));
}

bool WheelAliases(double revolutionsPerSecond, int spokes, double framesPerSecond)
{
	if (spokes <= 0 || framesPerSecond <= 0.0)
	{
		return false;
	}
	const double spokeGapsPerFrame = std::abs(revolutionsPerSecond) * spokes / framesPerSecond;
	return spokeGapsPerFrame > 0.5;
}

RiderAnimation::RiderAnimation(const RiderAnimationSettings& settings)
    : m_settings(settings),
      m_smoothedCadence(0.0),
      m_phase(0.0),
      m_wheelAngle(0.0),
      m_wheelRps(0.0),
      m_pose(RiderPose::IDLE),
      m_lean(0.0),
      m_standing(false)
{
}

void RiderAnimation::Update(double deltaSeconds, std::optional<double> cadenceRpm, double speedMetersPerSecond,
                            std::optional<double> powerFraction, double curvature)
{
	const double delta = std::clamp(deltaSeconds, 0.0, 0.25);
	if (!cadenceRpm)
	{
		m_smoothedCadence = 0.0;
		m_pose            = RiderPose::IDLE;
	}
	else
	{
		const double cadence = std::clamp(*cadenceRpm, 0.0, 250.0);
		const double blend   = 1.0 - std::exp(-delta / std::max(0.01, m_settings.cadenceSmoothingSeconds));
		m_smoothedCadence += (cadence - m_smoothedCadence) * blend;
		if (cadence < COASTING_CADENCE_RPM)
		{
			// Freeze straight away: legs drifting on after the rider stopped pedalling looks wrong.
			m_smoothedCadence = 0.0;
			m_pose            = RiderPose::COASTING;
		}
		else
		{
			m_phase += m_smoothedCadence / 60.0 * delta;
			m_phase -= std::floor(m_phase);
			if (powerFraction)
			{
				if (*powerFraction > m_settings.outOfSaddlePowerFraction)
				{
					m_standing = true;
				}
				else if (*powerFraction < m_settings.outOfSaddlePowerFraction - STAND_HYSTERESIS)
				{
					m_standing = false;
				}
			}
			m_pose = m_standing ? RiderPose::OUT_OF_SADDLE : RiderPose::PEDALLING;
		}
	}
	m_wheelRps   = std::max(0.0, speedMetersPerSecond) / (2.0 * PI * m_settings.wheelRadiusMeters);
	m_wheelAngle = std::fmod(m_wheelAngle + std::max(0.0, speedMetersPerSecond) / m_settings.wheelRadiusMeters * delta,
	                         2.0 * PI);

	// Lean into bends: tan(lean) = v^2 * curvature / g.
	const double wantedLean =
	    std::clamp(std::atan(speedMetersPerSecond * speedMetersPerSecond * curvature / GRAVITY) * 180.0 / PI,
	               -m_settings.maxLeanDegrees, m_settings.maxLeanDegrees);
	const double leanBlend = 1.0 - std::exp(-delta / std::max(0.01, m_settings.leanSmoothingSeconds));
	m_lean += (wantedLean - m_lean) * leanBlend;
}

double RiderAnimation::GetSmoothedCadence() const
{
	return m_smoothedCadence;
}

double RiderAnimation::GetCrankPhase() const
{
	return m_phase;
}

double RiderAnimation::GetWheelAngle() const
{
	return m_wheelAngle;
}

double RiderAnimation::GetWheelRevolutionsPerSecond() const
{
	return m_wheelRps;
}

RiderPose RiderAnimation::GetPose() const
{
	return m_pose;
}

double RiderAnimation::GetLeanDegrees() const
{
	return m_lean;
}

int RiderAnimation::GetFrame(int framesPerRevolution, int coastFrame) const
{
	if (m_pose == RiderPose::COASTING || m_pose == RiderPose::IDLE)
	{
		return std::clamp(coastFrame, 0, std::max(0, framesPerRevolution - 1));
	}
	return CrankFrame(m_phase, framesPerRevolution);
}
}  // namespace Struktur::World
