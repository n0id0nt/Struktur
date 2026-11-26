#include "RideLog.h"

#include <algorithm>
#include <cmath>

namespace Struktur::Ride
{
void RideLog::Start(int64_t unixTime, const std::string& firstLapLabel)
{
	m_started   = true;
	m_startUnix = unixTime;
	m_endUnix   = unixTime;
	m_endTimer  = 0.0;
	m_samples.clear();
	m_laps.clear();
	m_timerEvents.clear();
	m_timerEvents.push_back(TimerEvent{unixTime, true});
	RideLap lap;
	lap.label     = firstLapLabel;
	lap.startUnix = unixTime;
	m_laps.push_back(lap);
}

void RideLog::AddSample(const RideSample& sample)
{
	m_samples.push_back(sample);
	m_endUnix  = std::max(m_endUnix, sample.unixTime);
	m_endTimer = sample.timerSeconds;
	if (!m_laps.empty())
	{
		m_laps.back().endSample = m_samples.size();
		m_laps.back().endUnix   = sample.unixTime;
		m_laps.back().endTimer  = sample.timerSeconds;
	}
}

void RideLog::NewLap(const std::string& label, int64_t unixTime, double timerSeconds)
{
	if (!m_laps.empty())
	{
		RideLap& open  = m_laps.back();
		open.endSample = m_samples.size();
		open.endUnix   = unixTime;
		open.endTimer  = timerSeconds;
		open.closed    = true;
	}
	RideLap lap;
	lap.label       = label;
	lap.firstSample = m_samples.size();
	lap.endSample   = m_samples.size();
	lap.startUnix   = unixTime;
	lap.endUnix     = unixTime;
	lap.startTimer  = timerSeconds;
	lap.endTimer    = timerSeconds;
	m_laps.push_back(lap);
}

void RideLog::Close(int64_t unixTime, double timerSeconds)
{
	m_endUnix  = std::max(m_endUnix, unixTime);
	m_endTimer = timerSeconds;
	if (!m_laps.empty())
	{
		RideLap& open  = m_laps.back();
		open.endSample = m_samples.size();
		open.endUnix   = m_endUnix;
		open.endTimer  = timerSeconds;
		open.closed    = true;
	}
}

void RideLog::AddTimerEvent(int64_t unixTime, bool started)
{
	m_timerEvents.push_back(TimerEvent{unixTime, started});
}

bool RideLog::IsStarted() const
{
	return m_started;
}

int64_t RideLog::GetStartUnix() const
{
	return m_startUnix;
}

int64_t RideLog::GetEndUnix() const
{
	return m_endUnix;
}

const std::vector<RideSample>& RideLog::GetSamples() const
{
	return m_samples;
}

const std::vector<RideLap>& RideLog::GetLaps() const
{
	return m_laps;
}

const std::vector<TimerEvent>& RideLog::GetTimerEvents() const
{
	return m_timerEvents;
}

RideSummary RideLog::Summarize(size_t firstSample, size_t endSample, int64_t startUnix, int64_t endUnix,
                               double timerSeconds) const
{
	RideSummary summary;
	summary.timerSeconds   = std::max(0.0, timerSeconds);
	summary.elapsedSeconds = (double)std::max<int64_t>(0, endUnix - startUnix);
	endSample              = std::min(endSample, m_samples.size());
	if (firstSample >= endSample)
	{
		return summary;
	}

	double powerTotal          = 0.0;
	int powerCount             = 0;
	double heartTotal          = 0.0;
	int heartCount             = 0;
	double cadenceTotal        = 0.0;
	int cadenceCount           = 0;
	const double startDistance = firstSample > 0 ? m_samples[firstSample - 1].distanceMeters : 0.0;
	double previousAltitude    = firstSample > 0 ? m_samples[firstSample - 1].altitudeMeters : 0.0;

	for (size_t i = firstSample; i < endSample; ++i)
	{
		const RideSample& sample = m_samples[i];
		summary.maximumSpeed     = std::max(summary.maximumSpeed, sample.speedMetersPerSecond);
		if (sample.altitudeMeters > previousAltitude)
		{
			summary.ascentMeters += sample.altitudeMeters - previousAltitude;
		}
		previousAltitude = sample.altitudeMeters;
		if (sample.powerWatts)
		{
			powerTotal += *sample.powerWatts;
			++powerCount;
			summary.maximumPower = std::max(summary.maximumPower.value_or(0), *sample.powerWatts);
		}
		if (sample.heartRateBpm)
		{
			heartTotal += *sample.heartRateBpm;
			++heartCount;
			summary.maximumHeartRate = std::max(summary.maximumHeartRate.value_or(0), *sample.heartRateBpm);
		}
		if (sample.cadenceRpm && *sample.cadenceRpm > 0)
		{
			cadenceTotal += *sample.cadenceRpm;
			++cadenceCount;
		}
		if (sample.targetWatts)
		{
			++summary.targetSeconds;
			if (sample.inTarget)
			{
				++summary.inTargetSeconds;
			}
		}
	}
	summary.distanceMeters = std::max(0.0, m_samples[endSample - 1].distanceMeters - startDistance);
	summary.averageSpeed   = summary.timerSeconds > 0.0 ? summary.distanceMeters / summary.timerSeconds : 0.0;
	if (powerCount > 0)
	{
		summary.averagePower = (int)std::lround(powerTotal / powerCount);
	}
	if (heartCount > 0)
	{
		summary.averageHeartRate = (int)std::lround(heartTotal / heartCount);
	}
	if (cadenceCount > 0)
	{
		summary.averageCadence = (int)std::lround(cadenceTotal / cadenceCount);
	}
	return summary;
}

RideSummary RideLog::SummarizeLap(const RideLap& lap) const
{
	return Summarize(lap.firstSample, lap.endSample, lap.startUnix, lap.endUnix, lap.endTimer - lap.startTimer);
}

RideSummary RideLog::SummarizeRide() const
{
	return Summarize(0, m_samples.size(), m_startUnix, m_endUnix, m_endTimer);
}
}  // namespace Struktur::Ride
