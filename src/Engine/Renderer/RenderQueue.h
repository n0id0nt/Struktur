#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Game/RenderLayer.h"
#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Util/MathUtil.h"
#include "entt/entt.hpp"
#include "glm/glm.hpp"

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
};

struct CullBounds
{
	float minX, minY, maxX, maxY;
};

// Owns the per-frame draw list: sprites and tilemap tiles are Submit()-ed into it (already culled),
// then Flush() sorts everything by (layer, orderInLayer, texture) and issues the actual draws, so
// consecutive same-texture items end up adjacent and raylib/rlgl can batch them into fewer GPU draws.
class RenderQueue
{
   public:
	RenderQueue();

	static CullBounds ComputeCullBounds(GameContext& context);
	static uint64_t PackSortKey(GameResource::RenderLayer layer, float orderInLayer, unsigned int textureId);

	void Clear();
	void Submit(GameResource::RenderLayer layer, float orderInLayer, entt::entity entity,
	           const TextureHandle& texture, const Util::Math::Rect& sourceRec, const Util::Math::Rect& destRec,
	           const glm::vec2& origin, float rotation, const Util::Math::Color& tint, const CullBounds& cullBounds);
	void Flush(GameContext& context);

   private:
	std::vector<DrawItem> m_drawItems;
};
}  // namespace System
}  // namespace Struktur
