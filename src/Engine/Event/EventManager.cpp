#include "EventManager.h"

void Struktur::Event::EventManager::AddEvent(const std::string& type, const Struktur::Callback::Variant& data)
{
	m_eventQueue.push({type, data});
}

Struktur::Event::EventManager::ReadIterator::ReadIterator(const std::queue<Event>& queue)
    : m_snapshot(queue)
{
}

bool Struktur::Event::EventManager::ReadIterator::operator==(const ReadIterator& other) const
{
	// Two iterators are equal when both are empty (i.e. at end)
	return m_snapshot.empty() == other.m_snapshot.empty();
}

bool Struktur::Event::EventManager::ReadIterator::operator!=(const ReadIterator& other) const
{
	return !(*this == other);
}

Struktur::Event::EventManager::ReadIterator::reference Struktur::Event::EventManager::ReadIterator::operator*() const
{
	return m_snapshot.front();
}

Struktur::Event::EventManager::ReadIterator::pointer Struktur::Event::EventManager::ReadIterator::operator->() const
{
	return &m_snapshot.front();
}

Struktur::Event::EventManager::ReadIterator& Struktur::Event::EventManager::ReadIterator::operator++()
{
	m_snapshot.pop();
	return *this;
}

Struktur::Event::EventManager::ReadIterator Struktur::Event::EventManager::ReadRange::begin() const
{
	return ReadIterator(queue);
}

Struktur::Event::EventManager::ReadIterator Struktur::Event::EventManager::ReadRange::end() const
{
	return ReadIterator();
}

Struktur::Event::EventManager::ReadRange Struktur::Event::EventManager::Read() const
{
	return {m_eventQueue};
}

Struktur::Event::EventManager::DrainIterator::DrainIterator(std::queue<Event>& queue)
    : m_queue(&queue)
{
}

bool Struktur::Event::EventManager::DrainIterator::operator==(const DrainIterator& other) const
{
	bool thisEmpty  = !m_queue || m_queue->empty();
	bool otherEmpty = !other.m_queue || other.m_queue->empty();
	return thisEmpty == otherEmpty;
}

bool Struktur::Event::EventManager::DrainIterator::operator!=(const DrainIterator& other) const
{
	return !(*this == other);
}

Struktur::Event::EventManager::DrainIterator::reference Struktur::Event::EventManager::DrainIterator::operator*() const
{
	return m_queue->front();
}

Struktur::Event::EventManager::DrainIterator::pointer Struktur::Event::EventManager::DrainIterator::operator->() const
{
	return &m_queue->front();
}

Struktur::Event::EventManager::DrainIterator& Struktur::Event::EventManager::DrainIterator::operator++()
{
	m_queue->pop();
	return *this;
}

Struktur::Event::EventManager::DrainIterator Struktur::Event::EventManager::DrainRange::begin()
{
	return DrainIterator(queue);
}

Struktur::Event::EventManager::DrainIterator Struktur::Event::EventManager::DrainRange::end()
{
	return DrainIterator();
}

Struktur::Event::EventManager::DrainRange Struktur::Event::EventManager::Drain()
{
	return {m_eventQueue};
}

void Struktur::Event::EventManager::Clear()
{
	m_eventQueue = {};
}

bool Struktur::Event::EventManager::IsEmpty() const
{
	return m_eventQueue.empty();
}

std::size_t Struktur::Event::EventManager::Size() const
{
	return m_eventQueue.size();
}
