#include "WrenDialogue.h"

#include <memory>
#include "wren.hpp"

#include "Debug/Assertions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/DialogueNode.h"
#include "Engine/Dialogue/Command.h"
#include "Engine/Dialogue/Condition.h"
#include "Engine/Callback/WrenCallback.h"
#include "Engine/Callback/CallbackHelperFunctions.h"
#include "WrenFunctionCallback.h"

static void AddDialogueValueToWren(WrenVM* vm, int slot, const Struktur::Dialogue::DialogueValue& dialogueValue)
{
	std::visit([vm, slot](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<T, std::string>)
			{
				wrenSetSlotString(vm, slot, arg.c_str());
			}
			else if constexpr (std::is_same_v<T, int>)
			{
				wrenSetSlotDouble(vm, slot, static_cast<double>(arg));
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				wrenSetSlotBool(vm, slot, arg);
			}
			else if constexpr (std::is_same_v<T, double>)
			{
				wrenSetSlotDouble(vm, slot, arg);
			}
			else if constexpr (std::is_same_v<T, std::shared_ptr<Struktur::Callback::VariantList>>)
			{
				if (!arg)
				{
					wrenSetSlotNull(vm, slot);
					return;
				}

				wrenEnsureSlots(vm, slot + 2);
				wrenSetSlotNewList(vm, slot);

				// Use slot + 1 as scratch space for each element
				int elementSlot = slot + 1;
				for (const auto& item : arg->items)
				{
					Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, elementSlot, item);
					wrenInsertInList(vm, slot, -1, elementSlot);
				}
			}
			else if constexpr (std::is_same_v<T, std::shared_ptr<Struktur::Callback::VariantMap>>)
			{
				if (!arg)
				{
					wrenSetSlotNull(vm, slot);
					return;
				}

				wrenEnsureSlots(vm, slot + 3);
				wrenSetSlotNewMap(vm, slot);

				// Use slot + 1 and slot + 2 as scratch space for key and value
				int keySlot = slot + 1;
				int valueSlot = slot + 2;
				for (const auto& [key, value] : arg->items)
				{
					wrenSetSlotString(vm, keySlot, key.c_str());
					Struktur::Callback::HelperFunctions::VariantToWrenSlot(vm, valueSlot, value);
					wrenSetMapValue(vm, slot, keySlot, valueSlot);
				}
			}
			else
			{
				wrenSetSlotNull(vm, slot);
			}
		}, dialogueValue.GetVariant());
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

static std::unique_ptr<Struktur::Dialogue::Command> DialogueParseWrenCommand(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return std::make_unique<Struktur::Dialogue::Command>(type, params);
}

static std::unique_ptr<Struktur::Dialogue::Condition> DialogueParseWrenCondition(WrenVM* vm, int slot)
{
	wrenEnsureSlots(vm, slot + 3);
	wrenSetSlotString(vm, slot + 1, "type");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	std::string type = wrenGetSlotString(vm, slot + 2);

	wrenSetSlotString(vm, slot + 1, "parameters");
	wrenGetMapValue(vm, slot, slot + 1, slot + 2);
	auto params = DialogueParseWrenParameters(vm, slot + 2);

	return std::make_unique<Struktur::Dialogue::Condition>(type, params);
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
			std::unique_ptr<Struktur::Dialogue::Condition> target = DialogueParseWrenCondition(vm, slot + 3);

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
			std::unique_ptr<Struktur::Dialogue::Command> command = DialogueParseWrenCommand(vm, slot + 3);
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
	for (int i = 0; i < choicesCount; i++)
	{
		wrenSetSlotString(vm, 1, result->dataResult.choices[i].text.c_str());
		wrenInsertInList(vm, 0, i, 1);
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

// DialogueResult.nodeNotFound(nodeId)
void wren_DialogueResultNodeNotFound(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	const char* nodeId = wrenGetSlotString(vm, 1);
	auto result = Struktur::Dialogue::DialogueResult::NodeNotFound(nodeId);
	new (wrenResult) WrenDialogueResult{ result };
}

// DialogueResult.endDialogue(node)
void wren_DialogueResultEndDialogue(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 1));
	auto result = Struktur::Dialogue::DialogueResult::Success(node->dataNode->GetId());
	result.speaker = node->dataNode->GetSpeaker();
	result.text = node->dataNode->GetText();
	result.hasEnded = true;
	new (wrenResult) WrenDialogueResult{ result };
}

