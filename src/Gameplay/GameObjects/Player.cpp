#include "Player.h"

#include <limits>
#include <format>

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
#include "Engine/ECS/Component/WrenScript.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"

#include "Engine/Resource/TextureResource.h"

constexpr static const float INTERACTABLE_DISTANCE = 64.0f;

std::string GetPlayerAnimimation(const glm::vec2& facing, const std::string& animationType)
{
    if (facing.x > 0.01 || facing.x < -0.01)
    {
        return std::format("side{}Animation", animationType);
    }
    if (facing.y > 0.01)
    {
        return std::format("up{}Animation", animationType);
    }
    else
    {
        return std::format("down{}Animation", animationType);
    }
}

void Struktur::Player::Create(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
    Resource::ResourceManager& resourceManager = context.GetResourceManager();
    System::SystemManager& systemManager = context.GetSystemManager();
    auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
    auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();
    auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

    Component::WrenScript& scriptComponent = registry.emplace<Component::WrenScript>(entity, "Assets/Scripts/GameObjects/Player.wren", "Player", "Test Player Name");

    Resource::ResourcePtr<Resource::TextureResource> texture = resourceManager.GetTexture("assets/Tiles/PlayerSpriteSheet.png");
    registry.emplace<Component::Sprite>(entity, texture, WHITE, glm::vec2(48, 64), 6, 3, false, 0, 3);
    registry.emplace<Component::Player>(entity, 5.f);
    Component::Camera& parentCamera = registry.emplace<Component::Camera>(entity);
    parentCamera.zoom = 1.5f;
    parentCamera.forcePosition = true;
    parentCamera.damping = glm::vec2(4.f, 4.f);
    b2BodyDef kinematicBodyDef;
    kinematicBodyDef.type = b2_dynamicBody;
    b2CircleShape playerShape;
    playerShape.m_radius = 0.25f;
    physicsSystem.CreatePhysicsBody(context, entity, kinematicBodyDef, playerShape);
    Component::PhysicsBody& physicsBody = registry.get<Component::PhysicsBody>(entity);
    physicsBody.body->SetFixedRotation(true);
    physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
    physicsBody.syncToPhysics = true;     // Let transform drive physics
    Component::SpriteAnimation& spriteAnimation = registry.emplace<Component::SpriteAnimation>(entity);
    // animation could possibly be a resource stored in the resource pool and loaded in from a file.
    Animation::SpriteAnimation downIdleAnimation{ 0u, 2u, 1.f, true };
    Animation::SpriteAnimation upIdleAnimation{ 6u, 8u, 1.f, true };
    Animation::SpriteAnimation sideIdleAnimation{ 12u, 14u, 1.f, true };
    Animation::SpriteAnimation downRunAnimation{ 2u, 6u, 0.7f, true };
    Animation::SpriteAnimation upRunAnimation{ 8u, 12u, 0.7f, true };
    Animation::SpriteAnimation sideRunAnimation{ 14u, 18u, 0.7f, true };

    animationSystem.AddAnimation(context, entity, "upIdleAnimation", upIdleAnimation);
    animationSystem.AddAnimation(context, entity, "downIdleAnimation", downIdleAnimation);
    animationSystem.AddAnimation(context, entity, "sideIdleAnimation", sideIdleAnimation);
    animationSystem.AddAnimation(context, entity, "upRunAnimation", upRunAnimation);
    animationSystem.AddAnimation(context, entity, "downRunAnimation", downRunAnimation);
    animationSystem.AddAnimation(context, entity, "sideRunAnimation", sideRunAnimation);
}

void Struktur::Player::PlayerForceStop(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    System::SystemManager& systemManager = context.GetSystemManager();
    auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();
    auto& player = registry.get<Component::Player>(entity);
    auto& physicsBody = registry.get<Component::PhysicsBody>(entity);
    auto& spriteAnimation = registry.get<Component::SpriteAnimation>(entity);

    b2Vec2 velecity = b2Vec2_zero;
    physicsBody.body->SetLinearVelocity(velecity);
    
    std::string animation = GetPlayerAnimimation(player.facing, "Idle");
    if (!animationSystem.IsAnimationPlaying(context, entity, animation))
    {
        animationSystem.PlayAnimation(context, entity, animation);
    }
}

void Struktur::Player::PlayerControl(GameContext &context, entt::entity entity, glm::vec2 dir)
{
    entt::registry& registry = context.GetRegistry();
    System::SystemManager& systemManager = context.GetSystemManager();
    auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

    auto& player = registry.get<Component::Player>(entity);
    auto& physicsBody = registry.get<Component::PhysicsBody>(entity);
    auto& sprite = registry.get<Component::Sprite>(entity);
    auto& spriteAnimation = registry.get<Component::SpriteAnimation>(entity);

    if (glm::length(dir) > 0.001f)
    {
        dir = glm::normalize(dir);
        player.facing = dir;
    }

    b2Vec2 velecity = b2Vec2(dir.x *  player.speed, dir.y * -player.speed);
    physicsBody.body->SetLinearVelocity(velecity);
    if (glm::length(dir) > 0.001f)
    {
        std::string animation = GetPlayerAnimimation(player.facing, "Run");
        if (!animationSystem.IsAnimationPlaying(context, entity, animation))
        {
            animationSystem.PlayAnimation(context, entity, animation);
        }

        if (dir.x > 0)
        {
            sprite.flipped = false;
        }
        else if (dir.x < 0)
        {
            sprite.flipped = true;
        }
    }
    else
    {
        std::string animation = GetPlayerAnimimation(player.facing, "Idle");
        if (!animationSystem.IsAnimationPlaying(context, entity, animation))
        {
            animationSystem.PlayAnimation(context, entity, animation);
        }
    }
}

entt::entity Struktur::Player::CanInteract(GameContext &context, entt::entity entity)
{
    return entt::null;
}
