#include "GameObjectManager.h"

#include "Engine/ECS/Component/Active.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/System/HierarchySystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/GameContext.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."

void Struktur::System::GameObjectManager::CreateObjectCallBack(GameContext& context)
{
	m_context                = &context;
	entt::registry& registry = context.GetRegistry();

	// Listen for entity destruction to clean up references
	registry.on_destroy<Component::Children>().connect<&GameObjectManager::OnChildrenDestroy>(*this);
	registry.on_destroy<Component::PhysicsBody>().connect<&GameObjectManager::OnPhysicsBodyDestory>(*this);
	registry.on_construct<Component::WrenScript>().connect<&GameObjectManager::OnScriptConstruct>(*this);
	registry.on_destroy<Component::WrenScript>().connect<&GameObjectManager::OnScriptDestory>(*this);

	// In GameObjectManager::Init or wherever you set up your registry signals
	registry.on_construct<Component::Active>().connect<&GameObjectManager::OnActiveStateChanged>(*this);
	registry.on_update<Component::Active>().connect<&GameObjectManager::OnActiveStateChanged>(*this);
}

void Struktur::System::GameObjectManager::Shutdown(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();

	registry.on_destroy<Component::Children>().disconnect<&GameObjectManager::OnChildrenDestroy>(*this);
	registry.on_destroy<Component::PhysicsBody>().disconnect<&GameObjectManager::OnPhysicsBodyDestory>(*this);
	registry.on_construct<Component::WrenScript>().disconnect<&GameObjectManager::OnScriptConstruct>(*this);
	registry.on_destroy<Component::WrenScript>().disconnect<&GameObjectManager::OnScriptDestory>(*this);
	registry.on_construct<Component::Active>().disconnect<&GameObjectManager::OnActiveStateChanged>(*this);
	registry.on_update<Component::Active>().disconnect<&GameObjectManager::OnActiveStateChanged>(*this);
}

entt::entity Struktur::System::GameObjectManager::CreateGameObject(GameContext& context, const std::string& identifier,
                                                                   entt::entity parent)
{
	entt::registry& registry         = context.GetRegistry();
	SystemManager& systemManager     = context.GetSystemManager();
	HierarchySystem& hierarchySystem = systemManager.GetSystem<HierarchySystem>();

	auto entity = registry.create();
	registry.emplace<Component::Transform>(entity);
	registry.emplace<Component::Identifier>(entity, identifier);
	registry.emplace<Component::Active>(entity);

	if (parent != entt::null)
	{
		hierarchySystem.SetParent(context, entity, parent);
	}

	return entity;
}

void Struktur::System::GameObjectManager::SafeDeleteGameObject(GameContext& context, entt::entity entity)
{
	m_queueOfObjectsToSafeDelete.emplace_back(entity);
}

void Struktur::System::GameObjectManager::DeleteGameObjectsInSafeToDeleteQueue(GameContext& context)
{
	for (auto& entity : m_queueOfObjectsToSafeDelete)
	{
		DestroyGameObject(context, entity);
	}
	m_queueOfObjectsToSafeDelete.clear();
}

void Struktur::System::GameObjectManager::UpdateGameObjectsActiveStateQueue(GameContext& context)
{
	entt::registry& registry = context.GetRegistry();
	for (auto entity : m_queueOfObjectsToUpdateActiveState)
	{
		if (!registry.valid(entity))
		{
			continue;
		}

		const auto* active = registry.try_get<Component::Active>(entity);
		if (!active)
		{
			continue;
		}

		const bool isActive = active->activeState == Component::Active::ActiveState::Active;

		if (isActive)
		{
			registry.remove<Inactive>(entity);
		}
		else
		{
			registry.emplace_or_replace<Inactive>(entity);
		}

		if (auto* body = registry.try_get<Component::PhysicsBody>(entity))
		{
			body->body->SetEnabled(isActive);
		}
	}
	m_queueOfObjectsToUpdateActiveState.clear();
}

void Struktur::System::GameObjectManager::DestroyGameObject(GameContext& context, entt::entity entity)
{
	SystemManager& systemManager     = context.GetSystemManager();
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

void Struktur::System::GameObjectManager::OnPhysicsBodyDestory(entt::registry& reg, entt::entity entity)
{
	// remove physics bodies from entities from the physics worked
	auto& physicsBody = reg.get<Component::PhysicsBody>(entity);
	if (physicsBody.body)
	{
		m_context->GetPhysicsWorld().DestroyBody(physicsBody.body);
		physicsBody.body = nullptr;
	}
}

void Struktur::System::GameObjectManager::OnScriptConstruct(entt::registry& reg, entt::entity entity)
{
	// Must not call InitialiseScript() here - on_construct can fire mid-Wren-call, which wrenCall() doesn't support.
	auto& scriptSystem = m_context->GetSystemManager().GetSystem<System::WrenScriptSystem>();
	scriptSystem.QueuePendingInitialise(entity);
}

void Struktur::System::GameObjectManager::OnScriptDestory(entt::registry& reg, entt::entity entity)
{
	auto& scriptSystem = m_context->GetSystemManager().GetSystem<System::WrenScriptSystem>();
	auto& wrenScript   = reg.get<Component::WrenScript>(entity);
	scriptSystem.DestroyScript(*m_context, entity, wrenScript);
}

void Struktur::System::GameObjectManager::OnActiveStateChanged(entt::registry& reg, entt::entity entity)
{
	m_queueOfObjectsToUpdateActiveState.push_back(entity);
}
