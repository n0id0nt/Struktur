#include "WrenDialogue.h"

#include "wren.h"

#include "Debug/Assertions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/DialogueLoader.h"
#include "Engine/Dialogue/DialogueNode.h"

static void AddDialogueValueToWren(WrenVM* vm, int slot, const Struktur::Dialogue::DialogueValue& dialogueValue)
{
	BREAK_MSG("Not Implemented");
}

static void ConvertParamsMapToWrenMap(WrenVM* vm, int slot, const std::map<std::string, Struktur::Dialogue::DialogueValue>& params)
{
	BREAK_MSG("Not Implemented");
}

// ============================================================================
// DIALOGUE STATUS BINDINGS
// ============================================================================

WREN_ENUM("dialogue", DialogueStatus, "Enum for the status codes for dialogue operations",
	WREN_ENUM_PAIR("SUCCESS", Struktur::Dialogue::DialogueStatus::SUCCESS),
	WREN_ENUM_PAIR("NO_ACTIVE_NODE", Struktur::Dialogue::DialogueStatus::NO_ACTIVE_NODE),
	WREN_ENUM_PAIR("INVALID_CHOICE", Struktur::Dialogue::DialogueStatus::INVALID_CHOICE),
	WREN_ENUM_PAIR("NODE_NOT_FOUND", Struktur::Dialogue::DialogueStatus::NODE_NOT_FOUND),
	WREN_ENUM_PAIR("ERROR", Struktur::Dialogue::DialogueStatus::ERROR),
	);

// ============================================================================
// DIALOGUE DATA BINDINGS
// ============================================================================

void wren_DialogueDataAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenDialogueData));
}

void wren_DialogueDataFinalize(void* data)
{
	WrenDialogueData* dialogueData = static_cast<WrenDialogueData*>(data);
	dialogueData->~WrenDialogueData();
}

// DialogueData.new()
void wren_DialogueDataNew(WrenVM* vm)
{
	WrenDialogueData* data = static_cast<WrenDialogueData*>(wrenGetSlotForeign(vm, 0));
	new (data) WrenDialogueData();
}

// DialogueData.addNodeSimple(nodeId, speaker, text, next)
// Simplified version for nodes with just basic fields
void wren_DialogueDataAddNodeSimple(WrenVM* vm)
{
	WrenDialogueData* data = static_cast<WrenDialogueData*>(wrenGetSlotForeign(vm, 0));
	const char* nodeId = wrenGetSlotString(vm, 1);

	std::map<std::string, Struktur::Dialogue::DialogueValue> nodeData;

	// Parse optional fields
	if (wrenGetSlotType(vm, 2) != WREN_TYPE_NULL)
	{
		nodeData["speaker"] = Struktur::Dialogue::DialogueValue(wrenGetSlotString(vm, 2));
	}

	if (wrenGetSlotType(vm, 3) != WREN_TYPE_NULL)
	{
		nodeData["text"] = Struktur::Dialogue::DialogueValue(wrenGetSlotString(vm, 3));
	}

	if (wrenGetSlotType(vm, 4) != WREN_TYPE_NULL)
	{
		nodeData["next"] = Struktur::Dialogue::DialogueValue(wrenGetSlotString(vm, 4));
	}

	data->dataMap[nodeId] = nodeData;
}

// DialogueData.loadIntoManager()
void wren_DialogueDataLoadIntoManager(WrenVM* vm)
{
	WrenDialogueData* data = static_cast<WrenDialogueData*>(wrenGetSlotForeign(vm, 0));
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	// Load nodes from data map
	auto nodes = Struktur::Dialogue::DialogueLoader::LoadFromData(registry, vm, data->dataMap);
	manager.LoadDialogueNodes(std::move(nodes));

	data->loadedIntoManager = true;

	DEBUG_INFO("Loaded dialogue data into manager");
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "DialogueData", wren_DialogueDataAllocate, wren_DialogueDataFinalize, "Container for dialogue data");

WREN_CONSTRUCTOR("dialogue", "DialogueData", "new()", wren_DialogueDataNew, "Constructor to create DialogueData");

WREN_CLASS_METHOD("dialogue", "DialogueData", "addNodeSimple(_,_,_,_)", wren_DialogueDataAddNodeSimple, "Add a simple node with id, speaker, text, next");
WREN_CLASS_METHOD("dialogue", "DialogueData", "loadIntoManager()", wren_DialogueDataLoadIntoManager, "Load dialogue data into DialogueManager");

// ============================================================================
// DIALOGUE RESULT BINDINGS
// ============================================================================

void wren_DialogueResultAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenDialogueResult));
}

void wren_DialogueResultFinalize(void* data)
{
	WrenDialogueResult* dialogueResult = static_cast<WrenDialogueResult*>(data);
	dialogueResult->~WrenDialogueResult();
}

