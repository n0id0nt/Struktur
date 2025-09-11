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
    namespace NPC
    {
        struct NPCData { int spriteIndex; };
        static std::unordered_map<std::string, NPCData> s_spriteDataMap = {
            {"Scholar", { 0 }, },
            {"Gardener", { 6 }, },
            {"Cook", { 4 }, },
            {"Inventor", { 2 }, },
            {"Dreamer", { 5 }, },
            {"Astronomer", { 8 }, },
            {"Merchant", { 1 }, },
            {"Guardian", { 7 }, },
            {"Cordelia", { 3 }, },
        };

        void Create(GameContext& context, entt::entity entity, const std::string& name)
        {
            entt::registry& registry = context.GetRegistry();
            System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
            Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
            System::SystemManager& systemManager = context.GetSystemManager();
            auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
            auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();
            auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();
            Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture = resourceManager.GetTexture("assets/Tiles/NPCs.png");

            const NPCData& npcData = s_spriteDataMap[name];
            registry.emplace<Component::Sprite>(entity, texture, WHITE, glm::vec2(64, 100), 9, 1, false, npcData.spriteIndex, 2);
            registry.emplace<Component::Interactable>(entity, name);
            b2BodyDef kinematicBodyDef;
            kinematicBodyDef.type = b2_staticBody;
            b2CircleShape playerShape;
            playerShape.m_radius = 0.25f;
            physicsSystem.CreatePhysicsBody(context, entity, kinematicBodyDef, playerShape);
            Component::PhysicsBody& physicsBody = registry.get<Component::PhysicsBody>(entity);
            physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
            physicsBody.syncToPhysics = true;     // Let transform drive physics
            Component::SpriteAnimation& spriteAnimation = registry.emplace<Component::SpriteAnimation>(entity);
            // animation could possibly be a resource stored in the resource pool and loaded in from a file.
            Animation::SpriteAnimation idleAnimation{ npcData.spriteIndex, npcData.spriteIndex + 1, 1.f, true };

            animationSystem.AddAnimation(context, entity, "idle", idleAnimation);
            animationSystem.PlayAnimation(context, entity, "idle");
        }
    }
}