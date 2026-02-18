#include "DialogueEditorWindow.h"

#include "Engine/GameContext.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Debug/Editor/Exporters/DialogueExporter.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Assertions.h"
#include <imgui.h>
#include <fstream>
#include <sstream>

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

		// Top toolbar
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					// TODO: Show new dialogue dialog
				}
				if (ImGui::MenuItem("Open", "Ctrl+O"))
				{
					// TODO: Show file browser
				}
				if (ImGui::MenuItem("Save", "Ctrl+S", false, m_hasUnsavedChanges))
				{
					if (!m_currentFile.empty())
					{
						SaveDialogueFile(m_currentFile);
					}
				}
				if (ImGui::MenuItem("Save As..."))
				{
					// TODO: Show save dialog
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Close"))
				{
					m_nodes.clear();
					m_currentFile = "";
					m_hasUnsavedChanges = false;
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Add Node", "Insert", false, !m_nodes.empty()))
				{
					ImGui::OpenPopup("AddNodePopup");
				}
				if (ImGui::MenuItem("Delete Node", "Delete", false, !m_selectedNodeId.empty()))
				{
					DeleteNode(m_selectedNodeId);
				}
				if (ImGui::MenuItem("Duplicate Node", "Ctrl+D", false, !m_selectedNodeId.empty()))
				{
					DuplicateNode(m_selectedNodeId);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Validate", "Ctrl+V"))
				{
					ValidateDialogue();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				bool isEdit = (m_viewMode == ViewMode::Edit);
				bool isPlayback = (m_viewMode == ViewMode::Playback);

				if (ImGui::MenuItem("Edit Mode", nullptr, isEdit))
				{
					m_viewMode = ViewMode::Edit;
					if (m_isPlaybackActive)
					{
						StopPlayback(context);
					}
				}
				if (ImGui::MenuItem("Playback Mode", nullptr, isPlayback, !m_nodes.empty()))
				{
					m_viewMode = ViewMode::Playback;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Reset Layout"))
				{
					CalculateGraphLayout();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// Status bar at top
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

		ImGui::Separator();

		// Main content area
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
			static char filepathBuffer[256] = "assets/dialogue/";
			ImGui::InputText("Filepath", filepathBuffer, sizeof(filepathBuffer));

			if (ImGui::Button("Load"))
			{
				LoadDialogueFile(context, filepathBuffer);
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
