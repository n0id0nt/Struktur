#include "wren.hpp"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Flag/FlagManager.h"

// ============================================================================
// FLAG MANAGER BINDINGS - Boolean Flags
// ============================================================================

// FlagManager.getFlag(name)
void wren_FlagManagerGetFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	bool value = flagManager.GetFlag(flagName);
	wrenSetSlotBool(vm, 0, value);
}

// FlagManager.setFlag(name, value)
void wren_FlagManagerSetFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	bool value = wrenGetSlotBool(vm, 2);
	flagManager.SetFlag(flagName, value);
}

// FlagManager.hasFlag(name)
void wren_FlagManagerHasFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	bool exists = flagManager.HasFlag(flagName);
	wrenSetSlotBool(vm, 0, exists);
}

// FlagManager.removeFlag(name)
void wren_FlagManagerRemoveFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	flagManager.RemoveFlag(flagName);
}

// ============================================================================
// FLAG MANAGER BINDINGS - Integer Flags
// ============================================================================

// FlagManager.getIntFlag(name)
void wren_FlagManagerGetIntFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	int value = flagManager.GetIntFlag(flagName);
	wrenSetSlotDouble(vm, 0, static_cast<double>(value));
}

// FlagManager.setIntFlag(name, value)
void wren_FlagManagerSetIntFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	int value = static_cast<int>(wrenGetSlotDouble(vm, 2));
	flagManager.SetIntFlag(flagName, value);
}

// FlagManager.hasIntFlag(name)
void wren_FlagManagerHasIntFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	bool exists = flagManager.HasIntFlag(flagName);
	wrenSetSlotBool(vm, 0, exists);
}

// FlagManager.removeIntFlag(name)
void wren_FlagManagerRemoveIntFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	flagManager.RemoveIntFlag(flagName);
}

// ============================================================================
// FLAG MANAGER BINDINGS - Float Flags
// ============================================================================

// FlagManager.getFloatFlag(name)
void wren_FlagManagerGetFloatFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	float value = flagManager.GetFloatFlag(flagName);
	wrenSetSlotDouble(vm, 0, static_cast<double>(value));
}

// FlagManager.setFloatFlag(name, value)
void wren_FlagManagerSetFloatFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	float value = static_cast<float>(wrenGetSlotDouble(vm, 2));
	flagManager.SetFloatFlag(flagName, value);
}

// FlagManager.hasFloatFlag(name)
void wren_FlagManagerHasFloatFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	bool exists = flagManager.HasFloatFlag(flagName);
	wrenSetSlotBool(vm, 0, exists);
}

// FlagManager.removeFloatFlag(name)
void wren_FlagManagerRemoveFloatFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	flagManager.RemoveFloatFlag(flagName);
}

// ============================================================================
// FLAG MANAGER BINDINGS - String Flags
// ============================================================================

// FlagManager.getStringFlag(name)
void wren_FlagManagerGetStringFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	std::string value = flagManager.GetStringFlag(flagName);
	wrenSetSlotString(vm, 0, value.c_str());
}

// FlagManager.setStringFlag(name, value)
void wren_FlagManagerSetStringFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	const char* value = wrenGetSlotString(vm, 2);
	flagManager.SetStringFlag(flagName, value);
}

// FlagManager.hasStringFlag(name)
void wren_FlagManagerHasStringFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	bool exists = flagManager.HasStringFlag(flagName);
	wrenSetSlotBool(vm, 0, exists);
}

// FlagManager.removeStringFlag(name)
void wren_FlagManagerRemoveStringFlag(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	
	const char* flagName = wrenGetSlotString(vm, 1);
	flagManager.RemoveStringFlag(flagName);
}

// ============================================================================
// FLAG MANAGER BINDINGS - Utility
// ============================================================================

// FlagManager.clear()
void wren_FlagManagerClear(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Flag::FlagManager& flagManager = context->GetFlagManager();
	flagManager.Clear();
}

// Register FlagManager static methods
WREN_CLASS_STATIC("flags", "FlagManager", "getFlag(_)", wren_FlagManagerGetFlag, "Get a boolean flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "setFlag(_,_)", wren_FlagManagerSetFlag, "Set a boolean flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "hasFlag(_)", wren_FlagManagerHasFlag, "Check if a boolean flag exists");
WREN_CLASS_STATIC("flags", "FlagManager", "removeFlag(_)", wren_FlagManagerRemoveFlag, "Remove a boolean flag");

WREN_CLASS_STATIC("flags", "FlagManager", "getIntFlag(_)", wren_FlagManagerGetIntFlag, "Get an integer flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "setIntFlag(_,_)", wren_FlagManagerSetIntFlag, "Set an integer flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "hasIntFlag(_)", wren_FlagManagerHasIntFlag, "Check if an integer flag exists");
WREN_CLASS_STATIC("flags", "FlagManager", "removeIntFlag(_)", wren_FlagManagerRemoveIntFlag, "Remove an integer flag");

WREN_CLASS_STATIC("flags", "FlagManager", "getFloatFlag(_)", wren_FlagManagerGetFloatFlag, "Get a float flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "setFloatFlag(_,_)", wren_FlagManagerSetFloatFlag, "Set a float flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "hasFloatFlag(_)", wren_FlagManagerHasFloatFlag, "Check if a float flag exists");
WREN_CLASS_STATIC("flags", "FlagManager", "removeFloatFlag(_)", wren_FlagManagerRemoveFloatFlag, "Remove a float flag");

WREN_CLASS_STATIC("flags", "FlagManager", "getStringFlag(_)", wren_FlagManagerGetStringFlag, "Get a string flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "setStringFlag(_,_)", wren_FlagManagerSetStringFlag, "Set a string flag value");
WREN_CLASS_STATIC("flags", "FlagManager", "hasStringFlag(_)", wren_FlagManagerHasStringFlag, "Check if a string flag exists");
WREN_CLASS_STATIC("flags", "FlagManager", "removeStringFlag(_)", wren_FlagManagerRemoveStringFlag, "Remove a string flag");

WREN_CLASS_STATIC("flags", "FlagManager", "clear()", wren_FlagManagerClear, "Clear all flags");
