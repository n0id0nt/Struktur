#pragma once

#include <glm/glm.hpp>

#include "Engine/Util/MathUtil.h"

#include <bgfx/bgfx.h>

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Renderer
{
// bgfx-native world-space debug-shape renderer (physics shapes, entity gizmos, grid, level bounds) - the
// world-space counterpart to UIRenderer. Targets GraphicsDevice::DebugViewId (camera-transformed, composited
// above WorldViewId and below UIViewId) instead of UIViewId's screen-space projection. Owns its own 1x1 white
// texture so every shape (line/polygon/circle) reuses the same textured-triangle-fan submission and therefore
// the same "sprite" program as everything else in the desktop pipeline, matching fs_sprite.sc's
// texture.rgba * vertexColor.rgba blend model.
class DebugRenderer
{
   public:
	DebugRenderer();
	~DebugRenderer();

	DebugRenderer(const DebugRenderer&)            = delete;
	DebugRenderer& operator=(const DebugRenderer&) = delete;

	// Sets DebugViewId's transform to the world camera's view/projection - call once per frame before any
	// Draw* call (see DebugSystem::Update). Mirrors WorldRenderer::Flush's own setViewTransform for WorldViewId
	// (not UIRenderer::SetupView's screen-space identity/ortho) so debug shapes track sprites under pan/zoom.
	void SetupView(GameContext& context);

	void DrawLine(const glm::vec2& p1, const glm::vec2& p2, float thickness, const Util::Math::Color& color);
	// Loops the last point back to the first.
	void DrawPolygonOutline(const glm::vec2* points, int count, float thickness, const Util::Math::Color& color);
	// Triangle-fans from points[0] - valid for any convex, CCW-ordered point set (box2d polygons always are).
	void DrawSolidPolygon(const glm::vec2* points, int count, const Util::Math::Color& color);
	void DrawCircleOutline(const glm::vec2& center, float radius, float thickness, const Util::Math::Color& color);
	void DrawSolidCircle(const glm::vec2& center, float radius, const Util::Math::Color& color);
	void DrawRectOutline(const glm::vec2& min, const glm::vec2& max, float thickness, const Util::Math::Color& color);

   private:
	void SubmitTriangleFan(const glm::vec2* points, int count, uint32_t abgr);

	bgfx::TextureHandle m_whiteTexture    = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur
