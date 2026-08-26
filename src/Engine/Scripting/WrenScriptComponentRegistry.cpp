#include "WrenScriptComponentRegistry.h"

#include <format>

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#ifdef DEBUG
	#include <sys/stat.h>

	#include "Engine/ECS/Component/WrenScript.h"
#endif

void Struktur::Wren::WrenScriptComponentRegistry::RegisterScriptComponent(std::string module, std::string className)
{
	auto [it, inserted] = m_scriptComponents.insert({className, {module, className}});
	if (!inserted)
	{
		DEBUG_ERROR("[WrenScriptComponentRegistry] Script %s: has already been added to wren script registy",
		            className.c_str());
	}
}

bool Struktur::Wren::WrenScriptComponentRegistry::LoadAllScriptComponents(GameContext& context)
{
	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();

	if (!vm)
	{
		DEBUG_ERROR("[WrenScriptComponentRegistry] Wren VM not initialised");
		return false;
	}

	// wrenGetVariable() (used below to check fixedUpdate() support via Reflect.hasMethod) only looks up an
	// already-loaded module's variable table - unlike an `import` statement, it does not itself trigger module
	// loading. Nothing is guaranteed to have imported "reflect" yet at this point (only Player.wren happens to,
	// today), so force it to load once here via a real import statement, same mechanism Wren's own `import`
	// would use (see WrenScriptEngine::OnLoadModule's "reflect" special case).
	scriptEngine.InterpretString("__ensure_reflect_loaded", "import \"reflect\" for Reflect\n");

	bool successful = true;
	for (auto& it : m_scriptComponents)
	{
		WrenScriptComponent& scriptComponent = it.second;
		const std::string& className         = it.first;
		const std::string& module            = scriptComponent.module;

		if (scriptComponent.isInitialised)
		{
			DEBUG_WARNING("[WrenScriptComponentRegistry] %s.wren has already been initialised", module.c_str());
			continue;
		}

		DEBUG_INFO("[WrenScriptComponentRegistry] Initializing %s.wren", module.c_str());

		// Load
		if (!scriptEngine.InterpretFile(std::format("Scripts/{}.wren", module).c_str()))
		{
			DEBUG_ERROR("[WrenScriptComponentRegistry] Failed to load %s.wren", module.c_str());
			DEBUG_ERROR("[WrenScriptComponentRegistry] %s.wren must be in the scripts folder", module.c_str());
			successful = false;
			continue;
		}

		// Get class
		wrenEnsureSlots(vm, 1);
		wrenGetVariable(vm, className.c_str(), className.c_str(), 0);

		if (wrenGetSlotType(vm, 0) == WREN_TYPE_NULL)
		{
			DEBUG_ERROR("[WrenScriptComponentRegistry] %s class not found in %s", className.c_str(), module.c_str());
			successful = false;
			continue;
		}

		scriptComponent.classHandle           = wrenGetSlotHandle(vm, 0);
		scriptComponent.startMethodHandle     = wrenMakeCallHandle(vm, "start()");
		scriptComponent.updateMethodHandle    = wrenMakeCallHandle(vm, "update()");
		scriptComponent.onDestroyMethodHandle = wrenMakeCallHandle(vm, "onDestroy()");
		scriptComponent.onEventMethodHandle   = wrenMakeCallHandle(vm, "onEvent(_,_)");

		// fixedUpdate() is opt-in (see the field's own comment) - only create the handle if the class actually
		// implements it, via Reflect.hasMethod (the same "reflect" module/Reflect class Player.wren and the
		// exported-fields bootstrap already use).
		wrenEnsureSlots(vm, 3);
		wrenGetVariable(vm, "reflect", "Reflect", 0);
		wrenSetSlotHandle(vm, 1, scriptComponent.classHandle);
		wrenSetSlotString(vm, 2, "fixedUpdate()");
		WrenHandle* hasMethodCall = wrenMakeCallHandle(vm, "hasMethod(_,_)");
		bool hasFixedUpdate       = wrenCall(vm, hasMethodCall) == WREN_RESULT_SUCCESS && wrenGetSlotBool(vm, 0);
		wrenReleaseHandle(vm, hasMethodCall);
		if (hasFixedUpdate)
		{
			scriptComponent.fixedUpdateMethodHandle = wrenMakeCallHandle(vm, "fixedUpdate()");
		}
#ifdef DEBUG
		m_fileModificationTimes[module] = GetFileModificationTime(module);
#endif
		scriptComponent.isInitialised = true;
		DEBUG_INFO("Initialised Wren script: %s (%s)", module.c_str(), className.c_str());
	}

	return successful;
}

