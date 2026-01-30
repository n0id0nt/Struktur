// DialogueManager.h
// Main dialogue system manager - orchestrates dialogue flow
// Part of the Struktur dialogue system

#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <optional>

#include "DialogueNode.h"
#include "DialogueResult.h"
#include "DialogueRegistry.h"

namespace Struktur
{
	class GameContext;
}

namespace Struktur::Dialogue
{
	// Main dialogue manager
	// Manages dialogue flow, node processing, and condition/command execution
	class DialogueManager
	{
	public:
		DialogueManager();
		~DialogueManager();

		// Loading
		void LoadDialogueNodes(std::map<std::string, DialogueNode>&& nodes);
		void Clear();

		// Flow control
		DialogueResult StartDialogue(GameContext& context, const std::string& nodeId);
		DialogueResult MakeChoice(GameContext& context, int choiceIndex);
		DialogueResult Continue(GameContext& context);

		// Queries
		bool IsDialogueActive() const;
		std::optional<std::string> GetCurrentNodeId() const;
		const DialogueNode* GetNode(const std::string& nodeId) const;
		size_t GetNodeCount() const { return m_nodes.size(); }

	private:
		// Internal processing
		DialogueResult ProcessNode(GameContext& context, const std::string& nodeId);
		//TODO move this to the node
		std::optional<std::string> EvaluateTargets(GameContext& context, const std::vector<ConditionalTarget>& targets);
		bool EvaluateConditions(GameContext& context, const std::vector<Condition>& conditions);
		void ExecuteCommands(GameContext& context, const std::vector<Command>& commands);

		// Node storage
		std::map<std::string, DialogueNode> m_nodes;
		DialogueNode* m_currentNode;
		std::vector<std::string> m_history;

	};
}
