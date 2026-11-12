#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Engine/Input/Input.h"
#include "Engine/Resource/Pointers/ResourcePtr.h"
#include "Engine/Resource/TextureResource.h"
#include "glm/glm.hpp"

namespace Struktur
{
class GameContext;

namespace UI
{
// One picture of one button in a hint. A glyph with no texture is a text fallback - the binding has no art in the
// glyph map, so the hint draws `text` (e.g. "Backspace") in its own font instead.
struct InputGlyph
{
	Resource::ResourcePtr<Resource::TextureResource> texture;
	// Native size of the art in pixels - callers scale it by a whole number so pixel art stays crisp.
	glm::vec2 pixelSize{0.0f, 0.0f};
	std::string text;

	bool HasTexture() const
	{
		return (bool)texture;
	}
};

// Turns an action ("UIAccept", "UITab", "UIDir") into the button art to show for it right now. Input decides
// WHICH binding (active device, last used, config order - see Input::GetHintSources); this maps that binding to
// a picture through a data file, so the engine never hard-codes the game's art paths:
//
//   assets/Settings/InputBindings/InputGlyphs.json
//   {
//     "keyboard": { "e": "UI/Input/Keyboard/KeyE.png", "leftshift": "UI/Input/Keyboard/KeyShift.png", ... },
//     "mouse":    { "primary": "UI/Input/Mouse/MouseButtonLeft.png", ... },
//     "gamepad":  { "a": ".../ButtonA/Idle.png", "lbumper": ".../ButtonLB.png", "lefttrigger": ".../ButtonLT.png",
//                   "leftx": ".../JoystickLeft.png", ... },
//     "gamepadPositional": { "a": ".../ButtonDown.png", ... },   // face buttons for PlayStation/Nintendo pads
//     "composite": { "dpad": ".../DPad.png", "leftStick": ".../JoystickLeft.png", "rightStick": ... }
//   }
//
// Keys are the same names InputConfig.json uses, parsed to codes on load (so aliases like "ltrigger"/"lbumper"
// find the same art). "composite" pictures a whole direction set at once: an axis2 hint on a pad that would be
// four D-pad buttons shows the one D-pad image, and a stick shows the stick.
//
// Owned by GameContext; loaded from Wren (InputGlyphs.load) right after the input bindings.
class InputGlyphs
{
public:
	bool Load(GameContext& context, const std::string& filePath);

	// The glyphs a hint for `action` shows on the active device, left to right (see Input::GetHintSources for
	// `component`). Empty when the action has nothing bound on that device.
	std::vector<InputGlyph> Resolve(GameContext& context, const std::string& action,
	                                Input::InputComponent component);

	// "UITab" -> {"UITab", None}; "UITab:negative" -> {"UITab", Negative}; "UIDir:up" -> {"UIDir", Up}. The one
	// spelling hint elements and the rich-text [input=] tag both accept.
	static std::pair<std::string, Input::InputComponent> ParseActionSpec(const std::string& spec)
	{
		size_t colon = spec.find(':');
		if (colon == std::string::npos)
		{
			return {spec, Input::InputComponent::None};
		}
		return {spec.substr(0, colon), Input::Input::ParseInputComponent(spec.substr(colon + 1))};
	}

private:
	static uint64_t SourceKey(Input::InputSourceKind kind, int code)
	{
		return ((uint64_t)kind << 32) | (uint32_t)code;
	}
	InputGlyph GlyphForPath(GameContext& context, const std::string& path);
	InputGlyph GlyphForSource(GameContext& context, const Input::InputSource& source, Input::GamepadFamily family);

	std::unordered_map<uint64_t, std::string> m_paths;              // SourceKey -> art path
	std::unordered_map<int, std::string> m_positionalButtonPaths;   // SDL_GamepadButton -> art path
	std::string m_dpadPath;
	std::string m_leftStickPath;
	std::string m_rightStickPath;
	// Loaded on first use and kept for the game's lifetime - a few dozen tiny textures at most.
	std::unordered_map<std::string, Resource::ResourcePtr<Resource::TextureResource>> m_textures;
};
}  // namespace UI
}  // namespace Struktur
