#include "UIInputHint.h"

#include <algorithm>
#include <cmath>
#include <tuple>

#include "Engine/GameContext.h"
#include "Engine/Renderer/UIRenderer.h"
#include "Engine/Text/Font.h"

Struktur::UI::UIInputHint::UIInputHint(GameContext& context, const glm::vec2& absolutePosition,
                                       const glm::vec2& relativePosition, const std::string& action)
    : UIElement(absolutePosition, relativePosition, {0, 0}, {0, 0})
{
	m_font      = context.GetResourceManager().GetFont(context, "default", 32);
	m_focusable = false;
	SetAction(context, action);
}

void Struktur::UI::UIInputHint::SetAction(GameContext& context, const std::string& action)
{
	std::tie(m_action, m_component) = InputGlyphs::ParseActionSpec(action);
	Refresh(context);
}

void Struktur::UI::UIInputHint::SetLabel(GameContext& context, const std::string& label)
{
	m_label = label;
	Refresh(context);
}

void Struktur::UI::UIInputHint::SetFont(GameContext& context, Resource::ResourcePtr<Resource::FontResource> font)
{
	m_font = std::move(font);
	Refresh(context);
}

void Struktur::UI::UIInputHint::SetFontSize(GameContext& context, float fontSize)
{
	m_fontSize = fontSize;
	Refresh(context);
}

void Struktur::UI::UIInputHint::SetGlyphScale(GameContext& context, int scale)
{
	m_glyphScale = std::max(1, scale);
	Refresh(context);
}

void Struktur::UI::UIInputHint::SetSpacing(GameContext& context, float glyphGap, float labelGap)
{
	m_glyphGap = glyphGap;
	m_labelGap = labelGap;
	Refresh(context);
}

float Struktur::UI::UIInputHint::GlyphWidth(const InputGlyph& glyph) const
{
	if (glyph.HasTexture())
	{
		return glyph.pixelSize.x * (float)m_glyphScale;
	}
	return m_font ? Text::MeasureTextEx(m_font->font, FallbackLabel(glyph), m_fontSize, 1.0f).x : 0.0f;
}

void Struktur::UI::UIInputHint::Refresh(GameContext& context)
{
	m_glyphs       = context.GetInputGlyphs().Resolve(context, m_action, m_component);
	m_seenRevision = context.GetInput().GetHintRevision();

	// Width: glyphs side by side, then the label. Height: the tallest glyph, or the text if that's taller.
	float width  = 0.0f;
	float height = 0.0f;
	float textHeight =
	    m_font ? Text::MeasureTextEx(m_font->font, m_label.empty() ? "A" : m_label, m_fontSize, 1.0f).y : 0.0f;
	for (size_t i = 0; i < m_glyphs.size(); ++i)
	{
		width += GlyphWidth(m_glyphs[i]) + (i > 0 ? m_glyphGap : 0.0f);
		height = std::max(height, m_glyphs[i].HasTexture() ? m_glyphs[i].pixelSize.y * (float)m_glyphScale
		                                                   : textHeight);
	}
	if (!m_label.empty() && m_font)
	{
		width += (m_glyphs.empty() ? 0.0f : m_labelGap) + Text::MeasureTextEx(m_font->font, m_label, m_fontSize, 1.0f).x;
		height = std::max(height, textHeight);
	}
	SetSize({std::ceil(width), std::ceil(height)}, {0, 0});
	m_visualDirty = true;
}

uint32_t Struktur::UI::UIInputHint::CountTextQuads(const std::string& text)
{
	// Same rule as UIRenderer::DrawText: one quad per codepoint except space/tab.
	uint32_t quads = 0;
	int index      = 0;
	int size       = (int)text.size();
	while (index < size)
	{
		int byteCount = 0;
		int codepoint = Text::GetCodepointNext(&text[index], &byteCount);
		index += byteCount;
		if (codepoint != ' ' && codepoint != '\t')
		{
			++quads;
		}
	}
	return quads;
}

uint32_t Struktur::UI::UIInputHint::GetRequiredQuadCount() const
{
	uint32_t quads = CountTextQuads(m_label);
	for (const InputGlyph& glyph : m_glyphs)
	{
		quads += glyph.HasTexture() ? 1 : CountTextQuads(FallbackLabel(glyph));
	}
	return quads;
}

void Struktur::UI::UIInputHint::Update(GameContext& context)
{
	if (context.GetInput().GetHintRevision() != m_seenRevision)
	{
		Refresh(context);
	}
	UpdateChildren(context);
}

// The batch slot from its quadIndex-th quad on.
Struktur::Renderer::UIBatchSlot Struktur::UI::UIInputHint::SubSlot(uint32_t quadIndex) const
{
	Renderer::UIBatchSlot slot = m_batchSlot;
	slot.vertexOffset += quadIndex * 4;
	slot.quadCapacity -= std::min(quadIndex, slot.quadCapacity);
	return slot;
}

// Text at x, centred vertically in the hint, from the quadIndex-th quad on. Returns the quads it used.
uint32_t Struktur::UI::UIInputHint::DrawHintText(Renderer::UIRenderer& renderer, const std::string& text, float x,
                                                 uint32_t quadIndex)
{
	if (!m_font)
	{
		return 0;
	}
	float textHeight = Text::MeasureTextEx(m_font->font, text, m_fontSize, 1.0f).y;
	float y          = std::floor(m_bounds.y) + std::floor((m_bounds.height - textHeight) * 0.5f);
	return renderer.DrawText(m_batch, SubSlot(quadIndex), m_font->font, text, {x, y}, m_fontSize, m_textColor,
	                         GetZIndex());
}

void Struktur::UI::UIInputHint::Render(GameContext& context)
{
	if (m_visualDirty)
	{
		Renderer::UIRenderer& renderer = context.GetUIRenderer();
		uint32_t requiredQuads         = GetRequiredQuadCount();
		if (m_batchSlot.quadCapacity < requiredQuads)
		{
			m_batchSlot = renderer.AllocateOrResizeSlot(m_batch, m_batchSlot, requiredQuads);
		}
		if (m_font && !m_font->IsGpuReady())
		{
			m_font->LoadToGpu(context);
		}

		// Whole pixels throughout, so scaled pixel art never lands between screen pixels.
		float x          = std::floor(m_bounds.x);
		float top        = std::floor(m_bounds.y);
		float height     = m_bounds.height;
		uint32_t used    = 0;

		for (size_t i = 0; i < m_glyphs.size(); ++i)
		{
			const InputGlyph& glyph = m_glyphs[i];
			if (i > 0)
			{
				x += m_glyphGap;
			}
			if (glyph.HasTexture())
			{
				Resource::TextureResource* texture = glyph.texture.Get();
				if (!texture->IsGpuReady())
				{
					texture->LoadToGpu(context);
				}
				glm::vec2 size = glyph.pixelSize * (float)m_glyphScale;
				float y        = top + std::floor((height - size.y) * 0.5f);
				renderer.DrawTexturedRect(m_batch, SubSlot(used), Util::Math::Rect{x, y, size.x, size.y},
				                          texture->GetHandle(), Util::ColorWhite, GetZIndex());
				++used;
			}
			else
			{
				used += DrawHintText(renderer, FallbackLabel(glyph), x, used);
			}
			x += GlyphWidth(glyph);
		}
		if (!m_label.empty())
		{
			used += DrawHintText(renderer, m_label, x + (m_glyphs.empty() ? 0.0f : m_labelGap), used);
		}

		// A device switch can leave fewer quads than last time (one D-pad picture instead of four keys).
		renderer.ClearSlotFrom(m_batch, m_batchSlot, used);
		m_visualDirty = false;
	}

	RenderChildren(context);
}
