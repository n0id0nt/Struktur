// DialogueResult.h
// Result structure returned from dialogue operations
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace Struktur::Dialogue
{
	// Status codes for dialogue operations
	enum class DialogueStatus
	{
		SUCCESS,
		NO_ACTIVE_NODE,
		INVALID_CHOICE,
		NODE_NOT_FOUND,
		ERROR
	};

	// Information about a single choice
	struct ChoiceInfo
	{
		int index;
		std::string text;

		ChoiceInfo() : index(0), text("") {}
		ChoiceInfo(int i, const std::string& t) : index(i), text(t) {}
	};

	// Result returned from dialogue operations
	struct DialogueResult
	{
		DialogueStatus status;
		std::string nodeId;
		std::optional<std::string> speaker;
		std::optional<std::string> text;
		std::vector<ChoiceInfo> choices;
		bool hasEnded;
		bool shouldAutoAdvance;

		DialogueResult()
			: status(DialogueStatus::ERROR)
			, nodeId("")
			, speaker(std::nullopt)
			, text(std::nullopt)
			, choices()
			, hasEnded(false)
			, shouldAutoAdvance(false)
		{
		}

		static DialogueResult Success(const std::string& node)
		{
			DialogueResult result;
			result.status = DialogueStatus::SUCCESS;
			result.nodeId = node;
			return result;
		}

		static DialogueResult NoActiveNode()
		{
			DialogueResult result;
			result.status = DialogueStatus::NO_ACTIVE_NODE;
			result.hasEnded = true;
			return result;
		}

		static DialogueResult InvalidChoice()
		{
			DialogueResult result;
			result.status = DialogueStatus::INVALID_CHOICE;
			return result;
		}

		static DialogueResult NodeNotFound(const std::string& nodeId)
		{
			DialogueResult result;
			result.status = DialogueStatus::NODE_NOT_FOUND;
			result.nodeId = nodeId;
			result.hasEnded = true;
			return result;
		}

		static DialogueResult Error(const std::string& nodeId = "")
		{
			DialogueResult result;
			result.status = DialogueStatus::ERROR;
			result.nodeId = nodeId;
			result.hasEnded = true;
			return result;
		}
	};
}