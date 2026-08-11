#pragma once

#include <vector>

#include "Engine/World/TileMap.h"
#include "Engine/Renderer/TileChunk.h"
#include "glm/glm.hpp"

namespace Struktur
{
namespace Renderer
{
// Groups gridTiles into fixed-size spatial buckets and builds one static vertex/index buffer per bucket.
// worldOffset bakes the tilemap entity's world position into the chunk geometry once, up front - if the
// entity ever moves, the caller needs to rebuild the chunks (tilemaps are treated as static once placed).
std::vector<TileChunk> BuildTileChunks(const std::vector<World::TileMap::GridTile>& gridTiles, int tileSize,
                                       const glm::vec2& worldOffset, int textureWidth, int textureHeight);

// Destroys every chunk's GPU buffers. Not currently called on TileMap component destruction (see the plan's
// known-limitations note) - chunks are effectively leaked if a level is unloaded, only freed at process exit.
void DestroyTileChunks(std::vector<TileChunk>& chunks);
}  // namespace Renderer
}  // namespace Struktur
