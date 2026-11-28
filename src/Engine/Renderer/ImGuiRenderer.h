#pragma once

#include <bgfx/bgfx.h>

struct ImDrawData;

namespace Struktur
{
namespace Renderer
{
// Hand-rolled bgfx rendering backend for ImGui - replaces rlImGui's raylib/rlgl immediate-mode path. No
// imgui_impl_bgfx exists upstream; input/platform plumbing comes from ImGui's own imgui_impl_sdl3 instead
// (see Window::SetEventCallback), this class only turns ImDrawData into bgfx submissions.
//
// ImDrawVert (pos:2f, uv:2f, col:u32) is byte-for-byte the same layout as QuadVertex, so this reuses
// BuildQuadVertexLayout() and GraphicsDevice::GetDefaultSpriteProgram() rather than defining new ones.
class GraphicsDevice;

class ImGuiRenderer
{
public:
	ImGuiRenderer()  = default;
	~ImGuiRenderer() = default;

	ImGuiRenderer(const ImGuiRenderer&)            = delete;
	ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

	// Actually creates the font atlas texture - deferred from construction so GameContext can own this
	// unconditionally (see GameContext's constructor) while still requiring an ImGui context (with its font
	// atlas configured) to already exist by the time this runs.
	// Adds the editor's view, so call it after every view the editor draws over.
	void Initialise(GraphicsDevice& device);
	// Explicit rather than in the destructor - must run before GraphicsDevice::Shutdown() (see
	// GameContext::Shutdown()), since bgfx::destroy() is invalid once bgfx itself has shut down.
	void Shutdown();

	void Render(ImDrawData* drawData);

private:
	bgfx::ViewId m_viewId                 = 0;
	bgfx::TextureHandle m_fontTexture     = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur
