#include "LevelParser.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Text/RichText.h"

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
	world.Iid      = data["iid"];
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
	// enums - only their values' colours are kept (scripts read them through World.getLevelFieldColor)
	if (json.contains("enums"))
	{
		for (auto& enumJson : json["enums"])
		{
			auto& colors = world.enumColors[enumJson["identifier"].get<std::string>()];
			for (auto& valueJson : enumJson["values"])
			{
				int rgb = valueJson.value("color", 0);
				colors[valueJson["id"].get<std::string>()] =
				    Util::Color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
			}
		}
	}
	// tilesets
	for (auto& tilesetJson : json["tilesets"])
	{
		std::string tilesetName = tilesetJson["identifier"];
		DEBUG_INFO("Loading tileset %s", tilesetName);

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
		DEBUG_INFO("Loading level %s", levelName);

		Level level;
		level.identifier = levelName;
		level.Iid        = levelJson["iid"];
		level.worldX     = levelJson["worldX"];
		level.worldY     = levelJson["worldY"];
		level.pxWid      = levelJson["pxWid"];
		level.pxHei      = levelJson["pxHei"];
		LoadNeighbours(level, levelJson["__neighbours"]);
		LoadLevelTags(level, levelJson["fieldInstances"]);
		LoadFieldInstances(level.fieldInstances, levelJson["fieldInstances"]);
		LoadLayers(world, level, levelJson["layerInstances"]);
		world.levels.push_back(level);
	}
}

void Struktur::FileLoading::LevelParser::LoadNeighbours(Level& level, const nlohmann::json& json)
{
	for (auto& neighbourJson : json)
	{
		Neighbour neighbour;
		neighbour.levelIid = neighbourJson["levelIid"];
		neighbour.dir      = neighbourJson["dir"];
		level.neighbours.push_back(neighbour);
	}
}

namespace
{
std::vector<std::string> SplitAndTrim(const std::string& value, char delimiter)
{
	std::vector<std::string> result;
	std::stringstream stream(value);
	std::string piece;
	while (std::getline(stream, piece, delimiter))
	{
		size_t start = piece.find_first_not_of(" \t");
		size_t end   = piece.find_last_not_of(" \t");
		if (start == std::string::npos)
		{
			continue;
		}
		result.push_back(piece.substr(start, end - start + 1));
	}
	return result;
}
}  // namespace

void Struktur::FileLoading::LevelParser::LoadLevelTags(Level& level, const nlohmann::json& fieldInstancesJson)
{
	for (auto& fieldJson : fieldInstancesJson)
	{
		std::string fieldName = fieldJson["__identifier"];
		if (fieldName != "Tags")
		{
			continue;
		}

		if (fieldJson["__value"].is_null())
		{
			continue;
		}

		std::string value = fieldJson["__value"];
		level.tags         = SplitAndTrim(value, ',');
	}
}

int Struktur::FileLoading::LevelParser::FindLevelIndexByIdentifier(const World& world, const std::string& identifier)
{
	for (int i = 0; i < static_cast<int>(world.levels.size()); i++)
	{
		if (world.levels[i].identifier == identifier)
		{
			return i;
		}
	}
	return -1;
}

int Struktur::FileLoading::LevelParser::FindLevelIndexByIid(const World& world, const std::string& iid)
{
	for (int i = 0; i < static_cast<int>(world.levels.size()); i++)
	{
		if (world.levels[i].Iid == iid)
		{
			return i;
		}
	}
	return -1;
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
		DEBUG_INFO("Loading layer %s", layerName);

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
				// An IntGrid layer can have a tileset (its value icons / auto-rules in LDtk) - use it, and any tiles its
				// auto-rules produced, instead of the engine's fallback collision texture.
				if (layerJson.contains("__tilesetRelPath") && layerJson["__tilesetRelPath"].is_string())
				{
					std::filesystem::path relativePath = layerJson["__tilesetRelPath"];
					std::filesystem::path baseFile     = world.filePath;
					layer.tilesetPath = (baseFile.parent_path() / relativePath).lexically_normal().generic_string();
					LoadGridTiles(layer, layerJson["autoLayerTiles"]);
				}
			}
			else if (layerType == "AutoLayer")
			{
				layer.type                         = LayerType::AUTO_LAYER;
				std::filesystem::path relativePath = layerJson["__tilesetRelPath"];
				std::filesystem::path baseFile     = world.filePath;
				layer.tilesetPath = (baseFile.parent_path() / relativePath).lexically_normal().generic_string();
				LoadGridTiles(layer, layerJson["autoLayerTiles"]);
			}
			else if (layerType == "Tiles")
			{
				layer.type                         = LayerType::TILES;
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
			bool isForeground = LayerNameStartsWithAny(layerName, {"FG_", "Foreground"});
			layer.renderLayer = isForeground ? Struktur::World::RenderLayer::Foreground
			                                 : Struktur::World::RenderLayer::BackgroundOverlay;
		}
		else
		{
			// Listed below Entities (or no Entities layer in this level) -> drawn behind entities.
			bool isFar = LayerNameStartsWithAny(layerName, {"BG_Far", "Far_"});
			layer.renderLayer =
			    isFar ? Struktur::World::RenderLayer::BackgroundFar : Struktur::World::RenderLayer::BackgroundMid;
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
		DEBUG_INFO("Loading entity %s", entityName);

		Entity entity;
		entity.identifier = entityName;
		entity.Iid        = entityJson["iid"];
		entity.grid       = LoadJsonVector2(entityJson["__grid"]);
		entity.pivot      = LoadJsonVector2(entityJson["__pivot"]);
		entity.width      = entityJson["width"];
		entity.height     = entityJson["height"];
		entity.px         = LoadJsonVector2(entityJson["px"]);
		if (entityJson.contains("__smartColor"))
		{
			Text::TryParseHexColor(entityJson["__smartColor"].get<std::string>(), entity.color);
		}
		LoadFieldInstances(entity.fieldInstances, entityJson["fieldInstances"]);

		entityLayer.entityInstaces.push_back(entity);
	}
}

