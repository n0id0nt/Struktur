#include "PhysicsSystem.h"

#include "glm/gtc/quaternion.hpp."
#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/Physics/PhysicsWorld.h"

void Struktur::System::PhysicsSystem::Update(GameContext &context)
{
    float deltaTime = context.GetGameData().deltaTime;
    StepPhysics(context, deltaTime);
}

void Struktur::System::PhysicsSystem::StepPhysics(GameContext &context, float deltaTime)
{
    Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

    SyncTransformsToPhysics(context);
    physicsWorld.Step(deltaTime);
    SyncPhysicsToTransforms(context);
}

void Struktur::System::PhysicsSystem::SyncPhysicsToTransforms(GameContext &context)
{
    entt::registry& registry = context.GetRegistry();
    Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();
    TransformSystem& transformSystem = context.GetSystemManager().GetSystem<TransformSystem>();

    auto view = registry.view<Component::PhysicsBody, Component::Transform>();
    
    for (auto [entity, physicsBody, transform] : view.each())
    {
        if (physicsBody.body && physicsBody.syncFromPhysics) 
        {
            // Get world position from physics
            b2Vec2 position = physicsBody.body->GetPosition();
            float angle = physicsBody.body->GetAngle();
            
            // Convert to local space if entity has a parent
            if (auto* parent = registry.try_get<Component::Parent>(entity)) 
            {
                if (parent->entity != entt::null) 
                {
                    glm::vec3 worldPos(position.x * physicsWorld.GetPixelsPerMeter(), position.y * physicsWorld.GetPixelsPerMeter(), 0.0f);
                    glm::vec3 localPos = transformSystem.WorldToLocal(context, worldPos, parent->entity);
                    transform.position = localPos;
                    
                    // Handle rotation relative to parent
                    float parentAngle = transformSystem.GetWorldRotation(context, parent->entity);
                    float localAngle = angle - parentAngle;
                    transform.rotation = glm::angleAxis(localAngle, glm::vec3(0, 0, 1));
                }
                else
                {
                    // No parent, use world coordinates directly
                    transform.position = glm::vec3(position.x * physicsWorld.GetPixelsPerMeter(), position.y * physicsWorld.GetPixelsPerMeter(), 0.0f);
                    transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
                }
            }
            else
            {
                // No parent, use world coordinates directly
                transform.position = glm::vec3(position.x * physicsWorld.GetPixelsPerMeter(), position.y * physicsWorld.GetPixelsPerMeter(), 0.0f);
                transform.rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));
            }
        }
    }
}

void Struktur::System::PhysicsSystem::SyncTransformsToPhysics(GameContext &context)
{
    entt::registry& registry = context.GetRegistry();
    Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

    auto view = registry.view<Component::PhysicsBody, Component::Transform, Component::WorldTransform>();
    
    for (auto [entity, physicsBody, transform, worldTransform] : view.each())
    {
        if (physicsBody.body && physicsBody.syncToPhysics && physicsBody.isDirty)
        {
            // Extract position and rotation from world transform matrix
            glm::vec3 worldPos = glm::vec3(worldTransform.matrix[3]) / physicsWorld.GetPixelsPerMeter();
            
            // For 2D, extract rotation from matrix
            float angle = atan2(worldTransform.matrix[1][0], worldTransform.matrix[0][0]);
            
            physicsBody.body->SetTransform(b2Vec2(worldPos.x, worldPos.y), angle);
            physicsBody.isDirty = false;
        }
    }
}

b2Body *Struktur::System::PhysicsSystem::CreatePhysicsBody(GameContext &context, entt::entity entity, const b2BodyDef &bodyDef)
{
    entt::registry& registry = context.GetRegistry();
    Physics::PhysicsWorld& physicsWorld = context.GetPhysicsWorld();

    b2Body* body = physicsWorld.CreateBody(&bodyDef);
    body->GetUserData().pointer = static_cast<uintptr_t>(entity);

    //Create Ball shape
    b2PolygonShape groundBox;
    groundBox.SetAsBox(1 / 2.0f, 1 / 2.0f);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &groundBox;
    fixtureDef.density = 1.f;
    fixtureDef.friction = 0.4;
    fixtureDef.restitution = 0.f; 

    body->CreateFixture(&fixtureDef);
    
    registry.emplace<Component::PhysicsBody>(entity, body, bodyDef.type == b2_kinematicBody);
    
    return body;
}
