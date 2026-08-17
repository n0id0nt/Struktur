#pragma once

#include <bgfx/bgfx.h>

#include <glm/glm.hpp>

#include "Engine/Util/Color.h"

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
//
// TODO: no text rendering yet - DebugSystem's old FPS counter went through UIRenderer::DrawText for exactly this
// reason (removed rather than given a batch of its own, since the editor's ToolbarWindow already shows FPS via
// ImGui) and would be the first real user once this exists. Give this its own glyph-quad path (mirroring
// UIRenderer::DrawText, but world-space/DebugViewId like everything else here) rather than reaching back into
// UIRenderer for it, so debug/gizmo text doesn't depend on the UI system at all.
class DebugRenderer
{
public:
	DebugRenderer();
	~DebugRenderer();

	DebugRenderer(const DebugRenderer&)            = delete;
	DebugRenderer& operator=(const DebugRenderer&) = delete;

	// Sets DebugViewId's transform to the world camera's view/projection, and every Draw* call below to submit
	// there until SetupUIView below is called instead (see m_currentViewId) - call once before a group of
	// world-space Draw* calls (see DebugSystem::Update). Mirrors WorldRenderer::Flush's own setViewTransform for
	// WorldViewId (not UIRenderer::SetupView's screen-space identity/ortho) so debug shapes track sprites under
	// pan/zoom.
	void SetupView(GameContext& context);
	// Screen-space counterpart to SetupView - sets DebugUIViewId's transform to the same orthographic projection
	// UIRenderer::SetupView uses for UIViewId, and every Draw* call below to submit there until SetupView above
	// is called instead. Call once before a group of Draw* calls meant to highlight something in UI space (e.g.
	// a selected UIElement's on-screen bounds - see Debug::HierarchyWindow) rather than world space.
	void SetupUIView(GameContext& context);

	void DrawLine(const glm::vec2& p1, const glm::vec2& p2, float thickness, const Util::Color& color);
	// Loops the last point back to the first.
	void DrawPolygonOutline(const glm::vec2* points, int count, float thickness, const Util::Color& color);
	// Triangle-fans from points[0] - valid for any convex, CCW-ordered point set (box2d polygons always are).
	void DrawSolidPolygon(const glm::vec2* points, int count, const Util::Color& color);
	void DrawCircleOutline(const glm::vec2& center, float radius, float thickness, const Util::Color& color);
	void DrawSolidCircle(const glm::vec2& center, float radius, const Util::Color& color);
	void DrawRectOutline(const glm::vec2& min, const glm::vec2& max, float thickness, const Util::Color& color);

private:
	void SubmitTriangleFan(const glm::vec2* points, int count, uint32_t abgr);

	bgfx::TextureHandle m_whiteTexture    = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
	// Which view SubmitTriangleFan (and therefore every DrawX call) submits to - toggled by SetupView
	// (GraphicsDevice::DebugViewId) and SetupUIView (GraphicsDevice::DebugUIViewId), set in the constructor to
	// match SetupView's target so a caller that skips calling either still gets the pre-existing world-space
	// behavior rather than an unset/garbage view id.
	bgfx::ViewId m_currentViewId;
};
}  // namespace Renderer
}  // namespace Struktur
