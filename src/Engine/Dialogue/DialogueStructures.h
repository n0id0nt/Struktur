#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Struktur::Dialogue
{
	// Input format from Wren (or other sources)
	struct ConditionData
	{
		std::string type;
		std::unordered_map<std::string, std::string> parameters;
	};

	struct CommandData
	{
		std::string type;
		std::unordered_map<std::string, std::string> parameters;
	};

	struct ChoiceData
	{
		std::string text;
		std::string targetNode;
		std::vector<ConditionData> conditions;
	};

	struct NodeData
	{
		std::string speaker;
		std::string text;
		std::vector<ConditionData> conditions;
		std::vector<CommandData> commands;
		std::vector<ChoiceData> choices;
		std::string nextNode;
	};

	struct DialogueDataMap
	{
		std::unordered_map<std::string, NodeData> nodes;
	};

	// Output format to caller (Wren or C++)
	struct DialogueResult
	{
		enum class Status
		{
			SUCCESS,           // Dialogue processed successfully
			NO_ACTIVE_NODE,    // No dialogue is currently active
			INVALID_CHOICE,    // Choice index out of range
			NODE_NOT_FOUND,    // Referenced node doesn't exist
			CONDITION_FAILED,  // Node conditions not met (skipped)
			DIALOGUE_ENDED     // Dialogue reached natural end
		};

		struct ChoiceInfo
		{
			int index;
			std::string text;
		};

		Status status;
		std::string nodeId;
		std::string speaker;
		std::string text;
		std::vector<ChoiceInfo> choices;
		bool hasEnded;
	};

	struct ValidationError
	{
		enum class Type
		{
			MISSING_NODE_REFERENCE,
			UNREACHABLE_NODE,
			CIRCULAR_REFERENCE,
			MISSING_TEXT,
			INVALID_CONDITION,
			INVALID_COMMAND
		};

		Type type;
		std::string nodeId;
		std::string message;
	};
}
