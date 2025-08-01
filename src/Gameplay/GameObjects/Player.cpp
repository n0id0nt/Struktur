#include "Player.h"

#include <limits>

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

constexpr static const float INTERACTABLE_DISTANCE = 64.0f;

void Struktur::Player::Create(GameContext &context, entt::entity entity)
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
    Animation::SpriteAnimation idleAnimation{ 24u, 28u, 1.f, true };
    Animation::SpriteAnimation runAnimation{ 28u, 33u, 0.7f, true };

    animationSystem.AddAnimation(context, entity, "idle", idleAnimation);
    animationSystem.AddAnimation(context, entity, "run", runAnimation);
    animationSystem.PlayAnimation(context, entity, "idle");
}

void Struktur::Player::PlayerForceStop(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    System::SystemManager& systemManager = context.GetSystemManager();
    auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

    auto& physicsBody = registry.get<Component::PhysicsBody>(entity);
    auto& spriteAnimation = registry.get<Component::SpriteAnimation>(entity);

    b2Vec2 velecity = b2Vec2_zero;
    physicsBody.body->SetLinearVelocity(velecity);
    if (!animationSystem.IsAnimationPlaying(context, entity, "idle"))
    {
        animationSystem.PlayAnimation(context, entity, "idle");
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
    }

    b2Vec2 velecity = b2Vec2(dir.x *  player.speed, dir.y * -player.speed);
    physicsBody.body->SetLinearVelocity(velecity);
    if (glm::length(dir) > 0.001f)
    {
        if (!animationSystem.IsAnimationPlaying(context, entity, "run"))
        {
            animationSystem.PlayAnimation(context, entity, "run");
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
        if (!animationSystem.IsAnimationPlaying(context, entity, "idle"))
        {
            animationSystem.PlayAnimation(context, entity, "idle");
        }
    }
}

entt::entity Struktur::Player::CanInteract(GameContext &context, entt::entity entity)
{
    entt::registry& registry = context.GetRegistry();
    glm::vec3& playerWorldPosition = registry.get<Component::WorldTransform>(entity).position;
    entt::entity closestEntity = entt::null;
    float closestDistance = std::numeric_limits<float>::infinity();

    auto view = registry.view<Component::Interactable, Component::WorldTransform>();
    for (auto& interactableEntity : view)
    {
        glm::vec3& interactableWorldPosition = registry.get<Component::WorldTransform>(interactableEntity).position;
        float distance = glm::distance(interactableWorldPosition, playerWorldPosition);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestEntity = interactableEntity;
        }
    }
    if (closestDistance < INTERACTABLE_DISTANCE)
    {
        return closestEntity;
    }

    return entt::null;
}
