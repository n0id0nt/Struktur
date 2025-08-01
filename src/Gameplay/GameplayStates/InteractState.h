#pragma once

#include <cstdlib>
#include <algorithm>
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

constexpr static float TEXT_SCROLL_SPEED = 0.02f;

namespace Struktur
{    
	namespace GamePlay
	{
        struct DialogueStep
        {
            std::string dialogue;
            std::vector<std::string> itemAddVector; // Add this to the players inventory
            std::vector<std::string> itemRemoveVector; // Add this to the players inventory
            bool applyCameraShake;
            bool deleteInteractingEntity;
            bool animateText;
        };

        struct Interaction
        {
            std::vector<DialogueStep> dialougeVector;
        };

        static std::unordered_map<std::string, Interaction> s_interactionMap = {
            {"Scholar", {{
                {"This is the First dialogue step of the scholar", {}, {}, false, false, true},
                {"Camera Shake", {}, {}, true, false, false},
                {"This is the Third dialogue step of the scholar", {"Scholar's Note"}, {}, false, false, true},
            }}},
            {"Book", {{
                {"This is the First dialogue step of the book", {}, {}, false, false, true},
                {"Player Obtained Book", {"Book"}, {}, false, true, false},
            }}},
        };

		class InteractState : public GameResource::IState
		{
        private:
            UI::UIPanel* m_screenPanel;
            UI::UILabel* m_dialogueLabel;
            UI::UILabel* m_continueDialogueLabel;

            entt::entity m_interactingEntity = entt::null;

            Interaction m_interaction;

            bool m_dialogueSrolling = false;
            int m_interactionStep = 0;
            std::string m_currentString;

            double m_currentDialogueStartTime = 0.0;

        public:
            InteractState(entt::entity interactingEntity) : m_interactingEntity(interactingEntity) {}

            void Enter(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // Create Call the interact function and get the player to interact with

                Core::GameData& gameDate = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/machine-std/machine-std-regular.ttf_120");

                // UI layout
                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                m_screenPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{0, 0}, glm::vec2{0, 0}, glm::vec2{gameDate.screenWidth, gameDate.screenHeight}, glm::vec2{0, 0});
                m_screenPanel->SetBackgroundColor(BLANK); // don't render this
                m_screenPanel->SetBorderColor(BLANK);

                auto* textBackgroundPanel = static_cast<UI::UIPanel*>(m_screenPanel->AddChild(std::make_unique<UI::UIPanel>(glm::vec2{0, -50}, glm::vec2{0.5f, 1.0f}, glm::vec2{700, 150}, glm::vec2{0, 0})));
                textBackgroundPanel->SetAnchorPoint(glm::vec2{0.5f, 1.0f});
                textBackgroundPanel->SetBackgroundColor(DARKGRAY);
                textBackgroundPanel->SetBorderColor(WHITE);
                textBackgroundPanel->SetBorderWidth(2.0f);

                m_dialogueLabel = static_cast<UI::UILabel*>(textBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 20, 40 }, glm::vec2{ 0, 0 }, "", 20.0f)));
                m_dialogueLabel->SetTextColor(BLACK);
                m_continueDialogueLabel = static_cast<UI::UILabel*>(textBackgroundPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ -20, -20 }, glm::vec2{ 1, 1 }, "Continue", 20.0f)));
                m_continueDialogueLabel->SetAnchorPoint(glm::vec2{1.0f, 1.0f});
                m_continueDialogueLabel->SetTextColor(BLACK);
                m_continueDialogueLabel->SetVisible(false);

                // Set State variables
                Component::Interactable interactable = registry.get<Component::Interactable>(m_interactingEntity);

                m_interaction = s_interactionMap[interactable.interactionId];
                SetInteractionStep(context, 0);
            }

            void Update(GameContext& context, GameResource::StateManager& stateManager) override
            {
                Core::GameData& gameDate = context.GetGameData();
                Core::Input& input = context.GetInput();

                double gameTime = gameDate.gameTime;

                bool inputInteract = input.IsInputJustReleased("Interact");
                    
                if (inputInteract)
                {
                    if (m_dialogueSrolling)
                    {
                        m_dialogueSrolling = false;
                        m_continueDialogueLabel->SetVisible(true);
                    }
                    else if (m_interaction.dialougeVector.size() > m_interactionStep + 1)
                    {
                        SetInteractionStep(context, m_interactionStep + 1);
                        m_continueDialogueLabel->SetVisible(false);
                    }
                    else
                    {
                        stateManager.ChangeState(context, std::unique_ptr<GameResource::IState>(nullptr));
                        return; // we are finished with the interaction.
                    }
                }

                int numberOfCharactersToDraw = m_dialogueSrolling ? (int)std::floor((gameTime - m_currentDialogueStartTime) / TEXT_SCROLL_SPEED) : m_currentString.length();
                if (numberOfCharactersToDraw >= m_currentString.length())
                {
                    numberOfCharactersToDraw = m_currentString.length();
                    m_dialogueSrolling = false;
                    m_continueDialogueLabel->SetVisible(true);
                }
                std::string subString = m_currentString.substr(0, numberOfCharactersToDraw);
                m_dialogueLabel->SetText(subString);
                // animate the text scrolling, Interact with items, add items to inventory, delete items from scene
            }
            void Render(GameContext& context, GameResource::StateManager& stateManager) override {}
            void Exit(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // delete all players
                // delete all UI
				UI::UIManager& uiManager = context.GetUIManager();
                uiManager.RemoveElement(m_screenPanel);
            }

            std::string GetStateName() const override { return std::string(typeid(InteractState).name()); }

            void SetInteractionStep(GameContext& context, int stepIndex)
            {
                Core::GameData& gameDate = context.GetGameData();
                Inventory& inventory = context.GetInventory();
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                DialogueStep step = m_interaction.dialougeVector[stepIndex];
                m_currentString = step.dialogue;
                m_dialogueSrolling = step.animateText;
                m_interactionStep = stepIndex;
                m_currentDialogueStartTime = gameDate.gameTime;

                for (auto& item : step.itemAddVector)
                {
                    if(std::find(inventory.begin(), inventory.end(), item) == inventory.end())
                    {
                        inventory.push_back(item);
                    }
                }

                for (auto& item : step.itemRemoveVector)
                {
                    inventory.erase(std::find(inventory.begin(), inventory.end(), item));
                }

                if (step.deleteInteractingEntity)
                {
                    gameObjectManager.DestroyGameObject(context, m_interactingEntity);
                }

                if (step.applyCameraShake)
                {
                    entt::registry& registry = context.GetRegistry();
                    System::SystemManager& systemManager = context.GetSystemManager();
                    System::CameraSystem& cameraSystem = systemManager.GetSystem<System::CameraSystem>();

                    auto view = registry.view<Component::Camera>();
                    for (auto [entity, cameraComponent] : view.each())
                    {
                        cameraSystem.AddCameraTrauma(context, entity, 0.4f);
                    }
                }
            }
        };
    }
}
