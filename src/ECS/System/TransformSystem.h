#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "ECS/Component/Transform.h"
#include "Engine/Core/GameContext.h"

namespace Struktur
{
	namespace System
	{
        class TransformSystem {
        public:
            TransformSystem() {}

            void UpdateTransforms(Core::GameContext& context)
            {
                entt::registry& registry = context.GetRegistry();
                
                // First, update all root transforms (entities without parents)
                auto rootView = registry.view<Component::Transform>(entt::exclude<Component::Parent>);
                for (auto entity : rootView) {
                    UpdateWorldTransform(context, entity, glm::mat4(1.0f));
                }
            }

        private:
            void UpdateWorldTransform(Core::GameContext& context, entt::entity entity, const glm::mat4& parentMatrix)
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
        };
    }
}