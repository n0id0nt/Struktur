#include "Level.h"

#include <format>
#include <sstream>

#include "Engine/ECS/Component/Camera.h"
#include "Engine/ECS/Component/Identifier.h"
#include "Engine/ECS/Component/Level.h"
#include "Engine/ECS/Component/LevelEntityInstance.h"
#include "Engine/ECS/Component/PhysicsBody.h"
#include "Engine/ECS/Component/Sprite.h"
#include "Engine/ECS/Component/SpriteAnimation.h"
#include "Engine/ECS/Component/TileMap.h"
#include "Engine/ECS/Component/Transform.h"
#include "Engine/ECS/Component/World.h"
#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/System/AnimationSystem.h"
#include "Engine/ECS/System/PhysicsSystem.h"
#include "Engine/ECS/System/TransformSystem.h"
#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/FileLoading/LevelParser.h"
#include "Engine/GameContext.h"
#include "Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.h"
#include "Engine/Util/Color.h"

entt::entity Struktur::World::Level::CreateWorldEntity(GameContext& context, const std::string& filePath)
{
	entt::registry& registry                     = context.GetRegistry();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();

	FileLoading::LevelParser::World worldMap = FileLoading::LevelParser::LoadWorldMap(context, filePath);

	std::string worldIdentifier = "World: " + filePath;
	entt::entity worldEntity    = gameObjectManager.CreateGameObject(context, worldIdentifier);
	registry.emplace<Component::World>(worldEntity, worldMap);
	return worldEntity;
}

