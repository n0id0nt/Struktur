#include "WrenDialogue.h"

#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"

// ============================================================================
// DIALOGUE DATA - Foreign class for building dialogue
// ============================================================================

// Allocator
void wren_DialogueDataAllocate(WrenVM* vm)
{
	WrenDialogueData* data = (WrenDialogueData*)wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenDialogueData));
	new (data) WrenDialogueData();
}

// Finalizer
void wren_DialogueDataFinalize(void* data)
{
	WrenDialogueData* dialogueData = (WrenDialogueData*)data;
	dialogueData->~WrenDialogueData();
}

// Helper function to extract map values from Wren
std::string GetMapString(WrenVM* vm, int mapSlot, const char* key, int tempSlot)
{
	wrenSetSlotString(vm, tempSlot, key);
	if (wrenGetMapContainsKey(vm, mapSlot, tempSlot))
	{
		wrenGetMapValue(vm, mapSlot, tempSlot, tempSlot);
		if (wrenGetSlotType(vm, tempSlot) == WREN_TYPE_STRING)
		{
			return wrenGetSlotString(vm, tempSlot);
		}
	}
	return "";
}

std::vector<Struktur::Dialogue::ConditionData> GetConditionList(WrenVM* vm, int listSlot)
{
	std::vector<Struktur::Dialogue::ConditionData> conditions;
	
	if (wrenGetSlotType(vm, listSlot) != WREN_TYPE_LIST)
		return conditions;
	
	int count = wrenGetListCount(vm, listSlot);
	for (int i = 0; i < count; ++i)
	{
		wrenGetListElement(vm, listSlot, i, listSlot + 1);
		
		if (wrenGetSlotType(vm, listSlot + 1) != WREN_TYPE_MAP)
			continue;
		
		Struktur::Dialogue::ConditionData condData;
		
		// Get type
		condData.type = GetMapString(vm, listSlot + 1, "type", listSlot + 2);
		
		// Get parameters map
		wrenSetSlotString(vm, listSlot + 2, "parameters");
		if (wrenGetMapContainsKey(vm, listSlot + 1, listSlot + 2))
		{
			wrenGetMapValue(vm, listSlot + 1, listSlot + 2, listSlot + 2);
			
			if (wrenGetSlotType(vm, listSlot + 2) == WREN_TYPE_MAP)
			{
				// Extract common parameters
				const char* paramNames[] = {"flag", "value", "op", "item"};
				for (const char* paramName : paramNames)
				{
					std::string value = GetMapString(vm, listSlot + 2, paramName, listSlot + 3);
					if (!value.empty())
					{
						condData.parameters[paramName] = value;
					}
				}
			}
		}
		
		conditions.push_back(condData);
	}
	
	return conditions;
}

std::vector<Struktur::Dialogue::CommandData> GetCommandList(WrenVM* vm, int listSlot)
{
	std::vector<Struktur::Dialogue::CommandData> commands;
	
	if (wrenGetSlotType(vm, listSlot) != WREN_TYPE_LIST)
		return commands;
	
	int count = wrenGetListCount(vm, listSlot);
	for (int i = 0; i < count; ++i)
	{
		wrenGetListElement(vm, listSlot, i, listSlot + 1);
		
		if (wrenGetSlotType(vm, listSlot + 1) != WREN_TYPE_MAP)
			continue;
		
		Struktur::Dialogue::CommandData cmdData;
		
		// Get type
		cmdData.type = GetMapString(vm, listSlot + 1, "type", listSlot + 2);
		
		// Get parameters map
		wrenSetSlotString(vm, listSlot + 2, "parameters");
		if (wrenGetMapContainsKey(vm, listSlot + 1, listSlot + 2))
		{
			wrenGetMapValue(vm, listSlot + 1, listSlot + 2, listSlot + 2);
			
			if (wrenGetSlotType(vm, listSlot + 2) == WREN_TYPE_MAP)
			{
				// Extract common parameters
				const char* paramNames[] = {"flag", "value", "item", "quantity", "id", "name"};
				for (const char* paramName : paramNames)
				{
					std::string value = GetMapString(vm, listSlot + 2, paramName, listSlot + 3);
					if (!value.empty())
					{
						cmdData.parameters[paramName] = value;
					}
				}
			}
		}
		
		commands.push_back(cmdData);
	}
	
	return commands;
}

