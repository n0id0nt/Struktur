#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "Debug/Assertions.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/GameContext.h"
#include "glm/gtx/matrix_decompose.hpp"

Struktur::Component::Transform& Struktur::System::TransformSystem::EnsureWorldUpToDate(GameContext& context,
                                                                                       entt::entity entity)
{
	entt::registry& registry = context.GetRegistry();
	auto& transform          = registry.get<Component::Transform>(entity);

	if (!transform.dirty)
	{
		return transform;
	}

	glm::mat4 parentMatrix = glm::mat4(1.0f);
	if (auto* parent = registry.try_get<Component::Parent>(entity))
	{
		if (registry.valid(parent->entity) && registry.try_get<Component::Transform>(parent->entity))
		{
			parentMatrix = EnsureWorldUpToDate(context, parent->entity).worldMatrix;
		}
	}

	glm::mat4 worldMatrix = parentMatrix * transform.localMatrix;

	glm::vec3 scaleVec;
	glm::quat rotationQuat{};
	glm::vec3 translationVec;
	glm::vec3 skewVec;
	glm::vec4 perspectiveVec;
	glm::decompose(worldMatrix, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);

	transform.worldMatrix   = worldMatrix;
	transform.worldPosition = translationVec;
	transform.worldRotation = rotationQuat;
	transform.worldScale    = scaleVec;
	transform.dirty         = false;

	return transform;
}

void Struktur::System::TransformSystem::CascadeDirty(entt::registry& registry, entt::entity entity)
{
	if (auto* children = registry.try_get<Component::Children>(entity))
	{
		for (auto child : children->entities)
		{
			if (registry.valid(child))
			{
				auto& childTransform = registry.get<Component::Transform>(child);
				childTransform.dirty = true;
				childTransform.version++;
				CascadeDirty(registry, child);
			}
		}
	}
}

glm::vec3 Struktur::System::TransformSystem::WorldToLocal(GameContext& context, const glm::vec3& worldPos,
                                                          entt::entity parentEntity)
{
	entt::registry& registry = context.GetRegistry();
	if (registry.try_get<Component::Transform>(parentEntity))
	{
		glm::mat4 parentInverse = glm::inverse(EnsureWorldUpToDate(context, parentEntity).worldMatrix);
		glm::vec4 localPos      = parentInverse * glm::vec4(worldPos, 1.0f);
		return glm::vec3(localPos);
	}
	BREAK_MSG("Entity does not have a world transform");
	return worldPos;
}

void Struktur::System::TransformSystem::SetLocalTransform(GameContext& context, entt::entity entity,
                                                          const glm::mat4& matrix)
{
	entt::registry& registry = context.GetRegistry();
	auto& transform          = registry.get<Component::Transform>(entity);

	glm::vec3 scaleVec;
	glm::quat rotationQuat{};
	glm::vec3 translationVec;
	glm::vec3 skewVec;
	glm::vec4 perspectiveVec;
	glm::decompose(matrix, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);

	transform.localMatrix   = matrix;
	transform.localPosition = translationVec;
	transform.localRotation = rotationQuat;
	transform.localScale    = scaleVec;
	transform.dirty         = true;
	transform.version++;

	CascadeDirty(registry, entity);
}

void Struktur::System::TransformSystem::SetLocalTransform(GameContext& context, entt::entity entity,
                                                          const glm::vec3& position, const glm::vec3& scale,
                                                          const glm::quat& rotation)
{
	entt::registry& registry = context.GetRegistry();
	auto& transform          = registry.get<Component::Transform>(entity);

	transform.localMatrix =
	    glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
	transform.localPosition = position;
	transform.localRotation = rotation;
	transform.localScale    = scale;
	transform.dirty         = true;
	transform.version++;

	CascadeDirty(registry, entity);
}

void Struktur::System::TransformSystem::SetWorldTransform(GameContext& context, entt::entity entity,
                                                          const glm::mat4& matrix)
{
	entt::registry& registry = context.GetRegistry();

	glm::mat4 localMatrix = matrix;
	if (auto* parent = registry.try_get<Component::Parent>(entity))
	{
		if (registry.valid(parent->entity) && registry.try_get<Component::Transform>(parent->entity))
		{
			glm::mat4 parentInverse = glm::inverse(EnsureWorldUpToDate(context, parent->entity).worldMatrix);
			localMatrix *= parentInverse;
		}
	}

	SetLocalTransform(context, entity, localMatrix);
}

void Struktur::System::TransformSystem::SetWorldTransform(GameContext& context, entt::entity entity,
                                                          const glm::vec3& position, const glm::vec3& scale,
                                                          const glm::quat& rotation)
{
	glm::mat4 matrix =
	    glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);

	SetWorldTransform(context, entity, matrix);
}

void Struktur::System::TransformSystem::SetWorldTransformDirect(GameContext& context, entt::entity entity,
                                                                const glm::vec3& position, const glm::vec3& scale,
                                                                const glm::quat& rotation)
{
	entt::registry& registry = context.GetRegistry();

	// An entity with a moving parent still needs the parent-inverse to produce a correct local
	// transform, so fall back to the full path there.
	if (auto* parent = registry.try_get<Component::Parent>(entity))
	{
		if (registry.valid(parent->entity) && registry.try_get<Component::Transform>(parent->entity))
		{
			SetWorldTransform(context, entity, position, scale, rotation);
			return;
		}
	}

	auto& transform = registry.get<Component::Transform>(entity);

	glm::mat4 matrix =
	    glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);

	transform.localMatrix   = matrix;
	transform.localPosition = position;
	transform.localScale    = scale;
	transform.localRotation = rotation;

	// World == local here (no parent), so we already know the resolved world state - write it
	// straight into the cache instead of leaving it dirty for a redundant decompose on next read.
	transform.worldMatrix   = matrix;
	transform.worldPosition = position;
	transform.worldScale    = scale;
	transform.worldRotation = rotation;
	transform.dirty         = false;

	CascadeDirty(registry, entity);
}

glm::vec3 Struktur::System::TransformSystem::GetWorldPosition(GameContext& context, entt::entity entity)
{
	return EnsureWorldUpToDate(context, entity).worldPosition;
}

glm::quat Struktur::System::TransformSystem::GetWorldRotation(GameContext& context, entt::entity entity)
{
	return EnsureWorldUpToDate(context, entity).worldRotation;
}

glm::vec3 Struktur::System::TransformSystem::GetWorldScale(GameContext& context, entt::entity entity)
{
	return EnsureWorldUpToDate(context, entity).worldScale;
}

glm::mat4 Struktur::System::TransformSystem::GetWorldMatrix(GameContext& context, entt::entity entity)
{
	return EnsureWorldUpToDate(context, entity).worldMatrix;
}
