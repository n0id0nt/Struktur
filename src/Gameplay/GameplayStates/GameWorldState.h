#pragma once

#include <cstdlib>
//#include <ctime>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp."
#include "entt/entt.hpp"

#include "Engine/GameContext.h"

#include "Engine/Game/State.h"

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

constexpr static const char* TILE_TEXTURE = "assets/Tiles/cavesofgallet_tiles.png";
constexpr static const char* PLAYER_TEXTURE = "assets/Tiles/PlayerGrowthSprites.png";
constexpr static const char* WORLD_FILE_PATH = "assets/Levels/MemoryPalace.ldtk";

namespace Struktur
{    
	namespace GamePlay
	{
		class GameWorldState : public GameResource::IState
		{
        public:
            GameWorldState() {}

            void Enter(GameContext& context) override 
            {
                Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/machine-std/machine-std-regular.ttf_120");

                entt::entity worldEntity = GameResource::Level::CreateWorldEntity(context, WORLD_FILE_PATH);
                entt::entity northRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 0);
                entt::entity courtyard = GameResource::Level::LoadLevelEntities(context, worldEntity, 1);
                entt::entity EastRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 2);
                entt::entity WestRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 3);
                entt::entity SouthRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, 4);

                /*
                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();
                
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

                auto* infoLabel = uiManager.CreateElement<UI::UILabel>(context, glm::vec2{100, 120}, glm::vec2{0, 0}, "Use Tab/Arrow keys or Controller to navigate", 16.0f);
                infoLabel->SetFont(font);
                infoLabel->SetTextColor(LIGHTGRAY);
                
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

            void Update(GameContext& context) override 
            {
                Core::Input& input = context.GetInput();
                Core::Resource::ResourceManager& resoruceManager = context.GetResourceManager();
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                System::SystemManager& systemManager = context.GetSystemManager();
                auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
                auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

                // player movement system
                Core::GameData& gameData = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                glm::vec2 inputDir = input.GetInputAxis2("Move");
                bool interact = input.IsKeyJustPressed("Interact");

                auto view = registry.view<Component::Player>();
                for (auto& entity : view)
                {
                    Struktur::Player::PlayerControl(context, entity, inputDir);

                    entt::entity canInteract = Struktur::Player::CanInteract(context, entity);
                    // render interact UI toolTip

                    if (interact && canInteract != entt::null)
                    {
                        Struktur::Player::Interact(context, entity, canInteract);
                    }
                }
            }
            void Render(GameContext& context) override {}
            void Exit(GameContext& context) override 
            {
                // delete all players
            }

            std::string GetStateName() const override { return std::string(typeid(GameWorldState).name()); }
        };
    }
}