#pragma once

#include <cstdint>
#include <vector>

#include "Engine/Game/RenderLayer.h"
#include "entt/entt.hpp"
#include "raylib.h"

namespace Struktur
{
class GameContext;

namespace Renderer
{
struct DrawItem
{
	uint64_t sortKey;
	entt::entity entity;
	::Texture2D texture;
	::Rectangle sourceRec;
	::Rectangle destRec;
	::Vector2 origin;
	float rotation;
	::Color tint;
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
	void Submit(GameResource::RenderLayer layer, float orderInLayer, entt::entity entity, const ::Texture2D& texture,
	           const ::Rectangle& sourceRec, const ::Rectangle& destRec, const ::Vector2& origin, float rotation,
	           const ::Color& tint, const CullBounds& cullBounds);
	void Flush(GameContext& context);

   private:
	std::vector<DrawItem> m_drawItems;
};
}  // namespace System
}  // namespace Struktur
