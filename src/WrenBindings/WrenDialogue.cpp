#include "WrenDialogue.h"

#include <memory>
#include "wren.hpp"

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
	// Set the value in slot+2 based on its type
	switch (dialogueValue.type)
	{
	case Struktur::Dialogue::DialogueValue::Type::STRING:
		wrenSetSlotString(vm, slot, dialogueValue.stringValue.c_str());
		break;

	case Struktur::Dialogue::DialogueValue::Type::INT:
		wrenSetSlotDouble(vm, slot, static_cast<double>(dialogueValue.intValue));
		break;

	case Struktur::Dialogue::DialogueValue::Type::BOOL:
		wrenSetSlotBool(vm, slot, dialogueValue.boolValue);
		break;

	case Struktur::Dialogue::DialogueValue::Type::DOUBLE:
		wrenSetSlotDouble(vm, slot, dialogueValue.doubleValue);
		break;
	default:
		wrenSetSlotNull(vm, slot);
		break;
	}
}

static void ConvertParamsMapToWrenMap(WrenVM* vm, int slot, const std::unordered_map<std::string, Struktur::Dialogue::DialogueValue>& params)
{
	// Create a new Wren map in the specified slot
	wrenEnsureSlots(vm, slot + 3); // Need extra slots for key, value, and map operations
	wrenSetSlotNewMap(vm, slot);

	// Iterate through the params map
	for (const auto& [key, value] : params)
	{
		// Set the key (always a string) in slot+1
		wrenSetSlotString(vm, slot + 1, key.c_str());

		AddDialogueValueToWren(vm, slot + 2, value);

		// Insert the key-value pair into the map
		wrenSetMapValue(vm, slot, slot + 1, slot + 2);
	}
}

static Struktur::Dialogue::DialogueValue DialogueParseWrenDialogueValue(WrenVM* vm, int slot)
{
	switch (wrenGetSlotType(vm, slot))
	{
	case WrenType::WREN_TYPE_BOOL:
		return Struktur::Dialogue::DialogueValue(wrenGetSlotBool(vm, slot));
	case WrenType::WREN_TYPE_NUM:
		return Struktur::Dialogue::DialogueValue(wrenGetSlotDouble(vm, slot));
	case WrenType::WREN_TYPE_STRING:
		return Struktur::Dialogue::DialogueValue(wrenGetSlotString(vm, slot));
	}
	return {};
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

	return { type, value };
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

static std::unique_ptr<Struktur::Dialogue::CallbackCommand> DialogueParseWrenCommand(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return std::make_unique<Struktur::Dialogue::CallbackCommand>(type, params);
}

static std::unique_ptr<Struktur::Dialogue::CallbackCondition> DialogueParseWrenCondition(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return std::make_unique<Struktur::Dialogue::CallbackCondition>(type, params);
}

static std::unique_ptr<Struktur::Dialogue::Choice> DialogueParseWrenChoice(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "text");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string text = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "target");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string target = wrenGetSlotString(vm, slot + 2);

	return std::make_unique<Struktur::Dialogue::Choice>(text, target);
}

static std::unique_ptr<Struktur::Dialogue::ConditionalTarget> DialogueParseWrenConditionalTarget(WrenVM* vm, int slot)
{
	auto conditionalTarget = std::make_unique<Struktur::Dialogue::ConditionalTarget>();

	wrenEnsureSlots(vm, slot + 4);
	wrenSetSlotString(vm, slot + 1, "node");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	conditionalTarget->targetNode = wrenGetSlotString(vm, slot + 2);


	wrenSetSlotString(vm, slot + 1, "conditions");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<Struktur::Dialogue::CallbackCondition> target = DialogueParseWrenCondition(vm, slot + 3);

			conditionalTarget->conditions.push_back(std::move(target));
		}
	}

	return conditionalTarget;
}

static std::unique_ptr<Struktur::Dialogue::DialogueNode> DialogueParseWrenNodeData(WrenVM* vm, int slot, std::string id)
{
	wrenEnsureSlots(vm, slot + 4);
	auto node = std::make_unique<Struktur::Dialogue::DialogueNode>(id);

	wrenSetSlotString(vm, slot + 1, "speaker");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string speaker = wrenGetSlotString(vm, slot + 2);
		node->SetSpeaker(speaker);
	}

	wrenSetSlotString(vm, slot + 1, "text");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string text = wrenGetSlotString(vm, slot + 2);
		node->SetText(text);
	}

	wrenSetSlotString(vm, slot + 1, "commands");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<Struktur::Dialogue::CallbackCommand> command = DialogueParseWrenCommand(vm, slot + 3);
			node->AddCommand(std::move(command));
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
			std::unique_ptr<Struktur::Dialogue::Choice> choice = DialogueParseWrenChoice(vm, slot + 3);
			node->AddChoice(std::move(choice));
		}
	}

	wrenSetSlotString(vm, slot + 1, "next");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		std::string next = wrenGetSlotString(vm, slot + 2);
		node->SetNext(next);
	}

	wrenSetSlotString(vm, slot + 1, "targets");
	if (wrenGetMapContainsKey(vm, slot, slot + 1))
	{
		wrenGetMapValue(vm, slot, slot + 1, slot + 2);
		int count = wrenGetListCount(vm, slot + 2);
		for (int i = 0; i < count; i++)
		{
			wrenGetListElement(vm, slot + 2, i, slot + 3);
			std::unique_ptr<Struktur::Dialogue::ConditionalTarget> target = DialogueParseWrenConditionalTarget(vm, slot + 3);
			node->AddTarget(std::move(target));
		}
	}

	return node;
}

