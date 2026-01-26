#include "DialogueManager.h"

#include "Engine/GameContext.h"
#include "Debug/Assertions.h"

namespace Struktur::Dialogue
{
	DialogueManager::DialogueManager()
		: m_registry()
		, m_nodes()
		, m_currentNode(nullptr)
		, m_history()
	{
		DEBUG_INFO("DialogueManager initialized");
	}

	DialogueManager::~DialogueManager()
	{
		DEBUG_INFO("DialogueManager destroyed");
	}

	void DialogueManager::LoadDialogueFromMap(const DialogueDataMap& data)
	{
		DEBUG_INFO("Loading dialogue with %zu nodes", data.nodes.size());

		DialogueConverter converter(m_registry);

		// Convert data map to internal node format
		auto convertedNodes = converter.Convert(data);

		// Merge into existing nodes (or replace if node already exists)
		for (auto& [nodeId, node] : convertedNodes)
		{
			m_nodes[nodeId] = std::move(node);
		}

		// Validate after loading
		auto errors = ValidateDialogue();
		if (!errors.empty())
		{
			DEBUG_WARNING("Dialogue validation found %zu errors", errors.size());
			for (const auto& error : errors)
			{
				DEBUG_WARNING("Validation error in node '%s': %s",
					error.nodeId.c_str(), error.message.c_str());
			}
		}
	}

	void DialogueManager::UnloadDialogue(const std::string& dialogueSetName)
	{
		// TODO: Track which nodes belong to which dialogue set
		// For now, this is a placeholder
		DEBUG_WARNING("UnloadDialogue not yet implemented for set: %s", dialogueSetName.c_str());
	}

	void DialogueManager::Clear()
	{
		DEBUG_INFO("Clearing all dialogue");
		m_nodes.clear();
		m_currentNode = nullptr;
		m_history.clear();
	}

	DialogueResult DialogueManager::StartDialogue(const std::string& nodeId)
	{
		DEBUG_INFO("Starting dialogue at node: %s", nodeId.c_str());

		// Find the starting node
		auto it = m_nodes.find(nodeId);
		if (it == m_nodes.end())
		{
			DEBUG_ERROR("Starting node '%s' not found", nodeId.c_str());
			return DialogueResult{
				DialogueResult::Status::NODE_NOT_FOUND,
				nodeId,
				"", "", {}, true
			};
		}

		// Clear history and set current node
		m_history.clear();
		m_currentNode = it->second.get();

		// Process and return result
		return ProcessCurrentNode();
	}

	DialogueResult DialogueManager::MakeChoice(int choiceIndex)
	{
		if (m_currentNode == nullptr)
		{
			DEBUG_WARNING("MakeChoice called with no active dialogue");
			return DialogueResult{
				DialogueResult::Status::NO_ACTIVE_NODE,
				"", "", "", {}, true
			};
		}

		// Get available choices (filtered by conditions)
		auto availableChoices = GetAvailableChoices();

		if (choiceIndex < 0 || choiceIndex >= static_cast<int>(availableChoices.size()))
		{
			DEBUG_ERROR("Invalid choice index %d (available: %zu)",
				choiceIndex, availableChoices.size());
			return DialogueResult{
				DialogueResult::Status::INVALID_CHOICE,
				m_currentNode->GetId(), "", "", {}, false
			};
		}

		// Get the target node from the choice
		const auto& allChoices = m_currentNode->GetChoices();
		const std::string& targetNodeId = allChoices[availableChoices[choiceIndex].index].GetTargetNode();

		DEBUG_INFO("Choice %d selected, moving to node: %s", choiceIndex, targetNodeId.c_str());

		// Find target node
		auto it = m_nodes.find(targetNodeId);
		if (it == m_nodes.end())
		{
			DEBUG_ERROR("Target node '%s' not found", targetNodeId.c_str());
			m_currentNode = nullptr;
			return DialogueResult{
				DialogueResult::Status::NODE_NOT_FOUND,
				targetNodeId, "", "", {}, true
			};
		}

		// Move to target node
		m_currentNode = it->second.get();

		// Process the new node
		return ProcessCurrentNode();
	}

	DialogueResult DialogueManager::ContinueDialogue()
	{
		if (m_currentNode == nullptr)
		{
			DEBUG_WARNING("ContinueDialogue called with no active dialogue");
			return DialogueResult{
				DialogueResult::Status::NO_ACTIVE_NODE,
				"", "", "", {}, true
			};
		}

		const std::string& nextNodeId = m_currentNode->GetNextNode();

		if (nextNodeId.empty())
		{
			DEBUG_INFO("Dialogue ended naturally");
			m_currentNode = nullptr;
			return DialogueResult{
				DialogueResult::Status::DIALOGUE_ENDED,
				"", "", "", {}, true
			};
		}

		DEBUG_INFO("Continuing to next node: %s", nextNodeId.c_str());

		// Find next node
		auto it = m_nodes.find(nextNodeId);
		if (it == m_nodes.end())
		{
			DEBUG_ERROR("Next node '%s' not found", nextNodeId.c_str());
			m_currentNode = nullptr;
			return DialogueResult{
				DialogueResult::Status::NODE_NOT_FOUND,
				nextNodeId, "", "", {}, true
			};
		}

		// Move to next node
		m_currentNode = it->second.get();

		// Process the new node
		return ProcessCurrentNode();
	}

	void DialogueManager::EndDialogue()
	{
		DEBUG_INFO("Ending dialogue");
		m_currentNode = nullptr;
	}

	bool DialogueManager::IsDialogueActive() const
	{
		return m_currentNode != nullptr;
	}

	std::string DialogueManager::GetCurrentNodeId() const
	{
		return m_currentNode ? m_currentNode->GetId() : "";
	}

