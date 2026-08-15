#include "HierarchySystem.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/GameContext.h"

void Struktur::System::HierarchySystem::SetParent(GameContext& context, entt::entity child, entt::entity parent)
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

	// Local didn't change, but the ancestor chain did - the cached world transform no longer
	// reflects it until the next resolve.
	if (auto* childTransform = registry.try_get<Component::Transform>(child))
	{
		childTransform->dirty = true;
	}
}

void Struktur::System::HierarchySystem::RemoveFromParent(GameContext& context, entt::entity child, entt::entity parent)
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

void Struktur::System::HierarchySystem::DestroyEntity(GameContext& context, entt::entity entity)
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

void Struktur::System::HierarchySystem::ActivevateEntity(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	registry.patch<Component::Active>(entity, [](Component::Active& active)
	                                  { active.activeState = Component::Active::ActiveState::Active; });
	PropagateActiveStateToChildren(context, entity, Component::Active::ActiveState::Active);
}

void Struktur::System::HierarchySystem::DeactivevateEntity(GameContext& context, entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	registry.patch<Component::Active>(entity, [](Component::Active& active)
	                                  { active.activeState = Component::Active::ActiveState::InactiveSelf; });
	PropagateActiveStateToChildren(context, entity, Component::Active::ActiveState::InactiveParent);
}

void Struktur::System::HierarchySystem::PropagateActiveStateToChildren(GameContext& context, entt::entity entity,
                                                                       Component::Active::ActiveState active)
{
	entt::registry& registry = context.GetRegistry();

	if (auto* children = registry.try_get<Component::Children>(entity))
	{
		for (int i = children->entities.size() - 1; i >= 0; --i)
		{
			auto child                 = children->entities[i];
			auto& childActiveComponent = registry.get<Component::Active>(child);

			// A child that was explicitly deactivated stays deactivated regardless of parent state
			if (childActiveComponent.activeState == Component::Active::ActiveState::InactiveSelf)
			{
				continue;
			}

			registry.patch<Component::Active>(child, [active](Component::Active& patchActiveComponent)
			                                  { patchActiveComponent.activeState = active; });
			PropagateActiveStateToChildren(context, child, active);
		}
	}
}
