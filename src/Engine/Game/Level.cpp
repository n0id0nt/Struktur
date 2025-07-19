#include "Level.h"

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
//#include "Engine/ECS/Component/Level.h"
#include "Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.h"

void Struktur::GameResource::Level::LoadLevelEntities(GameContext& context, const FileLoading::LevelParser::Level& level)
{
    //const auto levelEntity = registry.create();
    entt::registry& registry = context.GetRegistry();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
    System::SystemManager& systemManager = context.GetSystemManager();
    System::TransformSystem& transformSystem = systemManager.GetSystem<System::TransformSystem>();
    System::PhysicsSystem& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();

    for (auto& layer : level.layers) {
        switch (layer.type)
        {
        case FileLoading::LevelParser::LayerType::INT_GRID:
        case FileLoading::LevelParser::LayerType::AUTO_LAYER:
        {
            const auto layerEntity = gameObjectManager.CreateGameObject(context);
            transformSystem.SetWorldTransform(context, layerEntity, glm::vec3(layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            //registry.emplace<Struktur::Component::Level>(layerEntity, level.Iid);
            std::vector<TileMap::GridTile> grid;
            grid.reserve(layer.autoLayerTiles.size());
            for (auto& gridTile : layer.autoLayerTiles)
            {
                TileMap::GridTile newGridTile{ gridTile.px, gridTile.src, (TileMap::FlipBit)gridTile.f };
                grid.push_back(newGridTile);
            }
            Component::TileMap& tileMap = registry.emplace<Component::TileMap>(layerEntity, "assets/Tiles/cavesofgallet_tiles.png", layer.cWid, layer.cHei, layer.gridSize, grid, layer.intGrid);

            if (layer.identifier == "Collision")
            {
                bool isSensor = false;
                b2BodyDef kinematicBodyDef;
                kinematicBodyDef.type = b2_dynamicBody;
                Component::PhysicsBody& physicsBody = physicsSystem.CreatePhysicsBody(context, layerEntity, kinematicBodyDef);
                Physics::TileMapCollisionBodyGenerator::CreateTileMapShape(context, tileMap, isSensor, physicsBody);
            }
            break;
        }
        case FileLoading::LevelParser::LayerType::ENTITIES:
        {
            for (auto& entityInstance : layer.entityInstaces)
            {
                const auto layerEntity = gameObjectManager.CreateGameObject(context);
                transformSystem.SetWorldTransform(context, layerEntity, glm::vec3(layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                registry.emplace<Component::Identifier>(layerEntity, entityInstance.identifier);
                registry.emplace<Component::Sprite>(layerEntity, "assets/Tiles/PlayerGrowthSprite.png", WHITE, glm::vec2(16, 16), 12, 5, false, 0);
                //auto& luaComponent = registry.emplace<Struktur::Component::LuaComponent>(layerEntity, false, luaState.CreateTable());
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
}
