#include "Level.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Level.h"

void Struktur::Game::Level::LoadLevelEntities(GameContext& context, const Struktur::FileLoading::LevelParser::Level& level)
{
    //const auto levelEntity = registry.create();


    for (auto& layer : level.layers) {
        switch (layer.type)
        {
        case Struktur::FileLoading::LevelParser::LayerType::INT_GRID:
        case Struktur::FileLoading::LevelParser::LayerType::AUTO_LAYER:
        {
            const auto layerEntity = registry.create();
            registry.emplace<Struktur::Component::LevelComponent>(layerEntity, level.Iid);
            auto& transform = registry.emplace<Struktur::Component::TransformComponent>(layerEntity, layerEntity);
            transform.SetPosition2(Vector2(layer.pxTotalOffsetX, layer.pxTotalOffsetY));
            std::vector<Struktur::Game::TileMap::GridTile> grid;
            grid.reserve(layer.autoLayerTiles.size());
            for (auto& gridTile : layer.autoLayerTiles)
            {
                Struktur::Game::TileMap::GridTile newGridTile{ gridTile.px, gridTile.src, (Struktur::Game::TileMap::FlipBit)gridTile.f };
                grid.push_back(newGridTile);
            }
            registry.emplace<Struktur::Component::TileMapComponent>(layerEntity, "../ExampleGame/Tiles/cavesofgallet_tiles.png", layer.cWid, layer.cHei, layer.gridSize, grid, layer.intGrid);
            break;
        }
        case Struktur::FileLoading::LevelParser::LayerType::ENTITIES:
        {
            for (auto& entityInstance : layer.entityInstaces)
            {
                const auto layerEntity = registry.create();
                registry.emplace<Struktur::Component::LevelComponent>(layerEntity, level.Iid);
                Vector2 position = entityInstance.px;
                auto& transform = registry.emplace<Struktur::Component::TransformComponent>(layerEntity, layerEntity);
                transform.SetPosition2(Vector2(position.x, position.y));
                registry.emplace<Struktur::Component::IdentifierComponent>(layerEntity, entityInstance.identifier);
                auto& luaComponent = registry.emplace<Struktur::Component::LuaComponent>(layerEntity, false, luaState.CreateTable());
                for (auto fieldInstance : entityInstance.fieldInstances)
                {
                    switch (fieldInstance.type)
                    {
                    case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
                    {
                        float value = std::any_cast<float>(fieldInstance.value);
                        luaComponent.table[fieldInstance.identifier] = value;
                        break;
                    }
                    case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
                    {
                        int value = std::any_cast<int>(fieldInstance.value);
                        luaComponent.table[fieldInstance.identifier] = value;
                        break;
                    }
                    default:
                        assert(false);
                        break;
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
}
