#pragma once

#include "wren.hpp"

#include "Engine/Callback/Callback.h"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrapper for DialogueResult
struct WrenFunctionCallback
{
	Struktur::Callback::ICallback* callback;

	WrenFunctionCallback(Struktur::Callback::ICallback* callback) : callback(callback) {}
};

void wrenSetSlotCallback(WrenVM* vm, int slot, Struktur::Callback::ICallback* callback);