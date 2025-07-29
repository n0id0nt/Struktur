#include "TransformSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"

#include "Debug/Assertions.h"

void Struktur::System::TransformSystem::UpdateWorldTransform(GameContext &context, entt::entity entity, const glm::mat4 &parentMatrix)
{
    entt::registry& registry = context.GetRegistry();
    auto& transform = registry.get<Component::LocalTransform>(entity);
    auto& worldTransform = registry.get_or_emplace<Component::WorldTransform>(entity);

    glm::mat4 localMatrix = transform.matrix;
    glm::mat4 worldMatrix = parentMatrix * localMatrix;

	glm::vec3 scaleVec;
	glm::quat rotationQuat{};
	glm::vec3 translationVec;
	glm::vec3 skewVec;
	glm::vec4 perspectiveVec;
	glm::decompose(worldMatrix, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);

    worldTransform.matrix = worldMatrix;
    worldTransform.position = translationVec;
    worldTransform.rotation = rotationQuat;
    worldTransform.scale = scaleVec;

    // Recursively update children
    if (auto* children = registry.try_get<Component::Children>(entity)) {
        for (auto child : children->entities) {
            if (registry.valid(child)) {
                UpdateWorldTransform(context, child, worldTransform.matrix);
            }
        }
    }
}

glm::vec3 Struktur::System::TransformSystem::WorldToLocal(GameContext &context, const glm::vec3 &worldPos, entt::entity parentEntity)
{
    entt::registry& registry = context.GetRegistry();
    if (auto* parentWorldTransform = registry.try_get<Component::WorldTransform>(parentEntity))
    {
        glm::mat4 parentInverse = glm::inverse(parentWorldTransform->matrix);
        glm::vec4 localPos = parentInverse * glm::vec4(worldPos, 1.0f);
        return glm::vec3(localPos);
    }
    BREAK_MSG("Entity does not have a world transform");
    return worldPos;
}

float Struktur::System::TransformSystem::GetWorldRotation(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    if (auto* worldTransform = registry.try_get<Component::WorldTransform>(entity)) 
    {
        return atan2(worldTransform->matrix[1][0], worldTransform->matrix[0][0]);
    }
    BREAK_MSG("Entity does not have a world transform");
    return 0.0f;
}

void Struktur::System::TransformSystem::SetLocalTransform(GameContext& context, entt::entity entity, const glm::mat4& matrix)
{
	entt::registry& registry = context.GetRegistry();
	auto& transform = registry.get<Component::LocalTransform>(entity);

	glm::vec3 scaleVec;
	glm::quat rotationQuat{};
	glm::vec3 translationVec;
	glm::vec3 skewVec;
	glm::vec4 perspectiveVec;
	glm::decompose(matrix, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);

    transform.matrix = matrix;
    transform.position = translationVec;
    transform.rotation = rotationQuat;
    transform.scale = scaleVec;

    glm::mat4 parentMatrix = glm::mat4(1.0f);

    if (auto* parent = registry.try_get<Component::Parent>(entity))
    {
        if (auto* parentWorldTransform = registry.try_get<Component::WorldTransform>(parent->entity))
        {
            parentMatrix = parentWorldTransform->matrix;
        }
        else
        {
            auto* parentLocalTransform = registry.try_get<Component::LocalTransform>(parent->entity);
            ASSERT_MSG(parentLocalTransform, "Parent Entt does not contain a local transform this suggests that this object was not created with the game object manager");
            SetLocalTransform(context, parent->entity, parentLocalTransform->matrix);
            auto* justCreatedParentWorldTransform = registry.try_get<Component::WorldTransform>(parent->entity);
            ASSERT_MSG(justCreatedParentWorldTransform, "Parent Entt does not contain a world transform. Something here is fucked");
			parentMatrix = justCreatedParentWorldTransform->matrix;
        }
    }

    UpdateWorldTransform(context, entity, parentMatrix);
}

void Struktur::System::TransformSystem::SetLocalTransform(GameContext& context, entt::entity entity, const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation)
{
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position) *
		               glm::mat4_cast(rotation) *
		               glm::scale(glm::mat4(1.0f), scale);

    SetLocalTransform(context, entity, matrix);
}

void Struktur::System::TransformSystem::SetWorldTransform(GameContext& context, entt::entity entity, const glm::mat4& matrix)
{
	entt::registry& registry = context.GetRegistry();
	auto& transform = registry.get<Component::LocalTransform>(entity);

    glm::mat4 parentMatrix = glm::mat4(1.0f);
    glm::mat4 localMatrix = matrix;
    if (auto* parent = registry.try_get<Component::Parent>(entity))
    {
        if (auto* parentWorldTransform = registry.try_get<Component::WorldTransform>(parent->entity))
        {
            glm::mat4 parentInverse = glm::inverse(parentWorldTransform->matrix);
            localMatrix *= parentInverse;
            parentMatrix = parentWorldTransform->matrix;
        }
    }

	glm::vec3 scaleVec;
	glm::quat rotationQuat{};
	glm::vec3 translationVec;
	glm::vec3 skewVec;
	glm::vec4 perspectiveVec;
	glm::decompose(localMatrix, scaleVec, rotationQuat, translationVec, skewVec, perspectiveVec);

	transform.matrix = localMatrix;
	transform.position = translationVec;
	transform.rotation = rotationQuat;
	transform.scale = scaleVec;

    UpdateWorldTransform(context, entity, parentMatrix);
}

void Struktur::System::TransformSystem::SetWorldTransform(GameContext& context, entt::entity entity, const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation)
{
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), position) *
		               glm::mat4_cast(rotation) *
		               glm::scale(glm::mat4(1.0f), scale);

    SetWorldTransform(context, entity, matrix);
}
