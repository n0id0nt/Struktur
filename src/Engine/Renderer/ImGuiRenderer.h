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
// ImDrawVert (pos:2f, uv:2f, col:u32) is byte-for-byte the same layout as SpriteVertex, so this reuses
// BuildSpriteVertexLayout() and GraphicsDevice::GetDefaultSpriteProgram() rather than defining new ones.
class ImGuiRenderer
{
   public:
	ImGuiRenderer() = default;
	~ImGuiRenderer();

	ImGuiRenderer(const ImGuiRenderer&)            = delete;
	ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

	// Actually creates the font atlas texture - deferred from construction so GameContext can own this
	// unconditionally (see GameContext's constructor) while still requiring an ImGui context (with its font
	// atlas configured) to already exist by the time this runs.
	void Initialise();

	void Render(ImDrawData* drawData);

   private:
	bgfx::TextureHandle m_fontTexture     = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur
