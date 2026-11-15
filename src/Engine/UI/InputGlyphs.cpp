#include "InputGlyphs.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <cctype>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"
#include "Engine/Input/InputMaps.h"
#include "nlohmann/json.hpp"

namespace
{
using Struktur::Input::InputComponent;
using Struktur::Input::InputSource;
using Struktur::Input::InputSourceKind;

bool IsDPadButton(const InputSource& source)
{
	return source.kind == InputSourceKind::GamepadButton &&
	       (source.code == SDL_GAMEPAD_BUTTON_DPAD_UP || source.code == SDL_GAMEPAD_BUTTON_DPAD_DOWN ||
	        source.code == SDL_GAMEPAD_BUTTON_DPAD_LEFT || source.code == SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
}

bool IsStickAxis(const InputSource& source, SDL_GamepadAxis x, SDL_GamepadAxis y)
{
	return source.kind == InputSourceKind::GamepadAxis && (source.code == x || source.code == y);
}

bool AllOf(const std::vector<InputSource>& sources, bool (*test)(const InputSource&))
{
	return !sources.empty() && std::all_of(sources.begin(), sources.end(), test);
}

bool IsLeftStick(const InputSource& source)
{
	return IsStickAxis(source, SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY);
}

bool IsRightStick(const InputSource& source)
{
	return IsStickAxis(source, SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY);
}

// Readable name for a binding with no art - drawn as text instead.
std::string FallbackText(const InputSource& source)
{
	std::string text;
	switch (source.kind)
	{
		case InputSourceKind::Key:
			text = ::SDL_GetScancodeName((SDL_Scancode)source.code);
			break;
		case InputSourceKind::PointerButton:
			text = source.code == 0 ? "Left Click" : source.code == 1 ? "Right Click" : "Middle Click";
			break;
		case InputSourceKind::GamepadButton:
			if (const char* name = ::SDL_GetGamepadStringForButton((SDL_GamepadButton)source.code))
			{
				text = name;
			}
			break;
		case InputSourceKind::GamepadAxis:
			if (const char* name = ::SDL_GetGamepadStringForAxis((SDL_GamepadAxis)source.code))
			{
				text = name;
			}
			break;
	}
	if (text.empty())
	{
		text = "?";
	}
	// SDL's pad names are lower case ("a", "leftshoulder") - capitalise the first letter at least.
	text[0] = (char)std::toupper((unsigned char)text[0]);
	return text;
}
}  // namespace

bool Struktur::UI::InputGlyphs::Load(GameContext& context, const std::string& filePath)
{
	auto result = FileSystem::ReadString(filePath);
	if (!result.success)
	{
		DEBUG_ERROR("InputGlyphs: failed to read %s: %s", filePath, result.errorMessage.c_str());
		return false;
	}
	// No-exceptions parse, same as InputConfigLoader - a malformed file logs instead of unwinding through Wren.
	nlohmann::json config = nlohmann::json::parse(result.value, nullptr, false);
	if (config.is_discarded() || !config.is_object())
	{
		DEBUG_ERROR("InputGlyphs: %s is not a valid JSON object", filePath);
		return false;
	}

	m_paths.clear();
	m_positionalButtonPaths.clear();
	m_dpadPath.clear();
	m_leftStickPath.clear();
	m_rightStickPath.clear();

	// Each section maps InputConfig.json names to art paths; a name that doesn't parse is warned about and skipped.
	auto eachEntry = [&](const char* section, auto&& add)
	{
		if (!config.contains(section) || !config[section].is_object())
		{
			return;
		}
		for (const auto& [name, path] : config[section].items())
		{
			if (!path.is_string() || !add(name, path.get<std::string>()))
			{
				DEBUG_WARNING("InputGlyphs: unknown %s entry '%s' in %s", section, name, filePath);
			}
		}
	};

	eachEntry("keyboard",
	          [&](const std::string& name, const std::string& path)
	          {
		          SDL_Scancode key;
		          if (!Input::InputMaps::TryGetKeycodeFromString(name, key))
		          {
			          return false;
		          }
		          m_paths[SourceKey(Input::InputSourceKind::Key, key)] = path;
		          return true;
	          });
	eachEntry("mouse",
	          [&](const std::string& name, const std::string& path)
	          {
		          static const std::unordered_map<std::string, Input::Input::PointerButton> kButtons = {
		              {"primary", Input::Input::PointerButton::Primary},
		              {"secondary", Input::Input::PointerButton::Secondary},
		              {"middle", Input::Input::PointerButton::Middle},
		          };
		          auto it = kButtons.find(name);
		          if (it == kButtons.end())
		          {
			          return false;
		          }
		          m_paths[SourceKey(Input::InputSourceKind::PointerButton, (int)it->second)] = path;
		          return true;
	          });
	eachEntry("gamepad",
	          [&](const std::string& name, const std::string& path)
	          {
		          SDL_GamepadButton button;
		          SDL_GamepadAxis axis;
		          if (Input::InputMaps::TryGetControllerButtonFromString(name, button))
		          {
			          m_paths[SourceKey(Input::InputSourceKind::GamepadButton, button)] = path;
			          return true;
		          }
		          if (Input::InputMaps::TryGetControllerAxisFromString(name, axis))
		          {
			          m_paths[SourceKey(Input::InputSourceKind::GamepadAxis, axis)] = path;
			          return true;
		          }
		          return false;
	          });
	eachEntry("gamepadPositional",
	          [&](const std::string& name, const std::string& path)
	          {
		          SDL_GamepadButton button;
		          if (!Input::InputMaps::TryGetControllerButtonFromString(name, button))
		          {
			          return false;
		          }
		          m_positionalButtonPaths[button] = path;
		          return true;
	          });
	eachEntry("composite",
	          [&](const std::string& name, const std::string& path)
	          {
		          std::string* target = name == "dpad"         ? &m_dpadPath
		                                : name == "leftStick"  ? &m_leftStickPath
		                                : name == "rightStick" ? &m_rightStickPath
		                                                       : nullptr;
		          if (!target)
		          {
			          return false;
		          }
		          *target = path;
		          return true;
	          });

	// Hints already on screen re-resolve against the new map.
	context.GetInput().NotifyHintsChanged();
	return true;
}

void Struktur::UI::InputGlyphs::Clear()
{
	m_paths.clear();
	m_positionalButtonPaths.clear();
	m_dpadPath.clear();
	m_leftStickPath.clear();
	m_rightStickPath.clear();
	m_textures.clear();
}

Struktur::UI::InputGlyph Struktur::UI::InputGlyphs::GlyphForPath(GameContext& context, const std::string& path)
{
	auto it = m_textures.find(path);
	if (it == m_textures.end())
	{
		it = m_textures.emplace(path, context.GetResourceManager().GetTexture(context, path)).first;
	}
	InputGlyph glyph;
	if (it->second)
	{
		glyph.texture   = it->second;
		glyph.pixelSize = {(float)it->second->GetWidth(), (float)it->second->GetHeight()};
	}
	return glyph;
}

Struktur::UI::InputGlyph Struktur::UI::InputGlyphs::GlyphForSource(GameContext& context,
                                                                   const Input::InputSource& source,
                                                                   Input::GamepadFamily family)
{
	const std::string* path = nullptr;
	if (family == Input::GamepadFamily::Positional && source.kind == Input::InputSourceKind::GamepadButton)
	{
		auto it = m_positionalButtonPaths.find(source.code);
		if (it != m_positionalButtonPaths.end())
		{
			path = &it->second;
		}
	}
	if (!path)
	{
		auto it = m_paths.find(SourceKey(source.kind, source.code));
		if (it != m_paths.end())
		{
			path = &it->second;
		}
	}

	if (path)
	{
		InputGlyph glyph = GlyphForPath(context, *path);
		if (glyph.HasTexture())
		{
			return glyph;
		}
	}
	InputGlyph glyph;
	glyph.text = FallbackText(source);
	return glyph;
}

std::vector<Struktur::UI::InputGlyph> Struktur::UI::InputGlyphs::Resolve(GameContext& context,
                                                                       const std::string& action,
                                                                       Input::InputComponent component)
{
	Input::Input& input                    = context.GetInput();
	std::vector<Input::InputSource> sources = input.GetHintSources(action, component);
	std::vector<InputGlyph> glyphs;
	if (sources.empty())
	{
		return glyphs;
	}

	// A whole direction set on a pad reads better as one picture than four.
	if (sources.size() > 1)
	{
		const std::string* composite = nullptr;
		if (sources.size() == 4 && AllOf(sources, IsDPadButton) && !m_dpadPath.empty())
		{
			composite = &m_dpadPath;
		}
		else if (AllOf(sources, IsLeftStick) && !m_leftStickPath.empty())
		{
			composite = &m_leftStickPath;
		}
		else if (AllOf(sources, IsRightStick) && !m_rightStickPath.empty())
		{
			composite = &m_rightStickPath;
		}
		if (composite)
		{
			InputGlyph glyph = GlyphForPath(context, *composite);
			if (glyph.HasTexture())
			{
				glyphs.push_back(glyph);
				return glyphs;
			}
		}
	}

	// One glyph per distinct button (a stick bound to a whole axis would otherwise appear once per side).
	Input::GamepadFamily family = input.GetGamepadFamily();
	std::vector<std::pair<Input::InputSourceKind, int>> seen;
	for (const Input::InputSource& source : sources)
	{
		std::pair<Input::InputSourceKind, int> key{source.kind, source.code};
		if (std::find(seen.begin(), seen.end(), key) != seen.end())
		{
			continue;
		}
		seen.push_back(key);
		glyphs.push_back(GlyphForSource(context, source, family));
	}
	return glyphs;
}
