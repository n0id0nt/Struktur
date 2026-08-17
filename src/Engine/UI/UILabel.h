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
		m_font        = std::move(newFont);
		m_visualDirty = true;
	}
	void SetTextColor(Util::Color color)
	{
		m_textColor   = color;
		m_visualDirty = true;
	}
	void SetAlignment(TextAlignment align)
	{
		m_alignment   = align;
		m_visualDirty = true;
	}
	void SetFontSize(float size)
	{
		m_fontSize    = size;
		m_visualDirty = true;
	}
	void SetWordWrap(TextWrapping wrap)
	{
		m_wrapping    = wrap;
		m_visualDirty = true;
	}
	void SetBoundingBoxToText();

	const std::string& GetText() const
	{
		return m_text;
	}

	void Update(GameContext& context) override;
	void Render(GameContext& context) override;

	// One quad per non-space/tab/newline codepoint in m_text - wrapping/justification only change where line
	// breaks land, not how many glyphs actually get drawn, so counting over the raw (unwrapped) text is a
	// correct total. Pure text, no background/border quads (see UIPanel/UIColor/UIBorder for those).
	uint32_t GetRequiredQuadCount() const override;

	std::vector<std::string> GetTextLines(const std::string& text) const;
	std::vector<std::string> WrapText(const std::string& text, float maxWidth) const;

	Util::Math::Rect GetFormattedTextBounds() const;
	glm::vec2 GetFormattedTextSize() const;
	float GetLineHeight() const;

private:
	// FontResource populates real glyph metrics (baseSize/glyphs) on both platforms, so Text::MeasureTextEx
	// (GPU-independent - see FontResource) works everywhere; centralising it here just keeps every layout call
	// site (wrapping, justification, bounding box) going through one place.
	static glm::vec2 MeasureText(const Text::Font& font, const std::string& text, float fontSize);

	// Writes into a sub-slot of m_batchSlot offset by quadOffset quads (always 0 from Render() itself - kept as a
	// parameter since these three call each other recursively while accumulating how many quads they've each
	// used) and returns the quad count it actually wrote, which the caller adds to quadOffset before the next of
	// these called into the same slot.
	uint32_t RenderGlyphs(GameContext& context, const std::string& text, glm::vec2 pos, uint32_t quadOffset);
	uint32_t RenderJustifiedLine(GameContext& context, const std::string& line, glm::vec2 pos, float targetWidth,
	                             bool isLastLine, uint32_t quadOffset);
	uint32_t RenderTextLines(GameContext& context, const std::string& text, glm::vec2 startPos, float lineHeight,
	                         uint32_t quadOffset);
};
}  // namespace UI
}  // namespace Struktur