// DialogueResult.choices(node)
void wren_DialogueResultChoices(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 1));
	auto result = Struktur::Dialogue::DialogueResult::Success(node->dataNode->GetId());
	result.speaker = node->dataNode->GetSpeaker();
	result.text = node->dataNode->GetText();
	const auto& choices = node->dataNode->GetChoices();
	for (size_t i = 0; i < choices.size(); ++i)
	{
		result.choices.emplace_back(static_cast<int>(i), choices[i]->text);
	}
	new (wrenResult) WrenDialogueResult{ result };
}

// DialogueResult.advance(node)
void wren_DialogueResultAdvance(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 1));
	auto result = Struktur::Dialogue::DialogueResult::Success(node->dataNode->GetId());
	result.speaker = node->dataNode->GetSpeaker();
	result.text = node->dataNode->GetText();
	result.shouldAutoAdvance = true;
	new (wrenResult) WrenDialogueResult{ result };
}

// DialogueResult.invalidChoice()
void wren_DialogueResultInvalidChoice(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	auto result = Struktur::Dialogue::DialogueResult::InvalidChoice();
	new (wrenResult) WrenDialogueResult{ result };
}

// DialogueResult.noActivaNode()
void wren_DialogueResultNoActiveNode(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	auto result = Struktur::Dialogue::DialogueResult::NoActiveNode();
	new (wrenResult) WrenDialogueResult{ result };
}

// DialogueResult.error(_)
void wren_DialogueResultError(WrenVM* vm)
{
	WrenDialogueResult* wrenResult = static_cast<WrenDialogueResult*>(wrenGetSlotForeign(vm, 0));
	const char* errorCode = wrenGetSlotString(vm, 1);
	auto result = Struktur::Dialogue::DialogueResult::Error(errorCode);
	new (wrenResult) WrenDialogueResult{ result };
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "DialogueResult", wren_DialogueResultAllocate, wren_DialogueResultFinalize, "Container for dialogue Result data");

WREN_CONSTRUCTOR("dialogue", "DialogueResult", "nodeNotFound(_)", wren_DialogueResultNodeNotFound, "Node not found result");
WREN_CONSTRUCTOR("dialogue", "DialogueResult", "endDialogue(_)", wren_DialogueResultEndDialogue, "End dialogue result");
WREN_CONSTRUCTOR("dialogue", "DialogueResult", "choices(_)", wren_DialogueResultChoices, "Choices result");
WREN_CONSTRUCTOR("dialogue", "DialogueResult", "advance(_)", wren_DialogueResultAdvance, "Advance result");
WREN_CONSTRUCTOR("dialogue", "DialogueResult", "invalidChoice()", wren_DialogueResultInvalidChoice, "Invalid choice result");
WREN_CONSTRUCTOR("dialogue", "DialogueResult", "noActiveNode()", wren_DialogueResultNoActiveNode, "No active node result");
WREN_CONSTRUCTOR("dialogue", "DialogueResult", "error()", wren_DialogueResultError, "Error result");

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

// ConditionalTarget.hasConditions()
void wren_ConditionalTargetHasConditions(WrenVM* vm)
{
	WrenConditionalTarget* target = static_cast<WrenConditionalTarget*>(wrenGetSlotForeign(vm, 0));
	auto& conditions = target->target->conditions;
	wrenSetSlotBool(vm, 0, !target->target->conditions.empty());
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

	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);
	wrenGetVariable(vm, "dialogue", "Condition", 2);
	for (int i = 0; i < conditionsCount; i++)
	{
		WrenCondition* wrenCondition = static_cast<WrenCondition*>(wrenSetSlotNewForeign(vm, 1, 2, sizeof(WrenCondition)));
		new (wrenCondition) WrenCondition(conditions[i].get());
		wrenInsertInList(vm, 0, i, 1);
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
WREN_FOREIGN_CLASS("dialogue", "ConditionalTarget", wren_ConditionalTargetAllocate, wren_ConditionalTargetFinalize, "Container for dialogue conditional target data");

WREN_CLASS_METHOD("dialogue", "ConditionalTarget", "hasConditions()", wren_ConditionalTargetHasConditions, "Does the Target have conditions?");
WREN_CLASS_METHOD("dialogue", "ConditionalTarget", "conditions", wren_ConditionalTargetGetConditions, "Get the conditions of the Conditional Target");
WREN_CLASS_METHOD("dialogue", "ConditionalTarget", "targetNode", wren_ConditionalTargetGetTargetNode, "Get the target node of the Conditional Target");

// ============================================================================
// CONDITION BINDINGS
// ============================================================================

void wren_ConditionAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenCondition));
}

