// DialogueEditorWindow_Operations.cpp
// File operations, node management, and validation for dialogue editor
// This provides all the missing function implementations

#include "DialogueEditorWindow.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Debug/Editor/Exporters/DialogueExporter.h"
#include "Debug/Assertions.h"
#include <fstream>
#include <sstream>

namespace Struktur::Debug
{
	// ============================================================================
	// FILE OPERATIONS
	// ============================================================================

	void DialogueEditorWindow::LoadDialogueFile(GameContext& context, const std::string& filepath)
	{
		// TODO: In a full implementation, this would:
		// 1. Execute the Wren script to load dialogue data
		// 2. Get nodes from DialogueManager
		// 3. Copy them into m_nodes
		// For now, create a stub implementation

		DEBUG_INFO("Loading dialogue file: %s", filepath.c_str());

		// Clear existing nodes
		m_nodes.clear();
		m_selectedNodeId = "";
		m_entryNodeId = "";

		// Extract class name from filepath
		size_t lastSlash = filepath.find_last_of("/\\");
		size_t lastDot = filepath.find_last_of('.');
		if (lastSlash != std::string::npos && lastDot != std::string::npos)
		{
			m_currentClassName = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
		}

		m_currentFile = filepath;
		m_hasUnsavedChanges = false;

		// TODO: Actual Wren script execution and node extraction
		// For now, this is a placeholder that would be implemented as:
		// 1. WrenVM execution of the dialogue script
		// 2. Call getData() method
		// 3. Parse returned dialogue data
		// 4. Create DialogueNode objects
		// 5. Add to m_nodes

		// Calculate layout for loaded nodes
		if (!m_nodes.empty())
		{
			CalculateGraphLayout();
		}

		// Validate
		ValidateDialogue();

		DEBUG_INFO("Loaded dialogue with %zu nodes", m_nodes.size());
	}

	void DialogueEditorWindow::SaveDialogueFile(const std::string& filepath)
	{
		DEBUG_INFO("Saving dialogue file: %s", filepath.c_str());

		// Convert nodes to unique_ptr map for exporter
		std::map<std::string, std::unique_ptr<Dialogue::DialogueNode>> nodesToExport;

		for (auto& [nodeId, nodeData] : m_nodes)
		{
			// Create a copy of the node for export
			auto nodeCopy = std::make_unique<Dialogue::DialogueNode>(nodeId);

			// Copy all node data
			if (nodeData.node->GetSpeaker().has_value())
			{
				nodeCopy->SetSpeaker(nodeData.node->GetSpeaker().value());
			}

			if (nodeData.node->GetText().has_value())
			{
				nodeCopy->SetText(nodeData.node->GetText().value());
			}

			if (nodeData.node->GetNext().has_value())
			{
				nodeCopy->SetNext(nodeData.node->GetNext().value());
			}

			// Copy choices
			for (const auto& choice : nodeData.node->GetChoices())
			{
				auto choiceCopy = std::make_unique<Dialogue::Choice>(choice->text, choice->targetNode);
                nodeCopy->AddChoice(std::move(choiceCopy));
			}

			// Copy commands (deep copy needed)
			// Note: This is simplified - full implementation would need proper command copying
			// For now, commands/targets aren't copied as they need special handling

			nodesToExport[nodeId] = std::move(nodeCopy);
		}

		// Export to Wren format
		std::string wrenOutput = Dialogue::DialogueExporter::ExportToWren(nodesToExport, m_currentClassName);

		// Write to file
		std::ofstream file(filepath);
		if (file.is_open())
		{
			file << wrenOutput;
			file.close();

			m_currentFile = filepath;
			m_hasUnsavedChanges = false;

			DEBUG_INFO("Dialogue saved successfully");
		}
		else
		{
			DEBUG_ERROR("Failed to save dialogue file: %s", filepath.c_str());
		}
	}

	void DialogueEditorWindow::CreateNewDialogue(const std::string& className)
	{
		DEBUG_INFO("Creating new dialogue: %s", className.c_str());

		// Clear existing dialogue
		m_nodes.clear();
		m_selectedNodeId = "";
		m_errors.clear();
		m_warnings.clear();

		m_currentClassName = className;
		m_currentFile = "";
		m_hasUnsavedChanges = true;

		// Create an entry node
		std::string entryNodeId = className.empty() ? "entry" : className;
		// Convert first character to lowercase for node name
		if (!entryNodeId.empty())
		{
			entryNodeId[0] = std::tolower(entryNodeId[0]);
		}

		auto entryNode = std::make_unique<Dialogue::DialogueNode>(entryNodeId);
		entryNode->SetSpeaker("Speaker");
		entryNode->SetText("Welcome to the dialogue!");

		NodeData nodeData;
		nodeData.node = std::move(entryNode);
		nodeData.visualPosition = glm::vec2(100.0f, 100.0f);

		m_nodes[entryNodeId] = std::move(nodeData);
		m_entryNodeId = entryNodeId;
		m_selectedNodeId = entryNodeId;

		DEBUG_INFO("Created new dialogue with entry node: %s", entryNodeId.c_str());
	}

	// ============================================================================
	// NODE OPERATIONS
	// ============================================================================

	void DialogueEditorWindow::SelectNode(const std::string& nodeId)
	{
		if (m_nodes.find(nodeId) != m_nodes.end())
		{
			m_selectedNodeId = nodeId;
			DEBUG_INFO("Selected node: %s", nodeId.c_str());
		}
	}

	void DialogueEditorWindow::AddNode(const std::string& nodeId)
	{
		if (nodeId.empty())
		{
			DEBUG_WARNING("Cannot add node with empty ID");
			return;
		}

		if (m_nodes.find(nodeId) != m_nodes.end())
		{
			DEBUG_WARNING("Node '%s' already exists", nodeId.c_str());
			return;
		}

		auto node = std::make_unique<Dialogue::DialogueNode>(nodeId);
		node->SetText("New node text");

		NodeData nodeData;
		nodeData.node = std::move(node);

		// Position new node offset from selected node or at origin
		if (!m_selectedNodeId.empty())
		{
			auto selectedIt = m_nodes.find(m_selectedNodeId);
			if (selectedIt != m_nodes.end())
			{
				nodeData.visualPosition = selectedIt->second.visualPosition + glm::vec2(250.0f, 0.0f);
			}
		}
		else
		{
			nodeData.visualPosition = glm::vec2(100.0f, 100.0f);
		}

		m_nodes[nodeId] = std::move(nodeData);
		m_selectedNodeId = nodeId;
		m_hasUnsavedChanges = true;

		DEBUG_INFO("Added node: %s", nodeId.c_str());
	}

	void DialogueEditorWindow::DeleteNode(const std::string& nodeId)
	{
		auto it = m_nodes.find(nodeId);
		if (it == m_nodes.end())
		{
			DEBUG_WARNING("Cannot delete non-existent node: %s", nodeId.c_str());
			return;
		}

		// Don't allow deleting the entry node
		if (nodeId == m_entryNodeId)
		{
			DEBUG_WARNING("Cannot delete entry node");
			return;
		}

		m_nodes.erase(it);

		if (m_selectedNodeId == nodeId)
		{
			m_selectedNodeId = "";
		}

		m_hasUnsavedChanges = true;

		// Re-validate after deletion
		ValidateDialogue();

		DEBUG_INFO("Deleted node: %s", nodeId.c_str());
	}

