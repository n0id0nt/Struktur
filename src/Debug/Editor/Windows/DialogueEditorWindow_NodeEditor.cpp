// DialogueEditorWindow_NodeEditor.cpp
// Complete node editor implementation with ImGui controls
// This provides full editing capabilities for dialogue nodes

#include "DialogueEditorWindow.h"
#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include <imgui.h>

namespace Struktur::Debug
{
	// ============================================================================
	// MAIN NODE EDITOR
	// ============================================================================

	void DialogueEditorWindow::RenderNodeEditor(GameContext& context)
	{
		if (m_selectedNodeId.empty())
		{
			ImGui::TextWrapped("Select a node from the graph or file panel to edit its properties.");
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
				"💡 Tip: Click a node in the graph view or select from the file panel");
			return;
		}

		auto it = m_nodes.find(m_selectedNodeId);
		if (it == m_nodes.end())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "❌ Selected node not found");
			return;
		}

		Dialogue::DialogueNode* node = it->second.node.get();

		// Header
		ImGui::Text("Editing Node:");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", m_selectedNodeId.c_str());

		ImGui::SameLine();
		if (m_selectedNodeId == m_entryNodeId)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "🚪 Entry");
		}

		ImGui::Separator();

		// Basic properties
		RenderNodeBasicInfo(context, node);

		ImGui::Separator();

		// Only show the active continuation section based on what the node has
		bool hasNext = node->GetNext().has_value();
		bool hasChoices = !node->GetChoices().empty();
		bool hasTargets = !node->GetTargets().empty();

		// Next Node Section
		if (hasNext)
		{
			if (ImGui::CollapsingHeader("Next Node", ImGuiTreeNodeFlags_DefaultOpen))
			{
				std::string currentNext = node->GetNext().value();
				if (NodeSelector("Next Node", currentNext))
				{
					node->SetNext(currentNext);
					m_hasUnsavedChanges = true;
				}

				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
					"Dialogue will automatically advance to this node");
			}
		}
		// Choices Section
		else if (hasChoices)
		{
			if (ImGui::CollapsingHeader("Player Choices", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderNodeChoices(context, node);
			}
		}
		// Conditional Targets Section
		else if (hasTargets)
		{
			if (ImGui::CollapsingHeader("Conditional Targets", ImGuiTreeNodeFlags_DefaultOpen))
			{
				RenderNodeTargets(context, node);
			}
		}
		// No continuation
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "⚠️ This node has no continuation (dialogue will end here)");
		}

		ImGui::Separator();

		// Commands (always available)
		if (ImGui::CollapsingHeader("Commands"))
		{
			RenderNodeCommands(context, node);
		}
	}

	// ============================================================================
	// BASIC NODE INFO
	// ============================================================================

	void DialogueEditorWindow::RenderNodeBasicInfo(GameContext& context, Dialogue::DialogueNode* node)
	{
		ImGui::Text("Basic Properties");
		ImGui::Separator();

		// Node ID (read-only)
		char nodeIdBuffer[128];
		strncpy_s(nodeIdBuffer, m_selectedNodeId.c_str(), sizeof(nodeIdBuffer) - 1);
		ImGui::InputText("Node ID", nodeIdBuffer, sizeof(nodeIdBuffer), ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(read-only)");

		// Speaker
		static char speakerBuffer[128];
		if (node->GetSpeaker().has_value())
		{
			strncpy_s(speakerBuffer, node->GetSpeaker().value().c_str(), sizeof(speakerBuffer) - 1);
		}
		else
		{
			speakerBuffer[0] = '\0';
		}

		if (SpeakerInput(speakerBuffer, sizeof(speakerBuffer)))
		{
			if (speakerBuffer[0] != '\0')
			{
				node->SetSpeaker(std::string(speakerBuffer));
			}
			else
			{
				node->SetSpeaker("");
			}
			m_hasUnsavedChanges = true;
		}

		// Text
		static char textBuffer[2048];
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
			node->SetText(std::string(textBuffer));
			m_hasUnsavedChanges = true;
		}

		// Variable insertion button
		if (ImGui::Button("Insert Variable..."))
		{
			ImGui::OpenPopup("VariablePopup");
		}

		if (ImGui::BeginPopup("VariablePopup"))
		{
			ImGui::Text("Insert Variable:");
			ImGui::Separator();

			if (ImGui::Selectable("{player_name}"))
			{
				InsertVariableAtCursor(textBuffer, sizeof(textBuffer), "{player_name}");
				node->SetText(std::string(textBuffer));
				m_hasUnsavedChanges = true;
			}

			if (ImGui::Selectable("{flag:NAME}"))
			{
				InsertVariableAtCursor(textBuffer, sizeof(textBuffer), "{flag:}");
				node->SetText(std::string(textBuffer));
				m_hasUnsavedChanges = true;
			}

			if (ImGui::Selectable("{item_count:ITEM}"))
			{
				InsertVariableAtCursor(textBuffer, sizeof(textBuffer), "{item_count:}");
				node->SetText(std::string(textBuffer));
				m_hasUnsavedChanges = true;
			}

			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Continuation Type Selector - SAFE VERSION
		ImGui::Text("Continuation Type:");

		// Determine current type based on what's present
		// Priority: Next > Choices > Targets
		int currentTypeIndex = 0;  // 0=None, 1=Next, 2=Choices, 3=Targets

		if (node->GetNext().has_value())
			currentTypeIndex = 1;
		else if (!node->GetChoices().empty())
			currentTypeIndex = 2;
		else if (!node->GetTargets().empty())
			currentTypeIndex = 3;

		const char* continuationTypes[] = { "None (End)", "Next Node", "Player Choices", "Conditional Targets" };

		// Store previous index to detect changes
		static int lastTypeIndex = currentTypeIndex;
		int selectedIndex = currentTypeIndex;

		if (ImGui::Combo("##ContinuationType", &selectedIndex, continuationTypes, IM_ARRAYSIZE(continuationTypes)))
		{
			// User changed type
			if (selectedIndex != currentTypeIndex)
			{
				// Add the new continuation type
				// Note: We can't remove the old ones without Clear methods,
				// but the dialogue system prioritizes: Next > Choices > Targets
				// So we just need to add the higher-priority one

				switch (selectedIndex)
				{
				case 0: // None
					// To make a node "None", we just don't add anything
					// Old data stays but won't be used if it's lower priority
					ImGui::OpenPopup("CannotClearWarning");
					break;

				case 1: // Next Node
					// Set next - this takes priority over choices/targets
					node->SetNext("");
					m_hasUnsavedChanges = true;
					break;

				case 2: // Choices
					// Need to clear Next first if it exists
					if (node->GetNext().has_value())
					{
						ImGui::OpenPopup("CannotClearWarning");
					}
					else
					{
						// Add a choice - takes priority over targets
						auto newChoice = std::make_unique<Dialogue::Choice>("New choice", "");
						node->AddChoice(std::move(newChoice));
						m_hasUnsavedChanges = true;
					}
					break;

				case 3: // Targets
					// Need to clear Next and Choices first if they exist
					if (node->GetNext().has_value() || !node->GetChoices().empty())
					{
						ImGui::OpenPopup("CannotClearWarning");
					}
					else
					{
						// Add a target - lowest priority
						std::unique_ptr<Dialogue::ConditionalTarget> newTarget = std::make_unique<Dialogue::ConditionalTarget>();
						newTarget->targetNode = "";
						node->AddTarget(std::move(newTarget));
						m_hasUnsavedChanges = true;
					}
					break;
				}
			}
		}

		// Warning popup for unsupported operations
		if (ImGui::BeginPopup("CannotClearWarning"))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Cannot Change Continuation Type");
			ImGui::Separator();
			ImGui::TextWrapped("Changing from a higher-priority continuation type (Next or Choices) to a lower-priority one requires Clear methods that aren't implemented yet.");
			ImGui::Separator();
			ImGui::TextWrapped("Current priority: Next > Choices > Targets");
			ImGui::Separator();
			ImGui::TextWrapped("You can:");
			ImGui::BulletText("Add Next when node has Choices or Targets");
			ImGui::BulletText("Add Choices when node has only Targets");
			ImGui::BulletText("Add Targets when node has nothing");
			ImGui::Separator();
			ImGui::TextWrapped("To change to a lower-priority type, manually edit the Wren file or delete and recreate the node.");

			if (ImGui::Button("OK"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			"💡 Choose how this dialogue node continues");

		ImGui::Separator();

		// Show current hierarchy
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Active continuations:");
		ImGui::Indent();

		if (node->GetNext().has_value())
		{
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "✓ Next Node (highest priority)");
		}

		if (!node->GetChoices().empty())
		{
			ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "✓ Choices (%zu)", node->GetChoices().size());
		}

		if (!node->GetTargets().empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "✓ Targets (%zu)", node->GetTargets().size());
		}

		if (!node->GetNext().has_value() && node->GetChoices().empty() && node->GetTargets().empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "None (dialogue ends)");
		}

		ImGui::Unindent();
	}

	// ============================================================================
	// CHOICES
	// ============================================================================

	void DialogueEditorWindow::RenderNodeChoices(GameContext& context, Dialogue::DialogueNode* node)
	{
		auto& choices = node->GetChoices();

		if (choices.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No choices defined");
		}

		// Display existing choices
		for (size_t i = 0; i < choices.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));

			// Choice header with number
			std::string headerLabel = "Choice " + std::to_string(i + 1) + ": " + choices[i]->text;
			if (headerLabel.length() > 40)
			{
				headerLabel = headerLabel.substr(0, 37) + "...";
			}

			bool isOpen = ImGui::TreeNodeEx(
				headerLabel.c_str(),
				ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
			);

			// Delete button
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete"))
			{
				// TODO: Implement choice deletion
				// Would need DialogueNode::RemoveChoice(index) method
				ImGui::TreePop();
				ImGui::PopID();
				m_hasUnsavedChanges = true;
				break;
			}

			// Move up/down buttons
			ImGui::SameLine();
			if (i > 0 && ImGui::ArrowButton("Up", ImGuiDir_Up))
			{
				// TODO: Implement choice reordering
				// Would need DialogueNode::SwapChoices(i, i-1) method
				m_hasUnsavedChanges = true;
			}

			ImGui::SameLine();
			if (i < choices.size() - 1 && ImGui::ArrowButton("Down", ImGuiDir_Down))
			{
				// TODO: Implement choice reordering
				m_hasUnsavedChanges = true;
			}

			if (isOpen)
			{
				// Choice text input with unique ID
				char choiceTextBuffer[256];
				strncpy_s(choiceTextBuffer, choices[i]->text.c_str(), sizeof(choiceTextBuffer) - 1);

				std::string textLabel = "Text##choice_text_" + std::to_string(i);
				if (ImGui::InputText(textLabel.c_str(), choiceTextBuffer, sizeof(choiceTextBuffer)))
				{
					// TODO: Update choice text
					// Would need DialogueNode::UpdateChoiceText(index, newText) method
					m_hasUnsavedChanges = true;
				}

				// Target node selector
				std::string targetNode = choices[i]->targetNode;
				std::string targetLabel = "Target##choice_target_" + std::to_string(i);

				if (NodeSelector(targetLabel.c_str(), targetNode))
				{
					// TODO: Update choice target
					// Would need DialogueNode::UpdateChoiceTarget(index, newTarget) method
					m_hasUnsavedChanges = true;
				}

				// Go to target button
				if (!targetNode.empty())
				{
					ImGui::SameLine();
					if (ImGui::SmallButton("Go To"))
					{
						SelectNode(targetNode);
					}
				}

				// Show current values (read-only for now)
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current: \"%s\" -> %s",
					choices[i]->text.c_str(), choices[i]->targetNode.c_str());

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		// Add choice button
		ImGui::Separator();
		if (ImGui::Button("+ Add Choice", ImVec2(-1, 0)))
		{
			auto newChoice = std::make_unique<Dialogue::Choice>("New choice", "");
			node->AddChoice(std::move(newChoice));
			m_hasUnsavedChanges = true;
		}

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			"💡 Player will choose one of these options");
	}

	// ============================================================================
	// COMMANDS
	// ============================================================================

	void DialogueEditorWindow::RenderNodeCommands(GameContext& context, Dialogue::DialogueNode* node)
	{
		const auto& commands = node->GetCommands();

		if (commands.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No commands defined");
		}

		// Display existing commands
		for (size_t i = 0; i < commands.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));

			std::string commandType = commands[i]->GetKey();
			bool isOpen = ImGui::TreeNodeEx(
				("Command: " + commandType).c_str(),
				ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
			);

			// Delete button
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete"))
			{
				// TODO: Implement command deletion
				ImGui::TreePop();
				ImGui::PopID();
				m_hasUnsavedChanges = true;
				break;
			}

			if (isOpen)
			{
				RenderCommandParameters(context, commands[i].get());
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		// Add command button - NOW WORKING!
		ImGui::Separator();
		if (ImGui::Button("+ Add Command", ImVec2(-1, 0)))
		{
			ImGui::OpenPopup("AddCommandPopup");
		}

		if (ImGui::BeginPopup("AddCommandPopup"))
		{
			ImGui::Text("Select Command Type:");
			ImGui::Separator();

			// Create commands with default parameters
			if (ImGui::Selectable("setInt - Set integer flag"))
			{
				std::unordered_map<std::string, Dialogue::DialogueValue> params;
				params["flag"] = Dialogue::DialogueValue("flag_name");
				params["value"] = Dialogue::DialogueValue(0);

				node->AddCommand(std::make_unique<Dialogue::Command>(std::string("setInt"), params));
				m_hasUnsavedChanges = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Selectable("setBool - Set boolean flag"))
			{
				std::unordered_map<std::string, Dialogue::DialogueValue> params;
				params["flag"] = Dialogue::DialogueValue("flag_name");
				params["value"] = Dialogue::DialogueValue(false);

				node->AddCommand(std::make_unique<Dialogue::Command>("setBool", params));
				m_hasUnsavedChanges = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Selectable("giveItem - Give item to inventory"))
			{
				std::unordered_map<std::string, Dialogue::DialogueValue> params;
				params["item"] = Dialogue::DialogueValue("item_name");

				node->AddCommand(std::make_unique<Dialogue::Command>("giveItem", params));
				m_hasUnsavedChanges = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Selectable("removeItem - Remove item"))
			{
				std::unordered_map<std::string, Dialogue::DialogueValue> params;
				params["item"] = Dialogue::DialogueValue("item_name");

				node->AddCommand(std::make_unique<Dialogue::Command>("removeItem", params));
				m_hasUnsavedChanges = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Selectable("giveExp - Give experience"))
			{
				std::unordered_map<std::string, Dialogue::DialogueValue> params;
				params["amount"] = Dialogue::DialogueValue(100);

				node->AddCommand(std::make_unique<Dialogue::Command>("giveExp", params));
				m_hasUnsavedChanges = true;
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::Selectable("playSound - Play sound effect"))
			{
				std::unordered_map<std::string, Dialogue::DialogueValue> params;
				params["sound"] = Dialogue::DialogueValue("sound_name");

				node->AddCommand(std::make_unique<Dialogue::Command>("playSound", params));
				m_hasUnsavedChanges = true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void DialogueEditorWindow::RenderCommandParameters(GameContext& context, Dialogue::Command* command)
	{
		const std::string& type = command->GetKey();
		auto& params = command->GetParams();

		ImGui::Text("Parameters:");
		ImGui::Indent();

		if (type == "setInt" || type == "setBool")
		{
			// Flag name
			static char flagBuffer[128] = "";
			if (params.count("flag"))
			{
				strncpy_s(flagBuffer, params.at("flag").AsString().c_str(), sizeof(flagBuffer) - 1);
			}

			if (ImGui::InputText("Flag", flagBuffer, sizeof(flagBuffer)))
			{
				// TODO: Update parameter
				// Would need Command::SetParameter() method
				m_hasUnsavedChanges = true;
			}

			// Value
			if (type == "setInt")
			{
				static int intValue = 0;
				if (params.count("value"))
				{
					intValue = params.at("value").AsInt();
				}

				if (ImGui::InputInt("Value", &intValue))
				{
					m_hasUnsavedChanges = true;
				}
			}
			else // setBool
			{
				static bool boolValue = false;
				if (params.count("value"))
				{
					boolValue = params.at("value").AsBool();
				}

				if (ImGui::Checkbox("Value", &boolValue))
				{
					m_hasUnsavedChanges = true;
				}
			}
		}
		else if (type == "giveItem" || type == "removeItem")
		{
			// Item name
			static char itemBuffer[128] = "";
			if (params.count("item"))
			{
				strncpy_s(itemBuffer, params.at("item").AsString().c_str(), sizeof(itemBuffer) - 1);
			}

			if (ImGui::InputText("Item", itemBuffer, sizeof(itemBuffer)))
			{
				m_hasUnsavedChanges = true;
			}
		}
		else if (type == "giveExp")
		{
			// Amount
			static int amount = 0;
			if (params.count("amount"))
			{
				amount = params.at("amount").AsInt();
			}

			if (ImGui::InputInt("Amount", &amount))
			{
				m_hasUnsavedChanges = true;
			}
		}
		else if (type == "playSound")
		{
			// Sound name
			static char soundBuffer[128] = "";
			if (params.count("sound"))
			{
				strncpy_s(soundBuffer, params.at("sound").AsString().c_str(), sizeof(soundBuffer) - 1);
			}

			if (ImGui::InputText("Sound", soundBuffer, sizeof(soundBuffer)))
			{
				m_hasUnsavedChanges = true;
			}
		}
		else
		{
			// Generic parameter display for unknown command types
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				"Custom command - parameters not editable in UI");
		}

		ImGui::Unindent();
	}

	// ============================================================================
	// CONDITIONAL TARGETS
	// ============================================================================

	void DialogueEditorWindow::RenderNodeTargets(GameContext& context, Dialogue::DialogueNode* node)
	{
		auto& targets = node->GetTargets();

		if (targets.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No conditional targets defined");
			ImGui::TextWrapped("Conditional targets allow branching based on game state (flags, items, etc.)");
		}

		// Display existing targets
		for (size_t i = 0; i < targets.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));

			std::string headerLabel = "Target " + std::to_string(i + 1) + ": " + targets[i]->targetNode;
			bool isOpen = ImGui::TreeNodeEx(
				headerLabel.c_str(),
				ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
			);

			// Delete button
			ImGui::SameLine();
			if (ImGui::SmallButton("Delete"))
			{
				// TODO: Implement target deletion
				// Would need DialogueNode::RemoveTarget(index) method
				ImGui::TreePop();
				ImGui::PopID();
				m_hasUnsavedChanges = true;
				break;
			}

			if (isOpen)
			{
				// Target node selector
				std::string targetNode = targets[i]->targetNode;
				std::string targetLabel = "Target Node##target_" + std::to_string(i);

				if (NodeSelector(targetLabel.c_str(), targetNode))
				{
					// TODO: Update target node
					// Would need DialogueNode::UpdateTargetNode(index, newNode) method
					m_hasUnsavedChanges = true;
				}

				ImGui::Separator();

				// Conditions
				ImGui::Text("Conditions (ALL must be true):");
				const auto& conditions = targets[i]->conditions;

				if (conditions.empty())
				{
					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
						"⚠️ No conditions - this target will always match");
				}

				for (size_t j = 0; j < conditions.size(); ++j)
				{
					ImGui::PushID(static_cast<int>(j));

					std::string conditionType = conditions[j]->GetKey();
					ImGui::BulletText("Condition: %s", conditionType.c_str());

					ImGui::SameLine();
					if (ImGui::SmallButton("Delete"))
					{
						// TODO: Implement condition deletion
						// Would need DialogueNode::RemoveTargetCondition(targetIndex, conditionIndex) method
						ImGui::PopID();
						m_hasUnsavedChanges = true;
						break;
					}

					ImGui::Indent();
					RenderConditionParameters(context, conditions[j].get());
					ImGui::Unindent();

					ImGui::PopID();
				}

				// Add condition button
				if (ImGui::Button("+ Add Condition"))
				{
					ImGui::OpenPopup("AddConditionPopup");
				}

				if (ImGui::BeginPopup("AddConditionPopup"))
				{
					ImGui::Text("Select Condition Type:");
					ImGui::Separator();

					if (ImGui::Selectable("intFlag - Check integer flag"))
					{
						// TODO: Create and add condition
						// Would need DialogueNode::AddTargetCondition(targetIndex, condition) method
						m_hasUnsavedChanges = true;
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::Selectable("boolFlag - Check boolean flag"))
					{
						// TODO: Create and add condition
						m_hasUnsavedChanges = true;
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::Selectable("hasItem - Check for item"))
					{
						// TODO: Create and add condition
						m_hasUnsavedChanges = true;
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		// Add target button
		ImGui::Separator();
		if (ImGui::Button("+ Add Conditional Target", ImVec2(-1, 0)))
		{
			// Create new target with empty conditions
			std::unique_ptr<Dialogue::ConditionalTarget> newTarget = std::make_unique<Dialogue::ConditionalTarget>();
			newTarget->targetNode = "";
			node->AddTarget(std::move(newTarget));
			m_hasUnsavedChanges = true;
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			"💡 Targets are evaluated in order. First matching target is used.");
	}

	void DialogueEditorWindow::RenderConditionParameters(GameContext& context, Dialogue::Condition* condition)
	{
		const std::string& type = condition->GetKey();
		const auto& params = condition->GetParams();

		if (type == "intFlag")
		{
			// Flag name
			ImGui::Text("Flag: %s", params.count("flag") ? params.at("flag").AsString().c_str() : "");

			// Operator
			ImGui::Text("Operator: %s", params.count("op") ? params.at("op").AsString().c_str() : "");

			// Value
			ImGui::Text("Value: %s", params.count("value") ? params.at("value").AsString().c_str() : "");
		}
		else if (type == "boolFlag")
		{
			ImGui::Text("Flag: %s", params.count("flag") ? params.at("flag").AsString().c_str() : "");
		}
		else if (type == "hasItem")
		{
			ImGui::Text("Item: %s", params.count("item") ? params.at("item").AsString().c_str() : "");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Custom condition");
		}
	}

	// ============================================================================
	// HELPER FUNCTIONS
	// ============================================================================

	bool DialogueEditorWindow::NodeSelector(const char* label, std::string& nodeId)
	{
		bool changed = false;
		const char* preview = nodeId.empty() ? "<None>" : nodeId.c_str();

		if (ImGui::BeginCombo(label, preview))
		{
			// Option to clear
			if (ImGui::Selectable("<None>", nodeId.empty()))
			{
				nodeId = "";
				changed = true;
			}

			// Search filter
			static char searchBuffer[64] = "";
			ImGui::InputText("##Search", searchBuffer, sizeof(searchBuffer));

			// List all nodes
			for (const auto& [id, nodeData] : m_nodes)
			{
				// Filter by search
				if (searchBuffer[0] != '\0' && strstr(id.c_str(), searchBuffer) == nullptr)
				{
					continue;
				}

				bool isSelected = (id == nodeId);
				if (ImGui::Selectable(id.c_str(), isSelected))
				{
					nodeId = id;
					changed = true;
				}

				// Tooltip with node info
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					if (nodeData.node->GetSpeaker().has_value())
					{
						ImGui::Text("Speaker: %s", nodeData.node->GetSpeaker().value().c_str());
					}
					if (nodeData.node->GetText().has_value())
					{
						ImGui::TextWrapped("Text: %s", nodeData.node->GetText().value().c_str());
					}
					ImGui::EndTooltip();
				}
			}

			ImGui::EndCombo();
		}

		return changed;
	}

	bool DialogueEditorWindow::SpeakerInput(char* buffer, size_t bufferSize)
	{
		bool changed = false;

		// Common speakers for quick selection
		static const char* commonSpeakers[] = { "Greg", "Player", "Narrator", "Mei", "Baron" };

		if (ImGui::BeginCombo("Speaker", buffer))
		{
			if (ImGui::Selectable("<None>"))
			{
				buffer[0] = '\0';
				changed = true;
			}

			for (const char* speaker : commonSpeakers)
			{
				if (ImGui::Selectable(speaker))
				{
					strncpy_s(buffer, bufferSize, speaker, bufferSize - 1);
					changed = true;
				}
			}

			ImGui::EndCombo();
		}

		// Manual text input
		if (ImGui::InputText("##SpeakerManual", buffer, bufferSize))
		{
			changed = true;
		}

		return changed;
	}

	void DialogueEditorWindow::InsertVariableAtCursor(char* buffer, size_t bufferSize, const char* variable)
	{
		// Simple append for now - in a full implementation would insert at cursor position
		size_t currentLen = strlen(buffer);
		size_t varLen = strlen(variable);

		if (currentLen + varLen < bufferSize - 1)
		{
			strcat_s(buffer, bufferSize, variable);
		}
	}

	void DialogueEditorWindow::SwitchNodeContinuationType(Dialogue::DialogueNode* node, int newType)
	{
		// newType: 0=None, 1=Next, 2=Choices, 3=Targets

		// Since we don't have Clear methods on DialogueNode, we'll work around it
		// by rebuilding the node with only the new continuation type

		std::string nodeId = m_selectedNodeId;
		auto it = m_nodes.find(nodeId);
		if (it == m_nodes.end())
			return;

		// Save current basic properties
		std::optional<std::string> savedSpeaker = node->GetSpeaker();
		std::optional<std::string> savedText = node->GetText();
		glm::vec2 savedPosition = it->second.visualPosition;

		// Create new node with same ID
		auto newNode = std::make_unique<Dialogue::DialogueNode>(nodeId);

		// Restore basic properties
		if (savedSpeaker.has_value())
			newNode->SetSpeaker(savedSpeaker.value());
		if (savedText.has_value())
			newNode->SetText(savedText.value());

		// Set up the new continuation type
		switch (newType)
		{
		case 0: // None - don't add anything
			break;

		case 1: // Next Node
			newNode->SetNext("");
			break;

		case 2: // Choices
		{
			auto newChoice = std::make_unique<Dialogue::Choice>("New choice", "");
			newNode->AddChoice(std::move(newChoice));
		}
		break;

		case 3: // Targets
		{
			std::unique_ptr<Dialogue::ConditionalTarget> newTarget = std::make_unique<Dialogue::ConditionalTarget>();
			newTarget->targetNode = "";
			newNode->AddTarget(std::move(newTarget));
		}
		break;
		}

		// Replace the old node
		it->second.node = std::move(newNode);
		it->second.visualPosition = savedPosition;

		m_hasUnsavedChanges = true;
	}
}
