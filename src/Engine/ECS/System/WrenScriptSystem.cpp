#include "WrenScriptSystem.h"

#include <fstream>
#include <sstream>

#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/GameContext.h"

namespace Struktur::System
{

bool WrenScriptSystem::InitialiseScript(GameContext& context, entt::entity entity, Component::WrenScript& script)
{
	Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                           = scriptEngine.GetVM();
	if (!vm)
	{
		DEBUG_ERROR("Wren VM not initialised");
		script.hasError     = true;
		script.errorMessage = "Wren VM not initialised";
		return false;
	}

	Wren::WrenScriptComponentRegistry& scriptComponentRegistry = context.GetWrenScriptComponentRegistry();
	Wren::WrenScriptComponent* scriptComponent = scriptComponentRegistry.TryGetScriptComponent(script.className);
	if (!scriptComponent)
	{
		DEBUG_ERROR("Script Component %s not initialised", script.className.c_str());
		script.hasError     = true;
		script.errorMessage = "Wren VM not initialised";
		return false;
	}

	script.scriptComponent = scriptComponent;

	wrenEnsureSlots(vm, 2);
	wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
	// Call constructor: new(entity) or new(entity, args)
	WrenHandle* constructMethod;
	if (!script.constructorArgs.empty())
	{
		wrenEnsureSlots(vm, 5);
		// If constructor args exist, create signature with 2 params
		wrenSetSlotNewMap(vm, 2);
		for (const auto& arg : script.constructorArgs)
		{
			wrenSetSlotString(vm, 3, arg.identifier.c_str());
			switch (arg.type)
			{
				case WrenType::WREN_TYPE_NUM:
					wrenSetSlotDouble(vm, 4, std::any_cast<double>(arg.value));
					break;

				case WrenType::WREN_TYPE_BOOL:
					wrenSetSlotBool(vm, 4, std::any_cast<bool>(arg.value));
					break;

				case WrenType::WREN_TYPE_STRING:
					wrenSetSlotString(vm, 4, std::any_cast<std::string>(arg.value).c_str());
					break;

				default:
					wrenSetSlotNull(vm, 4);
					break;
			}

			// Append to the list
			wrenSetMapValue(vm, 2, 3, 4);
		}
		constructMethod = wrenMakeCallHandle(vm, "new(_,_)");
	}
	else
	{
		constructMethod = wrenMakeCallHandle(vm, "new(_)");
	}

	wrenSetSlotHandle(vm, 0, scriptComponent->classHandle);
	WrenInterpretResult callResult = wrenCall(vm, constructMethod);
	wrenReleaseHandle(vm, constructMethod);

	if (callResult != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("Failed to instantiate Wren class: %s", script.className.c_str());
		script.hasError     = true;
		script.errorMessage = "Instantiation failed";
		return false;
	}

	// Store instance handle (it's in slot 0 after constructor call)
	script.instanceHandle = wrenGetSlotHandle(vm, 0);

	script.isInitialised = true;

	return true;
}

bool WrenScriptSystem::CallStart(GameContext& context, Component::WrenScript& script)
{
	if (!script.isInitialised || script.hasError)
	{
		return false;
	}

	auto* startMethodHandle = script.scriptComponent->startMethodHandle;
	if (!startMethodHandle)
	{
		DEBUG_WARNING("Script %s does not have a Start method", script.className.c_str());
		return true;  // Not an error, just doesn't have the method
	}

	Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                           = scriptEngine.GetVM();

	wrenEnsureSlots(vm, 1);
	wrenSetSlotHandle(vm, 0, script.instanceHandle);

	WrenInterpretResult result = wrenCall(vm, startMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("Error calling Start() on script: %s", script.className.c_str());
		script.hasError     = true;
		script.errorMessage = "Start() call failed";
		return false;
	}

	return true;
}

void WrenScriptSystem::Update(GameContext& context)
{
	Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                           = scriptEngine.GetVM();
	if (!vm)
	{
		return;
	}

	auto& registry = context.GetRegistry();
	auto view      = registry.view<Component::WrenScript>();

	for (auto entity : view)
	{
		auto& script = view.get<Component::WrenScript>(entity);

		// Skip if not initialised or has error
		if (script.hasError)
		{
			continue;
		}

		// Initialise if needed
		// This should not be called here and be called as needed.
		if (!script.isInitialised && !script.instanceHandle)
		{
			DEBUG_WARNING(
			    "Script %s is not initialised - THIS IS HERE AS A TODO TO FIX PREVIOUS IMPLEMENTAITON OF INHERATENCE "
			    "ONVE DONE MAKE THIS AN ERROR",
			    script.className.c_str());
			if (!InitialiseScript(context, entity, script))
			{
				continue;
			}
			CallStart(context, script);
		}

		// Skip if no update method
		if (!script.scriptComponent->updateMethodHandle)
		{
			continue;
		}

		// Call Update()
		wrenEnsureSlots(vm, 1);
		wrenSetSlotHandle(vm, 0, script.instanceHandle);

		WrenInterpretResult result = wrenCall(vm, script.scriptComponent->updateMethodHandle);

		if (result != WREN_RESULT_SUCCESS)
		{
			DEBUG_ERROR("Error calling Update() on entity %d script: %s", static_cast<int>(entity),
			            script.className.c_str());
			script.hasError     = true;
			script.errorMessage = "Update() call failed";
		}
	}
}

void WrenScriptSystem::DestroyScript(GameContext& context, entt::entity entity, Component::WrenScript& script)
{
	if (!script.isInitialised)
	{
		return;
	}

	Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                           = scriptEngine.GetVM();
	if (!vm)
	{
		return;
	}

	// Call OnDestroy if it exists
	if (script.scriptComponent->onDestroyMethodHandle && script.instanceHandle)
	{
		wrenEnsureSlots(vm, 1);
		wrenSetSlotHandle(vm, 0, script.instanceHandle);

		WrenInterpretResult result = wrenCall(vm, script.scriptComponent->onDestroyMethodHandle);

		if (result != WREN_RESULT_SUCCESS)
		{
			DEBUG_WARNING("Error calling OnDestroy() on script: %s", script.className.c_str());
		}
	}

	// Release all handles
	if (script.instanceHandle)
	{
		wrenReleaseHandle(vm, script.instanceHandle);
	}

	script.instanceHandle  = nullptr;
	script.scriptComponent = nullptr;
	script.isInitialised   = false;
}

void WrenScriptSystem::SendEvent(GameContext& context, entt::entity entity, Component::WrenScript& script,
                                 const Event::Event& event)
{
	if (!script.isInitialised || script.hasError || !script.scriptComponent->onEventMethodHandle)
	{
		return;
	}

	Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
	WrenVM* vm                           = scriptEngine.GetVM();

	// Create event map in Wren
	wrenEnsureSlots(vm, 3);
	wrenSetSlotHandle(vm, 0, script.instanceHandle);

	// Add event data
	wrenSetSlotString(vm, 1, event.type.c_str());
	Callback::HelperFunctions::VariantToWrenSlot(vm, 2, event.data);

	// Call OnEvent(eventMap)
	WrenInterpretResult result = wrenCall(vm, script.scriptComponent->onEventMethodHandle);

	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("Error calling OnEvent() on script: %s", script.className.c_str());
	}
}

}  // namespace Struktur::System
