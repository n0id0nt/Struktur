#pragma once

#include <any>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/World/RenderLayer.h"
#include "glm/glm.hpp"
#include "nlohmann/json.hpp"

namespace Struktur
{
class GameContext;

namespace FileLoading
{
namespace LevelParser
{
enum class FieldInstanceType
{
	INTEGER,
	FLOAT,
	BOOLEAN,
	STRING,
	MULTILINE,
	COLOUR,
	ENUM,
	FILE_PATH,
	TILE,
	ENTITY_REF,
	POINT,

	COUNT
};

struct FieldInstance
{
	std::string identifier;
	std::any value;
	FieldInstanceType type;
};

struct Entity
{
	std::string identifier;
	std::string Iid;
	glm::vec2 grid;
	glm::vec2 pivot;
	int width;
	int height;
	glm::vec2 px;
	int worldX;
	int worldY;
	std::vector<FieldInstance> fieldInstances;
};

enum class FlipBit
{
	NONE       = 0,
	HORIZONTAL = 1,
	VERTIAL    = 2,
	BOTH       = 3,

	COUNT
};

enum class LayerType
{
	ENTITIES,
	INT_GRID,
	AUTO_LAYER,
	TILES,

	COUNT
};

struct GridTile
{
	glm::vec2 px;
	glm::vec2 src;
	//glm::vec2 d;
	FlipBit f;
	int t;
	int a;
};

struct Layer
{
	std::string identifier;
	std::string Iid;
	LayerType type;
	int cWid;
	int cHei;
	int gridSize;
	int pxTotalOffsetX;
	int pxTotalOffsetY;
	float opacity;
	std::vector<Entity> entityInstaces;
	std::optional<std::string> tilesetPath;
	std::vector<int> intGrid;
	std::vector<GridTile> autoLayerTiles;
	Struktur::World::RenderLayer renderLayer;
	// Derived from this layer's position relative to the Entities layer in the LDtk file - see
	// LoadLayers. Also usable as a within-layer draw-order tie-break for tile layers.
	float orderInLayer;
};

struct Level
{
	std::string identifier;
	std::string Iid;
	std::vector<Layer> layers;
	int worldX;
	int worldY;
	int pxWid;
	int pxHei;
	std::vector<std::string> neighbours;
};

struct World
{
	std::string Iid;
	std::string filePath;
	std::vector<Level> levels;
};

struct TileSet
{
	std::string identifier;
	int cWid;
	int cHei;
	std::string path;
	int pxWid;
	int pxHei;
	int tileGridSize;
	int spacing;
	int padding;
};

struct Definitions
{
	std::vector<TileSet> tilesets;
};

glm::vec2 LoadJsonVector2(const nlohmann::json& json);

World LoadWorldMap(GameContext& context, const std::string& filePath);
void LoadDefinitions(World& world, Definitions& definitions, const nlohmann::json& json);
void LoadLevels(World& world, const nlohmann::json& json);
void LoadLayers(World& world, Level& level, const nlohmann::json& json);
void LoadEntities(Layer& entityLayer, const nlohmann::json& json);
void LoadFieldInstances(Entity& entity, const nlohmann::json& json);
void LoadIntGrid(Layer& gridLayer, const nlohmann::json& json);
void LoadGridTiles(Layer& gridLayer, const nlohmann::json& json);
FieldInstanceType ConvertFieldTypeToEnum(const std::string& fieldInstanceType);
}  // namespace LevelParser
}  // namespace FileLoading
}  // namespace Struktur
