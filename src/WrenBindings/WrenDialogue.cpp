#include "WrenDialogue.h"

#include "wren.h"

#include "Debug/Assertions.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/Dialogue/DialogueLoader.h"
#include "Engine/Dialogue/DialogueNode.h"

// ============================================================================
// FOREIGN CLASS WRAPPERS
// ============================================================================

WrenDialogueData::WrenDialogueData()
	: dataMap(new Struktur::Dialogue::DialogueDataMap())
	, loadedIntoManager(false)
{
}

WrenDialogueData::~WrenDialogueData()
{
	if (!loadedIntoManager)
	{
		DEBUG_WARNING("WrenDialogueData destroyed without being loaded into manager");
	}
	delete dataMap;
}

// ============================================================================
// DIALOGUE DATA BINDINGS
// ============================================================================

// DialogueData.new()
void wren_DialogueDataAllocate(WrenVM* vm)
{
	WrenDialogueData* data = static_cast<WrenDialogueData*>(wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenDialogueData)));
	new (data) WrenDialogueData();
}

void wren_DialogueDataFinalize(void* data)
{
	WrenDialogueData* dialogueData = static_cast<WrenDialogueData*>(data);
	dialogueData->~WrenDialogueData();
}

// Helper to parse a DialogueValue from a Wren slot
static Struktur::Dialogue::DialogueValue ParseDialogueValueFromSlot(WrenVM* vm, int slot)
{
	WrenType type = wrenGetSlotType(vm, slot);
	switch (type)
	{
		case WREN_TYPE_STRING:
			return Struktur::Dialogue::DialogueValue(wrenGetSlotString(vm, slot));
		case WREN_TYPE_NUM:
		{
			double value = wrenGetSlotDouble(vm, slot);
			// Check if it's an integer
			if (value == static_cast<int>(value))
			{
				return Struktur::Dialogue::DialogueValue(static_cast<int>(value));
			}
			return Struktur::Dialogue::DialogueValue(value);
		}
		case WREN_TYPE_BOOL:
			return Struktur::Dialogue::DialogueValue(wrenGetSlotBool(vm, slot));
		default:
			DEBUG_WARNING("Unsupported DialogueValue type");
			return Struktur::Dialogue::DialogueValue("");
	}
}

// Helper to parse a map of string -> DialogueValue
static std::map<std::string, Struktur::Dialogue::DialogueValue> ParseMapFromSlot(WrenVM* vm, int mapSlot)
{
	std::map<std::string, Struktur::Dialogue::DialogueValue> result;

	// Get map count
	int count = wrenGetMapCount(vm, mapSlot);
	if (count == 0)
	{
		return result;
	}

	// Unfortunately Wren doesn't have a direct iterator for maps
	// We need to use a different approach - store as a list in the wrapper
	// For now, return empty map
	DEBUG_WARNING("Map iteration not fully implemented - use addNodeSimple instead");
	return result;
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

	(*data->dataMap)[nodeId] = nodeData;
}

// Helper to parse commands/conditions/targets from Wren lists
static void ParseAndAddChoices(WrenVM* vm, int slot, Struktur::Dialogue::DialogueNode* node)
{
	int count = wrenGetListCount(vm, slot);
	for (int i = 0; i < count; ++i)
	{
		wrenGetListElement(vm, slot, i, slot + 1);

		// Get "text" key
		wrenSetSlotString(vm, slot + 2, "text");
		wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
		const char* text = wrenGetSlotString(vm, slot + 3);

		// Get "target" key
		wrenSetSlotString(vm, slot + 2, "target");
		wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
		const char* target = wrenGetSlotString(vm, slot + 3);

		node->AddChoice(Struktur::Dialogue::Choice(text, target));
	}
}