std::vector<Struktur::Dialogue::ChoiceData> GetChoiceList(WrenVM* vm, int listSlot)
{
	std::vector<Struktur::Dialogue::ChoiceData> choices;
	
	if (wrenGetSlotType(vm, listSlot) != WREN_TYPE_LIST)
		return choices;
	
	int count = wrenGetListCount(vm, listSlot);
	for (int i = 0; i < count; ++i)
	{
		wrenGetListElement(vm, listSlot, i, listSlot + 1);
		
		if (wrenGetSlotType(vm, listSlot + 1) != WREN_TYPE_MAP)
			continue;
		
		Struktur::Dialogue::ChoiceData choiceData;
		
		// Get text
		choiceData.text = GetMapString(vm, listSlot + 1, "text", listSlot + 2);
		
		// Get target
		choiceData.targetNode = GetMapString(vm, listSlot + 1, "target", listSlot + 2);
		
		// Get conditions (optional)
		wrenSetSlotString(vm, listSlot + 2, "conditions");
		if (wrenGetMapContainsKey(vm, listSlot + 1, listSlot + 2))
		{
			wrenGetMapValue(vm, listSlot + 1, listSlot + 2, listSlot + 2);
			choiceData.conditions = GetConditionList(vm, listSlot + 2);
		}
		
		choices.push_back(choiceData);
	}
	
	return choices;
}

// DialogueData.addNode(nodeId, nodeData)
void wren_DialogueDataAddNode(WrenVM* vm)
{
	WrenDialogueData* data = (WrenDialogueData*)wrenGetSlotForeign(vm, 0);
	
	// Get node ID (slot 1)
	const char* nodeId = wrenGetSlotString(vm, 1);
	
	// Get node data map (slot 2)
	if (wrenGetSlotType(vm, 2) != WREN_TYPE_MAP)
	{
		DEBUG_ERROR("DialogueData.addNode: nodeData must be a map");
		return;
	}
	
	Struktur::Dialogue::NodeData nodeData;
	
	// Extract speaker
	nodeData.speaker = GetMapString(vm, 2, "speaker", 3);
	
	// Extract text
	nodeData.text = GetMapString(vm, 2, "text", 3);
	
	// Extract next node
	nodeData.nextNode = GetMapString(vm, 2, "next", 3);
	
	// Extract conditions
	wrenSetSlotString(vm, 3, "conditions");
	if (wrenGetMapContainsKey(vm, 2, 3))
	{
		wrenGetMapValue(vm, 2, 3, 3);
		nodeData.conditions = GetConditionList(vm, 3);
	}
	
	// Extract commands
	wrenSetSlotString(vm, 3, "commands");
	if (wrenGetMapContainsKey(vm, 2, 3))
	{
		wrenGetMapValue(vm, 2, 3, 3);
		nodeData.commands = GetCommandList(vm, 3);
	}
	
	// Extract choices
	wrenSetSlotString(vm, 3, "choices");
	if (wrenGetMapContainsKey(vm, 2, 3))
	{
		wrenGetMapValue(vm, 2, 3, 3);
		nodeData.choices = GetChoiceList(vm, 3);
	}
	
	// Add to data map
	data->dataMap->nodes[nodeId] = nodeData;
}

// Register DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "DialogueData", wren_DialogueDataAllocate, wren_DialogueDataFinalize, "Container for dialogue data");

// Register constructors and methods
WREN_CONSTRUCTOR("dialogue", "DialogueData", "new()", wren_DialogueDataAllocate, "Create new DialogueData container");
WREN_CLASS_METHOD("dialogue", "DialogueData", "addNode(_,_)", wren_DialogueDataAddNode, "Add a dialogue node");

// ============================================================================
// DIALOGUE MANAGER BINDINGS
// ============================================================================

// DialogueManager.loadDialogue(dialogueData)
void wren_DialogueManagerLoadDialogue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	
	WrenDialogueData* data = (WrenDialogueData*)wrenGetSlotForeign(vm, 1);
	
	// Load into dialogue manager
	dialogueManager.LoadDialogueFromMap(*data->dataMap);
	data->loadedIntoManager = true;
	
	DEBUG_INFO("Loaded dialogue with %zu nodes from Wren", data->dataMap->nodes.size());
}

