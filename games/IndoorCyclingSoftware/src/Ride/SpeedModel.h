#pragma once

namespace Struktur::Ride
{
struct SpeedModelParameters
{
	double riderMassKg           = 75.0;
	double bikeMassKg            = 8.0;
	double rollingResistance     = 0.004;  // Crr - road tyres on smooth tarmac
	double dragAreaSquareMeters  = 0.32;   // CdA - hoods, road bike
	double airDensityKgPerCubicM = 1.225;  // sea level, 15 C
	double drivetrainEfficiency  = 0.976;
	double gravity               = 9.80665;
};

// Standard cycling power model, integrated through kinetic energy so speed responds to grade changes and coasting
// with sensible inertia instead of jumping to the steady-state speed:
//
//   P_wheel = P_rider * efficiency
//   F_resist = m g sin(theta) + m g cos(theta) Crr + 0.5 rho CdA v^2      (theta = atan(grade), still air)
//   d(0.5 m v^2)/dt = P_wheel - F_resist * v
//
// Distance integrates v; virtual elevation gain sums the climbed component (v dt sin theta, positive only), and
// altitude tracks both directions. Pure and deterministic - time steps are passed in.
class SpeedModel
{
public:
	explicit SpeedModel(const SpeedModelParameters& parameters = SpeedModelParameters());

	void SetParameters(const SpeedModelParameters& parameters);
	const SpeedModelParameters& GetParameters() const;

	// Advances by deltaSeconds (internally sub-stepped at <= 50 ms) with the rider's power and the road grade
	// (percent, positive = uphill).
	void Step(double powerWatts, double gradePercent, double deltaSeconds);

	void Reset();

	double GetSpeedMetersPerSecond() const;
	double GetDistanceMeters() const;
	double GetElevationGainMeters() const;
	double GetAltitudeMeters() const;  // relative to the ride start

	// Total resisting force at a speed and grade (newtons).
	static double ResistingForce(const SpeedModelParameters& parameters, double speedMetersPerSecond,
	                             double gradePercent);
	// Speed the rider would settle at holding `powerWatts` on `gradePercent` (bisection, 0 if power can't overcome
	// the grade).
	static double SteadyStateSpeed(const SpeedModelParameters& parameters, double powerWatts, double gradePercent);
	// The grade at which `powerWatts` exactly holds `speedMetersPerSecond` - used to hand over from ERG to simulation
	// without a jump in resistance. Clamped to [-40, 40] %.
	static double EquivalentGrade(const SpeedModelParameters& parameters, double powerWatts,
	                              double speedMetersPerSecond);

private:
	SpeedModelParameters m_parameters;
	double m_speed;
	double m_distance;
	double m_elevationGain;
	double m_altitude;
};
}  // namespace Struktur::Ride