Struktur::Wren::WrenScriptComponent* Struktur::Wren::WrenScriptComponentRegistry::TryGetScriptComponent(
    std::string className)
{
	auto it = m_scriptComponents.find(className);
	if (it != m_scriptComponents.end())
	{
		return &it->second;
	}
	return nullptr;
}

void Struktur::Wren::WrenScriptComponentRegistry::Clear(GameContext& context)
{
	Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                           = scriptEngine.GetVM();
	if (!vm)
	{
		return;
	}

	for (auto& it : m_scriptComponents)
	{
		WrenScriptComponent& scriptComponent = it.second;
		const std::string& className         = it.first;

		if (!scriptComponent.isInitialised)
		{
			continue;
		}

		if (scriptComponent.classHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.classHandle);
		}
		if (scriptComponent.startMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.startMethodHandle);
		}
		if (scriptComponent.updateMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.updateMethodHandle);
		}
		if (scriptComponent.fixedUpdateMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.fixedUpdateMethodHandle);
		}
		if (scriptComponent.onDestroyMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.onDestroyMethodHandle);
		}
		if (scriptComponent.onEventMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.onEventMethodHandle);
		}

		scriptComponent.classHandle              = nullptr;
		scriptComponent.startMethodHandle        = nullptr;
		scriptComponent.updateMethodHandle       = nullptr;
		scriptComponent.fixedUpdateMethodHandle  = nullptr;
		scriptComponent.onDestroyMethodHandle    = nullptr;
		scriptComponent.onEventMethodHandle      = nullptr;

#ifdef DEBUG
		ExportedFields::ReleaseFields(vm, scriptComponent.exportedFields);
		scriptComponent.exportedFieldsResolved = false;
#endif

		scriptComponent.isInitialised = false;
	}

#ifdef DEBUG
	ExportedFields::ShutdownBootstrap(vm);
#endif

	m_scriptComponents.clear();
}

#ifdef DEBUG
void Struktur::Wren::WrenScriptComponentRegistry::CheckForScriptChanges(GameContext& context)
{
	auto& registry = context.GetRegistry();
	auto view      = registry.view<Component::WrenScript>();

	for (auto entity : view)
	{
		auto& script = view.get<Component::WrenScript>(entity);

		if (!script.isInitialised)
		{
			continue;
		}

		time_t currentModTime = GetFileModificationTime(script.filePath);

		auto it = m_fileModificationTimes.find(script.filePath);
		if (it != m_fileModificationTimes.end() && currentModTime > it->second)
		{
			DEBUG_INFO("Script file changed, reloading: %s", script.filePath.c_str());
			ReloadScript(context, entity, script);
			m_fileModificationTimes[script.filePath] = currentModTime;
		}
	}
}

void Struktur::Wren::WrenScriptComponentRegistry::ReloadScript(GameContext& context, entt::entity entity,
                                                               Component::WrenScript& script)
{
	//// Destroy old script
	// DestroyScript(context, entity, script);
	//
	//// Reset error state
	// script.hasError = false;
	// script.errorMessage.clear();
	//
	//// Reinitialise
	// if (InitialiseScript(context, entity, script))
	//{
	//	CallCreate(context, entity, script);
	//	DEBUG_INFO("Successfully reloaded script: %s", script.scriptPath.c_str());
	// }
	// else
	//{
	//	DEBUG_ERROR("Failed to reload script: %s", script.scriptPath.c_str());
	// }
}

time_t Struktur::Wren::WrenScriptComponentRegistry::GetFileModificationTime(const std::string& path)
{
	struct stat fileInfo;
	if (stat(path.c_str(), &fileInfo) == 0)
	{
		return fileInfo.st_mtime;
	}
	return 0;
}

void Struktur::Wren::WrenScriptComponentRegistry::ResolveExportedFields(GameContext& context,
                                                                        WrenScriptComponent& scriptComponent)
{
	// Set up front regardless of outcome below - if this fails there's nothing that would make a retry
	// succeed later (the class is already loaded by the time this runs), so don't keep re-attempting it
	// every time the inspector asks.
	scriptComponent.exportedFieldsResolved = true;

	scriptComponent.exportedFields =
	    ExportedFields::DiscoverExportedFields(context, scriptComponent.classHandle, scriptComponent.className);
}

const std::vector<Struktur::Wren::WrenExportedField>& Struktur::Wren::WrenScriptComponentRegistry::GetExportedFields(
    GameContext& context, const std::string& className)
{
	static const std::vector<WrenExportedField> kEmpty;

	auto it = m_scriptComponents.find(className);
	if (it == m_scriptComponents.end() || !it->second.isInitialised)
	{
		return kEmpty;
	}

	WrenScriptComponent& scriptComponent = it->second;
	if (!scriptComponent.exportedFieldsResolved)
	{
		ResolveExportedFields(context, scriptComponent);
	}

	return scriptComponent.exportedFields;
}
#endif
