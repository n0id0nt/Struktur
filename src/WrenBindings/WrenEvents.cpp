#include "Engine/Callback/CallbackHelperFunctions.h"
#include "Engine/Event/Event.h"
#include "Engine/Event/EventManager.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "wren.hpp"

// Event.addEvent(type)
void wren_EventAddEvent(WrenVM* vm)
{
	Struktur::GameContext* context              = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Event::EventManager& eventManager = context->GetEventManager();

	const char* type = wrenGetSlotString(vm, 1);

	eventManager.AddEvent(type, nullptr);
}

// Event.addEvent(type,data)
void wren_EventAddEventAndData(WrenVM* vm)
{
	Struktur::GameContext* context              = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Event::EventManager& eventManager = context->GetEventManager();

	const char* type                 = wrenGetSlotString(vm, 1);
	Struktur::Callback::Variant data = Struktur::Callback::HelperFunctions::WrenSlotToVariant(vm, 2);

	eventManager.AddEvent(type, data);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

WREN_BINDING_MODULE(Events)
{
	WREN_CLASS_STATIC(registry, "events", "Event", "addEvent(_)", wren_EventAddEvent,
	                  "Adds an event to the event queue.");
	WREN_CLASS_STATIC(registry, "events", "Event", "addEvent(_,_)", wren_EventAddEventAndData,
	                  "Adds an event to the event queue.");
}
