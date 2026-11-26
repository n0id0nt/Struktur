#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Struktur::Ride
{
// One second of riding (only recorded while the ride timer runs).
struct RideSample
{
	int64_t unixTime    = 0;    // wall clock (UTC seconds)
	double timerSeconds = 0.0;  // moving time at this sample
	std::optional<int> powerWatts;
	std::optional<int> heartRateBpm;
	std::optional<int> cadenceRpm;
	double speedMetersPerSecond = 0.0;
	double distanceMeters       = 0.0;
	double altitudeMeters       = 0.0;
	std::optional<int> targetWatts;  // workout/ERG target at this second
	double gradePercent = 0.0;
	bool ergEnabled     = false;
	bool inTarget       = false;  // power inside the target band (only meaningful when targetWatts is set)
};

struct RideLap
{
	std::string label;
	size_t firstSample = 0;  // [firstSample, endSample)
	size_t endSample   = 0;
	int64_t startUnix  = 0;
	int64_t endUnix    = 0;
	double startTimer  = 0.0;
	double endTimer    = 0.0;
	bool closed        = false;
};

struct TimerEvent
{
	int64_t unixTime = 0;
	bool started     = true;  // false = stopped (pause)
};

struct RideSummary
{
	double timerSeconds   = 0.0;  // moving time
	double elapsedSeconds = 0.0;  // wall time including pauses
	double distanceMeters = 0.0;
	double ascentMeters   = 0.0;
	double averageSpeed   = 0.0;
	double maximumSpeed   = 0.0;
	std::optional<int> averagePower;  // includes zeros (coasting), like head units
	std::optional<int> maximumPower;
	std::optional<int> averageHeartRate;
	std::optional<int> maximumHeartRate;
	std::optional<int> averageCadence;  // excludes zeros
	int targetSeconds   = 0;            // seconds that had a target
	int inTargetSeconds = 0;
};

// The ride's raw log: 1 Hz samples, laps and timer start/stop events. Everything the FIT and JSON writers need.
class RideLog
{
public:
	void Start(int64_t unixTime, const std::string& firstLapLabel);
	void AddSample(const RideSample& sample);
	// Closes the open lap at the last sample and opens a new one.
	void NewLap(const std::string& label, int64_t unixTime, double timerSeconds);
	void Close(int64_t unixTime, double timerSeconds);
	void AddTimerEvent(int64_t unixTime, bool started);

	bool IsStarted() const;
	int64_t GetStartUnix() const;
	int64_t GetEndUnix() const;
	const std::vector<RideSample>& GetSamples() const;
	const std::vector<RideLap>& GetLaps() const;
	const std::vector<TimerEvent>& GetTimerEvents() const;

	// Summary of samples [first, end), with elapsed time from startUnix to endUnix.
	RideSummary Summarize(size_t firstSample, size_t endSample, int64_t startUnix, int64_t endUnix,
	                      double timerSeconds) const;
	RideSummary SummarizeLap(const RideLap& lap) const;
	RideSummary SummarizeRide() const;

private:
	bool m_started      = false;
	int64_t m_startUnix = 0;
	int64_t m_endUnix   = 0;
	double m_endTimer   = 0.0;
	std::vector<RideSample> m_samples;
	std::vector<RideLap> m_laps;
	std::vector<TimerEvent> m_timerEvents;
};
}  // namespace Struktur::Ride
