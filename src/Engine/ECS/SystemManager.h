#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Debug/Assertions.h"
#include "Engine/ECS/CommandList.h"
#include "Engine/ECS/SystemAccess.h"
#include "Engine/ECS/SystemSchedule.h"

namespace Struktur
{
class GameContext;

namespace System
{
struct Inactive
{
};

// Paused but still in the world: drawn, animated and colliding, but its script's update() / fixedUpdate() aren't called
// and a moving body is stopped. Set on a whole subtree by HierarchySystem::SetFrozen - the rooms next to the player's
// (States/GameWorldState.wren) are frozen so they show without their creatures roaming about.
struct Frozen
{
};

class ISystem
{
public:
	virtual ~ISystem()                        = default;
	virtual void Update(GameContext& context) = 0;
	virtual std::string Name() const          = 0;
	// What Update() touches. Main-thread-only unless a system declares otherwise - see SystemAccess.h.
	virtual SystemAccess Access() const
	{
		return SystemAccess::MainThread();
	}
};

class SystemManager
{
public:
	SystemManager() {}
	~SystemManager() {}

	void Update(GameContext& context);
	void Render(GameContext& context);
	void FixedUpdate(GameContext& context);

	// Structural changes recorded by worker systems, applied after their batch (see CommandList.h).
	CommandList& GetCommands()
	{
		return m_commands;
	}
	// Off: every system runs on the main thread in registration order, whatever it declares (debugging; also what
	// zero job workers gives). On by default; `--serial-systems` turns it off.
	void SetParallel(bool parallel)
	{
		m_parallel = parallel;
	}
	bool IsParallel() const
	{
		return m_parallel;
	}

	template <typename T, typename... Args>
	T& AddUpdateSystem(Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
		std::type_index typeIndex = std::type_index(typeid(T));

		m_updateSystems.push_back(typeIndex);
		++m_scheduleVersion;

		auto system            = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr                 = system.get();
		m_systemMap[typeIndex] = std::move(system);

		return *ptr;
	}

	template <typename T, typename... Args>
	T& AddRenderSystem(Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
		std::type_index typeIndex = std::type_index(typeid(T));

		m_renderSystems.push_back(typeIndex);
		++m_scheduleVersion;

		auto system            = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr                 = system.get();
		m_systemMap[typeIndex] = std::move(system);

		return *ptr;
	}

	// Runs at a fixed cadence (see GameLoop's accumulator loop in Game.cpp), not once per render frame - used for
	// physics and anything that needs to stay in step with it. Constructs + registers a new instance, same as
	// AddUpdateSystem/AddRenderSystem/AddHelperSystem, so a type already added via one of those must not be
	// passed here too (that would construct a second, orphaned instance sharing the same m_systemMap slot). A
	// system that needs both a per-frame pass and a fixed-rate pass instead gets a small dedicated wrapper type
	// registered here that reaches the already-constructed real instance (see WrenStateFixedUpdateSystem/
	// WrenScriptFixedUpdateSystem) - mirroring how AddRenderSystem's own callers are always distinct wrapper
	// types, never a type already added via AddUpdateSystem.
	template <typename T, typename... Args>
	T& AddFixedUpdateSystem(Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
		std::type_index typeIndex = std::type_index(typeid(T));

		m_fixedUpdateSystems.push_back(typeIndex);
		++m_scheduleVersion;

		auto system            = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr                 = system.get();
		m_systemMap[typeIndex] = std::move(system);

		return *ptr;
	}

	template <typename T, typename... Args>
	T& AddHelperSystem(Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
		std::type_index typeIndex = std::type_index(typeid(T));

		m_helperSystems.push_back(typeIndex);

		auto system            = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr                 = system.get();
		m_systemMap[typeIndex] = std::move(system);

		return *ptr;
	}

	template <typename T>
	T* TryGetSystem()
	{
		std::type_index typeIndex = std::type_index(typeid(T));

		auto it = m_systemMap.find(typeIndex);
		if (it != m_systemMap.end())
		{
			return static_cast<T*>(it->second.get());
		}

		return nullptr;
	}

	template <typename T>
	T& GetSystem()
	{
		T* system = TryGetSystem<T>();
		ASSERT_MSG(system, "System Type Not Registered");
		return *system;
	}

private:
	struct Phase
	{
		std::vector<ISystem*> systems;
		std::vector<ScheduleStep> steps;
		int builtVersion = -1;
	};
	void RunPhase(GameContext& context, const std::vector<std::type_index>& ids, Phase& phase);

	std::vector<std::type_index> m_updateSystems;
	std::vector<std::type_index> m_renderSystems;
	std::vector<std::type_index> m_helperSystems;
	std::vector<std::type_index> m_fixedUpdateSystems;

	std::unordered_map<std::type_index, std::unique_ptr<ISystem>> m_systemMap;

	Phase m_updatePhase;
	Phase m_renderPhase;
	Phase m_fixedUpdatePhase;
	int m_scheduleVersion = 0;
	bool m_parallel       = true;
	CommandList m_commands;
};
}  // namespace System
}  // namespace Struktur
