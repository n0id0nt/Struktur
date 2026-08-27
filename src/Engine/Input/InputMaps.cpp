#include "InputMaps.h"

#include <unordered_map>

#include "Debug/Assertions.h"

namespace
{
// Fixed, read-only lookup tables - safe as plain namespace-scope statics (no static-initialization-order
// fiasco risk here, since they only depend on SDL's own compile-time enum constants, never on another
// translation unit's global state).
const std::unordered_map<std::string, SDL_Scancode> kKeycodeMap = {
    {"null", SDL_SCANCODE_UNKNOWN},
    // Alphanumeric keys
    {"apostrophe", SDL_SCANCODE_APOSTROPHE},
    {"comma", SDL_SCANCODE_COMMA},
    {"minus", SDL_SCANCODE_MINUS},
    {"period", SDL_SCANCODE_PERIOD},
    {"slash", SDL_SCANCODE_SLASH},
    {"0", SDL_SCANCODE_0},
    {"1", SDL_SCANCODE_1},
    {"2", SDL_SCANCODE_2},
    {"3", SDL_SCANCODE_3},
    {"4", SDL_SCANCODE_4},
    {"5", SDL_SCANCODE_5},
    {"6", SDL_SCANCODE_6},
    {"7", SDL_SCANCODE_7},
    {"8", SDL_SCANCODE_8},
    {"9", SDL_SCANCODE_9},
    {"semicolon", SDL_SCANCODE_SEMICOLON},
    {"equal", SDL_SCANCODE_EQUALS},
    {"a", SDL_SCANCODE_A},
    {"b", SDL_SCANCODE_B},
    {"c", SDL_SCANCODE_C},
    {"d", SDL_SCANCODE_D},
    {"e", SDL_SCANCODE_E},
    {"f", SDL_SCANCODE_F},
    {"g", SDL_SCANCODE_G},
    {"h", SDL_SCANCODE_H},
    {"i", SDL_SCANCODE_I},
    {"j", SDL_SCANCODE_J},
    {"k", SDL_SCANCODE_K},
    {"l", SDL_SCANCODE_L},
    {"m", SDL_SCANCODE_M},
    {"n", SDL_SCANCODE_N},
    {"o", SDL_SCANCODE_O},
    {"p", SDL_SCANCODE_P},
    {"q", SDL_SCANCODE_Q},
    {"r", SDL_SCANCODE_R},
    {"s", SDL_SCANCODE_S},
    {"t", SDL_SCANCODE_T},
    {"u", SDL_SCANCODE_U},
    {"v", SDL_SCANCODE_V},
    {"w", SDL_SCANCODE_W},
    {"x", SDL_SCANCODE_X},
    {"y", SDL_SCANCODE_Y},
    {"z", SDL_SCANCODE_Z},
    {"leftbracket", SDL_SCANCODE_LEFTBRACKET},
    {"backslash", SDL_SCANCODE_BACKSLASH},
    {"rightbracket", SDL_SCANCODE_RIGHTBRACKET},
    {"grave", SDL_SCANCODE_GRAVE},
    // Function keys
    {"space", SDL_SCANCODE_SPACE},
    {"escape", SDL_SCANCODE_ESCAPE},
    {"enter", SDL_SCANCODE_RETURN},
    {"tab", SDL_SCANCODE_TAB},
    {"backspace", SDL_SCANCODE_BACKSPACE},
    {"insert", SDL_SCANCODE_INSERT},
    {"delete", SDL_SCANCODE_DELETE},
    {"right", SDL_SCANCODE_RIGHT},
    {"left", SDL_SCANCODE_LEFT},
    {"down", SDL_SCANCODE_DOWN},
    {"up", SDL_SCANCODE_UP},
    {"pageup", SDL_SCANCODE_PAGEUP},
    {"pagedown", SDL_SCANCODE_PAGEDOWN},
    {"home", SDL_SCANCODE_HOME},
    {"end", SDL_SCANCODE_END},
    {"capslock", SDL_SCANCODE_CAPSLOCK},
    {"scrolllock", SDL_SCANCODE_SCROLLLOCK},
    {"numlock", SDL_SCANCODE_NUMLOCKCLEAR},
    {"printscreen", SDL_SCANCODE_PRINTSCREEN},
    {"pause", SDL_SCANCODE_PAUSE},
    {"f1", SDL_SCANCODE_F1},
    {"f2", SDL_SCANCODE_F2},
    {"f3", SDL_SCANCODE_F3},
    {"f4", SDL_SCANCODE_F4},
    {"f5", SDL_SCANCODE_F5},
    {"f6", SDL_SCANCODE_F6},
    {"f7", SDL_SCANCODE_F7},
    {"f8", SDL_SCANCODE_F8},
    {"f9", SDL_SCANCODE_F9},
    {"f10", SDL_SCANCODE_F10},
    {"f11", SDL_SCANCODE_F11},
    {"f12", SDL_SCANCODE_F12},
    {"leftshift", SDL_SCANCODE_LSHIFT},
    {"leftcontrol", SDL_SCANCODE_LCTRL},
    {"leftalt", SDL_SCANCODE_LALT},
    {"leftsuper", SDL_SCANCODE_LGUI},
    {"rightshift", SDL_SCANCODE_RSHIFT},
    {"rightcontrol", SDL_SCANCODE_RCTRL},
    {"rightalt", SDL_SCANCODE_RALT},
    {"rightsuper", SDL_SCANCODE_RGUI},
    {"kbmenu", SDL_SCANCODE_MENU},
    // Keypad keys
    {"kp0", SDL_SCANCODE_KP_0},
    {"kp1", SDL_SCANCODE_KP_1},
    {"kp2", SDL_SCANCODE_KP_2},
    {"kp3", SDL_SCANCODE_KP_3},
    {"kp4", SDL_SCANCODE_KP_4},
    {"kp5", SDL_SCANCODE_KP_5},
    {"kp6", SDL_SCANCODE_KP_6},
    {"kp7", SDL_SCANCODE_KP_7},
    {"kp8", SDL_SCANCODE_KP_8},
    {"kp9", SDL_SCANCODE_KP_9},
    {"kpdecimal", SDL_SCANCODE_KP_DECIMAL},
    {"kpdivide", SDL_SCANCODE_KP_DIVIDE},
    {"kpmultiply", SDL_SCANCODE_KP_MULTIPLY},
    {"kpsubtract", SDL_SCANCODE_KP_MINUS},
    {"kpadd", SDL_SCANCODE_KP_PLUS},
    {"kpenter", SDL_SCANCODE_KP_ENTER},
    {"kpequal", SDL_SCANCODE_KP_EQUALS},
    // "anback"/"anmenu"/"anvolumeup"/"anvolumedown" (Android hardware buttons) are deliberately not mapped -
    // there's no desktop/web SDL scancode equivalent, and neither platform this project targets has them.
};

const std::unordered_map<std::string, SDL_GamepadButton> kControllerButtonMap = {
    {"unknown", SDL_GAMEPAD_BUTTON_INVALID},
    {"a", SDL_GAMEPAD_BUTTON_SOUTH},
    {"b", SDL_GAMEPAD_BUTTON_EAST},
    {"x", SDL_GAMEPAD_BUTTON_WEST},
    {"y", SDL_GAMEPAD_BUTTON_NORTH},
    {"select", SDL_GAMEPAD_BUTTON_BACK},
    {"guide", SDL_GAMEPAD_BUTTON_GUIDE},
    {"start", SDL_GAMEPAD_BUTTON_START},
    {"ljoystick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {"rjoystick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {"ltrigger", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {"rtrigger", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    // "lbumper"/"rbumper" have no SDL_GamepadButton equivalent - SDL3 models the analog triggers as axes only
    // (SDL_GAMEPAD_AXIS_LEFT/RIGHT_TRIGGER, see kControllerAxisMap's "lefttrigger"/"righttrigger"), so these
    // were already silently non-functional before this file stopped going through raylib-style button codes -
    // kept mapped to INVALID rather than removed, so an existing config binding these doesn't hard-fail.
    {"lbumper", SDL_GAMEPAD_BUTTON_INVALID},
    {"rbumper", SDL_GAMEPAD_BUTTON_INVALID},
    {"up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {"down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {"left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {"right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
};

const std::unordered_map<std::string, SDL_GamepadAxis> kControllerAxisMap = {
    {"leftx", SDL_GAMEPAD_AXIS_LEFTX},
    {"lefty", SDL_GAMEPAD_AXIS_LEFTY},
    {"rightx", SDL_GAMEPAD_AXIS_RIGHTX},
    {"righty", SDL_GAMEPAD_AXIS_RIGHTY},
    {"lefttrigger", SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {"righttrigger", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
};

template <typename T>
std::unordered_map<T, std::string> BuildReverseMap(const std::unordered_map<std::string, T>& forward)
{
	std::unordered_map<T, std::string> reverse;
	for (const auto& [str, code] : forward)
	{
		reverse[code] = str;
	}
	return reverse;
}

const std::unordered_map<SDL_Scancode, std::string> kKeycodeReverseMap = BuildReverseMap(kKeycodeMap);
const std::unordered_map<SDL_GamepadButton, std::string> kControllerButtonReverseMap =
    BuildReverseMap(kControllerButtonMap);
const std::unordered_map<SDL_GamepadAxis, std::string> kControllerAxisReverseMap = BuildReverseMap(kControllerAxisMap);
}  // namespace

namespace Struktur::Input::InputMaps
{
SDL_Scancode GetKeycodeFromString(const std::string& input)
{
	auto it = kKeycodeMap.find(input);
	ASSERT_MSG(it != kKeycodeMap.end(), "Unknown keycode string: '%s'", input);
	return it->second;
}

SDL_GamepadButton GetControllerButtonFromString(const std::string& input)
{
	auto it = kControllerButtonMap.find(input);
	ASSERT_MSG(it != kControllerButtonMap.end(), "Unknown controller button string: '%s'", input);
	return it->second;
}

SDL_GamepadAxis GetControllerAxisFromString(const std::string& input)
{
	auto it = kControllerAxisMap.find(input);
	ASSERT_MSG(it != kControllerAxisMap.end(), "Unknown controller axis string: '%s'", input);
	return it->second;
}

std::string GetStringFromKeycode(SDL_Scancode key)
{
	auto it = kKeycodeReverseMap.find(key);
	if (it != kKeycodeReverseMap.end())
	{
		return it->second;
	}
	return "unknown";
}

std::string GetStringFromControllerButton(SDL_GamepadButton button)
{
	auto it = kControllerButtonReverseMap.find(button);
	if (it != kControllerButtonReverseMap.end())
	{
		return it->second;
	}
	return "unknown";
}

std::string GetStringFromControllerAxis(SDL_GamepadAxis axis)
{
	auto it = kControllerAxisReverseMap.find(axis);
	if (it != kControllerAxisReverseMap.end())
	{
		return it->second;
	}
	return "unknown";
}
}  // namespace Struktur::Input::InputMaps
