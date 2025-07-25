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
                FileLoading::LevelParser::World world = FileLoading::LevelParser::LoadWorldMap(context, WORLD_FILE_PATH); // need to sore the world somewhere to be refered to later. - probably Have a world entity as entities store states
                FileLoading::LevelParser::Level& firstLevel = world.levels[0]; // should probably actually store the first level somewhere so it can more dynamically be fetched
                GameResource::Level::LoadLevelEntities(context, firstLevel);
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