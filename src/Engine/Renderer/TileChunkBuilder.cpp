#include "TileChunkBuilder.h"

#include <cfloat>
#include <cmath>
#include <unordered_map>

#include "Engine/Renderer/SpriteVertex.h"

namespace
{
using Struktur::GameResource::TileMap::FlipBit;
using Struktur::GameResource::TileMap::GridTile;

constexpr int kChunkTiles = 16;

int64_t BucketKey(int bucketX, int bucketY)
{
	return (static_cast<int64_t>(bucketX) << 32) | static_cast<uint32_t>(bucketY);
}
}  // namespace

std::vector<Struktur::Renderer::TileChunk> Struktur::Renderer::BuildTileChunks(
    const std::vector<GameResource::TileMap::GridTile>& gridTiles, int tileSize, const glm::vec2& worldOffset,
    int textureWidth, int textureHeight)
{
	int chunkPixelSize = tileSize * kChunkTiles;

	std::unordered_map<int64_t, std::vector<const GridTile*>> buckets;
	for (const GridTile& tile : gridTiles)
	{
		int bucketX = (int)std::floor(tile.position.x / chunkPixelSize);
		int bucketY = (int)std::floor(tile.position.y / chunkPixelSize);
		buckets[BucketKey(bucketX, bucketY)].push_back(&tile);
	}

	std::vector<TileChunk> chunks;
	chunks.reserve(buckets.size());

	for (auto& [key, tiles] : buckets)
	{
		std::vector<SpriteVertex> vertices;
		std::vector<uint16_t> indices;
		vertices.reserve(tiles.size() * 4);
		indices.reserve(tiles.size() * 6);

		float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

		for (const GridTile* tilePtr : tiles)
		{
			const GridTile& tile = *tilePtr;

			// Mirrors SpriteRenderSystem's per-tile source-rect math exactly (flip via negative width/height,
			// epsilon inset to avoid sampling a sliver of the neighbouring tile in the atlas).
			Util::Math::Rect sourceRec{tile.sourcePosition.x, tile.sourcePosition.y, (float)tileSize, (float)tileSize};
			switch (tile.flipBit)
			{
				case FlipBit::BOTH:
					sourceRec.width *= -1;
					sourceRec.height *= -1;
					break;
				case FlipBit::HORIZONTAL:
					sourceRec.width *= -1;
					break;
				case FlipBit::VERTIAL:
					sourceRec.height *= -1;
					break;
				default:
					break;
			}
			sourceRec.x += 0.0001f;
			sourceRec.y += 0.0001f;
			sourceRec.width -= 0.0002f;
			sourceRec.height -= 0.0002f;

			float destX = tile.position.x + worldOffset.x;
			float destY = tile.position.y + worldOffset.y;
			float destW = (float)tileSize;
			float destH = (float)tileSize;

			float u0 = sourceRec.x / (float)textureWidth;
			float v0 = sourceRec.y / (float)textureHeight;
			float u1 = (sourceRec.x + sourceRec.width) / (float)textureWidth;
			float v1 = (sourceRec.y + sourceRec.height) / (float)textureHeight;
			uint32_t abgr = PackColor(Util::Math::ColorWhite);

			uint16_t base = (uint16_t)vertices.size();
			vertices.push_back({destX, destY, u0, v0, abgr});
			vertices.push_back({destX + destW, destY, u1, v0, abgr});
			vertices.push_back({destX, destY + destH, u0, v1, abgr});
			vertices.push_back({destX + destW, destY + destH, u1, v1, abgr});

			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 2);
			indices.push_back(base + 1);
			indices.push_back(base + 3);

			minX = std::min({minX, destX, destX + destW});
			minY = std::min({minY, destY, destY + destH});
			maxX = std::max({maxX, destX, destX + destW});
			maxY = std::max({maxY, destY, destY + destH});
		}

		if (vertices.empty())
		{
			continue;
		}

		static const bgfx::VertexLayout layout = BuildSpriteVertexLayout();

		TileChunk chunk;
		chunk.vb = bgfx::createVertexBuffer(
		    bgfx::copy(vertices.data(), (uint32_t)(vertices.size() * sizeof(SpriteVertex))), layout);
		chunk.ib = bgfx::createIndexBuffer(bgfx::copy(indices.data(), (uint32_t)(indices.size() * sizeof(uint16_t))));
		chunk.indexCount  = (uint32_t)indices.size();
		chunk.worldBounds = Util::Math::Rect{minX, minY, maxX - minX, maxY - minY};
		chunks.push_back(chunk);
	}

	return chunks;
}

void Struktur::Renderer::DestroyTileChunks(std::vector<TileChunk>& chunks)
{
	for (TileChunk& chunk : chunks)
	{
		if (bgfx::isValid(chunk.vb))
		{
			bgfx::destroy(chunk.vb);
		}
		if (bgfx::isValid(chunk.ib))
		{
			bgfx::destroy(chunk.ib);
		}
	}
	chunks.clear();
}
