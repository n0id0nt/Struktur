#include "InputConfigLoader.h"

#include <fstream>
#include <format>
#include "nlohmann/json.hpp"

#include "Input.h"
#include "InputMaps.h"
#include "Engine/Core/FileSystem.h"
#include "Debug/Assertions.h"

namespace Struktur::Input
{

	// Forward declarations of helper functions
	static void LoadButtonBindings(Input& input, const std::string& name, const nlohmann::json& bindings, InputMaps& maps);
	static void LoadVariableBindings(Input& input, const std::string& name, const nlohmann::json& bindings, InputMaps& maps);
	static void LoadAxisBindings(Input& input, const std::string& name, const nlohmann::json& bindings, InputMaps& maps);
	static void LoadAxis2Bindings(Input& input, const std::string& name, const nlohmann::json& bindings, InputMaps& maps);

	bool InputConfigLoader::LoadFromFile(Input& input, const std::string& filePath)
	{
		auto result = FileSystem::ReadString(filePath);
		ASSERT_MSG(result.success, "Failed to load config: %s", result.errorMessage.c_str());

		nlohmann::json config = nlohmann::json::parse(result.value);

		// Clear existing bindings
		input.Clear();

		// Load controller settings
		if (config.contains("controllerSettings"))
		{
			const auto& settings = config["controllerSettings"];
			if (settings.contains("deadzone"))
			{
				float deadzone = settings["deadzone"].get<float>();
				input.SetDeadzone(deadzone);
				DEBUG_INFO(std::format("Set deadzone to {}", deadzone).c_str());
			}
		}

		// Load input bindings
		if (!config.contains("inputs"))
		{
			DEBUG_WARNING("No 'inputs' array found in config file");
			return false;
		}

		const auto& inputs = config["inputs"];
		if (!inputs.is_array())
		{
			DEBUG_ERROR("'inputs' must be an array");
			return false;
		}

		InputMaps& maps = InputMaps::Instance();

		// Process each input binding
		for (const auto& inputEntry : inputs)
		{
			if (!inputEntry.contains("name") || !inputEntry.contains("type") || !inputEntry.contains("bindings"))
			{
				DEBUG_WARNING("Input entry missing required fields (name, type, bindings)");
				continue;
			}

			std::string name = inputEntry["name"].get<std::string>();
			std::string type = inputEntry["type"].get<std::string>();
			const auto& bindings = inputEntry["bindings"];

			if (!bindings.is_array())
			{
				DEBUG_WARNING(std::format("Bindings for '{}' must be an array", name).c_str());
				continue;
			}

			// Process based on input type
			if (type == "button")
			{
				LoadButtonBindings(input, name, bindings, maps);
			}
			else if (type == "variable")
			{
				LoadVariableBindings(input, name, bindings, maps);
			}
			else if (type == "axis")
			{
				LoadAxisBindings(input, name, bindings, maps);
			}
			else if (type == "axis2")
			{
				LoadAxis2Bindings(input, name, bindings, maps);
			}
			else
			{
				DEBUG_WARNING(std::format("Unknown input type '{}' for '{}'", type, name).c_str());
			}
		}

		DEBUG_INFO(std::format("Successfully loaded input config from: {}", filePath).c_str());
		return true;
	}

	// ============================================================================
	// HELPER FUNCTIONS (Static)
	// ============================================================================

	static void LoadButtonBindings(
		Input& input,
		const std::string& name,
		const nlohmann::json& bindings,
		InputMaps& maps)
	{
		for (const auto& binding : bindings)
		{
			if (!binding.contains("type") || !binding.contains("value"))
			{
				DEBUG_WARNING(std::format("Button binding for '{}' missing type or value", name).c_str());
				continue;
			}

			std::string bindingType = binding["type"].get<std::string>();
			std::string value = binding["value"].get<std::string>();

			if (bindingType == "keycode")
			{
				input.CreateButtonBinding(name, maps.GetKeycodeFromString(value));
			}
			else if (bindingType == "controllerButton")
			{
				input.CreateButtonBinding(name, maps.GetControllerButtonFromString(value));
			}
			else
			{
				DEBUG_WARNING(std::format("Unknown binding type '{}' for button '{}'", bindingType, name).c_str());
			}
		}
	}

