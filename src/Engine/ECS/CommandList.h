#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace Struktur
{
class GameContext;
}

// Structural changes recorded by systems running on job workers (create/destroy entities, add/remove components,
// anything that must happen on the main thread), applied in the order they were recorded once the batch of systems
// that recorded them has finished. Safe to record into from any thread.
//
//   context.GetSystemManager().GetCommands().Defer([entity](GameContext& context) {
//       context.GetGameObjectManager().DeleteGameObject(context, entity);
//   });
namespace Struktur::System
{
class CommandList
{
public:
	using Command = std::function<void(GameContext&)>;

	void Defer(Command command)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_commands.push_back(std::move(command));
	}

	// Main thread only. Commands recorded while applying run in the same pass.
	size_t Apply(GameContext& context)
	{
		size_t applied = 0;
		for (;;)
		{
			std::vector<Command> commands;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_commands.empty())
				{
					return applied;
				}
				commands.swap(m_commands);
			}
			for (Command& command : commands)
			{
				command(context);
				++applied;
			}
		}
	}

	size_t GetPendingCount() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_commands.size();
	}

private:
	mutable std::mutex m_mutex;
	std::vector<Command> m_commands;
};
}  // namespace Struktur::System
