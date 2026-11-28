#include "SpeedModel.h"

#include <algorithm>
#include <cmath>

namespace Struktur::Ride
{
static constexpr double MAX_SUBSTEP_SECONDS = 0.05;
static constexpr double MAX_GRADE_PERCENT   = 40.0;
// Below this the integrator switches from kinetic energy to a force form (see Step).
static constexpr double LOW_SPEED_METERS_PER_SECOND = 1.0;

SpeedModel::SpeedModel(const SpeedModelParameters& parameters)
    : m_parameters(parameters),
      m_speed(0.0),
      m_distance(0.0),
      m_elevationGain(0.0),
      m_altitude(0.0)
{
}

void SpeedModel::SetParameters(const SpeedModelParameters& parameters)
{
	m_parameters = parameters;
}

const SpeedModelParameters& SpeedModel::GetParameters() const
{
	return m_parameters;
}

double SpeedModel::ResistingForce(const SpeedModelParameters& parameters, double speedMetersPerSecond,
                                  double gradePercent)
{
	const double mass         = parameters.riderMassKg + parameters.bikeMassKg;
	const double theta        = std::atan(gradePercent / 100.0);
	const double gravityForce = mass * parameters.gravity * std::sin(theta);
	const double rollingForce = mass * parameters.gravity * std::cos(theta) * parameters.rollingResistance;
	const double aeroForce    = 0.5 * parameters.airDensityKgPerCubicM * parameters.dragAreaSquareMeters *
	                         speedMetersPerSecond * speedMetersPerSecond;
	return gravityForce + rollingForce + aeroForce;
}

void SpeedModel::Step(double powerWatts, double gradePercent, double deltaSeconds)
{
	if (deltaSeconds <= 0.0)
	{
		return;
	}
	const double mass          = m_parameters.riderMassKg + m_parameters.bikeMassKg;
	const double wheelPower    = std::max(0.0, powerWatts) * m_parameters.drivetrainEfficiency;
	const double grade         = std::clamp(gradePercent, -MAX_GRADE_PERCENT, MAX_GRADE_PERCENT);
	const double sinTheta      = std::sin(std::atan(grade / 100.0));
	const int substeps         = std::max(1, (int)std::ceil(deltaSeconds / MAX_SUBSTEP_SECONDS));
	const double substepLength = deltaSeconds / substeps;

	for (int i = 0; i < substeps; ++i)
	{
		const double resisting     = ResistingForce(m_parameters, m_speed, grade);
		const double previousSpeed = m_speed;
		if (m_speed >= LOW_SPEED_METERS_PER_SECOND)
		{
			// Kinetic energy: d(0.5 m v^2) = (P - F v) dt.
			const double energy = 0.5 * mass * m_speed * m_speed + (wheelPower - resisting * m_speed) * substepLength;
			m_speed             = energy > 0.0 ? std::sqrt(2.0 * energy / mass) : 0.0;
		}
		else
		{
			// Near standstill P / v blows up, so use the force form with the drive force capped at P / v_low. A
			// stopped bike only moves if the net force pushes it forward (pedalling, or a descent steep enough to beat
			// rolling resistance) - it never rolls backwards.
			const double driveForce = wheelPower / LOW_SPEED_METERS_PER_SECOND;
			m_speed                 = std::max(0.0, m_speed + (driveForce - resisting) / mass * substepLength);
		}
		const double travelled = 0.5 * (previousSpeed + m_speed) * substepLength;
		m_distance += travelled;
		const double climbed = travelled * sinTheta;
		m_altitude += climbed;
		if (climbed > 0.0)
		{
			m_elevationGain += climbed;
		}
	}
}

void SpeedModel::Reset()
{
	m_speed         = 0.0;
	m_distance      = 0.0;
	m_elevationGain = 0.0;
	m_altitude      = 0.0;
}

double SpeedModel::GetSpeedMetersPerSecond() const
{
	return m_speed;
}

double SpeedModel::GetDistanceMeters() const
{
	return m_distance;
}

double SpeedModel::GetElevationGainMeters() const
{
	return m_elevationGain;
}

double SpeedModel::GetAltitudeMeters() const
{
	return m_altitude;
}

static double NetPower(const SpeedModelParameters& parameters, double wheelPower, double gradePercent, double speed)
{
	return wheelPower - SpeedModel::ResistingForce(parameters, speed, gradePercent) * speed;
}

double SpeedModel::SteadyStateSpeed(const SpeedModelParameters& parameters, double powerWatts, double gradePercent)
{
	const double wheelPower = std::max(0.0, powerWatts) * parameters.drivetrainEfficiency;
	// Net power at speed v: wheelPower - F(v) v. Positive below the steady state, negative above it.
	double low  = 0.0;
	double high = 40.0;
	if (NetPower(parameters, wheelPower, gradePercent, high) > 0.0)
	{
		return high;
	}
	for (int i = 0; i < 80; ++i)
	{
		const double middle = 0.5 * (low + high);
		if (NetPower(parameters, wheelPower, gradePercent, middle) > 0.0)
		{
			low = middle;
		}
		else
		{
			high = middle;
		}
	}
	// On a steep descent with no power the "steady state" is still a positive speed; on a climb too steep for the
	// power the net is negative everywhere and bisection collapses to ~0.
	return 0.5 * (low + high);
}

double SpeedModel::EquivalentGrade(const SpeedModelParameters& parameters, double powerWatts,
                                   double speedMetersPerSecond)
{
	if (speedMetersPerSecond < 0.5)
	{
		return 0.0;
	}
	const double wheelPower = std::max(0.0, powerWatts) * parameters.drivetrainEfficiency;
	// Resisting force rises monotonically with grade, so bisect for F(v, grade) v == wheelPower.
	double low  = -MAX_GRADE_PERCENT;
	double high = MAX_GRADE_PERCENT;
	for (int i = 0; i < 80; ++i)
	{
		const double middle = 0.5 * (low + high);
		if (ResistingForce(parameters, speedMetersPerSecond, middle) * speedMetersPerSecond < wheelPower)
		{
			low = middle;
		}
		else
		{
			high = middle;
		}
	}
	return 0.5 * (low + high);
}
}  // namespace Struktur::Ride
