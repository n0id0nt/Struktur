#include "InputMaps.h"

#include <format>

#include "Debug/Assertions.h"

namespace Struktur::Input
{

InputMaps& InputMaps::Instance()
{
	static InputMaps instance;
	return instance;
}

InputMaps::InputMaps()
{
	InitializeKeycodeMap();
	InitializeControllerButtonMap();
	InitializeControllerAxisMap();
}

void InputMaps::InitializeKeycodeMap()
{
	m_keycodeMap = {
		{"null",				KEY_NULL},
		// Alphanumeric keys
		{"apostrophe",          KEY_APOSTROPHE},
		{"comma",               KEY_COMMA},
		{"minus",               KEY_MINUS},
		{"period",              KEY_PERIOD},
		{"slash",				KEY_SLASH},
		{"0",					KEY_ZERO},
		{"1",					KEY_ONE},
		{"2",					KEY_TWO},
		{"3",					KEY_THREE},
		{"4",					KEY_FOUR},
		{"5",					KEY_FIVE},
		{"6",					KEY_SIX},
		{"7",					KEY_SEVEN},
		{"8",					KEY_EIGHT},
		{"9",					KEY_NINE},
		{"semicolon",           KEY_SEMICOLON},
		{"equal",				KEY_EQUAL},
		{"a",					KEY_A},
		{"b",                   KEY_B},
		{"c",                   KEY_C},
		{"d",                   KEY_D},
		{"e",                   KEY_E},
		{"f",                   KEY_F},
		{"g",                   KEY_G},
		{"h",                   KEY_H},
		{"i",                   KEY_I},
		{"j",                   KEY_J},
		{"k",                   KEY_K},
		{"l",					KEY_L},
		{"m",					KEY_M},
		{"n",					KEY_N},
		{"o",					KEY_O},
		{"p",					KEY_P},
		{"q",					KEY_Q},
		{"r",					KEY_R},
		{"s",					KEY_S},
		{"t",					KEY_T},
		{"u",					KEY_U},
		{"v",					KEY_V},
		{"w",					KEY_W},
		{"x",					KEY_X},
		{"y",                   KEY_Y},
		{"z",                   KEY_Z},
		{"leftbracket",         KEY_LEFT_BRACKET},
		{"backslash",           KEY_BACKSLASH},
		{"rightbracket",        KEY_RIGHT_BRACKET},
		{"grave",               KEY_GRAVE},
		// Function keys
		{"space",               KEY_SPACE},
		{"escape",              KEY_ESCAPE},
		{"enter",               KEY_ENTER},
		{"tab",                 KEY_TAB},
		{"backspace",           KEY_BACKSPACE},
		{"insert",              KEY_INSERT},
		{"delete",              KEY_DELETE},
		{"right",               KEY_RIGHT},
		{"left",                KEY_LEFT},
		{"down",                KEY_DOWN},
		{"up",                  KEY_UP},
		{"pageup",              KEY_PAGE_UP},
		{"pagedown",            KEY_PAGE_DOWN},
		{"home",                KEY_HOME},
		{"end",                 KEY_END},
		{"capslock",            KEY_CAPS_LOCK},
		{"scrolllock",          KEY_SCROLL_LOCK},
		{"numlock",             KEY_NUM_LOCK},
		{"printscreen",         KEY_PRINT_SCREEN},
		{"pause",				KEY_PAUSE},
		{"f1",                  KEY_F1},
		{"f2",                  KEY_F2},
		{"f3",                  KEY_F3},
		{"f4",                  KEY_F4},
		{"f5",                  KEY_F5},
		{"f6",                  KEY_F6},
		{"f7",                  KEY_F7},
		{"f8",                  KEY_F8},
		{"f9",                  KEY_F9},
		{"f10",                 KEY_F10},
		{"f11",                 KEY_F11},
		{"f12",                 KEY_F12},
		{"leftshift",			KEY_LEFT_SHIFT},
		{"leftcontrol",         KEY_LEFT_CONTROL},
		{"leftalt",             KEY_LEFT_ALT},
		{"leftsuper",           KEY_LEFT_SUPER},
		{"rightshift",          KEY_RIGHT_SHIFT},
		{"rightcontrol",        KEY_RIGHT_CONTROL},
		{"rightalt",            KEY_RIGHT_ALT},
		{"rightsuper",          KEY_RIGHT_SUPER},
		{"kbmenu",				KEY_KB_MENU},
		// Keypad keys
		{"kp0",					KEY_KP_0},
		{"kp1",					KEY_KP_1},
		{"kp2",                 KEY_KP_2},
		{"kp3",					KEY_KP_3},
		{"kp4",					KEY_KP_4},
		{"kp5",					KEY_KP_5},
		{"kp6",					KEY_KP_6},
		{"kp7",					KEY_KP_7},
		{"kp8",					KEY_KP_8},
		{"kp9",					KEY_KP_9},
		{"kpdecimal",           KEY_KP_DECIMAL},
		{"kpdivide",            KEY_KP_DIVIDE},
		{"kpmultiply",          KEY_KP_MULTIPLY},
		{"kpsubtract",          KEY_KP_SUBTRACT},
		{"kpadd",               KEY_KP_ADD},
		{"kpenter",             KEY_KP_ENTER},
		{"kpequal",             KEY_KP_EQUAL},
		// Android key buttons
		{"anback",              KEY_BACK},
		{"anmenu",              KEY_MENU},
		{"anvolumeup",          KEY_VOLUME_UP},
		{"anvolumedown",        KEY_VOLUME_DOWN},
	};

	// Build reverse map
	for (const auto& [str, key] : m_keycodeMap)
	{
		m_keycodeReverseMap[key] = str;
	}
}

void InputMaps::InitializeControllerButtonMap()
{
	m_controllerButtonMap = {
		{"unknown",     GAMEPAD_BUTTON_UNKNOWN},
		{"a",           GAMEPAD_BUTTON_RIGHT_FACE_DOWN},
		{"b",           GAMEPAD_BUTTON_RIGHT_FACE_RIGHT},
		{"x",           GAMEPAD_BUTTON_RIGHT_FACE_LEFT},
		{"y",           GAMEPAD_BUTTON_RIGHT_FACE_UP},
		{"select",      GAMEPAD_BUTTON_MIDDLE_LEFT},
		{"guide",       GAMEPAD_BUTTON_MIDDLE},
		{"start",       GAMEPAD_BUTTON_MIDDLE_RIGHT},
		{"ljoystick",   GAMEPAD_BUTTON_LEFT_THUMB},
		{"rjoystick",   GAMEPAD_BUTTON_RIGHT_THUMB},
		{"ltrigger",    GAMEPAD_BUTTON_LEFT_TRIGGER_1},
		{"lbumper",     GAMEPAD_BUTTON_LEFT_TRIGGER_2},
		{"rtrigger",    GAMEPAD_BUTTON_RIGHT_TRIGGER_1},
		{"rbumper",     GAMEPAD_BUTTON_RIGHT_TRIGGER_2},
		{"up",          GAMEPAD_BUTTON_LEFT_FACE_UP},
		{"down",        GAMEPAD_BUTTON_LEFT_FACE_DOWN},
		{"left",        GAMEPAD_BUTTON_LEFT_FACE_LEFT},
		{"right",       GAMEPAD_BUTTON_LEFT_FACE_RIGHT},
	};

	// Build reverse map
	for (const auto& [str, button] : m_controllerButtonMap)
	{
		m_controllerButtonReverseMap[button] = str;
	}
}

void InputMaps::InitializeControllerAxisMap()
{
	m_controllerAxisMap = {
		{"leftx",           GAMEPAD_AXIS_LEFT_X},
		{"lefty",           GAMEPAD_AXIS_LEFT_Y},
		{"rightx",          GAMEPAD_AXIS_RIGHT_X},
		{"righty",          GAMEPAD_AXIS_RIGHT_Y},
		{"lefttrigger",     GAMEPAD_AXIS_LEFT_TRIGGER},
		{"righttrigger",    GAMEPAD_AXIS_RIGHT_TRIGGER},
	};

	// Build reverse map
	for (const auto& [str, axis] : m_controllerAxisMap)
	{
		m_controllerAxisReverseMap[axis] = str;
	}
}

::KeyboardKey InputMaps::GetKeycodeFromString(const std::string& input) const
{
	auto it = m_keycodeMap.find(input);
	ASSERT_MSG(it != m_keycodeMap.end(), 
		std::format("Unknown keycode string: '{}'", input).c_str());
	return it->second;
}

::GamepadButton InputMaps::GetControllerButtonFromString(const std::string& input) const
{
	auto it = m_controllerButtonMap.find(input);
	ASSERT_MSG(it != m_controllerButtonMap.end(), 
		std::format("Unknown controller button string: '{}'", input).c_str());
	return it->second;
}

::GamepadAxis InputMaps::GetControllerAxisFromString(const std::string& input) const
{
	auto it = m_controllerAxisMap.find(input);
	ASSERT_MSG(it != m_controllerAxisMap.end(), 
		std::format("Unknown controller axis string: '{}'", input).c_str());
	return it->second;
}

std::string InputMaps::GetStringFromKeycode(::KeyboardKey key) const
{
	auto it = m_keycodeReverseMap.find(key);
	if (it != m_keycodeReverseMap.end())
		return it->second;
	return "unknown";
}

std::string InputMaps::GetStringFromControllerButton(::GamepadButton button) const
{
	auto it = m_controllerButtonReverseMap.find(button);
	if (it != m_controllerButtonReverseMap.end())
		return it->second;
	return "unknown";
}

std::string InputMaps::GetStringFromControllerAxis(::GamepadAxis axis) const
{
	auto it = m_controllerAxisReverseMap.find(axis);
	if (it != m_controllerAxisReverseMap.end())
		return it->second;
	return "unknown";
}

}
