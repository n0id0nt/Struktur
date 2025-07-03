#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "ECS/Component/Transform.h"
#include "ECS/System/HierrarchySystem.h"
#include "ECS/System/TransformSystem.h"
#include "Engine/Core/GameContext.h"
#include "Engine/Core/SystemManager.h"

namespace Struktur
{
	namespace System
	{
        class GameObjectManager : public Core::ISystem
        {
        private:
            HierarchySystem hierarchySystem;
            TransformSystem transformSystem;

        public:
            GameObjectManager(Core::GameContext& context) : hierarchySystem(), transformSystem() 
            {
                entt::registry& registry = context.GetRegistry();
                
                // Listen for entity destruction to clean up references
                registry.on_destroy<Component::Children>().connect<&GameObjectManager::OnChildrenDestroy>(*this);
            }

            entt::entity CreateGameObject(Core::GameContext& context, entt::entity parent = entt::null) 
            {
                entt::registry& registry = context.GetRegistry();

                auto entity = registry.create();
                registry.emplace<Component::Transform>(entity);
                
                if (parent != entt::null) 
                {
                    hierarchySystem.SetParent(context, entity, parent);
                }
                
                return entity;
            }

            void DestroyGameObject(Core::GameContext& context, entt::entity entity) 
            {
                hierarchySystem.DestroyEntity(context, entity);
            }

            void Update(Core::GameContext& context) override 
            {
                transformSystem.UpdateTransforms(context);
            }

        private:
            void OnChildrenDestroy(entt::registry& reg, entt::entity entity) 
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
        };
    }
}