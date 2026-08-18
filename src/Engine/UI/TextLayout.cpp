#include "TextLayout.h"

#include <algorithm>

std::vector<std::string> Struktur::UI::TextLayout::WrapText(const std::string& text, float maxWidth,
                                                             TextWrapping mode,
                                                             const std::function<float(const std::string&)>& measureWidth)
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

		if (mode == TextWrapping::WORD_WRAP)
		{
			// Word wrap mode
			if (c == ' ' || c == '\t')
			{
				std::string testLine = currentLine + word + c;
				float width           = measureWidth(testLine);

				if (width > maxWidth && !currentLine.empty())
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
		else if (mode == TextWrapping::CHARACTER_WRAP)
		{
			// Character wrap mode
			std::string testLine = currentLine + c;
			float width           = measureWidth(testLine);

			if (width > maxWidth && !currentLine.empty())
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

std::vector<std::string> Struktur::UI::TextLayout::SplitWords(const std::string& line)
{
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
	return words;
}

float Struktur::UI::TextLayout::ComputeAlignedStartX(TextAlignment alignment, float boxX, float boxWidth,
                                                      float lineWidth, float padding)
{
	switch (alignment)
	{
		case TextAlignment::CENTER:
			return boxX + (boxWidth - lineWidth) / 2.0f;
		case TextAlignment::RIGHT:
			return boxX + boxWidth - lineWidth - padding;
		case TextAlignment::JUSTIFY:
		case TextAlignment::LEFT:
		default:
			return boxX + padding;
	}
}

float Struktur::UI::TextLayout::ComputeJustifySpaceWidth(float targetWidth, float totalWordWidth, size_t wordCount)
{
	float totalSpaceWidth = targetWidth - totalWordWidth;
	return totalSpaceWidth / (float)(wordCount - 1);
}

glm::vec2 Struktur::UI::TextLayout::SumLineBounds(const std::vector<float>& lineWidths,
                                                  const std::vector<float>& lineHeights, float paddingX,
                                                  float paddingY)
{
	float totalWidth  = 0.0f;
	float totalHeight = 0.0f;
	for (float width : lineWidths)
	{
		totalWidth = std::max(totalWidth, width);
	}
	for (float height : lineHeights)
	{
		totalHeight += height;
	}
	totalWidth += paddingX;
	totalHeight += paddingY;
	return {totalWidth, totalHeight};
}

std::vector<Struktur::UI::TextLayout::WrapToken> Struktur::UI::TextLayout::Tokenize(
    const std::string& text, TextWrapping mode, int styleId, const std::function<float(const std::string&)>& measureWidth)
{
	std::vector<WrapToken> tokens;

	if (mode == TextWrapping::CHARACTER_WRAP)
	{
		for (size_t i = 0; i < text.size(); ++i)
		{
			std::string ch(1, text[i]);
			tokens.push_back(WrapToken{ch, measureWidth(ch), styleId});
		}
		return tokens;
	}

	// WORD_WRAP (also used for TextWrapping::NONE callers that still want tokens for some other reason - NONE
	// itself never reaches here in practice, see UIRichLabel::BuildRenderLines).
	std::string word;
	for (char c : text)
	{
		word += c;
		if (c == ' ' || c == '\t')
		{
			tokens.push_back(WrapToken{word, measureWidth(word), styleId});
			word.clear();
		}
	}
	if (!word.empty())
	{
		tokens.push_back(WrapToken{word, measureWidth(word), styleId});
	}
	return tokens;
}

std::vector<std::vector<Struktur::UI::TextLayout::WrapToken>> Struktur::UI::TextLayout::WrapTokens(
    const std::vector<WrapToken>& tokens, float maxWidth)
{
	std::vector<std::vector<WrapToken>> lines;
	std::vector<WrapToken> currentLine;
	float currentWidth = 0.0f;

	for (const WrapToken& token : tokens)
	{
		if (currentWidth + token.width > maxWidth && !currentLine.empty())
		{
			lines.push_back(std::move(currentLine));
			currentLine.clear();
			currentWidth = 0.0f;
		}
		currentLine.push_back(token);
		currentWidth += token.width;
	}
	if (!currentLine.empty())
	{
		lines.push_back(std::move(currentLine));
	}
	return lines;
}
