#include "Input.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_touch.h>

#include <cmath>

#include "InputConfigLoader.h"
#include "InputMaps.h"

namespace
{
// SDL3 identifies gamepads by a (non-sequential) joystick instance ID rather than a flat 0-based index, so this
// re-derives "the Nth currently connected gamepad" from SDL_GetGamepads()'s list.
SDL_JoystickID GetGamepadInstanceIdAtIndex(int index)
{
	int count                = 0;
	SDL_JoystickID* gamepads = ::SDL_GetGamepads(&count);
	SDL_JoystickID result    = 0;
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

	RegisterDefaultUIBindings();
}

void Input::RegisterDefaultUIBindings()
{
	// Mirrors InputConfig.json's own UIDir/UITab/UIAccept/UICancel bindings (assets/Settings/InputBindings/
	// InputConfig.json) - a successfully-loaded config's own entries for these names simply overwrite these
	// afterward. Deliberately real SDL enum constants directly rather than routing through InputMaps' string
	// parsing (that layer exists for the JSON loader's benefit, not needed for a fixed C++-side default set) -
	// and deliberately SDL_GAMEPAD_BUTTON_LEFT/RIGHT_SHOULDER rather than mirroring the JSON's own "lbumper"/
	// "rbumper" strings for UITab, which InputMaps has no mapping for (resolves to SDL_GAMEPAD_BUTTON_INVALID,
	// a pre-existing no-op in the JSON config, not something worth reproducing here).
	CreateAxis2Binding("UIDir", SDL_SCANCODE_UP, Axis2Component::Up);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_W, Axis2Component::Up);
	CreateAxis2Binding("UIDir", SDL_GAMEPAD_BUTTON_DPAD_UP, Axis2Component::Up);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_DOWN, Axis2Component::Down);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_S, Axis2Component::Down);
	CreateAxis2Binding("UIDir", SDL_GAMEPAD_BUTTON_DPAD_DOWN, Axis2Component::Down);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_LEFT, Axis2Component::Left);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_A, Axis2Component::Left);
	CreateAxis2Binding("UIDir", SDL_GAMEPAD_BUTTON_DPAD_LEFT, Axis2Component::Left);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_RIGHT, Axis2Component::Right);
	CreateAxis2Binding("UIDir", SDL_SCANCODE_D, Axis2Component::Right);
	CreateAxis2Binding("UIDir", SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Axis2Component::Right);

	CreateAxisBinding("UITab", SDL_SCANCODE_Q, AxisComponent::Negative);
	CreateAxisBinding("UITab", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, AxisComponent::Negative);
	CreateAxisBinding("UITab", SDL_SCANCODE_E, AxisComponent::Positive);
	CreateAxisBinding("UITab", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, AxisComponent::Positive);

	CreateButtonBinding("UIAccept", SDL_SCANCODE_SPACE);
	CreateButtonBinding("UIAccept", SDL_SCANCODE_RETURN);
	CreateButtonBinding("UIAccept", SDL_GAMEPAD_BUTTON_SOUTH);
	// Click/tap activates the hovered element the same way Space/Enter/GamepadA activates the focused one - see
	// the pointer design's own reasoning (PointerButton, UIManager's hover/click wiring) for why this shares
	// UIAccept rather than being a separate action.
	CreateButtonBinding("UIAccept", PointerButton::Primary);

	CreateButtonBinding("UICancel", SDL_SCANCODE_ESCAPE);
	CreateButtonBinding("UICancel", SDL_GAMEPAD_BUTTON_EAST);
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
			m_sdlGamepad          = ::SDL_OpenGamepad(instanceId);
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
	m_prevKeyboardState       = m_currKeyboardState;
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
	    SDL_GAMEPAD_BUTTON_DPAD_UP,        SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_DOWN,
	    SDL_GAMEPAD_BUTTON_DPAD_LEFT,      SDL_GAMEPAD_BUTTON_NORTH,      SDL_GAMEPAD_BUTTON_EAST,
	    SDL_GAMEPAD_BUTTON_SOUTH,          SDL_GAMEPAD_BUTTON_WEST,       SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
	    SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_BACK,       SDL_GAMEPAD_BUTTON_GUIDE,
	    SDL_GAMEPAD_BUTTON_START,          SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK,
	};
	for (SDL_GamepadButton button : kAllButtons)
	{
		m_prevControllerButtons[button] = m_currControllerButtons[button];
		m_currControllerButtons[button] = IsControllerButtonDown(button);
	}

	// Pointer - mouse position/buttons are polled here (SDL3's SDL_GetMouseState is a live global query, same
	// shape as SDL_GetKeyboardState above), touch position/down-state is instead accumulated from HandleEvent
	// (SDL3 only reports touch through the event stream, no equivalent poll function) and simply left alone
	// here while a touch is active, so a platform's synthetic touch-driven mouse events (many platforms emit
	// these by default) can't fight the real touch position for control of m_pointerPosition within one frame.
	m_prevPointerButtons = m_currPointerButtons;
	if (!m_hasActiveTouch)
	{
		float mouseX = 0.0f, mouseY = 0.0f;
		SDL_MouseButtonFlags buttons = ::SDL_GetMouseState(&mouseX, &mouseY);
		m_pointerPosition           = glm::vec2(mouseX, mouseY);
		m_currPointerButtons[(size_t)PointerButton::Primary]   = (buttons & SDL_BUTTON_LMASK) != 0;
		m_currPointerButtons[(size_t)PointerButton::Secondary] = (buttons & SDL_BUTTON_RMASK) != 0;
		m_currPointerButtons[(size_t)PointerButton::Middle]    = (buttons & SDL_BUTTON_MMASK) != 0;
	}
}