void wren_ConditionFinalize(void* data)
{
	WrenCondition* condition = static_cast<WrenCondition*>(data);
	condition->~WrenCondition();
}

// Condition.callback -> callbackHandle
void wren_ConditionGetCallback(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenCondition* wrenCondition = static_cast<WrenCondition*>(wrenGetSlotForeign(vm, 0));
	auto* callback = wrenCondition->condition->GetCallback(*context);
	ASSERT_MSG(callback, "condition callback %s not found, likley not registered", wrenCondition->condition->GetKey().c_str());
	wrenSetSlotCallback(vm, 0, callback);
}

// Condition.params -> map
void wren_ConditionGetParams(WrenVM* vm)
{
	WrenCondition* wrenCondition = static_cast<WrenCondition*>(wrenGetSlotForeign(vm, 0));
	ConvertParamsMapToWrenMap(vm, 0, wrenCondition->condition->GetParams());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "Condition", wren_ConditionAllocate, wren_ConditionFinalize, "Container for dialogue condition data");

WREN_CLASS_METHOD("dialogue", "Condition", "callback", wren_ConditionGetCallback, "Get the callback handle");
WREN_CLASS_METHOD("dialogue", "Condition", "params", wren_ConditionGetParams, "Get the conditions params");

// ============================================================================
// COMMAND BINDINGS
// ============================================================================

void wren_CommandAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenCommand));
}

void wren_CommandFinalize(void* data)
{
	WrenCommand* command = static_cast<WrenCommand*>(data);
	command->~WrenCommand();
}

// Command.callback -> callbackHandle
void wren_CommandGetCallback(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	WrenCommand* wrenCommand = static_cast<WrenCommand*>(wrenGetSlotForeign(vm, 0));
	auto* callback = wrenCommand->command->GetCallback(*context);
	ASSERT_MSG(callback, "command callback %s not found, likley not registered", wrenCommand->command->GetKey().c_str());
	wrenSetSlotCallback(vm, 0, callback);
}

// Command.params -> map
void wren_CommandGetParams(WrenVM* vm)
{
	WrenCommand* wrenCommand = static_cast<WrenCommand*>(wrenGetSlotForeign(vm, 0));
	ConvertParamsMapToWrenMap(vm, 0, wrenCommand->command->GetParams());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "Command", wren_CommandAllocate, wren_CommandFinalize, "Container for dialogue command data");

WREN_CLASS_METHOD("dialogue", "Command", "callback", wren_CommandGetCallback, "Get the callback handle");
WREN_CLASS_METHOD("dialogue", "Command", "params", wren_CommandGetParams, "Get the conditions params");

// ============================================================================
// CHOICE BINDINGS
// ============================================================================

void wren_ChoiceAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenChoice));
}

void wren_ChoiceFinalize(void* data)
{
	WrenChoice* choice = static_cast<WrenChoice*>(data);
	choice->~WrenChoice();
}

// Choice.text -> string
void wren_ChoiceGetText(WrenVM* vm)
{
	WrenChoice* choice = static_cast<WrenChoice*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, choice->choice->text.c_str());
}

// Choice.targetNodeId -> string
void wren_ChoiceGetTargetNode(WrenVM* vm)
{
	WrenChoice* choice = static_cast<WrenChoice*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, choice->choice->targetNode.c_str());
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "Choice", wren_ChoiceAllocate, wren_ChoiceFinalize, "Container for dialogue chioce data");

