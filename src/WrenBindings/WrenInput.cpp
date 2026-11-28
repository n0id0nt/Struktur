#include "Engine/Callback/Variant.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/CallbackHelperFunctions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "WrenMath.h"
#include "wren.hpp"

// ============================================================================
// INPUT BINDINGS
// ============================================================================

// Input.isKeyDown(inputKey) -> bool
void wren_InputIsKeyDown(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputDown = input.IsStringKeyDown(inputKey);

	wrenSetSlotBool(vm, 0, inputDown);
}

// Input.isKeyJustPressed(inputKey) -> bool
void wren_InputIsKeyJustPressed(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustPressed = input.IsStringKeyJustPressed(inputKey);

	wrenSetSlotBool(vm, 0, inputJustPressed);
}

// Input.isKeyJustReleased(inputKey) -> bool
void wren_InputIsKeyJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsStringKeyJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

// Input.isControllerButtonDown(inputKey) -> bool
void wren_InputIsControllerButtonDown(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputDown = input.IsStringControllerButtonDown(inputKey);

	wrenSetSlotBool(vm, 0, inputDown);
}

// Input.isControllerButtonJustPressed(inputKey) -> bool
void wren_InputIsControllerButtonJustPressed(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustPressed = input.IsStringControllerButtonJustPressed(inputKey);

	wrenSetSlotBool(vm, 0, inputJustPressed);
}

// Input.isControllerButtonJustReleased(inputKey) -> bool
void wren_InputIsControllerButtonJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsStringControllerButtonJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

// Input.getControllerAxisValue(inputKey) -> number
void wren_InputGetControllerAxisValue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	double inputValue = input.GetStringControllerAxisValue(inputKey);

	wrenSetSlotDouble(vm, 0, inputValue);
}

// Input.isInputDown(inputKey) -> bool
void wren_InputIsInputDown(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputDown = input.IsInputDown(inputKey);

	wrenSetSlotBool(vm, 0, inputDown);
}

// Input.isInputJustPressed(inputKey) -> bool
void wren_InputIsInputJustPressed(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustPressed = input.IsInputJustPressed(inputKey);

	wrenSetSlotBool(vm, 0, inputJustPressed);
}

// Input.isInputJustReleased(inputKey) -> bool
void wren_InputIsInputJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsInputJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

// Input.getInputVariable(inputKey) -> Number
void wren_InputGetInputVariable(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	double inputJustReleased = input.GetInputVariable(inputKey);

	wrenSetSlotDouble(vm, 0, inputJustReleased);
}

// Input.getInputAxis(inputKey) -> Number
void wren_InputGetInputAxis(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	double inputJustReleased = input.GetInputAxis(inputKey);

	wrenSetSlotDouble(vm, 0, inputJustReleased);
}

// Input.getInputAxis2(inputKey) -> Vec2
void wren_InputGetInputAxis2(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	glm::vec2 inputDir = input.GetInputAxis2(inputKey);

	wrenGetVariable(vm, "math", "Vec2", 1);  // Get class into slot 1
	WrenVec2* vec2 = (WrenVec2*)wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenVec2));
	new (vec2) WrenVec2(inputDir);
}

// Input.loadInputBindings(filePath)
void wren_InputLoadInputBindings(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input  = context->GetInput();

	const char* filePath = wrenGetSlotString(vm, 1);

	input.LoadInputBindings(filePath);
}

// Input.startTextInput() - typed characters start arriving through takeTypedText(), and keyboard keys stop
// driving input actions until stopTextInput() (see Input::StartTextInput).
void wren_InputStartTextInput(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	context->GetInput().StartTextInput(context->GetWindow().GetSDLWindow());
}

// Input.stopTextInput()
void wren_InputStopTextInput(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	context->GetInput().StopTextInput(context->GetWindow().GetSDLWindow());
}

// Input.isTextInputActive -> bool
void wren_InputIsTextInputActive(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	wrenSetSlotBool(vm, 0, context->GetInput().IsTextInputActive());
}

