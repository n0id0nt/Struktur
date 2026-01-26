#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include "DialogueNode.h"
#include "DialogueStructures.h"
#include "DialogueRegistry.h"
#include "DialogueConverter.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	class DialogueManager
	{
	public:
		DialogueManager();
		~DialogueManager();

		// Registry access
		DialogueRegistry& GetRegistry() { return m_registry; }
		const DialogueRegistry& GetRegistry() const { return m_registry; }

		// Data Loading
		void LoadDialogueFromMap(const DialogueDataMap& data);
		void UnloadDialogue(const std::string& dialogueSetName);
		void Clear();

		// Dialogue Flow Control
		DialogueResult StartDialogue(const std::string& nodeId);
		DialogueResult MakeChoice(int choiceIndex);
		DialogueResult ContinueDialogue();
		void EndDialogue();

		// State Query
		bool IsDialogueActive() const;
		std::string GetCurrentNodeId() const;
		const DialogueNode* GetNode(const std::string& nodeId) const;

		// Validation & Debugging
		std::vector<ValidationError> ValidateDialogue() const;
		std::vector<std::string> GetUnreachableNodes() const;
		std::vector<std::string> GetAllNodeIds() const;

	private:
		DialogueRegistry m_registry;
		std::unordered_map<std::string, std::unique_ptr<DialogueNode>> m_nodes;
		DialogueNode* m_currentNode;
		std::vector<std::string> m_history;

		DialogueResult ProcessCurrentNode();
		bool EvaluateConditions(const std::vector<std::unique_ptr<Condition>>& conditions);
		void ExecuteCommands(const std::vector<std::unique_ptr<Command>>& commands);
		std::vector<DialogueResult::ChoiceInfo> GetAvailableChoices();
	};
}