static void ParseAndAddCommands(WrenVM* vm, int slot, 
								Struktur::Dialogue::DialogueRegistry& registry,
								Struktur::Dialogue::DialogueNode* node)
{
	int count = wrenGetListCount(vm, slot);
	for (int i = 0; i < count; ++i)
	{
		wrenGetListElement(vm, slot, i, slot + 1);

		// Get "type"
		wrenSetSlotString(vm, slot + 2, "type");
		wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
		const char* type = wrenGetSlotString(vm, slot + 3);

		// Get "parameters" map
		wrenSetSlotString(vm, slot + 2, "parameters");
		wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);

		// Parse parameters (this is simplified - full implementation needed)
		std::map<std::string, Struktur::Dialogue::DialogueValue> params;
		
		// For now, manually get known parameters
		// This would need to be more generic in production
		
		auto command = registry.CreateCommand(type, params);
		if (command)
		{
			node->AddCommand(std::move(command));
		}
	}
}

static void ParseAndAddTargets(WrenVM* vm, int slot,
							   Struktur::Dialogue::DialogueRegistry& registry,
							   Struktur::Dialogue::DialogueNode* node)
{
	int count = wrenGetListCount(vm, slot);
	for (int i = 0; i < count; ++i)
	{
		wrenGetListElement(vm, slot, i, slot + 1);

		Struktur::Dialogue::ConditionalTarget target;

		// Get "node"
		wrenSetSlotString(vm, slot + 2, "node");
		wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
		target.targetNode = wrenGetSlotString(vm, slot + 3);

		// Get "conditions" (optional)
		wrenSetSlotString(vm, slot + 2, "conditions");
		wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
		if (wrenGetSlotType(vm, slot + 3) != WREN_TYPE_NULL)
		{
			// Parse conditions list
			int condCount = wrenGetListCount(vm, slot + 3);
			for (int j = 0; j < condCount; ++j)
			{
				wrenGetListElement(vm, slot + 3, j, slot + 4);

				// Get "type"
				wrenSetSlotString(vm, slot + 5, "type");
				wrenGetMapValue(vm, slot + 4, slot + 5, slot + 6);
				const char* type = wrenGetSlotString(vm, slot + 6);

				// Get "parameters"
				wrenSetSlotString(vm, slot + 5, "parameters");
				wrenGetMapValue(vm, slot + 4, slot + 5, slot + 6);

				std::map<std::string, Struktur::Dialogue::DialogueValue> params;
				// Parse parameters (simplified)

				auto condition = registry.CreateCondition(type, params);
				if (condition)
				{
					target.conditions.push_back(std::move(condition));
				}
			}
		}

		node->AddTarget(std::move(target));
	}
}

// DialogueData.loadIntoManager()
void wren_DialogueDataLoadIntoManager(WrenVM* vm)
{
	WrenDialogueData* data = static_cast<WrenDialogueData*>(wrenGetSlotForeign(vm, 0));
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	// Load nodes from data map
	auto nodes = Struktur::Dialogue::DialogueLoader::LoadFromData(registry, vm, *data->dataMap);
	manager.LoadDialogueNodes(std::move(nodes));

	data->loadedIntoManager = true;

	DEBUG_INFO("Loaded dialogue data into manager");
}

// ============================================================================
// DIALOGUE REGISTRY BINDINGS
// ============================================================================

// DialogueRegistry.registerCondition(type, callback)
void wren_DialogueRegistryRegisterCondition(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* type = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	registry.RegisterConditionType(type, callback);
}

// DialogueRegistry.registerCommand(type, callback)
void wren_DialogueRegistryRegisterCommand(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* type = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	registry.RegisterCommandType(type, callback);
}

// DialogueRegistry.registerOperator(op, callback)
void wren_DialogueRegistryRegisterOperator(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* op = wrenGetSlotString(vm, 1);
	WrenHandle* callback = wrenGetSlotHandle(vm, 2);

	registry.RegisterOperator(op, callback);
}

