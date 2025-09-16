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
                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/medieval_sharp/MedievalSharp-Bold.ttf_120");

                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                m_screenPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0, 0}, glm::vec2{gameDate.screenWidth, gameDate.screenHeight}, glm::vec2{0, 0});
                m_screenPanel->SetBackgroundColor(BLANK); // don't render this
                m_screenPanel->SetBorderColor(BLANK);

                auto* inventoryBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0.3f, 0.5f}, glm::vec2{394, 500}, glm::vec2{0, 0})));
                inventoryBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 0.5f});
                inventoryBackgroundPanel->SetBorderColor(BLANK);
                inventoryBackgroundPanel->SetBorderWidth(2.0f);
                Core::Resource::ResourcePtr<Core::Resource::TextureResource> inventoryBackgroundPanelTexture = resourceManager.GetTexture("assets/Tiles/InventoryBackgroundPanel.png");
                inventoryBackgroundPanel->SetBackgroundTexture(inventoryBackgroundPanelTexture);

                auto* focusedBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0.7f, 0.5f}, glm::vec2{400, 500}, glm::vec2{0, 0})));
                focusedBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 0.5f});
                focusedBackgroundPanel->SetBorderColor(BLANK);
                focusedBackgroundPanel->SetBorderWidth(2.0f);
                Core::Resource::ResourcePtr<Core::Resource::TextureResource> focusedItemBackgroundPanelTexture = resourceManager.GetTexture("assets/Tiles/FocusedItemBackgroundPanel.png");
                focusedBackgroundPanel->SetBackgroundTexture(focusedItemBackgroundPanelTexture);
                
                auto* inventoryFocusedItemNameLabel = static_cast<UI::UILabel*>(focusedBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 0.0f, -20.0f }, glm::vec2{ 0.5f, 0.75f }, "No Items", 30.0f)));
                inventoryFocusedItemNameLabel->SetTextColor(WHITE);
                inventoryFocusedItemNameLabel->SetAnchorPoint(glm::vec2{ 0.5f, 0.0f });
                inventoryFocusedItemNameLabel->SetFont(font);
                //auto* inventoryFocusedItemDescriptionLabel = static_cast<UI::UILabel*>(focusedBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 35.0f, 70.0f }, glm::vec2{ 0.0f, 0.5f }, "No Items in your inventory.\n\nGo collect items and\nprogress the game", 16.0f)));
                //inventoryFocusedItemDescriptionLabel->SetTextColor(WHITE);
                //inventoryFocusedItemDescriptionLabel->SetAnchorPoint(glm::vec2{ 0.0f, 0.0f });

                auto* focusedItemPanel = static_cast<UI::UIPanel*>(focusedBackgroundPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0.5f, 0.25f}, glm::vec2{250, 250}, glm::vec2{0, 0})));
                focusedItemPanel->SetAnchorPoint(glm::vec2{0.5f, 0.5f});
                focusedItemPanel->SetBorderColor(BLANK);
                focusedItemPanel->SetBorderWidth(2.0f);
                
                int index = 0;
                int row = 0;
                int column = 0;
                float curX = 25.f;
                float curY = 35.f;
                for (auto& item : inventory)
                {
                    auto* inventoryItemPanel = static_cast<UI::UIPanel*>(inventoryBackgroundPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{curX, curY}, glm::vec2{0.f, 0.f}, glm::vec2{64, 64}, glm::vec2{0, 0})));
                    inventoryItemPanel->SetBackgroundColor(BLANK);
                    inventoryItemPanel->SetBorderColor(BLANK);
                    inventoryItemPanel->SetBorderWidth(2.0f);
                    inventoryItemPanel->SetFocusable(true);
                    Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture;
                    const std::string suffix1 = " Note";
                    const std::string suffix2 = " Recipt";
                    // Check if string ends with " returns"
                    if ((item.length() >= suffix1.length() && 
                        item.substr(item.length() - suffix1.length()) == suffix1) ||
                        (item.length() >= suffix2.length() && 
                        item.substr(item.length() - suffix2.length()) == suffix2)
                    )
                    {
                        texture = resourceManager.GetTexture("assets/Tiles/Items/Recipt.png");
                    }
                    else
                    {
                        texture = resourceManager.GetTexture(std::format("assets/Tiles/Items/{}.png", item).c_str());
                    }
                    
                    inventoryItemPanel->SetBackgroundTexture(texture);

                    inventoryItemPanel->SetOnFocus([item, inventoryFocusedItemNameLabel, /*inventoryFocusedItemDescriptionLabel,*/ texture, focusedItemPanel](UI::UIElement* sender) {
                        inventoryFocusedItemNameLabel->SetText(item);
                        //inventoryFocusedItemDescriptionLabel->SetText(item);
                        focusedItemPanel->SetBackgroundTexture(texture);
                    });
                    focusNavigator->RegisterElement(inventoryItemPanel);
                    curX += 90.f;
                    
                    if (index == 0)
                    {
                        uiManager.SetFocus(inventoryItemPanel);
                    }
                    index++;
                    if (index % 4 == 0)
                    {
                        curX = 25.f;
                        curY += 90.f;
                    }
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
                focusNavigator->Clear();
                uiManager.RemoveElement(m_screenPanel);
            }

            std::string GetStateName() const override { return std::string(typeid(InventoryState).name()); }
        };
    }
}