WREN_CLASS_METHOD("dialogue", "Choice", "text", wren_ChoiceGetText, "Get the text");
WREN_CLASS_METHOD("dialogue", "Choice", "targetNodeId", wren_ChoiceGetTargetNode, "Get the target node id");

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
	for (int i = 0; i < targetsCount; i++)
	{
		wrenGetVariable(vm, "dialogue", "ConditionalTarget", 2);
		WrenConditionalTarget* target = static_cast<WrenConditionalTarget*>(wrenSetSlotNewForeign(vm, 1, 2, sizeof(WrenConditionalTarget)));
		new (target) WrenConditionalTarget{ targets[i].get() };

		wrenInsertInList(vm, 0, i, 1);
	}
}

// DialogueNode.commands
void wren_DialogueNodeGetCommands(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));

	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenGetSlotForeign(vm, 0));
	auto& commands = node->dataNode->GetCommands();

	int commandsCount = commands.size();
	if (commandsCount == 0)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);
	wrenGetVariable(vm, "dialogue", "Command", 2);
	for (int i = 0; i < commandsCount; i++)
	{
		WrenCommand* wrenCommand = static_cast<WrenCommand*>(wrenSetSlotNewForeign(vm, 1, 2, sizeof(WrenCommand)));
		new (wrenCommand) WrenCommand(commands[i].get());
		wrenInsertInList(vm, 0, i, 1);
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

	wrenEnsureSlots(vm, 3);
	wrenSetSlotNewList(vm, 0);
	wrenGetVariable(vm, "dialogue", "Choice", 2);
	for (int i = 0; i < choicesCount; i++)
	{
		WrenChoice* wrenChoive = static_cast<WrenChoice*>(wrenSetSlotNewForeign(vm, 1, 2, sizeof(WrenChoice)));
		new (wrenChoive) WrenChoice{ choices[i].get() };
		wrenInsertInList(vm, 0, i, 1);
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

	registry.RegisterConditionType(*context, type, std::make_unique<Struktur::Callback::WrenCallback>(callback));
}

// DialogueRegistry.registerCommand(type) { |params| ... }
void wren_DialogueRegistryRegisterCommand(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* type = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	registry.RegisterCommandType(*context, type, std::make_unique<Struktur::Callback::WrenCallback>(callback));
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueRegistry static methods
WREN_CLASS_STATIC("dialogue", "DialogueRegistry", "registerCondition(_,_)", wren_DialogueRegistryRegisterCondition, "Register a condition type with callback");
WREN_CLASS_STATIC("dialogue", "DialogueRegistry", "registerCommand(_,_)", wren_DialogueRegistryRegisterCommand, "Register a command type with callback");

// ============================================================================
// DIALOGUE MANAGER BINDINGS
// ============================================================================

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

// DialogueManager.currentNode
void wren_DialogueManagerGetCurrentNode(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();
	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "dialogue", "DialogueNode", 1);
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueNode)));
	new (node) WrenDialogueNode{ manager.GetCurrentNode() };
}

// DialogueManager.setActiveNode(_)
void wren_DialogueManagerSetActiveNode(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();
	std::string nodeId = wrenGetSlotString(vm, 1);
	Struktur::Dialogue::DialogueNode* dialogueNode = manager.SetActiveNode(*context, nodeId);
	if (!dialogueNode)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}
	wrenEnsureSlots(vm, 2);
	wrenGetVariable(vm, "dialogue", "DialogueNode", 1);
	WrenDialogueNode* node = static_cast<WrenDialogueNode*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenDialogueNode)));
	new (node) WrenDialogueNode{ dialogueNode };
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueManager static methods
WREN_CLASS_STATIC("dialogue", "DialogueManager", "isDialogueActive()", wren_DialogueManagerIsDialogueActive, "Check if dialogue is active");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getCurrentNodeId()", wren_DialogueManagerGetCurrentNodeId, "Get current node ID");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "getNodeCount()", wren_DialogueManagerGetNodeCount, "Get total number of loaded nodes");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "clearAllNodes()", wren_DialogueManagerClearAllNodes, "Clear all loaded dialogue nodes");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "loadDialogueData(_)", wren_DialogueManagerLoadDialogueData, "Loads in and interprets a wren map as dialogue");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "clearDialogue()", wren_DialogueManagerClearDialogue, "Clear the current dialogue interaction");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "currentNode", wren_DialogueManagerGetCurrentNode, "Get the current node in of the dialogue");
WREN_CLASS_STATIC("dialogue", "DialogueManager", "setActiveNode(_)", wren_DialogueManagerSetActiveNode, "Set the active node in the dialogue");
