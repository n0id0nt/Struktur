// WrenDialogue.h
// Wren bindings for dialogue system
// Part of the Struktur dialogue system

#pragma once

#include "Engine/Dialogue/DialogueLoader.h"

struct WrenVM;

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrapper for DialogueData - used to build dialogue data in Wren
struct WrenDialogueData
{
	Struktur::Dialogue::DialogueDataMap* dataMap;
	bool loadedIntoManager;

	WrenDialogueData();
	~WrenDialogueData();
};
