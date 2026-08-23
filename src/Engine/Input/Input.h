#pragma once

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>

#include "Debug/Assertions.h"
#include "glm/glm.hpp"

union SDL_Event;

namespace Struktur::Input
{
class InputConfigLoader;

class Input
{
public:
	// Enums need to be public for InputConfigLoader
	enum class VariableBindingAxis
	{
		Positive,
		Negative,
		NegativeToPositive,
		PositiveToNegative,

		Count
	};

	enum class AxisComponent
	{
		Positive,
		Negative,

		Count
	};

	enum class Axis2Component
	{
		Up,
		Down,
		Left,
		Right,

		Count
	};

	enum class Axis2Direction
	{
		X,
		Y,

		Count
	};

	// The pointer is one unified concept covering both real mouse buttons and touch - see
	// GetPointerPosition()/CreateButtonBinding(PointerButton)'s own comments. Primary fires from either the
	// mouse's left button or an active touch; Secondary/Middle only ever fire from a real mouse (touch has no
	// equivalent).
	enum class PointerButton
	{
		Primary,
		Secondary,
		Middle,

		Count
	};

private:
	struct Binding
	{
		std::set<SDL_Scancode> keycodes;
		std::set<SDL_GamepadButton> controllerButtons;
		std::set<PointerButton> pointerButtons;
	};

	struct AxisBinding
	{
		Binding positive;
		Binding negative;
		std::set<SDL_GamepadAxis> controllerAxis;
	};

	struct GamepadVariable
	{
		VariableBindingAxis variableBindingAxis;
		SDL_GamepadAxis controllerAxis;

		// Comparison operator for std::set
		bool operator<(const GamepadVariable& other) const
		{
			if (controllerAxis != other.controllerAxis)
			{
				return controllerAxis < other.controllerAxis;
			}
			return variableBindingAxis < other.variableBindingAxis;
		}
	};

	struct VariableBinding
	{
		Binding buttonBindings;
		std::set<GamepadVariable> controllerVariables;
	};

	struct Axis2Binding
	{
		AxisBinding xAxis;
		AxisBinding yAxis;
	};

public:
	Input(int gamepadIndex = 0);
	~Input();

	void Update();

	void LoadInputBindings(const std::string& file);
	void Clear();

	// Raw input queries
	bool IsKeyDown(SDL_Scancode key);
	bool IsKeyJustPressed(SDL_Scancode key);
	bool IsKeyJustReleased(SDL_Scancode key);

	bool IsControllerButtonDown(SDL_GamepadButton button);
	bool IsControllerButtonJustPressed(SDL_GamepadButton button);
	bool IsControllerButtonJustReleased(SDL_GamepadButton button);

	float GetControllerAxisValue(SDL_GamepadAxis code);
	float GetControllerVariableValue(SDL_GamepadAxis code, VariableBindingAxis variableBindingAxis);

	// Pointer (mouse + touch, unified - see PointerButton's own comment) raw queries. Position is a plain
	// accessor, not a binding: "where is the pointer" has no remap meaning the way a button/axis does, mirroring
	// how there's no "binding" for raw keyboard scancode state either - GetPointerPosition() is this class's
	// equivalent of IsKeyDown() for the pointer. Fed by SDL3 mouse-motion (polled in Update(), same as keyboard/
	// gamepad) and touch-finger events (received via HandleEvent(), since SDL3 only reports touch through the
	// event stream, not a pollable global state function the way mouse position/buttons are) - whichever moved
	// most recently wins, so this is one coordinate regardless of source.
	glm::vec2 GetPointerPosition() const
	{
		return m_pointerPosition;
	}
	bool IsPointerButtonDown(PointerButton button);
	bool IsPointerButtonJustPressed(PointerButton button);
	bool IsPointerButtonJustReleased(PointerButton button);

	// Feeds touch-finger SDL events into the pointer state (see GetPointerPosition's own comment) - register
	// this with Platform::Window::AddEventCallback so it sees the same event stream ImGui's own callback does.
	// Mouse motion/buttons don't need this - they're polled in Update() like keyboard/gamepad already are.
	void HandleEvent(const SDL_Event& event);

