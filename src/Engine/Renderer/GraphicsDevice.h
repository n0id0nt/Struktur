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
#ifdef EDITOR
	// Composited after WorldViewId/DebugViewId/UIViewId, straight to the backbuffer - the editor's ImGui overlay
	// (see ImGuiRenderer), including the Game Viewport's display of the shared framebuffer (see UIViewId above).
	static constexpr bgfx::ViewId EditorViewId = 3;
#endif

	GraphicsDevice() = default;
	~GraphicsDevice();

	GraphicsDevice(const GraphicsDevice&)            = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) = delete;

	// Actually initialises bgfx - deferred from construction so GameContext can own this unconditionally (see
	// GameContext's constructor) while the caller still picks when the native window handle/size are known.
	void Initialise(void* nativeWindowHandle, int width, int height);

	void Resize(int width, int height);

	// Redirects WorldViewId into an offscreen framebuffer (the editor's game-viewport panel) instead of the
	// real backbuffer - a one-time sticky redirect, not a per-frame push/pop like raylib's BeginTextureMode.
	// width/height are the framebuffer's own (fixed, game-resolution) size, cached so Resize() can keep the
	// view rect matching it instead of stretching it to the real window's size.
	void SetWorldRenderTarget(bgfx::FrameBufferHandle frameBuffer, uint16_t width, uint16_t height);
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
	// Cached for RestoreWorldRenderTarget - ResetWorldRenderTarget intentionally leaves these untouched.
	bgfx::FrameBufferHandle m_worldFrameBuffer = BGFX_INVALID_HANDLE;
	uint16_t m_worldFrameBufferWidth           = 0;
	uint16_t m_worldFrameBufferHeight          = 0;
	// Tracks whether World/Debug/UI are currently pointed at m_worldFrameBuffer (true) or the backbuffer
	// (false - either no redirect was ever set, or ResetWorldRenderTarget bypassed it). Resize() needs this to
	// know which one to reapply after bgfx::reset(), which unconditionally clears every view's frame buffer
	// binding back to the backbuffer as a side effect (see bgfx's Context::reset).
	bool m_worldRenderTargetActive = false;
	// Guards ~GraphicsDevice's bgfx::shutdown() - default-constructed instances never call bgfx::init, so
	// shutting down unconditionally would tear down a bgfx context that was never brought up.
	bool m_initialised = false;
};
}  // namespace Renderer
}  // namespace Struktur
