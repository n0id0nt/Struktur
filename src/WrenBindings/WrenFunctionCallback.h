#pragma once

#include "Engine/Callback/Callback.h"
#include "wren.hpp"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrapper for DialogueResult
struct WrenFunctionCallback
{
	Struktur::Callback::ICallback* callback;

	WrenFunctionCallback(Struktur::Callback::ICallback* callback)
	    : callback(callback)
	{
	}
};

void wrenSetSlotCallback(WrenVM* vm, int slot, Struktur::Callback::ICallback* callback);
