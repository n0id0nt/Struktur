#pragma once

#include "wren.hpp"
#include "Engine/Dialogue/DialogueStructures.h"
#include "Debug/Assertions.h"

// Wrapper for DialogueData to track ownership like WrenUIElement
struct WrenDialogueData
{
	Struktur::Dialogue::DialogueDataMap* dataMap;
	bool loadedIntoManager;

	WrenDialogueData()
		: dataMap(new Struktur::Dialogue::DialogueDataMap())
		, loadedIntoManager(false)
	{
	}

	~WrenDialogueData()
	{
		if (!loadedIntoManager)
		{
			DEBUG_WARNING("WrenDialogueData was not loaded into DialogueManager - memory may be wasted");
		}
		delete dataMap;
	}
};
