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
	// The world view all sprites/tiles render into; UI/debug views may be added in later steps.
	static constexpr bgfx::ViewId WorldViewId = 0;
	// Composited after WorldViewId, straight to the backbuffer - the editor's ImGui overlay (see ImGuiRenderer).
	static constexpr bgfx::ViewId EditorViewId = 1;

	GraphicsDevice(void* nativeWindowHandle, int width, int height);
	~GraphicsDevice();

	GraphicsDevice(const GraphicsDevice&)            = delete;
	GraphicsDevice& operator=(const GraphicsDevice&) = delete;

	void Resize(int width, int height);

	// Redirects WorldViewId into an offscreen framebuffer (the editor's game-viewport panel) instead of the
	// real backbuffer - a one-time sticky redirect, not a per-frame push/pop like raylib's BeginTextureMode.
	void SetWorldRenderTarget(bgfx::FrameBufferHandle frameBuffer);
	void ResetWorldRenderTarget();

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
};
}  // namespace Renderer
}  // namespace Struktur
