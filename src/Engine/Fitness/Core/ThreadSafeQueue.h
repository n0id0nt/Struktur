#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

namespace Struktur::Fitness
{
// Multi-producer, single-consumer hand-off from BLE callback threads to the game thread. Producers Push() from any
// thread; the game thread Drain()s once per Update. Bounded: when full, the OLDEST item is dropped (stale sensor
// data is worth less than fresh data) and counted.
template <typename T>
class ThreadSafeQueue
{
public:
	explicit ThreadSafeQueue(size_t capacity = 1024)
	    : m_capacity(capacity)
	{
	}

	void Push(T item)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_items.size() >= m_capacity)
		{
			m_items.erase(m_items.begin());
			++m_dropped;
		}
		m_items.push_back(std::move(item));
	}

	std::vector<T> Drain()
	{
		std::vector<T> drained;
		std::lock_guard<std::mutex> lock(m_mutex);
		drained.swap(m_items);
		return drained;
	}

	size_t GetDroppedCount() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_dropped;
	}

private:
	mutable std::mutex m_mutex;
	std::vector<T> m_items;
	size_t m_capacity;
	size_t m_dropped = 0;
};
}  // namespace Struktur::Fitness
