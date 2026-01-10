#include "Level.h"

#include <format>

#include "Engine/GameContext.h"

#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/Component/WrenScript.h"

#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"

#include "Engine/FileLoading/LevelParser.h"
#include "Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.h"

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
	Resource::ResourceManager& resoruceManager = context.GetResourceManager();
	System::SystemManager& systemManager = context.GetSystemManager();
	auto& transformSystem = systemManager.GetSystem<System::TransformSystem>();
	auto& physicsSystem = systemManager.GetSystem<System::PhysicsSystem>();
	auto& animationSystem = systemManager.GetSystem<System::AnimationSystem>();

	auto* worldComponent = registry.try_get<Component::World>(worldEntity);
	if (!worldComponent)
	{
		BREAK_MSG("Entity provided does not contain a World Component");
		return entt::null;
	}

	FileLoading::LevelParser::Level levelToLoad = worldComponent->worldMap.levels[levelIndex];

	entt::entity levelEntity = gameObjectManager.CreateGameObject(context, levelToLoad.identifier, worldEntity);
	registry.emplace<Component::Level>(levelEntity, levelIndex, levelToLoad.Iid, levelToLoad.pxWid, levelToLoad.pxHei);
	transformSystem.SetWorldTransform(context, levelEntity, glm::vec3(levelToLoad.worldX, levelToLoad.worldY, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

	for (auto& layer : levelToLoad.layers) {
		const auto layerEntity = gameObjectManager.CreateGameObject(context, layer.identifier, levelEntity);
		switch (layer.type)
		{
		case FileLoading::LevelParser::LayerType::INT_GRID:
		case FileLoading::LevelParser::LayerType::AUTO_LAYER:
		{
			Resource::ResourcePtr<Resource::TextureResource> texture = resoruceManager.GetTexture("assets/Tiles/MemoryPalaceCollisionTiles.png");
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
				transformSystem.SetLocalTransform(context, layerInstaceEntity, glm::vec3(entityInstance.px.x + layer.gridSize / 2.f, entityInstance.px.y + layer.gridSize / 2.f, 0.0f), glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

				std::string identifier = entityInstance.identifier;

				// convert fieldInstances to wren map
				std::vector<Wren::WrenItem> wrenArgMap;
				for (auto field : entityInstance.fieldInstances)
				{
					Wren::WrenItem item{ field.identifier };
					switch (field.type)
					{
					case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
						item.type = WrenType::WREN_TYPE_NUM;
						item.value = static_cast<double>(std::any_cast<int>(field.value));
						break;
					case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
						item.type = WrenType::WREN_TYPE_NUM;
						item.value = static_cast<double>(std::any_cast<float>(field.value));
						break;
					case Struktur::FileLoading::LevelParser::FieldInstanceType::BOOLEAN:
						item.type = WrenType::WREN_TYPE_BOOL;
						item.value = field.value;
						break;
					case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
					case Struktur::FileLoading::LevelParser::FieldInstanceType::MULTILINE:
					case Struktur::FileLoading::LevelParser::FieldInstanceType::FILE_PATH:
					case Struktur::FileLoading::LevelParser::FieldInstanceType::ENUM:
						item.type = WrenType::WREN_TYPE_STRING;
						item.value = field.value;
						break;
					case Struktur::FileLoading::LevelParser::FieldInstanceType::COLOUR:
					{
						item.type = WrenType::WREN_TYPE_LIST;
						auto color = std::any_cast<Color>(field.value);
						// Create nested list [r, g, b, a]
						std::vector<Wren::WrenItem>	colorList;
						Wren::WrenItem r{ "r", static_cast<double>(color.r), WrenType::WREN_TYPE_NUM };
						colorList.emplace_back(r);
						Wren::WrenItem g{ "g", static_cast<double>(color.g), WrenType::WREN_TYPE_NUM };
						colorList.emplace_back(g);
						Wren::WrenItem b{ "b", static_cast<double>(color.b), WrenType::WREN_TYPE_NUM };
						colorList.emplace_back(b);
						Wren::WrenItem a{ "a", static_cast<double>(color.a), WrenType::WREN_TYPE_NUM };
						colorList.emplace_back(a);

						item.value = colorList;
						break;
					}
					case Struktur::FileLoading::LevelParser::FieldInstanceType::POINT:
					{
						item.type = WrenType::WREN_TYPE_LIST;
						auto point = std::any_cast<glm::vec2>(field.value);
						// Create nested list [x, y]
						std::vector<Wren::WrenItem>	colorList;
						Wren::WrenItem x{ "x", static_cast<double>(point.x), WrenType::WREN_TYPE_NUM };
						colorList.emplace_back(x);
						Wren::WrenItem y{ "y", static_cast<double>(point.y), WrenType::WREN_TYPE_NUM };
						colorList.emplace_back(y);

						item.value = colorList;
						break;
					}
					case Struktur::FileLoading::LevelParser::FieldInstanceType::TILE:
					case Struktur::FileLoading::LevelParser::FieldInstanceType::ENTITY_REF:
						item.type = WrenType::WREN_TYPE_STRING;
						item.value = field.value;
						break;
					default:
						item.type = WrenType::WREN_TYPE_NULL;
						item.value = 0;
						break;
					}
					wrenArgMap.emplace_back(item);
				}
				Component::WrenScript& scriptComponent = registry.emplace<Component::WrenScript>(layerInstaceEntity, identifier, wrenArgMap);
			}
			break;
		}
		default:
			break;
		}
	}

	return levelEntity;
}
