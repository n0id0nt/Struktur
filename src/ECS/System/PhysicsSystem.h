#pragma once
#include "glm/glm.hpp"
#include "entt/entt.hpp"
#include "box2d/box2d.h"

#include "Engine/Core/SystemManager.h"

namespace Struktur
{
    namespace Core
    {
        class GameContext;
    }
	namespace System
	{
        class PhysicsSystem : public Core::ISystem
        {
        private:
            std::unique_ptr<b2World> m_physicsWorld;
            const float m_pixelsPerPhysicsMeter = 32.f; // pixels per game unit

        public:
            PhysicsSystem();
            
            void Update(Core::GameContext& context) override;

            void StepPhysics(Core::GameContext& context, float deltaTime);
            void SyncPhysicsToTransforms(Core::GameContext& context);
            void SyncTransformsToPhysics(Core::GameContext& context) ;

            b2Body* CreatePhysicsBody(Core::GameContext& context, entt::entity entity, const b2BodyDef& bodyDef);
            
        private:
            glm::vec3 WorldToLocal(Core::GameContext& context, const glm::vec3& worldPos, entt::entity parentEntity);

            float GetWorldRotation(Core::GameContext& context, entt::entity entity);
        };
    }
}