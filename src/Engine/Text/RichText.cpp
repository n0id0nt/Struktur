#include "RichText.h"

#include <cstdint>
#include <cstdlib>
#include <map>

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

// Splits "key1=val1 key2=val2" (arbitrary whitespace between/around pairs) into a lookup - used by the
// [wave]/[shake]/[pulse] tags' multi-param argument lists, the first tags this parser has needed more than
// one value for. A malformed token with no '=' is simply skipped rather than erroring, and an unrecognised
// key is just absent from the result (GetFloatOr below then falls back to that param's default) - same
// leniency stance as the rest of this parser.
std::map<std::string, std::string> ParseKeyValuePairs(const std::string& text)
{
	std::map<std::string, std::string> result;
	size_t i      = 0;
	size_t length = text.size();
	while (i < length)
	{
		while (i < length && (text[i] == ' ' || text[i] == '\t'))
		{
			++i;
		}
		size_t tokenStart = i;
		while (i < length && text[i] != ' ' && text[i] != '\t')
		{
			++i;
		}
		if (i == tokenStart)
		{
			break;
		}
		std::string token = text.substr(tokenStart, i - tokenStart);
		size_t eq         = token.find('=');
		if (eq != std::string::npos)
		{
			result[Trim(token.substr(0, eq))] = Trim(token.substr(eq + 1));
		}
	}
	return result;
}

// Looks up key in params, parsing it as a float - returns fallback if the key is absent, or if present but
// not parseable as a number (strtof consumed zero characters).
float GetFloatOr(const std::map<std::string, std::string>& params, const std::string& key, float fallback)
{
	auto it = params.find(key);
	if (it == params.end())
	{
		return fallback;
	}
	char* end   = nullptr;
	float value = std::strtof(it->second.c_str(), &end);
	return (end != it->second.c_str()) ? value : fallback;
}

