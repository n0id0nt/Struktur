#include "DialogueLoader.h"

#include "wren.h"
#include "Debug/Assertions.h"
#include "Engine/GameContext.h"

namespace Struktur::Dialogue
{
	std::unordered_map<std::string, std::unique_ptr<DialogueNode>> DialogueLoader::LoadFromData(
		DialogueRegistry& registry,
		WrenVM* vm,
		const DialogueDataMap& dataMap)
	{
		std::unordered_map<std::string, std::unique_ptr<DialogueNode>> nodes;

		for (const auto& [nodeId, nodeData] : dataMap)
		{
			auto node = ParseNode(registry, vm, nodeId, nodeData);
			if (node)
			{
				nodes[nodeId] = std::move(node);
			}
		}

		DEBUG_INFO("Loaded %zu dialogue nodes", nodes.size());
		return nodes;
	}

	std::unique_ptr<DialogueNode> DialogueLoader::ParseNode(
		DialogueRegistry& registry,
		WrenVM* vm,
		const std::string& nodeId,
		const std::unordered_map<std::string, DialogueValue>& nodeData)
	{
		auto node = std::make_unique<DialogueNode>(nodeId);

		// Parse speaker (optional)
		auto speakerIt = nodeData.find("speaker");
		if (speakerIt != nodeData.end() && speakerIt->second.type == DialogueValue::Type::STRING)
		{
			node->SetSpeaker(speakerIt->second.stringValue);
		}

		// Parse text (optional)
		auto textIt = nodeData.find("text");
		if (textIt != nodeData.end() && textIt->second.type == DialogueValue::Type::STRING)
		{
			node->SetText(textIt->second.stringValue);
		}

		// Parse next (optional)
		auto nextIt = nodeData.find("next");
		if (nextIt != nodeData.end() && nextIt->second.type == DialogueValue::Type::STRING)
		{
			node->SetNext(nextIt->second.stringValue);
		}

		// Parse commands (optional)
		auto commandsIt = nodeData.find("commands");
		if (commandsIt != nodeData.end())
		{
			// Commands should be stored as a special marker that we'll handle in Wren
			// This is a placeholder - actual parsing happens in WrenDialogue
		}

		// Parse choices (optional)
		auto choicesIt = nodeData.find("choices");
		if (choicesIt != nodeData.end())
		{
			// Choices should be stored as a special marker
			// Actual parsing happens in WrenDialogue
		}

		// Parse targets (optional)
		auto targetsIt = nodeData.find("targets");
		if (targetsIt != nodeData.end())
		{
			// Targets should be stored as a special marker
			// Actual parsing happens in WrenDialogue
		}

		return node;
	}

	std::vector<Choice> DialogueLoader::ParseChoices(WrenVM* vm, int slot)
	{
		wrenEnsureSlots(vm, slot + 4);
		
		std::vector<Choice> choices;

		// Get list count
		int count = wrenGetListCount(vm, slot);

		for (int i = 0; i < count; ++i)
		{
			wrenGetListElement(vm, slot, i, slot + 1);

			// Each choice is a map with "text" and "target"
			wrenSetSlotString(vm, slot + 2, "text");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			const char* text = wrenGetSlotString(vm, slot + 3);

			wrenSetSlotString(vm, slot + 2, "target");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			const char* target = wrenGetSlotString(vm, slot + 3);

			choices.emplace_back(text, target);
		}

		return choices;
	}

	std::vector<ConditionalTarget> DialogueLoader::ParseTargets(
		GameContext& context,
		DialogueRegistry& registry,
		WrenVM* vm,
		int slot)
	{
		wrenEnsureSlots(vm, slot + 4);
		
		std::vector<ConditionalTarget> targets;

		// Get list count
		int count = wrenGetListCount(vm, slot);

		for (int i = 0; i < count; ++i)
		{
			wrenGetListElement(vm, slot, i, slot + 1);

			ConditionalTarget target;

			// Parse conditions (optional)
			wrenSetSlotString(vm, slot + 2, "conditions");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			if (wrenGetSlotType(vm, slot + 3) != WREN_TYPE_NULL)
			{
				target.conditions = ParseConditions(context, registry, vm, slot + 3);
			}

			// Parse target node
			wrenSetSlotString(vm, slot + 2, "node");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			target.targetNode = wrenGetSlotString(vm, slot + 3);

			targets.push_back(std::move(target));
		}

		return targets;
	}

	std::vector<std::unique_ptr<Condition>> DialogueLoader::ParseConditions(
		GameContext& context,
		DialogueRegistry& registry,
		WrenVM* vm,
		int slot)
	{
		wrenEnsureSlots(vm, slot + 4);
		
		std::vector<std::unique_ptr<Condition>> conditions;

		// Get list count
		int count = wrenGetListCount(vm, slot);

		for (int i = 0; i < count; ++i)
		{
			wrenGetListElement(vm, slot, i, slot + 1);

			// Get type
			wrenSetSlotString(vm, slot + 2, "type");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			const char* type = wrenGetSlotString(vm, slot + 3);

			// Get parameters
			wrenSetSlotString(vm, slot + 2, "parameters");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			auto params = ParseParameters(vm, slot + 3);

			// Create condition
			auto condition = registry.CreateCondition(context, type, params);
			if (condition)
			{
				conditions.push_back(std::move(condition));
			}
		}

		return conditions;
	}

	std::vector<std::unique_ptr<Command>> DialogueLoader::ParseCommands(
		GameContext& context,
		DialogueRegistry& registry,
		WrenVM* vm,
		int slot)
	{
		wrenEnsureSlots(vm, slot + 4);
		
		std::vector<std::unique_ptr<Command>> commands;

		// Get list count
		int count = wrenGetListCount(vm, slot);

		for (int i = 0; i < count; ++i)
		{
			wrenGetListElement(vm, slot, i, slot + 1);

			// Get type
			wrenSetSlotString(vm, slot + 2, "type");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			const char* type = wrenGetSlotString(vm, slot + 3);

			// Get parameters
			wrenSetSlotString(vm, slot + 2, "parameters");
			wrenGetMapValue(vm, slot + 1, slot + 2, slot + 3);
			auto params = ParseParameters(vm, slot + 3);

			// Create command
			auto command = registry.CreateCommand(context, type, params);
			if (command)
			{
				commands.push_back(std::move(command));
			}
		}

		return commands;
	}

	std::unordered_map<std::string, DialogueValue> DialogueLoader::ParseParameters(WrenVM* vm, int slot)
	{
		wrenEnsureSlots(vm, slot + 4);
		
		std::unordered_map<std::string, DialogueValue> params;

		// Iterate through map
		// Note: Wren doesn't have a direct API to iterate maps, so this assumes
		// the parameters are handled differently in the actual Wren binding

		return params;
	}
}