#include "RenderQueue.h"

#include <algorithm>
#include <cfloat>
#include <cstring>

#include "Engine/Core/GameData.h"
#include "Engine/ECS/System/ShaderSystem.h"
#include "Engine/GameContext.h"
#include "raylib.h"

namespace
{
// Maps a float to a uint32 that sorts (as an unsigned integer) in the same order as the float,
// including negatives - lets orderInLayer (which can be a negative world Y) participate in the
// packed integer sort key correctly.
uint32_t FloatToSortableBits(float value)
{
	uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	uint32_t mask = (bits & 0x80000000u) ? 0xFFFFFFFFu : 0x80000000u;
	return bits ^ mask;
}

bool RectOverlaps(const Struktur::Util::Math::Rect& rect, const Struktur::Renderer::CullBounds& bounds)
{
	return rect.x < bounds.maxX && rect.x + rect.width > bounds.minX && rect.y < bounds.maxY &&
	      rect.y + rect.height > bounds.minY;
}
}  // namespace

Struktur::Renderer::RenderQueue::RenderQueue()
{
	m_drawItems.reserve(2048);
}

Struktur::Renderer::CullBounds Struktur::Renderer::RenderQueue::ComputeCullBounds(GameContext& context)
{
	GameResource::Camera& camera = context.GetCamera();
	Core::GameData& gameData     = context.GetGameData();
	::Camera2D raylibCamera      = camera.GetRaylibCamera();

	// Transform all 4 screen corners to world space (rather than 2) so a rotated camera still
	// produces a correct, if slightly conservative, axis-aligned world-space bound to cull against.
	::Vector2 corners[4] = {
	    ::GetScreenToWorld2D(::Vector2{0.0f, 0.0f}, raylibCamera),
	    ::GetScreenToWorld2D(::Vector2{(float)gameData.gameWidth, 0.0f}, raylibCamera),
	    ::GetScreenToWorld2D(::Vector2{0.0f, (float)gameData.gameHeight}, raylibCamera),
	    ::GetScreenToWorld2D(::Vector2{(float)gameData.gameWidth, (float)gameData.gameHeight}, raylibCamera),
	};

	CullBounds bounds{FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};
	for (const auto& corner : corners)
	{
		bounds.minX = std::min(bounds.minX, corner.x);
		bounds.minY = std::min(bounds.minY, corner.y);
		bounds.maxX = std::max(bounds.maxX, corner.x);
		bounds.maxY = std::max(bounds.maxY, corner.y);
	}
	return bounds;
}

uint64_t Struktur::Renderer::RenderQueue::PackSortKey(GameResource::RenderLayer layer, float orderInLayer,
                                                    unsigned int textureId)
{
	// [63:60] layer (4 bits) | [59:28] orderInLayer, sortable-float-bits (32 bits) | [27:0] textureId (28 bits)
	uint64_t layerBits   = static_cast<uint64_t>(layer) & 0xFull;
	uint64_t orderBits   = static_cast<uint64_t>(FloatToSortableBits(orderInLayer));
	uint64_t textureBits = static_cast<uint64_t>(textureId) & 0xFFFFFFFull;
	return (layerBits << 60) | (orderBits << 28) | textureBits;
}

void Struktur::Renderer::RenderQueue::Clear()
{
	m_drawItems.clear();
}

void Struktur::Renderer::RenderQueue::Submit(GameResource::RenderLayer layer, float orderInLayer, entt::entity entity,
                                           const TextureHandle& texture, const Util::Math::Rect& sourceRec,
                                           const Util::Math::Rect& destRec, const glm::vec2& origin, float rotation,
                                           const Util::Math::Color& tint, const CullBounds& cullBounds)
{
	if (!RectOverlaps(destRec, cullBounds))
	{
		return;
	}

	m_drawItems.push_back(DrawItem{PackSortKey(layer, orderInLayer, texture.id), entity, texture, sourceRec, destRec,
	                               origin, rotation, tint});
}

void Struktur::Renderer::RenderQueue::Flush(GameContext& context)
{
	std::sort(m_drawItems.begin(), m_drawItems.end(),
	          [](const DrawItem& a, const DrawItem& b) { return a.sortKey < b.sortKey; });

	System::ShaderSystem& shaderSystem = context.GetSystemManager().GetSystem<System::ShaderSystem>();
	// BeginShader/EndShader are no-ops for entities without a Component::Shader, so plain sprites and
	// tiles draw back-to-back here - since the list is sorted with texture as the lowest-priority key,
	// consecutive same-texture DrawTexturePro calls end up adjacent and raylib/rlgl's internal batch
	// renderer coalesces them into a single GPU draw call automatically.
	for (const auto& item : m_drawItems)
	{
		::Texture2D rlTexture{item.texture.id, item.texture.width, item.texture.height, 1,
		                      PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
		::Rectangle rlSourceRec{item.sourceRec.x, item.sourceRec.y, item.sourceRec.width, item.sourceRec.height};
		::Rectangle rlDestRec{item.destRec.x, item.destRec.y, item.destRec.width, item.destRec.height};
		::Vector2 rlOrigin{item.origin.x, item.origin.y};
		::Color rlTint{item.tint.r, item.tint.g, item.tint.b, item.tint.a};

		shaderSystem.BeginShader(context, item.entity);
		::DrawTexturePro(rlTexture, rlSourceRec, rlDestRec, rlOrigin, item.rotation, rlTint);
		shaderSystem.EndShader(context, item.entity);
	}
}
