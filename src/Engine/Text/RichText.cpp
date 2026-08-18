#include "RichText.h"

#include <cstdint>
#include <cstdlib>

namespace
{
std::string Trim(const std::string& s)
{
	size_t begin = s.find_first_not_of(" \t");
	if (begin == std::string::npos)
	{
		return "";
	}
	size_t end = s.find_last_not_of(" \t");
	return s.substr(begin, end - begin + 1);
}

bool IsHexDigit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

uint8_t HexByte(const std::string& digits, size_t offset)
{
	return (uint8_t)std::strtoul(digits.substr(offset, 2).c_str(), nullptr, 16);
}
}  // namespace

bool Struktur::Text::TryParseHexColor(const std::string& hex, Util::Color& outColor)
{
	if (hex.size() < 2 || hex[0] != '#')
	{
		return false;
	}
	std::string digits = hex.substr(1);
	if (digits.size() != 6 && digits.size() != 8)
	{
		return false;
	}
	for (char c : digits)
	{
		if (!IsHexDigit(c))
		{
			return false;
		}
	}

	outColor.r = HexByte(digits, 0);
	outColor.g = HexByte(digits, 2);
	outColor.b = HexByte(digits, 4);
	outColor.a = digits.size() == 8 ? HexByte(digits, 6) : 255;
	return true;
}

namespace
{
// Mutable parse state threaded through the scan loop in ParseMarkup - kept as one struct rather than a pile
// of loose locals since every tag boundary needs to read/mutate several of these together.
struct ParseState
{
	std::vector<Struktur::Text::RichLine> lines;
	Struktur::Text::RichLine currentLine;
	std::string currentText;
	int boldDepth   = 0;
	int italicDepth = 0;
	// Always at least 1 entry (ParseMarkup seeds it with baseColor) - a stray [/color] past that floor is
	// treated as a mismatched close (see ParseMarkup) rather than popping past the base color.
	std::vector<Struktur::Util::Color> colorStack;
};

// Pushes currentText as a new run (styled per the *current* state) onto currentLine, then clears it - a no-op
// if currentText is empty, so back-to-back tags with no text between them don't create an empty run.
void FlushTextRun(ParseState& state)
{
	if (state.currentText.empty())
	{
		return;
	}
	Struktur::Text::RichRun run;
	run.text   = state.currentText;
	run.bold   = state.boldDepth > 0;
	run.italic = state.italicDepth > 0;
	run.color  = state.colorStack.back();
	state.currentLine.runs.push_back(std::move(run));
	state.currentText.clear();
}

void FlushLine(ParseState& state)
{
	FlushTextRun(state);
	state.lines.push_back(std::move(state.currentLine));
	state.currentLine = Struktur::Text::RichLine{};
}

// Appends a tag's original source text (including its brackets) as literal content - the fallback for
// anything that doesn't parse as a recognised, balanced tag (see ParseMarkup's leniency comment in
// RichText.h) so a malformed/mistyped/unknown tag stays visible rather than silently vanishing.
void AppendLiteralTag(ParseState& state, const std::string& tagContent)
{
	state.currentText += '[';
	state.currentText += tagContent;
	state.currentText += ']';
}
}  // namespace

std::vector<Struktur::Text::RichLine> Struktur::Text::ParseMarkup(const std::string& markup, Util::Color baseColor)
{
	ParseState state;
	state.colorStack.push_back(baseColor);

	size_t i      = 0;
	size_t length = markup.size();
	while (i < length)
	{
		char c = markup[i];

		if (c == '\n')
		{
			FlushLine(state);
			++i;
			continue;
		}

		if (c != '[')
		{
			state.currentText += c;
			++i;
			continue;
		}

		size_t closeBracket = markup.find(']', i + 1);
		if (closeBracket == std::string::npos)
		{
			// No ']' left anywhere in the rest of the string - there's no valid tag to find, so the remaining
			// "[..." is literal text and there's nothing further to scan.
			state.currentText += markup.substr(i);
			break;
		}

		std::string tagContent = markup.substr(i + 1, closeBracket - i - 1);
		i                      = closeBracket + 1;

		if (tagContent.empty())
		{
			AppendLiteralTag(state, tagContent);  // "[]" - nothing to interpret.
			continue;
		}

		if (tagContent[0] == '/')
		{
			std::string name = Trim(tagContent.substr(1));
			if (name == "b" && state.boldDepth > 0)
			{
				FlushTextRun(state);
				--state.boldDepth;
			}
			else if (name == "i" && state.italicDepth > 0)
			{
				FlushTextRun(state);
				--state.italicDepth;
			}
			else if (name == "color" && state.colorStack.size() > 1)
			{
				FlushTextRun(state);
				state.colorStack.pop_back();
			}
			else
			{
				// Stray/mismatched close (e.g. [/b] with no open [b]) or an unrecognised tag name - shown
				// verbatim so a writer notices the mistake instead of it silently vanishing.
				AppendLiteralTag(state, tagContent);
			}
			continue;
		}

		size_t equals    = tagContent.find('=');
		std::string name = Trim(equals == std::string::npos ? tagContent : tagContent.substr(0, equals));
		std::string arg  = equals == std::string::npos ? "" : Trim(tagContent.substr(equals + 1));

		if (name == "b")
		{
			FlushTextRun(state);
			++state.boldDepth;
		}
		else if (name == "i")
		{
			FlushTextRun(state);
			++state.italicDepth;
		}
		else if (name == "color")
		{
			Util::Color parsed{};
			if (TryParseHexColor(arg, parsed))
			{
				FlushTextRun(state);
				state.colorStack.push_back(parsed);
			}
			else
			{
				AppendLiteralTag(state, tagContent);
			}
		}
		else if (name == "icon")
		{
			FlushTextRun(state);
			RichRun iconRun;
			iconRun.bold     = state.boldDepth > 0;
			iconRun.italic   = state.italicDepth > 0;
			iconRun.color    = state.colorStack.back();
			iconRun.iconName = arg;
			state.currentLine.runs.push_back(std::move(iconRun));
		}
		else
		{
			// Unrecognised tag name - shown verbatim rather than silently swallowed, so unsupported/future/
			// mistyped tags stay visible instead of disappearing.
			AppendLiteralTag(state, tagContent);
		}
	}

	FlushLine(state);
	return state.lines;
}