entt::entity Struktur::World::Level::LoadLevelEntities(GameContext& context, const entt::entity worldEntity,
                                                       int levelIndex, const LevelLoadOptions& options)
{
	entt::registry& registry                     = context.GetRegistry();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();
	Resource::ResourceManager& resoruceManager   = context.GetResourceManager();
	System::SystemManager& systemManager         = context.GetSystemManager();
	auto& transformSystem                        = systemManager.GetSystem<System::TransformSystem>();
	auto& physicsSystem                          = systemManager.GetSystem<System::PhysicsSystem>();
	auto& animationSystem                        = systemManager.GetSystem<System::AnimationSystem>();
	auto& wrenScriptSystem                       = systemManager.GetSystem<System::WrenScriptSystem>();

	auto* worldComponent = registry.try_get<Component::World>(worldEntity);
	if (!worldComponent)
	{
		BREAK_MSG("Entity provided does not contain a World Component");
		return entt::null;
	}

	// Loading an already-loaded level is a no-op that returns the existing entity, so adjacency-driven
	// callers can call this repeatedly without tracking load state themselves.
	auto existingIt = worldComponent->loadedLevels.find(levelIndex);
	if (existingIt != worldComponent->loadedLevels.end())
	{
		return existingIt->second;
	}

	FileLoading::LevelParser::Level levelToLoad = worldComponent->worldMap.levels[levelIndex];

	entt::entity levelEntity = gameObjectManager.CreateGameObject(context, levelToLoad.identifier, worldEntity);
	registry.emplace<Component::Level>(levelEntity, levelIndex, levelToLoad.Iid, levelToLoad.identifier,
	                                   levelToLoad.pxWid, levelToLoad.pxHei, levelToLoad.tags);
	worldComponent->loadedLevels[levelIndex] = levelEntity;
	transformSystem.SetWorldTransform(context, levelEntity, glm::vec3(levelToLoad.worldX, levelToLoad.worldY, 0.0f),
	                                  glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

	for (auto& layer : levelToLoad.layers)
	{
		const auto layerEntity = gameObjectManager.CreateGameObject(context, layer.identifier, levelEntity);
		switch (layer.type)
		{
			case FileLoading::LevelParser::LayerType::INT_GRID:
			case FileLoading::LevelParser::LayerType::AUTO_LAYER:
			case FileLoading::LevelParser::LayerType::TILES:
			{
				// A layer with no tileset (an IntGrid layer used only for collision / data) gets no texture: it
				// still holds its grid (and a layer tagged "Collision" still builds its physics body below), it just
				// draws nothing - SpriteRenderSystem skips tile maps without a texture.
				Resource::ResourcePtr<Resource::TextureResource> texture;
				if (layer.tilesetPath.has_value())
				{
					texture = resoruceManager.GetTexture(context, layer.tilesetPath.value());
				}
				transformSystem.SetLocalTransform(context, layerEntity,
				                                  glm::vec3(layer.pxTotalOffsetX, layer.pxTotalOffsetY, 0.0f),
				                                  glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
				std::vector<TileMap::GridTile> grid;
				grid.reserve(layer.autoLayerTiles.size());
				for (auto& gridTile : layer.autoLayerTiles)
				{
					TileMap::GridTile newGridTile{gridTile.px, gridTile.src, (Renderer::FlipBit)gridTile.f};
					grid.push_back(newGridTile);
				}

				// TODO - grab the tileset path from the level somehow - possibly have a store the tilesets in the
				// resource pool and grab is here
				Component::TileMap& tileMap =
				    registry.emplace<Component::TileMap>(layerEntity, std::move(texture), layer.cWid, layer.cHei,
				                                         layer.gridSize, grid, layer.intGrid, layer.renderLayer,
				                                         layer.orderInLayer, layer.rowSorted);

				if (options.createPhysics && FileLoading::LevelParser::LayerHasTag(layer, "Collision"))
				{
					bool isSensor = false;
					b2BodyDef staticBodyDef;
					staticBodyDef.type = b2_staticBody;
					Component::PhysicsBody& physicsBody =
					    physicsSystem.CreatePhysicsBody(context, layerEntity, staticBodyDef);
					Physics::TileMapCollisionBodyGenerator::CreateTileMapShape(context, tileMap, isSensor, physicsBody);
				}
				break;
			}
			case FileLoading::LevelParser::LayerType::ENTITIES:
			{
				if (options.entityMode == EntityMode::NONE)
				{
					break;
				}
				for (auto& entityInstance : layer.entityInstaces)
				{
					const auto layerInstaceEntity =
					    gameObjectManager.CreateGameObject(context, entityInstance.identifier, levelEntity);
					// LDtk's px is the instance's pivot point; the entity sits at the centre of its own rect (so a
					// 32x32 door is centred on its doorway, not on its top-left cell).
					glm::vec2 size{entityInstance.width, entityInstance.height};
					glm::vec2 centre = entityInstance.px - entityInstance.pivot * size + size * 0.5f;
					transformSystem.SetLocalTransform(context, layerInstaceEntity, glm::vec3(centre.x, centre.y, 0.0f),
					                                  glm::vec3(1.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

					std::vector<Wren::WrenItem> wrenArgMap =
					    BuildEntityScriptArgs(entityInstance, worldComponent->worldMap);
					if (options.entityMode == EntityMode::SCRIPT)
					{
						registry.emplace<Component::WrenScript>(layerInstaceEntity, entityInstance.identifier,
						                                        wrenArgMap);
					}
					else
					{
						// From the entity (the rect's centre) to the rect's top-left corner.
						glm::vec2 offset = -size * 0.5f;
						registry.emplace<Component::LevelEntityInstance>(layerInstaceEntity, entityInstance.identifier,
						                                                 entityInstance.Iid, wrenArgMap, offset, size,
						                                                 entityInstance.color);
						wrenScriptSystem.CallPreview(context, layerInstaceEntity, entityInstance.identifier, wrenArgMap);
					}
				}
				break;
			}
			default:
				break;
		}
	}

	return levelEntity;
}

void Struktur::World::Level::UnloadLevelEntities(GameContext& context, const entt::entity worldEntity, int levelIndex)
{
	entt::registry& registry                     = context.GetRegistry();
	System::GameObjectManager& gameObjectManager = context.GetGameObjectManager();

	auto* worldComponent = registry.try_get<Component::World>(worldEntity);
	if (!worldComponent)
	{
		BREAK_MSG("Entity provided does not contain a World Component");
		return;
	}

	auto it = worldComponent->loadedLevels.find(levelIndex);
	if (it == worldComponent->loadedLevels.end())
	{
		return;
	}

	gameObjectManager.SafeDeleteGameObject(context, it->second);
	worldComponent->loadedLevels.erase(it);
}

struct EntityLookup
{
	std::string name;   // its "Name" field, or ""
	std::string level;  // the identifier of the level it's in, or "" if not found
};

// The entity with this iid, anywhere in the world.
static EntityLookup FindEntity(const Struktur::FileLoading::LevelParser::World& world, const std::string& iid)
{
	for (const auto& level : world.levels)
	{
		for (const auto& layer : level.layers)
		{
			for (const auto& entity : layer.entityInstaces)
			{
				if (entity.Iid != iid)
				{
					continue;
				}
				EntityLookup found{"", level.identifier};
				for (const auto& field : entity.fieldInstances)
				{
					if (field.identifier == "Name" && field.value.has_value() &&
					    field.value.type() == typeid(std::string))
					{
						found.name = std::any_cast<std::string>(field.value);
					}
				}
				return found;
			}
		}
	}
	return {};
}

std::vector<Struktur::Wren::WrenItem> Struktur::World::Level::BuildEntityScriptArgs(
    const FileLoading::LevelParser::Entity& entityInstance, const FileLoading::LevelParser::World& world)
{
	// Convert fieldInstances to a Wren map.
	std::vector<Wren::WrenItem> wrenArgMap;
	for (auto field : entityInstance.fieldInstances)
	{
		Wren::WrenItem item{field.identifier};
		// An optional field left empty in LDtk.
		if (!field.value.has_value())
		{
			item.type  = WrenType::WREN_TYPE_NULL;
			item.value = 0;
			wrenArgMap.emplace_back(item);
			continue;
		}
		switch (field.type)
		{
			case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
				item.type  = WrenType::WREN_TYPE_NUM;
				item.value = static_cast<double>(std::any_cast<int>(field.value));
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
				item.type  = WrenType::WREN_TYPE_NUM;
				item.value = static_cast<double>(std::any_cast<float>(field.value));
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::BOOLEAN:
				item.type  = WrenType::WREN_TYPE_BOOL;
				item.value = field.value;
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::MULTILINE:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::FILE_PATH:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::ENUM:
				item.type  = WrenType::WREN_TYPE_STRING;
				item.value = field.value;
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::COLOUR:
			{
				item.type  = WrenType::WREN_TYPE_LIST;
				auto color = std::any_cast<Util::Color>(field.value);
				// Create nested list [r, g, b, a]
				std::vector<Wren::WrenItem> colorList;
				Wren::WrenItem r{"r", static_cast<double>(color.r), WrenType::WREN_TYPE_NUM};
				colorList.emplace_back(r);
				Wren::WrenItem g{"g", static_cast<double>(color.g), WrenType::WREN_TYPE_NUM};
				colorList.emplace_back(g);
				Wren::WrenItem b{"b", static_cast<double>(color.b), WrenType::WREN_TYPE_NUM};
				colorList.emplace_back(b);
				Wren::WrenItem a{"a", static_cast<double>(color.a), WrenType::WREN_TYPE_NUM};
				colorList.emplace_back(a);

				item.value = colorList;
				break;
			}
			case Struktur::FileLoading::LevelParser::FieldInstanceType::POINT:
			{
				item.type  = WrenType::WREN_TYPE_LIST;
				auto point = std::any_cast<glm::vec2>(field.value);
				// Create nested list [x, y]
				std::vector<Wren::WrenItem> colorList;
				Wren::WrenItem x{"x", static_cast<double>(point.x), WrenType::WREN_TYPE_NUM};
				colorList.emplace_back(x);
				Wren::WrenItem y{"y", static_cast<double>(point.y), WrenType::WREN_TYPE_NUM};
				colorList.emplace_back(y);

				item.value = colorList;
				break;
			}
			case Struktur::FileLoading::LevelParser::FieldInstanceType::ENTITY_REF:
			{
				item.type  = WrenType::WREN_TYPE_STRING;
				item.value = field.value;

				std::string names;
				std::string levels;
				std::stringstream iids(std::any_cast<std::string>(field.value));
				std::string iid;
				bool first = true;
				while (std::getline(iids, iid, ','))
				{
					if (!first)
					{
						names += ",";
						levels += ",";
					}
					first                = false;
					EntityLookup found   = FindEntity(world, iid);
					names               += found.name;
					levels              += found.level;
				}
				wrenArgMap.emplace_back(Wren::WrenItem{field.identifier + "Names", names, WrenType::WREN_TYPE_STRING});
				wrenArgMap.emplace_back(Wren::WrenItem{field.identifier + "Levels", levels, WrenType::WREN_TYPE_STRING});
				break;
			}
			case Struktur::FileLoading::LevelParser::FieldInstanceType::TILE:
				item.type  = WrenType::WREN_TYPE_NULL;
				item.value = 0;
				break;
			default:
				item.type  = WrenType::WREN_TYPE_NULL;
				item.value = 0;
				break;
		}
		wrenArgMap.emplace_back(item);
	}
	// The LDtk instance id - stable across level reloads and sessions, so scripts can remember
	// per-instance state (e.g. a defeated critter staying gone - see assets/Scripts/Defeated.wren).
	wrenArgMap.emplace_back(Wren::WrenItem{"Iid", entityInstance.Iid, WrenType::WREN_TYPE_STRING});
	// Its size as placed in LDtk, in px - e.g. a DoorWorp's trigger covers exactly its rect.
	wrenArgMap.emplace_back(
	    Wren::WrenItem{"Width", static_cast<double>(entityInstance.width), WrenType::WREN_TYPE_NUM});
	wrenArgMap.emplace_back(
	    Wren::WrenItem{"Height", static_cast<double>(entityInstance.height), WrenType::WREN_TYPE_NUM});
	return wrenArgMap;
}
