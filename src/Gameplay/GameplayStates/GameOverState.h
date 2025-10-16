#pragma once

#include <cstdlib>
#include <algorithm>
#include <format>
//#include <ctime>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/GameContext.h"

#include "Engine/Game/State.h"
#include "Engine/Game/StateManager.h"

#include "engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/CameraSystem.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/Camera.h"

#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIPanel.h"

#include "Gameplay/GameObjects/Player.h"

namespace Struktur
{    
	namespace GamePlay
	{
		class GameOverState : public GameResource::IState
		{
        private:
            UI::UIPanel* m_screenPanel;

        public:
            GameOverState() {}

            void Enter(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // Create Call the interact function and get the player to interact with

                Core::GameData& gameDate = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                Resource::ResourceManager& resourceManager = context.GetResourceManager();
                Resource::ResourcePtr<Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120");

                // UI layout
                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                m_screenPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0, 0}, glm::vec2{gameDate.screenWidth, gameDate.screenHeight}, glm::vec2{0, 0});
                m_screenPanel->SetBackgroundColor(Color{ 0, 0, 0, 200 }); // don't render this
                m_screenPanel->SetBorderColor(BLANK);

                auto* textBackgroundPanel = static_cast<UI::UILabel*>(m_screenPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ -20, -20 }, glm::vec2{ 0.5f, 0.5f }, "Game Complete", 60.0f)));
                textBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 0.5f});
                textBackgroundPanel->SetTextColor(WHITE);
                textBackgroundPanel->SetFont(font);
            }

            void Update(GameContext& context, GameResource::StateManager& stateManager) override
            {
            }
            void Render(GameContext& context, GameResource::StateManager& stateManager) override {}
            void Exit(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // delete all players
                // delete all UI
				UI::UIManager& uiManager = context.GetUIManager();
                uiManager.RemoveElement(m_screenPanel);
            }

            std::string GetStateName() const override { return std::string(typeid(GameOverState).name()); }
        };
    }
}