	// SDL3 reports touch finger positions normalized to the window (0-1), but GetPointerPosition() needs to
	// return the same window-relative pixel space SDL_GetMouseState already uses for the mouse - this is the
	// one piece of window info Input needs to do that conversion. A plain width/height setter (not a stored
	// Platform::Window& reference) keeps Input decoupled from Window the same way it already is for everything
	// else (keyboard/gamepad are read via bare global SDL calls, no window handle needed there either). Call
	// this once per frame (window size can change on resize) - see Game.cpp's main loop.
	void SetWindowSize(int width, int height)
	{
		m_windowWidth  = width;
		m_windowHeight = height;
	}

	// String-based raw input (for editor/debug)
	bool IsStringKeyDown(const std::string& input);
	bool IsStringKeyJustPressed(const std::string& input);
	bool IsStringKeyJustReleased(const std::string& input);

	bool IsStringControllerButtonDown(const std::string& input);
	bool IsStringControllerButtonJustPressed(const std::string& input);
	bool IsStringControllerButtonJustReleased(const std::string& input);

	float GetStringControllerAxisValue(const std::string& input);

	// Binding creation
	void CreateButtonBinding(const std::string& input, SDL_Scancode code);
	void CreateButtonBinding(const std::string& input, SDL_GamepadButton code);
	void CreateButtonBinding(const std::string& input, PointerButton code);

	void CreateVariableBinding(const std::string& input, SDL_Scancode code);
	void CreateVariableBinding(const std::string& input, SDL_GamepadButton code);
	void CreateVariableBinding(const std::string& input, SDL_GamepadAxis code, VariableBindingAxis variableBindingAxis);

	void CreateAxisBinding(const std::string& input, SDL_Scancode code, AxisComponent axis);
	void CreateAxisBinding(const std::string& input, SDL_GamepadButton code, AxisComponent axis);
	void CreateAxisBinding(const std::string& input, SDL_GamepadAxis code);

	void CreateAxis2Binding(const std::string& input, SDL_Scancode code, Axis2Component axis);
	void CreateAxis2Binding(const std::string& input, SDL_GamepadButton code, Axis2Component axis);
	void CreateAxis2Binding(const std::string& input, SDL_GamepadAxis code, Axis2Direction axis);

	// High-level input queries (use these in game code)
	bool IsInputDown(const std::string& input);
	bool IsInputJustPressed(const std::string& input);
	bool IsInputJustReleased(const std::string& input);
	float GetInputVariable(const std::string& input);
	float GetInputAxis(const std::string& input);
	glm::vec2 GetInputAxis2(const std::string& input);

	// Gamepad management
	void SetGamepadIndex(int index);
	int GetGamepadIndex() const
	{
		return m_gamepadIndex;
	}
	bool IsGamepadConnected() const;
	const std::string& GetGamepadId() const
	{
		return m_gamepadId;
	}

	// Deadzone configuration
	void SetDeadzone(float deadzone)
	{
		m_deadzone = deadzone;
	}
	float GetDeadzone() const
	{
		return m_deadzone;
	}

	// Parsing helpers (public for InputConfigLoader)
	static AxisComponent ParseAxisComponent(const std::string& component);
	static Axis2Component ParseAxis2Component(const std::string& component);
	static Axis2Direction ParseAxis2Direction(const std::string& direction);
	static VariableBindingAxis ParseVariableBindingAxis(const std::string& axis);
	static PointerButton ParsePointerButton(const std::string& button);

private:
	// Binding storage
	std::unordered_map<std::string, Binding> m_buttonBindings;
	std::unordered_map<std::string, VariableBinding> m_variableBindings;
	std::unordered_map<std::string, AxisBinding> m_axisBindings;
	std::unordered_map<std::string, Axis2Binding> m_axis2Bindings;

