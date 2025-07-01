#pragma once

#include <memory>
#include <stdexcept>
#include "Engine/Core/Input.h"
#include "Engine/Core/GameData.h"

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
            }
            Input& GetInput() const { 
                if (!m_input) throw std::runtime_error("Input not initialized");
                return *m_input; 
            }
            
            GameData& GetGameData() const { 
                if (!m_gameData) throw std::runtime_error("GameData not initialized");
                return *m_gameData; 
            }

        private:
            std::unique_ptr<GameData> m_gameData;
            std::unique_ptr<Input> m_input;
        };
    };
};