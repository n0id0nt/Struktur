#pragma once

#include "Engine/Dialogue/DialogueLoader.h"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrapper for DialogueData - used to build dialogue data in Wren
struct WrenDialogueData
{
	Struktur::Dialogue::DialogueDataMap dataMap;
};

// Wrapper for DialogueResult
struct WrenDialogueResult
{
	Struktur::Dialogue::DialogueResult dataResult;
};
