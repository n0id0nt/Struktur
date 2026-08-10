#pragma once

#if !defined(PLATFORM_WEB)

#include <string>

#include "Engine/Renderer/RenderTypes.h"
#include "raylib.h"

#include <bgfx/bgfx.h>

namespace Struktur
{
class GameContext;
}

namespace Struktur
{
namespace Renderer
{
// bgfx-native replacement for raylib's DrawRectangleRec/DrawRectangleLinesEx/DrawTexturePro/DrawTextEx on the
// desktop UI path (UIElement/UIPanel/UILabel) - the UI-rendering equivalent of ImGuiRenderer. Owns a lazily-
// created 1x1 white texture so solid-color quads reuse the same textured-quad submission (and therefore the
// same "sprite" program) as panel backgrounds and glyphs, matching fs_sprite.sc's
// texture.rgba * vertexColor.rgba blend model.
class UIRenderer
{
   public:
	UIRenderer();
	~UIRenderer();

	UIRenderer(const UIRenderer&)            = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	// Sets GraphicsDevice::UIViewId's per-frame orthographic transform, sized to the game viewport (not the
	// real window) - call once per frame before any Draw* call (see UIRenderSystem::Update).
	void SetupView(GameContext& context);

	void DrawRect(const ::Rectangle& rect, const ::Color& color);
	void DrawRectOutline(const ::Rectangle& rect, float thickness, const ::Color& color);
	void DrawTexturedRect(const ::Rectangle& rect, const TextureHandle& texture, const ::Color& tint);
	// Walks `text` as UTF-8 via ::GetCodepointNext/::GetGlyphIndex and emits one quad per glyph from
	// font.recs/font.glyphs - single line only (callers already split multi-line text, see UILabel::GetTextLines).
	void DrawText(const ::Font& font, const std::string& text, const ::Vector2& position, float fontSize,
	              const ::Color& color);

   private:
	void SubmitTexturedQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
	                         uint32_t abgr, bgfx::TextureHandle texture);

	bgfx::TextureHandle m_whiteTexture    = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur

#endif