	void DialogueEditorWindow::DuplicateNode(const std::string& nodeId)
	{
		auto it = m_nodes.find(nodeId);
		if (it == m_nodes.end())
		{
			DEBUG_WARNING("Cannot duplicate non-existent node: %s", nodeId.c_str());
			return;
		}

		// Generate unique ID for duplicate
		std::string newNodeId = nodeId + "_copy";
		int counter = 1;
		while (m_nodes.find(newNodeId) != m_nodes.end())
		{
			newNodeId = nodeId + "_copy" + std::to_string(counter++);
		}

		// Create duplicate node
		const Dialogue::DialogueNode* sourceNode = it->second.node.get();
		auto duplicateNode = std::make_unique<Dialogue::DialogueNode>(newNodeId);

		// Copy properties
		if (sourceNode->GetSpeaker().has_value())
		{
			duplicateNode->SetSpeaker(sourceNode->GetSpeaker().value());
		}

		if (sourceNode->GetText().has_value())
		{
			duplicateNode->SetText(sourceNode->GetText().value());
		}

		if (sourceNode->GetNext().has_value())
		{
			duplicateNode->SetNext(sourceNode->GetNext().value());
		}

		// Copy choices
		for (const auto& choice : sourceNode->GetChoices())
		{
			auto choiceCopy = std::make_unique<Dialogue::Choice>(choice->text, choice->targetNode);
            duplicateNode->AddChoice(std::move(choiceCopy));
		}

		// Note: Commands and targets would need deep copying
		// Skipping for now as they require special handling

		NodeData nodeData;
		nodeData.node = std::move(duplicateNode);
		nodeData.visualPosition = it->second.visualPosition + glm::vec2(250.0f, 150.0f);

		m_nodes[newNodeId] = std::move(nodeData);
		m_selectedNodeId = newNodeId;
		m_hasUnsavedChanges = true;

		DEBUG_INFO("Duplicated node '%s' as '%s'", nodeId.c_str(), newNodeId.c_str());
	}

	// ============================================================================
	// VALIDATION
	// ============================================================================

	void DialogueEditorWindow::ValidateDialogue()
	{
		m_errors.clear();
		m_warnings.clear();

		if (m_nodes.empty())
		{
			return;
		}

		// Check for missing entry node
		if (m_entryNodeId.empty() || m_nodes.find(m_entryNodeId) == m_nodes.end())
		{
			ValidationWarning warning;
			warning.type = ValidationWarning::Type::UnreachableNode;
			warning.nodeId = "";
			warning.message = "No entry node set for dialogue";
			m_warnings.push_back(warning);
		}

		// Check each node
		std::set<std::string> reachableNodes;
		std::set<std::string> allNodeIds;

		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			allNodeIds.insert(nodeId);
			const Dialogue::DialogueNode* node = nodeData.node.get();

			// Check for missing target nodes
			if (node->GetNext().has_value())
			{
				const std::string& targetId = node->GetNext().value();
				if (m_nodes.find(targetId) == m_nodes.end())
				{
					ValidationError error;
					error.type = ValidationError::Type::MissingTargetNode;
					error.nodeId = nodeId;
					error.message = "Next node '" + targetId + "' does not exist";
					error.suggestion = "Create the node or change the 'next' reference";
					m_errors.push_back(error);
				}
				else
				{
					reachableNodes.insert(targetId);
				}
			}

			// Check choice targets
			for (const auto& choice : node->GetChoices())
			{
				if (m_nodes.find(choice->targetNode) == m_nodes.end())
				{
					ValidationError error;
					error.type = ValidationError::Type::MissingTargetNode;
					error.nodeId = nodeId;
					error.message = "Choice target '" + choice->targetNode + "' does not exist";
					error.suggestion = "Create the node or change the choice target";
					m_errors.push_back(error);
				}
				else
				{
					reachableNodes.insert(choice->targetNode);
				}
			}

			// Check conditional targets
			for (const auto& target : node->GetTargets())
			{
				if (m_nodes.find(target->targetNode) == m_nodes.end())
				{
					ValidationError error;
					error.type = ValidationError::Type::MissingTargetNode;
					error.nodeId = nodeId;
					error.message = "Conditional target '" + target->targetNode + "' does not exist";
					error.suggestion = "Create the node or change the target reference";
					m_errors.push_back(error);
				}
				else
				{
					reachableNodes.insert(target->targetNode);
				}
			}

			// Check for dead ends (no continuation)
			if (!node->HasNext() && node->GetChoices().empty() && node->GetTargets().empty())
			{
				ValidationWarning warning;
				warning.type = ValidationWarning::Type::DeadEnd;
				warning.nodeId = nodeId;
				warning.message = "Node has no continuation (no next, choices, or targets)";
				m_warnings.push_back(warning);
			}

			// Check for missing speaker
			if (!node->GetSpeaker().has_value())
			{
				ValidationWarning warning;
				warning.type = ValidationWarning::Type::MissingSpeaker;
				warning.nodeId = nodeId;
				warning.message = "Node has no speaker set";
				m_warnings.push_back(warning);
			}

			// Check for empty text
			if (!node->GetText().has_value() || node->GetText().value().empty())
			{
				ValidationWarning warning;
				warning.type = ValidationWarning::Type::EmptyText;
				warning.nodeId = nodeId;
				warning.message = "Node has no text";
				m_warnings.push_back(warning);
			}
		}