// DialogueRegistry.evalOperator(op, lhs, rhs) -> Bool
void wren_DialogueRegistryEvalOperator(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueRegistry& registry = context->GetDialogueRegistry();

	const char* op = wrenGetSlotString(vm, 1);
	Struktur::Dialogue::DialogueValue lhs = ParseDialogueValueFromSlot(vm, 2);
	Struktur::Dialogue::DialogueValue rhs = ParseDialogueValueFromSlot(vm, 3);

	bool result = registry.EvaluateOperator(op, lhs, rhs);
	wrenSetSlotBool(vm, 0, result);
}

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

	// Create result map
	wrenSetSlotNewMap(vm, 0);

	// Add status
	wrenSetSlotString(vm, 1, "status");
	wrenSetSlotDouble(vm, 2, static_cast<double>(result.status));
	wrenSetMapValue(vm, 0, 1, 2);

	// Add nodeId
	wrenSetSlotString(vm, 1, "nodeId");
	wrenSetSlotString(vm, 2, result.nodeId.c_str());
	wrenSetMapValue(vm, 0, 1, 2);

	// Add speaker (optional)
	wrenSetSlotString(vm, 1, "speaker");
	if (result.speaker.has_value())
	{
		wrenSetSlotString(vm, 2, result.speaker.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 2);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	// Add text (optional)
	wrenSetSlotString(vm, 1, "text");
	if (result.text.has_value())
	{
		wrenSetSlotString(vm, 2, result.text.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 2);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	// Add choices list
	wrenSetSlotString(vm, 1, "choices");
	wrenSetSlotNewList(vm, 2);
	for (const auto& choice : result.choices)
	{
		wrenSetSlotNewMap(vm, 3);

		wrenSetSlotString(vm, 4, "index");
		wrenSetSlotDouble(vm, 5, static_cast<double>(choice.index));
		wrenSetMapValue(vm, 3, 4, 5);

		wrenSetSlotString(vm, 4, "text");
		wrenSetSlotString(vm, 5, choice.text.c_str());
		wrenSetMapValue(vm, 3, 4, 5);

		wrenInsertInList(vm, 2, -1, 3);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	// Add hasEnded
	wrenSetSlotString(vm, 1, "hasEnded");
	wrenSetSlotBool(vm, 2, result.hasEnded);
	wrenSetMapValue(vm, 0, 1, 2);

	// Add shouldAutoAdvance
	wrenSetSlotString(vm, 1, "shouldAutoAdvance");
	wrenSetSlotBool(vm, 2, result.shouldAutoAdvance);
	wrenSetMapValue(vm, 0, 1, 2);
}

// DialogueManager.makeChoice(choiceIndex) -> Map
void wren_DialogueManagerMakeChoice(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	int choiceIndex = static_cast<int>(wrenGetSlotDouble(vm, 1));

	auto result = manager.MakeChoice(*context, choiceIndex);

	// Create result map (same as startDialogue)
	wrenSetSlotNewMap(vm, 0);

	wrenSetSlotString(vm, 1, "status");
	wrenSetSlotDouble(vm, 2, static_cast<double>(result.status));
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "nodeId");
	wrenSetSlotString(vm, 2, result.nodeId.c_str());
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "speaker");
	if (result.speaker.has_value())
	{
		wrenSetSlotString(vm, 2, result.speaker.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 2);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "text");
	if (result.text.has_value())
	{
		wrenSetSlotString(vm, 2, result.text.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 2);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "choices");
	wrenSetSlotNewList(vm, 2);
	for (const auto& choice : result.choices)
	{
		wrenSetSlotNewMap(vm, 3);

		wrenSetSlotString(vm, 4, "index");
		wrenSetSlotDouble(vm, 5, static_cast<double>(choice.index));
		wrenSetMapValue(vm, 3, 4, 5);

		wrenSetSlotString(vm, 4, "text");
		wrenSetSlotString(vm, 5, choice.text.c_str());
		wrenSetMapValue(vm, 3, 4, 5);

		wrenInsertInList(vm, 2, -1, 3);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "hasEnded");
	wrenSetSlotBool(vm, 2, result.hasEnded);
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "shouldAutoAdvance");
	wrenSetSlotBool(vm, 2, result.shouldAutoAdvance);
	wrenSetMapValue(vm, 0, 1, 2);
}

