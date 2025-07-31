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
    namespace Item
    {
        void Create(GameContext& context, entt::entity entity)
        {
            entt::registry& registry = context.GetRegistry();
            System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
            Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
            System::SystemManager& systemManager = context.GetSystemManager();
            auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
            auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();
            auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();
            Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture = resourceManager.GetTexture("assets/Tiles/PlayerGrowthSprites.png");

            registry.emplace<Component::Sprite>(entity, texture, WHITE, glm::vec2(32, 48), 12, 5, false, 0);
        }
    }
}