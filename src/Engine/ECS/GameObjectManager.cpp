#include "GameObjectManager.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

#include "Engine/GameContext.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/ECS/System/HierrarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/WrenScript.h"

Struktur::System::GameObjectManager::~GameObjectManager()
{
    entt::registry& registry = m_context->GetRegistry();

    registry.on_destroy<Component::Children>().disconnect<&GameObjectManager::OnChildrenDestroy>(*this);
    registry.on_destroy<Component::PhysicsBody>().disconnect<&GameObjectManager::OnPhysicsBodyDestory>(*this);
    registry.on_destroy<Component::WrenScript>().disconnect<&GameObjectManager::OnScriptDestory>(*this);
}

void Struktur::System::GameObjectManager::CreateDeleteObjectCallBack(GameContext &context)
{
    m_context = &context;
    entt::registry& registry = context.GetRegistry();
    
    // Listen for entity destruction to clean up references
    registry.on_destroy<Component::Children>().connect<&GameObjectManager::OnChildrenDestroy>(*this);
    registry.on_destroy<Component::PhysicsBody>().connect<&GameObjectManager::OnPhysicsBodyDestory>(*this);
    registry.on_destroy<Component::WrenScript>().connect<&GameObjectManager::OnScriptDestory>(*this);
}

entt::entity Struktur::System::GameObjectManager::CreateGameObject(GameContext& context, const std::string& identifier, entt::entity parent)
{
    entt::registry& registry = context.GetRegistry();
    SystemManager& systemManager = context.GetSystemManager();
    HierarchySystem& hierarchySystem = systemManager.GetSystem<HierarchySystem>();

    auto entity = registry.create();
    registry.emplace<Component::LocalTransform>(entity);
    registry.emplace<Component::Identifier>(entity, identifier);
    
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

void Struktur::System::GameObjectManager::OnScriptDestory(entt::registry &reg, entt::entity entity)
{
    auto& scriptSystem = m_context->GetSystemManager().GetSystem<System::WrenScriptSystem>();
    auto& wrenScript = reg.get<Component::WrenScript>(entity);
    scriptSystem.DestroyScript(*m_context, entity, wrenScript);
}

// ============================================================================
// ENTITY BINDINGS
// ============================================================================

// Entity.create(name, parent) -> entityId
void wren_EntityCreate(WrenVM* vm) {
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& gameObjectManager = context->GetGameObjectManager();
    
    const char* name = wrenGetSlotString(vm, 1);
    double parentId = wrenGetSlotDouble(vm, 2);
    entt::entity parent = static_cast<entt::entity>(parentId);
    
    entt::entity entity = gameObjectManager.CreateGameObject(*context, name, parent);
    
    wrenSetSlotDouble(vm, 0, static_cast<double>(entity));
}

WREN_CLASS_STATIC("game", "Entity", "create(_,_)", wren_EntityCreate,
                  "Create a new entity with the given name and parent. Returns entity ID.");

// Entity.destroy(entity)
void wren_EntityDestroy(WrenVM* vm) {
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& gameObjectManager = context->GetGameObjectManager();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    gameObjectManager.DestroyGameObject(*context, entity);
}

WREN_CLASS_STATIC("game", "Entity", "destroy(_)", wren_EntityDestroy,
                  "Destroy an entity and all its children.");

// Entity.isValid(entity) -> bool
void wren_EntityIsValid(WrenVM* vm) {
    Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
    auto& registry = context->GetRegistry();
    
    double entityId = wrenGetSlotDouble(vm, 1);
    entt::entity entity = static_cast<entt::entity>(entityId);
    
    bool valid = registry.valid(entity);
    wrenSetSlotBool(vm, 0, valid);
}

WREN_CLASS_STATIC("game", "Entity", "isValid(_)", wren_EntityIsValid,
                  "Check if an entity ID is valid.");
