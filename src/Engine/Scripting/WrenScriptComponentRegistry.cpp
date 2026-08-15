#include "WrenScriptComponentRegistry.h"

#include <format>

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#ifdef DEBUG
	#include <sys/stat.h>

	#include "Engine/ECS/Component/WrenScript.h"

namespace
{
// Interpreted once, lazily, into its own module. Walks a class's compile-time Attributes (stock Wren
// feature - see third_party/wren doc/site/classes.markdown "Attributes") to find getters tagged
// #!export, and whether a matching setter exists. There is no way to enumerate a Wren Map's keys from
// the public C API, so this has to be done in Wren itself: Reflect.getMethods() (this project's own
// WrenExtensions/Modules/Reflect) gives every method signature on the class, and each is checked
// against the attributes map by known key instead of iterating it directly.
constexpr const char* kExportReflectionModule    = "__export_reflection";
constexpr const char* kExportReflectionClassName = "ExportedFieldReflection";
constexpr const char* kExportReflectionSource    = R"WREN(
import "reflect" for Reflect

class ExportedFieldReflection {
  static discover(cls) {
    var result = []
    var methods = Reflect.getMethods(cls)
    var attrs = cls.attributes.methods
    for (sig in methods) {
      if (attrs.containsKey(sig)) {
        var declAttrs = attrs[sig]
        if (declAttrs.containsKey(null) && declAttrs[null].containsKey("export")) {
          var entry = {}
          entry["name"] = sig
          entry["hasSetter"] = Reflect.hasMethod(cls, sig + "=(_)")
          result.add(entry)
        }
      }
    }
    return result
  }
}
)WREN";
}  // namespace
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
		if (scriptComponent.onDestroyMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.onDestroyMethodHandle);
		}
		if (scriptComponent.onEventMethodHandle)
		{
			wrenReleaseHandle(vm, scriptComponent.onEventMethodHandle);
		}

		scriptComponent.classHandle           = nullptr;
		scriptComponent.startMethodHandle     = nullptr;
		scriptComponent.updateMethodHandle    = nullptr;
		scriptComponent.onDestroyMethodHandle = nullptr;
		scriptComponent.onEventMethodHandle   = nullptr;

#ifdef DEBUG
		for (WrenExportedField& field : scriptComponent.exportedFields)
		{
			if (field.getterHandle)
			{
				wrenReleaseHandle(vm, field.getterHandle);
			}
			if (field.setterHandle)
			{
				wrenReleaseHandle(vm, field.setterHandle);
			}
		}
		scriptComponent.exportedFields.clear();
		scriptComponent.exportedFieldsResolved = false;
#endif

		scriptComponent.isInitialised = false;
	}

#ifdef DEBUG
	if (m_exportReflectionClassHandle)
	{
		wrenReleaseHandle(vm, m_exportReflectionClassHandle);
		m_exportReflectionClassHandle = nullptr;
	}
	if (m_exportReflectionCallHandle)
	{
		wrenReleaseHandle(vm, m_exportReflectionCallHandle);
		m_exportReflectionCallHandle = nullptr;
	}
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

void Struktur::Wren::WrenScriptComponentRegistry::EnsureExportReflectionBootstrap(GameContext& context)
{
	if (m_exportReflectionClassHandle)
	{
		return;
	}

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();
	if (!vm)
	{
		return;
	}

	if (!scriptEngine.InterpretString(kExportReflectionModule, kExportReflectionSource))
	{
		DEBUG_ERROR("[WrenScriptComponentRegistry] Failed to load export-reflection bootstrap module");
		return;
	}

	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, kExportReflectionModule, kExportReflectionClassName, 0);
	m_exportReflectionClassHandle = wrenGetSlotHandle(vm, 0);
	m_exportReflectionCallHandle  = wrenMakeCallHandle(vm, "discover(_)");
}

void Struktur::Wren::WrenScriptComponentRegistry::ResolveExportedFields(GameContext& context,
                                                                        WrenScriptComponent& scriptComponent)
{
	// Set up front regardless of outcome below - if this fails there's nothing that would make a retry
	// succeed later (the class is already loaded by the time this runs), so don't keep re-attempting it
	// every time the inspector asks.
	scriptComponent.exportedFieldsResolved = true;

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();
	if (!vm || !scriptComponent.classHandle)
	{
		return;
	}

	EnsureExportReflectionBootstrap(context);
	if (!m_exportReflectionClassHandle || !m_exportReflectionCallHandle)
	{
		return;
	}

	wrenEnsureSlots(vm, 4);
	wrenSetSlotHandle(vm, 0, m_exportReflectionClassHandle);
	wrenSetSlotHandle(vm, 1, scriptComponent.classHandle);

	if (wrenCall(vm, m_exportReflectionCallHandle) != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[WrenScriptComponentRegistry] Failed to discover exported fields for %s",
		            scriptComponent.className.c_str());
		return;
	}

	// wrenCall() truncates the API stack down to whatever the called closure itself needed, so the
	// slot count ensured before the call does not carry over - must re-ensure before using slots 1-3.
	wrenEnsureSlots(vm, 4);

	// Slot 0 now holds the List<Map{"name": String, "hasSetter": Bool}> returned by discover().
	int fieldCount = wrenGetListCount(vm, 0);
	scriptComponent.exportedFields.reserve(fieldCount);

	for (int i = 0; i < fieldCount; i++)
	{
		wrenGetListElement(vm, 0, i, 1);  // slot 1 = entry map

		wrenSetSlotString(vm, 2, "name");
		wrenGetMapValue(vm, 1, 2, 3);
		std::string name = wrenGetSlotString(vm, 3);

		wrenSetSlotString(vm, 2, "hasSetter");
		wrenGetMapValue(vm, 1, 2, 3);
		bool hasSetter = wrenGetSlotBool(vm, 3);

		WrenExportedField field;
		field.name         = name;
		field.hasSetter    = hasSetter;
		field.getterHandle = wrenMakeCallHandle(vm, name.c_str());
		field.setterHandle = hasSetter ? wrenMakeCallHandle(vm, (name + "=(_)").c_str()) : nullptr;

		scriptComponent.exportedFields.push_back(field);
	}
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
