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

	// 1 quad for the background + 4 for the border (mirroring Render(), same as UIPanel) plus one quad per
	// non-space/tab/newline codepoint in m_text - wrapping/justification only change where line breaks land,
	// not how many glyphs actually get drawn, so counting over the raw (unwrapped) text is a correct total.
	uint32_t GetRequiredQuadCount() const override;

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

	// Batched counterparts of DrawGlyphs/RenderJustifiedLine/RenderText above (mirroring their structure exactly,
	// just writing into m_batch instead of submitting immediately) - used by Render() when m_visualDirty. Each
	// writes into a sub-slot of m_batchSlot offset by quadOffset quads (background/border occupy the slot's
	// first quads - see Render() - so text always starts after those) and returns the quad count it actually
	// wrote, which the caller adds to quadOffset before the next Write* call into the same slot.
	uint32_t WriteGlyphs(GameContext& context, const std::string& text, glm::vec2 pos, uint32_t quadOffset);
	uint32_t WriteJustifiedLine(GameContext& context, const std::string& line, glm::vec2 pos, float targetWidth,
	                            bool isLastLine, uint32_t quadOffset);
	uint32_t WriteTextLines(GameContext& context, const std::string& text, glm::vec2 startPos, float lineHeight,
	                        uint32_t quadOffset);
};
}  // namespace UI
}  // namespace Struktur
