#pragma once

#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <vector>

#include "Engine/Fitness/Protocol/Ftms.h"

namespace Struktur::Fitness
{
// How a control point procedure ended.
struct ControlPointOutcome
{
	enum class Kind : uint8_t
	{
		RESPONSE,      // the trainer indicated a result code (see `result`)
		TIMEOUT,       // no indication within the timeout
		WRITE_FAILED,  // the GATT write itself failed
		SUPERSEDED,    // replaced in the queue by a newer request of the same op code before it was sent
		CANCELLED,     // dropped (disconnect, control lost, shutdown)
	};

	uint32_t requestId      = 0;
	Ftms::OpCode opCode     = Ftms::OpCode::REQUEST_CONTROL;
	Kind kind               = Kind::RESPONSE;
	Ftms::ResultCode result = Ftms::ResultCode::OPERATION_FAILED;
	Bytes responseParameters;

	bool IsSuccess() const
	{
		return kind == Kind::RESPONSE && result == Ftms::ResultCode::SUCCESS;
	}
};

struct ControlPointSend
{
	uint32_t requestId = 0;
	Bytes payload;
};

// FTMS allows ONE outstanding control point procedure: write a command, then wait for its Response Code indication
// before writing the next. This queue enforces that, matches indications to the in-flight request, times out a
// procedure the trainer never answers, and coalesces target updates (a newer Set Target Power replaces a queued,
// not-yet-sent one) so a game streaming targets cannot build a backlog.
//
// Pure: no BLE, host time passed in.
class ControlPointQueue
{
public:
	explicit ControlPointQueue(double timeoutSeconds = 5.0);

	uint32_t Enqueue(Ftms::OpCode opCode, Bytes payload, bool coalesce);

	// The next command to write, if nothing is in flight. Marks it in flight from `now`.
	std::optional<ControlPointSend> TakeNextToSend(double now);

	// Feed every control point indication. Returns false if it did not match the in-flight request.
	bool OnIndication(std::span<const uint8_t> data);
	void OnWriteFailed(uint32_t requestId);
	void Update(double now);

	// Cancels everything not yet sent. The in-flight request (if any) still completes normally.
	void CancelQueued();
	// Cancels everything, including the in-flight request (use on disconnect).
	void CancelAll();

	std::vector<ControlPointOutcome> TakeOutcomes();

	bool IsIdle() const;
	bool HasInFlight() const;
	size_t GetQueuedCount() const;

private:
	struct Request
	{
		uint32_t id         = 0;
		Ftms::OpCode opCode = Ftms::OpCode::REQUEST_CONTROL;
		Bytes payload;
		double sentAtSeconds = 0.0;
	};

	void Complete(const Request& request, ControlPointOutcome::Kind kind,
	              Ftms::ResultCode result = Ftms::ResultCode::OPERATION_FAILED, Bytes parameters = {});

	double m_timeoutSeconds;
	uint32_t m_nextId;
	std::deque<Request> m_queued;
	std::optional<Request> m_inFlight;
	std::vector<ControlPointOutcome> m_outcomes;
};
}  // namespace Struktur::Fitness
