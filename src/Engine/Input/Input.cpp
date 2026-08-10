#include "Input.h"

#include <cmath>

#include "InputConfigLoader.h"
#include "InputMaps.h"

#include <SDL3/SDL.h>

namespace
{
using namespace Struktur::Input;
// raylib exposes gamepads as a flat 0-based index; SDL3 identifies them by a (non-sequential) joystick instance
// ID, so this re-derives the "Nth currently connected gamepad" raylib's model assumes.
SDL_JoystickID GetGamepadInstanceIdAtIndex(int index)
{
	int count               = 0;
	SDL_JoystickID* gamepads = ::SDL_GetGamepads(&count);
	SDL_JoystickID result   = 0;
	if (gamepads && index >= 0 && index < count)
	{
		result = gamepads[index];
	}
	if (gamepads)
	{
		::SDL_free(gamepads);
	}
	return result;
}

// raylib's KeyboardKey values are ASCII/GLFW-derived; SDL3 scancodes use their own USB-HID-based numbering, so
// callers still passing raylib key constants (Wren bindings, InputConfigLoader) need an explicit translation.
SDL_Scancode ToSDLScancode(KeyboardKey key)
{
	switch (key)
	{
		case KEY_APOSTROPHE: return SDL_SCANCODE_APOSTROPHE;
		case KEY_COMMA: return SDL_SCANCODE_COMMA;
		case KEY_MINUS: return SDL_SCANCODE_MINUS;
		case KEY_PERIOD: return SDL_SCANCODE_PERIOD;
		case KEY_SLASH: return SDL_SCANCODE_SLASH;
		case KEY_ZERO: return SDL_SCANCODE_0;
		case KEY_ONE: return SDL_SCANCODE_1;
		case KEY_TWO: return SDL_SCANCODE_2;
		case KEY_THREE: return SDL_SCANCODE_3;
		case KEY_FOUR: return SDL_SCANCODE_4;
		case KEY_FIVE: return SDL_SCANCODE_5;
		case KEY_SIX: return SDL_SCANCODE_6;
		case KEY_SEVEN: return SDL_SCANCODE_7;
		case KEY_EIGHT: return SDL_SCANCODE_8;
		case KEY_NINE: return SDL_SCANCODE_9;
		case KEY_SEMICOLON: return SDL_SCANCODE_SEMICOLON;
		case KEY_EQUAL: return SDL_SCANCODE_EQUALS;
		case KEY_A: return SDL_SCANCODE_A;
		case KEY_B: return SDL_SCANCODE_B;
		case KEY_C: return SDL_SCANCODE_C;
		case KEY_D: return SDL_SCANCODE_D;
		case KEY_E: return SDL_SCANCODE_E;
		case KEY_F: return SDL_SCANCODE_F;
		case KEY_G: return SDL_SCANCODE_G;
		case KEY_H: return SDL_SCANCODE_H;
		case KEY_I: return SDL_SCANCODE_I;
		case KEY_J: return SDL_SCANCODE_J;
		case KEY_K: return SDL_SCANCODE_K;
		case KEY_L: return SDL_SCANCODE_L;
		case KEY_M: return SDL_SCANCODE_M;
		case KEY_N: return SDL_SCANCODE_N;
		case KEY_O: return SDL_SCANCODE_O;
		case KEY_P: return SDL_SCANCODE_P;
		case KEY_Q: return SDL_SCANCODE_Q;
		case KEY_R: return SDL_SCANCODE_R;
		case KEY_S: return SDL_SCANCODE_S;
		case KEY_T: return SDL_SCANCODE_T;
		case KEY_U: return SDL_SCANCODE_U;
		case KEY_V: return SDL_SCANCODE_V;
		case KEY_W: return SDL_SCANCODE_W;
		case KEY_X: return SDL_SCANCODE_X;
		case KEY_Y: return SDL_SCANCODE_Y;
		case KEY_Z: return SDL_SCANCODE_Z;
		case KEY_LEFT_BRACKET: return SDL_SCANCODE_LEFTBRACKET;
		case KEY_BACKSLASH: return SDL_SCANCODE_BACKSLASH;
		case KEY_RIGHT_BRACKET: return SDL_SCANCODE_RIGHTBRACKET;
		case KEY_GRAVE: return SDL_SCANCODE_GRAVE;
		case KEY_SPACE: return SDL_SCANCODE_SPACE;
		case KEY_ESCAPE: return SDL_SCANCODE_ESCAPE;
		case KEY_ENTER: return SDL_SCANCODE_RETURN;
		case KEY_TAB: return SDL_SCANCODE_TAB;
		case KEY_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
		case KEY_INSERT: return SDL_SCANCODE_INSERT;
		case KEY_DELETE: return SDL_SCANCODE_DELETE;
		case KEY_RIGHT: return SDL_SCANCODE_RIGHT;
		case KEY_LEFT: return SDL_SCANCODE_LEFT;
		case KEY_DOWN: return SDL_SCANCODE_DOWN;
		case KEY_UP: return SDL_SCANCODE_UP;
		case KEY_PAGE_UP: return SDL_SCANCODE_PAGEUP;
		case KEY_PAGE_DOWN: return SDL_SCANCODE_PAGEDOWN;
		case KEY_HOME: return SDL_SCANCODE_HOME;
		case KEY_END: return SDL_SCANCODE_END;
		case KEY_CAPS_LOCK: return SDL_SCANCODE_CAPSLOCK;
		case KEY_SCROLL_LOCK: return SDL_SCANCODE_SCROLLLOCK;
		case KEY_NUM_LOCK: return SDL_SCANCODE_NUMLOCKCLEAR;
		case KEY_PRINT_SCREEN: return SDL_SCANCODE_PRINTSCREEN;
		case KEY_PAUSE: return SDL_SCANCODE_PAUSE;
		case KEY_F1: return SDL_SCANCODE_F1;
		case KEY_F2: return SDL_SCANCODE_F2;
		case KEY_F3: return SDL_SCANCODE_F3;
		case KEY_F4: return SDL_SCANCODE_F4;
		case KEY_F5: return SDL_SCANCODE_F5;
		case KEY_F6: return SDL_SCANCODE_F6;
		case KEY_F7: return SDL_SCANCODE_F7;
		case KEY_F8: return SDL_SCANCODE_F8;
		case KEY_F9: return SDL_SCANCODE_F9;
		case KEY_F10: return SDL_SCANCODE_F10;
		case KEY_F11: return SDL_SCANCODE_F11;
		case KEY_F12: return SDL_SCANCODE_F12;
		case KEY_LEFT_SHIFT: return SDL_SCANCODE_LSHIFT;
		case KEY_LEFT_CONTROL: return SDL_SCANCODE_LCTRL;
		case KEY_LEFT_ALT: return SDL_SCANCODE_LALT;
		case KEY_LEFT_SUPER: return SDL_SCANCODE_LGUI;
		case KEY_RIGHT_SHIFT: return SDL_SCANCODE_RSHIFT;
		case KEY_RIGHT_CONTROL: return SDL_SCANCODE_RCTRL;
		case KEY_RIGHT_ALT: return SDL_SCANCODE_RALT;
		case KEY_RIGHT_SUPER: return SDL_SCANCODE_RGUI;
		case KEY_KB_MENU: return SDL_SCANCODE_MENU;
		case KEY_KP_0: return SDL_SCANCODE_KP_0;
		case KEY_KP_1: return SDL_SCANCODE_KP_1;
		case KEY_KP_2: return SDL_SCANCODE_KP_2;
		case KEY_KP_3: return SDL_SCANCODE_KP_3;
		case KEY_KP_4: return SDL_SCANCODE_KP_4;
		case KEY_KP_5: return SDL_SCANCODE_KP_5;
		case KEY_KP_6: return SDL_SCANCODE_KP_6;
		case KEY_KP_7: return SDL_SCANCODE_KP_7;
		case KEY_KP_8: return SDL_SCANCODE_KP_8;
		case KEY_KP_9: return SDL_SCANCODE_KP_9;
		case KEY_KP_DECIMAL: return SDL_SCANCODE_KP_DECIMAL;
		case KEY_KP_DIVIDE: return SDL_SCANCODE_KP_DIVIDE;
		case KEY_KP_MULTIPLY: return SDL_SCANCODE_KP_MULTIPLY;
		case KEY_KP_SUBTRACT: return SDL_SCANCODE_KP_MINUS;
		case KEY_KP_ADD: return SDL_SCANCODE_KP_PLUS;
		case KEY_KP_ENTER: return SDL_SCANCODE_KP_ENTER;
		case KEY_KP_EQUAL: return SDL_SCANCODE_KP_EQUALS;
		default: return SDL_SCANCODE_UNKNOWN;
	}
}

SDL_GamepadButton ToSDLGamepadButton(GamepadButton button)
{
	switch (button)
	{
		case GAMEPAD_BUTTON_LEFT_FACE_UP: return SDL_GAMEPAD_BUTTON_DPAD_UP;
		case GAMEPAD_BUTTON_LEFT_FACE_RIGHT: return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
		case GAMEPAD_BUTTON_LEFT_FACE_DOWN: return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
		case GAMEPAD_BUTTON_LEFT_FACE_LEFT: return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
		case GAMEPAD_BUTTON_RIGHT_FACE_UP: return SDL_GAMEPAD_BUTTON_NORTH;
		case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return SDL_GAMEPAD_BUTTON_EAST;
		case GAMEPAD_BUTTON_RIGHT_FACE_DOWN: return SDL_GAMEPAD_BUTTON_SOUTH;
		case GAMEPAD_BUTTON_RIGHT_FACE_LEFT: return SDL_GAMEPAD_BUTTON_WEST;
		case GAMEPAD_BUTTON_LEFT_TRIGGER_1: return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
		case GAMEPAD_BUTTON_RIGHT_TRIGGER_1: return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
		case GAMEPAD_BUTTON_MIDDLE_LEFT: return SDL_GAMEPAD_BUTTON_BACK;
		case GAMEPAD_BUTTON_MIDDLE: return SDL_GAMEPAD_BUTTON_GUIDE;
		case GAMEPAD_BUTTON_MIDDLE_RIGHT: return SDL_GAMEPAD_BUTTON_START;
		case GAMEPAD_BUTTON_LEFT_THUMB: return SDL_GAMEPAD_BUTTON_LEFT_STICK;
		case GAMEPAD_BUTTON_RIGHT_THUMB: return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
		// LEFT_TRIGGER_2/RIGHT_TRIGGER_2 are analog axes in SDL3 (SDL_GAMEPAD_AXIS_LEFT/RIGHT_TRIGGER),
		// not digital buttons - GetControllerAxisValue() is the correct query for those.
		default: return SDL_GAMEPAD_BUTTON_INVALID;
	}
}

SDL_GamepadAxis ToSDLGamepadAxis(GamepadAxis axis)
{
	switch (axis)
	{
		case GAMEPAD_AXIS_LEFT_X: return SDL_GAMEPAD_AXIS_LEFTX;
		case GAMEPAD_AXIS_LEFT_Y: return SDL_GAMEPAD_AXIS_LEFTY;
		case GAMEPAD_AXIS_RIGHT_X: return SDL_GAMEPAD_AXIS_RIGHTX;
		case GAMEPAD_AXIS_RIGHT_Y: return SDL_GAMEPAD_AXIS_RIGHTY;
		case GAMEPAD_AXIS_LEFT_TRIGGER: return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
		case GAMEPAD_AXIS_RIGHT_TRIGGER: return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
		default: return SDL_GAMEPAD_AXIS_INVALID;
	}
}
}  // namespace

