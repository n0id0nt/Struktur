#pragma once

#include <cstdint>
#include <optional>

namespace Struktur::Fitness
{
// Derives cadence from Cycling Power crank revolution data (cumulative revolutions + last crank event time, both
// uint16 that roll over). Pure: host time is passed in, so it is deterministic under test.
//
// Rules:
//  - The first sample only establishes a baseline.
//  - Deltas are taken modulo 2^16, so one rollover between samples is handled. A sensor that reports less often
//    than every ~64 s (the event-time wrap period) cannot be disambiguated and is not supported.
//  - No new crank event (both deltas zero) keeps the last cadence until staleAfterSeconds of host time pass with no
//    new event, then cadence drops to 0 (rider stopped pedalling).
//  - Implausible results (> MAX_PLAUSIBLE_RPM, or revolutions without elapsed event time) are discarded and only
//    re-baseline; this covers sensor resets and duplicate packets.
class CadenceCalculator
{
public:
	static constexpr double MAX_PLAUSIBLE_RPM = 250.0;
	// The 1/1024 s event clock wraps every 64 s; beyond this gap a delta is ambiguous, so re-baseline instead.
	static constexpr double MAX_EVENT_GAP_SECONDS = 60.0;

	explicit CadenceCalculator(double staleAfterSeconds = 3.0);

	// Returns the cadence after this sample (nullopt until a second sample has produced a value).
	std::optional<double> AddSample(uint16_t cumulativeRevolutions, uint16_t lastEventTime1024, double nowSeconds);

	// Cadence as of nowSeconds - applies the staleness rule without a new sample.
	std::optional<double> GetCadence(double nowSeconds) const;

	void Reset();

private:
	double m_staleAfterSeconds;
	bool m_hasBaseline;
	uint16_t m_lastRevolutions;
	uint16_t m_lastEventTime;
	double m_lastEventHostSeconds;
	std::optional<double> m_cadence;
};
}  // namespace Struktur::Fitness
