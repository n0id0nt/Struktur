#include "Route.h"

#include <algorithm>
#include <cmath>

#include "World/Noise.h"

namespace Struktur::World
{
static constexpr double PI                 = 3.14159265358979323846;
static constexpr int DENSE_STEPS_PER_SEGMENT = 384;
// Tuned so the resampled spline's peak curvature stays just under 1/minCurveRadius (see RideWorldTests).
static constexpr double TURN_LIMIT_FACTOR = 3.8;
static constexpr int CURVATURE_STENCIL     = 2;
static constexpr double HEADING_MARGIN     = 0.88;  // heading difference across +/- this many samples

static double WrapAngle(double angle)
{
	while (angle > PI)
	{
		angle -= 2.0 * PI;
	}
	while (angle < -PI)
	{
		angle += 2.0 * PI;
	}
	return angle;
}

static double DegreesToRadians(double degrees)
{
	return degrees * PI / 180.0;
}

Route::Route(const RouteSettings& settings)
    : m_settings(settings),
      m_turnLimit(settings.controlSpacingMeters / (TURN_LIMIT_FACTOR * settings.minCurveRadiusMeters)),
      m_firstControl(-1),
      m_nextSegment(0),
      m_previousHeading(0.0),
      m_firstSample(0),
      m_carryLength(0.0),
      m_lastDense(),
      m_hasDense(false)
{
	m_settings.sampleSpacingMeters  = std::max(0.25, m_settings.sampleSpacingMeters);
	m_settings.controlSpacingMeters = std::max(20.0, m_settings.controlSpacingMeters);
	// Control -1 and 0 lie on the mean direction (+x) so the ride starts straight.
	m_controls.push_back(Point{-m_settings.controlSpacingMeters, 0.0});
	m_controls.push_back(Point{0.0, 0.0});
	m_previousHeading = 0.0;
}

const RouteSettings& Route::GetSettings() const
{
	return m_settings;
}

double Route::HeadingFor(int64_t controlIndex) const
{
	const double distance = static_cast<double>(controlIndex) * m_settings.controlSpacingMeters;
	double heading        = DegreesToRadians(m_settings.sweepAmplitudeDeg) *
	                 Fbm1D(m_settings.seed ^ 0x51u, distance / m_settings.sweepWavelengthMeters, 2);
	heading += DegreesToRadians(m_settings.wiggleAmplitudeDeg) *
	           Fbm1D(m_settings.seed ^ 0xA7u, distance / m_settings.wiggleWavelengthMeters, 2);
	// Occasional tight bend: a one-off kink, which the turn limit below shapes into the tightest allowed curve.
	if (HashUnit(m_settings.seed ^ 0xBEu, controlIndex) < m_settings.bendChance)
	{
		heading += DegreesToRadians(HashSigned(m_settings.seed ^ 0xBFu, controlIndex) * 70.0);
	}
	// Control headings stay a margin inside the limit: the spline between them overshoots a little.
	const double deviation = DegreesToRadians(m_settings.maxHeadingDeviationDeg * HEADING_MARGIN);
	return std::clamp(heading, -deviation, deviation);
}

void Route::AddControlPoint()
{
	const int64_t index = m_firstControl + static_cast<int64_t>(m_controls.size());
	// The first few controls ease in from straight.
	double heading = index <= 1 ? 0.0 : HeadingFor(index);
	const double turn = std::clamp(WrapAngle(heading - m_previousHeading), -m_turnLimit, m_turnLimit);
	heading           = m_previousHeading + turn;
	// Control headings stay a margin inside the limit: the spline between them overshoots a little.
	const double deviation = DegreesToRadians(m_settings.maxHeadingDeviationDeg * HEADING_MARGIN);
	heading                = std::clamp(heading, -deviation, deviation);
	m_previousHeading      = heading;
	const Point& last      = m_controls.back();
	m_controls.push_back(Point{last.x + std::cos(heading) * m_settings.controlSpacingMeters,
	                           last.z + std::sin(heading) * m_settings.controlSpacingMeters});
}

// Centripetal (alpha = 0.5) Catmull-Rom between p1 and p2, Barry-Goldman form.
static void CatmullRom(const double p0[2], const double p1[2], const double p2[2], const double p3[2], double t,
                       double out[2])
{
	const double t0 = 0.0;
	const double t1 = t0 + std::max(1e-6, std::sqrt(std::hypot(p1[0] - p0[0], p1[1] - p0[1])));
	const double t2 = t1 + std::max(1e-6, std::sqrt(std::hypot(p2[0] - p1[0], p2[1] - p1[1])));
	const double t3 = t2 + std::max(1e-6, std::sqrt(std::hypot(p3[0] - p2[0], p3[1] - p2[1])));
	const double u  = t1 + (t2 - t1) * t;
	for (int axis = 0; axis < 2; ++axis)
	{
		const double a1 = (t1 - u) / (t1 - t0) * p0[axis] + (u - t0) / (t1 - t0) * p1[axis];
		const double a2 = (t2 - u) / (t2 - t1) * p1[axis] + (u - t1) / (t2 - t1) * p2[axis];
		const double a3 = (t3 - u) / (t3 - t2) * p2[axis] + (u - t2) / (t3 - t2) * p3[axis];
		const double b1 = (t2 - u) / (t2 - t0) * a1 + (u - t0) / (t2 - t0) * a2;
		const double b2 = (t3 - u) / (t3 - t1) * a2 + (u - t1) / (t3 - t1) * a3;
		out[axis]       = (t2 - u) / (t2 - t1) * b1 + (u - t1) / (t2 - t1) * b2;
	}
}

const Route::Point& Route::ControlAt(int64_t index) const
{
	return m_controls[static_cast<size_t>(index - m_firstControl)];
}

void Route::EmitSegment()
{
	const int64_t segment = m_nextSegment;
	// Needs controls segment-1 .. segment+2.
	while (m_firstControl + static_cast<int64_t>(m_controls.size()) <= segment + 2)
	{
		AddControlPoint();
	}
	const double p0[2] = {ControlAt(segment - 1).x, ControlAt(segment - 1).z};
	const double p1[2] = {ControlAt(segment).x, ControlAt(segment).z};
	const double p2[2] = {ControlAt(segment + 1).x, ControlAt(segment + 1).z};
	const double p3[2] = {ControlAt(segment + 2).x, ControlAt(segment + 2).z};

	const double spacing = m_settings.sampleSpacingMeters;
	if (!m_hasDense)
	{
		m_lastDense = Point{p1[0], p1[1]};
		m_hasDense  = true;
		RouteSample first;
		first.x = p1[0];
		first.z = p1[1];
		m_samples.push_back(first);
		m_carryLength = 0.0;
	}
	const int64_t firstNew = m_firstSample + static_cast<int64_t>(m_samples.size()) - 1;
	for (int step = 1; step <= DENSE_STEPS_PER_SEGMENT; ++step)
	{
		double next[2];
		CatmullRom(p0, p1, p2, p3, static_cast<double>(step) / DENSE_STEPS_PER_SEGMENT, next);
		double fromX         = m_lastDense.x;
		double fromZ         = m_lastDense.z;
		double remaining     = std::hypot(next[0] - fromX, next[1] - fromZ);
		const double length  = remaining;
		if (length <= 0.0)
		{
			continue;
		}
		const double directionX = (next[0] - fromX) / length;
		const double directionZ = (next[1] - fromZ) / length;
		while (m_carryLength + remaining >= spacing)
		{
			const double advance = spacing - m_carryLength;
			fromX += directionX * advance;
			fromZ += directionZ * advance;
			remaining -= advance;
			m_carryLength = 0.0;
			RouteSample sample;
			sample.x        = fromX;
			sample.z        = fromZ;
			sample.tangentX = directionX;
			sample.tangentZ = directionZ;
			m_samples.push_back(sample);
		}
		m_carryLength += remaining;
		m_lastDense = Point{next[0], next[1]};
	}
	++m_nextSegment;
	UpdateCurvature(firstNew);
}

void Route::UpdateCurvature(int64_t fromIndex)
{
	const int64_t last  = m_firstSample + static_cast<int64_t>(m_samples.size()) - 1;
	const int64_t start = std::max(m_firstSample, fromIndex - CURVATURE_STENCIL - 1);
	// Tangents from neighbouring positions (central difference), then curvature from the heading change.
	for (int64_t index = std::max(start, m_firstSample + 1); index < last; ++index)
	{
		RouteSample& sample      = m_samples[static_cast<size_t>(index - m_firstSample)];
		const RouteSample& prev  = m_samples[static_cast<size_t>(index - 1 - m_firstSample)];
		const RouteSample& next  = m_samples[static_cast<size_t>(index + 1 - m_firstSample)];
		const double deltaX      = next.x - prev.x;
		const double deltaZ      = next.z - prev.z;
		const double length      = std::hypot(deltaX, deltaZ);
		if (length > 0.0)
		{
			sample.tangentX = deltaX / length;
			sample.tangentZ = deltaZ / length;
		}
	}
	const double spacing = m_settings.sampleSpacingMeters;
	for (int64_t index = std::max(start, m_firstSample + CURVATURE_STENCIL); index + CURVATURE_STENCIL < last; ++index)
	{
		const RouteSample& behind = m_samples[static_cast<size_t>(index - CURVATURE_STENCIL - m_firstSample)];
		const RouteSample& ahead  = m_samples[static_cast<size_t>(index + CURVATURE_STENCIL - m_firstSample)];
		const double headingBehind = std::atan2(behind.tangentZ, behind.tangentX);
		const double headingAhead  = std::atan2(ahead.tangentZ, ahead.tangentX);
		// Heading increases counter-clockwise in (x, z); with y up and left = (tz, -tx) a left bend turns clockwise
		// in that plane, so curvature is the negated heading rate.
		m_samples[static_cast<size_t>(index - m_firstSample)].curvature =
		    -WrapAngle(headingAhead - headingBehind) / (2.0 * CURVATURE_STENCIL * spacing);
	}
}

void Route::EnsureGenerated(double distanceMeters)
{
	// Keep a few samples of margin so curvature/tangents at the end are final.
	const double margin = (CURVATURE_STENCIL + 2) * m_settings.sampleSpacingMeters;
	while (GetEnd() < distanceMeters + margin)
	{
		EmitSegment();
	}
}

void Route::PruneBefore(double distanceMeters)
{
	const int64_t keepFrom = static_cast<int64_t>(std::floor(distanceMeters / m_settings.sampleSpacingMeters)) - 1;
	// Never prune into the samples the curvature pass still needs.
	const int64_t limit = m_firstSample + static_cast<int64_t>(m_samples.size()) - 8;
	while (m_firstSample < keepFrom && m_firstSample < limit && !m_samples.empty())
	{
		m_samples.pop_front();
		++m_firstSample;
	}
	// Controls well behind the next segment to emit are no longer needed.
	while (m_firstControl < m_nextSegment - 2 && m_controls.size() > 4)
	{
		m_controls.pop_front();
		++m_firstControl;
	}
}

double Route::GetStart() const
{
	return static_cast<double>(m_firstSample) * m_settings.sampleSpacingMeters;
}

double Route::GetEnd() const
{
	if (m_samples.empty())
	{
		return 0.0;
	}
	// The last few samples' curvature is provisional; report only the final part.
	const int64_t finalCount = static_cast<int64_t>(m_samples.size()) - CURVATURE_STENCIL - 2;
	return static_cast<double>(m_firstSample + std::max<int64_t>(0, finalCount)) * m_settings.sampleSpacingMeters;
}

int64_t Route::GetFirstIndex() const
{
	return m_firstSample;
}

int64_t Route::GetSampleCount() const
{
	return static_cast<int64_t>(m_samples.size());
}

const RouteSample& Route::GetSample(int64_t index) const
{
	const int64_t clamped =
	    std::clamp(index, m_firstSample, m_firstSample + static_cast<int64_t>(m_samples.size()) - 1);
	return m_samples[static_cast<size_t>(clamped - m_firstSample)];
}

RouteSample Route::SampleAt(double distanceMeters) const
{
	const double position = distanceMeters / m_settings.sampleSpacingMeters;
	const double cell     = std::floor(position);
	const int64_t index   = static_cast<int64_t>(cell);
	const double weight   = position - cell;
	const RouteSample& before = GetSample(index);
	const RouteSample& after  = GetSample(index + 1);
	RouteSample result;
	result.x            = before.x + (after.x - before.x) * weight;
	result.z            = before.z + (after.z - before.z) * weight;
	double tangentX     = before.tangentX + (after.tangentX - before.tangentX) * weight;
	double tangentZ     = before.tangentZ + (after.tangentZ - before.tangentZ) * weight;
	const double length = std::hypot(tangentX, tangentZ);
	if (length > 0.0)
	{
		tangentX /= length;
		tangentZ /= length;
	}
	result.tangentX  = tangentX;
	result.tangentZ  = tangentZ;
	result.curvature = before.curvature + (after.curvature - before.curvature) * weight;
	return result;
}

double Route::MaxCurvature(double from, double to) const
{
	const double spacing = m_settings.sampleSpacingMeters;
	const int64_t first  = std::max(m_firstSample, static_cast<int64_t>(std::floor(from / spacing)));
	const int64_t last =
	    std::min(m_firstSample + static_cast<int64_t>(m_samples.size()) - 1, static_cast<int64_t>(std::ceil(to / spacing)));
	double maximum = 0.0;
	for (int64_t index = first; index <= last; ++index)
	{
		maximum = std::max(maximum, std::abs(m_samples[static_cast<size_t>(index - m_firstSample)].curvature));
	}
	return maximum;
}

void Route::CopySamples(int64_t firstIndex, int64_t count, std::vector<RouteSample>& out) const
{
	out.resize(static_cast<size_t>(std::max<int64_t>(0, count)));
	for (int64_t offset = 0; offset < count; ++offset)
	{
		out[static_cast<size_t>(offset)] = GetSample(firstIndex + offset);
	}
}
}  // namespace Struktur::World