namespace
{
// One LDtk field value as the string a script arg carries: enums are their value id, entity refs the referenced
// instance's iid, numbers/bools their text. Null (an optional field left empty) is an empty string.
std::string FieldValueToString(const nlohmann::json& value)
{
	if (value.is_null())
	{
		return "";
	}
	if (value.is_string())
	{
		return value.get<std::string>();
	}
	if (value.is_object() && value.contains("entityIid"))
	{
		return value["entityIid"].get<std::string>();
	}
	return value.dump();
}
}  // namespace

void Struktur::FileLoading::LevelParser::LoadFieldInstances(std::vector<FieldInstance>& fields,
                                                            const nlohmann::json& json)
{
	for (auto& fieldInstanceJson : json)
	{
		std::string fieldName = fieldInstanceJson["__identifier"];
		DEBUG_INFO("Loading field instance %s", fieldName);

		const std::string typeName  = fieldInstanceJson["__type"];
		const nlohmann::json& value = fieldInstanceJson["__value"];

		FieldInstance field;
		field.identifier = fieldName;

		// Arrays reach scripts as one comma-separated string (script args only carry numbers, bools and strings) -
		// e.g. a Door's Keys, the iids of the entities it references. An array of entity refs stays ENTITY_REF so
		// World::Level can resolve the referenced entities too.
		if (typeName.starts_with("Array<"))
		{
			const std::string elementType = typeName.substr(6, typeName.size() - 7);
			field.type                    = elementType == "EntityRef" ? FieldInstanceType::ENTITY_REF : FieldInstanceType::STRING;
			std::string joined;
			if (value.is_array())
			{
				for (auto& element : value)
				{
					if (element.is_null())
					{
						continue;
					}
					if (!joined.empty())
					{
						joined += ",";
					}
					joined += FieldValueToString(element);
				}
			}
			field.value = joined;
			fields.push_back(field);
			continue;
		}

		field.type = ConvertFieldTypeToEnum(typeName);
		if (field.type == FieldInstanceType::ENUM)
		{
			field.enumType = typeName.substr(typeName.find('.') + 1);
		}
		// An optional field left empty in LDtk - the script sees null.
		if (value.is_null())
		{
			fields.push_back(field);
			continue;
		}

		switch (field.type)
		{
			case Struktur::FileLoading::LevelParser::FieldInstanceType::INTEGER:
				field.value = value.get<int>();
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::FLOAT:
				field.value = value.get<float>();
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::BOOLEAN:
				field.value = value.get<bool>();
				break;
			case Struktur::FileLoading::LevelParser::FieldInstanceType::STRING:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::MULTILINE:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::FILE_PATH:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::ENUM:
			case Struktur::FileLoading::LevelParser::FieldInstanceType::ENTITY_REF:
				field.value = FieldValueToString(value);
				break;
			default:
				// Colour / Point / Tile aren't used by any script yet - pass them as null rather than crash.
				DEBUG_WARNING("Field %s has unsupported type %s - passed to its script as null", fieldName.c_str(),
				              typeName.c_str());
				field.value.reset();
				break;
		}

		fields.push_back(field);
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
		// gridTile.d   = LoadJsonVector2(layerTileJson["d"]);
		gridTile.f = layerTileJson["f"];
		gridTile.t = layerTileJson["t"];
		gridTile.a = layerTileJson["a"];
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
	if (fieldInstanceType == "Multilines")
	{
		return FieldInstanceType::MULTILINE;
	}
	if (fieldInstanceType == "FilePath")
	{
		return FieldInstanceType::FILE_PATH;
	}
	if (fieldInstanceType == "EntityRef")
	{
		return FieldInstanceType::ENTITY_REF;
	}
	if (fieldInstanceType.starts_with("LocalEnum.") || fieldInstanceType.starts_with("ExternEnum."))
	{
		return FieldInstanceType::ENUM;
	}
	if (fieldInstanceType == "Color")
	{
		return FieldInstanceType::COLOUR;
	}
	if (fieldInstanceType == "Point")
	{
		return FieldInstanceType::POINT;
	}
	if (fieldInstanceType == "Tile")
	{
		return FieldInstanceType::TILE;
	}
	DEBUG_WARNING("Unknown LDtk field type: %s", fieldInstanceType.c_str());
	return FieldInstanceType::COUNT;
}
