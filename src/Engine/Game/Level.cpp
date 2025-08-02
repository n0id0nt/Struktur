#include "Level.h"

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Level.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"

#include "Engine/FileLoading/LevelParser.h"
#include "Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.h"

#include "GamePlay/GameObjects/NPC.h"
#include "GamePlay/GameObjects/Item.h"

entt::entity Struktur::GameResource::Level::CreateWorldEntity(GameContext& context, const std::string& filePath)
{
    entt::registry& registry = context.GetRegistry();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();

    FileLoading::LevelParser::World worldMap = FileLoading::LevelParser::LoadWorldMap(context, filePath);

    std::string worldIdentifier = "World: " + filePath;
    entt::entity worldEntity = gameObjectManager.CreateGameObject(context, worldIdentifier);
    registry.emplace<Component::World>(worldEntity, worldMap);
    return worldEntity;
}

entt::entity Struktur::GameResource::Level::LoadLevelEntities(GameContext& context, const entt::entity worldEntity, int levelIndex)
{
    entt::registry& registry = context.GetRegistry();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
    Core::Resource::ResourceManager& resoruceManager = context.GetResourceManager();
    System::SystemManager& systemManager = context.GetSystemManager();
    auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
    auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();
    auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

    auto* worldComponent = registry.try_get<Component::World>(worldEntity);
    if (!worldComponent)
    {
        BREAK_MSG("Entity provided does not contain a World Component");
        return entt::entity();
    }

    FileLoading::LevelParser::Level levelToLoad = worldComponent->worldMap.levels[levelIndex];

    entt::entity levelEntity = gameObjectManager.CreateGameObject(context, levelToLoad.identifier);
    registry.emplace<Component::Level>(levelEntity, levelIndex, levelToLoad.Iid, levelToLoad.pxWid, levelToLoad.pxHei);
    transformSystem.SetWorldTransform(context, levelEntity, glm::vec3(levelToLoad.worldX, levelToLoad.worldY, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    for (auto& layer : levelToLoad.layers) {
        const auto layerEntity = gameObjectManager.CreateGameObject(context, layer.identifier, levelEntity);
        switch (layer.type)
        {
        case FileLoading::LevelParser::LayerType::INT_GRID:
        case FileLoading::LevelParser::LayerType::AUTO_LAYER:
        {
            Core::Resource::ResourcePtr<Core::Resource::TextureResource> texture = resoruceManager.GetTexture("assets/Tiles/cavesofgallet_tiles.png");
            transformSystem.SetLocalTransform(context, layerEntity, glm::vec3(layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            std::vector<TileMap::GridTile> grid;
            grid.reserve(layer.autoLayerTiles.size());
            for (auto& gridTile : layer.autoLayerTiles)
            {
                TileMap::GridTile newGridTile{ gridTile.px, gridTile.src, (TileMap::FlipBit)gridTile.f };
                grid.push_back(newGridTile);
            }

            // TODO - grab the tileset path from the level somehow - possibly have a store the tilesets in the resource pool and grab is here
            Component::TileMap& tileMap = registry.emplace<Component::TileMap>(layerEntity, std::move(texture), layer.cWid, layer.cHei, layer.gridSize, grid, layer.intGrid);

            if (layer.identifier == "Collision")
            {
                bool isSensor = false;
                b2BodyDef kinematicBodyDef;
                kinematicBodyDef.type = b2_dynamicBody;
                Component::PhysicsBody& physicsBody = physicsSystem.CreatePhysicsBody(context, layerEntity, kinematicBodyDef);
				physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
				physicsBody.syncToPhysics = true;     // Let transform drive physics
                Physics::TileMapCollisionBodyGenerator::CreateTileMapShape(context, tileMap, isSensor, physicsBody);
            }
            break;
        }
        case FileLoading::LevelParser::LayerType::ENTITIES:
        {
            for (auto& entityInstance : layer.entityInstaces)
            {
                const auto layerInstaceEntity = gameObjectManager.CreateGameObject(context, entityInstance.identifier, levelEntity);
                transformSystem.SetLocalTransform(context, layerInstaceEntity, glm::vec3(entityInstance.px.x + layer.gridSize/2.f, entityInstance.px.y + layer.gridSize / 2.f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

                if (entityInstance.identifier == "NPC")
                {
                    std::string name;
                    for (auto fieldInstance : entityInstance.fieldInstances)
                    {
                        switch (fieldInstance.type)
                        {
                        case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
                        {
                            name = std::any_cast<std::string>(fieldInstance.value);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    NPC::Create(context, layerInstaceEntity, name);
                }
                else if (entityInstance.identifier == "Item")
                {
                    std::string name;
                    for (auto fieldInstance : entityInstance.fieldInstances)
                    {
                        switch (fieldInstance.type)
                        {
                        case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
                        {
                            name = std::any_cast<std::string>(fieldInstance.value);
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    Item::Create(context, layerInstaceEntity, name, true);
                }

                //auto& luaComponent = registry.emplace<Struktur::Component::LuaComponent>(layerInstaceEntity, false, luaState.CreateTable());
                //for (auto fieldInstance : entityInstance.fieldInstances)
                //{
                //    switch (fieldInstance.type)
                //    {
                //    case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
                //    {
                //        float value = std::any_cast<float>(fieldInstance.value);
                //        luaComponent.table[fieldInstance.identifier] = value;
                //        break;
                //    }
                //    case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
                //    {
                //        int value = std::any_cast<int>(fieldInstance.value);
                //        luaComponent.table[fieldInstance.identifier] = value;
                //        break;
                //    }
                //    default:
                //        assert(false);
                //        break;
                //    }
                //}
            }
            break;
        }
        default:
            break;
        }
    }

    return levelEntity;
}
