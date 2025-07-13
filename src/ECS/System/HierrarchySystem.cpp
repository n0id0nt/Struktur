#include "HierrarchySystem.h"

#include "Engine/Core/GameContext.h"
#include "ECS/Component/Transform.h"

void Struktur::System::HierarchySystem::SetParent(Core::GameContext &context, entt::entity child, entt::entity parent)
{
    entt::registry& registry = context.GetRegistry();
    
    // Remove from current parent if exists
    if (auto* currentParent = registry.try_get<Component::Parent>(child))
    {
        if (currentParent->entity != entt::null)
        {
            RemoveFromParent(context, child, currentParent->entity);
        }
    }

    // Set new parent
    if (parent != entt::null) 
    {
        registry.emplace_or_replace<Component::Parent>(child, parent);
        
        auto& parentChildren = registry.get_or_emplace<Component::Children>(parent);
        parentChildren.entities.push_back(child);
    }
    else
    {
        registry.remove<Component::Parent>(child);
    }
}

void Struktur::System::HierarchySystem::RemoveFromParent(Core::GameContext &context, entt::entity child, entt::entity parent)
{
    entt::registry& registry = context.GetRegistry();

    if (auto* children = registry.try_get<Component::Children>(parent))
    {
        auto it = std::find(children->entities.begin(), children->entities.end(), child);
        if (it != children->entities.end())
        {
            children->entities.erase(it);
        }
    }
}

void Struktur::System::HierarchySystem::DestroyEntity(Core::GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    
    // Remove from parent's children list
    if (auto* parent = registry.try_get<Component::Parent>(entity))
    {
        if (parent->entity != entt::null)
        {
            RemoveFromParent(context, entity, parent->entity);
        }
    }

    // Recursively destroy children
    if (auto* children = registry.try_get<Component::Children>(entity))
    {
        for (int i = children->entities.size() - 1; i >= 0; --i) 
        {
            auto child = children->entities[i];
            DestroyEntity(context, child);
        }
    }

    registry.destroy(entity);
}
