#include "Input.h"

#include <cmath>

#include "InputConfigLoader.h"
#include "InputMaps.h"

#include <SDL3/SDL.h>

namespace
{
// SDL3 identifies gamepads by a (non-sequential) joystick instance ID rather than a flat 0-based index, so this
// re-derives "the Nth currently connected gamepad" from SDL_GetGamepads()'s list.
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
		DEBUG_INFO("Gamepad %d '%s' successfully connected.", m_gamepadIndex, m_gamepadId);
	}
	else
	{
		DEBUG_WARNING("Gamepad %d is not connected.", m_gamepadIndex);
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
				DEBUG_INFO("Gamepad %d changed to '%s'", m_gamepadIndex, m_gamepadId);
			}
		}
	}
	else if (!m_gamepadId.empty())
	{
		// Gamepad was connected but is now disconnected
		DEBUG_WARNING("Gamepad %d '%s' disconnected", m_gamepadIndex, m_gamepadId);
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
	// LEFT_TRIGGER/RIGHT_TRIGGER are excluded - they're analog axes in SDL3 (SDL_GAMEPAD_AXIS_LEFT/RIGHT_TRIGGER),
	// not digital buttons; GetControllerAxisValue() is the correct query for those.
	static const SDL_GamepadButton kAllButtons[] = {
	    SDL_GAMEPAD_BUTTON_DPAD_UP,        SDL_GAMEPAD_BUTTON_DPAD_RIGHT,    SDL_GAMEPAD_BUTTON_DPAD_DOWN,
	    SDL_GAMEPAD_BUTTON_DPAD_LEFT,      SDL_GAMEPAD_BUTTON_NORTH,        SDL_GAMEPAD_BUTTON_EAST,
	    SDL_GAMEPAD_BUTTON_SOUTH,          SDL_GAMEPAD_BUTTON_WEST,         SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
	    SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_BACK,         SDL_GAMEPAD_BUTTON_GUIDE,
	    SDL_GAMEPAD_BUTTON_START,          SDL_GAMEPAD_BUTTON_LEFT_STICK,   SDL_GAMEPAD_BUTTON_RIGHT_STICK,
	};
	for (SDL_GamepadButton button : kAllButtons)
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
		DEBUG_ERROR("Failed to load input bindings from: %s", file);
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
		DEBUG_INFO("Switched to gamepad %d '%s'", m_gamepadIndex, m_gamepadId);
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

	ASSERT_MSG(false, "Invalid axis component: '%s'", component);
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

	ASSERT_MSG(false, "Invalid axis2 component: '%s'", component);
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

	ASSERT_MSG(false, "Invalid axis2 direction: '%s'", direction);
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

	ASSERT_MSG(false, "Invalid variable binding axis: '%s'", axis);
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

bool Input::IsKeyDown(SDL_Scancode key)
{
	return key < (int)m_currKeyboardState.size() && m_currKeyboardState[key];
}

bool Input::IsKeyJustPressed(SDL_Scancode key)
{
	return key < (int)m_currKeyboardState.size() && m_currKeyboardState[key] && !m_prevKeyboardState[key];
}

bool Input::IsKeyJustReleased(SDL_Scancode key)
{
	return key < (int)m_currKeyboardState.size() && !m_currKeyboardState[key] && m_prevKeyboardState[key];
}

bool Input::IsControllerButtonDown(SDL_GamepadButton button)
{
	return m_sdlGamepad && ::SDL_GetGamepadButton(m_sdlGamepad, button);
}

bool Input::IsControllerButtonJustPressed(SDL_GamepadButton button)
{
	// SDL3 has no polling-based "just pressed" query (that's event-driven, via SDL_EVENT_GAMEPAD_BUTTON_DOWN);
	// approximate it the same way keyboard edge detection works, off the same per-frame Update() snapshot.
	return IsControllerButtonDown(button) && !WasControllerButtonDownLastFrame(button);
}

bool Input::IsControllerButtonJustReleased(SDL_GamepadButton button)
{
	return !IsControllerButtonDown(button) && WasControllerButtonDownLastFrame(button);
}

bool Input::WasControllerButtonDownLastFrame(SDL_GamepadButton button)
{
	auto it = m_prevControllerButtons.find(button);
	return it != m_prevControllerButtons.end() && it->second;
}

float Input::GetControllerAxisValue(SDL_GamepadAxis code)
{
	if (!m_sdlGamepad)
	{
		return 0.0f;
	}
	float rawValue = ::SDL_GetGamepadAxis(m_sdlGamepad, code) / 32767.0f;
	return ApplyDeadzone(rawValue);
}

// FIXED: Added break statements
float Input::GetControllerVariableValue(SDL_GamepadAxis code, VariableBindingAxis variableBindingAxis)
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
	SDL_Scancode key = InputMaps::GetKeycodeFromString(input);
	return IsKeyDown(key);
}

