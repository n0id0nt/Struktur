#pragma once

#include "Engine/GameContext.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Level.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"

#include "Engine/Core/Resource/TextureResource.h"

namespace Struktur
{
    namespace Door
    {
        void Create(GameContext& context, entt::entity entity)
        {
            entt::registry& registry = context.GetRegistry();
            System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
            Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
            System::SystemManager& systemManager = context.GetSystemManager();
            auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
            auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();

            registry.emplace<Component::Interactable>(entity, "Entrance Door");
            b2BodyDef kinematicBodyDef;
            kinematicBodyDef.type = b2_staticBody;
            b2PolygonShape playerShape;
            playerShape.SetAsBox(0.6,0.2);
            physicsSystem.CreatePhysicsBody(context, entity, kinematicBodyDef, playerShape);
            Component::PhysicsBody& physicsBody = registry.get<Component::PhysicsBody>(entity);
            physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
            physicsBody.syncToPhysics = true;     // Let transform drive physics
        }
    }
}