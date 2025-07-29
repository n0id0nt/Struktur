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
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"

#include "Engine/UI/UILabel.h"
#include "Engine/UI/UIPanel.h"

constexpr static const char* TILE_TEXTURE = "assets/Tiles/cavesofgallet_tiles.png";
constexpr static const char* PLAYER_TEXTURE = "assets/Tiles/PlayerGrowthSprites.png";
constexpr static const char* WORLD_FILE_PATH = "assets/Levels/ExampleLDKTLevel.ldtk";

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
                entt::entity worldEntity = GameResource::Level::CreateWorldEntity(context, WORLD_FILE_PATH);
                GameResource::Level::LoadLevelEntities(context, worldEntity, 0);
                GameResource::Level::LoadLevelEntities(context, worldEntity, 1);
                
                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                // Create a main panel
                auto* mainPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{50, 50}, glm::vec2{0, 0}, glm::vec2{700, 500}, glm::vec2{0, 0});
                mainPanel->SetBackgroundColor(DARKGRAY);
                mainPanel->SetBorderColor(WHITE);
                mainPanel->SetBorderWidth(2.0f);
                
                // Create labels
                auto* titleLabel = uiManager.CreateElement<UI::UILabel>(glm::vec2{100, 80}, glm::vec2{0, 0}, "UI System Demo", 30.0f);
                titleLabel->SetTextColor(WHITE);
                titleLabel->SetAlignment(UI::TextAlignment::CENTER);
                titleLabel->SetFocusable(true);
                uiManager.SetFocus(titleLabel);
                focusNavigator->RegisterElement(titleLabel);

                auto* infoLabel = uiManager.CreateElement<UI::UILabel>(glm::vec2{100, 120}, glm::vec2{0, 0}, "Use Tab/Arrow keys or Controller to navigate", 16.0f);
                infoLabel->SetTextColor(LIGHTGRAY);
                
                // Create sub-panel
                auto* subPanel = uiManager.CreateElement<UI::UIPanel>(glm::vec2{100, 160}, glm::vec2{0, 0},  glm::vec2{600, 300}, glm::vec2{0, 0});
                subPanel->SetBackgroundColor(GRAY);
                subPanel->SetBorderColor(LIGHTGRAY);
                subPanel->SetBorderWidth(1.0f);

                // Add some content labels to the sub-panel
                auto* childLabel1 = subPanel->AddChild(std::make_unique<UI::UILabel>(glm::vec2{ 20, 40 }, glm::vec2{ 0, 0 }, "Content Area", 20.0f));
				childLabel1->SetFocusable(true);
				focusNavigator->RegisterElement(childLabel1);
                auto* childLabel2 = subPanel->AddChild(std::make_unique<UI::UILabel>(glm::vec2{ 0, 0 }, glm::vec2{ 0.5f,0.5f }, "This is where your UI content would go.", 14.0f));
				childLabel2->SetFocusable(true);
				childLabel2->SetAnchorPoint({ 0.5f,0.5f });
				focusNavigator->RegisterElement(childLabel2);
            }

            void Update(GameContext& context) override 
            {
                Core::Input& input = context.GetInput();
                Core::Resource::ResourceManager& resoruceManager = context.GetResourceManager();
                System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
                System::SystemManager& systemManager = context.GetSystemManager();
                auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();

                // player movement system
                Core::GameData& gameData = context.GetGameData();
                entt::registry& registry = context.GetRegistry();
                glm::vec2 inputDir = input.GetInputAxis2("Move");
                bool inputAddObject = input.IsInputJustPressed("AddObject");
                bool inputAddChild = input.IsInputJustPressed("AddChild");
                bool inputDeleteObject = input.IsInputJustPressed("DeleteObject");
                auto view = registry.view<Component::LocalTransform, Component::Player, Component::PhysicsBody>();
                for (auto [entity, transform, player, physicsBody] : view.each())
                {
                    b2Vec2 velecity = b2Vec2(inputDir.x *  player.speed, inputDir.y * -player.speed);
                    physicsBody.body->SetLinearVelocity(velecity);
                    if (inputAddObject)
                    {
                        //std::srand(std::time({}));
                        auto child = gameObjectManager.CreateGameObject(context, "Child", entity);
                        Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture = resoruceManager.GetTexture("assets/Tiles/cavesofgallet_tiles.png");
                        registry.emplace<Component::Sprite>(child, std::move(texture), PINK, glm::vec2(8, 8), 20, 20, false, 10);
                        transformSystem.SetLocalTransform(context, child, glm::vec3((float)(std::rand() % 200) - 100.0f, (float)(std::rand() % 200) - 100.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                        DEBUG_INFO("Add game object");
                    }
                    if (inputAddChild)
                    {
                        Component::Children* children = registry.try_get<Component::Children>(entity);
                        if (children)
                        {
                            if (!children->entities.empty())
                            {
                                entt::entity parent = children->entities[std::rand() % children->entities.size()];
                                //std::srand(std::time({}));
                                auto child = gameObjectManager.CreateGameObject(context, "Child of child", parent);
								Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture = resoruceManager.GetTexture("assets/Tiles/cavesofgallet_tiles.png");
                                registry.emplace<Component::Sprite>(child, std::move(texture), PURPLE, glm::vec2(8, 8), 20, 20, false, 11);
                                transformSystem.SetLocalTransform(context, child, glm::vec3((float)(std::rand() % 200) - 100.0f, (float)(std::rand() % 200) - 100.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                                DEBUG_INFO("Add child game object");

                                System::SystemManager& systemManager = context.GetSystemManager();
                                System::PhysicsSystem& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();

                                b2BodyDef kinematicBodyDef;
                                kinematicBodyDef.type = b2_dynamicBody;
                                b2PolygonShape childShape;
                                childShape.SetAsBox(1 / 2.0f, 1 / 2.0f);
                                physicsSystem.CreatePhysicsBody(context, child, kinematicBodyDef, childShape);
                            }
                        }
                    }
                    if (inputDeleteObject)
                    {
                        Component::Children* children = registry.try_get<Component::Children>(entity);
                        if (children)
                        {
                            if (!children->entities.empty())
                            {
                                gameObjectManager.DestroyGameObject(context, children->entities[0]);
                                DEBUG_INFO("Delete game object");
                            }
                        }
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