// Input.takeTypedText() -> String typed since the last call (UTF-8, may be empty)
void wren_InputTakeTypedText(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	const std::string typed        = context->GetInput().TakeTypedText();
	wrenSetSlotString(vm, 0, typed.c_str());
}

#ifdef STRUKTUR_FITNESS
static Struktur::Callback::Variant OptionalVariant(const std::optional<double>& value)
{
	return value ? Struktur::Callback::Variant(*value) : Struktur::Callback::Variant(nullptr);
}

// Input.trainer -> map of the fitness devices and the ride in progress (see Input::TrainerInput)
void wren_InputTrainer(WrenVM* vm)
{
	Struktur::GameContext* context                = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	const Struktur::Input::TrainerInput& trainer = context->GetInput().GetTrainer();
	Struktur::Callback::VariantMap map;
	map.items["power"]               = OptionalVariant(trainer.powerWatts);
	map.items["smoothedPower"]       = OptionalVariant(trainer.smoothedPowerWatts);
	map.items["cadence"]             = OptionalVariant(trainer.cadenceRpm);
	map.items["heartRate"]           = OptionalVariant(trainer.heartRateBpm);
	map.items["trainerConnected"]    = trainer.trainerConnected;
	map.items["heartRateConnected"]  = trainer.heartRateConnected;
	map.items["powerMeterConnected"] = trainer.powerMeterConnected;
	map.items["speed"]               = trainer.speedMetersPerSecond;
	map.items["grade"]               = trainer.chosenGradePercent;
	map.items["erg"]                 = trainer.ergMode;
	map.items["powerZone"]           = trainer.powerZone;
	map.items["powerFraction"]       = OptionalVariant(trainer.powerFraction);
	map.items["targetDelta"]         = OptionalVariant(trainer.targetDeltaWatts);
	map.items["workoutProgress"]     = trainer.workoutProgress;
	Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, 0, map);
}
#endif

// Input.activeDevice -> "keyboard" | "gamepad"
void wren_InputActiveDevice(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	bool gamepad = context->GetInput().GetActiveDevice() == Struktur::Input::InputDevice::Gamepad;
	wrenSetSlotString(vm, 0, gamepad ? "gamepad" : "keyboard");
}

// Input.gamepadFamily -> "lettered" | "positional"
void wren_InputGamepadFamily(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	bool positional = context->GetInput().GetGamepadFamily() == Struktur::Input::GamepadFamily::Positional;
	wrenSetSlotString(vm, 0, positional ? "positional" : "lettered");
}

// Input.hintRevision -> Num
void wren_InputHintRevision(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	wrenSetSlotDouble(vm, 0, (double)context->GetInput().GetHintRevision());
}

// Input.debugForceDevice(name) - "keyboard", "gamepad", or anything else (e.g. "none") to stop forcing.
void wren_InputDebugForceDevice(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	std::string name               = wrenGetSlotType(vm, 1) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 1) : "";
	std::optional<Struktur::Input::InputDevice> device;
	if (name == "keyboard")
	{
		device = Struktur::Input::InputDevice::KeyboardMouse;
	}
	else if (name == "gamepad")
	{
		device = Struktur::Input::InputDevice::Gamepad;
	}
	context->GetInput().DebugForceDevice(device);
}

