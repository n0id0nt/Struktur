#include "Level.h"

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Sprite.h"
//#include "Engine/ECS/Component/Level.h"

void Struktur::GameResource::Level::LoadLevelEntities(GameContext& context, const Struktur::FileLoading::LevelParser::Level& level)
{
    //const auto levelEntity = registry.create();
    entt::registry& registry = context.GetRegistry();
    System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();

    for (auto& layer : level.layers) {
        switch (layer.type)
        {
        case Struktur::FileLoading::LevelParser::LayerType::INT_GRID:
        case Struktur::FileLoading::LevelParser::LayerType::AUTO_LAYER:
        {
            const auto layerEntity = gameObjectManager.CreateGameObject(context);
            auto& layerTransform = registry.get<Component::Transform>(layerEntity);
            layerTransform.position = {layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f};
            //registry.emplace<Struktur::Component::Level>(layerEntity, level.Iid);
            std::vector<Struktur::GameResource::TileMap::GridTile> grid;
            grid.reserve(layer.autoLayerTiles.size());
            for (auto& gridTile : layer.autoLayerTiles)
            {
                Struktur::GameResource::TileMap::GridTile newGridTile{ gridTile.px, gridTile.src, (Struktur::GameResource::TileMap::FlipBit)gridTile.f };
                grid.push_back(newGridTile);
            }
            registry.emplace<Struktur::Component::TileMap>(layerEntity, "assets/Tiles/cavesofgallet_tiles.png", layer.cWid, layer.cHei, layer.gridSize, grid, layer.intGrid);
            break;
        }
        case Struktur::FileLoading::LevelParser::LayerType::ENTITIES:
        {
            for (auto& entityInstance : layer.entityInstaces)
            {
                const auto layerEntity = gameObjectManager.CreateGameObject(context);
                auto& layerTransform = registry.get<Component::Transform>(layerEntity);
                layerTransform.position = {layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f};
                registry.emplace<Struktur::Component::Identifier>(layerEntity, entityInstance.identifier);
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