// DialogueManager.startDialogue(nodeId)
void wren_DialogueManagerStartDialogue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	
	const char* nodeId = wrenGetSlotString(vm, 1);
	auto result = dialogueManager.StartDialogue(nodeId);
	
	// Return result as map
	wrenSetSlotNewMap(vm, 0);
	
	// Add status
	wrenSetSlotString(vm, 1, "status");
	wrenSetSlotDouble(vm, 2, static_cast<double>(result.status));
	wrenSetMapValue(vm, 0, 1, 2);
	
	// Add nodeId
	wrenSetSlotString(vm, 1, "nodeId");
	wrenSetSlotString(vm, 2, result.nodeId.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	// Add speaker
	wrenSetSlotString(vm, 1, "speaker");
	wrenSetSlotString(vm, 2, result.speaker.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	// Add text
	wrenSetSlotString(vm, 1, "text");
	wrenSetSlotString(vm, 2, result.text.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	// Add hasEnded
	wrenSetSlotString(vm, 1, "hasEnded");
	wrenSetSlotBool(vm, 2, result.hasEnded);
	wrenSetMapValue(vm, 0, 1, 2);
	
	// Add choices as list
	wrenSetSlotString(vm, 1, "choices");
	wrenSetSlotNewList(vm, 2);
	for (const auto& choice : result.choices)
	{
		wrenSetSlotNewMap(vm, 3);
		
		wrenSetSlotString(vm, 4, "index");
		wrenSetSlotDouble(vm, 5, choice.index);
		wrenSetMapValue(vm, 3, 4, 5);
		
		wrenSetSlotString(vm, 4, "text");
		wrenSetSlotString(vm, 5, choice.text.c_str());
		wrenSetMapValue(vm, 3, 4, 5);
		
		wrenInsertInList(vm, 2, -1, 3);
	}
	wrenSetMapValue(vm, 0, 1, 2);
}

// DialogueManager.makeChoice(choiceIndex)
void wren_DialogueManagerMakeChoice(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	
	int choiceIndex = static_cast<int>(wrenGetSlotDouble(vm, 1));
	auto result = dialogueManager.MakeChoice(choiceIndex);
	
	// Return result as map (same format as startDialogue)
	wrenSetSlotNewMap(vm, 0);
	
	wrenSetSlotString(vm, 1, "status");
	wrenSetSlotDouble(vm, 2, static_cast<double>(result.status));
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "nodeId");
	wrenSetSlotString(vm, 2, result.nodeId.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "speaker");
	wrenSetSlotString(vm, 2, result.speaker.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "text");
	wrenSetSlotString(vm, 2, result.text.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "hasEnded");
	wrenSetSlotBool(vm, 2, result.hasEnded);
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "choices");
	wrenSetSlotNewList(vm, 2);
	for (const auto& choice : result.choices)
	{
		wrenSetSlotNewMap(vm, 3);
		
		wrenSetSlotString(vm, 4, "index");
		wrenSetSlotDouble(vm, 5, choice.index);
		wrenSetMapValue(vm, 3, 4, 5);
		
		wrenSetSlotString(vm, 4, "text");
		wrenSetSlotString(vm, 5, choice.text.c_str());
		wrenSetMapValue(vm, 3, 4, 5);
		
		wrenInsertInList(vm, 2, -1, 3);
	}
	wrenSetMapValue(vm, 0, 1, 2);
}

// DialogueManager.continueDialogue()
void wren_DialogueManagerContinueDialogue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	
	auto result = dialogueManager.ContinueDialogue();
	
	// Return result as map (same format as startDialogue)
	wrenSetSlotNewMap(vm, 0);
	
	wrenSetSlotString(vm, 1, "status");
	wrenSetSlotDouble(vm, 2, static_cast<double>(result.status));
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "nodeId");
	wrenSetSlotString(vm, 2, result.nodeId.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "speaker");
	wrenSetSlotString(vm, 2, result.speaker.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "text");
	wrenSetSlotString(vm, 2, result.text.c_str());
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "hasEnded");
	wrenSetSlotBool(vm, 2, result.hasEnded);
	wrenSetMapValue(vm, 0, 1, 2);
	
	wrenSetSlotString(vm, 1, "choices");
	wrenSetSlotNewList(vm, 2);
	for (const auto& choice : result.choices)
	{
		wrenSetSlotNewMap(vm, 3);
		
		wrenSetSlotString(vm, 4, "index");
		wrenSetSlotDouble(vm, 5, choice.index);
		wrenSetMapValue(vm, 3, 4, 5);
		
		wrenSetSlotString(vm, 4, "text");
		wrenSetSlotString(vm, 5, choice.text.c_str());
		wrenSetMapValue(vm, 3, 4, 5);
		
		wrenInsertInList(vm, 2, -1, 3);
	}
	wrenSetMapValue(vm, 0, 1, 2);
}

// DialogueManager.endDialogue()
void wren_DialogueManagerEndDialogue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	dialogueManager.EndDialogue();
}

// DialogueManager.isActive()
void wren_DialogueManagerIsActive(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	bool isActive = dialogueManager.IsDialogueActive();
	wrenSetSlotBool(vm, 0, isActive);
}

// DialogueManager.getCurrentNodeId()
void wren_DialogueManagerGetCurrentNodeId(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& dialogueManager = context->GetDialogueManager();
	std::string nodeId = dialogueManager.GetCurrentNodeId();
	wrenSetSlotString(vm, 0, nodeId.c_str());
}

// Register DialogueManager static methods
WREN_CLASS_STATIC("dialogue", "DialogueManager", "loadDialogue(_)", wren_DialogueManagerLoadDialogue, "Load dialogue data into the manager");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "startDialogue(_)", wren_DialogueManagerStartDialogue, "Start dialogue at a specific node");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "makeChoice(_)", wren_DialogueManagerMakeChoice, "Make a dialogue choice");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "continueDialogue()", wren_DialogueManagerContinueDialogue, "Continue to next dialogue node");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "endDialogue()", wren_DialogueManagerEndDialogue, "End the current dialogue");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "isActive()", wren_DialogueManagerIsActive, "Check if dialogue is currently active");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getCurrentNodeId()", wren_DialogueManagerGetCurrentNodeId, "Get current dialogue node ID");
