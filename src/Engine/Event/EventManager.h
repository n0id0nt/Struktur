#pragma once

#include <queue>
#include <string>

#include "Engine/Callback/Variant.h"
#include "Event.h"

namespace Struktur
{
namespace Event
{
class EventManager
{
   public:
	void AddEvent(const std::string& type, const Struktur::Callback::Variant& data)
	{
		m_eventQueue.push({type, data});
	}

	class ReadIterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = const Event;
		using pointer           = const Event*;
		using reference         = const Event&;
		using difference_type   = std::ptrdiff_t;

		// begin — copy the queue so the original is untouched
		explicit ReadIterator(const std::queue<Event>& queue)
		    : m_snapshot(queue)
		{
		}

		// end sentinel
		ReadIterator() = default;

		bool operator==(const ReadIterator& other) const
		{
			// Two iterators are equal when both are empty (i.e. at end)
			return m_snapshot.empty() == other.m_snapshot.empty();
		}

		bool operator!=(const ReadIterator& other) const
		{
			return !(*this == other);
		}

		reference operator*() const
		{
			return m_snapshot.front();
		}
		pointer operator->() const
		{
			return &m_snapshot.front();
		}

		ReadIterator& operator++()
		{
			m_snapshot.pop();
			return *this;
		}

	   private:
		std::queue<Event> m_snapshot;
	};

	// Range proxy returned by Read()
	struct ReadRange
	{
		const std::queue<Event>& queue;
		ReadIterator begin() const
		{
			return ReadIterator(queue);
		}
		ReadIterator end() const
		{
			return ReadIterator();
		}
	};

	// for (const Event& e : events.Read()) { ... }
	ReadRange Read() const
	{
		return {m_eventQueue};
	}

	class DrainIterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = Event;
		using pointer           = const Event*;
		using reference         = const Event&;
		using difference_type   = std::ptrdiff_t;

		// begin — holds a reference to the live queue, pops as it advances
		explicit DrainIterator(std::queue<Event>& queue)
		    : m_queue(&queue)
		{
		}

		// end sentinel
		DrainIterator() = default;

		bool operator==(const DrainIterator& other) const
		{
			bool thisEmpty  = !m_queue || m_queue->empty();
			bool otherEmpty = !other.m_queue || other.m_queue->empty();
			return thisEmpty == otherEmpty;
		}

		bool operator!=(const DrainIterator& other) const
		{
			return !(*this == other);
		}

		reference operator*() const
		{
			return m_queue->front();
		}
		pointer operator->() const
		{
			return &m_queue->front();
		}

		DrainIterator& operator++()
		{
			m_queue->pop();
			return *this;
		}

	   private:
		std::queue<Event>* m_queue = nullptr;
	};

	// Range proxy returned by Drain()
	struct DrainRange
	{
		std::queue<Event>& queue;
		DrainIterator begin()
		{
			return DrainIterator(queue);
		}
		DrainIterator end()
		{
			return DrainIterator();
		}
	};

	// for (const Event& e : events.Drain()) { ... }
	// Pops each event as the loop advances. Stopping early leaves the rest.
	DrainRange Drain()
	{
		return {m_eventQueue};
	}

	void Clear()
	{
		m_eventQueue = {};
	}
	bool IsEmpty() const
	{
		return m_eventQueue.empty();
	}
	std::size_t Size() const
	{
		return m_eventQueue.size();
	}

   private:
	std::queue<Event> m_eventQueue;
};
}  // namespace Event
}  // namespace Struktur