namespace Struktur::Input
{

// ============================================================================
// CONSTRUCTOR & LIFECYCLE
// ============================================================================

Input::Input(int gamepadIndex)
    : m_deadzone(0.0f),
      m_gamepadIndex(gamepadIndex)
{
	SDL_JoystickID instanceId = GetGamepadInstanceIdAtIndex(m_gamepadIndex);
	if (instanceId != 0)
	{
		m_sdlGamepad = ::SDL_OpenGamepad(instanceId);
		m_gamepadId  = m_sdlGamepad ? ::SDL_GetGamepadName(m_sdlGamepad) : "";
		DEBUG_INFO(std::format("Gamepad {} '{}' successfully connected.", m_gamepadIndex, m_gamepadId).c_str());
	}
	else
	{
		DEBUG_WARNING(std::format("Gamepad {} is not connected.", m_gamepadIndex).c_str());
	}
}

Input::~Input()
{
	Clear();
	if (m_sdlGamepad)
	{
		::SDL_CloseGamepad(m_sdlGamepad);
		m_sdlGamepad = nullptr;
	}
}

void Input::Update()
{
	SDL_JoystickID instanceId = GetGamepadInstanceIdAtIndex(m_gamepadIndex);
	if (instanceId != 0)
	{
		if (!m_sdlGamepad || ::SDL_GetGamepadID(m_sdlGamepad) != instanceId)
		{
			if (m_sdlGamepad)
			{
				::SDL_CloseGamepad(m_sdlGamepad);
			}
			m_sdlGamepad           = ::SDL_OpenGamepad(instanceId);
			std::string currentId = m_sdlGamepad ? ::SDL_GetGamepadName(m_sdlGamepad) : "";
			if (currentId != m_gamepadId)
			{
				m_gamepadId = currentId;
				DEBUG_INFO(std::format("Gamepad {} changed to '{}'", m_gamepadIndex, m_gamepadId).c_str());
			}
		}
	}
	else if (!m_gamepadId.empty())
	{
		// Gamepad was connected but is now disconnected
		DEBUG_WARNING(std::format("Gamepad {} '{}' disconnected", m_gamepadIndex, m_gamepadId).c_str());
		m_gamepadId.clear();
		if (m_sdlGamepad)
		{
			::SDL_CloseGamepad(m_sdlGamepad);
			m_sdlGamepad = nullptr;
		}
	}

	// Rotate snapshots so IsKeyJustPressed/IsKeyJustReleased can diff "this frame" against "last frame".
	m_prevKeyboardState = m_currKeyboardState;
	int numKeys               = 0;
	const bool* keyboardState = ::SDL_GetKeyboardState(&numKeys);
	for (int i = 0; i < (int)m_currKeyboardState.size() && i < numKeys; ++i)
	{
		m_currKeyboardState[i] = keyboardState[i];
	}

	// Same rotation for gamepad buttons, so IsControllerButtonJustPressed/Released can edge-detect too.
	static const GamepadButton kAllButtons[] = {
	    GAMEPAD_BUTTON_LEFT_FACE_UP,     GAMEPAD_BUTTON_LEFT_FACE_RIGHT,  GAMEPAD_BUTTON_LEFT_FACE_DOWN,
	    GAMEPAD_BUTTON_LEFT_FACE_LEFT,   GAMEPAD_BUTTON_RIGHT_FACE_UP,    GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,
	    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,  GAMEPAD_BUTTON_RIGHT_FACE_LEFT,  GAMEPAD_BUTTON_LEFT_TRIGGER_1,
	    GAMEPAD_BUTTON_RIGHT_TRIGGER_1,  GAMEPAD_BUTTON_MIDDLE_LEFT,      GAMEPAD_BUTTON_MIDDLE,
	    GAMEPAD_BUTTON_MIDDLE_RIGHT,     GAMEPAD_BUTTON_LEFT_THUMB,       GAMEPAD_BUTTON_RIGHT_THUMB,
	};
	for (GamepadButton button : kAllButtons)
	{
		m_prevControllerButtons[button] = m_currControllerButtons[button];
		m_currControllerButtons[button] = IsControllerButtonDown(button);
	}
}

void Input::Clear()
{
	m_buttonBindings.clear();
	m_variableBindings.clear();
	m_axisBindings.clear();
	m_axis2Bindings.clear();
	m_deadzone = 0.0f;
}

// ============================================================================
// LOADING
// ============================================================================

void Input::LoadInputBindings(const std::string& file)
{
	// Delegate to InputConfigLoader
	if (!InputConfigLoader::LoadFromFile(*this, file))
	{
		DEBUG_ERROR(std::format("Failed to load input bindings from: {}", file).c_str());
	}
}

// ============================================================================
// GAMEPAD MANAGEMENT
// ============================================================================

void Input::SetGamepadIndex(int index)
{
	m_gamepadIndex = index;
	if (m_sdlGamepad)
	{
		::SDL_CloseGamepad(m_sdlGamepad);
		m_sdlGamepad = nullptr;
	}
	SDL_JoystickID instanceId = GetGamepadInstanceIdAtIndex(m_gamepadIndex);
	if (instanceId != 0)
	{
		m_sdlGamepad = ::SDL_OpenGamepad(instanceId);
		m_gamepadId  = m_sdlGamepad ? ::SDL_GetGamepadName(m_sdlGamepad) : "";
		DEBUG_INFO(std::format("Switched to gamepad {} '{}'", m_gamepadIndex, m_gamepadId).c_str());
	}
	else
	{
		m_gamepadId.clear();
	}
}

bool Input::IsGamepadConnected() const
{
	return m_sdlGamepad != nullptr;
}

// ============================================================================
// PARSING HELPERS
// ============================================================================

Input::AxisComponent Input::ParseAxisComponent(const std::string& component)
{
	if (component == "positive")
	{
		return AxisComponent::Positive;
	}
	if (component == "negative")
	{
		return AxisComponent::Negative;
	}

	ASSERT_MSG(false, std::format("Invalid axis component: '{}'", component).c_str());
	return AxisComponent::Positive;
}

Input::Axis2Component Input::ParseAxis2Component(const std::string& component)
{
	if (component == "up")
	{
		return Axis2Component::Up;
	}
	if (component == "down")
	{
		return Axis2Component::Down;
	}
	if (component == "left")
	{
		return Axis2Component::Left;
	}
	if (component == "right")
	{
		return Axis2Component::Right;
	}

	ASSERT_MSG(false, std::format("Invalid axis2 component: '{}'", component).c_str());
	return Axis2Component::Up;
}

Input::Axis2Direction Input::ParseAxis2Direction(const std::string& direction)
{
	if (direction == "x")
	{
		return Axis2Direction::X;
	}
	if (direction == "y")
	{
		return Axis2Direction::Y;
	}

	ASSERT_MSG(false, std::format("Invalid axis2 direction: '{}'", direction).c_str());
	return Axis2Direction::X;
}

Input::VariableBindingAxis Input::ParseVariableBindingAxis(const std::string& axis)
{
	if (axis == "positive")
	{
		return VariableBindingAxis::Positive;
	}
	if (axis == "negative")
	{
		return VariableBindingAxis::Negative;
	}
	if (axis == "negativeToPositive")
	{
		return VariableBindingAxis::NegativeToPositive;
	}
	if (axis == "positiveToNegative")
	{
		return VariableBindingAxis::PositiveToNegative;
	}

	ASSERT_MSG(false, std::format("Invalid variable binding axis: '{}'", axis).c_str());
	return VariableBindingAxis::Positive;
}

// ============================================================================
// DEADZONE APPLICATION
// ============================================================================

float Input::ApplyDeadzone(float value) const
{
	// Simple axial deadzone with rescaling
	if (std::abs(value) < m_deadzone)
	{
		return 0.0f;
	}

	// Rescale to maintain full range outside deadzone
	float sign     = (value > 0.0f) ? 1.0f : -1.0f;
	float absValue = std::abs(value);
	return sign * ((absValue - m_deadzone) / (1.0f - m_deadzone));
}

glm::vec2 Input::ApplyRadialDeadzone(glm::vec2 value) const
{
	// Radial deadzone - treats the stick as a circle
	float magnitude = glm::length(value);

	if (magnitude < m_deadzone)
	{
		return glm::vec2(0.0f, 0.0f);
	}

	// Rescale to maintain full range outside deadzone
	glm::vec2 direction   = value / magnitude;
	float scaledMagnitude = (magnitude - m_deadzone) / (1.0f - m_deadzone);
	scaledMagnitude       = std::min(scaledMagnitude, 1.0f);

	return direction * scaledMagnitude;
}

// ============================================================================
// RAW INPUT
// ============================================================================

bool Input::IsKeyDown(KeyboardKey key)
{
	SDL_Scancode sc = ToSDLScancode(key);
	return sc < (int)m_currKeyboardState.size() && m_currKeyboardState[sc];
}

bool Input::IsKeyJustPressed(KeyboardKey key)
{
	SDL_Scancode sc = ToSDLScancode(key);
	return sc < (int)m_currKeyboardState.size() && m_currKeyboardState[sc] && !m_prevKeyboardState[sc];
}

bool Input::IsKeyJustReleased(KeyboardKey key)
{
	SDL_Scancode sc = ToSDLScancode(key);
	return sc < (int)m_currKeyboardState.size() && !m_currKeyboardState[sc] && m_prevKeyboardState[sc];
}

bool Input::IsControllerButtonDown(GamepadButton button)
{
	return m_sdlGamepad && ::SDL_GetGamepadButton(m_sdlGamepad, ToSDLGamepadButton(button));
}

bool Input::IsControllerButtonJustPressed(GamepadButton button)
{
	// SDL3 has no polling-based "just pressed" query (that's event-driven, via SDL_EVENT_GAMEPAD_BUTTON_DOWN);
	// approximate it the same way keyboard edge detection works, off the same per-frame Update() snapshot.
	return IsControllerButtonDown(button) && !WasControllerButtonDownLastFrame(button);
}

bool Input::IsControllerButtonJustReleased(GamepadButton button)
{
	return !IsControllerButtonDown(button) && WasControllerButtonDownLastFrame(button);
}

bool Input::WasControllerButtonDownLastFrame(GamepadButton button)
{
	auto it = m_prevControllerButtons.find(button);
	return it != m_prevControllerButtons.end() && it->second;
}

float Input::GetControllerAxisValue(GamepadAxis code)
{
	if (!m_sdlGamepad)
	{
		return 0.0f;
	}
	float rawValue = ::SDL_GetGamepadAxis(m_sdlGamepad, ToSDLGamepadAxis(code)) / 32767.0f;
	return ApplyDeadzone(rawValue);
}

// FIXED: Added break statements
float Input::GetControllerVariableValue(GamepadAxis code, VariableBindingAxis variableBindingAxis)
{
	float rawValue = GetControllerAxisValue(code);

	switch (variableBindingAxis)
	{
		case VariableBindingAxis::Positive:
			if (rawValue > 0.0f)
			{
				return rawValue;
			}
			break;

		case VariableBindingAxis::Negative:
			if (rawValue < 0.0f)
			{
				return -rawValue;
			}
			break;

		case VariableBindingAxis::NegativeToPositive:
			return (rawValue + 1.0f) / 2.0f;

		case VariableBindingAxis::PositiveToNegative:
			return (1.0f - rawValue) / 2.0f;
	}

	return 0.0f;
}

// ============================================================================
// STRING-BASED RAW INPUT
// ============================================================================

bool Input::IsStringKeyDown(const std::string& input)
{
	KeyboardKey key = InputMaps::Instance().GetKeycodeFromString(input);
	return IsKeyDown(key);
}

bool Input::IsStringKeyJustPressed(const std::string& input)
{
	KeyboardKey key = InputMaps::Instance().GetKeycodeFromString(input);
	return IsKeyJustPressed(key);
}

bool Input::IsStringKeyJustReleased(const std::string& input)
{
	KeyboardKey key = InputMaps::Instance().GetKeycodeFromString(input);
	return IsKeyJustReleased(key);
}

bool Input::IsStringControllerButtonDown(const std::string& input)
{
	GamepadButton button = InputMaps::Instance().GetControllerButtonFromString(input);
	return IsControllerButtonDown(button);
}

bool Input::IsStringControllerButtonJustPressed(const std::string& input)
{
	GamepadButton button = InputMaps::Instance().GetControllerButtonFromString(input);
	return IsControllerButtonJustPressed(button);
}

bool Input::IsStringControllerButtonJustReleased(const std::string& input)
{
	GamepadButton button = InputMaps::Instance().GetControllerButtonFromString(input);
	return IsControllerButtonJustReleased(button);
}

float Input::GetStringControllerAxisValue(const std::string& input)
{
	GamepadAxis axis = InputMaps::Instance().GetControllerAxisFromString(input);
	return GetControllerAxisValue(axis);
}

// ============================================================================
// BINDING CREATION
// ============================================================================

void Input::CreateButtonBinding(const std::string& input, KeyboardKey code)
{
	m_buttonBindings[input].keycodes.insert(code);
}

void Input::CreateButtonBinding(const std::string& input, GamepadButton code)
{
	m_buttonBindings[input].controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, KeyboardKey code)
{
	m_variableBindings[input].buttonBindings.keycodes.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, GamepadButton code)
{
	m_variableBindings[input].buttonBindings.controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, GamepadAxis code, VariableBindingAxis variableBindingAxis)
{
	m_variableBindings[input].controllerVariables.insert({variableBindingAxis, code});
}

void Input::CreateAxisBinding(const std::string& input, KeyboardKey code, AxisComponent axis)
{
	switch (axis)
	{
		case AxisComponent::Positive:
			m_axisBindings[input].positive.keycodes.insert(code);
			break;
		case AxisComponent::Negative:
			m_axisBindings[input].negative.keycodes.insert(code);
			break;
		default:
			ASSERT(false);
	}
}

void Input::CreateAxisBinding(const std::string& input, GamepadButton code, AxisComponent axis)
{
	switch (axis)
	{
		case AxisComponent::Positive:
			m_axisBindings[input].positive.controllerButtons.insert(code);
			break;
		case AxisComponent::Negative:
			m_axisBindings[input].negative.controllerButtons.insert(code);
			break;
		default:
			ASSERT(false);
	}
}

void Input::CreateAxisBinding(const std::string& input, GamepadAxis code)
{
	m_axisBindings[input].controllerAxis.insert(code);
}

void Input::CreateAxis2Binding(const std::string& input, KeyboardKey code, Axis2Component axis)
{
	switch (axis)
	{
		case Axis2Component::Up:
			m_axis2Bindings[input].yAxis.positive.keycodes.insert(code);
			break;
		case Axis2Component::Down:
			m_axis2Bindings[input].yAxis.negative.keycodes.insert(code);
			break;
		case Axis2Component::Left:
			m_axis2Bindings[input].xAxis.negative.keycodes.insert(code);
			break;
		case Axis2Component::Right:
			m_axis2Bindings[input].xAxis.positive.keycodes.insert(code);
			break;
		default:
			ASSERT(false);
	}
}

void Input::CreateAxis2Binding(const std::string& input, GamepadButton code, Axis2Component axis)
{
	switch (axis)
	{
		case Axis2Component::Up:
			m_axis2Bindings[input].yAxis.positive.controllerButtons.insert(code);
			break;
		case Axis2Component::Down:
			m_axis2Bindings[input].yAxis.negative.controllerButtons.insert(code);
			break;
		case Axis2Component::Left:
			m_axis2Bindings[input].xAxis.negative.controllerButtons.insert(code);
			break;
		case Axis2Component::Right:
			m_axis2Bindings[input].xAxis.positive.controllerButtons.insert(code);
			break;
		default:
			ASSERT(false);
	}
}

void Input::CreateAxis2Binding(const std::string& input, GamepadAxis code, Axis2Direction axis)
{
	switch (axis)
	{
		case Axis2Direction::X:
			m_axis2Bindings[input].xAxis.controllerAxis.insert(code);
			break;
		case Axis2Direction::Y:
			m_axis2Bindings[input].yAxis.controllerAxis.insert(code);
			break;
		default:
			ASSERT(false);
	}
}

// ============================================================================
// HIGH-LEVEL BUTTON INPUT
// ============================================================================

bool Input::IsInputDown(const std::string& input)
{
	return CheckBinding(
	    input, [this](KeyboardKey k) { return IsKeyDown(k); },
	    [this](GamepadButton b) { return IsControllerButtonDown(b); });
}

bool Input::IsInputJustPressed(const std::string& input)
{
	return CheckBinding(
	    input, [this](KeyboardKey k) { return IsKeyJustPressed(k); },
	    [this](GamepadButton b) { return IsControllerButtonJustPressed(b); });
}

bool Input::IsInputJustReleased(const std::string& input)
{
	return CheckBinding(
	    input, [this](KeyboardKey k) { return IsKeyJustReleased(k); },
	    [this](GamepadButton b) { return IsControllerButtonJustReleased(b); });
}

// ============================================================================
// AXIS CALCULATION HELPER
// ============================================================================

float Input::CalculateAxisValue(const AxisBinding& binding)
{
	float positive = 0.0f;

	// Check positive keyboard inputs
	for (auto keycode : binding.positive.keycodes)
	{
		if (IsKeyDown(keycode))
		{
			positive = 1.0f;
			break;
		}
	}

	// Check positive controller button inputs
	if (positive == 0.0f)
	{
		for (auto controllerButton : binding.positive.controllerButtons)
		{
			if (IsControllerButtonDown(controllerButton))
			{
				positive = 1.0f;
				break;
			}
		}
	}

	float negative = 0.0f;

	// Check negative keyboard inputs
	for (auto keycode : binding.negative.keycodes)
	{
		if (IsKeyDown(keycode))
		{
			negative = 1.0f;
			break;
		}
	}

	// Check negative controller button inputs
	if (negative == 0.0f)
	{
		for (auto controllerButton : binding.negative.controllerButtons)
		{
			if (IsControllerButtonDown(controllerButton))
			{
				negative = 1.0f;
				break;
			}
		}
	}

	float value = positive - negative;

	// Add analog stick contribution
	for (auto controllerAxis : binding.controllerAxis)
	{
		value += GetControllerAxisValue(controllerAxis);
	}

	// Clamp between -1 and 1
	return std::clamp(value, -1.0f, 1.0f);
}

// ============================================================================
// HIGH-LEVEL AXIS/VARIABLE INPUT
// ============================================================================

float Input::GetInputVariable(const std::string& input)
{
	auto it = m_variableBindings.find(input);
	ASSERT_MSG(it != m_variableBindings.end(), std::format("Variable binding '{}' not found", input).c_str());

	float value = 0.0f;

	// Check button bindings (keyboard)
	for (auto keycode : it->second.buttonBindings.keycodes)
	{
		if (IsKeyDown(keycode))
		{
			value = 1.0f;
			break;
		}
	}

	// Check button bindings (controller)
	if (value == 0.0f)
	{
		for (auto controllerButton : it->second.buttonBindings.controllerButtons)
		{
			if (IsControllerButtonDown(controllerButton))
			{
				value = 1.0f;
				break;
			}
		}
	}

	// Add controller variable contributions
	for (const auto& controllerVar : it->second.controllerVariables)
	{
		value += GetControllerVariableValue(controllerVar.controllerAxis, controllerVar.variableBindingAxis);
	}

	// Clamp to [0, 1]
	return std::clamp(value, 0.0f, 1.0f);
}

float Input::GetInputAxis(const std::string& input)
{
	auto it = m_axisBindings.find(input);
	ASSERT_MSG(it != m_axisBindings.end(), std::format("Axis binding '{}' not found", input).c_str());

	return CalculateAxisValue(it->second);
}

glm::vec2 Input::GetInputAxis2(const std::string& input)
{
	auto it = m_axis2Bindings.find(input);
	ASSERT_MSG(it != m_axis2Bindings.end(), std::format("Axis2 binding '{}' not found", input).c_str());

	float xAxis = CalculateAxisValue(it->second.xAxis);
	float yAxis = CalculateAxisValue(it->second.yAxis);

	return ApplyRadialDeadzone(glm::vec2{xAxis, yAxis});
}

}  // namespace Struktur::Input
