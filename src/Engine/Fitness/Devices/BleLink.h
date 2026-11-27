#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Fitness/Ble/IBleCentral.h"
#include "Engine/Fitness/Core/Backoff.h"
#include "Engine/Fitness/Core/ThreadSafeQueue.h"
#include "Engine/Fitness/Devices/DeviceTypes.h"

namespace Struktur::Fitness
{
// One step of post-connect setup, run in order after discovery.
struct LinkSetupStep
{
	enum class Kind : uint8_t
	{
		READ,
		SUBSCRIBE,
	};

	Kind kind = Kind::READ;
	BleCharacteristicRef characteristic;
	SubscriptionKind subscription = SubscriptionKind::NOTIFY;
	bool required                 = true;  // missing/failed optional steps are skipped, required ones fail the attempt
};

struct BleLinkConfig
{
	BleAddress address = 0;
	std::vector<LinkSetupStep> setup;
	ReconnectPolicy reconnect;
	double setupTimeoutSeconds = 20.0;  // connect + discover + every setup step
};

enum class LinkState : uint8_t
{
	IDLE,
	CONNECTING,
	DISCOVERING,
	SETTING_UP,
	READY,
	WAITING_TO_RECONNECT,
	FAILED,
	STOPPED,
};

struct LinkEvent
{
	enum class Type : uint8_t
	{
		READY,            // setup finished; data flows and writes are allowed
		DISCONNECTED,     // a READY link dropped
		RETRY_SCHEDULED,  // an attempt failed or a link dropped; retrying after retryDelaySeconds
		FAILED,           // reconnect attempts exhausted
		READ_VALUE,       // a setup READ completed
		NOTIFICATION,     // notification or indication value
		WRITE_COMPLETE,
	};

	Type type = Type::READY;
	BleCharacteristicRef characteristic;
	Bytes value;
	BleStatus status         = BleStatus::SUCCESS;
	uint32_t writeId         = 0;
	double retryDelaySeconds = 0.0;
	std::string detail;
};

// Game-thread state machine for one BLE peripheral: connect -> discover -> setup steps -> READY, with automatic
// reconnect (exponential backoff) when an attempt fails or an established link drops.
//
// All central callbacks land in a thread-safe inbox tagged with a connection generation; Update() drains it on the
// game thread, so device logic never runs on a BLE thread and late callbacks from an abandoned attempt are ignored.
//
// Holds a pointer to the central: the central must outlive every link (FitnessManager guarantees this).
class BleLink
{
public:
	BleLink(IBleCentral& central, BleLinkConfig config);
	~BleLink();

	BleLink(const BleLink&)            = delete;
	BleLink& operator=(const BleLink&) = delete;

	void Start(double now);
	// Releases the link immediately (central Disconnect). Terminal until Start() is called again.
	void Stop();

	std::vector<LinkEvent> Update(double now);

	LinkState GetState() const;
	ConnectionState GetConnectionState() const;
	bool IsReady() const;
	BleAddress GetAddress() const;
	// Valid once discovery has completed for the current attempt.
	bool HasCharacteristic(const BleCharacteristicRef& characteristic) const;

	// Only valid while READY. Returns an id echoed by the matching WRITE_COMPLETE event.
	uint32_t Write(const BleCharacteristicRef& characteristic, Bytes value, bool withResponse);

private:
	struct Message
	{
		enum class Kind : uint8_t
		{
			CONNECTED,
			DISCOVERED,
			READ_DONE,
			SUBSCRIBED,
			VALUE,
			WRITE_DONE,
			LINK_LOST,
		};

		Kind kind           = Kind::CONNECTED;
		uint32_t generation = 0;
		BleStatus status    = BleStatus::SUCCESS;
		BleCharacteristicRef characteristic;
		Bytes value;
		std::vector<BleServiceInfo> services;
		uint32_t writeId = 0;
	};

	using Inbox = ThreadSafeQueue<Message>;

	void BeginAttempt(double now);
	void RunNextSetupStep(double now, std::vector<LinkEvent>& events);
	void HandleMessage(Message& message, double now, std::vector<LinkEvent>& events);
	void FailAttempt(const std::string& reason, double now, std::vector<LinkEvent>& events);
	void Release();

	IBleCentral* m_central;
	BleLinkConfig m_config;
	LinkState m_state;
	uint32_t m_generation;
	// Shared on purpose: the central's callbacks capture it and can fire on another thread after this link is gone (a
	// late disconnect or write result), so the queue must outlive whichever goes last.
	std::shared_ptr<Inbox> m_inbox;
	Backoff m_backoff;
	double m_retryAtSeconds;
	double m_attemptStartedSeconds;
	size_t m_setupIndex;
	std::vector<BleServiceInfo> m_services;
	uint32_t m_nextWriteId;
	bool m_centralHasLink;
};
}  // namespace Struktur::Fitness
