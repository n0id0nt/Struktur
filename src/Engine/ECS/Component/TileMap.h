#pragma once

#include <string>
#include <vector>

#include "Engine/Game/RenderLayer.h"
#include "Engine/Game/TileMap.h"
#include "Engine/Resource/TextureResource.h"

namespace Struktur
{
namespace Component
{
struct TileMap
{
	Resource::ResourcePtr<Resource::TextureResource> texture;
	int width;
	int height;
	int tileSize;
	std::vector<GameResource::TileMap::GridTile> gridTiles;
	std::vector<int> grid;
	GameResource::RenderLayer layer;
	// Preserves the LDtk layer's authored back-to-front order within whatever RenderLayer bucket it lands in.
	float orderInLayer;
};
}  // namespace Component
}  // namespace Struktur
