#include "UIRenderer.h"

#if !defined(PLATFORM_WEB)

	#include "Debug/Assertions.h"
	#include "Engine/Core/GameData.h"
	#include "Engine/GameContext.h"
	#include "Engine/Renderer/EmbeddedShaders.h"
	#include "Engine/Renderer/GraphicsDevice.h"
	#include "Engine/Renderer/SpriteVertex.h"

	#include <glm/gtc/matrix_transform.hpp>
	#include <glm/gtc/type_ptr.hpp>

Struktur::Renderer::UIRenderer::UIRenderer()
{
	m_texColorSampler = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

	uint8_t whitePixel[4] = {255, 255, 255, 255};
	m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, 0, bgfx::copy(whitePixel, 4));
	ASSERT_MSG(bgfx::isValid(m_whiteTexture), "Failed to create UI white texture");
}

Struktur::Renderer::UIRenderer::~UIRenderer()
{
	if (bgfx::isValid(m_whiteTexture))
	{
		bgfx::destroy(m_whiteTexture);
	}
}

void Struktur::Renderer::UIRenderer::SetupView(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	glm::mat4 identity(1.0f);
	// Screen-space, top-left origin, sized to the game's logical resolution (not the real window) - matches
	// WorldRenderer's camera projection sizing convention so UI lines up with the same target when
	// GraphicsDevice::SetWorldRenderTarget redirects both views into the editor's Game Viewport framebuffer.
	glm::mat4 proj = glm::ortho(0.0f, (float)gameData.gameWidth, (float)gameData.gameHeight, 0.0f, -1.0f, 1.0f);
	bgfx::setViewTransform(GraphicsDevice::UIViewId, glm::value_ptr(identity), glm::value_ptr(proj));
}

void Struktur::Renderer::UIRenderer::SubmitTexturedQuad(float x, float y, float w, float h, float u0, float v0,
                                                         float u1, float v1, uint32_t abgr,
                                                         bgfx::TextureHandle texture)
{
	static const bgfx::VertexLayout layout = BuildSpriteVertexLayout();
	if (4 > bgfx::getAvailTransientVertexBuffer(4, layout) || 6 > bgfx::getAvailTransientIndexBuffer(6))
	{
		return;
	}

	bgfx::TransientVertexBuffer tvb;
	bgfx::TransientIndexBuffer tib;
	bgfx::allocTransientVertexBuffer(&tvb, 4, layout);
	bgfx::allocTransientIndexBuffer(&tib, 6);

	SpriteVertex* vertices = (SpriteVertex*)tvb.data;
	vertices[0]            = {x, y, u0, v0, abgr};
	vertices[1]            = {x + w, y, u1, v0, abgr};
	vertices[2]            = {x + w, y + h, u1, v1, abgr};
	vertices[3]            = {x, y + h, u0, v1, abgr};

	uint16_t* indices = (uint16_t*)tib.data;
	indices[0]         = 0;
	indices[1]         = 1;
	indices[2]         = 2;
	indices[3]         = 0;
	indices[4]         = 2;
	indices[5]         = 3;

	bgfx::setTexture(0, m_texColorSampler, texture);
	bgfx::setVertexBuffer(0, &tvb, 0, 4);
	bgfx::setIndexBuffer(&tib, 0, 6);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
	bgfx::submit(GraphicsDevice::UIViewId, GetEmbeddedProgram("sprite"));
}

void Struktur::Renderer::UIRenderer::DrawRect(const ::Rectangle& rect, const ::Color& color)
{
	SubmitTexturedQuad(rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, 1.0f, 1.0f, PackColorRGBA(color),
	                    m_whiteTexture);
}

void Struktur::Renderer::UIRenderer::DrawRectOutline(const ::Rectangle& rect, float thickness, const ::Color& color)
{
	DrawRect(::Rectangle{rect.x, rect.y, rect.width, thickness}, color);                                    // top
	DrawRect(::Rectangle{rect.x, rect.y + rect.height - thickness, rect.width, thickness}, color);          // bottom
	DrawRect(::Rectangle{rect.x, rect.y, thickness, rect.height}, color);                                   // left
	DrawRect(::Rectangle{rect.x + rect.width - thickness, rect.y, thickness, rect.height}, color);          // right
}

void Struktur::Renderer::UIRenderer::DrawTexturedRect(const ::Rectangle& rect, const TextureHandle& texture,
                                                       const ::Color& tint)
{
	SubmitTexturedQuad(rect.x, rect.y, rect.width, rect.height, 0.0f, 0.0f, 1.0f, 1.0f, PackColorRGBA(tint),
	                    bgfx::TextureHandle{(uint16_t)texture.id});
}

void Struktur::Renderer::UIRenderer::DrawText(const ::Font& font, const std::string& text, const ::Vector2& position,
                                               float fontSize, const ::Color& color)
{
	// Stub/unloaded font (see FontResource) - draw nothing rather than divide by zero below.
	if (font.baseSize <= 0 || font.glyphCount <= 0)
	{
		return;
	}

	float scale               = fontSize / (float)font.baseSize;
	float atlasWidth          = (float)font.texture.width;
	float atlasHeight         = (float)font.texture.height;
	uint32_t abgr             = PackColorRGBA(color);
	bgfx::TextureHandle atlas = {(uint16_t)font.texture.id};

	float penX = position.x;
	int index  = 0;
	int size   = (int)text.size();
	while (index < size)
	{
		int codepointByteCount = 0;
		int codepoint          = ::GetCodepointNext(&text[index], &codepointByteCount);
		int glyphIndex         = ::GetGlyphIndex(font, codepoint);
		index += codepointByteCount;

		const ::GlyphInfo& glyph = font.glyphs[glyphIndex];
		const ::Rectangle& rec   = font.recs[glyphIndex];

		if (codepoint != ' ' && codepoint != '\t')
		{
			float dstX = penX + (float)glyph.offsetX * scale;
			float dstY = position.y + (float)glyph.offsetY * scale;
			SubmitTexturedQuad(dstX, dstY, rec.width * scale, rec.height * scale, rec.x / atlasWidth,
			                   rec.y / atlasHeight, (rec.x + rec.width) / atlasWidth,
			                   (rec.y + rec.height) / atlasHeight, abgr, atlas);
		}

		penX += (float)(glyph.advanceX > 0 ? glyph.advanceX : (int)rec.width) * scale;
	}
}

#endif
