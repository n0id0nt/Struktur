// DialogueLoader.h
// Loads dialogue data from Wren map format into DialogueNode structures
// Part of the Struktur dialogue system

#pragma once

#include "DialogueNode.h"
#include "DialogueRegistry.h"
#include "DialogueValue.h"
#include <string>
#include <map>
#include <vector>
#include <memory>

struct WrenVM;

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Temporary storage for dialogue data from Wren
	// Maps node ID to its data map
	using DialogueDataMap = std::unordered_map<std::string, std::unordered_map<std::string, DialogueValue>>;

	// Loads dialogue nodes from Wren data format
	class DialogueLoader
	{
	public:
		// Load dialogue nodes from data map
		// Returns map of node ID to DialogueNode
		static std::unordered_map<std::string, std::unique_ptr<DialogueNode>> LoadFromData(
			DialogueRegistry& registry,
			WrenVM* vm,
			const DialogueDataMap& dataMap);

	private:
		// Parse a single node from data
		static std::unique_ptr<DialogueNode> ParseNode(
			DialogueRegistry& registry,
			WrenVM* vm,
			const std::string& nodeId,
			const std::unordered_map<std::string, DialogueValue>& nodeData);

		// Parse choices array
		static std::vector<Choice> ParseChoices(WrenVM* vm, int slot);

		// Parse targets array
		static std::vector<ConditionalTarget> ParseTargets(
			GameContext& context,
			DialogueRegistry& registry,
			WrenVM* vm,
			int slot);

		// Parse conditions array
		static std::vector<std::unique_ptr<Condition>> ParseConditions(
			GameContext& context,
			DialogueRegistry& registry,
			WrenVM* vm,
			int slot);

		// Parse commands array
		static std::vector<std::unique_ptr<Command>> ParseCommands(
			GameContext& context,
			DialogueRegistry& registry,
			WrenVM* vm,
			int slot);

		// Parse parameters map from Wren map
		static std::unordered_map<std::string, DialogueValue> ParseParameters(WrenVM* vm, int slot);
	};
}