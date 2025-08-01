#pragma once

#include <cstdlib>
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

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"

#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIPanel.h"

#include "Gameplay/GameObjects/Player.h"

namespace Struktur
{    
	namespace GamePlay
	{
		class InventoryState : public GameResource::IState
		{
        private:
            UI::UIPanel* m_screenPanel;

            std::string m_highlightedItemId;

        public:
            InventoryState() {}

            void Enter(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // Create Call the interact function and get the player to interact with
                Inventory& inventory = context.GetInventory();
                Core::GameData& gameDate = context.GetGameData();
                Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/machine-std/machine-std-regular.ttf_120");

                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                m_screenPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0, 0}, glm::vec2{gameDate.screenWidth, gameDate.screenHeight}, glm::vec2{0, 0});
                m_screenPanel->SetBackgroundColor(BLANK); // don't render this
                m_screenPanel->SetBorderColor(BLANK);

                auto* inventoryBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0.3f, 0.5f}, glm::vec2{500, 500}, glm::vec2{0, 0})));
                inventoryBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 0.5f});
                inventoryBackgroundPanel->SetBackgroundColor(LIGHTGRAY);
                inventoryBackgroundPanel->SetBorderColor(BLACK);
                inventoryBackgroundPanel->SetBorderWidth(2.0f);

                auto* focusedBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0.8f, 0.5f}, glm::vec2{300, 500}, glm::vec2{0, 0})));
                focusedBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 0.5f});
                focusedBackgroundPanel->SetBackgroundColor(DARKGRAY);
                focusedBackgroundPanel->SetBorderColor(WHITE);
                focusedBackgroundPanel->SetBorderWidth(2.0f);

                auto* inventoryFocusedItemNameLabel = static_cast<UI::UILabel*>(focusedBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 0.0f, 5.0f }, glm::vec2{ 0.5f, 0.5f }, "No Items", 45.0f)));
                inventoryFocusedItemNameLabel->SetTextColor(WHITE);
                inventoryFocusedItemNameLabel->SetAnchorPoint(glm::vec2{ 0.5f, 0.0f });
                auto* inventoryFocusedItemDescriptionLabel = static_cast<UI::UILabel*>(focusedBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 5.0f, 55.0f }, glm::vec2{ 0.0f, 0.5f }, "No Items in your inventory.\n\nGo collect items and\nprogress the game", 20.0f)));
                inventoryFocusedItemDescriptionLabel->SetTextColor(WHITE);
                inventoryFocusedItemDescriptionLabel->SetAnchorPoint(glm::vec2{ 0.0f, 0.0f });

                int index = 0;
                float curX = 10.f;
                for (auto& item : inventory)
                {
                    auto* inventoryItemPanel = static_cast<UI::UIPanel*>(inventoryBackgroundPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{curX, 10.0f}, glm::vec2{0.f, 0.f}, glm::vec2{50, 50}, glm::vec2{0, 0})));
                    inventoryItemPanel->SetBackgroundColor(DARKGRAY);
                    inventoryItemPanel->SetBorderColor(WHITE);
                    inventoryItemPanel->SetBorderWidth(2.0f);
                    inventoryItemPanel->SetFocusable(true);
                    inventoryItemPanel->SetOnFocus([item, inventoryFocusedItemNameLabel, inventoryFocusedItemDescriptionLabel](UI::UIElement* sender) {
                        inventoryFocusedItemNameLabel->SetText(item);
                        inventoryFocusedItemDescriptionLabel->SetText(item);
                    });
                    focusNavigator->RegisterElement(inventoryItemPanel);
                    curX += 70.f;
                    
                    auto* inventoryItemLabel = static_cast<UI::UILabel*>(inventoryItemPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 0.0f, 0.0f }, glm::vec2{ 0.5f, 1.0f }, item, 15.0f)));
                    inventoryItemLabel->SetTextColor(WHITE);
                    inventoryItemLabel->SetAnchorPoint(glm::vec2{0.5f, 1.0f});
                    
                    if (index == 0)
                    {
                        uiManager.SetFocus(inventoryItemPanel);
                    }
                    index++;
                }
                
            }

            void Update(GameContext& context, GameResource::StateManager& stateManager) override
            {
                Core::Input input = context.GetInput();
                bool inputInvetory = input.IsInputJustReleased("Inventory");
                if (inputInvetory)
                {
                    stateManager.ChangeState(context, std::unique_ptr<GameResource::IState>(nullptr));
                    return;
                }
            }
            void Render(GameContext& context, GameResource::StateManager& stateManager) override {}
            void Exit(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // delete all players
                // delete all UI
				UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();
                uiManager.SetFocus(nullptr);
                uiManager.RemoveElement(m_screenPanel);
            }

            std::string GetStateName() const override { return std::string(typeid(InventoryState).name()); }
        };
    }
}