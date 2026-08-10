#pragma once

#include <cstdint>

#include <bgfx/bgfx.h>

namespace Struktur
{
namespace Renderer
{
// Owns the bgfx renderer - replaces raylib's rlgl (implicitly initialised by ::InitWindow()). Plain class, no
// singleton, owned by GameContext via unique_ptr and reached through it like every other subsystem.
class GraphicsDevice
{
   public:
	// The world view all sprites/tiles render into.
	static constexpr bgfx::ViewId WorldViewId = 0;
	// Physics shapes/gizmos/grid/level bounds (DebugSystem) - composited on top of WorldViewId's contents in the
	// same target, using the same camera transform WorldRenderer::Flush sets for WorldViewId every frame (see
	// DebugRenderer::SetupView). Must have a lower ViewId than UIViewId/EditorViewId for the same "shares a
	// render target that EditorViewId later samples via ImGui::Image" reasoning documented on UIViewId below.
	static constexpr bgfx::ViewId DebugViewId = 1;
	// In-game UI (UIRenderSystem) - composited on top of WorldViewId/DebugViewId's contents in the same target.
	// Shares WorldViewId's render target redirect (see SetWorldRenderTarget) so UI stays inside the game viewport
	// panel rather than smearing across the real editor window. Must execute (and therefore have a lower ViewId,
	// bgfx runs views in ascending order by default) BEFORE EditorViewId - the editor's Game Viewport panel
	// displays this shared framebuffer via ImGui::Image from within EditorViewId, so if EditorViewId ran first it
	// would sample the texture one step before UIViewId's draws land in it, and in-game UI would never appear.
	static constexpr bgfx::ViewId UIViewId = 2;
	// Composited after WorldViewId/DebugViewId/UIViewId, straight to the backbuffer - the editor's ImGui overlay
	// (see ImGuiRenderer), including the Game Viewport's display of the shared framebuffer (see UIViewId above).
	static constexpr bgfx::ViewId EditorViewId = 3;

	GraphicsDevice(void* nativeWindowHandle, int width, int height);
	~GraphicsDevice();

	GraphicsDevice(const GraphicsDevice&)            = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) = delete;

	void Resize(int width, int height);

	// Redirects WorldViewId into an offscreen framebuffer (the editor's game-viewport panel) instead of the
	// real backbuffer - a one-time sticky redirect, not a per-frame push/pop like raylib's BeginTextureMode.
	void SetWorldRenderTarget(bgfx::FrameBufferHandle frameBuffer);
	// Points World/Debug/UI views at the real backbuffer. Does not forget the framebuffer passed to
	// SetWorldRenderTarget - see RestoreWorldRenderTarget, used by SplashScreenLoop to draw straight to the
	// window (matching web's raw BeginDrawing/EndDrawing) before the editor's game-viewport panel exists to
	// composite anything for it.
	void ResetWorldRenderTarget();
	// Re-applies the last framebuffer passed to SetWorldRenderTarget (no-op if none was ever set).
	void RestoreWorldRenderTarget();

	// Clears the world view and marks the start of a frame's draw submissions.
	void BeginFrame();
	// bgfx::frame() - hands the recorded draw calls to the render thread and swaps buffers.
	void EndFrame();

	// Fallback program for sprites/tiles with no Component::Shader - WorldRenderer::Flush's batch renderer uses
	// this instead of relying on an implicit default the way raylib's rlgl provided one.
	bgfx::ProgramHandle GetDefaultSpriteProgram() const;

   private:
	int m_width  = 0;
	int m_height = 0;
	// Cached for RestoreWorldRenderTarget - ResetWorldRenderTarget intentionally leaves this untouched.
	bgfx::FrameBufferHandle m_worldFrameBuffer = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur
