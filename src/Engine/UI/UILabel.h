#pragma once

#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/ResourcePtr.h"
#include "Engine/UI/UIElement.h"

namespace Struktur
{
class GameContext;

namespace UI
{
enum class TextAlignment
{
	LEFT,
	CENTER,
	RIGHT,
	JUSTIFY
};

enum class TextWrapping
{
	NONE,
	WORD_WRAP,
	CHARACTER_WRAP
};

//=============================================================================
// UILabel - Text display element
//=============================================================================
class UILabel : public UIElement
{
   private:
	std::string m_text;
	Resource::ResourcePtr<Resource::FontResource> m_font;
	::Color m_textColor;
	TextAlignment m_alignment;
	TextWrapping m_wrapping;
	float m_fontSize;

   public:
	UILabel(GameContext& context, const glm::vec2& absolutePosition, const glm::vec2& relativePosition,
	        const std::string& labelText, float fontSz = 20.0f);

	void SetText(const std::string& newText);

	void SetFont(Resource::ResourcePtr<Resource::FontResource> newFont)
	{
		m_font = std::move(newFont);
	}
	void SetTextColor(::Color color)
	{
		m_textColor = color;
	}
	void SetAlignment(TextAlignment align)
	{
		m_alignment = align;
	}
	void SetFontSize(float size)
	{
		m_fontSize = size;
	}
	void SetWordWrap(TextWrapping wrap)
	{
		m_wrapping = wrap;
	}
	void SetBoundingBoxToText();

	const std::string& GetText() const
	{
		return m_text;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	std::vector<std::string> GetTextLines(const std::string& text) const;
	std::vector<std::string> WrapText(const std::string& text, float maxWidth) const;
	void RenderJustifiedLine(GameContext& context, const std::string& line, ::Vector2 pos, float targetWidth,
	                         bool isLastLine);
	void RenderText(GameContext& context, const std::string& text, ::Vector2 startPos, float lineHeight);

	::Rectangle GetFormattedTextBounds() const;
	::Vector2 GetFormattedTextSize() const;
	float GetLineHeight() const;

   private:
	// FontResource populates real glyph metrics (baseSize/glyphs/recs) on both platforms now, so raylib's own
	// MeasureTextEx (GPU-independent - see FontResource) works everywhere; centralising it here just keeps
	// every layout call site (wrapping, justification, bounding box) going through one place.
	static ::Vector2 MeasureText(const ::Font& font, const std::string& text, float fontSize);

	// Web draws via raylib's DrawTextEx; desktop submits glyph quads through UIRenderer (see FontResource for
	// why this needs no platform split for measurement, only for the actual draw).
	void DrawGlyphs(GameContext& context, const std::string& text, ::Vector2 pos) const;
};
}  // namespace UI
}  // namespace Struktur
