#pragma once

#include <cstddef>
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
	void AddEvent(const std::string& type, const Struktur::Callback::Variant& data);

	class ReadIterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = const Event;
		using pointer           = const Event*;
		using reference         = const Event&;
		using difference_type   = std::ptrdiff_t;

		// begin — copy the queue so the original is untouched
		explicit ReadIterator(const std::queue<Event>& queue);

		// end sentinel
		ReadIterator() = default;

		bool operator==(const ReadIterator& other) const;
		bool operator!=(const ReadIterator& other) const;
		reference operator*() const;
		pointer operator->() const;
		ReadIterator& operator++();

	   private:
		std::queue<Event> m_snapshot;
	};

	// Range proxy returned by Read()
	struct ReadRange
	{
		const std::queue<Event>& queue;
		ReadIterator begin() const;
		ReadIterator end() const;
	};

	// for (const Event& e : events.Read()) { ... }
	ReadRange Read() const;

	class DrainIterator
	{
	   public:
		using iterator_category = std::forward_iterator_tag;
		using value_type        = Event;
		using pointer           = const Event*;
		using reference         = const Event&;
		using difference_type   = std::ptrdiff_t;

		// begin — holds a reference to the live queue, pops as it advances
		explicit DrainIterator(std::queue<Event>& queue);

		// end sentinel
		DrainIterator() = default;

		bool operator==(const DrainIterator& other) const;
		bool operator!=(const DrainIterator& other) const;
		reference operator*() const;
		pointer operator->() const;
		DrainIterator& operator++();

	   private:
		std::queue<Event>* m_queue = nullptr;
	};

	// Range proxy returned by Drain()
	struct DrainRange
	{
		std::queue<Event>& queue;
		DrainIterator begin();
		DrainIterator end();
	};

	// for (const Event& e : events.Drain()) { ... }
	// Pops each event as the loop advances. Stopping early leaves the rest.
	DrainRange Drain();

	void Clear();
	bool IsEmpty() const;
	std::size_t Size() const;

   private:
	std::queue<Event> m_eventQueue;
};
}  // namespace Event
}  // namespace Struktur
