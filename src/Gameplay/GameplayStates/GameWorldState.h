#pragma once

#include <cstdlib>
//#include <ctime>
#include <format>

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

#include "GamePlay/GameObjects/Player.h"
#include "GamePlay/GameObjects/Door.h"

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
            UI::UILabel* m_loopCountLabel;

            GameResource::StateManager m_stateManager;
            entt::entity m_worldEntity;

        public:
            GameWorldState() {}

            int GetLevelIndex(const FileLoading::LevelParser::World& worldMap, const std::string& levelName)
            {
                for (int i = 0; i < worldMap.levels.size(); i++)
                {
                    if (worldMap.levels[i].identifier == levelName)
                    {
                        return i;
                    }
                }
                return -1;
            }

            std::string GetNorthRoom(GameContext& context)
            {
                Inventory& inventory = context.GetInventory();
                if(std::find(inventory.begin(), inventory.end(), "Family Portrait") != inventory.end())
                {
                    return "Garden";
                }
                if(std::find(inventory.begin(), inventory.end(), "Broken Clockwork") != inventory.end())
                {
                    return "Armory";
                }
                return "Library";
            }

            std::string GetEastRoom(GameContext& context)
            {
                Inventory& inventory = context.GetInventory();
                if(std::find(inventory.begin(), inventory.end(), "Love Letter") != inventory.end())
                {
                    return "Kitchen";
                }
                if(std::find(inventory.begin(), inventory.end(), "Star Chart") != inventory.end())
                {
                    return "Lab";
                }
                return "Treasury";
            }

            std::string GetSouthRoom(GameContext& context)
            {
                Inventory& inventory = context.GetInventory();
                if(std::find(inventory.begin(), inventory.end(), "Recipe Box") != inventory.end())
                {
                    return "Bedroom";
                }
                if(std::find(inventory.begin(), inventory.end(), "Ritual Candle") != inventory.end())
                {
                    return "Study";
                }
                return "Vault";
            }

            std::string GetWestRoom(GameContext& context)
            {
                Inventory& inventory = context.GetInventory();
                if(std::find(inventory.begin(), inventory.end(), "Old Journal") != inventory.end())
                {
                    return "Workshop";
                }
                if(std::find(inventory.begin(), inventory.end(), "Research Notes") != inventory.end())
                {
                    return "Observatory";
                }
                return "Portal_Chamber";
            }

            std::string GetCourtyard(GameContext& context)
            {
                Inventory& inventory = context.GetInventory();
                if(std::find(inventory.begin(), inventory.end(), "Red Master Key") != inventory.end())
                {
                    if(std::find(inventory.begin(), inventory.end(), "Red Master Key") != inventory.end())
                    {
                        if(std::find(inventory.begin(), inventory.end(), "Red Master Key") != inventory.end())
                        {
                            if(std::find(inventory.begin(), inventory.end(), "Red Master Key") != inventory.end())
                            {
                                return "Courtyard_Complete";
                            }
                        }
                    }
                }
                return "Courtyard";
            }

            std::vector<int> CalculateRoomListToLoad(GameContext& context, entt::entity worldEntity)
            {
                entt::registry& registry = context.GetRegistry();
                auto* worldComponent = registry.try_get<Component::World>(worldEntity);
                if (!worldComponent)
                {
                    BREAK_MSG("Entity provided does not contain a World Component");
                    return std::vector<int>(5);
                }

                std::vector<int> roomList(5);
                // NorthRoom (0-2)
                int northRoom = GetLevelIndex(worldComponent->worldMap, GetNorthRoom(context));
                // EastRoom (3-5)
                int eastRoom = GetLevelIndex(worldComponent->worldMap, GetEastRoom(context));
                // SouthRoom (6-8)
                int southRoom = GetLevelIndex(worldComponent->worldMap, GetSouthRoom(context));
                // WestRoom (9-11)
                int westRoom = GetLevelIndex(worldComponent->worldMap, GetWestRoom(context));
                // Courtyard (12)
                int courtyard = GetLevelIndex(worldComponent->worldMap, GetCourtyard(context));
                return std::vector<int>{northRoom, eastRoom, westRoom, southRoom, courtyard};
            }

            void Enter(GameContext& context, GameResource::StateManager& stateManager) override
            {
                Core::GameData& gameData = context.GetGameData();
                gameData.Loops++; // increment the game loop count

                Core::Resource::ResourceManager& resourceManager = context.GetResourceManager();
                System::SystemManager& systemManager = context.GetSystemManager();
                System::GameObjectManager& gameObjectManger = context.GetGameObjectManager();

                Core::Resource::ResourcePtr<Core::Resource::FontResource> font = resourceManager.GetFontResource("assets/Fonts/machine-std/machine-std-regular.ttf_120");
                System::TransformSystem& transformSystem = systemManager.GetSystem<System::TransformSystem>();

                entt::entity worldEntity = GameResource::Level::CreateWorldEntity(context, WORLD_FILE_PATH);
                m_worldEntity = worldEntity;

                std::vector<int> roomList = CalculateRoomListToLoad(context, worldEntity);
                entt::entity northRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, roomList[0]);
                transformSystem.SetWorldTransform(context, northRoom, glm::vec3(576.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                entt::entity eastRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, roomList[1]);
                transformSystem.SetWorldTransform(context, eastRoom, glm::vec3(1152.0f, 576.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));                
                entt::entity westRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, roomList[2]);
                transformSystem.SetWorldTransform(context, westRoom, glm::vec3(0.0f, 576.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                entt::entity southRoom = GameResource::Level::LoadLevelEntities(context, worldEntity, roomList[3]);
                transformSystem.SetWorldTransform(context, southRoom, glm::vec3(576.0f, 1152.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                entt::entity courtyard = GameResource::Level::LoadLevelEntities(context, worldEntity, roomList[4]);
                transformSystem.SetWorldTransform(context, courtyard, glm::vec3(576.0f, 576.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

                entt::entity northRoomDupe = GameResource::Level::LoadLevelEntities(context, worldEntity, roomList[0]);
                transformSystem.SetWorldTransform(context, northRoomDupe, glm::vec3(576.0f, 1728.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

                entt::entity playerEntity = gameObjectManger.CreateGameObject(context, "Player", worldEntity);
                transformSystem.SetWorldTransform(context, playerEntity, glm::vec3(864.0f, 32.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                Player::Create(context, playerEntity);

                entt::entity lockedDoorEntity = gameObjectManger.CreateGameObject(context, "Door", worldEntity);
                transformSystem.SetWorldTransform(context, lockedDoorEntity, glm::vec3(864.0f, 0.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                Door::Create(context, lockedDoorEntity);

                
                UI::UIManager& uiManager = context.GetUIManager();
                UI::FocusNavigator* focusNavigator = uiManager.GetFocusNavigator();

                // Create the UI for the level.
                m_interactLabel = uiManager.CreateElement<UI::UILabel>(context, glm::vec2{0, 0}, glm::vec2{0, 0}, "Interact", 16.0f);
                m_interactLabel->SetVisible(false);
                //m_interactLabel->SetFont(font);
                m_interactLabel->SetTextColor(WHITE); // Change this when the background is created.
                m_interactLabel->SetAnchorPoint({ 0.5f,0.5f });

                m_loopCountLabel = uiManager.CreateElement<UI::UILabel>(context, glm::vec2{20, 20}, glm::vec2{0, 0}, std::format("Loops: {}", gameData.Loops).c_str(), 30.0f);
                //m_interactLabel->SetFont(font);
                m_loopCountLabel->SetTextColor(WHITE); // Change this when the background is created.
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

                    // check player at bottom of screen
                    auto& playerPosition = registry.get<Component::WorldTransform>(entity).position;
                    if (playerPosition.y > 1760.0f)
                    {
                        // reset the current state
                        std::unique_ptr<GamePlay::GameWorldState> gameWorldState = std::make_unique<GamePlay::GameWorldState>();
                        stateManager.ChangeState(context, std::move(gameWorldState));
                        return;
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
                uiManager.RemoveElement(m_loopCountLabel);
            }

            std::string GetStateName() const override { return std::string(typeid(GameWorldState).name()); }
        };
    }
}