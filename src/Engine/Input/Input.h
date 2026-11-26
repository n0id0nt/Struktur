#pragma once

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Debug/Assertions.h"
#include "glm/glm.hpp"

union SDL_Event;
struct SDL_Window;

namespace Struktur::Input
{
class InputConfigLoader;

// Which kind of device the player is using right now - drives which bindings button hints show (see
// Input::GetActiveDevice). Keyboard and mouse count as one device: using either shows keyboard/mouse bindings.
enum class InputDevice : uint8_t
{
	KeyboardMouse,
	Gamepad,
};

// How the active pad's face buttons should be pictured. Lettered = Xbox-style A/B/X/Y art (Xbox, standard and
// unknown pads); Positional = art that only shows WHICH face button (bottom/right/left/top) - used for PlayStation
// and Nintendo pads, whose labels differ but whose buttons SDL reports by position, so a positional picture is
// never wrong.
enum class GamepadFamily : uint8_t
{
	Lettered,
	Positional,
};

// The physical thing a binding listens to.
enum class InputSourceKind : uint8_t
{
	Key,            // code = SDL_Scancode
	PointerButton,  // code = Input::PointerButton
	GamepadButton,  // code = SDL_GamepadButton
	GamepadAxis,    // code = SDL_GamepadAxis
};

// Which part of an action a binding drives. None = the whole action (a button, a variable, or a stick bound to a
// whole axis); Negative/Positive = one side of an axis; Up/Down/Left/Right = one direction of an axis2; X/Y = a
// stick axis driving both directions of an axis2's horizontal/vertical.
enum class InputComponent : uint8_t
{
	None,
	Negative,
	Positive,
	Up,
	Down,
	Left,
	Right,
	X,
	Y,
};

// One binding of one action, in the order InputConfig.json lists it (see Input::GetHintSources). Kept alongside
// the query-side Binding sets rather than replacing them, so the per-frame input queries stay untouched.
struct InputSource
{
	InputSourceKind kind;
	int code;
	InputComponent component = InputComponent::None;

	InputDevice Device() const
	{
		return kind == InputSourceKind::Key || kind == InputSourceKind::PointerButton ? InputDevice::KeyboardMouse
		                                                                               : InputDevice::Gamepad;
	}
	bool operator==(const InputSource& other) const
	{
		return kind == other.kind && code == other.code && component == other.component;
	}
};

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
		// Subtracted rather than added - see CreateAxis2Binding(SDL_GamepadAxis)'s Y case.
		std::set<SDL_GamepadAxis> invertedControllerAxis;
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

	// Text entry (a name field). While active, SDL delivers typed characters (layout- and IME-aware UTF-8) and
	// keyboard keys stop driving input ACTIONS - so typing a space doesn't also press UIAccept, and letters don't
	// trigger gameplay bindings. Raw key queries (IsKeyJustPressed - Backspace/Enter/Escape) still work, and so do
	// gamepad and pointer bindings. Always pair Start with Stop.
	void StartTextInput(SDL_Window* window);
	void StopTextInput(SDL_Window* window);
	bool IsTextInputActive() const;
	// Characters typed since the last call, as UTF-8.
	std::string TakeTypedText();

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

	// --- Button hints (see UI::InputGlyphs / UIInputHint / UIRichLabel's [input=] tag) ---

	// The device the player used most recently - any key, mouse button or mouse movement means KeyboardMouse; a
	// pad button press or a stick/trigger pushed past the activity threshold means Gamepad. Starts on
	// KeyboardMouse, and falls back to it when the active pad is unplugged.
	InputDevice GetActiveDevice() const
	{
		return m_forcedDevice ? *m_forcedDevice : m_activeDevice;
	}
	GamepadFamily GetGamepadFamily() const
	{
		return m_forcedFamily ? *m_forcedFamily : m_gamepadFamily;
	}
	// Goes up whenever anything a button hint shows could have changed - the active device, the pad family, which
	// binding of an action was used last, or the bindings themselves. Hint elements compare it each frame.
	uint32_t GetHintRevision() const
	{
		return m_hintRevision;
	}
	// For other things hints depend on (UI::InputGlyphs reloading its art map) - makes every hint re-resolve.
	void NotifyHintsChanged()
	{
		BumpHintRevision();
	}
	// The binding(s) a hint for `action` should picture on the active device, one per component: component None
	// asks for the whole action (1 for a button/variable, Negative+Positive for an axis, Up+Left+Down+Right for an
	// axis2); any other component asks for just that part. Within a device, an action shows the binding the player
	// used last for it (by its rank in config order - so using W for one direction shows W/A/S/D for all four),
	// defaulting to the first listed. Empty if the action has nothing bound on the active device.
	std::vector<InputSource> GetHintSources(const std::string& action, InputComponent component) const;
	static InputComponent ParseInputComponent(const std::string& component);

