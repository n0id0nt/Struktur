#pragma once

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>

#include <string>

namespace Struktur::Input
{
// String <-> input-code lookups. Every table here is fixed, read-only data with no shared mutable state to
// guard, so this is a plain namespace of free functions (backed by static maps built once, the first time
// any of them is called) rather than a singleton class - there's nothing an Instance() accessor would be
// protecting that a free function doesn't already give for free.
namespace InputMaps
{
// String to code conversion
SDL_Scancode GetKeycodeFromString(const std::string& input);
SDL_GamepadButton GetControllerButtonFromString(const std::string& input);
SDL_GamepadAxis GetControllerAxisFromString(const std::string& input);

// Same lookups without the unknown-name assert - for data files where a bad name should be warned about and
// skipped rather than stop the game (see UI::InputGlyphs). Return false and leave `out` untouched if unknown.
bool TryGetKeycodeFromString(const std::string& input, SDL_Scancode& out);
bool TryGetControllerButtonFromString(const std::string& input, SDL_GamepadButton& out);
bool TryGetControllerAxisFromString(const std::string& input, SDL_GamepadAxis& out);

// Code to string conversion (useful for debugging/UI)
std::string GetStringFromKeycode(SDL_Scancode key);
std::string GetStringFromControllerButton(SDL_GamepadButton button);
std::string GetStringFromControllerAxis(SDL_GamepadAxis axis);
}  // namespace InputMaps
}  // namespace Struktur::Input
