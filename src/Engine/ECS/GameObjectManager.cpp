#include "GameObjectManager.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

#include "Engine/GameContext.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/ECS/System/HierrarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/PhysicsBody.h"

Struktur::System::GameObjectManager::~GameObjectManager()
{
    entt::registry& registry = m_context->GetRegistry();

    registry.on_destroy<Component::Children>().disconnect<&GameObjectManager::OnChildrenDestroy>(*this);
    registry.on_destroy<Component::PhysicsBody>().disconnect<&GameObjectManager::OnPhysicsBodyDestory>(*this);
}

void Struktur::System::GameObjectManager::CreateDeleteObjectCallBack(GameContext &context)
{
    m_context = &context;
    entt::registry& registry = context.GetRegistry();
    
    // Listen for entity destruction to clean up references
    registry.on_destroy<Component::Children>().connect<&GameObjectManager::OnChildrenDestroy>(*this);
    registry.on_destroy<Component::PhysicsBody>().connect<&GameObjectManager::OnPhysicsBodyDestory>(*this);
}

entt::entity Struktur::System::GameObjectManager::CreateGameObject(GameContext& context, entt::entity parent)
{
    entt::registry& registry = context.GetRegistry();
    SystemManager& systemManager = context.GetSystemManager();
    HierarchySystem& hierarchySystem = systemManager.GetSystem<HierarchySystem>();

    auto entity = registry.create();
    registry.emplace<Component::LocalTransform>(entity);
    
    if (parent != entt::null)
    {
        hierarchySystem.SetParent(context, entity, parent);
    }
    
    return entity;
}

void Struktur::System::GameObjectManager::DestroyGameObject(GameContext& context, entt::entity entity) 
{
    SystemManager& systemManager = context.GetSystemManager();
    HierarchySystem& hierarchySystem = systemManager.GetSystem<HierarchySystem>();
    hierarchySystem.DestroyEntity(context, entity);
}

void Struktur::System::GameObjectManager::OnChildrenDestroy(entt::registry& reg, entt::entity entity) 
{
    // Clean up any dangling references in children's Parent components
    if (auto* children = reg.try_get<Component::Children>(entity))
    {
        for (auto child : children->entities)
        {
            if (reg.valid(child)) 
            {
                reg.remove<Component::Parent>(child);
            }
        }
    }
}

void Struktur::System::GameObjectManager::OnPhysicsBodyDestory(entt::registry &reg, entt::entity entity)
{
    // remove physics bodies from entities from the physics worked
    auto& physicsBody = reg.get<Component::PhysicsBody>(entity);
    if (physicsBody.body)
    {
        m_context->GetPhysicsWorld().DestroyBody(physicsBody.body);
        physicsBody.body = nullptr;
    }
}
