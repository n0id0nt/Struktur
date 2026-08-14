#pragma once

#include "Engine/Resource/FontResource.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Text/Font.h"
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
	Util::Color m_textColor;
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
	void SetTextColor(Util::Color color)
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
	void RenderJustifiedLine(GameContext& context, const std::string& line, glm::vec2 pos, float targetWidth,
	                         bool isLastLine);
	void RenderText(GameContext& context, const std::string& text, glm::vec2 startPos, float lineHeight);

	Util::Math::Rect GetFormattedTextBounds() const;
	glm::vec2 GetFormattedTextSize() const;
	float GetLineHeight() const;

   private:
	// FontResource populates real glyph metrics (baseSize/glyphs) on both platforms, so Text::MeasureTextEx
	// (GPU-independent - see FontResource) works everywhere; centralising it here just keeps every layout call
	// site (wrapping, justification, bounding box) going through one place.
	static glm::vec2 MeasureText(const Text::Font& font, const std::string& text, float fontSize);

	// Web draws via raylib's DrawTextEx; desktop submits glyph quads through UIRenderer (see FontResource for
	// why this needs no platform split for measurement, only for the actual draw).
	void DrawGlyphs(GameContext& context, const std::string& text, glm::vec2 pos) const;
};
}  // namespace UI
}  // namespace Struktur
