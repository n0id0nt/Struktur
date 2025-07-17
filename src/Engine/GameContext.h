#pragma once

#include <memory>
#include <stdexcept>
#include "entt/entt.hpp"

#include "Engine/Core/Input.h"
#include "Engine/Core/GameData.h"
#include "Engine/Core/ResourcePool.h"

#include "Engine/ECS/SystemManager.h"
#include "Engine/ECS/GameObjectManager.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Game/Camera.h"
#include "Debug/Assertions.h"

namespace Struktur
{
    class GameContext 
    {
    public:
        GameContext() 
        {
            m_input = std::make_unique<Core::Input>(0);
            m_gameData = std::make_unique<Core::GameData>();
            m_registry = std::make_unique<entt::registry>();
            m_resourcePool = std::make_unique<Core::ResourcePool>();
            m_systemManager = std::make_unique<System::SystemManager>();
            m_gameObjectManager = std::make_unique<System::GameObjectManager>();
            m_camera = std::make_unique<GameResource::Camera>();
            
            //TODO These variables don't belong here - Possibly initialise these from a file or just pass them in??
            glm::vec2 gravity(0.0f, 0.0f);
            int32 velocityIterations = 8;
            int32 positionIterations = 3;
            float pixelsPerMeter = 32.f;

            m_physicsWorld = std::make_unique<Physics::PhysicsWorld>(gravity, velocityIterations, positionIterations, pixelsPerMeter);
        }

        Core::Input& GetInput() const { 
            ASSERT_MSG(m_input.get(), "Input not initialized");
            return *m_input; 
        }
        
        Core::GameData& GetGameData() const { 
            ASSERT_MSG(m_gameData.get(), "GameData not initialized");
            return *m_gameData; 
        }

        entt::registry& GetRegistry() const {
            ASSERT_MSG(m_registry.get(), "Registry not initialized");
            return *m_registry;
        }

        Core::ResourcePool& GetResourcePool() const {
            ASSERT_MSG(m_resourcePool.get(), "Resource Pool not initialized");
            return *m_resourcePool;
        }

        System::SystemManager& GetSystemManager() const {
            ASSERT_MSG(m_systemManager.get(), "System Manager not initialized");
            return *m_systemManager;
        }

        System::GameObjectManager& GetGameObjectManager() const {
            ASSERT_MSG(m_gameObjectManager.get(), "Game Object Manager not initialized");
            return *m_gameObjectManager;
        }

        Physics::PhysicsWorld& GetPhysicsWorld() const {
            ASSERT_MSG(m_physicsWorld.get(), "Physics World not initialized");
            return *m_physicsWorld;
        }

        GameResource::Camera& GetCamera() const {
            ASSERT_MSG(m_camera.get(), "Camera not initialized");
            return *m_camera;
        }

    private:
        std::unique_ptr<Core::GameData> m_gameData;
        std::unique_ptr<Core::Input> m_input;
        std::unique_ptr<entt::registry> m_registry;
        std::unique_ptr<Core::ResourcePool> m_resourcePool;
        std::unique_ptr<System::SystemManager> m_systemManager;
        std::unique_ptr<System::GameObjectManager> m_gameObjectManager;
        std::unique_ptr<Physics::PhysicsWorld> m_physicsWorld;
        std::unique_ptr<GameResource::Camera> m_camera;
    };
}
