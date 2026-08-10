#include "Font.h"

#include <algorithm>

int Struktur::Text::GetCodepointNext(const char* text, int* codepointSize)
{
	const unsigned char* bytes = (const unsigned char*)text;

	if (bytes[0] < 0x80)
	{
		*codepointSize = 1;
		return bytes[0];
	}
	if ((bytes[0] & 0xE0) == 0xC0 && (bytes[1] & 0xC0) == 0x80)
	{
		*codepointSize = 2;
		return ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
	}
	if ((bytes[0] & 0xF0) == 0xE0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80)
	{
		*codepointSize = 3;
		return ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
	}
	if ((bytes[0] & 0xF8) == 0xF0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80 &&
	    (bytes[3] & 0xC0) == 0x80)
	{
		*codepointSize = 4;
		return ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
	}

	// Invalid/truncated sequence - matches raylib's own fallback (consume 1 byte, report '?').
	*codepointSize = 1;
	return '?';
}

int Struktur::Text::GetGlyphIndex(const Font& font, int codepoint)
{
	for (size_t i = 0; i < font.glyphs.size(); ++i)
	{
		if (font.glyphs[i].value == codepoint)
		{
			return (int)i;
		}
	}
	// Fall back to the '?' glyph (matches raylib's own behaviour for an unmapped codepoint), else index 0.
	for (size_t i = 0; i < font.glyphs.size(); ++i)
	{
		if (font.glyphs[i].value == '?')
		{
			return (int)i;
		}
	}
	return 0;
}

glm::vec2 Struktur::Text::MeasureTextEx(const Font& font, const std::string& text, float fontSize, float spacing)
{
	if (font.baseSize <= 0 || font.glyphs.empty())
	{
		return glm::vec2(0.0f, 0.0f);
	}

	float scaleFactor    = fontSize / (float)font.baseSize;
	float textWidth      = 0.0f;
	float tempTextWidth  = 0.0f;
	float textHeight     = (float)font.baseSize;
	int byteCounter      = 0;
	int tempByteCounter  = 0;

	int index = 0;
	int size  = (int)text.size();
	while (index < size)
	{
		byteCounter++;
		int codepointSize = 0;
		int codepoint      = GetCodepointNext(&text[index], &codepointSize);
		int glyphIndex     = GetGlyphIndex(font, codepoint);
		index += codepointSize;

		if (codepoint != '\n')
		{
			const Glyph& glyph = font.glyphs[glyphIndex];
			if (glyph.advanceX != 0)
			{
				textWidth += (float)glyph.advanceX;
			}
			else
			{
				textWidth += glyph.rec.width + (float)glyph.offsetX;
			}
		}
		else
		{
			tempTextWidth = std::max(tempTextWidth, textWidth);
			byteCounter   = 0;
			textWidth     = 0.0f;
			textHeight += (float)font.baseSize;
		}

		tempByteCounter = std::max(tempByteCounter, byteCounter);
	}

	tempTextWidth = std::max(tempTextWidth, textWidth);

	return glm::vec2(tempTextWidth * scaleFactor + (float)(tempByteCounter - 1) * spacing, textHeight * scaleFactor);
}
