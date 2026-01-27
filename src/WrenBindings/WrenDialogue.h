// WrenDialogue.h
// Wren bindings for dialogue system
// Part of the Struktur dialogue system

#pragma once

#include "Dialogue/DialogueLoader.h"

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

// ============================================================================
// BINDING FUNCTIONS
// ============================================================================

// DialogueData foreign class
void wren_DialogueDataAllocate(WrenVM* vm);
void wren_DialogueDataFinalize(void* data);
void wren_DialogueDataAddNodeSimple(WrenVM* vm);
void wren_DialogueDataLoadIntoManager(WrenVM* vm);

// DialogueRegistry static methods
void wren_DialogueRegistryRegisterCondition(WrenVM* vm);
void wren_DialogueRegistryRegisterCommand(WrenVM* vm);
void wren_DialogueRegistryRegisterOperator(WrenVM* vm);
void wren_DialogueRegistryEvalOperator(WrenVM* vm);

// DialogueManager static methods
void wren_DialogueManagerStartDialogue(WrenVM* vm);
void wren_DialogueManagerMakeChoice(WrenVM* vm);
void wren_DialogueManagerContinue(WrenVM* vm);
void wren_DialogueManagerIsDialogueActive(WrenVM* vm);
void wren_DialogueManagerGetCurrentNodeId(WrenVM* vm);
void wren_DialogueManagerGetNodeCount(WrenVM* vm);
void wren_DialogueManagerClearAllNodes(WrenVM* vm);

// Helper functions
void RegisterDialogueBindings();
