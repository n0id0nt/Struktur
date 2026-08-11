#include "LevelParser.h"

#include <format>
#include <fstream>
#include <filesystem>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"

glm::vec2 Struktur::FileLoading::LevelParser::LoadJsonVector2(const nlohmann::json& json)
{
	glm::vec2 vector{json[0], json[1]};
	return vector;
}

Struktur::FileLoading::LevelParser::World Struktur::FileLoading::LevelParser::LoadWorldMap(GameContext& context,
                                                                                           const std::string& filePath)
{
	auto result = FileSystem::ReadString(filePath);
	ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());

	nlohmann::json data = nlohmann::json::parse(result.value);

	DEBUG_INFO("Loading world");

	World world;
	world.Iid = data["iid"];
	world.filePath = filePath;
	
	Definitions definitions;
	LoadDefinitions(world, definitions, data["defs"]);
	LoadLevels(world, data["levels"]);

	return world;
}

void Struktur::FileLoading::LevelParser::LoadDefinitions(World& world, Definitions& definitions,
                                                         const nlohmann::json& json)
{
	// layers
	// entities
	// enums
	// tilesets
	for (auto& tilesetJson : json["tilesets"])
	{
		std::string tilesetName = tilesetJson["identifier"];
		DEBUG_INFO(std::format("Loading tileset {}", tilesetName).c_str());

		std::filesystem::path relativePath = tilesetJson["relPath"];
		std::filesystem::path baseFile     = world.filePath;
		std::string path = (baseFile.parent_path() / relativePath).lexically_normal().generic_string();

		TileSet tileset;
		tileset.identifier   = tilesetName;
		tileset.cWid         = tilesetJson["__cWid"];
		tileset.cHei         = tilesetJson["__cHei"];
		tileset.path         = path;
		tileset.pxWid        = tilesetJson["pxWid"];
		tileset.pxHei        = tilesetJson["pxHei"];
		tileset.tileGridSize = tilesetJson["tileGridSize"];
		tileset.spacing      = tilesetJson["spacing"];
		tileset.padding      = tilesetJson["padding"];
		definitions.tilesets.push_back(tileset);
	}
}

void Struktur::FileLoading::LevelParser::LoadLevels(World& world, const nlohmann::json& json)
{
	for (auto& levelJson : json)
	{
		std::string levelName = levelJson["identifier"];
		DEBUG_INFO(std::format("Loading level {}", levelName).c_str());

		Level level;
		level.identifier = levelName;
		level.Iid        = levelJson["iid"];
		level.worldX     = levelJson["worldX"];
		level.worldY     = levelJson["worldY"];
		level.pxWid      = levelJson["pxWid"];
		level.pxHei      = levelJson["pxHei"];
		LoadLayers(world, level, levelJson["layerInstances"]);
		world.levels.push_back(level);
	}
}

namespace
{
bool LayerNameStartsWithAny(const std::string& identifier, std::initializer_list<const char*> prefixes)
{
	for (const char* prefix : prefixes)
	{
		if (identifier.starts_with(prefix))
		{
			return true;
		}
	}
	return false;
}
}  // namespace

void Struktur::FileLoading::LevelParser::LoadLayers(World& world, Level& level, const nlohmann::json& json)
{
	// LDtk orders layerInstances top-to-bottom as shown in the editor's layer panel, i.e. index 0 is
	// the FRONTMOST layer (drawn last). Bucket every other layer's RenderLayer by its position
	// relative to the Entities layer, so authored front/behind intent survives without renaming layers.
	int entitiesIndex = -1;
	for (int i = 0; i < static_cast<int>(json.size()); i++)
	{
		if (json[i]["__type"] == "Entities")
		{
			entitiesIndex = i;
			break;
		}
	}

	int layerIndex = 0;
	for (auto& layerJson : json)
	{
		std::string layerName = layerJson["__identifier"];
		DEBUG_INFO(std::format("Loading layer {}", layerName).c_str());

		std::string layerType = layerJson["__type"];
		Layer layer;
		if (layerType == "Entities")
		{
			layer.type = LayerType::ENTITIES;
			LoadEntities(layer, layerJson["entityInstances"]);
		}
		else if (layerType == "IntGrid" || layerType == "AutoLayer" || layerType == "Tiles")
		{
			if (layerType == "IntGrid")
			{
				layer.type = LayerType::INT_GRID;
				LoadIntGrid(layer, layerJson["intGridCsv"]);
			}
			else if (layerType == "AutoLayer")
			{
				layer.type = LayerType::AUTO_LAYER;
				std::filesystem::path relativePath = layerJson["__tilesetRelPath"];
				std::filesystem::path baseFile     = world.filePath;
				layer.tilesetPath = (baseFile.parent_path() / relativePath).lexically_normal().generic_string();
				LoadGridTiles(layer, layerJson["autoLayerTiles"]);
			}
			else if (layerType == "Tiles")
			{
				layer.type = LayerType::TILES;
				std::filesystem::path relativePath = layerJson["__tilesetRelPath"];
				std::filesystem::path baseFile     = world.filePath;
				layer.tilesetPath = (baseFile.parent_path() / relativePath).lexically_normal().generic_string();
				LoadGridTiles(layer, layerJson["gridTiles"]);
			}
		}

		layer.identifier     = layerName;
		layer.Iid            = layerJson["iid"];
		layer.cWid           = layerJson["__cWid"];
		layer.cHei           = layerJson["__cHei"];
		layer.gridSize       = layerJson["__gridSize"];
		layer.pxTotalOffsetX = layerJson["__pxTotalOffsetX"];
		layer.pxTotalOffsetY = layerJson["__pxTotalOffsetY"];
		layer.opacity        = layerJson["__opacity"];

		// Higher array index = further back (drawn earlier), so a negated index keeps layers ordered
		// correctly relative to each other even when several land in the same RenderLayer bucket.
		layer.orderInLayer = -static_cast<float>(layerIndex);
		if (layer.type == LayerType::ENTITIES)
		{
			layer.renderLayer = Struktur::World::RenderLayer::Entities;
		}
		else if (entitiesIndex >= 0 && layerIndex < entitiesIndex)
		{
			// Listed above Entities in the layer panel -> drawn in front of entities.
			bool isForeground   = LayerNameStartsWithAny(layerName, {"FG_", "Foreground"});
			layer.renderLayer   = isForeground ? Struktur::World::RenderLayer::Foreground
			                                    : Struktur::World::RenderLayer::BackgroundOverlay;
		}
		else
		{
			// Listed below Entities (or no Entities layer in this level) -> drawn behind entities.
			bool isFar        = LayerNameStartsWithAny(layerName, {"BG_Far", "Far_"});
			layer.renderLayer = isFar ? Struktur::World::RenderLayer::BackgroundFar
			                          : Struktur::World::RenderLayer::BackgroundMid;
		}

		level.layers.push_back(layer);
		layerIndex++;
	}
}

