#pragma once

#include <string>
#include <vector>
#ifdef DEBUG
	#include <unordered_map>
#endif

#include "Engine/Event/Event.h"
#include "Engine/Scripting/WrenExportedFields.h"
#include "WrenScriptEngine.h"

namespace Struktur
{

class GameContext;

namespace Wren
{
struct WrenItem;
// C++ bridge to Wren state management system
// Holds handle to Wren Game instance and delegates all calls to Wren
class WrenStateManager
{
public:
	WrenStateManager()
	    : m_rootStateInstanceHandle(nullptr),
	      m_updateMethodHandle(nullptr),
	      m_fixedUpdateMethodHandle(nullptr),
	      m_renderMethodHandle(nullptr),
	      m_startMethodHandle(nullptr),
	      m_quitMethodHandle(nullptr),
	      m_sendEventMethodHandle(nullptr),
	      m_isInitialised(false)
	{
	}

	~WrenStateManager() {}

	// Initialise the Wren state system
	// Loads Main.wren, calls Boot.initialise(initialStateName)
	// Returns Game instance handle
	// Called before the game window is created and will initial the project settings.
	bool Initialise(GameContext& context);

	// Called after the all the systems are initialised
	void Start(GameContext& context);

	// Update the game state (calls Game.update(dt) in Wren)
	void Update(GameContext& context);

	// Fixed-rate update, called from SystemManager::FixedUpdate at GameData::timeStep cadence (calls
	// Game.fixedUpdate() in Wren, which forwards to StateManager.fixedUpdate() -> BaseState.fixedUpdate(this)).
	void FixedUpdate(GameContext& context);

	// Optional: Render (calls Game.render() in Wren)
	void Render(GameContext& context);

	// Send event to current state
	void SendEvent(GameContext& context, const Event::Event& event);

	// Clean up
	void Shutdown(GameContext& context);

	// State-debug window support (see StateWindow.cpp) - all three reach the root StateManager via
	// Game.stateManager (Main.wren), chaining wrenMakeCallHandle/wrenCall the same way every call below does.

	// The full active state stack (outermost to innermost, including any nested sub-state-managers a state
	// opts into via BaseState.subStateManager) as JSON - see StateManager.wren's activeStateStackJson().
	std::string GetActiveStateStackJson(GameContext& context);

	// The live state instances at each stack depth (same order as the JSON above) - one WrenHandle* per level,
	// caller owns releasing them via wrenReleaseHandle once done. Needed alongside the JSON because JSON can't
	// carry object handles, and exported-field inspection needs a real instance to read/write against.
	std::vector<WrenHandle*> GetActiveStateInstances(GameContext& context);

	// Every state name the root StateManager can changeState() to.
	std::vector<std::string> GetRegisteredStateNames(GameContext& context);

	// Force a transition on the root StateManager - passes empty params (see StateManager.wren's 1-arg
	// changeState overload), so states whose enter() needs specific params will likely break if triggered this
	// way. Known, accepted limitation for a "quick jump" editor action.
	bool TriggerStateChange(GameContext& context, const std::string& stateName);

#ifdef DEBUG
	// Exported-field inspection for a live state instance - mirrors WrenScriptComponentRegistry's
	// discover-and-cache pattern (see WrenExportedFields.h), just keyed by the state's own class name
	// (resolved via calling .type/.name on the instance) instead of a registered script className.
	const std::vector<Wren::WrenExportedField>& GetStateExportedFields(GameContext& context,
	                                                                   WrenHandle* stateInstanceHandle);
	bool GetStateFieldValue(GameContext& context, WrenHandle* stateInstanceHandle, const std::string& fieldName,
	                        Wren::WrenItem& out_value);
	bool SetStateFieldValue(GameContext& context, WrenHandle* stateInstanceHandle, const std::string& fieldName,
	                        const Wren::WrenItem& value);
#endif

	// Getters
	bool IsInitialised() const
	{
		return m_isInitialised;
	}
	WrenHandle* GetGameHandle() const
	{
		return m_rootStateInstanceHandle;
	}

private:
	// Chains Game(m_rootStateInstanceHandle) -> stateManager -> `methodSignature` (no args), leaving the
	// result in slot 0. Shared by GetActiveStateStackJson/GetActiveStateInstances/GetRegisteredStateNames -
	// the three read-only root-StateManager queries - since they're identical up to which zero-arg method gets
	// called at the end. Returns false (slot 0 left in whatever wrenCall left it in) if either hop fails.
	bool CallOnRootStateManager(GameContext& context, const char* methodSignature);

	WrenHandle* m_rootStateInstanceHandle;   // Handle to Wren Game object
	WrenHandle* m_updateMethodHandle;        // Cached Game.update() method
	WrenHandle* m_fixedUpdateMethodHandle;   // Cached Game.fixedUpdate() method
	WrenHandle* m_renderMethodHandle;        // Cached Game.render() method
	WrenHandle* m_startMethodHandle;        // Cached Game.start() method
	WrenHandle* m_quitMethodHandle;         // Cached Game.quit() method
	WrenHandle* m_sendEventMethodHandle;    // Cached Game.sendEvent() method

	bool m_isInitialised;

#ifdef DEBUG
	// className -> exported fields, populated lazily the first time a state of that class is inspected.
	std::unordered_map<std::string, std::vector<Wren::WrenExportedField>> m_stateExportedFieldsCache;
#endif
};
}  // namespace Wren
}  // namespace Struktur
