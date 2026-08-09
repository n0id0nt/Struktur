#pragma once

#include <string>
#include <vector>

#include "Engine/Game/RenderLayer.h"
#include "Engine/Game/TileMap.h"
#include "Engine/Resource/TextureResource.h"

#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/TileChunk.h"
#endif

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

#if !defined(PLATFORM_WEB)
	// Built lazily on first render (see SpriteRenderSystem) from gridTiles/tileSize - cached static meshes,
	// not rebuilt every frame. Left as-is if a tile changes at runtime; nothing currently invalidates them.
	std::vector<Renderer::TileChunk> chunks;
	bool chunksBuilt = false;
#endif
};
}  // namespace Component
}  // namespace Struktur