		// Check for unreachable nodes
		if (!m_entryNodeId.empty())
		{
			reachableNodes.insert(m_entryNodeId);

			for (const auto& nodeId : allNodeIds)
			{
				if (reachableNodes.find(nodeId) == reachableNodes.end() && nodeId != m_entryNodeId)
				{
					ValidationWarning warning;
					warning.type = ValidationWarning::Type::UnreachableNode;
					warning.nodeId = nodeId;
					warning.message = "Node is not reachable from entry point";
					m_warnings.push_back(warning);
				}
			}
		}

		DEBUG_INFO("Validation complete: %zu errors, %zu warnings", m_errors.size(), m_warnings.size());
	}

	// ============================================================================
	// PLAYBACK OPERATIONS
	// ============================================================================

	void DialogueEditorWindow::StartPlayback(GameContext& context, const std::string& entryNodeId)
	{
		DEBUG_INFO("Starting playback from node: %s", entryNodeId.c_str());

		m_isPlaybackActive = true;
		m_playbackHistory.clear();
		m_playbackIntFlags.clear();
		m_playbackBoolFlags.clear();
		m_playbackInventory.clear();

		// Initialize with entry node
		// TODO: Actually call DialogueManager to process the node
		// For now, create a stub result

		m_currentPlaybackResult = Dialogue::DialogueResult::Success(entryNodeId);
		m_playbackHistory.push_back(entryNodeId);

		auto it = m_nodes.find(entryNodeId);
		if (it != m_nodes.end())
		{
			const Dialogue::DialogueNode* node = it->second.node.get();
			m_currentPlaybackResult.speaker = node->GetSpeaker();
			m_currentPlaybackResult.text = node->GetText();

			// Add choices
			const auto& choices = node->GetChoices();
			for (size_t i = 0; i < choices.size(); ++i)
			{
				m_currentPlaybackResult.choices.emplace_back(static_cast<int>(i), choices[i]->text);
			}

			m_currentPlaybackResult.shouldAutoAdvance = node->HasNext();
			m_currentPlaybackResult.hasEnded = !node->HasNext() && choices.empty() && node->GetTargets().empty();
		}
	}

	void DialogueEditorWindow::StopPlayback(GameContext& context)
	{
		DEBUG_INFO("Stopping playback");
		m_isPlaybackActive = false;
		m_playbackHistory.clear();
	}

	void DialogueEditorWindow::ContinuePlayback(GameContext& context)
	{
		if (!m_isPlaybackActive)
			return;

		auto it = m_nodes.find(m_currentPlaybackResult.nodeId);
		if (it == m_nodes.end())
			return;

		const Dialogue::DialogueNode* node = it->second.node.get();
		if (node->HasNext())
		{
			const std::string& nextNodeId = node->GetNext().value();
			m_playbackHistory.push_back(nextNodeId);

			// Process next node
			auto nextIt = m_nodes.find(nextNodeId);
			if (nextIt != m_nodes.end())
			{
				m_currentPlaybackResult = Dialogue::DialogueResult::Success(nextNodeId);
				const Dialogue::DialogueNode* nextNode = nextIt->second.node.get();

				m_currentPlaybackResult.speaker = nextNode->GetSpeaker();
				m_currentPlaybackResult.text = nextNode->GetText();

				const auto& choices = nextNode->GetChoices();
				for (size_t i = 0; i < choices.size(); ++i)
				{
					m_currentPlaybackResult.choices.emplace_back(static_cast<int>(i), choices[i]->text);
				}

				m_currentPlaybackResult.shouldAutoAdvance = nextNode->HasNext();
				m_currentPlaybackResult.hasEnded = !nextNode->HasNext() && choices.empty() && nextNode->GetTargets().empty();
			}
		}
	}

	void DialogueEditorWindow::MakePlaybackChoice(GameContext& context, int choiceIndex)
	{
		if (!m_isPlaybackActive)
			return;

		auto it = m_nodes.find(m_currentPlaybackResult.nodeId);
		if (it == m_nodes.end())
			return;

		const Dialogue::DialogueNode* node = it->second.node.get();
		const auto& choices = node->GetChoices();

		if (choiceIndex < 0 || choiceIndex >= static_cast<int>(choices.size()))
		{
			DEBUG_ERROR("Invalid choice index: %d", choiceIndex);
			return;
		}

		const std::string& targetNodeId = choices[choiceIndex]->targetNode;
		m_playbackHistory.push_back(targetNodeId);

		// Process target node
		auto targetIt = m_nodes.find(targetNodeId);
		if (targetIt != m_nodes.end())
		{
			m_currentPlaybackResult = Dialogue::DialogueResult::Success(targetNodeId);
			const Dialogue::DialogueNode* targetNode = targetIt->second.node.get();

			m_currentPlaybackResult.speaker = targetNode->GetSpeaker();
			m_currentPlaybackResult.text = targetNode->GetText();

			const auto& targetChoices = targetNode->GetChoices();
			for (size_t i = 0; i < targetChoices.size(); ++i)
			{
				m_currentPlaybackResult.choices.emplace_back(static_cast<int>(i), targetChoices[i]->text);
			}

			m_currentPlaybackResult.shouldAutoAdvance = targetNode->HasNext();
			m_currentPlaybackResult.hasEnded = !targetNode->HasNext() && targetChoices.empty() && targetNode->GetTargets().empty();
		}
	}

	// ============================================================================
	// RENDER PANEL STUBS
	// ============================================================================

	void DialogueEditorWindow::RenderNodeEditor(GameContext& context)
	{
		if (m_selectedNodeId.empty())
		{
			ImGui::TextWrapped("Select a node from the graph or file panel to edit its properties.");
			return;
		}

		auto it = m_nodes.find(m_selectedNodeId);
		if (it == m_nodes.end())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Selected node not found");
			return;
		}

		Dialogue::DialogueNode* node = it->second.node.get();

		ImGui::Text("Editing Node:");
		ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", m_selectedNodeId.c_str());
		ImGui::Separator();

		// Basic node info
		static char speakerBuffer[128];
		static char textBuffer[2048];

		if (node->GetSpeaker().has_value())
		{
			strncpy_s(speakerBuffer, node->GetSpeaker().value().c_str(), sizeof(speakerBuffer) - 1);
		}
		else
		{
			speakerBuffer[0] = '\0';
		}

		if (ImGui::InputText("Speaker", speakerBuffer, sizeof(speakerBuffer)))
		{
			if (speakerBuffer[0] != '\0')
			{
				node->SetSpeaker(speakerBuffer);
			}
			else
			{
				// Clear speaker
			}
			m_hasUnsavedChanges = true;
		}

		if (node->GetText().has_value())
		{
			strncpy_s(textBuffer, node->GetText().value().c_str(), sizeof(textBuffer) - 1);
		}
		else
		{
			textBuffer[0] = '\0';
		}

		if (ImGui::InputTextMultiline("Text", textBuffer, sizeof(textBuffer), ImVec2(-1, 100)))
		{
			node->SetText(textBuffer);
			m_hasUnsavedChanges = true;
		}

		// TODO: Implement full node editor with choices, commands, targets
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
						  "Full node editor coming soon - see DIALOGUE_EDITOR_README.md");
	}

	void DialogueEditorWindow::RenderPlaybackView(GameContext& context)
	{
		ImGui::Text("Dialogue Playback Test");
		ImGui::Separator();

		if (!m_isPlaybackActive)
		{
			ImGui::TextWrapped("Start playback to test dialogue flow.");

			if (!m_entryNodeId.empty() && ImGui::Button("Start from Entry Node"))
			{
				StartPlayback(context, m_entryNodeId);
			}

			if (!m_selectedNodeId.empty() && ImGui::Button("Start from Selected Node"))
			{
				StartPlayback(context, m_selectedNodeId);
			}

			return;
		}

		// Display current dialogue
		if (m_currentPlaybackResult.speaker.has_value())
		{
			ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), 
							  "%s:", m_currentPlaybackResult.speaker.value().c_str());
		}

		if (m_currentPlaybackResult.text.has_value())
		{
			ImGui::TextWrapped("%s", m_currentPlaybackResult.text.value().c_str());
		}

		ImGui::Separator();

		// Show choices
		for (const auto& choice : m_currentPlaybackResult.choices)
		{
			if (ImGui::Button(choice.text.c_str(), ImVec2(-1, 0)))
			{
				MakePlaybackChoice(context, choice.index);
			}
		}

		// Auto-advance
		if (m_currentPlaybackResult.shouldAutoAdvance)
		{
			if (ImGui::Button("Continue", ImVec2(-1, 0)))
			{
				ContinuePlayback(context);
			}
		}

		// End indicator
		if (m_currentPlaybackResult.hasEnded)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "[Dialogue Ended]");
		}

		ImGui::Separator();

		if (ImGui::Button("Stop Playback"))
		{
			StopPlayback(context);
		}

		// Show history
		if (ImGui::CollapsingHeader("History"))
		{
			for (const auto& nodeId : m_playbackHistory)
			{
				ImGui::BulletText("%s", nodeId.c_str());
			}
		}
	}

	void DialogueEditorWindow::RenderValidationPanel(GameContext& context)
	{
		ImGui::BeginTabBar("ValidationTabs");

		if (ImGui::BeginTabItem("Errors"))
		{
			if (m_errors.empty())
			{
				ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "No errors");
			}
			else
			{
				for (const auto& error : m_errors)
				{
					ImGui::PushID(&error);
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "❌");
					ImGui::SameLine();
					ImGui::TextWrapped("[%s] %s", error.nodeId.c_str(), error.message.c_str());

					if (!error.suggestion.empty())
					{
						ImGui::Indent();
						ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "💡 %s", error.suggestion.c_str());
						ImGui::Unindent();
					}

					if (!error.nodeId.empty() && ImGui::SmallButton("Go To Node"))
					{
						SelectNode(error.nodeId);
					}

					ImGui::Separator();
					ImGui::PopID();
				}
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Warnings"))
		{
			if (m_warnings.empty())
			{
				ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "No warnings");
			}
			else
			{
				for (const auto& warning : m_warnings)
				{
					ImGui::PushID(&warning);
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "⚠️");
					ImGui::SameLine();
					ImGui::TextWrapped("[%s] %s", warning.nodeId.c_str(), warning.message.c_str());

					if (!warning.nodeId.empty() && ImGui::SmallButton("Go To Node"))
					{
						SelectNode(warning.nodeId);
					}

					ImGui::Separator();
					ImGui::PopID();
				}
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}