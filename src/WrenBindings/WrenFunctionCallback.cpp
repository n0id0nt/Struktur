#include "WrenFunctionCallback.h"

#include "Debug/Assertions.h"
#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/Callback/Variant.h"
#include "Engine/Callback/WrenCallback.h"
#include "Engine/Scripting/WrenBindingRegistry.h"

void wrenSetSlotCallback(WrenVM* vm, int slot, Struktur::Callback::ICallback* callback)
{
	wrenEnsureSlots(vm, slot + 2);

	if (callback == nullptr)
	{
		wrenSetSlotNull(vm, slot);
		return;
	}

	if (auto* wrenCallback = dynamic_cast<Struktur::Callback::WrenCallback*>(callback))
	{
		// Already a Wren closure handle - push it directly onto the stack
		// No need to wrap it, Wren can call it natively with .call()
		wrenSetSlotHandle(vm, slot, wrenCallback->GetHandle());
	}
	else
	{
		// C++ callback - wrap it in a WrenCallback foreign object so Wren
		// can call it via FunctionCallback.call()
		wrenGetVariable(vm, "callback", "FunctionCallback", slot + 1);
		WrenFunctionCallback* wrenCb =
		    static_cast<WrenFunctionCallback*>(wrenSetSlotNewForeign(vm, slot, slot + 1, sizeof(WrenFunctionCallback)));

		// Placement new to properly initialise the WrenCallback wrapper
		new (wrenCb) WrenFunctionCallback(callback);
	}
}

// ============================================================================
// CALLBACK BINDINGS
// ============================================================================

void wren_FunctionCallbackAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenFunctionCallback));
}

void wren_FunctionCallbackFinalize(void* data)
{
	WrenFunctionCallback* callback = static_cast<WrenFunctionCallback*>(data);
	callback->~WrenFunctionCallback();
}

// functionCallback.call()
void wren_FunctionCallbackCall(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenFunctionCallback* callback = static_cast<WrenFunctionCallback*>(wrenGetSlotForeign(vm, 0));

	int slotCount = wrenGetSlotCount(vm);
	std::vector<Struktur::Callback::Variant> args;
	args.reserve(slotCount - 1);

	for (int i = 1; i < slotCount; ++i)
	{
		args.push_back(Struktur::Callback::HelperFunctions::WrenSlotToVariant(vm, i));
	}

	Struktur::Callback::Variant result = callback->callback->Invoke(*context, args);
	Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, 0, result);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================
WREN_BINDING_MODULE(FunctionCallback)
{
	// DialogueData foreign class
	WREN_FOREIGN_CLASS(registry, "callback", "FunctionCallback", wren_FunctionCallbackAllocate,
	                   wren_FunctionCallbackFinalize, "Container for Callback");

	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call()", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_,_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_,_,_,_,_,_)", wren_FunctionCallbackCall,
	                  "Execute the callback");
	WREN_CLASS_METHOD(registry, "callback", "FunctionCallback", "call(_,_,_,_,_,_,_,_,_,_,_)",
	                  wren_FunctionCallbackCall, "Execute the callback");
}
