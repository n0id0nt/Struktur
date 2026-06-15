#include "WrenStateManager.h"

#include <sstream>

#include "Debug/Assertions.h"
#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/GameContext.h"

namespace Struktur::Wren
{

bool WrenStateManager::Initialise(GameContext& context)
{
	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	if (!vm)
	{
		DEBUG_ERROR("[WrenStateManager] Wren VM not initialised");
		return false;
	}

	DEBUG_INFO("[WrenStateManager] Initializing Main.wren");

	// Load Main.wren
	if (!scriptEngine.InterpretFile("Scripts/Main.wren"))
	{
		DEBUG_ERROR("[WrenStateManager] Failed to load Main.wren");
		DEBUG_ERROR("[WrenStateManager] Main.wren must be in the scripts folder");
		return false;
	}

	// Get Main class
	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, "Main", "Game", 0);

	if (wrenGetSlotType(vm, 0) == WREN_TYPE_NULL)
	{
		DEBUG_ERROR("[WrenStateManager] Game class not found");
		return false;
	}

	WrenHandle* initialiseMethod = wrenMakeCallHandle(vm, "new()");
	WrenInterpretResult result   = wrenCall(vm, initialiseMethod);
	wrenReleaseHandle(vm, initialiseMethod);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenStateManager] Game.new() failed");
		return false;
	}

	// Store Game instance handle (returned from initialise)
	m_rootStateInstanceHandle = wrenGetSlotHandle(vm, 0);

	if (!m_rootStateInstanceHandle)
	{
		DEBUG_ERROR("[WrenStateManager] Failed to get Game instance handle");
		return false;
	}

	// Cache method handles
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	m_updateMethodHandle    = wrenMakeCallHandle(vm, "update()");
	m_renderMethodHandle    = wrenMakeCallHandle(vm, "render()");
	m_startMethodHandle     = wrenMakeCallHandle(vm, "start()");
	m_quitMethodHandle      = wrenMakeCallHandle(vm, "quit()");
	m_sendEventMethodHandle = wrenMakeCallHandle(vm, "onEvent(_,_)");

	if (!m_updateMethodHandle)
	{
		DEBUG_ERROR("[WrenStateManager] Failed to get update method handle");
		return false;
	}

	m_isInitialised = true;
	DEBUG_INFO("[WrenStateManager] Initialised successfully");

	return true;
}

void WrenStateManager::Start(GameContext& context)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle || !m_startMethodHandle)
	{
		return;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	// Call Game.start()
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenInterpretResult result = wrenCall(vm, m_startMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenStateManager] Start failed");
	}
}

void WrenStateManager::Update(GameContext& context)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle || !m_updateMethodHandle)
	{
		return;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	// Call Game.update(dt)
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenInterpretResult result = wrenCall(vm, m_updateMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenStateManager] Update failed");
	}
}

void WrenStateManager::Render(GameContext& context)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle || !m_renderMethodHandle)
	{
		return;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	// Call Game.render()
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenInterpretResult result = wrenCall(vm, m_renderMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_WARNING("[WrenStateManager] Render failed");
	}
}

void WrenStateManager::SendEvent(GameContext& context, const Event::Event& event)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle || !m_sendEventMethodHandle)
	{
		return;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	// Add event data
	wrenEnsureSlots(vm, 3);
	wrenSetSlotHandle(vm, 0, m_sendEventMethodHandle);
	wrenSetSlotString(vm, 1, event.type.c_str());
	Callback::HelperFunctions::VariantToWrenSlot(vm, 2, event.data);

	WrenInterpretResult result = wrenCall(vm, m_sendEventMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenStateManager] SendEvent failed");
	}
}

void WrenStateManager::Shutdown(GameContext& context)
{
	if (!m_isInitialised)
	{
		return;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	// Call Game.start()
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenInterpretResult result = wrenCall(vm, m_quitMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenStateManager] Quit failed");
	}

	if (vm)
	{
		if (m_rootStateInstanceHandle)
		{
			wrenReleaseHandle(vm, m_rootStateInstanceHandle);
		}
		if (m_updateMethodHandle)
		{
			wrenReleaseHandle(vm, m_updateMethodHandle);
		}
		if (m_renderMethodHandle)
		{
			wrenReleaseHandle(vm, m_renderMethodHandle);
		}
		if (m_startMethodHandle)
		{
			wrenReleaseHandle(vm, m_startMethodHandle);
		}
		if (m_quitMethodHandle)
		{
			wrenReleaseHandle(vm, m_quitMethodHandle);
		}
		if (m_sendEventMethodHandle)
		{
			wrenReleaseHandle(vm, m_sendEventMethodHandle);
		}
	}

	m_rootStateInstanceHandle = nullptr;
	m_updateMethodHandle      = nullptr;
	m_renderMethodHandle      = nullptr;
	m_startMethodHandle       = nullptr;
	m_quitMethodHandle        = nullptr;
	m_sendEventMethodHandle   = nullptr;
	m_isInitialised           = false;

	DEBUG_INFO("[WrenStateManager] Shutdown complete");
}

std::string WrenStateManager::GetCurrentStateName(GameContext& context)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle)
	{
		return "None";
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	// Get StateManager from Game
	wrenEnsureSlots(vm, 2);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenHandle* getStateManager = wrenMakeCallHandle(vm, "stateManager");
	WrenInterpretResult result  = wrenCall(vm, getStateManager);
	wrenReleaseHandle(vm, getStateManager);

	if (result != WREN_RESULT_SUCCESS)
	{
		return "Error";
	}

	// Get currentState from StateManager
	WrenHandle* getCurrentState = wrenMakeCallHandle(vm, "currentState");
	result                      = wrenCall(vm, getCurrentState);
	wrenReleaseHandle(vm, getCurrentState);

	if (result != WREN_RESULT_SUCCESS || wrenGetSlotType(vm, 0) == WREN_TYPE_NULL)
	{
		return "None";
	}

	// Get name property from state
	WrenHandle* getName = wrenMakeCallHandle(vm, "name");
	result              = wrenCall(vm, getName);
	wrenReleaseHandle(vm, getName);

	if (result != WREN_RESULT_SUCCESS)
	{
		return "Unknown";
	}

	const char* name = wrenGetSlotString(vm, 0);
	return name ? std::string(name) : "Unknown";
}

}  // namespace Struktur::Wren
