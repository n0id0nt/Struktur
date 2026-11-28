#include "ElevationProfile.h"

#include <algorithm>
#include <cmath>

namespace Struktur::World
{
ElevationProfile::ElevationProfile(const ElevationLimits& limits)
    : m_limits(limits),
      m_start(0.0)
{
	Reset(0.0, 0.0);
}

void ElevationProfile::SetLimits(const ElevationLimits& limits)
{
	m_limits = limits;
}

const ElevationLimits& ElevationProfile::GetLimits() const
{
	return m_limits;
}

void ElevationProfile::Reset(double distance, double gradePercent)
{
	m_start = std::floor(distance / m_limits.spacingMeters) * m_limits.spacingMeters;
	m_heights.assign(1, 0.0);
	m_grades.assign(1, std::clamp(gradePercent, -m_limits.maxGradePercent, m_limits.maxGradePercent));
}

void ElevationProfile::Update(double riderDistance, const std::vector<double>& targetGrades, double keepBehindMeters)
{
	const double spacing = m_limits.spacingMeters;
	// Drop history beyond keepBehindMeters.
	while (m_heights.size() > 2 && m_start + spacing < riderDistance - keepBehindMeters)
	{
		m_heights.pop_front();
		m_grades.pop_front();
		m_start += spacing;
	}
	// Extend history up to the rider with whatever was ahead last frame (it is now fixed).
	const int64_t riderIndex = static_cast<int64_t>(std::floor((riderDistance - m_start) / spacing));
	while (static_cast<int64_t>(m_heights.size()) <= riderIndex + 1)
	{
		const double grade = m_grades.back();
		m_heights.push_back(m_heights.back() + grade * spacing / 100.0);
		m_grades.push_back(grade);
	}
	// Re-derive everything after the sample just ahead of the rider.
	const size_t firstFree = static_cast<size_t>(riderIndex + 2);
	m_heights.resize(firstFree);
	m_grades.resize(firstFree);
	const double maxStep = m_limits.maxGradeChangePerMeter * spacing;
	for (size_t offset = 2; offset < targetGrades.size(); ++offset)
	{
		const double previous = m_grades.back();
		const double wanted   = std::clamp(targetGrades[offset], -m_limits.maxGradePercent, m_limits.maxGradePercent);
		const double grade    = previous + std::clamp(wanted - previous, -maxStep, maxStep);
		m_heights.push_back(m_heights.back() + previous * spacing / 100.0);
		m_grades.push_back(grade);
	}
}

double ElevationProfile::GetStart() const
{
	return m_start;
}

double ElevationProfile::GetEnd() const
{
	return m_start + static_cast<double>(m_heights.size() - 1) * m_limits.spacingMeters;
}

double ElevationProfile::HeightAt(double distance) const
{
	const double position = std::clamp((distance - m_start) / m_limits.spacingMeters, 0.0,
	                                   static_cast<double>(m_heights.size() - 1));
	const size_t index    = static_cast<size_t>(std::floor(position));
	const size_t next     = std::min(index + 1, m_heights.size() - 1);
	const double weight   = position - static_cast<double>(index);
	return m_heights[index] + (m_heights[next] - m_heights[index]) * weight;
}

double ElevationProfile::GradeAt(double distance) const
{
	const double position = std::clamp((distance - m_start) / m_limits.spacingMeters, 0.0,
	                                   static_cast<double>(m_grades.size() - 1));
	const size_t index    = static_cast<size_t>(std::floor(position));
	const size_t next     = std::min(index + 1, m_grades.size() - 1);
	const double weight   = position - static_cast<double>(index);
	return m_grades[index] + (m_grades[next] - m_grades[index]) * weight;
}

void ElevationProfile::FillRelativeHeights(double firstDistance, double origin, size_t count,
                                           std::vector<float>& out) const
{
	out.resize(count);
	for (size_t index = 0; index < count; ++index)
	{
		out[index] =
		    static_cast<float>(HeightAt(firstDistance + static_cast<double>(index) * m_limits.spacingMeters) - origin);
	}
}
}  // namespace Struktur::World
