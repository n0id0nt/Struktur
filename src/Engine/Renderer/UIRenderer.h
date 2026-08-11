#pragma once

#include <string>

#include "Engine/Renderer/RenderTypes.h"
#include "Engine/Text/Font.h"
#include "Engine/Util/Color.h"
#include "Engine/Util/MathUtil.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

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
	UIRenderer() = default;
	~UIRenderer();

	UIRenderer(const UIRenderer&)            = delete;
	UIRenderer& operator=(const UIRenderer&) = delete;

	// Actually creates the white texture/sampler uniform - deferred from construction so GameContext can own
	// this unconditionally (see GameContext's constructor) while still requiring bgfx to already exist by the
	// time this runs (call right after GraphicsDevice::Initialise).
	void Initialise();

	// Sets GraphicsDevice::UIViewId's per-frame orthographic transform, sized to the game viewport (not the
	// real window) - call once per frame before any Draw* call (see UIRenderSystem::Update).
	void SetupView(GameContext& context);

	void DrawRect(const Util::Math::Rect& rect, const Util::Color& color);
	void DrawRectOutline(const Util::Math::Rect& rect, float thickness, const Util::Color& color);
	void DrawTexturedRect(const Util::Math::Rect& rect, const TextureHandle& texture, const Util::Color& tint);
	// Walks `text` as UTF-8 via Text::GetCodepointNext/Text::GetGlyphIndex and emits one quad per glyph from
	// font.glyphs - single line only (callers already split multi-line text, see UILabel::GetTextLines).
	void DrawText(const Text::Font& font, const std::string& text, const glm::vec2& position, float fontSize,
	              const Util::Color& color);

   private:
	void SubmitTexturedQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
	                         uint32_t abgr, bgfx::TextureHandle texture);

	bgfx::TextureHandle m_whiteTexture    = BGFX_INVALID_HANDLE;
	bgfx::UniformHandle m_texColorSampler = BGFX_INVALID_HANDLE;
};
}  // namespace Renderer
}  // namespace Struktur
