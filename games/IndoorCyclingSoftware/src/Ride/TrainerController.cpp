#include "TrainerController.h"

#include <algorithm>
#include <cmath>

namespace Struktur::Ride
{
// Changes smaller than these aren't worth a control-point command.
static constexpr double POWER_SEND_THRESHOLD_WATTS   = 1.0;
static constexpr double GRADE_SEND_THRESHOLD_PERCENT = 0.05;

TrainerController::TrainerController(const TrainerControlSettings& settings,
                                     const SpeedModelParameters& speedParameters)
    : m_settings(settings),
      m_speedParameters(speedParameters),
      m_ergEnabled(false),
      m_baseTarget(),
      m_trimWatts(0.0),
      m_trimPercent(0.0),
      m_gradePercent(0.0),
      m_minimumPower(settings.defaultMinimumPowerWatts),
      m_maximumPower(settings.defaultMaximumPowerWatts),
      m_minimumGrade(settings.defaultMinimumGradePercent),
      m_maximumGrade(settings.defaultMaximumGradePercent),
      m_mode(Mode::NONE),
      m_rampFrom(0.0),
      m_rampStart(-1.0e9),
      m_currentValue(0.0),
      m_lastSentValue(),
      m_lastSentMode(Mode::NONE),
      m_lastSentTime(-1.0e9),
      m_hasSent(false),
      m_protectionFactor(1.0),
      m_lowCadenceSince(),
      m_recoveredSince(),
      m_lastUpdate(0.0),
      m_hasUpdated(false)
{
}

void TrainerController::SetSettings(const TrainerControlSettings& settings)
{
	m_settings = settings;
}

void TrainerController::SetSpeedParameters(const SpeedModelParameters& speedParameters)
{
	m_speedParameters = speedParameters;
}

void TrainerController::SetErgEnabled(bool enabled)
{
	m_ergEnabled = enabled;
}

bool TrainerController::IsErgEnabled() const
{
	return m_ergEnabled;
}

void TrainerController::SetBaseTarget(std::optional<double> watts)
{
	m_baseTarget = watts;
}

std::optional<double> TrainerController::GetBaseTarget() const
{
	return m_baseTarget;
}

void TrainerController::AdjustResistance(int direction)
{
	const int step = direction > 0 ? 1 : (direction < 0 ? -1 : 0);
	if (m_ergEnabled)
	{
		if (m_settings.trimMode == TrimMode::WATTS)
		{
			m_trimWatts = std::clamp(m_trimWatts + step * m_settings.trimStepWatts, -m_settings.maxTrimWatts,
			                         m_settings.maxTrimWatts);
		}
		else
		{
			m_trimPercent = std::clamp(m_trimPercent + step * m_settings.trimStepPercent, -m_settings.maxTrimPercent,
			                           m_settings.maxTrimPercent);
		}
		return;
	}
	m_gradePercent = ClampGrade(m_gradePercent + step * m_settings.gradeStepPercent);
}

void TrainerController::SetTrimMode(TrimMode mode)
{
	m_settings.trimMode = mode;
}

TrimMode TrainerController::GetTrimMode() const
{
	return m_settings.trimMode;
}

double TrainerController::GetTrimWatts() const
{
	return m_trimWatts;
}

double TrainerController::GetTrimPercent() const
{
	return m_trimPercent;
}

void TrainerController::SetRoadGrade(std::optional<double> gradePercent)
{
	m_roadGrade = gradePercent;
}

std::optional<double> TrainerController::GetRoadGrade() const
{
	return m_roadGrade;
}

double TrainerController::GetGradePercent() const
{
	return m_gradePercent;
}

void TrainerController::SetGradePercent(double gradePercent)
{
	m_gradePercent = ClampGrade(gradePercent);
}

void TrainerController::SetCapabilities(const Fitness::TrainerCapabilities& capabilities)
{
	m_minimumPower = capabilities.minimumPowerWatts.value_or(m_settings.defaultMinimumPowerWatts);
	m_maximumPower = capabilities.maximumPowerWatts.value_or(m_settings.defaultMaximumPowerWatts);
	m_minimumGrade = capabilities.minimumGradePercent.value_or(m_settings.defaultMinimumGradePercent);
	m_maximumGrade = capabilities.maximumGradePercent.value_or(m_settings.defaultMaximumGradePercent);
	m_gradePercent = ClampGrade(m_gradePercent);
}

double TrainerController::GetMinimumGrade() const
{
	return m_minimumGrade;
}

double TrainerController::GetMaximumGrade() const
{
	return m_maximumGrade;
}

double TrainerController::ClampPower(double watts) const
{
	return std::clamp(watts, m_minimumPower, std::max(m_minimumPower, m_maximumPower));
}

double TrainerController::ClampGrade(double grade) const
{
	return std::clamp(grade, m_minimumGrade, std::max(m_minimumGrade, m_maximumGrade));
}

std::optional<double> TrainerController::DesiredErgTarget() const
{
	if (!m_ergEnabled || !m_baseTarget)
	{
		return std::nullopt;
	}
	double target = m_settings.trimMode == TrimMode::WATTS ? *m_baseTarget + m_trimWatts
	                                                       : *m_baseTarget * (1.0 + m_trimPercent / 100.0);
	return ClampPower(ClampPower(target) * m_protectionFactor);
}

void TrainerController::UpdateProtection(const ControlInputs& inputs, double deltaSeconds)
{
	const bool inErg = m_ergEnabled && m_baseTarget.has_value();
	if (!m_settings.cadenceProtection || !inErg || !inputs.cadenceRpm)
	{
		m_lowCadenceSince.reset();
		m_recoveredSince.reset();
		// Ease back to full whenever protection doesn't apply (ERG switched off, cadence sensor gone).
		m_protectionFactor = std::min(1.0, m_protectionFactor + deltaSeconds / std::max(0.1, m_settings.rampSeconds));
		return;
	}
	const double cadence = *inputs.cadenceRpm;
	const double easeRate =
	    (1.0 - m_settings.protectionMinimumFraction) / std::max(0.1, m_settings.protectionEaseSeconds);

	if (cadence < m_settings.lowCadenceRpm)
	{
		m_recoveredSince.reset();
		if (!m_lowCadenceSince)
		{
			m_lowCadenceSince = inputs.now;
		}
		if (inputs.now - *m_lowCadenceSince >= m_settings.lowCadenceSeconds)
		{
			m_protectionFactor =
			    std::max(m_settings.protectionMinimumFraction, m_protectionFactor - easeRate * deltaSeconds);
		}
		return;
	}

	m_lowCadenceSince.reset();
	if (m_protectionFactor >= 1.0)
	{
		return;
	}
	if (cadence >= m_settings.recoveryCadenceRpm)
	{
		if (!m_recoveredSince)
		{
			m_recoveredSince = inputs.now;
		}
		if (inputs.now - *m_recoveredSince >= m_settings.recoverySeconds)
		{
			// Restore over twice the ramp time - gentler than the ease-down.
			m_protectionFactor =
			    std::min(1.0, m_protectionFactor + deltaSeconds * (1.0 - m_settings.protectionMinimumFraction) /
			                                           std::max(0.1, 2.0 * m_settings.rampSeconds));
		}
	}
	else
	{
		m_recoveredSince.reset();  // in between: hold where we are
	}
}

void TrainerController::Update(const ControlInputs& inputs, Fitness::Trainer* trainer)
{
	const double deltaSeconds = m_hasUpdated ? std::max(0.0, inputs.now - m_lastUpdate) : 0.0;
	m_lastUpdate              = inputs.now;
	m_hasUpdated              = true;

	UpdateProtection(inputs, deltaSeconds);

	const std::optional<double> ergTarget = DesiredErgTarget();
	const Mode desiredMode                = ergTarget ? Mode::ERG : Mode::SIM;
	const double desiredValue             = ergTarget ? *ergTarget : ClampGrade(m_roadGrade.value_or(m_gradePercent));

	if (desiredMode != m_mode)
	{
		// Start the new mode from where the rider is now, so resistance doesn't jump.
		if (desiredMode == Mode::ERG)
		{
			m_rampFrom = inputs.powerWatts ? ClampPower(*inputs.powerWatts) : desiredValue;
		}
		else if (m_mode == Mode::ERG)
		{
			const double heldPower = inputs.powerWatts ? *inputs.powerWatts : m_currentValue;
			m_rampFrom =
			    ClampGrade(SpeedModel::EquivalentGrade(m_speedParameters, heldPower, inputs.speedMetersPerSecond));
		}
		else
		{
			m_rampFrom = desiredValue;  // first command: nothing to blend from
		}
		m_rampStart = inputs.now;
		m_mode      = desiredMode;
	}

	const double rampProgress =
	    m_settings.rampSeconds > 0.0 ? std::clamp((inputs.now - m_rampStart) / m_settings.rampSeconds, 0.0, 1.0) : 1.0;
	m_currentValue = m_rampFrom + (desiredValue - m_rampFrom) * rampProgress;

	if (trainer == nullptr || trainer->GetConnectionState() != Fitness::ConnectionState::CONNECTED)
	{
		// Nothing to talk to; when one appears it gets the current value on the next allowed tick.
		m_hasSent = m_hasSent && trainer != nullptr;
		return;
	}
	if (m_hasSent && inputs.now - m_lastSentTime < m_settings.minCommandIntervalSeconds)
	{
		return;
	}
	const double threshold = m_mode == Mode::ERG ? POWER_SEND_THRESHOLD_WATTS : GRADE_SEND_THRESHOLD_PERCENT;
	if (m_hasSent && m_lastSentMode == m_mode && m_lastSentValue &&
	    std::abs(*m_lastSentValue - m_currentValue) < threshold)
	{
		return;
	}

	if (m_mode == Mode::ERG)
	{
		const double watts = std::round(m_currentValue);
		trainer->SetTargetPower((int)watts);
		m_lastSentValue = watts;
	}
	else
	{
		Fitness::SimulationTarget target;
		target.gradePercent                 = std::round(m_currentValue * 100.0) / 100.0;
		target.rollingResistanceCoefficient = m_speedParameters.rollingResistance;
		// FTMS Cw (kg/m) is the aero term's 0.5 * rho * CdA.
		target.windResistanceCoefficient =
		    0.5 * m_speedParameters.airDensityKgPerCubicM * m_speedParameters.dragAreaSquareMeters;
		trainer->SetSimulation(target);
		m_lastSentValue = target.gradePercent;
	}
	m_lastSentMode = m_mode;
	m_lastSentTime = inputs.now;
	m_hasSent      = true;
}

TrainerController::Mode TrainerController::GetMode() const
{
	return m_mode;
}

std::optional<double> TrainerController::GetErgTargetWatts() const
{
	return DesiredErgTarget();
}

std::optional<double> TrainerController::GetLastSentValue() const
{
	return m_lastSentValue;
}

double TrainerController::GetEffectiveGradePercent() const
{
	return m_mode == Mode::SIM ? m_currentValue : ClampGrade(m_roadGrade.value_or(m_gradePercent));
}

bool TrainerController::IsRamping(double now) const
{
	return m_settings.rampSeconds > 0.0 && now - m_rampStart < m_settings.rampSeconds;
}

bool TrainerController::IsCadenceProtectionActive() const
{
	return m_protectionFactor < 0.999;
}

double TrainerController::GetProtectionFactor() const
{
	return m_protectionFactor;
}
}  // namespace Struktur::Ride