	// Gamepad state
	float m_deadzone;
	std::string m_gamepadId;
	int m_gamepadIndex;
	SDL_Gamepad* m_sdlGamepad = nullptr;
	// Two snapshots (this frame vs last frame) so IsKeyJustPressed/Released can edge-detect - SDL's own
	// SDL_GetKeyboardState() is a live array with no history.
	std::array<bool, SDL_SCANCODE_COUNT> m_currKeyboardState{};
	std::array<bool, SDL_SCANCODE_COUNT> m_prevKeyboardState{};
	std::unordered_map<SDL_GamepadButton, bool> m_currControllerButtons;
	std::unordered_map<SDL_GamepadButton, bool> m_prevControllerButtons;
	bool WasControllerButtonDownLastFrame(SDL_GamepadButton button);

	// Pointer (mouse + touch) state - see GetPointerPosition's own comment. Same current/previous-frame
	// snapshot shape as keyboard/gamepad above, so IsPointerButtonJustPressed/Released can edge-detect the same
	// way. Indexed by PointerButton.
	glm::vec2 m_pointerPosition{0.0f, 0.0f};
	std::array<bool, (size_t)PointerButton::Count> m_currPointerButtons{};
	std::array<bool, (size_t)PointerButton::Count> m_prevPointerButtons{};
	// Set while at least one finger is down (HandleEvent tracks the first one by its SDL finger ID, stored as a
	// plain uint64_t so this header doesn't need <SDL3/SDL_touch.h>) - lets Update()'s mouse-motion poll avoid
	// stomping a touch-driven position with a stale/synthetic mouse position on platforms that also emit
	// synthetic mouse events for touch.
	bool m_hasActiveTouch = false;
	uint64_t m_activeTouchId = 0;
	// See SetWindowSize's own comment - defaults to 1x1 rather than 0x0 purely to avoid a multiply-by-zero
	// producing an always-(0,0) touch position if a touch event somehow arrives before the first SetWindowSize
	// call; harmless either way since real touch input can't happen before a real window exists.
	int m_windowWidth  = 1;
	int m_windowHeight = 1;

	// See CheckBinding/GetInputAxis/GetInputAxis2/GetInputVariable - logs once per unique missing binding name
	// instead of asserting-then-continuing-into-undefined-behavior (the previous behavior: ASSERT_MSG only
	// hard-stops in Debug builds, so Release builds fell straight through into dereferencing an end() iterator).
	// A single set shared across all four binding kinds is fine - collisions here would only mean a name used as
	// e.g. both a button and an axis somewhere, which would be a real config bug worth surfacing once either way.
	std::set<std::string> m_warnedMissingBindings;
	void WarnMissingBindingOnce(const std::string& input);

	// Registers C++-side defaults for the reserved UI action names (UIDir/UITab/UIAccept/UICancel), mirroring
	// InputConfig.json's own bindings for them exactly - called from both the constructor and Clear() (see
	// Clear()'s own comment) so these four names can never be missing regardless of whether/when a config file
	// successfully loads. A config that does define them simply overwrites these afterward, same as always.
	void RegisterDefaultUIBindings();

	// Helper functions to reduce code duplication
	// NOTE: Template must be defined in header
	template <typename KeyFunc, typename ButtonFunc, typename PointerFunc>
	bool CheckBinding(const std::string& input, KeyFunc keyCheck, ButtonFunc buttonCheck, PointerFunc pointerCheck)
	{
		auto it = m_buttonBindings.find(input);
		if (it == m_buttonBindings.end())
		{
			WarnMissingBindingOnce(input);
			return false;
		}

		// Check keyboard inputs
		for (auto keycode : it->second.keycodes)
		{
			if (keyCheck(keycode))
			{
				return true;
			}
		}

		// Check controller button inputs
		for (auto controllerButton : it->second.controllerButtons)
		{
			if (buttonCheck(controllerButton))
			{
				return true;
			}
		}

		// Check pointer (mouse/touch) button inputs - see PointerButton's own comment.
		for (auto pointerButton : it->second.pointerButtons)
		{
			if (pointerCheck(pointerButton))
			{
				return true;
			}
		}

		return false;
	}

	float CalculateAxisValue(const AxisBinding& binding);
	float ApplyDeadzone(float value) const;
	glm::vec2 ApplyRadialDeadzone(glm::vec2 value) const;

	// Make InputConfigLoader a friend so it can call private helper methods
	friend class InputConfigLoader;
};
}  // namespace Struktur::Input
