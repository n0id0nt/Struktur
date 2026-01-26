#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "DialogueNode.h"
#include "DialogueStructures.h"
#include "DialogueRegistry.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Converts external data formats to internal DialogueNode representation
	class DialogueConverter
	{
	public:
		DialogueConverter(DialogueRegistry& registry);
		~DialogueConverter() = default;

		// Convert full dialogue data map to nodes
		std::unordered_map<std::string, std::unique_ptr<DialogueNode>> 
			Convert(const DialogueDataMap& dataMap);

		// Convert single node
		std::unique_ptr<DialogueNode> ConvertNode(
			const std::string& nodeId,
			const NodeData& nodeData
		);

		// Convert components
		std::unique_ptr<Condition> ConvertCondition(const ConditionData& condData);
		std::unique_ptr<Command> ConvertCommand(const CommandData& cmdData);
		DialogueChoice ConvertChoice(const ChoiceData& choiceData);

	private:
		DialogueRegistry& m_registry;

		// Helper for nested condition parsing (and, or, not)
		std::vector<std::unique_ptr<Condition>> ConvertConditionList(
			const std::vector<ConditionData>& condDataList
		);
	};
}
