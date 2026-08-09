#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Game/RenderLayer.h"
#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Util/MathUtil.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/TileChunk.h"
#endif

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
	Util::Math::Color tint;
#if !defined(PLATFORM_WEB)
	// Set only for chunk items (see SubmitChunk) - Flush() submits these directly from their own cached
	// static buffers instead of folding them into a transient sprite batch.
	const TileChunk* chunk = nullptr;
#endif
};

struct CullBounds
{
	float minX, minY, maxX, maxY;
};

// Owns the per-frame draw list for everything in the game world - individual sprites/tiles (Submit) and
// cached tilemap chunk meshes (SubmitChunk) alike - so both funnel through the same cull, sort, and flush,
// even though tiles-as-a-chunk and a moving sprite end up drawn very differently once Flush() gets to them.
class WorldRenderer
{
   public:
	WorldRenderer();

	static CullBounds ComputeCullBounds(GameContext& context);
	static uint64_t PackSortKey(GameResource::RenderLayer layer, float orderInLayer, unsigned int textureId);

	void Clear();
	void Submit(GameResource::RenderLayer layer, float orderInLayer, entt::entity entity,
	           const TextureHandle& texture, const Util::Math::Rect& sourceRec, const Util::Math::Rect& destRec,
	           const glm::vec2& origin, float rotation, const Util::Math::Color& tint, const CullBounds& cullBounds);
#if !defined(PLATFORM_WEB)
	void SubmitChunk(GameResource::RenderLayer layer, float orderInLayer, const TileChunk& chunk,
	                 const TextureHandle& texture, const CullBounds& cullBounds);
#endif
	void Flush(GameContext& context);

   private:
	std::vector<DrawItem> m_drawItems;
};
}  // namespace Renderer
}  // namespace Struktur
