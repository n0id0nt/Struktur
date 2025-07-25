#include "LevelParser.h"

#include <fstream>
#include "raylib.h"
#include <format>

#include "Engine/GameContext.h"
#include "Debug/Assertions.h"

glm::vec2 Struktur::FileLoading::LevelParser::LoadJsonVector2(const nlohmann::json& json)
{
	glm::vec2 vector{ json[0], json[1] };
	return vector;
}

Struktur::FileLoading::LevelParser::World Struktur::FileLoading::LevelParser::LoadWorldMap(GameContext& context, const std::string& filePath)
{
	std::ifstream file(filePath);
	assert(file);
	nlohmann::json data = nlohmann::json::parse(file);
	file.close();

	DEBUG_INFO("Loading world");

	World world;
	world.Iid = data["iid"];

	LoadLevels(world, data["levels"]);

	return world;
}

void Struktur::FileLoading::LevelParser::LoadLevels(World& world, const nlohmann::json& json)
{
	for (auto& levelJson : json)
	{
		std::string levelName = levelJson["identifier"];
		DEBUG_INFO(std::format("Loading level {}", levelName).c_str());

		Level level;
		level.identifier = levelName;
		level.Iid = levelJson["iid"];
		level.worldX = levelJson["worldX"];
		level.worldY = levelJson["worldY"];
		level.pxWid = levelJson["pxWid"];
		level.pxHei = levelJson["pxHei"];
		LoadLayers(level, levelJson["layerInstances"]);
		world.levels.push_back(level);
	}
}

void Struktur::FileLoading::LevelParser::LoadLayers(Level& level, const nlohmann::json& json)
{
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
		else if (layerType == "IntGrid" || layerType == "AutoLayer")
		{
			if (layerType == "IntGrid")
			{
				layer.type = LayerType::INT_GRID;
				LoadIntGrid(layer, layerJson["intGridCsv"]);
			}
			else if (layerType == "AutoLayer")
			{
				layer.type = LayerType::AUTO_LAYER;
			}
			layer.tilesetRelPath = layerJson["__tilesetRelPath"];
			LoadAutoLayerTiles(layer, layerJson["autoLayerTiles"]);
		}
		
		layer.identifier = layerName;
		layer.Iid = layerJson["iid"];
		layer.cWid = layerJson["__cWid"];
		layer.cHei = layerJson["__cHei"];
		layer.gridSize = layerJson["__gridSize"];
		layer.pxTotalOffsetX = layerJson["__pxTotalOffsetX"];
		layer.pxTotalOffsetY = layerJson["__pxTotalOffsetY"];
		layer.opacity = layerJson["__opacity"];

		level.layers.push_back(layer);
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
		entity.Iid = entityJson["iid"];
		entity.grid = LoadJsonVector2(entityJson["__grid"]);
		entity.pivot = LoadJsonVector2(entityJson["__pivot"]);
		entity.width = entityJson["width"];
		entity.height = entityJson["height"];
		entity.px = LoadJsonVector2(entityJson["px"]);
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
		field.type = fieldType;
		switch (fieldType)
		{
		case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
		{
			int fieldValue = fieldInstanceJson["__value"];
			field.value = fieldValue;
			break;
		}		
		case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
		{
			float fieldValue = fieldInstanceJson["__value"];
			field.value = fieldValue;
			break;
		}
		case Struktur::FileLoading::LevelParser::FieldInstanceType::BOOLEAN:
		{
			bool fieldValue = fieldInstanceJson["__value"];
			field.value = fieldValue;
			break;
		}
		case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
		{
			std::string fieldValue = fieldInstanceJson["__value"];
			field.value = fieldValue;
			break;
		}		
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::MULTILINE:
		//	break;
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::COLOUR:
		//	break;
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::ENUM:
		//	break;
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::FILE_PATH:
		//	break;
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::TILE:
		//	break;
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::ENTITY_REF:
		//	break;
		//case Struktur::FileLoading::LevelParser::FieldInstanceType::POINT:
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
	//gridLayer.intGrid.resize(json.size());
	for (auto& intGridJson : json)
	{
		gridLayer.intGrid.push_back(intGridJson);
	}
}

void Struktur::FileLoading::LevelParser::LoadAutoLayerTiles(Layer& gridLayer, const nlohmann::json& json)
{
	DEBUG_INFO("Loading int grid");
	//gridLayer.autoLayerTiles.resize(json.size());
	for (auto& autoLayerTileJson : json)
	{
		GridTile gridTile;
		gridTile.px = LoadJsonVector2(autoLayerTileJson["px"]);
		gridTile.src = LoadJsonVector2(autoLayerTileJson["src"]);
		gridTile.d = LoadJsonVector2(autoLayerTileJson["d"]);
		gridTile.f = autoLayerTileJson["f"];
		gridTile.t = autoLayerTileJson["t"];
		gridTile.a = autoLayerTileJson["a"];
		gridLayer.autoLayerTiles.push_back(gridTile);
	}
}

Struktur::FileLoading::LevelParser::FieldInstanceType Struktur::FileLoading::LevelParser::ConvertFieldTypeToEnum(const std::string& fieldInstanceType)
{
	if (fieldInstanceType == "Float")
	{
		return FieldInstanceType::FLOAT;
	}
	if (fieldInstanceType == "Int")
	{
		return FieldInstanceType::INTEGER;
	}
	assert(false); // need to implement this type
	return FieldInstanceType::COUNT;
}
