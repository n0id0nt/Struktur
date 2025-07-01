#pragma once
#include <set>
#include <unordered_map>
#include <string>
#include "raylib.h"
#include "glm\glm.hpp"

namespace Struktur
{
	namespace Core
	{
		class Input
		{
		private:
			struct Binding
			{
				std::set<KeyboardKey> keycodes;
				std::set<GamepadButton> controllerButtons;
			};

			struct AxisBinding
			{
				Binding positive;
				Binding negetive;
				std::set<GamepadAxis> controllerAxis;
			};

			enum class AxisComponent
			{
				Positive,
				Negetive,

				Count
			};

			struct Axis2Binding
			{
				AxisBinding xAxis;
				AxisBinding yAxis;
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
		public:

			Input(int gamer = 0);
			~Input();

			void Update();

			void LoadInputBindings(const std::string& file);

			bool IsKeyDown(KeyboardKey key);
			bool IsKeyJustPressed(KeyboardKey key);
			bool IsKeyJustReleased(KeyboardKey key);

			bool IsControllerButtonDown(GamepadButton button);
			bool IsControllerButtonJustPressed(GamepadButton button);
			bool IsControllerButtonJustReleased(GamepadButton button);

			float GetControllerAxisValue(GamepadAxis code);

			bool IsStringKeyDown(const std::string& input);
			bool IsStringKeyJustPressed(const std::string& input);
			bool IsStringKeyJustReleased(const std::string& input);

			bool IsStringControllerButtonDown(const std::string& input);
			bool IsStringControllerButtonJustPressed(const std::string& input);
			bool IsStringControllerButtonJustReleased(const std::string& input);

			float GetStringControllerAxisValue(const std::string& input);

			void CreateButtonBinding(const std::string& input, KeyboardKey code);
			void CreateButtonBinding(const std::string& input, GamepadButton code);

			void CreateVairableBinding(const std::string& input, KeyboardKey code);
			void CreateVairableBinding(const std::string& input, GamepadButton code);
			void CreateVairableBinding(const std::string& input, GamepadAxis code);

			void CreateAxisBinding(const std::string& input, KeyboardKey code, AxisComponent axis);
			void CreateAxisBinding(const std::string& input, GamepadButton code, AxisComponent axis);
			void CreateAxisBinding(const std::string& input, GamepadAxis code);

			void CreateAxis2Binding(const std::string& input, KeyboardKey code, Axis2Component axis);
			void CreateAxis2Binding(const std::string& input, GamepadButton code, Axis2Component axis);
			void CreateAxis2Binding(const std::string& input, GamepadAxis code, Axis2Direction axis);

			bool IsInputDown(const std::string& input);
			bool IsInputJustPressed(const std::string& input);
			bool IsInputJustReleased(const std::string& input);
			float GetInputVariable(const std::string& input);
			float GetInputAxis(const std::string& input);
			glm::vec2 GetInputAxis2(const std::string& input);

			KeyboardKey GetKeycodeFromString(const std::string& input);
			GamepadButton GetControllerButtonFromString(const std::string& input);
			GamepadAxis GetControllerAxisFromString(const std::string& input);
            
		private:
			std::unordered_map<std::string, Binding> m_buttonBindings;
			std::unordered_map<std::string, Binding> m_variableBindings;
			std::unordered_map<std::string, AxisBinding> m_axisBindings;
			std::unordered_map<std::string, Axis2Binding> m_axis2Bindings;
			static std::unordered_map<std::string, KeyboardKey> s_keycodeMap;
			static std::unordered_map<std::string, GamepadButton> s_controllerButtonMap;
			static std::unordered_map<std::string, GamepadAxis> s_controllerAxisMap;

			float m_deadzone;

			std::string m_gamepadId;
			int m_gamepadIndex;
		};
	};
};