#include "CadenceCalculator.h"

namespace Struktur::Fitness
{
CadenceCalculator::CadenceCalculator(double staleAfterSeconds)
    : m_staleAfterSeconds(staleAfterSeconds),
      m_hasBaseline(false),
      m_lastRevolutions(0),
      m_lastEventTime(0),
      m_lastEventHostSeconds(0.0),
      m_cadence()
{
}

std::optional<double> CadenceCalculator::AddSample(uint16_t cumulativeRevolutions, uint16_t lastEventTime1024,
                                                   double nowSeconds)
{
	// After a long pause the event-time counter may have wrapped more than once: start over from this sample.
	const bool gapTooLong = m_hasBaseline && nowSeconds - m_lastEventHostSeconds > MAX_EVENT_GAP_SECONDS;
	if (!m_hasBaseline || gapTooLong)
	{
		if (gapTooLong)
		{
			m_cadence = 0.0;
		}
		m_hasBaseline          = true;
		m_lastRevolutions      = cumulativeRevolutions;
		m_lastEventTime        = lastEventTime1024;
		m_lastEventHostSeconds = nowSeconds;
		return m_cadence;
	}

	// uint16 subtraction wraps, which is exactly the rollover handling we want.
	const uint16_t revolutionDelta = static_cast<uint16_t>(cumulativeRevolutions - m_lastRevolutions);
	const uint16_t timeDelta       = static_cast<uint16_t>(lastEventTime1024 - m_lastEventTime);

	if (revolutionDelta == 0 && timeDelta == 0)
	{
		// Same crank event re-reported: nothing new.
		return GetCadence(nowSeconds);
	}

	m_lastRevolutions = cumulativeRevolutions;
	m_lastEventTime   = lastEventTime1024;

	if (revolutionDelta == 0 || timeDelta == 0)
	{
		// Time moved without revolutions (or the reverse): inconsistent sample, re-baseline only.
		return GetCadence(nowSeconds);
	}

	const double rpm = revolutionDelta * 60.0 * 1024.0 / timeDelta;
	if (rpm > MAX_PLAUSIBLE_RPM)
	{
		return GetCadence(nowSeconds);
	}

	m_cadence              = rpm;
	m_lastEventHostSeconds = nowSeconds;
	return m_cadence;
}

std::optional<double> CadenceCalculator::GetCadence(double nowSeconds) const
{
	if (m_cadence && nowSeconds - m_lastEventHostSeconds > m_staleAfterSeconds)
	{
		return 0.0;
	}
	return m_cadence;
}

void CadenceCalculator::Reset()
{
	m_hasBaseline          = false;
	m_lastRevolutions      = 0;
	m_lastEventTime        = 0;
	m_lastEventHostSeconds = 0.0;
	m_cadence.reset();
}
}  // namespace Struktur::Fitness
