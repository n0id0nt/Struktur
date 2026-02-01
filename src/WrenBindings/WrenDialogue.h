#pragma once

#include "Engine/Dialogue/DialogueLoader.h"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

// Wrapper for DialogueResult
struct WrenDialogueResult
{
	Struktur::Dialogue::DialogueResult dataResult;

	WrenDialogueResult(Struktur::Dialogue::DialogueResult& dataResult) : dataResult(dataResult) {}
};