	static void LoadVariableBindings(
		Input& input,
		const std::string& name,
		const nlohmann::json& bindings,
		InputMaps& maps)
	{
		for (const auto& binding : bindings)
		{
			if (!binding.contains("type") || !binding.contains("value"))
			{
				DEBUG_WARNING(std::format("Variable binding for '{}' missing type or value", name).c_str());
				continue;
			}

			std::string bindingType = binding["type"].get<std::string>();
			std::string value = binding["value"].get<std::string>();

			if (bindingType == "keycode")
			{
				input.CreateVariableBinding(name, maps.GetKeycodeFromString(value));
			}
			else if (bindingType == "controllerButton")
			{
				input.CreateVariableBinding(name, maps.GetControllerButtonFromString(value));
			}
			else if (bindingType == "controllerAxis")
			{
				if (!binding.contains("axis"))
				{
					DEBUG_WARNING(std::format("Controller axis binding for '{}' missing axis type", name).c_str());
					continue;
				}

				std::string axisType = binding["axis"].get<std::string>();
				Input::VariableBindingAxis variableAxis = Input::ParseVariableBindingAxis(axisType);
				input.CreateVariableBinding(name, maps.GetControllerAxisFromString(value), variableAxis);
			}
			else
			{
				DEBUG_WARNING(std::format("Unknown binding type '{}' for variable '{}'", bindingType, name).c_str());
			}
		}
	}

	static void LoadAxisBindings(
		Input& input,
		const std::string& name,
		const nlohmann::json& bindings,
		InputMaps& maps)
	{
		for (const auto& binding : bindings)
		{
			if (!binding.contains("type") || !binding.contains("value"))
			{
				DEBUG_WARNING(std::format("Axis binding for '{}' missing type or value", name).c_str());
				continue;
			}

			std::string bindingType = binding["type"].get<std::string>();
			std::string value = binding["value"].get<std::string>();

			if (bindingType == "keycode")
			{
				if (!binding.contains("component"))
				{
					DEBUG_WARNING(std::format("Keycode axis binding for '{}' missing component", name).c_str());
					continue;
				}

				std::string component = binding["component"].get<std::string>();
				Input::AxisComponent axis = Input::ParseAxisComponent(component);
				input.CreateAxisBinding(name, maps.GetKeycodeFromString(value), axis);
			}
			else if (bindingType == "controllerButton")
			{
				if (!binding.contains("component"))
				{
					DEBUG_WARNING(std::format("Controller button axis binding for '{}' missing component", name).c_str());
					continue;
				}

				std::string component = binding["component"].get<std::string>();
				Input::AxisComponent axis = Input::ParseAxisComponent(component);
				input.CreateAxisBinding(name, maps.GetControllerButtonFromString(value), axis);
			}
			else if (bindingType == "controllerAxis")
			{
				input.CreateAxisBinding(name, maps.GetControllerAxisFromString(value));
			}
			else
			{
				DEBUG_WARNING(std::format("Unknown binding type '{}' for axis '{}'", bindingType, name).c_str());
			}
		}
	}

	static void LoadAxis2Bindings(
		Input& input,
		const std::string& name,
		const nlohmann::json& bindings,
		InputMaps& maps)
	{
		for (const auto& binding : bindings)
		{
			if (!binding.contains("type") || !binding.contains("value") || !binding.contains("component"))
			{
				DEBUG_WARNING(std::format("Axis2 binding for '{}' missing type, value, or component", name).c_str());
				continue;
			}

			std::string bindingType = binding["type"].get<std::string>();
			std::string value = binding["value"].get<std::string>();
			std::string component = binding["component"].get<std::string>();

			if (bindingType == "keycode")
			{
				Input::Axis2Component axis2Comp = Input::ParseAxis2Component(component);
				input.CreateAxis2Binding(name, maps.GetKeycodeFromString(value), axis2Comp);
			}
			else if (bindingType == "controllerButton")
			{
				Input::Axis2Component axis2Comp = Input::ParseAxis2Component(component);
				input.CreateAxis2Binding(name, maps.GetControllerButtonFromString(value), axis2Comp);
			}
			else if (bindingType == "controllerAxis")
			{
				Input::Axis2Direction direction = Input::ParseAxis2Direction(component);
				input.CreateAxis2Binding(name, maps.GetControllerAxisFromString(value), direction);
			}
			else
			{
				DEBUG_WARNING(std::format("Unknown binding type '{}' for axis2 '{}'", bindingType, name).c_str());
			}
		}
	}

}