// DialogueResult.new()
void wren_DialogueResultNew(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	new (result) WrenDialogueResult();
}

// DialogueResult.status
void wren_DialogueResultGetStatus(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, static_cast<double>(result->dataResult.status));
}

// DialogueResult.nodeId
void wren_DialogueResultGetNodeId(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, result->dataResult.nodeId.c_str());
}

// DialogueResult.speaker
void wren_DialogueResultGetSpeaker(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	if (result->dataResult.speaker.has_value())
	{
		wrenSetSlotString(vm, 0, result->dataResult.speaker.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// DialogueResult.text
void wren_DialogueResultGetText(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	if (result->dataResult.text.has_value())
	{
		wrenSetSlotString(vm, 0, result->dataResult.text.value().c_str());
	}
	else
	{
		
	}
}

// DialogueResult.choices
void wren_DialogueResultGetChoices(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	int choicesCount = result->dataResult.choices.size();
	if (choicesCount == 0)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i > choicesCount; i++)
	{

		wrenSetSlotString(vm, 1, result->dataResult.choices[i].text.c_str());
		wrenSetListElement(vm, 0, i, 1);
	}
}

// DialogueResult.hasEnded
void wren_DialogueResultGetHasEnded(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, result->dataResult.hasEnded);
}

// DialogueResult.shouldAutoAdvance
void wren_DialogueResultGetShouldAutoAdvance(WrenVM* vm)
{
	WrenDialogueResult* result = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, result->dataResult.shouldAutoAdvance);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "DialogueData", wren_DialogueDataAllocate, wren_DialogueDataFinalize, "Container for dialogue data");

WREN_CONSTRUCTOR("dialogue", "DialogueData", "new()", wren_DialogueDataNew, "Constructor to create DialogueData");

WREN_CLASS_METHOD("dialogue", "DialogueData", "status", wren_DialogueResultGetStatus, "Get the status of the dialogue result");
WREN_CLASS_METHOD("dialogue", "DialogueData", "nodeId", wren_DialogueResultGetNodeId, "Get the status of the node id");
WREN_CLASS_METHOD("dialogue", "DialogueData", "speaker", wren_DialogueResultGetSpeaker, "Get the status speaker of the node");
WREN_CLASS_METHOD("dialogue", "DialogueData", "text", wren_DialogueResultGetText, "Get the status text of the node");
WREN_CLASS_METHOD("dialogue", "DialogueData", "choices", wren_DialogueResultGetChoices, "Get the text choices");
WREN_CLASS_METHOD("dialogue", "DialogueData", "hasEnded", wren_DialogueResultGetHasEnded, "Get the text has ended");
WREN_CLASS_METHOD("dialogue", "DialogueData", "shouldAutoAdvance", wren_DialogueResultGetShouldAutoAdvance, "Get the text should auto advance");

// ============================================================================
// DIALOGUE REGISTRY BINDINGS
// ============================================================================

// DialogueRegistry.registerCondition(type) { |params| ... }
void wren_DialogueRegistryRegisterCondition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* type = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	Struktur::Dialogue::ConditionCallback conditionCallback = [vm, callback](const std::map<std::string, Struktur::Dialogue::DialogueValue>& params) -> bool {
		wrenEnsureSlots(vm, 4);
		wrenSetSlotHandle(vm, 0, callback);
		
		ConvertParamsMapToWrenMap(vm, 1, params);
		
		WrenHandle* method = wrenMakeCallHandle(vm, "call(_)");
		wrenCall(vm, method);
		wrenReleaseHandle(vm, method);
	};
	
	Struktur::Dialogue::DisposeCallback disposeCallback = [vm, callback](const Struktur::GameContext& params) {
		wrenReleaseHandle(vm, callback);
	};
	
	registry.RegisterConditionType(*context, type, std::move(conditionCallback), std::move(disposeCallback));
}

// DialogueRegistry.registerCommand(type) { |params| ... }
void wren_DialogueRegistryRegisterCommand(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* type = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	Struktur::Dialogue::CommandCallback conditionCallback = [vm, callback](const std::map<std::string, Struktur::Dialogue::DialogueValue>& params) {
		wrenEnsureSlots(vm, 4);
		wrenSetSlotHandle(vm, 0, callback);
		
		ConvertParamsMapToWrenMap(vm, 1, params);
		
		WrenHandle* method = wrenMakeCallHandle(vm, "call(_)");
		wrenCall(vm, method);
		wrenReleaseHandle(vm, method);
	};
	
	Struktur::Dialogue::DisposeCallback disposeCallback = [vm, callback](const Struktur::GameContext& params) {
		wrenReleaseHandle(vm, callback);
	};
	
	registry.RegisterCommandType(*context, type, std::move(conditionCallback), std::move(disposeCallback));
}

