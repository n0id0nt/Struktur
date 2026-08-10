#include "UILabel.h"

#include "Engine/GameContext.h"
#if !defined(PLATFORM_WEB)
	#include "Engine/Renderer/UIRenderer.h"
#endif

Struktur::UI::UILabel::UILabel(GameContext& context, const glm::vec2& absolutePosition,
                               const glm::vec2& relativePosition, const std::string& labelText, float fontSz)
    : UIElement(absolutePosition, relativePosition, {0, 0}, {0, 0}),
      m_text(labelText),
      m_textColor(BLACK),
      m_alignment(TextAlignment::LEFT),
      m_wrapping(TextWrapping::NONE),
      m_fontSize(fontSz)
{
	m_font = context.GetResourceManager().GetFont(context, "default", 32);

	// Auto-size based on text
	//::Vector2 textSize = ::MeasureTextEx(m_font->font, m_text.c_str(), m_fontSize, 1.0f);
	// SetSize({textSize.x + 10, textSize.y + 5}, {0, 0}); // Add some padding

	// Labels are typically not focusable
	m_focusable       = false;
	m_backgroundColor = BLANK;  // Transparent by default
	m_borderWidth     = 0.0f;
}

void Struktur::UI::UILabel::SetText(const std::string& newText)
{
	m_text = newText;
	// Recalculate size
	// SetBoundingBoxToText();
}

void Struktur::UI::UILabel::SetBoundingBoxToText()
{
	::Vector2 size = GetFormattedTextSize();

	SetSize({size.x, size.y}, {0, 0});
}

void Struktur::UI::UILabel::Update(GameContext& context)
{
	UpdateChildren(context);
}

void Struktur::UI::UILabel::Render(GameContext& context)
{
#if defined(PLATFORM_WEB)
	// Draw background if not transparent
	if (m_backgroundColor.a > 0)
	{
		::DrawRectangleRec(m_bounds, m_backgroundColor);
	}

	// Draw border if needed
	if (m_borderWidth > 0)
	{
		::DrawRectangleLinesEx(m_bounds, m_borderWidth, m_borderColor);
	}
#else
	if (m_backgroundColor.a > 0)
	{
		context.GetUIRenderer().DrawRect(m_bounds, m_backgroundColor);
	}
	if (m_borderWidth > 0)
	{
		context.GetUIRenderer().DrawRectOutline(m_bounds, m_borderWidth, m_borderColor);
	}
#endif

	// Calculate line height
	float lineHeight = GetLineHeight();

	// Calculate starting position
	::Vector2 startPos = {m_bounds.x + 5, m_bounds.y + 2.5f};

	// Render text with wrapping support - DrawGlyphs (called inside) handles the platform split now.
	RenderText(context, m_text, startPos, lineHeight);

	RenderChildren(context);
}

::Vector2 Struktur::UI::UILabel::MeasureText(const ::Font& font, const std::string& text, float fontSize)
{
	// GPU-independent on both platforms - see FontResource for why this is safe on desktop too.
	return ::MeasureTextEx(font, text.c_str(), fontSize, 1.0f);
}

void Struktur::UI::UILabel::DrawGlyphs(GameContext& context, const std::string& text, ::Vector2 pos) const
{
#if defined(PLATFORM_WEB)
	::DrawTextEx(m_font->font, text.c_str(), pos, m_fontSize, 1.0f, m_textColor);
#else
	// LoadFromDisk only builds the CPU-side atlas/metrics (see FontResource) - the bgfx texture upload is
	// deferred until first actually needed here, same lazy-GPU-upload pattern SpriteRenderSystem/UIPanel use
	// for TextureResource.
	if (!m_font->IsGpuReady())
	{
		m_font->LoadToGpu(context);
	}
	context.GetUIRenderer().DrawText(m_font->font, text, pos, m_fontSize, m_textColor);
#endif
}

std::vector<std::string> Struktur::UI::UILabel::GetTextLines(const std::string& text) const
{
	if (m_wrapping != TextWrapping::NONE)
	{
		float maxWidth = m_bounds.width - 10;  // 5px padding on each side
		return WrapText(text, maxWidth);
	}

	std::vector<std::string> lines;

	// Split text by newlines
	size_t start = 0;
	size_t end   = 0;

	while (end != std::string::npos)
	{
		end = text.find('\n', start);

		// Get the substring for this line
		std::string line = (end == std::string::npos) ? text.substr(start) : text.substr(start, end - start);

		lines.push_back(line);
		start = end + 1;
	}

	return lines;
}