// Input.debugForceGamepadFamily(name) - "lettered", "positional", or anything else to stop forcing.
void wren_InputDebugForceGamepadFamily(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	std::string name               = wrenGetSlotType(vm, 1) == WREN_TYPE_STRING ? wrenGetSlotString(vm, 1) : "";
	std::optional<Struktur::Input::GamepadFamily> family;
	if (name == "lettered")
	{
		family = Struktur::Input::GamepadFamily::Lettered;
	}
	else if (name == "positional")
	{
		family = Struktur::Input::GamepadFamily::Positional;
	}
	context->GetInput().DebugForceGamepadFamily(family);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(Input)
{
	WREN_CLASS_STATIC(registry, "input", "Input", "isKeyDown(_)", wren_InputIsKeyDown, "Gets input is currently down.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isKeyJustPressed(_)", wren_InputIsKeyJustPressed,
	                  "Gets input was just pressed.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isKeyJustReleased(_)", wren_InputIsKeyJustReleased,
	                  "Gets input was just released.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isControllerButtonDown(_)", wren_InputIsControllerButtonDown,
	                  "Gets input is currently down.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isControllerButtonJustPressed(_)",
	                  wren_InputIsControllerButtonJustPressed, "Gets input was just pressed.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isControllerButtonJustReleased(_)",
	                  wren_InputIsControllerButtonJustReleased, "Gets input was just released.");
	WREN_CLASS_STATIC(registry, "input", "Input", "getControllerAxisValue(_)", wren_InputGetControllerAxisValue,
	                  "Gets input value from -1 to 1.");

	WREN_CLASS_STATIC(registry, "input", "Input", "isInputDown(_)", wren_InputIsInputDown,
	                  "Gets input is currently down.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isInputJustPressed(_)", wren_InputIsInputJustPressed,
	                  "Gets input was just pressed.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isInputJustReleased(_)", wren_InputIsInputJustReleased,
	                  "Gets input was just released.");
	WREN_CLASS_STATIC(registry, "input", "Input", "getInputVariable(_)", wren_InputGetInputVariable,
	                  "Gets input value from 0 to 1.");
	WREN_CLASS_STATIC(registry, "input", "Input", "getInputAxis(_)", wren_InputGetInputAxis,
	                  "Gets input value from -1 to 1.");
	WREN_CLASS_STATIC(registry, "input", "Input", "getInputAxis2(_)", wren_InputGetInputAxis2,
	                  "Gets an x and y input values from -1 to 1.");

	WREN_CLASS_STATIC(registry, "input", "Input", "loadInputBindings(_)", wren_InputLoadInputBindings,
	                  "Loads the input bindings.");

	WREN_CLASS_STATIC(registry, "input", "Input", "startTextInput()", wren_InputStartTextInput,
	                  "Starts text entry: typed characters arrive via takeTypedText() and keyboard keys stop driving actions.");
	WREN_CLASS_STATIC(registry, "input", "Input", "stopTextInput()", wren_InputStopTextInput,
	                  "Ends text entry; keyboard keys drive actions again.");
	WREN_CLASS_STATIC(registry, "input", "Input", "isTextInputActive", wren_InputIsTextInputActive,
	                  "True between startTextInput() and stopTextInput().");
	WREN_CLASS_STATIC(registry, "input", "Input", "takeTypedText()", wren_InputTakeTypedText,
	                  "Text typed since the last call (UTF-8, may be empty).");
#ifdef STRUKTUR_FITNESS
	WREN_CLASS_STATIC(registry, "input", "Input", "trainer", wren_InputTrainer,
	                  "Map of the fitness devices and the ride in progress: power, smoothedPower, cadence, heartRate "
	                  "(null while nothing reports them), *Connected, speed, grade, erg, powerZone, powerFraction, "
	                  "targetDelta, workoutProgress.");
#endif
	WREN_CLASS_STATIC(registry, "input", "Input", "activeDevice", wren_InputActiveDevice,
	                  "The device used most recently: \"keyboard\" (keyboard or mouse) or \"gamepad\".");
	WREN_CLASS_STATIC(registry, "input", "Input", "gamepadFamily", wren_InputGamepadFamily,
	                  "How the active pad's face buttons are pictured: \"lettered\" (Xbox) or \"positional\".");
	WREN_CLASS_STATIC(registry, "input", "Input", "hintRevision", wren_InputHintRevision,
	                  "Changes whenever button hints could need redrawing (device, last-used binding, bindings).");
	WREN_CLASS_STATIC(registry, "input", "Input", "debugForceDevice(_)", wren_InputDebugForceDevice,
	                  "Pins the active device for testing: \"keyboard\", \"gamepad\", or \"none\" to stop.");
	WREN_CLASS_STATIC(registry, "input", "Input", "debugForceGamepadFamily(_)", wren_InputDebugForceGamepadFamily,
	                  "Pins the pad family for testing: \"lettered\", \"positional\", or \"none\" to stop.");
}
