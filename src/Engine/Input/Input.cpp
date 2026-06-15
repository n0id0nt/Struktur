#include "Input.h"

#include <cmath>

#include "InputConfigLoader.h"
#include "InputMaps.h"

namespace Struktur::Input
{

// ============================================================================
// CONSTRUCTOR & LIFECYCLE
// ============================================================================

Input::Input(int gamepadIndex)
    : m_deadzone(0.0f),
      m_gamepadIndex(gamepadIndex)
{
	if (IsGamepadAvailable(m_gamepadIndex))
	{
		m_gamepadId = GetGamepadName(m_gamepadIndex);
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
}

void Input::Update()
{
	// Update gamepad connection state
	if (IsGamepadAvailable(m_gamepadIndex))
	{
		std::string currentId = GetGamepadName(m_gamepadIndex);
		if (currentId != m_gamepadId)
		{
			m_gamepadId = currentId;
			DEBUG_INFO(std::format("Gamepad {} changed to '{}'", m_gamepadIndex, m_gamepadId).c_str());
		}
	}
	else if (!m_gamepadId.empty())
	{
		// Gamepad was connected but is now disconnected
		DEBUG_WARNING(std::format("Gamepad {} '{}' disconnected", m_gamepadIndex, m_gamepadId).c_str());
		m_gamepadId.clear();
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
	if (IsGamepadAvailable(m_gamepadIndex))
	{
		m_gamepadId = GetGamepadName(m_gamepadIndex);
		DEBUG_INFO(std::format("Switched to gamepad {} '{}'", m_gamepadIndex, m_gamepadId).c_str());
	}
	else
	{
		m_gamepadId.clear();
	}
}

bool Input::IsGamepadConnected() const
{
	return IsGamepadAvailable(m_gamepadIndex);
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

bool Input::IsKeyDown(::KeyboardKey key)
{
	return ::IsKeyDown(key);
}

bool Input::IsKeyJustPressed(::KeyboardKey key)
{
	return ::IsKeyPressed(key);
}

bool Input::IsKeyJustReleased(::KeyboardKey key)
{
	return ::IsKeyReleased(key);
}

bool Input::IsControllerButtonDown(::GamepadButton button)
{
	return ::IsGamepadButtonDown(m_gamepadIndex, button);
}

bool Input::IsControllerButtonJustPressed(::GamepadButton button)
{
	return ::IsGamepadButtonPressed(m_gamepadIndex, button);
}

bool Input::IsControllerButtonJustReleased(::GamepadButton button)
{
	return ::IsGamepadButtonReleased(m_gamepadIndex, button);
}

float Input::GetControllerAxisValue(::GamepadAxis code)
{
	float rawValue = ::GetGamepadAxisMovement(m_gamepadIndex, code);
	return ApplyDeadzone(rawValue);
}

// FIXED: Added break statements
float Input::GetControllerVariableValue(::GamepadAxis code, VariableBindingAxis variableBindingAxis)
{
	float rawValue = ApplyDeadzone(::GetGamepadAxisMovement(m_gamepadIndex, code));

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
	::KeyboardKey key = InputMaps::Instance().GetKeycodeFromString(input);
	return IsKeyDown(key);
}

bool Input::IsStringKeyJustPressed(const std::string& input)
{
	::KeyboardKey key = InputMaps::Instance().GetKeycodeFromString(input);
	return IsKeyJustPressed(key);
}

bool Input::IsStringKeyJustReleased(const std::string& input)
{
	::KeyboardKey key = InputMaps::Instance().GetKeycodeFromString(input);
	return IsKeyJustReleased(key);
}

bool Input::IsStringControllerButtonDown(const std::string& input)
{
	::GamepadButton button = InputMaps::Instance().GetControllerButtonFromString(input);
	return IsControllerButtonDown(button);
}

bool Input::IsStringControllerButtonJustPressed(const std::string& input)
{
	::GamepadButton button = InputMaps::Instance().GetControllerButtonFromString(input);
	return IsControllerButtonJustPressed(button);
}

bool Input::IsStringControllerButtonJustReleased(const std::string& input)
{
	::GamepadButton button = InputMaps::Instance().GetControllerButtonFromString(input);
	return IsControllerButtonJustReleased(button);
}

float Input::GetStringControllerAxisValue(const std::string& input)
{
	::GamepadAxis axis = InputMaps::Instance().GetControllerAxisFromString(input);
	return GetControllerAxisValue(axis);
}

// ============================================================================
// BINDING CREATION
// ============================================================================

void Input::CreateButtonBinding(const std::string& input, ::KeyboardKey code)
{
	m_buttonBindings[input].keycodes.insert(code);
}

void Input::CreateButtonBinding(const std::string& input, ::GamepadButton code)
{
	m_buttonBindings[input].controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, ::KeyboardKey code)
{
	m_variableBindings[input].buttonBindings.keycodes.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, ::GamepadButton code)
{
	m_variableBindings[input].buttonBindings.controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, ::GamepadAxis code, VariableBindingAxis variableBindingAxis)
{
	m_variableBindings[input].controllerVariables.insert({variableBindingAxis, code});
}

void Input::CreateAxisBinding(const std::string& input, ::KeyboardKey code, AxisComponent axis)
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

void Input::CreateAxisBinding(const std::string& input, ::GamepadButton code, AxisComponent axis)
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

void Input::CreateAxisBinding(const std::string& input, ::GamepadAxis code)
{
	m_axisBindings[input].controllerAxis.insert(code);
}

void Input::CreateAxis2Binding(const std::string& input, ::KeyboardKey code, Axis2Component axis)
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

void Input::CreateAxis2Binding(const std::string& input, ::GamepadButton code, Axis2Component axis)
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

void Input::CreateAxis2Binding(const std::string& input, ::GamepadAxis code, Axis2Direction axis)
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
	    input, [this](::KeyboardKey k) { return IsKeyDown(k); },
	    [this](::GamepadButton b) { return IsControllerButtonDown(b); });
}

bool Input::IsInputJustPressed(const std::string& input)
{
	return CheckBinding(
	    input, [this](::KeyboardKey k) { return IsKeyJustPressed(k); },
	    [this](::GamepadButton b) { return IsControllerButtonJustPressed(b); });
}

bool Input::IsInputJustReleased(const std::string& input)
{
	return CheckBinding(
	    input, [this](::KeyboardKey k) { return IsKeyJustReleased(k); },
	    [this](::GamepadButton b) { return IsControllerButtonJustReleased(b); });
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
