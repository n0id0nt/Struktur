#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Debug/Assertions.h"

namespace Struktur
{
class GameContext;

namespace System
{
struct Inactive
{
};

class ISystem
{
public:
	virtual ~ISystem()                        = default;
	virtual void Update(GameContext& context) = 0;
	virtual std::string Name() const          = 0;
};

class SystemManager
{
public:
	SystemManager() {}
	~SystemManager() {}

	void Update(GameContext& context);
	void Render(GameContext& context);
	void FixedUpdate(GameContext& context);

	template <typename T, typename... Args>
	T& AddUpdateSystem(Args&&... args)
	{
		static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from Struktur::Core::ISystem");
		std::type_index typeIndex = std::type_index(typeid(T));

		m_updateSystems.push_back(typeIndex);

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
	std::vector<std::type_index> m_updateSystems;
	std::vector<std::type_index> m_renderSystems;
	std::vector<std::type_index> m_helperSystems;
	std::vector<std::type_index> m_fixedUpdateSystems;

	std::unordered_map<std::type_index, std::unique_ptr<ISystem>> m_systemMap;
};
}  // namespace System
}  // namespace Struktur
