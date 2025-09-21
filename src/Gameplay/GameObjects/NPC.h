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
#include "Engine/ECS/Component/Shader.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"

#include "Engine/Core/Resource/TextureResource.h"

namespace Struktur
{
    namespace NPC
    {
        struct NPCData
        {
            int spriteIndex;
            int xOffset, yOffset;
        };

        static std::unordered_map<std::string, NPCData> s_spriteDataMap = {
            {"Scholar", { 0, 48, 64 }, },
            {"Gardener", { 6, 48, 64 }, },
            {"Cook", { 4, 48, 64 }, },
            {"Inventor", { 2, 48, 64 }, },
            {"Dreamer", { 5, 60, 48 }, },
            {"Astronomer", { 8, 48, 64 }, },
            {"Merchant", { 1, 48, 64 }, },
            {"Guardian", { 7, 48, 64 }, },
            {"Cordelia", { 3, 48, 64 }, },
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
            auto& worldTransform = registry.get<Component::WorldTransform>(entity);
            registry.emplace<Component::Sprite>(entity, texture, WHITE, glm::vec2(npcData.xOffset, npcData.yOffset), 9, 1, false, npcData.spriteIndex, (int)worldTransform.position.y);
            registry.emplace<Component::Interactable>(entity, name);
            b2BodyDef kinematicBodyDef;
            kinematicBodyDef.type = b2_staticBody;
            b2CircleShape playerShape;
            playerShape.m_radius = 0.25f;
            physicsSystem.CreatePhysicsBody(context, entity, kinematicBodyDef, playerShape);
            Component::PhysicsBody& physicsBody = registry.get<Component::PhysicsBody>(entity);
            physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
            physicsBody.syncToPhysics = true;     // Let transform drive physics
            
            ::Shader soulShader = ::LoadShader("assets/Shaders/SoulEffect.vs", "assets/Shaders/SoulEffect.fs");
            registry.emplace<Component::Shader>(entity, soulShader, ::Vector3{0.3f, 0.7f, 1.0f}, 1.5f, 2.0f, 15.0f, 0.05f, 8.0f, 3.0f, ::Vector2{1.0f, 0.3f});

            //Component::SpriteAnimation& spriteAnimation = registry.emplace<Component::SpriteAnimation>(entity);
            //// animation could possibly be a resource stored in the resource pool and loaded in from a file.
            //Animation::SpriteAnimation idleAnimation{ (unsigned int)npcData.spriteIndex, (unsigned int)npcData.spriteIndex + 1u, 1.f, true };
//
            //animationSystem.AddAnimation(context, entity, "idle", idleAnimation);
            //animationSystem.PlayAnimation(context, entity, "idle");
        }
    }
}