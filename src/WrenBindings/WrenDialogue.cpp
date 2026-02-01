#include "WrenDialogue.h"

#include "wren.h"

#include "Debug/Assertions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/DialogueLoader.h"
#include "Engine/Dialogue/DialogueNode.h"
#include "Engine/Dialogue/CallbackCommand.h"
#include "Engine/Dialogue/CallbackCondition.h"

static void AddDialogueValueToWren(WrenVM* vm, int slot, const Struktur::Dialogue::DialogueValue& dialogueValue)
{
	BREAK_MSG("Not Implemented");
}

static void ConvertParamsMapToWrenMap(WrenVM* vm, int slot, const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& params)
{
	BREAK_MSG("Not Implemented");
}

static Struktur::Dialogue::DialogueValue DialogueParseWrenDialogueValue(WrenVM* vm, int slot)
{
	switch(wrenGetSlotType(vm, slot))
	{
	case WrenType::WREN_TYPE_BOOL:
		return Struktur::Dialogue::DialogueValue(wrenGetSlotBool(vm, slot));
	case WrenType::WREN_TYPE_NUM:
		return Struktur::Dialogue::DialogueValue(wrenGetSlotDouble(vm, slot));
	case WrenType::WREN_TYPE_STRING:
		return Struktur::Dialogue::DialogueValue(wrenGetSlotString(vm, slot));
	}
	return Struktur::Dialogue::DialogueValue();
}

static std::pair<std::string, Struktur::Dialogue::DialogueValue> DialogueParseWrenParameter(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "value");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	Struktur::Dialogue::DialogueValue value = DialogueParseWrenDialogueValue(vm, slot + 2);

	return {type, value};
}

static std::unordered_map<std::string, Struktur::Dialogue::DialogueValue> DialogueParseWrenParameters(WrenVM* vm, int slot)
{
	std::unordered_map<std::string, Struktur::Dialogue::DialogueValue> map;
	wrenEnsureSlots(vm, slot + 2);
	int count = wrenGetListCount(vm, slot);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, slot, i, slot + 1);
		auto dialoguePair = DialogueParseWrenParameter(vm, slot + 1);
		map[dialoguePair.first] = dialoguePair.second;
	}
	return map;
}

static Struktur::Dialogue::CallbackCommand DialogueParseWrenCommand(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return Struktur::Dialogue::CallbackCommand(type, params);
}

static Struktur::Dialogue::CallbackCondition DialogueParseWrenCondition(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return Struktur::Dialogue::CallbackCondition(type, params);
}

static Struktur::Dialogue::Choice DialogueParseWrenChoice(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "text");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string text = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "target");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string target = wrenGetSlotString(vm, slot + 2);

	return Struktur::Dialogue::Choice(text, target);
}

static Struktur::Dialogue::ConditionalTarget DialogueParseWrenConditionalTarget(WrenVM* vm, int slot)
{
	Struktur::Dialogue::ConditionalTarget conditionalTarget;

	wrenSetSlotString(vm, slot + 1, "node");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	conditionalTarget.targetNode = wrenGetSlotString(vm, slot + 2);

	int count = wrenGetListCount(vm, slot);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, slot, i, slot + 1);
		Struktur::Dialogue::CallbackCondition target = DialogueParseWrenCondition(vm, slot + 1);

		conditionalTarget.conditions.push_back(target);
	}

	return conditionalTarget;
}

static Struktur::Dialogue::DialogueNode DialogueParseWrenNodeData(WrenVM* vm, int slot, std::string id)
{
	wrenEnsureSlots(vm, slot + 4);
	Struktur::Dialogue::DialogueNode node(id);

	wrenSetSlotString(vm, slot + 1, "speaker");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string speaker = wrenGetSlotString(vm, slot + 2);
		node.SetSpeaker(speaker);
	}

	wrenSetSlotString(vm, slot + 1, "text");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::string text = wrenGetSlotString(vm, slot + 3);
			node.SetText(text);
		} 
	}

	wrenSetSlotString(vm, slot + 1, "commands");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			Struktur::Dialogue::CallbackCommand command = DialogueParseWrenCommand(vm, slot + 3);
			node.AddCommand(command);
		}
	}

	wrenSetSlotString(vm, slot + 1, "choices");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			Struktur::Dialogue::Choice choice = DialogueParseWrenChoice(vm, slot + 3);
			node.AddChoice(choice);
		}
	}

	wrenSetSlotString(vm, slot + 1, "next");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string next = wrenGetSlotString(vm, slot + 2);
		node.SetNext(next);
	}

	wrenSetSlotString(vm, slot + 1, "targets");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			Struktur::Dialogue::ConditionalTarget target = DialogueParseWrenConditionalTarget(vm, slot + 3);
			node.AddTarget(target);
		}
	}

	return node;
}

static std::pair<std::string, Struktur::Dialogue::DialogueNode> DialogueParseWrenNodeKeyPair(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);

	wrenSetSlotString(vm, slot + 1, "node");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string nodeKey = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "data");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	Struktur::Dialogue::DialogueNode node = DialogueParseWrenNodeData(vm, slot + 2, nodeKey);

	return {nodeKey, node};
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
WREN_FOREIGN_CLASS("dialogue", "DialogueResult", wren_DialogueResultAllocate, wren_DialogueResultFinalize, "Container for dialogue data");

WREN_CONSTRUCTOR("dialogue", "DialogueResult", "new()", wren_DialogueResultNew, "Constructor to create DialogueData");

WREN_CLASS_METHOD("dialogue", "DialogueResult", "status", wren_DialogueResultGetStatus, "Get the status of the dialogue result");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "nodeId", wren_DialogueResultGetNodeId, "Get the status of the node id");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "speaker", wren_DialogueResultGetSpeaker, "Get the status speaker of the node");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "text", wren_DialogueResultGetText, "Get the status text of the node");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "choices", wren_DialogueResultGetChoices, "Get the text choices");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "hasEnded", wren_DialogueResultGetHasEnded, "Get the text has ended");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "shouldAutoAdvance", wren_DialogueResultGetShouldAutoAdvance, "Get the text should auto advance");

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

	Struktur::Dialogue::ConditionCallback conditionCallback = [vm, callback](const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& params) -> bool {
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

	Struktur::Dialogue::CommandCallback conditionCallback = [vm, callback](const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& params) {
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

// DialogueManager.loadDialogueData(_)
void wren_DialogueManagerLoadDialogueData(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();
	wrenEnsureSlots(vm, 3);

	int count = wrenGetListCount(vm, 1);
	for (int i = 0; i < count; i++)
	{
		wrenGetListElement(vm, 1, i, 2);
		auto node = DialogueParseWrenNodeKeyPair(vm, 2);
		manager.AddNode(node.first, node.second);
	}
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
