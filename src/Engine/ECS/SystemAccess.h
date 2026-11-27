#pragma once

#include <algorithm>
#include <typeindex>
#include <vector>

// What a system touches, so the SystemManager can run systems that don't clash at the same time (docs/JobSystem.md,
// phase 4). Every system is main-thread-only unless it says otherwise:
//
//   SystemAccess Access() const override
//   {
//       return SystemAccess::Worker().Read<Component::SpriteAnimation, Core::TimeSystem>().Write<Component::Sprite>();
//   }
//
// A worker system may read and write the component types (and engine services - any type works as a token) it
// declares, and nothing else. It must not create or destroy entities, add or remove components, or touch Wren, bgfx,
// the UI or audio: record structural changes in the SystemManager's CommandList instead, which is applied on the main
// thread once the system's batch is done.
namespace Struktur::System
{
class SystemAccess
{
public:
	// Runs on the main thread, in registration order, never alongside anything else (the default).
	static SystemAccess MainThread() { return SystemAccess(true); }
	// May run on a job worker, alongside systems it doesn't clash with.
	static SystemAccess Worker() { return SystemAccess(false); }

	template <typename... Types>
	SystemAccess& Read()
	{
		(Add<Types>(m_reads), ...);
		return *this;
	}
	template <typename... Types>
	SystemAccess& Write()
	{
		(Add<Types>(m_writes), ...);
		return *this;
	}

	bool IsMainThread() const { return m_mainThread; }
	const std::vector<std::type_index>& GetReads() const { return m_reads; }
	const std::vector<std::type_index>& GetWrites() const { return m_writes; }

	// Two systems clash when either is main-thread-only, or one writes something the other reads or writes.
	bool ConflictsWith(const SystemAccess& other) const
	{
		if (m_mainThread || other.m_mainThread)
		{
			return true;
		}
		for (const std::type_index& written : m_writes)
		{
			if (Contains(other.m_reads, written) || Contains(other.m_writes, written))
			{
				return true;
			}
		}
		for (const std::type_index& written : other.m_writes)
		{
			if (Contains(m_reads, written))
			{
				return true;
			}
		}
		return false;
	}

private:
	explicit SystemAccess(bool mainThread) : m_mainThread(mainThread) {}

	template <typename Type>
	static void Add(std::vector<std::type_index>& list)
	{
		const std::type_index type(typeid(Type));
		if (!Contains(list, type))
		{
			list.push_back(type);
		}
	}
	static bool Contains(const std::vector<std::type_index>& list, const std::type_index& type)
	{
		return std::find(list.begin(), list.end(), type) != list.end();
	}

	bool m_mainThread;
	std::vector<std::type_index> m_reads;
	std::vector<std::type_index> m_writes;
};
}  // namespace Struktur::System
