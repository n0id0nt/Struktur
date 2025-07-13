#include "GameObjectManager.h"

#include "ECS/Component/Transform.h"
#include "ECS/System/HierrarchySystem.h"
#include "ECS/System/TransformSystem.h"
#include "Engine/Core/GameContext.h"
#include "Engine/Core/SystemManager.h"

void Struktur::System::GameObjectManager::CreateDeleteObjectCallBack(Core::GameContext& context)
{
    entt::registry& registry = context.GetRegistry();
    
    // Listen for entity destruction to clean up references
    registry.on_destroy<Component::Children>().connect<&GameObjectManager::OnChildrenDestroy>(*this);
}

entt::entity Struktur::System::GameObjectManager::CreateGameObject(Core::GameContext& context, entt::entity parent)
{
    entt::registry& registry = context.GetRegistry();
    Core::SystemManager& systemManager = context.GetSystemManager();
    HierarchySystem& hierarchySystem = systemManager.GetSystem<HierarchySystem>();

    auto entity = registry.create();
    registry.emplace<Component::Transform>(entity);
    
    if (parent != entt::null) 
    {
        hierarchySystem.SetParent(context, entity, parent);
    }
    
    return entity;
}

void Struktur::System::GameObjectManager::DestroyGameObject(Core::GameContext& context, entt::entity entity) 
{
    Core::SystemManager& systemManager = context.GetSystemManager();
    HierarchySystem& hierarchySystem = systemManager.GetSystem<HierarchySystem>();
    hierarchySystem.DestroyEntity(context, entity);
}

void Struktur::System::GameObjectManager::OnChildrenDestroy(entt::registry& reg, entt::entity entity) 
{
    // Clean up any dangling references in children's Parent components
    if (auto* children = reg.try_get<Component::Children>(entity)) 
    {
        for (auto child : children->entities) {
            if (reg.valid(child)) 
            {
                reg.remove<Component::Parent>(child);
            }
        }
    }
}