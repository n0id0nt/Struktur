#include "Level.h"

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Player.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"
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
    System::AnimationSystem& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

    const auto levelEntity = gameObjectManager.CreateGameObject(context, level.identifier);

    for (auto& layer : level.layers) {
        const auto layerEntity = gameObjectManager.CreateGameObject(context, layer.identifier, levelEntity);
        switch (layer.type)
        {
        case FileLoading::LevelParser::LayerType::INT_GRID:
        case FileLoading::LevelParser::LayerType::AUTO_LAYER:
        {
            transformSystem.SetWorldTransform(context, layerEntity, glm::vec3(layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            //registry.emplace<Struktur::Component::Level>(layerEntity, level.Iid);
            std::vector<TileMap::GridTile> grid;
            grid.reserve(layer.autoLayerTiles.size());
            for (auto& gridTile : layer.autoLayerTiles)
            {
                TileMap::GridTile newGridTile{ gridTile.px, gridTile.src, (TileMap::FlipBit)gridTile.f };
                grid.push_back(newGridTile);
            }

            // TODO - grab the tileset path from the level somehow - possibly have a store the tilesets in the resource pool and grab is here
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
                const auto layerInstaceEntity = gameObjectManager.CreateGameObject(context, entityInstance.identifier, levelEntity);
                transformSystem.SetWorldTransform(context, layerInstaceEntity, glm::vec3(entityInstance.px.x, entityInstance.px.y, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

                // All this is specific to the player and should be brought to a separate function
                registry.emplace<Component::Sprite>(layerInstaceEntity, "assets/Tiles/PlayerGrowthSprites.png", WHITE, glm::vec2(16, 16), 12, 5, false, 0);
				registry.emplace<Component::Player>(layerInstaceEntity, 10.f);
                Component::Camera& parentCamera = registry.emplace<Component::Camera>(layerInstaceEntity);
				parentCamera.zoom = 2.f;
				parentCamera.forcePosition = true;
				parentCamera.damping = glm::vec2(0.8f, 0.8f);
				transformSystem.SetLocalTransform(context, layerInstaceEntity, glm::vec3(500.0f, 300.0f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
				b2BodyDef kinematicBodyDef;
				kinematicBodyDef.type = b2_dynamicBody;
				b2PolygonShape playerShape;
				playerShape.SetAsBox(1 / 2.0f, 1 / 2.0f);
				physicsSystem.CreatePhysicsBody(context, layerInstaceEntity, kinematicBodyDef, playerShape);
                Component::PhysicsBody& physicsBody = registry.get<Component::PhysicsBody>(layerInstaceEntity);
                physicsBody.syncFromPhysics = true;  // Don't let physics drive transform
                physicsBody.syncToPhysics = true;     // Let transform drive physics
				Component::SpriteAnimation& spriteAnimation = registry.emplace<Component::SpriteAnimation>(layerInstaceEntity);
                // animation could possibly be a resource stored in the resource pool and loaded in from a file.
                Animation::SpriteAnimation idle32Animation{ 24u, 28u, 1.f, true };
                Animation::SpriteAnimation run32Animation{ 28u, 33u, 0.7f, true };
                Animation::SpriteAnimation jump32Animation{ 33u, 35u, 0.2f, false };
                Animation::SpriteAnimation fall32Animation{ 35u, 36u, 1.f, false };

                animationSystem.AddAnimation(context, layerInstaceEntity, "idle32", idle32Animation);
                animationSystem.AddAnimation(context, layerInstaceEntity, "run32", run32Animation);
                animationSystem.AddAnimation(context, layerInstaceEntity, "jump32", jump32Animation);
                animationSystem.AddAnimation(context, layerInstaceEntity, "fall32", fall32Animation);
                animationSystem.PlayAnimation(context, layerInstaceEntity, "idle32");

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
}
