#include "TransformSystem.h"

#include "glm/gtc/quaternion.hpp."

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"

#include "Debug/Assertions.h"

void Struktur::System::TransformSystem::Update(GameContext &context)
{
    entt::registry& registry = context.GetRegistry();
    
    // First, update all root transforms (entities without parents)
    auto rootView = registry.view<Component::Transform>(entt::exclude<Component::Parent>);
    for (auto entity : rootView) {
        UpdateWorldTransform(context, entity, glm::mat4(1.0f));
    }
}

void Struktur::System::TransformSystem::UpdateWorldTransform(GameContext &context, entt::entity entity, const glm::mat4 &parentMatrix)
{
    entt::registry& registry = context.GetRegistry();
    auto& transform = registry.get<Component::Transform>(entity);
    auto& worldTransform = registry.get_or_emplace<Component::WorldTransform>(entity);

    // Calculate local matrix
    glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), transform.position) *
                            glm::mat4_cast(transform.rotation) *
                            glm::scale(glm::mat4(1.0f), transform.scale);

    // Apply parent transformation
    worldTransform.matrix = parentMatrix * localMatrix;

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
    if (auto* parentWorld = registry.try_get<Component::WorldTransform>(parentEntity)) 
    {
        glm::mat4 parentInverse = glm::inverse(parentWorld->matrix);
        glm::vec4 localPos = parentInverse * glm::vec4(worldPos, 1.0f);
        return glm::vec3(localPos);
    }
    ASSERT_MSG(false, "Entity does not have a world transform");
    return worldPos;
}

float Struktur::System::TransformSystem::GetWorldRotation(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    if (auto* worldTransform = registry.try_get<Component::WorldTransform>(entity)) 
    {
        return atan2(worldTransform->matrix[1][0], worldTransform->matrix[0][0]);
    }
    ASSERT_MSG(false, "Entity does not have a world transform");
    return 0.0f;
}
