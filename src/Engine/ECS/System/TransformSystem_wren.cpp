#include "GameObjectManager.h"

#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/System/TransformSystem.h"

// ============================================================================
// TRANSFORM BINDINGS
// ============================================================================

// Transform.getPosition(entity) -> [x, y, z] or null
void wren_TransformGetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    auto* transform = registry.try_get<Struktur::Component::WorldTransform>(entity);
    if (!transform) {
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    wrenSetSlotNewList(vm, 0);
    wrenSetSlotDouble(vm, 1, transform->position.x);
    wrenInsertInList(vm, 0, -1, 1);
    wrenSetSlotDouble(vm, 1, transform->position.y);
    wrenInsertInList(vm, 0, -1, 1);
    wrenSetSlotDouble(vm, 1, transform->position.z);
    wrenInsertInList(vm, 0, -1, 1);
}

WREN_CLASS_STATIC("game", "Transform", "getPosition(_)", wren_TransformGetPosition, "Get the world position of an entity. Returns [x, y, z] or null if no transform.");

// Transform.setPosition(entity, x, y, z)
void wren_TransformSetPosition(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& systemManager = context->GetSystemManager();
    auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    double z = wrenGetSlotDouble(vm, 4);
    
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    transformSystem.SetWorldTransform(*context, entity,
        glm::vec3(x, y, z),
        glm::vec3(1.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );
}

WREN_CLASS_STATIC("game", "Transform", "setPosition(_,_,_,_)", wren_TransformSetPosition, "Set the world position of an entity.");

// Transform.setLocal(entity, x, y, z)
void wren_TransformSetLocal(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& systemManager = context->GetSystemManager();
    auto& transformSystem = systemManager.GetSystem<Struktur::System::TransformSystem>();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    double x = wrenGetSlotDouble(vm, 2);
    double y = wrenGetSlotDouble(vm, 3);
    double z = wrenGetSlotDouble(vm, 4);
    
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    transformSystem.SetLocalTransform(*context, entity,
        glm::vec3(x, y, z),
        glm::vec3(1.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
    );
}

WREN_CLASS_STATIC("game", "Transform", "setLocal(_,_,_,_)", wren_TransformSetLocal, "Set the local position of an entity.");

// Transform.getLocal(entity) -> [x, y, z] or null
void wren_TransformGetLocal(WrenVM* vm)
{
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    auto* transform = registry.try_get<Struktur::Component::LocalTransform>(entity);
    if (!transform) {
        wrenSetSlotNull(vm, 0);
        return;
    }
    
    wrenSetSlotNewList(vm, 0);
    wrenSetSlotDouble(vm, 1, transform->position.x);
    wrenInsertInList(vm, 0, -1, 1);
    wrenSetSlotDouble(vm, 1, transform->position.y);
    wrenInsertInList(vm, 0, -1, 1);
    wrenSetSlotDouble(vm, 1, transform->position.z);
    wrenInsertInList(vm, 0, -1, 1);
}

WREN_CLASS_STATIC("game", "Transform", "getLocal(_)", wren_TransformGetLocal, "Get the local position of an entity. Returns [x, y, z] or null if no transform.");

