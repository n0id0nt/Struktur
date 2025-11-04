#include "GameObjectManager.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// LEVEL BINDINGS
// ============================================================================

// GameObject.create(name, parent) -> entityId
void wren_GameObjectCreate(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& gameObjectManager = context->GetGameObjectManager();
    
    const char* name = wrenGetSlotString(vm, 1);
    double parentId = wrenGetSlotDouble(vm, 2);
    entt::entity parent = static_cast<entt::entity>(parentId);
    
    entt::entity entity = gameObjectManager.CreateGameObject(*context, name, parent);
    
    wrenSetSlotDouble(vm, 0, static_cast<double>(entity));
}

WREN_CLASS_STATIC("game", "GameObject", "create(_,_)", wren_GameObjectCreate, "Create a new Game Object with the given name and parent. Returns entity ID.");
