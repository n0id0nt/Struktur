#pragma once

#include <memory>
#include <stdexcept>
#include "entt/entt.hpp"
#include "Debug/Assertions.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/GameData.h"
#include "Engine/Core/ResourcePool.h"
#include "Engine/Core/SystemManager.h"

namespace Struktur
{
	namespace Core
	{
        class GameContext 
        {
        public:
            GameContext() {
                m_input = std::make_unique<Input>(0);
                m_gameData = std::make_unique<GameData>();
                m_registry = std::make_unique<entt::registry>();
                m_resourcePool = std::make_unique<ResourcePool>();
                m_systemManager = std::make_unique<SystemManager>();
            }

            Input& GetInput() const { 
                ASSERT_MSG(m_input.get(), "Input not initialized");
                return *m_input; 
            }
            
            GameData& GetGameData() const { 
                ASSERT_MSG(m_gameData.get(), "GameData not initialized");
                return *m_gameData; 
			}

            entt::registry& GetRegistry() const {
                ASSERT_MSG(m_registry.get(), "Registry not initialized");
				return *m_registry;
			}

            ResourcePool& GetResourcePool() const {
                ASSERT_MSG(m_resourcePool.get(), "Resource Pool not initialized");
				return *m_resourcePool;
			}

            SystemManager& GetSystemManager() const {
                ASSERT_MSG(m_systemManager.get(), "System Manager not initialized");
				return *m_systemManager;
			}

        private:
            std::unique_ptr<GameData> m_gameData;
            std::unique_ptr<Input> m_input;
            std::unique_ptr<entt::registry> m_registry;
            std::unique_ptr<ResourcePool> m_resourcePool;
            std::unique_ptr<SystemManager> m_systemManager;
        };
    };
};