static std::pair<std::string, std::unique_ptr<Struktur::Dialogue::DialogueNode>> DialogueParseWrenNodeKeyPair(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);

	wrenSetSlotString(vm, slot + 1, "node");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string nodeKey = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "data");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::unique_ptr<Struktur::Dialogue::DialogueNode> node = DialogueParseWrenNodeData(vm, slot + 2, nodeKey);

	return { nodeKey, std::move(node) };
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
		wrenSetSlotNull(vm, 0);
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
WREN_FOREIGN_CLASS("dialogue", "DialogueResult", wren_DialogueResultAllocate, wren_DialogueResultFinalize, "Container for dialogue Result data");

WREN_CLASS_METHOD("dialogue", "DialogueResult", "status", wren_DialogueResultGetStatus, "Get the status of the dialogue result");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "nodeId", wren_DialogueResultGetNodeId, "Get of the node id");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "speaker", wren_DialogueResultGetSpeaker, "Get the speaker of the node");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "text", wren_DialogueResultGetText, "Get the text of the node");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "choices", wren_DialogueResultGetChoices, "Get the text choices");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "hasEnded", wren_DialogueResultGetHasEnded, "Get the text has ended");
WREN_CLASS_METHOD("dialogue", "DialogueResult", "shouldAutoAdvance", wren_DialogueResultGetShouldAutoAdvance, "Get the text should auto advance");

// ============================================================================
// CONDITIONAL TARGET BINDINGS
// ============================================================================

void wren_ConditionalTargetAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenConditionalTarget));
}

void wren_ConditionalTargetFinalize(void* data)
{
	WrenConditionalTarget* conditionalTarget = static_cast<WrenConditionalTarget*>(data);
	conditionalTarget->~WrenConditionalTarget();
}

// ConditionalTarget.conditions
void wren_ConditionalTargetGetConditions(WrenVM* vm)
{
	WrenConditionalTarget* target = static_cast<WrenConditionalTarget*>(wrenGetSlotForeign(vm, 0));
	auto& conditions = target->target->conditions;

	int conditionsCount = conditions.size();
	if (conditionsCount == 0)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i > conditionsCount; i++)
	{
		// TODO make this a handle
		wrenSetSlotHandle(vm, 1, conditions[i]);
		wrenSetListElement(vm, 0, i, 1);
	}
}

// ConditionalTarget.targetNode
void wren_ConditionalTargetGetTargetNode(WrenVM* vm)
{
	WrenConditionalTarget* target = static_cast<WrenConditionalTarget*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, target->target->targetNode.c_str());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "ConditionalTarget", wren_ConditionalTargetAllocate, wren_ConditionalTargetFinalize, "Container for dialogue Result data");

WREN_CLASS_METHOD("dialogue", "ConditionalTarget", "conditions", wren_ConditionalTargetGetConditions, "Get the conditions of the Conditional Target");
WREN_CLASS_METHOD("dialogue", "ConditionalTarget", "targetNode", wren_ConditionalTargetGetTargetNode, "Get the target node of the Conditional Target");

// ============================================================================
// DIALOGUE RESULT BINDINGS
// ============================================================================

void wren_DialogueNodeAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenDialogueNode));
}

void wren_DialogueNodeFinalize(void* data)
{
	WrenDialogueNode* dialogueNode = static_cast<WrenDialogueNode*>(data);
	dialogueNode->~WrenDialogueNode();
}

// DialogueNode.id
void wren_DialogueNodeGetId(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, node->dataNode->GetId().c_str());
}