bool Input::IsStringKeyJustPressed(const std::string& input)
{
	SDL_Scancode key = InputMaps::GetKeycodeFromString(input);
	return IsKeyJustPressed(key);
}

bool Input::IsStringKeyJustReleased(const std::string& input)
{
	SDL_Scancode key = InputMaps::GetKeycodeFromString(input);
	return IsKeyJustReleased(key);
}

bool Input::IsStringControllerButtonDown(const std::string& input)
{
	SDL_GamepadButton button = InputMaps::GetControllerButtonFromString(input);
	return IsControllerButtonDown(button);
}

bool Input::IsStringControllerButtonJustPressed(const std::string& input)
{
	SDL_GamepadButton button = InputMaps::GetControllerButtonFromString(input);
	return IsControllerButtonJustPressed(button);
}

bool Input::IsStringControllerButtonJustReleased(const std::string& input)
{
	SDL_GamepadButton button = InputMaps::GetControllerButtonFromString(input);
	return IsControllerButtonJustReleased(button);
}

float Input::GetStringControllerAxisValue(const std::string& input)
{
	SDL_GamepadAxis axis = InputMaps::GetControllerAxisFromString(input);
	return GetControllerAxisValue(axis);
}

// ============================================================================
// BINDING CREATION
// ============================================================================

void Input::CreateButtonBinding(const std::string& input, SDL_Scancode code)
{
	m_buttonBindings[input].keycodes.insert(code);
}

void Input::CreateButtonBinding(const std::string& input, SDL_GamepadButton code)
{
	m_buttonBindings[input].controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, SDL_Scancode code)
{
	m_variableBindings[input].buttonBindings.keycodes.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, SDL_GamepadButton code)
{
	m_variableBindings[input].buttonBindings.controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, SDL_GamepadAxis code, VariableBindingAxis variableBindingAxis)
{
	m_variableBindings[input].controllerVariables.insert({variableBindingAxis, code});
}

void Input::CreateAxisBinding(const std::string& input, SDL_Scancode code, AxisComponent axis)
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

void Input::CreateAxisBinding(const std::string& input, SDL_GamepadButton code, AxisComponent axis)
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

void Input::CreateAxisBinding(const std::string& input, SDL_GamepadAxis code)
{
	m_axisBindings[input].controllerAxis.insert(code);
}

void Input::CreateAxis2Binding(const std::string& input, SDL_Scancode code, Axis2Component axis)
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

void Input::CreateAxis2Binding(const std::string& input, SDL_GamepadButton code, Axis2Component axis)
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

void Input::CreateAxis2Binding(const std::string& input, SDL_GamepadAxis code, Axis2Direction axis)
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
	    input, [this](SDL_Scancode k) { return IsKeyDown(k); },
	    [this](SDL_GamepadButton b) { return IsControllerButtonDown(b); });
}

bool Input::IsInputJustPressed(const std::string& input)
{
	return CheckBinding(
	    input, [this](SDL_Scancode k) { return IsKeyJustPressed(k); },
	    [this](SDL_GamepadButton b) { return IsControllerButtonJustPressed(b); });
}

bool Input::IsInputJustReleased(const std::string& input)
{
	return CheckBinding(
	    input, [this](SDL_Scancode k) { return IsKeyJustReleased(k); },
	    [this](SDL_GamepadButton b) { return IsControllerButtonJustReleased(b); });
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
	ASSERT_MSG(it != m_variableBindings.end(), "Variable binding '%s' not found", input);

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
	ASSERT_MSG(it != m_axisBindings.end(), "Axis binding '%s' not found", input);

	return CalculateAxisValue(it->second);
}

glm::vec2 Input::GetInputAxis2(const std::string& input)
{
	auto it = m_axis2Bindings.find(input);
	ASSERT_MSG(it != m_axis2Bindings.end(), "Axis2 binding '%s' not found", input);

	float xAxis = CalculateAxisValue(it->second.xAxis);
	float yAxis = CalculateAxisValue(it->second.yAxis);

	return ApplyRadialDeadzone(glm::vec2{xAxis, yAxis});
}

}  // namespace Struktur::Input
