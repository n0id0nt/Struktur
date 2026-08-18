#include "Engine/Audio/Mixer.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "wren.hpp"

// ============================================================================
// AUDIO BINDINGS
// ============================================================================

// Audio.masterVolume -> number (0 silent, 1 unattenuated)
void wren_AudioGetMasterVolume(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	wrenSetSlotDouble(vm, 0, context->GetMixer().GetMasterVolume());
}

// Audio.setMasterVolume(volume)
void wren_AudioSetMasterVolume(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	float volume                   = (float)wrenGetSlotDouble(vm, 1);
	context->GetMixer().SetMasterVolume(volume);
}

WREN_BINDING_MODULE(Audio)
{
	WREN_CLASS_STATIC(registry, "audio", "Audio", "masterVolume", wren_AudioGetMasterVolume,
	                  "Gets the master volume applied on top of every track's own gain (0 silent, 1 unattenuated)");
	WREN_CLASS_STATIC(registry, "audio", "Audio", "setMasterVolume(_)", wren_AudioSetMasterVolume,
	                  "Sets the master volume applied on top of every track's own gain (0 silent, 1 unattenuated)");
}
