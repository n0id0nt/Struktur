#pragma once

#include <cstdint>
#include <deque>
#include <vector>

namespace Struktur::World
{
// One arc-length sample of the road's centre line, in the horizontal (x, z) plane. y is up; positive t (lateral) is
// to the rider's left: left = (tangentZ, -tangentX).
struct RouteSample
{
	double x        = 0.0;
	double z        = 0.0;
	double tangentX = 1.0;
	double tangentZ = 0.0;
	double curvature = 0.0;  // 1/m, positive = bending left

	double LeftX() const
	{
		return tangentZ;
	}
	double LeftZ() const
	{
		return -tangentX;
	}
};

struct RouteSettings
{
	uint64_t seed                  = 1;
	double sampleSpacingMeters     = 1.0;    // arc-length resampling step
	double controlSpacingMeters    = 140.0;  // Catmull-Rom control point spacing
	double minCurveRadiusMeters    = 90.0;
	double maxHeadingDeviationDeg  = 75.0;   // from the route's mean direction: the road never turns back on itself
	double sweepAmplitudeDeg       = 45.0;   // long, sweeping bends
	double sweepWavelengthMeters   = 2600.0;
	double wiggleAmplitudeDeg      = 28.0;   // shorter bends
	double wiggleWavelengthMeters  = 650.0;
	double bendChance              = 0.18;   // per control point: a tight bend
};

// The endless road centre line: a centripetal Catmull-Rom spline through seeded control points, resampled every
// `sampleSpacingMeters` of arc length. Deterministic: the same settings give the same road. Generated lazily ahead
// of the rider and pruned behind, so memory stays bounded however far the ride goes.
//
// Curvature: the turn at each control point is limited so the spline's curvature stays under 1/minCurveRadius (the
// tests check this over 100 km), and the heading stays within maxHeadingDeviationDeg of the mean direction, so the
// road can't loop back over itself.
class Route
{
public:
	explicit Route(const RouteSettings& settings);

	const RouteSettings& GetSettings() const;
	// Generates samples until at least `distanceMeters` is covered.
	void EnsureGenerated(double distanceMeters);
	// Drops samples (and control points) entirely before `distanceMeters`.
	void PruneBefore(double distanceMeters);

	double GetStart() const;  // first retained distance
	double GetEnd() const;    // generated distance
	int64_t GetFirstIndex() const;
	int64_t GetSampleCount() const;  // retained
	// index is absolute (distance / spacing). Must be within the retained range.
	const RouteSample& GetSample(int64_t index) const;
	// Interpolated sample at a distance (clamped to the retained range).
	RouteSample SampleAt(double distanceMeters) const;
	// Largest |curvature| in [from, to] (retained samples only).
	double MaxCurvature(double from, double to) const;
	// Copies samples [firstIndex, firstIndex + count) - for the chunk worker, which must not touch the live route.
	void CopySamples(int64_t firstIndex, int64_t count, std::vector<RouteSample>& out) const;

private:
	struct Point
	{
		double x = 0.0;
		double z = 0.0;
	};

	double HeadingFor(int64_t controlIndex) const;
	void AddControlPoint();
	const Point& ControlAt(int64_t index) const;
	void EmitSegment();
	void UpdateCurvature(int64_t fromIndex);

	RouteSettings m_settings;
	double m_turnLimit;
	std::deque<Point> m_controls;  // m_controls[0] is control index m_firstControl
	int64_t m_firstControl;
	int64_t m_nextSegment;  // next segment (between control i and i+1) to emit
	double m_previousHeading;
	std::deque<RouteSample> m_samples;
	int64_t m_firstSample;
	// Resampling carry-over between segments.
	double m_carryLength;  // arc length already walked past the last emitted sample
	Point m_lastDense;
	bool m_hasDense;
};
}  // namespace Struktur::World