// DialogueManager.continue() -> Map
void wren_DialogueManagerContinue(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Dialogue::DialogueManager& manager = context->GetDialogueManager();

	auto result = manager.Continue(*context);

	// Create result map (same as startDialogue)
	wrenSetSlotNewMap(vm, 0);

	wrenSetSlotString(vm, 1, "status");
	wrenSetSlotDouble(vm, 2, static_cast<double>(result.status));
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "nodeId");
	wrenSetSlotString(vm, 2, result.nodeId.c_str());
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "speaker");
	if (result.speaker.has_value())
	{
		wrenSetSlotString(vm, 2, result.speaker.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 2);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "text");
	if (result.text.has_value())
	{
		wrenSetSlotString(vm, 2, result.text.value().c_str());
	}
	else
	{
		wrenSetSlotNull(vm, 2);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "choices");
	wrenSetSlotNewList(vm, 2);
	for (const auto& choice : result.choices)
	{
		wrenSetSlotNewMap(vm, 3);

		wrenSetSlotString(vm, 4, "index");
		wrenSetSlotDouble(vm, 5, static_cast<double>(choice.index));
		wrenSetMapValue(vm, 3, 4, 5);

		wrenSetSlotString(vm, 4, "text");
		wrenSetSlotString(vm, 5, choice.text.c_str());
		wrenSetMapValue(vm, 3, 4, 5);

		wrenInsertInList(vm, 2, -1, 3);
	}
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "hasEnded");
	wrenSetSlotBool(vm, 2, result.hasEnded);
	wrenSetMapValue(vm, 0, 1, 2);

	wrenSetSlotString(vm, 1, "shouldAutoAdvance");
	wrenSetSlotBool(vm, 2, result.shouldAutoAdvance);
	wrenSetMapValue(vm, 0, 1, 2);
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

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

// DialogueData foreign class
WREN_FOREIGN_CLASS("dialogue", "DialogueData", wren_DialogueDataAllocate, wren_DialogueDataFinalize, "Container for dialogue data");

WREN_CLASS_METHOD("dialogue", "DialogueData", "addNodeSimple(_,_,_,_)", wren_DialogueDataAddNodeSimple, "Add a simple node with id, speaker, text, next");

WREN_CLASS_METHOD("dialogue", "DialogueData",
					"loadIntoManager()",
					wren_DialogueDataLoadIntoManager,
					"Load dialogue data into DialogueManager");

// DialogueRegistry static methods
WREN_CLASS_STATIC("dialogue", "DialogueRegistry",
					"registerCondition(_,_)",
					wren_DialogueRegistryRegisterCondition,
					"Register a condition type with callback");

WREN_CLASS_STATIC("dialogue", "DialogueRegistry",
					"registerCommand(_,_)",
					wren_DialogueRegistryRegisterCommand,
					"Register a command type with callback");

WREN_CLASS_STATIC("dialogue", "DialogueRegistry",
					"registerOperator(_,_)",
					wren_DialogueRegistryRegisterOperator,
					"Register an operator with callback");

WREN_CLASS_STATIC("dialogue", "DialogueRegistry",
					"evalOperator(_,_,_)",
					wren_DialogueRegistryEvalOperator,
					"Evaluate operator with lhs and rhs");

// DialogueManager static methods
WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"startDialogue(_)",
					wren_DialogueManagerStartDialogue,
					"Start dialogue at a specific node");

WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"makeChoice(_)",
					wren_DialogueManagerMakeChoice,
					"Make a choice in dialogue");

WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"continue()",
					wren_DialogueManagerContinue,
					"Continue to next node");

WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"isDialogueActive()",
					wren_DialogueManagerIsDialogueActive,
					"Check if dialogue is active");

WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"getCurrentNodeId()",
					wren_DialogueManagerGetCurrentNodeId,
					"Get current node ID");

WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"getNodeCount()",
					wren_DialogueManagerGetNodeCount,
					"Get total number of loaded nodes");

WREN_CLASS_STATIC("dialogue", "DialogueManager",
					"clearAllNodes()",
					wren_DialogueManagerClearAllNodes,
					"Clear all loaded dialogue nodes");