void Input::Clear()
{
	m_buttonBindings.clear();
	m_variableBindings.clear();
	m_axisBindings.clear();
	m_axis2Bindings.clear();
	m_deadzone = 0.0f;

	// Re-establish the reserved UI action names immediately, not just once at construction -
	// InputConfigLoader::LoadFromFile calls Clear() right before loading a config file's own bindings
	// (InputConfigLoader.cpp), and if that file turns out to be missing its "inputs" array (or isn't found at
	// all), LoadFromFile bails out having already wiped everything Clear() just did. Re-registering here means
	// UIDir/UITab/UIAccept/UICancel survive that specific failure too, not only "the config load was never
	// attempted at all" (Game.start() erroring out before reaching Input.loadInputBindings). A config that DOES
	// define these names simply overwrites the defaults again right after, same as always.
	RegisterDefaultUIBindings();
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

Input::PointerButton Input::ParsePointerButton(const std::string& button)
{
	if (button == "primary")
	{
		return PointerButton::Primary;
	}
	if (button == "secondary")
	{
		return PointerButton::Secondary;
	}
	if (button == "middle")
	{
		return PointerButton::Middle;
	}

	ASSERT_MSG(false, "Invalid pointer button: '%s'", button);
	return PointerButton::Primary;
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

bool Input::IsPointerButtonDown(PointerButton button)
{
	return m_currPointerButtons[(size_t)button];
}

bool Input::IsPointerButtonJustPressed(PointerButton button)
{
	return m_currPointerButtons[(size_t)button] && !m_prevPointerButtons[(size_t)button];
}

bool Input::IsPointerButtonJustReleased(PointerButton button)
{
	return !m_currPointerButtons[(size_t)button] && m_prevPointerButtons[(size_t)button];
}

void Input::HandleEvent(const SDL_Event& event)
{
	// Only the first finger to touch down is tracked - see m_hasActiveTouch's own comment. A second
	// simultaneous finger is simply ignored (real multi-touch is out of scope, see the pointer design's own
	// explicit non-goals) rather than fighting the first for control of the pointer.
	switch (event.type)
	{
		case SDL_EVENT_FINGER_DOWN:
			if (!m_hasActiveTouch)
			{
				m_hasActiveTouch = true;
				m_activeTouchId  = event.tfinger.fingerID;
			}
			if (event.tfinger.fingerID == m_activeTouchId)
			{
				m_pointerPosition = glm::vec2(event.tfinger.x * (float)m_windowWidth, event.tfinger.y * (float)m_windowHeight);
				m_currPointerButtons[(size_t)PointerButton::Primary] = true;
			}
			break;
		case SDL_EVENT_FINGER_MOTION:
			if (m_hasActiveTouch && event.tfinger.fingerID == m_activeTouchId)
			{
				m_pointerPosition = glm::vec2(event.tfinger.x * (float)m_windowWidth, event.tfinger.y * (float)m_windowHeight);
			}
			break;
		case SDL_EVENT_FINGER_UP:
		case SDL_EVENT_FINGER_CANCELED:
			if (m_hasActiveTouch && event.tfinger.fingerID == m_activeTouchId)
			{
				m_hasActiveTouch                                     = false;
				m_currPointerButtons[(size_t)PointerButton::Primary] = false;
			}
			break;
		default:
			break;
	}
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

void Input::CreateButtonBinding(const std::string& input, PointerButton code)
{
	m_buttonBindings[input].pointerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, SDL_Scancode code)
{
	m_variableBindings[input].buttonBindings.keycodes.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, SDL_GamepadButton code)
{
	m_variableBindings[input].buttonBindings.controllerButtons.insert(code);
}

void Input::CreateVariableBinding(const std::string& input, SDL_GamepadAxis code,
                                  VariableBindingAxis variableBindingAxis)
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
	    [this](SDL_GamepadButton b) { return IsControllerButtonDown(b); },
	    [this](PointerButton p) { return IsPointerButtonDown(p); });
}

bool Input::IsInputJustPressed(const std::string& input)
{
	return CheckBinding(
	    input, [this](SDL_Scancode k) { return IsKeyJustPressed(k); },
	    [this](SDL_GamepadButton b) { return IsControllerButtonJustPressed(b); },
	    [this](PointerButton p) { return IsPointerButtonJustPressed(p); });
}

bool Input::IsInputJustReleased(const std::string& input)
{
	return CheckBinding(
	    input, [this](SDL_Scancode k) { return IsKeyJustReleased(k); },
	    [this](SDL_GamepadButton b) { return IsControllerButtonJustReleased(b); },
	    [this](PointerButton p) { return IsPointerButtonJustReleased(p); });
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

void Input::WarnMissingBindingOnce(const std::string& input)
{
	// insert() returns {iterator, false} if already present - only logs the first time a given name is queried
	// and found missing, so a binding that's genuinely absent (polled every frame, e.g. UIDir/UIAccept before
	// this whole safe-default fix existed) doesn't flood the log every single frame.
	if (m_warnedMissingBindings.insert(input).second)
	{
		DEBUG_ERROR("Input binding '%s' not found - returning a safe default instead of asserting", input);
	}
}

float Input::GetInputVariable(const std::string& input)
{
	auto it = m_variableBindings.find(input);
	if (it == m_variableBindings.end())
	{
		WarnMissingBindingOnce(input);
		return 0.0f;
	}

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
	if (it == m_axisBindings.end())
	{
		WarnMissingBindingOnce(input);
		return 0.0f;
	}

	return CalculateAxisValue(it->second);
}

glm::vec2 Input::GetInputAxis2(const std::string& input)
{
	auto it = m_axis2Bindings.find(input);
	if (it == m_axis2Bindings.end())
	{
		WarnMissingBindingOnce(input);
		return glm::vec2(0.0f, 0.0f);
	}

	float xAxis = CalculateAxisValue(it->second.xAxis);
	float yAxis = CalculateAxisValue(it->second.yAxis);

	return ApplyRadialDeadzone(glm::vec2{xAxis, yAxis});
}

}  // namespace Struktur::Input
