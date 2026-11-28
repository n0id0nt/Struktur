#pragma once

#include <cstdint>
#include <deque>
#include <vector>

namespace Struktur::World
{
struct ElevationLimits
{
	double spacingMeters          = 1.0;
	double maxGradePercent        = 12.0;
	double maxGradeChangePerMeter = 0.05;  // % per metre
};

// The road's height h(s), kept separate from the horizontal route; grade = dh/ds (in %).
//
// Everything behind the rider is history and never changes. Every frame the part ahead is re-derived from a target
// grade for each metre ahead (the rider's set grade in SIM, the workout's intensity in ERG, or the route profile),
// starting from the grade under the rider and limited to maxGradeChangePerMeter, so grade changes are always smooth
// and the grade at the rider only changes as the rider rides into the transition. Re-deriving with an unchanged
// target reproduces the same road (the limiter is causal), so nothing ahead wobbles while targets are steady.
class ElevationProfile
{
public:
	explicit ElevationProfile(const ElevationLimits& limits = ElevationLimits());

	void SetLimits(const ElevationLimits& limits);
	const ElevationLimits& GetLimits() const;
	// Starts again at `distance` with height 0 and grade `gradePercent`.
	void Reset(double distance, double gradePercent);
	// riderDistance: where the rider is now. targetGrades[i] is the wanted grade (%) at riderDistance + i * spacing
	// (i = 0 is ignored: the rider's own grade is history). keepBehindMeters of history are retained.
	void Update(double riderDistance, const std::vector<double>& targetGrades, double keepBehindMeters);

	double GetStart() const;
	double GetEnd() const;
	double HeightAt(double distance) const;   // clamped to the stored range
	double GradeAt(double distance) const;    // %
	// Heights relative to `origin`, sampled every spacing from `firstDistance` - for the GPU height texture.
	void FillRelativeHeights(double firstDistance, double origin, size_t count, std::vector<float>& out) const;

private:
	ElevationLimits m_limits;
	double m_start;  // distance of m_heights[0]
	std::deque<double> m_heights;
	std::deque<double> m_grades;
};
}  // namespace Struktur::World
