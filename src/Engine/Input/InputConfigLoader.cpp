#include "InputConfigLoader.h"

#include <fstream>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "Input.h"
#include "InputMaps.h"
#include "nlohmann/json.hpp"

namespace Struktur::Input
{

// Forward declarations of helper functions
static void LoadButtonBindings(Input& input, const std::string& name, const nlohmann::json& bindings);
static void LoadVariableBindings(Input& input, const std::string& name, const nlohmann::json& bindings);
static void LoadAxisBindings(Input& input, const std::string& name, const nlohmann::json& bindings);
static void LoadAxis2Bindings(Input& input, const std::string& name, const nlohmann::json& bindings);

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
			DEBUG_INFO("Set deadzone to %g", deadzone);
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

	// Process each input binding
	for (const auto& inputEntry : inputs)
	{
		if (!inputEntry.contains("name") || !inputEntry.contains("type") || !inputEntry.contains("bindings"))
		{
			DEBUG_WARNING("Input entry missing required fields (name, type, bindings)");
			continue;
		}

		std::string name     = inputEntry["name"].get<std::string>();
		std::string type     = inputEntry["type"].get<std::string>();
		const auto& bindings = inputEntry["bindings"];

		if (!bindings.is_array())
		{
			DEBUG_WARNING("Bindings for '%s' must be an array", name);
			continue;
		}

		// Process based on input type
		if (type == "button")
		{
			LoadButtonBindings(input, name, bindings);
		}
		else if (type == "variable")
		{
			LoadVariableBindings(input, name, bindings);
		}
		else if (type == "axis")
		{
			LoadAxisBindings(input, name, bindings);
		}
		else if (type == "axis2")
		{
			LoadAxis2Bindings(input, name, bindings);
		}
		else
		{
			DEBUG_WARNING("Unknown input type '%s' for '%s'", type, name);
		}
	}

	DEBUG_INFO("Successfully loaded input config from: %s", filePath);
	return true;
}

// ============================================================================
// HELPER FUNCTIONS (Static)
// ============================================================================

static void LoadButtonBindings(Input& input, const std::string& name, const nlohmann::json& bindings)
{
	for (const auto& binding : bindings)
	{
		if (!binding.contains("type") || !binding.contains("value"))
		{
			DEBUG_WARNING("Button binding for '%s' missing type or value", name);
			continue;
		}

		std::string bindingType = binding["type"].get<std::string>();
		std::string value       = binding["value"].get<std::string>();

		if (bindingType == "keycode")
		{
			input.CreateButtonBinding(name, InputMaps::GetKeycodeFromString(value));
		}
		else if (bindingType == "controllerButton")
		{
			input.CreateButtonBinding(name, InputMaps::GetControllerButtonFromString(value));
		}
		else
		{
			DEBUG_WARNING("Unknown binding type '%s' for button '%s'", bindingType, name);
		}
	}
}

static void LoadVariableBindings(Input& input, const std::string& name, const nlohmann::json& bindings)
{
	for (const auto& binding : bindings)
	{
		if (!binding.contains("type") || !binding.contains("value"))
		{
			DEBUG_WARNING("Variable binding for '%s' missing type or value", name);
			continue;
		}

		std::string bindingType = binding["type"].get<std::string>();
		std::string value       = binding["value"].get<std::string>();

		if (bindingType == "keycode")
		{
			input.CreateVariableBinding(name, InputMaps::GetKeycodeFromString(value));
		}
		else if (bindingType == "controllerButton")
		{
			input.CreateVariableBinding(name, InputMaps::GetControllerButtonFromString(value));
		}
		else if (bindingType == "controllerAxis")
		{
			if (!binding.contains("axis"))
			{
				DEBUG_WARNING("Controller axis binding for '%s' missing axis type", name);
				continue;
			}

			std::string axisType                    = binding["axis"].get<std::string>();
			Input::VariableBindingAxis variableAxis = Input::ParseVariableBindingAxis(axisType);
			input.CreateVariableBinding(name, InputMaps::GetControllerAxisFromString(value), variableAxis);
		}
		else
		{
			DEBUG_WARNING("Unknown binding type '%s' for variable '%s'", bindingType, name);
		}
	}
}

static void LoadAxisBindings(Input& input, const std::string& name, const nlohmann::json& bindings)
{
	for (const auto& binding : bindings)
	{
		if (!binding.contains("type") || !binding.contains("value"))
		{
			DEBUG_WARNING("Axis binding for '%s' missing type or value", name);
			continue;
		}

		std::string bindingType = binding["type"].get<std::string>();
		std::string value       = binding["value"].get<std::string>();

		if (bindingType == "keycode")
		{
			if (!binding.contains("component"))
			{
				DEBUG_WARNING("Keycode axis binding for '%s' missing component", name);
				continue;
			}

			std::string component     = binding["component"].get<std::string>();
			Input::AxisComponent axis = Input::ParseAxisComponent(component);
			input.CreateAxisBinding(name, InputMaps::GetKeycodeFromString(value), axis);
		}
		else if (bindingType == "controllerButton")
		{
			if (!binding.contains("component"))
			{
				DEBUG_WARNING("Controller button axis binding for '%s' missing component", name);
				continue;
			}

			std::string component     = binding["component"].get<std::string>();
			Input::AxisComponent axis = Input::ParseAxisComponent(component);
			input.CreateAxisBinding(name, InputMaps::GetControllerButtonFromString(value), axis);
		}
		else if (bindingType == "controllerAxis")
		{
			input.CreateAxisBinding(name, InputMaps::GetControllerAxisFromString(value));
		}
		else
		{
			DEBUG_WARNING("Unknown binding type '%s' for axis '%s'", bindingType, name);
		}
	}
}

static void LoadAxis2Bindings(Input& input, const std::string& name, const nlohmann::json& bindings)
{
	for (const auto& binding : bindings)
	{
		if (!binding.contains("type") || !binding.contains("value") || !binding.contains("component"))
		{
			DEBUG_WARNING("Axis2 binding for '%s' missing type, value, or component", name);
			continue;
		}

		std::string bindingType = binding["type"].get<std::string>();
		std::string value       = binding["value"].get<std::string>();
		std::string component   = binding["component"].get<std::string>();

		if (bindingType == "keycode")
		{
			Input::Axis2Component axis2Comp = Input::ParseAxis2Component(component);
			input.CreateAxis2Binding(name, InputMaps::GetKeycodeFromString(value), axis2Comp);
		}
		else if (bindingType == "controllerButton")
		{
			Input::Axis2Component axis2Comp = Input::ParseAxis2Component(component);
			input.CreateAxis2Binding(name, InputMaps::GetControllerButtonFromString(value), axis2Comp);
		}
		else if (bindingType == "controllerAxis")
		{
			Input::Axis2Direction direction = Input::ParseAxis2Direction(component);
			input.CreateAxis2Binding(name, InputMaps::GetControllerAxisFromString(value), direction);
		}
		else
		{
			DEBUG_WARNING("Unknown binding type '%s' for axis2 '%s'", bindingType, name);
		}
	}
}

}  // namespace Struktur::Input
