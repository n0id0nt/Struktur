#include "ControlPointQueue.h"

#include <utility>

#include "Debug/Assertions.h"

namespace Struktur::Fitness
{
ControlPointQueue::ControlPointQueue(double timeoutSeconds)
    : m_timeoutSeconds(timeoutSeconds),
      m_nextId(0),
      m_queued(),
      m_inFlight(),
      m_outcomes()
{
}

uint32_t ControlPointQueue::Enqueue(Ftms::OpCode opCode, Bytes payload, bool coalesce)
{
	if (coalesce)
	{
		for (auto request = m_queued.begin(); request != m_queued.end();)
		{
			if (request->opCode == opCode)
			{
				Complete(*request, ControlPointOutcome::Kind::SUPERSEDED);
				request = m_queued.erase(request);
			}
			else
			{
				++request;
			}
		}
	}
	Request request;
	request.id      = ++m_nextId;
	request.opCode  = opCode;
	request.payload = std::move(payload);
	m_queued.push_back(std::move(request));
	return m_nextId;
}

std::optional<ControlPointSend> ControlPointQueue::TakeNextToSend(double now)
{
	if (m_inFlight || m_queued.empty())
	{
		return std::nullopt;
	}
	m_inFlight = std::move(m_queued.front());
	m_queued.pop_front();
	m_inFlight->sentAtSeconds = now;
	return ControlPointSend{m_inFlight->id, m_inFlight->payload};
}

bool ControlPointQueue::OnIndication(std::span<const uint8_t> data)
{
	const std::optional<Ftms::ControlPointResponse> response = Ftms::ParseControlPointResponse(data);
	if (!response)
	{
		DEBUG_WARNING("Ignoring malformed FTMS control point indication (%zu bytes)", data.size());
		return false;
	}
	if (!m_inFlight || response->requestOpCode != static_cast<uint8_t>(m_inFlight->opCode))
	{
		// Late answer to a timed-out request, or unsolicited - either way nothing is waiting for it.
		DEBUG_WARNING("Ignoring unexpected FTMS response for op code 0x%02X", response->requestOpCode);
		return false;
	}
	const Request request = std::move(*m_inFlight);
	m_inFlight.reset();
	Complete(request, ControlPointOutcome::Kind::RESPONSE, response->result, response->parameters);
	return true;
}

void ControlPointQueue::OnWriteFailed(uint32_t requestId)
{
	if (!m_inFlight || m_inFlight->id != requestId)
	{
		return;
	}
	const Request request = std::move(*m_inFlight);
	m_inFlight.reset();
	Complete(request, ControlPointOutcome::Kind::WRITE_FAILED);
}

void ControlPointQueue::Update(double now)
{
	if (m_inFlight && now - m_inFlight->sentAtSeconds > m_timeoutSeconds)
	{
		const Request request = std::move(*m_inFlight);
		m_inFlight.reset();
		Complete(request, ControlPointOutcome::Kind::TIMEOUT);
	}
}

void ControlPointQueue::CancelQueued()
{
	for (const Request& request : m_queued)
	{
		Complete(request, ControlPointOutcome::Kind::CANCELLED);
	}
	m_queued.clear();
}

void ControlPointQueue::CancelAll()
{
	if (m_inFlight)
	{
		const Request request = std::move(*m_inFlight);
		m_inFlight.reset();
		Complete(request, ControlPointOutcome::Kind::CANCELLED);
	}
	CancelQueued();
}

std::vector<ControlPointOutcome> ControlPointQueue::TakeOutcomes()
{
	return std::exchange(m_outcomes, {});
}

bool ControlPointQueue::IsIdle() const
{
	return !m_inFlight && m_queued.empty();
}

bool ControlPointQueue::HasInFlight() const
{
	return m_inFlight.has_value();
}

size_t ControlPointQueue::GetQueuedCount() const
{
	return m_queued.size();
}

void ControlPointQueue::Complete(const Request& request, ControlPointOutcome::Kind kind, Ftms::ResultCode result,
                                 Bytes parameters)
{
	ControlPointOutcome outcome;
	outcome.requestId          = request.id;
	outcome.opCode             = request.opCode;
	outcome.kind               = kind;
	outcome.result             = result;
	outcome.responseParameters = std::move(parameters);
	m_outcomes.push_back(std::move(outcome));
}
}  // namespace Struktur::Fitness
