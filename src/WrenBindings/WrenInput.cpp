#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"

#include "WrenMath.h"

// ============================================================================
// INPUT BINDINGS
// ============================================================================

// Input.isKeyDown(inputKey) -> bool
void wren_InputIsKeyDown(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputDown = input.IsStringKeyDown(inputKey);

	wrenSetSlotBool(vm, 0, inputDown);
}

// Input.isKeyJustPressed(inputKey) -> bool
void wren_InputIsKeyJustPressed(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustPressed = input.IsStringKeyJustPressed(inputKey);

	wrenSetSlotBool(vm, 0, inputJustPressed);
}

// Input.isKeyJustReleased(inputKey) -> bool
void wren_InputIsKeyJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsStringKeyJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

// Input.isControllerButtonDown(inputKey) -> bool
void wren_InputIsControllerButtonDown(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputDown = input.IsStringControllerButtonDown(inputKey);

	wrenSetSlotBool(vm, 0, inputDown);
}

// Input.isControllerButtonJustPressed(inputKey) -> bool
void wren_InputIsControllerButtonJustPressed(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustPressed = input.IsStringControllerButtonJustPressed(inputKey);

	wrenSetSlotBool(vm, 0, inputJustPressed);
}

// Input.isControllerButtonJustReleased(inputKey) -> bool
void wren_InputIsControllerButtonJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsStringControllerButtonJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

// Input.getControllerAxisValue(inputKey) -> number
void wren_InputGetControllerAxisValue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	double inputValue = input.GetStringControllerAxisValue(inputKey);

	wrenSetSlotDouble(vm, 0, inputValue);
}

// Input.isInputDown(inputKey) -> bool
void wren_InputIsInputDown(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputDown = input.IsInputDown(inputKey);

	wrenSetSlotBool(vm, 0, inputDown);
}

// Input.isInputJustPressed(inputKey) -> bool
void wren_InputIsInputJustPressed(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustPressed = input.IsInputJustPressed(inputKey);

	wrenSetSlotBool(vm, 0, inputJustPressed);
}

// Input.isInputJustReleased(inputKey) -> bool
void wren_InputIsInputJustReleased(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	bool inputJustReleased = input.IsInputJustReleased(inputKey);

	wrenSetSlotBool(vm, 0, inputJustReleased);
}

// Input.getInputVariable(inputKey) -> Number
void wren_InputGetInputVariable(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	double inputJustReleased = input.GetInputVariable(inputKey);

	wrenSetSlotDouble(vm, 0, inputJustReleased);
}

// Input.getInputAxis(inputKey) -> Number
void wren_InputGetInputAxis(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

	const char* inputKey = wrenGetSlotString(vm, 1);

	double inputJustReleased = input.GetInputAxis(inputKey);

	wrenSetSlotDouble(vm, 0, inputJustReleased);
}

// Input.getInputAxis2(inputKey) -> Vec2
void wren_InputGetInputAxis2(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Input::Input& input = context->GetInput();

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
	Struktur::Input::Input& input = context->GetInput();

	const char* filePath = wrenGetSlotString(vm, 1);

	input.LoadInputBindings(filePath);
}

WREN_CLASS_STATIC("input", "Input", "isKeyDown(_)", wren_InputIsKeyDown, "Gets input is currently down.");
WREN_CLASS_STATIC("input", "Input", "isKeyJustPressed(_)", wren_InputIsKeyJustPressed, "Gets input was just pressed.");
WREN_CLASS_STATIC("input", "Input", "isKeyJustReleased(_)", wren_InputIsKeyJustReleased, "Gets input was just released.");
WREN_CLASS_STATIC("input", "Input", "isControllerButtonDown(_)", wren_InputIsControllerButtonDown, "Gets input is currently down.");
WREN_CLASS_STATIC("input", "Input", "isControllerButtonJustPressed(_)", wren_InputIsControllerButtonJustPressed, "Gets input was just pressed.");
WREN_CLASS_STATIC("input", "Input", "isControllerButtonJustReleased(_)", wren_InputIsControllerButtonJustReleased, "Gets input was just released.");
WREN_CLASS_STATIC("input", "Input", "getControllerAxisValue(_)", wren_InputGetControllerAxisValue, "Gets input value from -1 to 1.");

WREN_CLASS_STATIC("input", "Input", "isInputDown(_)", wren_InputIsInputDown, "Gets input is currently down.");
WREN_CLASS_STATIC("input", "Input", "isInputJustPressed(_)", wren_InputIsInputJustPressed, "Gets input was just pressed.");
WREN_CLASS_STATIC("input", "Input", "isInputJustReleased(_)", wren_InputIsInputJustReleased, "Gets input was just released.");
WREN_CLASS_STATIC("input", "Input", "getInputVariable(_)", wren_InputGetInputVariable, "Gets input value from 0 to 1.");
WREN_CLASS_STATIC("input", "Input", "getInputAxis(_)", wren_InputGetInputAxis, "Gets input value from -1 to 1.");
WREN_CLASS_STATIC("input", "Input", "getInputAxis2(_)", wren_InputGetInputAxis2, "Gets an x and y input values from -1 to 1.");

WREN_CLASS_STATIC("input", "Input", "loadInputBindings(_)", wren_InputLoadInputBindings, "Loads the input bindings.");
