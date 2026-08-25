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
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);
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

#ifdef DEBUG
	if (vm)
	{
		for (auto& [className, fields] : m_stateExportedFieldsCache)
		{
			ExportedFields::ReleaseFields(vm, fields);
		}
	}
	m_stateExportedFieldsCache.clear();
#endif

	DEBUG_INFO("[WrenStateManager] Shutdown complete");
}

bool WrenStateManager::CallOnRootStateManager(GameContext& context, const char* methodSignature)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle)
	{
		return false;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();
	if (!vm)
	{
		return false;
	}

	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenHandle* getStateManager = wrenMakeCallHandle(vm, "stateManager");
	WrenInterpretResult result  = wrenCall(vm, getStateManager);
	wrenReleaseHandle(vm, getStateManager);
	if (result != WREN_RESULT_SUCCESS)
	{
		return false;
	}

	// wrenCall() truncates the API stack down to whatever the called closure needed, so re-ensure before the
	// next call - slot 0 still holds the StateManager instance returned above.
	wrenEnsureSlots(vm, 1);
	WrenHandle* callMethod = wrenMakeCallHandle(vm, methodSignature);
	result                 = wrenCall(vm, callMethod);
	wrenReleaseHandle(vm, callMethod);

	return result == WREN_RESULT_SUCCESS;
}

std::string WrenStateManager::GetActiveStateStackJson(GameContext& context)
{
	if (!CallOnRootStateManager(context, "activeStateStackJson()"))
	{
		return "[]";
	}

	WrenVM* vm        = context.GetWrenScriptEngine().GetVM();
	const char* json  = wrenGetSlotString(vm, 0);
	return json ? std::string(json) : "[]";
}

std::vector<WrenHandle*> WrenStateManager::GetActiveStateInstances(GameContext& context)
{
	std::vector<WrenHandle*> instances;

	if (!CallOnRootStateManager(context, "activeStateInstances()"))
	{
		return instances;
	}

	WrenVM* vm = context.GetWrenScriptEngine().GetVM();
	wrenEnsureSlots(vm, 2);
	int count = wrenGetListCount(vm, 0);
	instances.reserve(count);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 0, i, 1);
		instances.push_back(wrenGetSlotHandle(vm, 1));
	}

	return instances;
}

std::vector<std::string> WrenStateManager::GetRegisteredStateNames(GameContext& context)
{
	std::vector<std::string> names;

	if (!CallOnRootStateManager(context, "registeredStateNames()"))
	{
		return names;
	}

	WrenVM* vm = context.GetWrenScriptEngine().GetVM();
	wrenEnsureSlots(vm, 2);
	int count = wrenGetListCount(vm, 0);
	names.reserve(count);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 0, i, 1);
		const char* name = wrenGetSlotString(vm, 1);
		names.emplace_back(name ? name : "");
	}

	return names;
}

bool WrenStateManager::TriggerStateChange(GameContext& context, const std::string& stateName)
{
	if (!m_isInitialised || !m_rootStateInstanceHandle)
	{
		return false;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();
	if (!vm)
	{
		return false;
	}

	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, m_rootStateInstanceHandle);

	WrenHandle* getStateManager = wrenMakeCallHandle(vm, "stateManager");
	WrenInterpretResult result  = wrenCall(vm, getStateManager);
	wrenReleaseHandle(vm, getStateManager);
	if (result != WREN_RESULT_SUCCESS)
	{
		return false;
	}

	// slot 0 still holds the StateManager instance - add the state-name argument in slot 1 before the
	// changeState(_) call (the 1-arg overload, i.e. empty params - see TriggerStateChange's own header comment).
	wrenEnsureSlots(vm, 2);
	wrenSetSlotString(vm, 1, stateName.c_str());

	WrenHandle* changeState = wrenMakeCallHandle(vm, "changeState(_)");
	result                  = wrenCall(vm, changeState);
	wrenReleaseHandle(vm, changeState);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenStateManager] TriggerStateChange('%s') failed", stateName.c_str());
		return false;
	}

	return true;
}

#ifdef DEBUG
const std::vector<Struktur::Wren::WrenExportedField>& WrenStateManager::GetStateExportedFields(
    GameContext& context, WrenHandle* stateInstanceHandle)
{
	static const std::vector<WrenExportedField> kEmpty;

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();
	if (!vm || !stateInstanceHandle)
	{
		return kEmpty;
	}

	// Resolve the instance's own class (not the state's `name` field - that's just app-level data that
	// happens to match by convention, not a structural guarantee) via the stock Wren `.type` primitive, then
	// its name via `.name` on the resulting class, to use as the cache key.
	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, stateInstanceHandle);
	WrenHandle* getType = wrenMakeCallHandle(vm, "type");
	if (wrenCall(vm, getType) != WREN_RESULT_SUCCESS)
	{
		wrenReleaseHandle(vm, getType);
		return kEmpty;
	}
	wrenReleaseHandle(vm, getType);
	WrenHandle* classHandle = wrenGetSlotHandle(vm, 0);

	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, classHandle);
	WrenHandle* getName = wrenMakeCallHandle(vm, "name");
	if (wrenCall(vm, getName) != WREN_RESULT_SUCCESS)
	{
		wrenReleaseHandle(vm, getName);
		wrenReleaseHandle(vm, classHandle);
		return kEmpty;
	}
	wrenReleaseHandle(vm, getName);
	const char* classNameRaw = wrenGetSlotString(vm, 0);
	std::string className    = classNameRaw ? classNameRaw : "Unknown";

	auto it = m_stateExportedFieldsCache.find(className);
	if (it != m_stateExportedFieldsCache.end())
	{
		wrenReleaseHandle(vm, classHandle);
		return it->second;
	}

	std::vector<WrenExportedField> fields = ExportedFields::DiscoverExportedFields(context, classHandle, className);
	wrenReleaseHandle(vm, classHandle);

	auto [insertedIt, inserted] = m_stateExportedFieldsCache.emplace(className, std::move(fields));
	return insertedIt->second;
}

bool WrenStateManager::GetStateFieldValue(GameContext& context, WrenHandle* stateInstanceHandle,
                                          const std::string& fieldName, WrenItem& out_value)
{
	const std::vector<WrenExportedField>& fields = GetStateExportedFields(context, stateInstanceHandle);
	for (const WrenExportedField& field : fields)
	{
		if (field.name == fieldName)
		{
			WrenVM* vm = context.GetWrenScriptEngine().GetVM();
			return ExportedFields::GetValue(vm, stateInstanceHandle, field, out_value);
		}
	}
	return false;
}

bool WrenStateManager::SetStateFieldValue(GameContext& context, WrenHandle* stateInstanceHandle,
                                          const std::string& fieldName, const WrenItem& value)
{
	const std::vector<WrenExportedField>& fields = GetStateExportedFields(context, stateInstanceHandle);
	for (const WrenExportedField& field : fields)
	{
		if (field.name == fieldName)
		{
			WrenVM* vm = context.GetWrenScriptEngine().GetVM();
			return ExportedFields::SetValue(vm, stateInstanceHandle, field, value);
		}
	}
	return false;
}
#endif

}  // namespace Struktur::Wren
