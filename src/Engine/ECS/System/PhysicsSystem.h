#pragma once

#include "entt/entt.hpp"
#include "box2d/box2d.h"

#include "Engine/ECS/SystemManager.h"

namespace Struktur
{
    class GameContext;

	namespace System
	{
        class PhysicsSystem : public ISystem
        {
        public:         
            void Update(GameContext& context) override;

            void StepPhysics(GameContext& context, float deltaTime);
            void SyncPhysicsToTransforms(GameContext& context);
            void SyncTransformsToPhysics(GameContext& context) ;

            b2Body* CreatePhysicsBody(GameContext& context, entt::entity entity, const b2BodyDef& bodyDef);
        };
    }
}
