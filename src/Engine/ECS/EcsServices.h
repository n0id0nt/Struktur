#pragma once

#include <memory>

#include "Engine/ECS/GameObjectManager.h"
#include "Engine/ECS/SystemManager.h"
#include "entt/entt.hpp"
#ifdef STRUKTUR_WREN
	#include "Engine/ECS/WrenScriptComponentRegistry.h"
#endif

namespace Struktur::System
{
// The ECS library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

#ifdef STRUKTUR_WREN
	std::unique_ptr<Wren::WrenScriptComponentRegistry> scriptComponentRegistry;
#endif
	std::unique_ptr<GameObjectManager> gameObjectManager;
	std::unique_ptr<entt::registry> registry;
	std::unique_ptr<SystemManager> systemManager;
};
}  // namespace Struktur::System
