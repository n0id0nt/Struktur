#include "BleLink.h"

#include <cstdio>

#include "Debug/Assertions.h"

namespace Struktur::Fitness
{
static std::string DescribeCharacteristic(const BleCharacteristicRef& characteristic)
{
	const std::optional<uint16_t> shortUuid = characteristic.characteristic.GetShort();
	if (shortUuid)
	{
		char buffer[8];
		std::snprintf(buffer, sizeof(buffer), "0x%04X", *shortUuid);
		return std::string(buffer);
	}
	return characteristic.characteristic.ToString();
}

BleLink::BleLink(IBleCentral& central, BleLinkConfig config)
    : m_central(&central),
      m_config(std::move(config)),
      m_state(LinkState::IDLE),
      m_generation(0),
      m_inbox(std::make_shared<Inbox>()),
      m_backoff(m_config.reconnect),
      m_retryAtSeconds(0.0),
      m_attemptStartedSeconds(0.0),
      m_setupIndex(0),
      m_services(),
      m_nextWriteId(0),
      m_centralHasLink(false)
{
}

BleLink::~BleLink()
{
	Release();
}

void BleLink::Start(double now)
{
	if (m_state != LinkState::IDLE && m_state != LinkState::STOPPED && m_state != LinkState::FAILED)
	{
		return;
	}
	m_backoff.Reset();
	BeginAttempt(now);
}

void BleLink::Stop()
{
	Release();
	++m_generation;
	m_state = LinkState::STOPPED;
}

std::vector<LinkEvent> BleLink::Update(double now)
{
	std::vector<LinkEvent> events;

	if (m_state == LinkState::WAITING_TO_RECONNECT && now >= m_retryAtSeconds)
	{
		BeginAttempt(now);
	}

	std::vector<Message> messages = m_inbox->Drain();
	for (Message& message : messages)
	{
		// Anything tagged with an older generation belongs to an abandoned attempt.
		if (message.generation == m_generation)
		{
			HandleMessage(message, now, events);
		}
	}

	const bool attemptInProgress =
	    m_state == LinkState::CONNECTING || m_state == LinkState::DISCOVERING || m_state == LinkState::SETTING_UP;
	if (attemptInProgress && now - m_attemptStartedSeconds > m_config.setupTimeoutSeconds)
	{
		FailAttempt("connection setup timed out", now, events);
	}
	return events;
}

LinkState BleLink::GetState() const
{
	return m_state;
}

ConnectionState BleLink::GetConnectionState() const
{
	switch (m_state)
	{
		case LinkState::IDLE:
			return ConnectionState::DISCONNECTED;
		case LinkState::CONNECTING:
		case LinkState::DISCOVERING:
		case LinkState::SETTING_UP:
			return ConnectionState::CONNECTING;
		case LinkState::READY:
			return ConnectionState::CONNECTED;
		case LinkState::WAITING_TO_RECONNECT:
			return ConnectionState::RECONNECTING;
		case LinkState::FAILED:
			return ConnectionState::FAILED;
		case LinkState::STOPPED:
			return ConnectionState::STOPPED;
	}
	return ConnectionState::DISCONNECTED;
}

bool BleLink::IsReady() const
{
	return m_state == LinkState::READY;
}

BleAddress BleLink::GetAddress() const
{
	return m_config.address;
}

bool BleLink::HasCharacteristic(const BleCharacteristicRef& characteristic) const
{
	for (const BleServiceInfo& service : m_services)
	{
		if (service.uuid != characteristic.service)
		{
			continue;
		}
		for (const BleCharacteristicInfo& info : service.characteristics)
		{
			if (info.uuid == characteristic.characteristic)
			{
				return true;
			}
		}
	}
	return false;
}

uint32_t BleLink::Write(const BleCharacteristicRef& characteristic, Bytes value, bool withResponse)
{
	if (m_state != LinkState::READY)
	{
		DEBUG_WARNING("BLE write to %s ignored: link not ready", FormatAddress(m_config.address).c_str());
		return 0;
	}
	const uint32_t writeId       = ++m_nextWriteId;
	const uint32_t generation    = m_generation;
	std::shared_ptr<Inbox> inbox = m_inbox;
	m_central->Write(m_config.address, characteristic, std::move(value), withResponse,
	                 [inbox, generation, writeId, characteristic](BleStatus status)
	                 {
		                 Message message;
		                 message.kind           = Message::Kind::WRITE_DONE;
		                 message.generation     = generation;
		                 message.status         = status;
		                 message.characteristic = characteristic;
		                 message.writeId        = writeId;
		                 inbox->Push(std::move(message));
	                 });
	return writeId;
}

void BleLink::BeginAttempt(double now)
{
	++m_generation;
	m_state                 = LinkState::CONNECTING;
	m_attemptStartedSeconds = now;
	m_setupIndex            = 0;
	m_services.clear();
	m_centralHasLink = true;  // Connect may establish one; Disconnect on an unconnected address is harmless

	const uint32_t generation    = m_generation;
	std::shared_ptr<Inbox> inbox = m_inbox;
	m_central->Connect(
	    m_config.address,
	    [inbox, generation]()
	    {
		    Message message;
		    message.kind       = Message::Kind::LINK_LOST;
		    message.generation = generation;
		    inbox->Push(std::move(message));
	    },
	    [inbox, generation](BleStatus status)
	    {
		    Message message;
		    message.kind       = Message::Kind::CONNECTED;
		    message.generation = generation;
		    message.status     = status;
		    inbox->Push(std::move(message));
	    });
}

void BleLink::RunNextSetupStep(double now, std::vector<LinkEvent>& events)
{
	while (m_setupIndex < m_config.setup.size())
	{
		const LinkSetupStep& step = m_config.setup[m_setupIndex];
		if (!HasCharacteristic(step.characteristic))
		{
			if (step.required)
			{
				FailAttempt("required characteristic " + DescribeCharacteristic(step.characteristic) + " not found",
				            now, events);
				return;
			}
			++m_setupIndex;
			continue;
		}

		const uint32_t generation                 = m_generation;
		std::shared_ptr<Inbox> inbox              = m_inbox;
		const BleCharacteristicRef characteristic = step.characteristic;
		if (step.kind == LinkSetupStep::Kind::READ)
		{
			m_central->Read(m_config.address, characteristic,
			                [inbox, generation, characteristic](BleStatus status, Bytes value)
			                {
				                Message message;
				                message.kind           = Message::Kind::READ_DONE;
				                message.generation     = generation;
				                message.status         = status;
				                message.characteristic = characteristic;
				                message.value          = std::move(value);
				                inbox->Push(std::move(message));
			                });
		}
		else
		{
			m_central->Subscribe(
			    m_config.address, characteristic, step.subscription,
			    [inbox, generation, characteristic](Bytes value)
			    {
				    Message message;
				    message.kind           = Message::Kind::VALUE;
				    message.generation     = generation;
				    message.characteristic = characteristic;
				    message.value          = std::move(value);
				    inbox->Push(std::move(message));
			    },
			    [inbox, generation, characteristic](BleStatus status)
			    {
				    Message message;
				    message.kind           = Message::Kind::SUBSCRIBED;
				    message.generation     = generation;
				    message.status         = status;
				    message.characteristic = characteristic;
				    inbox->Push(std::move(message));
			    });
		}
		return;
	}

	m_state = LinkState::READY;
	m_backoff.Reset();
	LinkEvent ready;
	ready.type = LinkEvent::Type::READY;
	events.push_back(std::move(ready));
}

void BleLink::HandleMessage(Message& message, double now, std::vector<LinkEvent>& events)
{
	switch (message.kind)
	{
		case Message::Kind::CONNECTED:
		{
			if (m_state != LinkState::CONNECTING)
			{
				return;
			}
			if (message.status != BleStatus::SUCCESS)
			{
				FailAttempt(std::string("connect failed: ") + ToString(message.status), now, events);
				return;
			}
			m_state                      = LinkState::DISCOVERING;
			const uint32_t generation    = m_generation;
			std::shared_ptr<Inbox> inbox = m_inbox;
			m_central->DiscoverServices(m_config.address,
			                            [inbox, generation](BleStatus status, std::vector<BleServiceInfo> services)
			                            {
				                            Message discovered;
				                            discovered.kind       = Message::Kind::DISCOVERED;
				                            discovered.generation = generation;
				                            discovered.status     = status;
				                            discovered.services   = std::move(services);
				                            inbox->Push(std::move(discovered));
			                            });
			return;
		}
		case Message::Kind::DISCOVERED:
		{
			if (m_state != LinkState::DISCOVERING)
			{
				return;
			}
			if (message.status != BleStatus::SUCCESS)
			{
				FailAttempt(std::string("service discovery failed: ") + ToString(message.status), now, events);
				return;
			}
			m_services = std::move(message.services);
			m_state    = LinkState::SETTING_UP;
			RunNextSetupStep(now, events);
			return;
		}
		case Message::Kind::READ_DONE:
		case Message::Kind::SUBSCRIBED:
		{
			if (m_state != LinkState::SETTING_UP || m_setupIndex >= m_config.setup.size())
			{
				return;
			}
			const LinkSetupStep& step = m_config.setup[m_setupIndex];
			if (message.status != BleStatus::SUCCESS)
			{
				if (step.required)
				{
					const char* verb = message.kind == Message::Kind::READ_DONE ? "read of " : "subscribe to ";
					FailAttempt(std::string(verb) + DescribeCharacteristic(step.characteristic) +
					                " failed: " + ToString(message.status),
					            now, events);
					return;
				}
				DEBUG_WARNING("Optional BLE setup step for %s failed: %s",
				              DescribeCharacteristic(step.characteristic).c_str(), ToString(message.status));
			}
			else if (message.kind == Message::Kind::READ_DONE)
			{
				LinkEvent read;
				read.type           = LinkEvent::Type::READ_VALUE;
				read.characteristic = message.characteristic;
				read.value          = std::move(message.value);
				events.push_back(std::move(read));
			}
			++m_setupIndex;
			RunNextSetupStep(now, events);
			return;
		}
		case Message::Kind::VALUE:
		{
			if (m_state != LinkState::SETTING_UP && m_state != LinkState::READY)
			{
				return;
			}
			LinkEvent notification;
			notification.type           = LinkEvent::Type::NOTIFICATION;
			notification.characteristic = message.characteristic;
			notification.value          = std::move(message.value);
			events.push_back(std::move(notification));
			return;
		}
		case Message::Kind::WRITE_DONE:
		{
			LinkEvent written;
			written.type           = LinkEvent::Type::WRITE_COMPLETE;
			written.characteristic = message.characteristic;
			written.status         = message.status;
			written.writeId        = message.writeId;
			events.push_back(std::move(written));
			return;
		}
		case Message::Kind::LINK_LOST:
		{
			if (m_state == LinkState::READY)
			{
				LinkEvent lost;
				lost.type   = LinkEvent::Type::DISCONNECTED;
				lost.detail = "link lost";
				events.push_back(std::move(lost));
			}
			// The central has already dropped the link; nothing to Disconnect.
			m_centralHasLink = false;
			FailAttempt("link lost", now, events);
			return;
		}
	}
}

void BleLink::FailAttempt(const std::string& reason, double now, std::vector<LinkEvent>& events)
{
	Release();
	++m_generation;

	if (m_backoff.IsExhausted())
	{
		m_state = LinkState::FAILED;
		DEBUG_ERROR("BLE device %s: giving up (%s)", FormatAddress(m_config.address).c_str(), reason.c_str());
		LinkEvent failed;
		failed.type   = LinkEvent::Type::FAILED;
		failed.detail = reason;
		events.push_back(std::move(failed));
		return;
	}

	const double delay = m_backoff.NextDelay();
	m_retryAtSeconds   = now + delay;
	m_state            = LinkState::WAITING_TO_RECONNECT;
	DEBUG_WARNING("BLE device %s: %s - retrying in %.1f s", FormatAddress(m_config.address).c_str(), reason.c_str(),
	              delay);
	LinkEvent retry;
	retry.type              = LinkEvent::Type::RETRY_SCHEDULED;
	retry.detail            = reason;
	retry.retryDelaySeconds = delay;
	events.push_back(std::move(retry));
}

void BleLink::Release()
{
	if (m_centralHasLink)
	{
		m_central->Disconnect(m_config.address);
		m_centralHasLink = false;
	}
}
}  // namespace Struktur::Fitness
