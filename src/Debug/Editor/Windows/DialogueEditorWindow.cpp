#include "DialogueEditorWindow.h"

#include <fstream>
#include <sstream>
#include <imgui.h>

#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Debug/Editor/Exporters/DialogueExporter.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Assertions.h"

#define DIALOGUE_FILE_PATH "Scripts/Dialogue/"

namespace Struktur::Debug
{
	DialogueEditorWindow::DialogueEditorWindow(PreviewWindow* previewWindow)
		: EditorWindow("Dialogue Editor")
		, m_viewMode(ViewMode::Edit)
		, m_currentFile("")
		, m_currentClassName("")
		, m_hasUnsavedChanges(false)
		, m_nodes()
		, m_entryNodeId("")
		, m_selectedNodeId("")
		, m_graphPanOffset(0.0f, 0.0f)
		, m_graphZoom(1.0f)
		, m_isDraggingGraph(false)
		, m_graphDragStart(0.0f, 0.0f)
		, m_isPlaybackActive(false)
		, m_currentPlaybackResult()
		, m_playbackIntFlags()
		, m_playbackBoolFlags()
		, m_playbackInventory()
		, m_playbackHistory()
		, m_errors()
		, m_warnings()
		, m_previewWindow(previewWindow)
		, m_importCount(0u)
	{
		m_newNodeIdBuffer[0] = '\0';
		m_searchBuffer[0] = '\0';
	}

	void DialogueEditorWindow::Render(GameContext& context)
	{
		if (!m_isOpen)
			return;

		ImGui::SetNextWindowSize(ImVec2(1400, 900), ImGuiCond_FirstUseEver);
		ImGui::Begin(m_name.c_str(), &m_isOpen);

		// ========================================================================
		// TOOLBAR (replaces menu bar)
		// ========================================================================

		// Status and save button on same line
		if (m_hasUnsavedChanges)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "● Modified");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "○ Saved");
		}

		ImGui::SameLine();
		if (!m_currentFile.empty())
		{
			ImGui::Text("| %s", m_currentFile.c_str());
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "| No file loaded");
		}

		ImGui::SameLine();
		ImGui::Text("| Nodes: %zu", m_nodes.size());

		if (!m_errors.empty())
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "| Errors: %zu", m_errors.size());
		}
		if (!m_warnings.empty())
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "| Warnings: %zu", m_warnings.size());
		}

		// Toolbar buttons
		ImGui::Separator();

		// File operations
		if (ImGui::Button("📄 New"))
		{
			ImGui::OpenPopup("NewDialoguePopup");
		}
		ImGui::SameLine();

		if (ImGui::Button("📂 Open"))
		{
			ImGui::OpenPopup("OpenDialoguePopup");
		}
		ImGui::SameLine();

		if (ImGui::Button("💾 Save"))
		{
			if (!m_currentFile.empty())
			{
				SaveDialogueFile(m_currentFile);
			}
			else
			{
				ImGui::OpenPopup("SaveAsPopup");
			}
		}

		if (!m_hasUnsavedChanges)
		{
			ImGui::BeginDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("💾 Save As"))
		{
			ImGui::OpenPopup("SaveAsPopup");
		}

		if (!m_hasUnsavedChanges)
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		// Edit operations
		if (ImGui::Button("➕ Add Node"))
		{
			ImGui::OpenPopup("AddNodePopup");
		}

		if (m_selectedNodeId.empty())
		{
			ImGui::BeginDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("🗑️ Delete"))
		{
			if (!m_selectedNodeId.empty())
			{
				DeleteNode(m_selectedNodeId);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("📋 Duplicate"))
		{
			if (!m_selectedNodeId.empty())
			{
				DuplicateNode(m_selectedNodeId);
			}
		}

		if (m_selectedNodeId.empty())
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		// View operations
		if (ImGui::Button("✓ Validate"))
		{
			ValidateDialogue();
		}
		ImGui::SameLine();
		if (ImGui::Button("🔄 Auto Layout"))
		{
			CalculateGraphLayout();
		}

		ImGui::SameLine();
		ImGui::Separator();
		ImGui::SameLine();

		// Mode switch
		const char* modeText = (m_viewMode == ViewMode::Edit) ? "✏️ Edit Mode" : "▶️ Playback Mode";
		ImVec4 modeColor = (m_viewMode == ViewMode::Edit) ? ImVec4(0.3f, 0.8f, 1.0f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
		ImGui::TextColored(modeColor, "%s", modeText);

		if (m_nodes.empty())
		{
			ImGui::BeginDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("Switch Mode"))
		{
			if (m_viewMode == ViewMode::Edit)
			{
				m_viewMode = ViewMode::Playback;
			}
			else
			{
				m_viewMode = ViewMode::Edit;
				if (m_isPlaybackActive)
				{
					StopPlayback(context);
				}
			}
		}

		if (m_nodes.empty())
		{
			ImGui::EndDisabled();
		}

		// ========================================================================
		// POPUPS for toolbar actions
		// ========================================================================

		// New Dialogue Popup
		if (ImGui::BeginPopup("NewDialoguePopup"))
		{
			ImGui::Text("Create New Dialogue");
			ImGui::Separator();

			static char classNameBuffer[128] = "";
			ImGui::InputText("Class Name", classNameBuffer, sizeof(classNameBuffer));
			ImGui::TextWrapped("e.g., 'GregDialogue', 'MerchantDialogue'");

			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				if (classNameBuffer[0] != '\0')
				{
					CreateNewDialogue(classNameBuffer);
					classNameBuffer[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				classNameBuffer[0] = '\0';
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// Open Dialogue Popup
		if (ImGui::BeginPopup("OpenDialoguePopup"))
		{
			ImGui::Text("Load Dialogue File");
			ImGui::Separator();

			static char filepathBuffer[256] = DIALOGUE_FILE_PATH;
			ImGui::InputText("Filepath", filepathBuffer, sizeof(filepathBuffer));

			static char classNameBuffer[256] = "";
			ImGui::InputText("Class Name", classNameBuffer, sizeof(classNameBuffer));

			static char entryNodeBuffer[256] = "";
			ImGui::InputText("Entry Node", entryNodeBuffer, sizeof(entryNodeBuffer));

			if (ImGui::Button("Load", ImVec2(120, 0)))
			{
				LoadDialogueFile(context, filepathBuffer, classNameBuffer, entryNodeBuffer);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// Save As Popup
		if (ImGui::BeginPopup("SaveAsPopup"))
		{
			ImGui::Text("Save Dialogue As");
			ImGui::Separator();

			static char savePathBuffer[256] = DIALOGUE_FILE_PATH;
			ImGui::InputText("Filepath", savePathBuffer, sizeof(savePathBuffer));

			if (ImGui::Button("Save", ImVec2(120, 0)))
			{
				if (savePathBuffer[0] != '\0')
				{
					SaveDialogueFile(savePathBuffer);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// Add Node Popup
		if (ImGui::BeginPopup("AddNodePopup"))
		{
			ImGui::Text("Add New Node");
			ImGui::Separator();

			ImGui::InputText("Node ID", m_newNodeIdBuffer, sizeof(m_newNodeIdBuffer));

			if (ImGui::Button("Add", ImVec2(120, 0)))
			{
				if (m_newNodeIdBuffer[0] != '\0')
				{
					AddNode(m_newNodeIdBuffer);
					m_newNodeIdBuffer[0] = '\0';
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_newNodeIdBuffer[0] = '\0';
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Separator();

		// ========================================================================
		// MAIN CONTENT AREA
		// ========================================================================

		ImGui::BeginChild("MainContent", ImVec2(0, -150), false);

		// Three-panel layout: File panel | Graph/Playback view | Node editor
		const float filePanelWidth = 200.0f;
		const float nodeEditorWidth = 400.0f;

		// File panel
		ImGui::BeginChild("FilePanel", ImVec2(filePanelWidth, 0), true);
		RenderFilePanel(context);
		ImGui::EndChild();

		ImGui::SameLine();

		// Center view (graph or playback)
		const float centerWidth = ImGui::GetContentRegionAvail().x - nodeEditorWidth - 10.0f;
		ImGui::BeginChild("CenterView", ImVec2(centerWidth, 0), true);

		if (m_viewMode == ViewMode::Edit)
		{
			RenderGraphView(context);
		}
		else
		{
			RenderPlaybackView(context);
		}

		ImGui::EndChild();

		ImGui::SameLine();

		// Node editor panel
		ImGui::BeginChild("NodeEditor", ImVec2(nodeEditorWidth, 0), true);
		RenderNodeEditor(context);
		ImGui::EndChild();

		ImGui::EndChild();

		// Validation panel at bottom
		ImGui::Separator();
		ImGui::BeginChild("ValidationPanel", ImVec2(0, 0), false);
		RenderValidationPanel(context);
		ImGui::EndChild();

		ImGui::End();
	}

	void DialogueEditorWindow::RenderFilePanel(GameContext& context)
	{
		ImGui::Text("Dialogue Files");
		ImGui::Separator();

		if (ImGui::Button("Load Dialogue...", ImVec2(-1, 0)))
		{
			// TODO: Open file browser
			// For now, just show a text input
			ImGui::OpenPopup("LoadDialoguePopup");
		}

		if (ImGui::BeginPopup("LoadDialoguePopup"))
		{
			static char filepathBuffer[256] = DIALOGUE_FILE_PATH;
			ImGui::InputText("Filepath", filepathBuffer, sizeof(filepathBuffer));

			static char classNameBuffer[256] = "";
			ImGui::InputText("Class Name", classNameBuffer, sizeof(classNameBuffer));

			static char entryNodeBuffer[256] = "";
			ImGui::InputText("Entry Node", entryNodeBuffer, sizeof(entryNodeBuffer));

			if (ImGui::Button("Load"))
			{
				LoadDialogueFile(context, filepathBuffer, classNameBuffer, entryNodeBuffer);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (ImGui::Button("New Dialogue...", ImVec2(-1, 0)))
		{
			ImGui::OpenPopup("NewDialoguePopup");
		}

		if (ImGui::BeginPopup("NewDialoguePopup"))
		{
			static char classNameBuffer[128] = "";
			ImGui::InputText("Class Name", classNameBuffer, sizeof(classNameBuffer));
			ImGui::TextWrapped("e.g., 'GregDialogue', 'MerchantDialogue'");

			if (ImGui::Button("Create"))
			{
				CreateNewDialogue(classNameBuffer);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::Separator();

		// Current dialogue info
		if (!m_currentClassName.empty())
		{
			ImGui::Text("Current Dialogue:");
			ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", m_currentClassName.c_str());
			ImGui::Text("Entry Node:");
			if (!m_entryNodeId.empty())
			{
				ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "%s", m_entryNodeId.c_str());

				if (ImGui::SmallButton("Go To Entry"))
				{
					SelectNode(m_entryNodeId);
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Not set");
			}

			ImGui::Separator();
		}

		// Node list
		if (!m_nodes.empty())
		{
			ImGui::Text("Nodes (%zu):", m_nodes.size());

			// Search filter
			ImGui::InputText("##Search", m_searchBuffer, sizeof(m_searchBuffer));
			ImGui::SameLine();
			ImGui::Text("🔍");

			ImGui::BeginChild("NodeList", ImVec2(0, 0), false);

			for (const auto& [nodeId, nodeData] : m_nodes)
			{
				// Filter by search
				if (m_searchBuffer[0] != '\0' &&
					strstr(nodeId.c_str(), m_searchBuffer) == nullptr)
				{
					continue;
				}

				bool isSelected = (nodeId == m_selectedNodeId);
				bool isEntry = (nodeId == m_entryNodeId);

				ImVec4 color = isEntry ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, color);

				if (ImGui::Selectable(nodeId.c_str(), isSelected))
				{
					SelectNode(nodeId);
				}

				ImGui::PopStyleColor();

				// Context menu
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Set as Entry"))
					{
						m_entryNodeId = nodeId;
						m_hasUnsavedChanges = true;
					}
					if (ImGui::MenuItem("Duplicate"))
					{
						DuplicateNode(nodeId);
					}
					if (ImGui::MenuItem("Delete"))
					{
						DeleteNode(nodeId);
					}
					ImGui::EndPopup();
				}
			}

			ImGui::EndChild();
		}
		else
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No nodes loaded");
			ImGui::TextWrapped("Create a new dialogue or load an existing one to begin.");
		}
	}

	// Continue in next file section...
}