std::vector<std::string> Struktur::UI::UILabel::WrapText(const std::string& text, float maxWidth) const
{
	std::vector<std::string> lines;

	std::string currentLine;
	std::string word;

	for (size_t i = 0; i < text.length(); ++i)
	{
		char c = text[i];

		// Handle newlines
		if (c == '\n')
		{
			if (!word.empty())
			{
				currentLine += word;
				word.clear();
			}
			lines.push_back(currentLine);
			currentLine.clear();
			continue;
		}

		if (m_wrapping == TextWrapping::WORD_WRAP)
		{
			// Word wrap mode
			if (c == ' ' || c == '\t')
			{
				std::string testLine = currentLine + word + c;
				::Vector2 size       = MeasureText(m_font->font, testLine, m_fontSize);

				if (size.x > maxWidth && !currentLine.empty())
				{
					lines.push_back(currentLine);
					currentLine = word + c;
				}
				else
				{
					currentLine = testLine;
				}
				word.clear();
			}
			else
			{
				word += c;
			}
		}
		else if (m_wrapping == TextWrapping::CHARACTER_WRAP)
		{
			// Character wrap mode
			std::string testLine = currentLine + c;
			::Vector2 size       = MeasureText(m_font->font, testLine, m_fontSize);

			if (size.x > maxWidth && !currentLine.empty())
			{
				lines.push_back(currentLine);
				currentLine = std::string(1, c);
			}
			else
			{
				currentLine += c;
			}
		}
	}

	// Add remaining content
	if (!word.empty())
	{
		currentLine += word;
	}
	if (!currentLine.empty())
	{
		lines.push_back(currentLine);
	}

	return lines;
}

void Struktur::UI::UILabel::RenderJustifiedLine(GameContext& context, const std::string& line, ::Vector2 pos,
                                                float targetWidth, bool isLastLine)
{
	// Don't justify last line or lines with only one word
	if (isLastLine || line.find(' ') == std::string::npos)
	{
		DrawGlyphs(context, line, pos);
		return;
	}

	// Count spaces and words
	std::vector<std::string> words;
	std::string currentWord;

	for (char c : line)
	{
		if (c == ' ' || c == '\t')
		{
			if (!currentWord.empty())
			{
				words.push_back(currentWord);
				currentWord.clear();
			}
		}
		else
		{
			currentWord += c;
		}
	}
	if (!currentWord.empty())
	{
		words.push_back(currentWord);
	}

	if (words.size() <= 1)
	{
		DrawGlyphs(context, line, pos);
		return;
	}

	// Calculate total word width
	float totalWordWidth = 0;
	for (const auto& word : words)
	{
		::Vector2 wordSize = MeasureText(m_font->font, word, m_fontSize);
		totalWordWidth += wordSize.x;
	}

	// Calculate space between words
	float totalSpaceWidth = targetWidth - totalWordWidth;
	float spaceWidth      = totalSpaceWidth / (words.size() - 1);

	// Draw words with calculated spacing
	float currentX = pos.x;
	for (const auto& word : words)
	{
		::Vector2 wordPos = {currentX, pos.y};
		DrawGlyphs(context, word, wordPos);

		::Vector2 wordSize = MeasureText(m_font->font, word, m_fontSize);
		currentX += wordSize.x + spaceWidth;
	}
}

void Struktur::UI::UILabel::RenderText(GameContext& context, const std::string& text, ::Vector2 startPos,
                                       float lineHeight)
{
	std::vector<std::string> lines = GetTextLines(text);

	float currentY = startPos.y;

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string& line = lines[i];
		::Vector2 textSize      = MeasureText(m_font->font, line, m_fontSize);
		::Vector2 textPos       = {startPos.x, currentY};

		switch (m_alignment)
		{
			case TextAlignment::CENTER:
				textPos.x = m_bounds.x + (m_bounds.width - textSize.x) / 2.0f;
				DrawGlyphs(context, line, textPos);
				break;

			case TextAlignment::RIGHT:
				textPos.x = m_bounds.x + m_bounds.width - textSize.x - 5;
				DrawGlyphs(context, line, textPos);
				break;

			case TextAlignment::JUSTIFY:
			{
				textPos.x       = m_bounds.x + 5;
				bool isLastLine = (i == lines.size() - 1);
				RenderJustifiedLine(context, line, textPos, m_bounds.x - 10.0f, isLastLine);
				break;
			}

			case TextAlignment::LEFT:
			default:
				textPos.x = m_bounds.x + 5;
				DrawGlyphs(context, line, textPos);
				break;
		}

		currentY += lineHeight;
	}
}

::Rectangle Struktur::UI::UILabel::GetFormattedTextBounds() const
{
	::Vector2 size = GetFormattedTextSize();

	// The bounds start at the label's position
	return {m_bounds.x, m_bounds.y, size.x, size.y};
}

::Vector2 Struktur::UI::UILabel::GetFormattedTextSize() const
{
	if (m_text.empty())
	{
		return {0, 0};
	}

	std::vector<std::string> lines = GetTextLines(m_text);

	float totalWidth  = 0;
	float totalHeight = 0;
	float lineHeight  = GetLineHeight();

	for (const auto& line : lines)
	{
		::Vector2 lineSize = MeasureText(m_font->font, line, m_fontSize);

		// For justified text, use the full available width (except last line)
		if (m_alignment == TextAlignment::JUSTIFY && &line != &lines.back())
		{
			totalWidth = std::max(totalWidth, m_bounds.width - 10);
		}
		else
		{
			totalWidth = std::max(totalWidth, lineSize.x);
		}

		totalHeight += lineHeight;
	}

	// Add padding
	totalWidth += 10;  // 5px on each side
	totalHeight += 5;  // 2.5px top + 2.5px bottom

	return {totalWidth, totalHeight};
}

float Struktur::UI::UILabel::GetLineHeight() const
{
	return m_fontSize * 1.5f;  // 1.5x line spacing for readability
}
