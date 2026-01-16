#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Debug/Profiling/Profiler.h"

// Profile.begin(name)
void wren_ProfileStaticProfileBegin(WrenVM* vm)
{
#ifdef DEBUG
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	const char* name = wrenGetSlotString(vm, 1);
	context->GetProfiler().BeginProfile(name);
#endif
}

// Profile.end()
void wren_ProfileStaticProfileEnd(WrenVM* vm)
{
#ifdef DEBUG
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	context->GetProfiler().EndProfile();
#endif
}

WREN_CLASS_STATIC("debug", "Profile", "begin(_)", wren_ProfileStaticProfileBegin, "Begin a profile scope.");
WREN_CLASS_STATIC("debug", "Profile", "end()", wren_ProfileStaticProfileEnd, "End a profile scope.");
