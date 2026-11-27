#include "EcsServices.h"

namespace Struktur::System
{
Services::Services()
{
	gameObjectManager = std::make_unique<GameObjectManager>();
	registry          = std::make_unique<entt::registry>();
	systemManager     = std::make_unique<SystemManager>();
#ifdef STRUKTUR_WREN
	scriptComponentRegistry = std::make_unique<Wren::WrenScriptComponentRegistry>();
#endif
}

void Services::Shutdown()
{
	gameObjectManager.reset();
	registry.reset();
	systemManager.reset();
#ifdef STRUKTUR_WREN
	scriptComponentRegistry.reset();
#endif
}
}  // namespace Struktur::System
