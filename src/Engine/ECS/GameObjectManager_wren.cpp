#include "GameObjectManager.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

// ============================================================================
// GAME OBJECT MANAGER BINDINGS
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

// GameObject.destroy(entity)
void wren_GameObjectDestroy(WrenVM* vm) {
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& gameObjectManager = context->GetGameObjectManager();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    gameObjectManager.DestroyGameObject(*context, entity);
}

WREN_CLASS_STATIC("game", "GameObject", "destroy(_)", wren_GameObjectDestroy, "Destroy an Game Object and all its children.");

// GameObject.isValid(entity) -> bool
void wren_GameObjectIsValid(WrenVM* vm) {
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    bool valid = registry.valid(entity);
    wrenSetSlotBool(vm, 0, valid);
}

WREN_CLASS_STATIC("game", "GameObject", "isValid(_)", wren_GameObjectIsValid, "Check if an entity ID is valid.");