	// Test/editor override - pins GetActiveDevice()/GetGamepadFamily() until cleared with std::nullopt.
	void DebugForceDevice(std::optional<InputDevice> device);
	void DebugForceGamepadFamily(std::optional<GamepadFamily> family);

	// Drops every binding of `input` (all four kinds) - InputConfigLoader calls this before loading an action, so
	// a config entry replaces that action's C++ defaults (RegisterDefaultUIBindings) instead of adding to them.
	void ClearBinding(const std::string& input);

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
	// Preferred pad (SDL list index) when nothing has been pressed yet - see RefreshGamepads().
	int m_gamepadIndex;
	// The pad every controller query reads from - whichever connected pad was used most recently.
	SDL_Gamepad* m_sdlGamepad = nullptr;
	// Every connected pad, kept open so any of them can become m_sdlGamepad.
	std::unordered_map<SDL_JoystickID, SDL_Gamepad*> m_openGamepads;
	void RefreshGamepads();
	void SetActiveGamepad(SDL_Gamepad* gamepad);
	static bool HasGamepadActivity(SDL_Gamepad* gamepad);
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
	// The keyboard keys an action binding may use right now - none while text entry is active.
	const std::set<SDL_Scancode>& KeyboardBindings(const std::set<SDL_Scancode>& keycodes) const;

	bool m_hasActiveTouch = false;
	bool m_textInputActive = false;
	std::string m_typedText;
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

	// --- Button hint state (see GetActiveDevice/GetHintSources) ---
	InputDevice m_activeDevice   = InputDevice::KeyboardMouse;
	GamepadFamily m_gamepadFamily = GamepadFamily::Lettered;
	std::optional<InputDevice> m_forcedDevice;
	std::optional<GamepadFamily> m_forcedFamily;
	uint32_t m_hintRevision = 1;
	// Every action's bindings in creation (= config) order, de-duplicated.
	std::unordered_map<std::string, std::vector<InputSource>> m_displaySources;
	// Per action, per device: the config-order rank of the binding used most recently (see GetHintSources).
	std::unordered_map<std::string, std::array<uint8_t, 2>> m_lastUsedRank;
	// Stick/trigger positions this frame and last, -1..1 - for "pushed past the threshold this frame" edges.
	std::array<float, SDL_GAMEPAD_AXIS_COUNT> m_currAxes{};
	std::array<float, SDL_GAMEPAD_AXIS_COUNT> m_prevAxes{};
	glm::vec2 m_prevPointerPosition{0.0f, 0.0f};

	void AddDisplaySource(const std::string& input, InputSource source);
	void SetActiveDevice(InputDevice device);
	void BumpHintRevision()
	{
		++m_hintRevision;
	}
	// Per-frame: which device was used, and which binding of each action fired (see Update()).
	void TrackDeviceAndLastUsed();
	bool SourceFiredThisFrame(const InputSource& source) const;
	bool AxisCrossedThreshold(SDL_GamepadAxis axis) const;
	// The parts an action is made of, derived from its sources: {None}, {Negative, Positive} or
	// {Up, Left, Down, Right} - see GetHintSources.
	static std::vector<InputComponent> ActionComponents(const std::vector<InputSource>& sources);
	// Whether a source with component `have` drives component `want` (e.g. a Y stick axis drives Up and Down).
	static bool Covers(const InputSource& source, InputComponent want);

	// Registers C++-side defaults for the reserved UI action names (UIDir/UITab/UIAccept/UICancel), mirroring
	// InputConfig.json's own bindings for them exactly - called from both the constructor and Clear() (see
	// Clear()'s own comment) so these four names can never be missing regardless of whether/when a config file
	// successfully loads. A config that does define them simply overwrites these afterward, same as always.
	void RegisterDefaultUIBindings();

	// True when any of the input's bindings passes its check - keyCheck / buttonCheck / pointerCheck are this
	// class's own per-source queries (IsKeyDown, IsControllerButtonJustPressed, ...), so the three IsInput*
	// functions share one walk over the bindings.
	bool CheckBinding(const std::string& input, bool (Input::*keyCheck)(SDL_Scancode),
	                  bool (Input::*buttonCheck)(SDL_GamepadButton), bool (Input::*pointerCheck)(PointerButton));

	float CalculateAxisValue(const AxisBinding& binding);
	float ApplyDeadzone(float value) const;
	glm::vec2 ApplyRadialDeadzone(glm::vec2 value) const;

	// Make InputConfigLoader a friend so it can call private helper methods
	friend class InputConfigLoader;
};
}  // namespace Struktur::Input