// One [wave]/[shake]/[pulse] tag's resolved params - kept as their own small structs (rather than reusing
// Text::RichRun's fields directly) so ParseState's per-effect stacks (below) can push/pop independently of
// how RichRun itself stores them.
struct WaveParams
{
	float amplitude = 20.0f;
	float frequency = 5.0f;
};
struct ShakeParams
{
	float rate  = 20.0f;
	float level = 3.0f;
};
struct PulseParams
{
	float frequency = 1.0f;
};
struct RainbowParams
{
	float frequency   = 1.0f;
	float saturation  = 0.8f;
	float value       = 0.8f;
};
struct TornadoParams
{
	float radius    = 10.0f;
	float frequency = 1.0f;
};
struct FadeParams
{
	float start  = 0.0f;
	float length = 8.0f;
};
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

	// Same depth-counter idiom as bold/italic above (so mismatched/stray closes degrade gracefully rather than
	// underflowing), plus a stack of each open tag's own resolved params - mirrors colorStack's "innermost
	// open wins, revert to the enclosing one's params on close" behavior, since [wave amp=10]...[wave
	// amp=50]...[/wave]...[/wave] should restore amp=10 after the inner close, not stay at 50.
	int waveDepth = 0, shakeDepth = 0, pulseDepth = 0, rainbowDepth = 0, tornadoDepth = 0, fadeDepth = 0;
	std::vector<WaveParams> waveStack;
	std::vector<ShakeParams> shakeStack;
	std::vector<PulseParams> pulseStack;
	std::vector<RainbowParams> rainbowStack;
	std::vector<TornadoParams> tornadoStack;
	std::vector<FadeParams> fadeStack;
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

	if (state.waveDepth > 0)
	{
		run.effectMask |= Struktur::Text::AnimationFlags::Wave;
		run.waveAmplitude = state.waveStack.back().amplitude;
		run.waveFrequency = state.waveStack.back().frequency;
	}
	if (state.shakeDepth > 0)
	{
		run.effectMask |= Struktur::Text::AnimationFlags::Shake;
		run.shakeRate  = state.shakeStack.back().rate;
		run.shakeLevel = state.shakeStack.back().level;
	}
	if (state.pulseDepth > 0)
	{
		run.effectMask |= Struktur::Text::AnimationFlags::Pulse;
		run.pulseFrequency = state.pulseStack.back().frequency;
	}
	if (state.rainbowDepth > 0)
	{
		run.effectMask |= Struktur::Text::AnimationFlags::Rainbow;
		run.rainbowFrequency  = state.rainbowStack.back().frequency;
		run.rainbowSaturation = state.rainbowStack.back().saturation;
		run.rainbowValue      = state.rainbowStack.back().value;
	}
	if (state.tornadoDepth > 0)
	{
		run.effectMask |= Struktur::Text::AnimationFlags::Tornado;
		run.tornadoRadius    = state.tornadoStack.back().radius;
		run.tornadoFrequency = state.tornadoStack.back().frequency;
	}
	if (state.fadeDepth > 0)
	{
		run.effectMask |= Struktur::Text::AnimationFlags::Fade;
		run.fadeStart  = state.fadeStack.back().start;
		run.fadeLength = state.fadeStack.back().length;
	}

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
			else if (name == "wave" && state.waveDepth > 0)
			{
				FlushTextRun(state);
				--state.waveDepth;
				state.waveStack.pop_back();
			}
			else if (name == "shake" && state.shakeDepth > 0)
			{
				FlushTextRun(state);
				--state.shakeDepth;
				state.shakeStack.pop_back();
			}
			else if (name == "pulse" && state.pulseDepth > 0)
			{
				FlushTextRun(state);
				--state.pulseDepth;
				state.pulseStack.pop_back();
			}
			else if (name == "rainbow" && state.rainbowDepth > 0)
			{
				FlushTextRun(state);
				--state.rainbowDepth;
				state.rainbowStack.pop_back();
			}
			else if (name == "tornado" && state.tornadoDepth > 0)
			{
				FlushTextRun(state);
				--state.tornadoDepth;
				state.tornadoStack.pop_back();
			}
			else if (name == "fade" && state.fadeDepth > 0)
			{
				FlushTextRun(state);
				--state.fadeDepth;
				state.fadeStack.pop_back();
			}
			else
			{
				// Stray/mismatched close (e.g. [/b] with no open [b]) or an unrecognised tag name - shown
				// verbatim so a writer notices the mistake instead of it silently vanishing.
				AppendLiteralTag(state, tagContent);
			}
			continue;
		}

		// Name ends at the first '=' (single-value tags like [color=#hex]/[icon=name]) OR the first
		// whitespace (multi-param tags like [wave amp=20 freq=5]) - whichever comes first. remainder starts
		// with "=value" for the former shape, " key=val ..." for the latter, or is empty for a bare tag like
		// [b]/[i]/[wave].
		size_t nameEnd        = tagContent.find_first_of(" \t=");
		std::string name      = Trim(nameEnd == std::string::npos ? tagContent : tagContent.substr(0, nameEnd));
		std::string remainder = nameEnd == std::string::npos ? "" : tagContent.substr(nameEnd);
		std::string arg       = (!remainder.empty() && remainder[0] == '=') ? Trim(remainder.substr(1)) : "";

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
			// effectMask deliberately left at 0 - icon runs don't animate yet (see the design doc's explicit
			// v1-v4 scoping note), regardless of any enclosing [wave]/[shake]/[pulse].
			state.currentLine.runs.push_back(std::move(iconRun));
		}
		else if (name == "wave")
		{
			FlushTextRun(state);
			std::map<std::string, std::string> params = ParseKeyValuePairs(remainder);
			WaveParams p;
			p.amplitude = GetFloatOr(params, "amp", p.amplitude);
			p.frequency = GetFloatOr(params, "freq", p.frequency);
			state.waveStack.push_back(p);
			++state.waveDepth;
		}
		else if (name == "shake")
		{
			FlushTextRun(state);
			std::map<std::string, std::string> params = ParseKeyValuePairs(remainder);
			ShakeParams p;
			p.rate  = GetFloatOr(params, "rate", p.rate);
			p.level = GetFloatOr(params, "level", p.level);
			state.shakeStack.push_back(p);
			++state.shakeDepth;
		}
		else if (name == "pulse")
		{
			FlushTextRun(state);
			std::map<std::string, std::string> params = ParseKeyValuePairs(remainder);
			PulseParams p;
			p.frequency = GetFloatOr(params, "freq", p.frequency);
			state.pulseStack.push_back(p);
			++state.pulseDepth;
		}
		else if (name == "rainbow")
		{
			FlushTextRun(state);
			std::map<std::string, std::string> params = ParseKeyValuePairs(remainder);
			RainbowParams p;
			p.frequency  = GetFloatOr(params, "freq", p.frequency);
			p.saturation = GetFloatOr(params, "sat", p.saturation);
			p.value      = GetFloatOr(params, "val", p.value);
			state.rainbowStack.push_back(p);
			++state.rainbowDepth;
		}
		else if (name == "tornado")
		{
			FlushTextRun(state);
			std::map<std::string, std::string> params = ParseKeyValuePairs(remainder);
			TornadoParams p;
			p.radius    = GetFloatOr(params, "radius", p.radius);
			p.frequency = GetFloatOr(params, "freq", p.frequency);
			state.tornadoStack.push_back(p);
			++state.tornadoDepth;
		}
		else if (name == "fade")
		{
			FlushTextRun(state);
			std::map<std::string, std::string> params = ParseKeyValuePairs(remainder);
			FadeParams p;
			p.start  = GetFloatOr(params, "start", p.start);
			p.length = GetFloatOr(params, "length", p.length);
			state.fadeStack.push_back(p);
			++state.fadeDepth;
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