// DialogueRegistry.registerOperator(op) { |lhs, rhs| ... }
void wren_DialogueRegistryRegisterOperator(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* op = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	Struktur::Dialogue::OperatorCallback conditionCallback = [vm, callback](const Struktur::Dialogue::DialogueValue& lhs, const Struktur::Dialogue::DialogueValue& rhs) -> bool {
		wrenEnsureSlots(vm, 4);
		wrenSetSlotHandle(vm, 0, callback);
		
		AddDialogueValueToWren(vm, 1, lhs);
		AddDialogueValueToWren(vm, 2, rhs);
		
		WrenHandle* method = wrenMakeCallHandle(vm, "call(_,_)");
		wrenCall(vm, method);
		wrenReleaseHandle(vm, method);
	};
	
	Struktur::Dialogue::DisposeCallback disposeCallback = [vm, callback](const Struktur::GameContext& params) {
		wrenReleaseHandle(vm, callback);
	};
	
	registry.RegisterOperator(*context, op, std::move(conditionCallback), std::move(disposeCallback));
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueRegistry static methods
WREN_CLASS_STATIC("dialogue", "DialogueRegistry", "registerCondition(_,_)", wren_DialogueRegistryRegisterCondition, "Register a condition type with callback");
WREN_CLASS_STATIC("dialogue", "DialogueRegistry", "registerCommand(_,_)", wren_DialogueRegistryRegisterCommand, "Register a command type with callback");
WREN_CLASS_STATIC("dialogue", "DialogueRegistry", "registerOperator(_,_)", wren_DialogueRegistryRegisterOperator, "Register an operator with callback");

// ============================================================================
// DIALOGUE MANAGER BINDINGS
// ============================================================================

// DialogueManager.startDialogue(nodeId) -> Map
// Returns: { "status": num, "nodeId": string, "speaker": string, "text": string, 
//            "choices": list, "hasEnded": bool, "shouldAutoAdvance": bool }
void wren_DialogueManagerStartDialogue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	const char* nodeId = wrenGetSlotString(vm, 1);

	auto result = manager.StartDialogue(*context, nodeId);

	WrenDialogueResult* dialigueResult = static_cast<WrenDialogueResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueResult)));
	new (dialigueResult) WrenDialogueResult {result};
}

// DialogueManager.makeChoice(choiceIndex) -> Map
void wren_DialogueManagerMakeChoice(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	int choiceIndex = static_cast<int>(wrenGetSlotDouble(vm, 1));

	auto result = manager.MakeChoice(*context, choiceIndex);

	WrenDialogueResult* dialigueResult = static_cast<WrenDialogueResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueResult)));
	new (dialigueResult) WrenDialogueResult {result};
}

// DialogueManager.continue() -> Map
void wren_DialogueManagerContinue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	auto result = manager.Continue(*context);

	WrenDialogueResult* dialigueResult = static_cast<WrenDialogueResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueResult)));
	new (dialigueResult) WrenDialogueResult {result};
}

// DialogueManager.isDialogueActive() -> Bool
void wren_DialogueManagerIsDialogueActive(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	bool isActive = manager.IsDialogueActive();
	wrenSetSlotBool(vm, 0, isActive);
}

// DialogueManager.getCurrentNodeId() -> String or null
void wren_DialogueManagerGetCurrentNodeId(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	auto nodeId = manager.GetCurrentNodeId();
	if (nodeId.has_value())
	{
		wrenSetSlotString(vm, 0, nodeId.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// DialogueManager.getNodeCount() -> Num
void wren_DialogueManagerGetNodeCount(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	size_t count = manager.GetNodeCount();
	wrenSetSlotDouble(vm, 0, static_cast<double>(count));
}

// DialogueManager.clearAllNodes()
void wren_DialogueManagerClearAllNodes(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	manager.Clear();
}

// DialogueManager.loadDialogueData()
void wren_DialogueManagerLoadDialogueData(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	manager.Clear();
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueManager static methods
WREN_CLASS_STATIC("dialogue", "DialogueManager", "startDialogue(_)", wren_DialogueManagerStartDialogue, "Start dialogue at a specific node");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "makeChoice(_)", wren_DialogueManagerMakeChoice, "Make a choice in dialogue");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "continue()", wren_DialogueManagerContinue, "Continue to next node");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "isDialogueActive()", wren_DialogueManagerIsDialogueActive, "Check if dialogue is active");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getCurrentNodeId()", wren_DialogueManagerGetCurrentNodeId, "Get current node ID");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getNodeCount()", wren_DialogueManagerGetNodeCount, "Get total number of loaded nodes");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "clearAllNodes()", wren_DialogueManagerClearAllNodes, "Clear all loaded dialogue nodes");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "loadDialogueData(_)", wren_DialogueManagerLoadDialogueData, "Loads in and interprets a wren map as dialogue");
