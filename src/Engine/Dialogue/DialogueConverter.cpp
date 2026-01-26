#include "DialogueConverter.h"

#include "Engine/GameContext.h"
#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
	DialogueConverter::DialogueConverter(DialogueRegistry& registry)
		: m_registry(registry)
	{
	}

	std::unordered_map<std::string, std::unique_ptr<DialogueNode>> 
		DialogueConverter::Convert(const DialogueDataMap& dataMap)
	{
		std::unordered_map<std::string, std::unique_ptr<DialogueNode>> nodes;

		for (const auto& [nodeId, nodeData] : dataMap.nodes)
		{
			auto node = ConvertNode(nodeId, nodeData);
			nodes[nodeId] = std::move(node);
		}

		return nodes;
	}

	std::unique_ptr<DialogueNode> DialogueConverter::ConvertNode(
		const std::string& nodeId,
		const NodeData& nodeData
	)
	{
		auto node = std::make_unique<DialogueNode>(nodeId);

		// Set basic properties
		node->SetSpeaker(nodeData.speaker);
		node->SetText(nodeData.text);
		node->SetNextNode(nodeData.nextNode);

		// Convert conditions
		for (const auto& condData : nodeData.conditions)
		{
			auto condition = ConvertCondition(condData);
			if (condition)
				node->AddCondition(std::move(condition));
		}

		// Convert commands
		for (const auto& cmdData : nodeData.commands)
		{
			auto command = ConvertCommand(cmdData);
			if (command)
				node->AddCommand(std::move(command));
		}

		// Convert choices
		for (const auto& choiceData : nodeData.choices)
		{
			auto choice = ConvertChoice(choiceData);
			node->AddChoice(std::move(choice));
		}

		return node;
	}

	std::unique_ptr<Condition> DialogueConverter::ConvertCondition(const ConditionData& condData)
	{
		const std::string& type = condData.type;

		// Handle composite conditions
		if (type == "and")
		{
			// For composite conditions, we expect a "conditions" parameter containing nested conditions
			// This would require special handling in the data format
			// For now, we'll log an error
			DEBUG_ERROR("Composite 'and' condition not fully implemented yet");
			return nullptr;
		}

		if (type == "or")
		{
			DEBUG_ERROR("Composite 'or' condition not fully implemented yet");
			return nullptr;
		}

		if (type == "not")
		{
			DEBUG_ERROR("Composite 'not' condition not fully implemented yet");
			return nullptr;
		}

		// Use registry to create condition
		if (!m_registry.HasConditionType(type))
		{
			DEBUG_ERROR("Unknown condition type: %s", type.c_str());
			return nullptr;
		}

		return m_registry.CreateCondition(type, condData.parameters);
	}

	std::unique_ptr<Command> DialogueConverter::ConvertCommand(const CommandData& cmdData)
	{
		const std::string& type = cmdData.type;

		// Handle composite commands
		if (type == "sequence")
		{
			DEBUG_ERROR("Composite 'sequence' command not fully implemented yet");
			return nullptr;
		}

		// Use registry to create command
		if (!m_registry.HasCommandType(type))
		{
			DEBUG_ERROR("Unknown command type: %s", type.c_str());
			return nullptr;
		}

		return m_registry.CreateCommand(type, cmdData.parameters);
	}

	DialogueChoice DialogueConverter::ConvertChoice(const ChoiceData& choiceData)
	{
		DialogueChoice choice(choiceData.text, choiceData.targetNode);

		// Convert choice conditions
		for (const auto& condData : choiceData.conditions)
		{
			auto condition = ConvertCondition(condData);
			if (condition)
				choice.AddCondition(std::move(condition));
		}

		return choice;
	}

	std::vector<std::unique_ptr<Condition>> DialogueConverter::ConvertConditionList(
		const std::vector<ConditionData>& condDataList
	)
	{
		std::vector<std::unique_ptr<Condition>> conditions;
		conditions.reserve(condDataList.size());

		for (const auto& condData : condDataList)
		{
			auto condition = ConvertCondition(condData);
			if (condition)
				conditions.push_back(std::move(condition));
		}

		return conditions;
	}
}
