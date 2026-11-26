#pragma once

namespace Struktur::Fitness
{
struct ReconnectPolicy
{
	double initialDelaySeconds = 1.0;
	double maximumDelaySeconds = 30.0;
	double multiplier          = 2.0;
	int maximumAttempts        = -1;  // -1 = keep trying forever
};

// Exponential reconnect delay: initial, initial*m, initial*m^2 ... capped at maximum. Pure and deterministic (no
// jitter - with at most three devices per PC there is no thundering herd to spread out).
class Backoff
{
public:
	explicit Backoff(const ReconnectPolicy& policy = ReconnectPolicy())
	    : m_policy(policy),
	      m_attempts(0)
	{
	}

	// Delay before the next attempt; counts the attempt.
	double NextDelay()
	{
		double delay = m_policy.initialDelaySeconds;
		for (int i = 0; i < m_attempts && delay < m_policy.maximumDelaySeconds; ++i)
		{
			delay *= m_policy.multiplier;
		}
		++m_attempts;
		return delay < m_policy.maximumDelaySeconds ? delay : m_policy.maximumDelaySeconds;
	}

	bool IsExhausted() const
	{
		return m_policy.maximumAttempts >= 0 && m_attempts >= m_policy.maximumAttempts;
	}

	int GetAttempts() const
	{
		return m_attempts;
	}

	void Reset()
	{
		m_attempts = 0;
	}

private:
	ReconnectPolicy m_policy;
	int m_attempts;
};
}  // namespace Struktur::Fitness