void Struktur::FileLoading::LevelParser::LoadEntities(Layer& entityLayer, const nlohmann::json& json)
{
	for (auto& entityJson : json)
	{
		std::string entityName = entityJson["__identifier"];
		DEBUG_INFO(std::format("Loading entity {}", entityName).c_str());

		Entity entity;
		entity.identifier = entityName;
		entity.Iid        = entityJson["iid"];
		entity.grid       = LoadJsonVector2(entityJson["__grid"]);
		entity.pivot      = LoadJsonVector2(entityJson["__pivot"]);
		entity.width      = entityJson["width"];
		entity.height     = entityJson["height"];
		entity.px         = LoadJsonVector2(entityJson["px"]);
		LoadFieldInstances(entity, entityJson["fieldInstances"]);

		entityLayer.entityInstaces.push_back(entity);
	}
}

void Struktur::FileLoading::LevelParser::LoadFieldInstances(Entity& entity, const nlohmann::json& json)
{
	for (auto& fieldInstanceJson : json)
	{
		std::string fieldName = fieldInstanceJson["__identifier"];
		DEBUG_INFO(std::format("Loading field instance {}", fieldName).c_str());

		FieldInstanceType fieldType = ConvertFieldTypeToEnum(fieldInstanceJson["__type"]);

		FieldInstance field;
		field.identifier = fieldName;
		field.type       = fieldType;
		switch (fieldType)
		{
			case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
			{
				int fieldValue = fieldInstanceJson["__value"];
				field.value    = fieldValue;
				break;
			}
			case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
			{
				float fieldValue = fieldInstanceJson["__value"];
				field.value      = fieldValue;
				break;
			}
			case Struktur::FileLoading::LevelParser::FieldInstanceType::BOOLEAN:
			{
				bool fieldValue = fieldInstanceJson["__value"];
				field.value     = fieldValue;
				break;
			}
			case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
			{
				std::string fieldValue = fieldInstanceJson["__value"];
				field.value            = fieldValue;
				break;
			}
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::MULTILINE:
			//	break;
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::COLOUR:
			//	break;
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::ENUM:
			//	break;
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::FILE_PATH:
			//	break;
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::TILE:
			//	break;
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::ENTITY_REF:
			//	break;
			// case Struktur::FileLoading::LevelParser::FieldInstanceType::POINT:
			//	break;
			default:
				assert(false);
				break;
		}

		entity.fieldInstances.push_back(field);
	}
}

void Struktur::FileLoading::LevelParser::LoadIntGrid(Layer& gridLayer, const nlohmann::json& json)
{
	DEBUG_INFO("Loading int grid");
	// gridLayer.intGrid.resize(json.size());
	for (auto& intGridJson : json)
	{
		gridLayer.intGrid.push_back(intGridJson);
	}
}

void Struktur::FileLoading::LevelParser::LoadGridTiles(Layer& gridLayer, const nlohmann::json& json)
{
	DEBUG_INFO("Loading tile layer");
	// gridLayer.autoLayerTiles.resize(json.size());
	for (auto& layerTileJson : json)
	{
		GridTile gridTile;
		gridTile.px  = LoadJsonVector2(layerTileJson["px"]);
		gridTile.src = LoadJsonVector2(layerTileJson["src"]);
		//gridTile.d   = LoadJsonVector2(layerTileJson["d"]);
		gridTile.f   = layerTileJson["f"];
		gridTile.t   = layerTileJson["t"];
		gridTile.a   = layerTileJson["a"];
		gridLayer.autoLayerTiles.push_back(gridTile);
	}
}

Struktur::FileLoading::LevelParser::FieldInstanceType Struktur::FileLoading::LevelParser::ConvertFieldTypeToEnum(
    const std::string& fieldInstanceType)
{
	if (fieldInstanceType == "Float")
	{
		return FieldInstanceType::FLOAT;
	}
	if (fieldInstanceType == "Int")
	{
		return FieldInstanceType::INTEGER;
	}
	if (fieldInstanceType == "String")
	{
		return FieldInstanceType::STRING;
	}
	if (fieldInstanceType == "Bool")
	{
		return FieldInstanceType::BOOLEAN;
	}
	BREAK_MSG(std::format("Need to implement type: {}", fieldInstanceType).c_str());
	return FieldInstanceType::COUNT;
}
