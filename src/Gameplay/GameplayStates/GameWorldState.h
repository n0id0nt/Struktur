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
#include "Gameplay/GameplayStates/InteractState.h"
#include "Gameplay/GameplayStates/InventoryState.h"

constexpr static const char* TILE_TEXTURE = "assets/Tiles/cavesofgallet_tiles.png";
constexpr static const char* PLAYER_TEXTURE = "assets/Tiles/PlayerGrowthSprites.png";
constexpr static const char* WORLD_FILE_PATH = "assets/Levels/MemoryPalace.ldtk";

namespace Struktur
{    
	namespace GamePlay
	{
		class GameWorldState : public GameResource::IState
		{
        private:
            UI::UILabel* m_interactLabel;

            GameResource::StateManager m_stateManager;
            entt::entity m_worldEntity;

        public:
            GameWorldState() {}

            void Enter(GameContext& context, GameResource::StateManager& stateManager) override
            {
                Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/machine-std/machine-std-regular.ttf_120");

                entt::entity worldEntity = GameResource::Level::CreateWorldEntity(context, WORLD_FILE_PATH);
                m_worldEntity = worldEntity;
                entt::entity northRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 0);
                entt::entity courtyard = GameResource::Level::LoadLevelEntities(context, worldEntity, 1);
                entt::entity EastRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 2);
                entt::entity WestRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 3);
                entt::entity SouthRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 4);

                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                // Create the UI for the level.
                m_interactLabel = uiManager.CreateElement<UI::UILabel>(context, glm::vec2{0, 0}, glm::vec2{0, 0}, "Interact", 16.0f);
                m_interactLabel->SetVisible(false);
                //m_interactLabel->SetFont(font);
                m_interactLabel->SetTextColor(WHITE); // Change this when the background is created.
                m_interactLabel->SetAnchorPoint({ 0.5f,0.5f });
                // could have same text behind slighly shifted for border effect
                /*
                
                // Create a main panel
                auto* mainPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{50, 50}, glm::vec2{0, 0}, glm::vec2{700, 500}, glm::vec2{0, 0});
                mainPanel->SetBackgroundColor(DARKGRAY);
                mainPanel->SetBorderColor(WHITE);
                mainPanel->SetBorderWidth(2.0f);
                
                
                // Create labels
                auto* titleLabel = uiManager.CreateElement<UI::UILabel>(context, glm::vec2{100, 80}, glm::vec2{0, 0}, "UI System Demo", 30.0f);
                titleLabel->SetTextColor(WHITE);
                titleLabel->SetAlignment(UI::TextAlignment::CENTER);
                titleLabel->SetFont(font);
                titleLabel->SetFocusable(true);
                uiManager.SetFocus(titleLabel);
                focusNavigator->RegisterElement(titleLabel);

                
                // Create sub-panel
                auto* subPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{100, 160}, glm::vec2{0, 0},  glm::vec2{600, 300}, glm::vec2{0, 0});
                subPanel->SetBackgroundColor(GRAY);
                subPanel->SetBorderColor(LIGHTGRAY);
                subPanel->SetBorderWidth(1.0f);

                // Add some content labels to the sub-panel
                auto* childLabel1 = static_cast<UI::UILabel*>(subPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 20, 40 }, glm::vec2{ 0, 0 }, "Content Area", 20.0f)));
                childLabel1->SetFont(font);
                childLabel1->SetFocusable(true);
                focusNavigator->RegisterElement(childLabel1);
                auto* childLabel2 = static_cast<UI::UILabel*>(subPanel->AddChild(std::make_unique<UI::UILabel>(context, glm::vec2{ 0, 0 }, glm::vec2{ 0.5f,0.5f }, "This is where your UI content would go.", 14.0f)));
                childLabel2->SetFont(font);
                childLabel2->SetFocusable(true);
                childLabel2->SetAnchorPoint({ 0.5f,0.5f });
                focusNavigator->RegisterElement(childLabel2);*/
            }

            void Update(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // if substate return out here
                if (auto* currentState = m_stateManager.GetCurrentState())
                {
                    m_stateManager.Update(context);
                    return;
                }

                Core::Input& input = context.GetInput();
                Core::Resource::ResourceManager& resoruceManager = context.GetResourceManager();
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                GameResource::Camera& camera = context.GetCamera();
                System::SystemManager& systemManager = context.GetSystemManager();
                auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
                auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

                // player movement system
                Core::GameData& gameData = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                glm::vec2 inputDir = input.GetInputAxis2("Move");
                bool inputInteract = input.IsInputJustReleased("Interact");
                bool inventoryInteract = input.IsInputJustReleased("Inventory");

                auto view = registry.view<Component::Player>();
                if (inventoryInteract)
                {
                    m_interactLabel->SetVisible(false);
                    //TODO also pause the game time to pause the players animation
                    // just forcing player to idle for now
                    for (auto& entity : view)
                    {
                        Struktur::Player::PlayerForceStop(context, entity);
                    }
                    std::unique_ptr<InventoryState> inventoryState = std::make_unique<InventoryState>();
                    m_stateManager.ChangeState(context, std::move(inventoryState));
                    return;
                }

                for (auto& entity : view)
                {
                    Struktur::Player::PlayerControl(context, entity, inputDir);

                    entt::entity canInteract = Struktur::Player::CanInteract(context, entity);

                    if (canInteract != entt::null)
                    {
                        auto& interactWorldTransform = registry.get<Component::WorldTransform>(canInteract);
                        m_interactLabel->SetVisible(true);
                        
                        glm::vec2 screenInteractPosition = camera.WorldPosToScreenPos(interactWorldTransform.position) + glm::vec2{ 0,-32 };
                        m_interactLabel->SetPosition(screenInteractPosition, glm::vec2{0,0});
                        if (inputInteract)
                        {
                            m_interactLabel->SetVisible(false);
                            Struktur::Player::PlayerForceStop(context, entity);
                            // Change state to interact state
                            std::unique_ptr<InteractState> interactState = std::make_unique<InteractState>(canInteract);
                            m_stateManager.ChangeState(context, std::move(interactState));
                            return;
                        }
                    }
                    else
                    {
                        m_interactLabel->SetVisible(false);
                    }
                }
            }
            void Render(GameContext& context, GameResource::StateManager& stateManager) override {}
            void Exit(GameContext& context, GameResource::StateManager& stateManager) override
            {
                // delete all players
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                gameObjectManager.DestroyGameObject(context, m_worldEntity);
                // delete all UI
                UI::UIManager& uiManager = context.GetUIManager();
                uiManager.RemoveElement(m_interactLabel);
            }

            std::string GetStateName() const override { return std::string(typeid(GameWorldState).name()); }
        };
    }
}