// DialogueNode.speaker
void wren_DialogueNodeGetSpeaker(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	if (node->dataNode->GetSpeaker().has_value())
	{
		wrenSetSlotString(vm, 0, node->dataNode->GetSpeaker().value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// DialogueNode.text
void wren_DialogueNodeGetText(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	if (node->dataNode->GetText().has_value())
	{
		wrenSetSlotString(vm, 0, node->dataNode->GetText().value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// DialogueNode.targets
void wren_DialogueNodeGetTargets(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	auto& targets = node->dataNode->GetTargets();

	int targetsCount = targets.size();
	if (targetsCount == 0)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i > targetsCount; i++)
	{
		wrenGetVariable(vm, "dialogue", "ConditionalTarget", 2);
		WrenConditionalTarget* target = static_cast<WrenConditionalTarget*>(wrenSetSlotNewForeign(vm, 1, 2, sizeof(WrenConditionalTarget)));
		new (target) WrenConditionalTarget{ targets[i].get() };

		wrenSetListElement(vm, 0, i, 1);
	}
}

// DialogueNode.commands
void wren_DialogueNodeGetCommands(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	auto& commands = node->dataNode->GetCommands();

	int commandsCount = commands.size();
	if (commandsCount == 0)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i > commandsCount; i++)
	{
		// TODO make this a handle
		wrenSetSlotHandle(vm, 1, commands[i]);
		wrenSetListElement(vm, 0, i, 1);
	}
}

// DialogueNode.choices
void wren_DialogueNodeGetChoices(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	auto& choices = node->dataNode->GetChoices();

	int choicesCount = choices.size();
	if (choicesCount == 0)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	for (int i = 0; i > choicesCount; i++)
	{
		wrenSetSlotString(vm, 1, choices[i]->text.c_str());
		wrenSetListElement(vm, 0, i, 1);
	}
}

// DialogueNode.next
void wren_DialogueNodeGetNext(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	if (node->dataNode->GetNext().has_value())
	{
		wrenSetSlotString(vm, 0, node->dataNode->GetNext().value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 0);
	}
}

// DialogueNode.hasChoices()
void wren_DialogueNodeHasChoices(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, node->dataNode->HasChoices());
}

// DialogueNode.hasNext()
void wren_DialogueNodeGetHasNext(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, node->dataNode->HasNext());
}

// DialogueNode.hasTargets()
void wren_DialogueNodeHasTargets(WrenVM* vm)
{
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, node->dataNode->HasTargets());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "DialogueNode", wren_DialogueNodeAllocate, wren_DialogueNodeFinalize, "Container for dialogue Node data");

WREN_CLASS_METHOD("dialogue", "DialogueNode", "id", wren_DialogueNodeGetId, "Get the node id");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "speaker", wren_DialogueNodeGetSpeaker, "Get the speaker of the node");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "text", wren_DialogueNodeGetText, "Get the text of the node");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "commands", wren_DialogueNodeGetCommands, "Get the commands of the node");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "choices", wren_DialogueNodeGetChoices, "Get the choices of the node");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "next", wren_DialogueNodeGetNext, "Get the next node");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "targets", wren_DialogueNodeGetTargets, "Get the conditional targets of the node");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "hasChoices()", wren_DialogueNodeHasChoices, "Check if node has choices");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "hasNext()", wren_DialogueNodeGetHasNext, "Check if node has next target");
WREN_CLASS_METHOD("dialogue", "DialogueNode", "hasTargets()", wren_DialogueNodeHasTargets, "Check if node has conditional targets");

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
		return wrenGetSlotBool(vm, 0);
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
		return wrenGetSlotBool(vm, 0);
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

	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "dialogue", "DialogueResult", 1);
	WrenDialogueResult* dialigueResult = static_cast<WrenDialogueResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueResult)));
	new (dialigueResult) WrenDialogueResult{ result };
}

// DialogueManager.makeChoice(choiceIndex) -> Map
void wren_DialogueManagerMakeChoice(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	int choiceIndex = static_cast<int>(wrenGetSlotDouble(vm, 1));

	auto result = manager.MakeChoice(*context, choiceIndex);

	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "dialogue", "DialogueResult", 1);
	WrenDialogueResult* dialigueResult = static_cast<WrenDialogueResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueResult)));
	new (dialigueResult) WrenDialogueResult{ result };
}

// DialogueManager.continue() -> Map
void wren_DialogueManagerContinue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	auto result = manager.Continue(*context);

	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "dialogue", "DialogueResult", 1);
	WrenDialogueResult* dialigueResult = static_cast<WrenDialogueResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueResult)));
	new (dialigueResult) WrenDialogueResult{ result };
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
		manager.AddNode(node.first, std::move(node.second));
	}
}

// DialogueManager.clearDialogue()
void wren_DialogueManagerClearDialogue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();
	manager.ClearDialogue(*context);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueManager static methods
WREN_CLASS_STATIC("dialogue", "DialogueManager", "startDialogue(_)", wren_DialogueManagerStartDialogue, "Start dialogue at a specific node");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "makeChoice(_)", wren_DialogueManagerMakeChoice, "Make a choice in dialogue");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "continueDialogue()", wren_DialogueManagerContinue, "Continue to next node");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "isDialogueActive()", wren_DialogueManagerIsDialogueActive, "Check if dialogue is active");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getCurrentNodeId()", wren_DialogueManagerGetCurrentNodeId, "Get current node ID");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getNodeCount()", wren_DialogueManagerGetNodeCount, "Get total number of loaded nodes");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "clearAllNodes()", wren_DialogueManagerClearAllNodes, "Clear all loaded dialogue nodes");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "loadDialogueData(_)", wren_DialogueManagerLoadDialogueData, "Loads in and interprets a wren map as dialogue");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "clearDialogue()", wren_DialogueManagerClearDialogue, "Clear the current dialogue interaction");
