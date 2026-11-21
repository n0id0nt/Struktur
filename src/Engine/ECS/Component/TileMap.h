#pragma once

#include <string>
#include <vector>

#include "Engine/Renderer/TileChunk.h"
#include "Engine/Resource/TextureResource.h"
#include "Engine/World/RenderLayer.h"
#include "Engine/World/TileMap.h"

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
	std::vector<Struktur::World::TileMap::GridTile> gridTiles;
	std::vector<int> grid;
	Struktur::World::RenderLayer layer;
	// Preserves the LDtk layer's authored back-to-front order within whatever RenderLayer bucket it lands in.
	float orderInLayer;
	// A layer tagged "Sort" in LDtk (see LevelParser::LoadLayers): drawn on RenderLayer::Entities, each tile row
	// y-sorted with the characters by its bottom edge instead of the whole map sharing orderInLayer.
	bool rowSorted = false;

	// Built lazily on first render (see SpriteRenderSystem) from gridTiles/tileSize - cached static meshes,
	// not rebuilt every frame. Left as-is if a tile changes at runtime; nothing currently invalidates them.
	std::vector<Renderer::TileChunk> chunks;
	bool chunksBuilt = false;
};
}  // namespace Component
}  // namespace Struktur
