#pragma once

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <unordered_map>

#include "Debug/Assertions.h"
#include "glm/glm.hpp"

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

private:
	struct Binding
	{
		std::set<SDL_Scancode> keycodes;
		std::set<SDL_GamepadButton> controllerButtons;
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

	// Helper functions to reduce code duplication
	// NOTE: Template must be defined in header
	template <typename KeyFunc, typename ButtonFunc>
	bool CheckBinding(const std::string& input, KeyFunc keyCheck, ButtonFunc buttonCheck)
	{
		auto it = m_buttonBindings.find(input);
		ASSERT_MSG(it != m_buttonBindings.end(), "Binding '%s' not found", input);

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

		return false;
	}

	float CalculateAxisValue(const AxisBinding& binding);
	float ApplyDeadzone(float value) const;
	glm::vec2 ApplyRadialDeadzone(glm::vec2 value) const;

	// Make InputConfigLoader a friend so it can call private helper methods
	friend class InputConfigLoader;
};
}  // namespace Struktur::Input
