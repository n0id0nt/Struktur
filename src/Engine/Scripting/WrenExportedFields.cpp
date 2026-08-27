#include "WrenExportedFields.h"

#ifdef DEBUG
#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenUtil.h"
#include "Engine/Scripting/WrenValueWrapper.h"

namespace
{
// Interpreted once, lazily, into its own module - shared across every caller for the lifetime of the process
// (not per-caller, not per-class). Walks a class's compile-time Attributes (stock Wren feature) to find
// getters tagged #!export, and whether a matching setter exists. There is no way to enumerate a Wren Map's
// keys from the public C API, so this has to be done in Wren itself: Reflect.getMethods() (this project's own
// WrenExtensions/Modules/Reflect) gives every method signature on the class, and each is checked against the
// attributes map by known key instead of iterating it directly.
constexpr const char* kExportReflectionModule    = "__export_reflection";
constexpr const char* kExportReflectionClassName = "ExportedFieldReflection";
constexpr const char* kExportReflectionSource    = R"WREN(
import "reflect" for Reflect

class ExportedFieldReflection {
  static discover(cls) {
    var result = []
    // cls.attributes is null (not an empty Map) for a class with zero #!-tagged attributes anywhere in its
    // body - most classes never opt into any exported field, so this is the common case, not an edge case.
    if (cls.attributes == null) return result
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

WrenHandle* g_exportReflectionClassHandle = nullptr;
WrenHandle* g_exportReflectionCallHandle  = nullptr;

void EnsureExportReflectionBootstrap(Struktur::GameContext& context)
{
	if (g_exportReflectionClassHandle)
	{
		return;
	}

	Struktur::Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                                     = scriptEngine.GetVM();
	if (!vm)
	{
		return;
	}

	if (!scriptEngine.InterpretString(kExportReflectionModule, kExportReflectionSource))
	{
		DEBUG_ERROR("[ExportedFields] Failed to load export-reflection bootstrap module");
		return;
	}

	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, kExportReflectionModule, kExportReflectionClassName, 0);
	g_exportReflectionClassHandle = wrenGetSlotHandle(vm, 0);
	g_exportReflectionCallHandle  = wrenMakeCallHandle(vm, "discover(_)");
}
}  // namespace

std::vector<Struktur::Wren::WrenExportedField> Struktur::Wren::ExportedFields::DiscoverExportedFields(
    GameContext& context, WrenHandle* classHandle, const std::string& classNameForLogging)
{
	std::vector<WrenExportedField> fields;

	WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                     = scriptEngine.GetVM();
	if (!vm || !classHandle)
	{
		return fields;
	}

	EnsureExportReflectionBootstrap(context);
	if (!g_exportReflectionClassHandle || !g_exportReflectionCallHandle)
	{
		return fields;
	}

	wrenEnsureSlots(vm, 4);
	wrenSetSlotHandle(vm, 0, g_exportReflectionClassHandle);
	wrenSetSlotHandle(vm, 1, classHandle);

	if (wrenCall(vm, g_exportReflectionCallHandle) != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[ExportedFields] Failed to discover exported fields for %s", classNameForLogging.c_str());
		return fields;
	}

	// wrenCall() truncates the API stack down to whatever the called closure itself needed, so the slot count
	// ensured before the call does not carry over - must re-ensure before using slots 1-3.
	wrenEnsureSlots(vm, 4);

	// Slot 0 now holds the List<Map{"name": String, "hasSetter": Bool}> returned by discover().
	int fieldCount = wrenGetListCount(vm, 0);
	fields.reserve(fieldCount);

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

		fields.push_back(field);
	}

	return fields;
}

bool Struktur::Wren::ExportedFields::GetValue(WrenVM* vm, WrenHandle* instanceHandle, const WrenExportedField& field,
                                              WrenItem& out_value)
{
	if (!vm || !instanceHandle || !field.getterHandle)
	{
		return false;
	}

	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, instanceHandle);

	if (wrenCall(vm, field.getterHandle) != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[ExportedFields] Error reading exported field '%s'", field.name.c_str());
		return false;
	}

	out_value = Util::GetWrenItemFromSlot(vm, 0);
	return true;
}

bool Struktur::Wren::ExportedFields::SetValue(WrenVM* vm, WrenHandle* instanceHandle, const WrenExportedField& field,
                                              const WrenItem& value)
{
	if (!vm || !instanceHandle || !field.setterHandle)
	{
		return false;
	}

	wrenEnsureSlots(vm, 2);
	wrenSetSlotHandle(vm, 0, instanceHandle);
	Util::SetSlotFromWrenItem(vm, 1, value);

	if (wrenCall(vm, field.setterHandle) != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("[ExportedFields] Error writing exported field '%s'", field.name.c_str());
		return false;
	}

	return true;
}

void Struktur::Wren::ExportedFields::ReleaseFields(WrenVM* vm, std::vector<WrenExportedField>& fields)
{
	if (!vm)
	{
		return;
	}

	for (WrenExportedField& field : fields)
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
	fields.clear();
}

void Struktur::Wren::ExportedFields::ShutdownBootstrap(WrenVM* vm)
{
	if (!vm)
	{
		return;
	}

	if (g_exportReflectionClassHandle)
	{
		wrenReleaseHandle(vm, g_exportReflectionClassHandle);
		g_exportReflectionClassHandle = nullptr;
	}
	if (g_exportReflectionCallHandle)
	{
		wrenReleaseHandle(vm, g_exportReflectionCallHandle);
		g_exportReflectionCallHandle = nullptr;
	}
}
#endif