	const DialogueNode* DialogueManager::GetNode(const std::string& nodeId) const
	{
		auto it = m_nodes.find(nodeId);
		return it != m_nodes.end() ? it->second.get() : nullptr;
	}

	DialogueResult DialogueManager::ProcessCurrentNode()
	{
		ASSERT_MSG(m_currentNode, "ProcessCurrentNode called with null current node");

		// Check if node conditions are met
		if (!EvaluateConditions(m_currentNode->GetConditions()))
		{
			DEBUG_INFO("Node '%s' conditions not met, skipping", m_currentNode->GetId().c_str());

			// Node conditions failed, try to skip to next
			const std::string& nextNodeId = m_currentNode->GetNextNode();

			if (nextNodeId.empty())
			{
				DEBUG_INFO("No next node, dialogue ending");
				m_currentNode = nullptr;
				return DialogueResult{
					DialogueResult::Status::DIALOGUE_ENDED,
					"", "", "", {}, true
				};
			}

			// Move to next node and try again
			auto it = m_nodes.find(nextNodeId);
			if (it == m_nodes.end())
			{
				DEBUG_ERROR("Next node '%s' not found", nextNodeId.c_str());
				m_currentNode = nullptr;
				return DialogueResult{
					DialogueResult::Status::NODE_NOT_FOUND,
					nextNodeId, "", "", {}, true
				};
			}

			m_currentNode = it->second.get();
			return ProcessCurrentNode();  // Recursively process next node
		}

		// Execute commands
		ExecuteCommands(m_currentNode->GetCommands());

		// Get available choices
		auto availableChoices = GetAvailableChoices();

		// Build result
		DialogueResult result;
		result.status = DialogueResult::Status::SUCCESS;
		result.nodeId = m_currentNode->GetId();
		result.speaker = m_currentNode->GetSpeaker();
		result.text = m_currentNode->GetText();
		result.choices = availableChoices;
		result.hasEnded = false;

		// Add current node to history
		m_history.push_back(m_currentNode->GetId());

		DEBUG_INFO("Processed node '%s', %zu choices available",
			result.nodeId.c_str(), availableChoices.size());

		return result;
	}

	bool DialogueManager::EvaluateConditions(
		const std::vector<std::unique_ptr<Condition>>& conditions
	)
	{
		for (const auto& condition : conditions)
		{
			if (!condition->Evaluate())
				return false;
		}
		return true;
	}

	void DialogueManager::ExecuteCommands(
		const std::vector<std::unique_ptr<Command>>& commands
	)
	{
		for (const auto& command : commands)
		{
			DEBUG_INFO("Executing command: %s", command->GetDescription().c_str());
			command->Execute();
		}
	}

	std::vector<DialogueResult::ChoiceInfo> DialogueManager::GetAvailableChoices()
	{
		std::vector<DialogueResult::ChoiceInfo> available;

		const auto& choices = m_currentNode->GetChoices();
		for (size_t i = 0; i < choices.size(); ++i)
		{
			// Check if choice conditions are met
			if (EvaluateConditions(choices[i].GetConditions()))
			{
				available.push_back({
					static_cast<int>(i),
					choices[i].GetText()
				});
			}
		}

		return available;
	}

	std::vector<ValidationError> DialogueManager::ValidateDialogue() const
	{
		std::vector<ValidationError> errors;

		for (const auto& [nodeId, node] : m_nodes)
		{
			// Check for missing text
			if (node->GetText().empty())
			{
				errors.push_back({
					ValidationError::Type::MISSING_TEXT,
					nodeId,
					"Node has empty text field"
				});
			}

			// Check next node reference
			const std::string& nextNode = node->GetNextNode();
			if (!nextNode.empty() && m_nodes.find(nextNode) == m_nodes.end())
			{
				errors.push_back({
					ValidationError::Type::MISSING_NODE_REFERENCE,
					nodeId,
					"Next node '" + nextNode + "' does not exist"
				});
			}

			// Check choice target references
			for (const auto& choice : node->GetChoices())
			{
				const std::string& targetNode = choice.GetTargetNode();
				if (m_nodes.find(targetNode) == m_nodes.end())
				{
					errors.push_back({
						ValidationError::Type::MISSING_NODE_REFERENCE,
						nodeId,
						"Choice target '" + targetNode + "' does not exist"
					});
				}
			}
		}

		// Check for unreachable nodes
		auto unreachable = GetUnreachableNodes();
		for (const auto& nodeId : unreachable)
		{
			errors.push_back({
				ValidationError::Type::UNREACHABLE_NODE,
				nodeId,
				"Node is not reachable from any other node"
			});
		}

		return errors;
	}

	std::vector<std::string> DialogueManager::GetUnreachableNodes() const
	{
		std::unordered_set<std::string> reachable;

		// Collect all referenced nodes
		for (const auto& [nodeId, node] : m_nodes)
		{
			const std::string& nextNode = node->GetNextNode();
			if (!nextNode.empty())
				reachable.insert(nextNode);

			for (const auto& choice : node->GetChoices())
				reachable.insert(choice.GetTargetNode());
		}

		// Find unreachable nodes
		std::vector<std::string> unreachable;
		for (const auto& [nodeId, node] : m_nodes)
		{
			if (reachable.find(nodeId) == reachable.end())
				unreachable.push_back(nodeId);
		}

		return unreachable;
	}

	std::vector<std::string> DialogueManager::GetAllNodeIds() const
	{
		std::vector<std::string> nodeIds;
		nodeIds.reserve(m_nodes.size());

		for (const auto& [nodeId, node] : m_nodes)
			nodeIds.push_back(nodeId);

		return nodeIds;
	}
}
