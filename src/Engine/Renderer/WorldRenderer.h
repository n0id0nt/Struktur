#pragma once

#include <cstdint>
#include <vector>

#include "Engine/World/RenderLayer.h"
#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

#include "Engine/Renderer/TileChunk.h"

namespace Struktur
{
class GameContext;

namespace Renderer
{
struct DrawItem
{
	uint64_t sortKey;
	entt::entity entity;
	TextureHandle texture;
	Util::Math::Rect sourceRec;
	Util::Math::Rect destRec;
	glm::vec2 origin;
	float rotation;
	Util::Color tint;
	// Set only for chunk items (see SubmitChunk) - Flush() submits these directly from their own cached
	// static buffers instead of folding them into a transient sprite batch.
	const TileChunk* chunk = nullptr;
};

struct CullBounds
{
	float minX, minY, maxX, maxY;
};

// Owns the per-frame draw list for everything in the game world - individual sprites/tiles (SubmitSprite) and
// cached tilemap chunk meshes (SubmitChunk) alike - so both funnel through the same cull, sort, and flush,
// even though tiles-as-a-chunk and a moving sprite end up drawn very differently once Flush() gets to them.
class WorldRenderer
{
   public:
	WorldRenderer();

	static CullBounds ComputeCullBounds(GameContext& context);
	static uint64_t PackSortKey(World::RenderLayer layer, float orderInLayer, unsigned int textureId);

	void Clear();
	void SubmitSprite(World::RenderLayer layer, float orderInLayer, entt::entity entity,
	                  const TextureHandle& texture, const Util::Math::Rect& sourceRec, const Util::Math::Rect& destRec,
	                  const glm::vec2& origin, float rotation, const Util::Color& tint, const CullBounds& cullBounds);
	void SubmitChunk(World::RenderLayer layer, float orderInLayer, const TileChunk& chunk,
	                 const TextureHandle& texture, const CullBounds& cullBounds);
	void Flush(GameContext& context);

   private:
	// Builds a transient vertex/index buffer for m_drawItems[runStart, runEnd) - all sharing runTexture/runProgram
	// - and submits it in a single draw call. See Flush() for how runs are formed.
	void FlushRun(size_t runStart, size_t runEnd, const bgfx::VertexLayout& spriteLayout,
	             bgfx::UniformHandle texColorSampler, uint64_t drawState, const TextureHandle& runTexture,
	             bgfx::ProgramHandle runProgram);

	std::vector<DrawItem> m_drawItems;
};
}  // namespace Renderer
}  // namespace Struktur
