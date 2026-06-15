#include "DialogueManager.h"

#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
DialogueManager::DialogueManager()
    : m_nodes(),
      m_currentNode(nullptr),
      m_history()
{
	DEBUG_INFO("DialogueManager initialized");
}

DialogueManager::~DialogueManager()
{
	ASSERT_MSG(m_nodes.empty(), "Dialogue manager has not been cleared");
}

void DialogueManager::AddNode(std::string nodeKey, std::unique_ptr<DialogueNode> node)
{
	m_nodes[nodeKey] = std::move(node);
}

void DialogueManager::Clear()
{
	m_nodes.clear();
	m_currentNode = nullptr;
	m_history.clear();
	DEBUG_INFO("Cleared all dialogue nodes");
}

DialogueResult DialogueManager::StartDialogue(GameContext& context, const std::string& nodeId)
{
	DEBUG_INFO("Starting dialogue at node: %s", nodeId.c_str());

	m_currentNode = nullptr;
	m_history.clear();

	return ProcessNode(context, nodeId);
}

DialogueResult DialogueManager::MakeChoice(GameContext& context, int choiceIndex)
{
	if (!m_currentNode)
	{
		DEBUG_WARNING("MakeChoice called but no active dialogue node");
		return DialogueResult::NoActiveNode();
	}

	const auto& choices = m_currentNode->GetChoices();
	if (choiceIndex < 0 || choiceIndex >= static_cast<int>(choices.size()))
	{
		DEBUG_ERROR("Invalid choice index %d (available: %zu)", choiceIndex, choices.size());
		return DialogueResult::InvalidChoice();
	}

	const std::string& targetNode = choices[choiceIndex]->targetNode;
	DEBUG_INFO("Player chose option %d, jumping to node: %s", choiceIndex, targetNode.c_str());

	return ProcessNode(context, targetNode);
}

DialogueResult DialogueManager::Continue(GameContext& context)
{
	if (!m_currentNode)
	{
		DEBUG_WARNING("Continue called but no active dialogue node");
		return DialogueResult::NoActiveNode();
	}

	if (!m_currentNode->HasNext())
	{
		DEBUG_WARNING("Continue called but current node has no 'next'");
		return DialogueResult::NoActiveNode();
	}

	const std::string& nextNode = m_currentNode->GetNext().value();
	DEBUG_INFO("Auto-continuing to node: %s", nextNode.c_str());

	return ProcessNode(context, nextNode);
}

void DialogueManager::ClearDialogue(GameContext& context)
{
	m_currentNode = nullptr;
	m_history.clear();
}

DialogueNode* DialogueManager::SetActiveNode(GameContext& context, const std::string& nodeId)
{
	// Find node
	auto it = m_nodes.find(nodeId);
	if (it == m_nodes.end())
	{
		DEBUG_ERROR("Node '%s' not found", nodeId.c_str());
		m_currentNode = nullptr;
		return nullptr;
	}

	// Set as current node
	m_currentNode = it->second.get();
	m_history.push_back(nodeId);
	return m_currentNode;
}

bool DialogueManager::IsDialogueActive() const
{
	return m_currentNode != nullptr;
}

std::optional<std::string> DialogueManager::GetCurrentNodeId() const
{
	if (m_currentNode)
	{
		return m_currentNode->GetId();
	}
	return std::nullopt;
}

DialogueNode* DialogueManager::GetCurrentNode() const
{
	return m_currentNode;
}

const DialogueNode* DialogueManager::GetNode(const std::string& nodeId) const
{
	auto it = m_nodes.find(nodeId);
	if (it != m_nodes.end())
	{
		return it->second.get();
	}
	return nullptr;
}

size_t DialogueManager::GetNodeCount() const
{
	return m_nodes.size();
}

DialogueResult DialogueManager::ProcessNode(GameContext& context, const std::string& nodeId)
{
	// Find node
	auto it = m_nodes.find(nodeId);
	if (it == m_nodes.end())
	{
		DEBUG_ERROR("Node '%s' not found", nodeId.c_str());
		m_currentNode = nullptr;
		return DialogueResult::NodeNotFound(nodeId);
	}

	// Set as current node
	m_currentNode = it->second.get();
	m_history.push_back(nodeId);

	// Execute all commands in this node
	ExecuteCommands(context, m_currentNode->GetCommands());

	// Create result
	DialogueResult result = DialogueResult::Success(nodeId);
	result.speaker        = m_currentNode->GetSpeaker();
	result.text           = m_currentNode->GetText();

	// Determine next step
	if (m_currentNode->HasTargets())
	{
		// Evaluate targets and jump to first matching node
		auto targetNode = EvaluateTargets(context, m_currentNode->GetTargets());
		if (targetNode.has_value())
		{
			DEBUG_INFO("Target conditions matched, jumping to: %s", targetNode.value().c_str());
			return ProcessNode(context, targetNode.value());
		}
		else
		{
			DEBUG_WARNING("No target conditions matched in node '%s', ending dialogue", nodeId.c_str());
			m_currentNode   = nullptr;
			result.hasEnded = true;
		}
	}
	else if (m_currentNode->HasChoices())
	{
		// Return choices to player
		const auto& choices = m_currentNode->GetChoices();
		for (size_t i = 0; i < choices.size(); ++i)
		{
			result.choices.emplace_back(static_cast<int>(i), choices[i]->text);
		}
	}
	else if (m_currentNode->HasNext())
	{
		// Signal that auto-advance is available
		result.shouldAutoAdvance = true;
	}
	else
	{
		// No continuation, dialogue ends
		m_currentNode   = nullptr;
		result.hasEnded = true;
	}

	return result;
}

std::optional<std::string> DialogueManager::EvaluateTargets(GameContext& context, const ConditionalTargetList& targets)
{
	for (const auto& target : targets)
	{
		// If no conditions, this target always matches
		if (target->conditions.empty())
		{
			return target->targetNode;
		}

		// Evaluate all conditions
		if (EvaluateConditions(context, target->conditions))
		{
			return target->targetNode;
		}
	}

	return std::nullopt;
}

bool DialogueManager::EvaluateConditions(GameContext& context, const ConditionList& conditions)
{
	// All conditions must be true
	for (const auto& condition : conditions)
	{
		if (!condition->Evaluate(context))
		{
			return false;
		}
	}
	return true;
}

void DialogueManager::ExecuteCommands(GameContext& context, const CommandList& commands)
{
	for (const auto& command : commands)
	{
		command->Execute(context);
	}
}
}  // namespace Struktur::Dialogue
