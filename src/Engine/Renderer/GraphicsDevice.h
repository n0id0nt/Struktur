#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <vector>

namespace Struktur
{
namespace Renderer
{
// Owns the bgfx renderer - replaces raylib's rlgl (implicitly initialised by ::InitWindow()). Plain class, no
// singleton, owned by GameContext via unique_ptr and reached through it like every other subsystem.
class GraphicsDevice
{
public:
	// The game's base view: cleared every frame, and the first to draw. It and every ViewTarget::Game view draw into
	// the game's render target (see SetWorldRenderTarget).
	static constexpr bgfx::ViewId WorldViewId = 0;

	// Where a view added with AddView draws.
	enum class ViewTarget
	{
		Game,    // the game's render target (redirected into the editor's Game Viewport), at its size
		Window,  // straight to the window, at its size
		Own      // the caller sets the framebuffer and rect itself
	};

	GraphicsDevice()  = default;
	~GraphicsDevice() = default;

	GraphicsDevice(const GraphicsDevice&)            = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) = delete;

	// Actually initialises bgfx - deferred from construction so GameContext can own this unconditionally (see
	// GameContext's constructor) while the caller still picks when the native window handle/size are known.
	void Initialise(void* nativeWindowHandle, int width, int height);
	// bgfx::shutdown() - explicit rather than in the destructor, since every other subsystem's own GPU resources
	// must be torn down first (their own bgfx::destroy calls fail once bgfx itself has shut down). Callers are
	// responsible for shutting down everything else before calling this (see GameContext::Shutdown()).
	void Shutdown();

	void Resize(int width, int height);

	// Adds a view that draws after every view added before it, without clearing. Sequential keeps the order of its
	// draw calls instead of letting bgfx sort them. Call after Initialise, in the order the views should draw: the
	// game's own scene views, then the editor's debug drawing, the UI, and the editor itself.
	bgfx::ViewId AddView(ViewTarget target, bool sequential = false);

	// Toggles BGFX_RESET_VSYNC and re-applies it immediately via bgfx::reset if already initialised (bgfx allows
	// re-resetting outside of a resize) - see Core::EngineSettings for the persisted setting this backs.
	void SetVSync(bool enabled);
	bool IsVSyncEnabled() const
	{
		return (m_resetFlags & BGFX_RESET_VSYNC) != 0;
	}

	// Redirects the game's views into an offscreen framebuffer (the editor's game-viewport panel) instead of the
	// real backbuffer - a one-time sticky redirect, not a per-frame push/pop like raylib's BeginTextureMode.
	// width/height are the framebuffer's own (fixed, game-resolution) size, cached so Resize() can keep the
	// view rect matching it instead of stretching it to the real window's size.
	void SetWorldRenderTarget(bgfx::FrameBufferHandle frameBuffer, uint16_t width, uint16_t height);
	// Points the game's views at the real backbuffer. Does not forget the framebuffer passed to
	// SetWorldRenderTarget - see RestoreWorldRenderTarget, used by SplashScreenLoop to draw straight to the
	// window (matching web's raw BeginDrawing/EndDrawing) before the editor's game-viewport panel exists to
	// composite anything for it.
	void ResetWorldRenderTarget();
	// Re-applies the last framebuffer passed to SetWorldRenderTarget (no-op if none was ever set).
	void RestoreWorldRenderTarget();

	// Where the game's views currently render: the redirect framebuffer (editor game viewport) or the backbuffer
	// (invalid handle), and its size.
	bgfx::FrameBufferHandle GetWorldTarget(uint16_t& width, uint16_t& height) const;

	// Clears the world view and marks the start of a frame's draw submissions.
	void BeginFrame();
	// bgfx::frame() - hands the recorded draw calls to the render thread and swaps buffers.
	void EndFrame();

	// Fallback program for sprites/tiles with no Component::Shader - WorldRenderer::Flush's batch renderer uses
	// this instead of relying on an implicit default the way raylib's rlgl provided one.
	bgfx::ProgramHandle GetDefaultSpriteProgram() const;

private:
	struct View
	{
		bgfx::ViewId id;
		ViewTarget target;
	};

	// Points the game's views at a framebuffer (or the backbuffer) and sizes them to it.
	void BindGameViews(bgfx::FrameBufferHandle frameBuffer, uint16_t width, uint16_t height);

	// The world view, then everything added, in draw order.
	std::vector<View> m_views;
	int m_width  = 0;
	int m_height = 0;
	// Passed to both bgfx::init (Initialise) and bgfx::reset (Resize/SetVSync) - stored so SetVSync can flip just
	// the VSync bit and re-apply without needing a resize to trigger it.
	uint32_t m_resetFlags = BGFX_RESET_VSYNC;
	// Cached for RestoreWorldRenderTarget - ResetWorldRenderTarget intentionally leaves these untouched.
	bgfx::FrameBufferHandle m_worldFrameBuffer = BGFX_INVALID_HANDLE;
	uint16_t m_worldFrameBufferWidth           = 0;
	uint16_t m_worldFrameBufferHeight          = 0;
	// Tracks whether the game's views are currently pointed at m_worldFrameBuffer (true) or the backbuffer
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
