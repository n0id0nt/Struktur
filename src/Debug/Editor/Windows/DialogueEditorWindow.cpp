// DialogueEditorWindow.cpp
// Main dialogue editor window with node graph, editor panels, and playback testing
// Part of the Struktur editor

#include "DialogueEditorWindow.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include "Debug/Assertions.h"
#include "Debug/Editor/Exporters/DialogueExporter.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/Dialogue/DialogueHelperFunctions.h"
#include "Engine/Dialogue/DialogueManager.h"
#include "Engine/Dialogue/DialogueRegistry.h"
#include "Engine/GameContext.h"

#define DIALOGUE_FILE_PATH "Scripts/Dialogue/"

namespace Struktur::Debug
{
DialogueEditorWindow::DialogueEditorWindow(PreviewWindow* previewWindow)
    : EditorWindow("Dialogue Editor"),
      m_viewMode(ViewMode::Edit),
      m_currentFile(""),
      m_currentClassName(""),
      m_hasUnsavedChanges(false),
      m_currentSaveFormat(Dialogue::DialogueExporter::DialogueSaveFormat::Wren),
      m_nodes(),
      m_entryNodeId(""),
      m_selectedNodeId(""),
      m_graphPanOffset(0.0f, 0.0f),
      m_graphZoom(1.0f),
      m_isDraggingGraph(false),
      m_graphDragStart(0.0f, 0.0f),
      m_isPlaybackActive(false),
      m_currentPlaybackResult(),
      m_playbackIntFlags(),
      m_playbackBoolFlags(),
      m_playbackInventory(),
      m_playbackHistory(),
      m_errors(),
      m_warnings(),
      m_previewWindow(previewWindow),
      m_importCount(0u)
{
	m_newNodeIdBuffer[0] = '\0';
	m_searchBuffer[0]    = '\0';
}

void DialogueEditorWindow::Render(GameContext& context)
{
	if (!m_isOpen)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(1400, 900), ImGuiCond_FirstUseEver);
	ImGui::Begin(m_name.c_str(), &m_isOpen);

	// ========================================================================
	// TOOLBAR
	// ========================================================================

	if (m_hasUnsavedChanges)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "* Modified");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "o Saved");
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

	// Format indicator
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "| Format: %s",
	                   m_currentSaveFormat == Dialogue::DialogueExporter::DialogueSaveFormat::Json ? "JSON" : "Wren");

	ImGui::Separator();

	// File operations
	if (ImGui::Button("New"))
	{
		ImGui::OpenPopup("NewDialoguePopup");
	}
	ImGui::SameLine();

	if (ImGui::Button("Open"))
	{
		OpenFileWithPicker(context);
	}
	RenderOpenDialogueOptionsPopup(context);
	ImGui::SameLine();

	bool saveDisabled = !m_hasUnsavedChanges;
	if (saveDisabled)
	{
		ImGui::BeginDisabled();
	}

	if (ImGui::Button("Save"))
	{
		if (!m_currentFile.empty())
		{
			SaveDialogueFile(m_currentFile, m_currentSaveFormat);
		}
		else
		{
			SaveFileWithPicker();
		}
	}

	if (saveDisabled)
	{
		ImGui::EndDisabled();
	}

	if (m_nodes.empty())
	{
		ImGui::BeginDisabled();
	}

	ImGui::SameLine();
	if (ImGui::Button("Save As"))
	{
		SaveFileWithPicker();
	}

	if (m_nodes.empty())
	{
		ImGui::EndDisabled();
	}

	// Format selector
	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();
	ImGui::Text("Format:");
	ImGui::SameLine();

	int formatIndex = (m_currentSaveFormat == Dialogue::DialogueExporter::DialogueSaveFormat::Json) ? 1 : 0;
	ImGui::SetNextItemWidth(80.0f);
	if (ImGui::Combo("##Format", &formatIndex, "Wren\0JSON\0"))
	{
		m_currentSaveFormat = (formatIndex == 1) ? Dialogue::DialogueExporter::DialogueSaveFormat::Json
		                                         : Dialogue::DialogueExporter::DialogueSaveFormat::Wren;
	}

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	// Edit operations
	if (ImGui::Button("Add Node"))
	{
		ImGui::OpenPopup("AddNodePopup");
	}

	if (m_selectedNodeId.empty())
	{
		ImGui::BeginDisabled();
	}

	ImGui::SameLine();
	if (ImGui::Button("Delete"))
	{
		if (!m_selectedNodeId.empty())
		{
			DeleteNode(m_selectedNodeId);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Duplicate"))
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
	if (ImGui::Button("Validate"))
	{
		ValidateDialogue();
	}
	ImGui::SameLine();
	if (ImGui::Button("Auto Layout"))
	{
		CalculateGraphLayout();
	}

	ImGui::SameLine();
	ImGui::Separator();
	ImGui::SameLine();

	// Mode switch
	const char* modeText = (m_viewMode == ViewMode::Edit) ? "Edit Mode" : "Playback Mode";
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
	// POPUPS
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

	const float filePanelWidth  = 200.0f;
	const float nodeEditorWidth = 400.0f;

	ImGui::BeginChild("FilePanel", ImVec2(filePanelWidth, 0), true);
	RenderFilePanel(context);
	ImGui::EndChild();

	ImGui::SameLine();

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

	ImGui::BeginChild("NodeEditor", ImVec2(nodeEditorWidth, 0), true);
	RenderNodeEditor(context);
	ImGui::EndChild();

	ImGui::EndChild();

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
		OpenFileWithPicker(context);
	}

	RenderOpenDialogueOptionsPopup(context);

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

	if (!m_nodes.empty())
	{
		ImGui::Text("Nodes (%zu):", m_nodes.size());

		ImGui::InputText("##Search", m_searchBuffer, sizeof(m_searchBuffer));
		ImGui::SameLine();
		ImGui::Text("[S]");

		ImGui::BeginChild("NodeList", ImVec2(0, 0), false);

		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			if (m_searchBuffer[0] != '\0' && strstr(nodeId.c_str(), m_searchBuffer) == nullptr)
			{
				continue;
			}

			bool isSelected = (nodeId == m_selectedNodeId);
			bool isEntry    = (nodeId == m_entryNodeId);

			ImVec4 color = isEntry ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);

			if (ImGui::Selectable(nodeId.c_str(), isSelected))
			{
				SelectNode(nodeId);
			}

			ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Set as Entry"))
				{
					m_entryNodeId       = nodeId;
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

void DialogueEditorWindow::SaveFileWithPicker()
{
	bool useJson = (m_currentSaveFormat == Dialogue::DialogueExporter::DialogueSaveFormat::Json);

	// Build a default filename from the current class name if we have one
	std::string defaultPath = DIALOGUE_FILE_PATH;
	if (!m_currentClassName.empty())
	{
		defaultPath += m_currentClassName;
		defaultPath += useJson ? ".json" : ".wren";
	}

	std::string filepath =
	    FileSystem::SaveFileDialog("Save Dialogue File", defaultPath,
	                               useJson ? std::vector<std::string>{"*.json"} : std::vector<std::string>{"*.wren"},
	                               useJson ? "JSON Dialogue (*.json)" : "Wren Dialogue (*.wren)");

	if (!filepath.empty())
	{
		// Infer format from extension in case the user changed it manually
		Dialogue::DialogueExporter::DialogueSaveFormat format = FormatFromExtension(filepath);
		SaveDialogueFile(filepath, format);
	}
}

void DialogueEditorWindow::OpenFileWithPicker(GameContext& context)
{
	std::string filepath = FileSystem::OpenFileDialog("Open Dialogue File", DIALOGUE_FILE_PATH, {"*.wren", "*.json"},
	                                                  "Dialogue Files (*.wren, *.json)");

	if (filepath.empty())
	{
		return;
	}

	// Derive class name from filename as the default
	std::string derivedClassName = "";
	size_t slashPos              = filepath.find_last_of("/\\");
	std::string filename         = (slashPos != std::string::npos) ? filepath.substr(slashPos + 1) : filepath;
	size_t dotPos                = filename.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		derivedClassName = filename.substr(0, dotPos);
	}

	// Store pending load state so the popup can access it
	m_pendingLoadFilepath  = filepath;
	m_pendingLoadClassName = derivedClassName;
	m_pendingLoadEntryNode = "";

	ImGui::OpenPopup("OpenDialogueOptionsPopup");
}

void DialogueEditorWindow::RenderOpenDialogueOptionsPopup(GameContext& context)
{
	if (ImGui::BeginPopup("OpenDialogueOptionsPopup"))
	{
		ImGui::Text("Opening: %s", m_pendingLoadFilepath.c_str());
		ImGui::Separator();
		ImGui::Spacing();

		// Class name - pre-filled with derived value, user can override
		static char classNameBuf[128];
		static char entryNodeBuf[128];
		static bool initialised = false;
		if (!initialised)
		{
			strncpy_s(classNameBuf, m_pendingLoadClassName.c_str(), sizeof(classNameBuf) - 1);
			strncpy_s(entryNodeBuf, m_pendingLoadEntryNode.c_str(), sizeof(entryNodeBuf) - 1);
			initialised = true;
		}

		ImGui::Text("Class Name");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##ClassName", classNameBuf, sizeof(classNameBuf));
		ImGui::TextDisabled("Derived from filename: %s", m_pendingLoadClassName.c_str());

		ImGui::Spacing();

		ImGui::Text("Entry Node");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##EntryNode", entryNodeBuf, sizeof(entryNodeBuf));
		ImGui::TextDisabled("Leave blank to use first node or detect automatically");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("Load", ImVec2(120, 0)))
		{
			LoadDialogueFile(context, m_pendingLoadFilepath, std::string(classNameBuf), std::string(entryNodeBuf));
			initialised = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			initialised = false;
			m_pendingLoadFilepath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

// ============================================================================
// GRAPH VIEW
// ============================================================================

void DialogueEditorWindow::RenderGraphView(GameContext& context)
{
	ImGui::Text("Dialogue Graph");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
	                   "(Pan: Middle Mouse, Zoom: Scroll, Right-Click: Add Node, Drag: Move Nodes)");

	if (m_nodes.empty())
	{
		ImGui::TextWrapped("No nodes to display. Create or load a dialogue to see the graph.");

		// Allow creating first node even when empty
		if (ImGui::Button("Create Entry Node"))
		{
			ImGui::OpenPopup("CreateFirstNode");
		}

		if (ImGui::BeginPopup("CreateFirstNode"))
		{
			static char firstNodeId[128] = "entry";
			ImGui::InputText("Node ID", firstNodeId, sizeof(firstNodeId));

			if (ImGui::Button("Create"))
			{
				AddNode(firstNodeId);
				m_entryNodeId = firstNodeId;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		return;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 canvasPos     = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize    = ImGui::GetContentRegionAvail();

	// Create invisible button for the entire canvas to capture input
	ImGui::InvisibleButton("canvas", canvasSize,
	                       ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
	                           ImGuiButtonFlags_MouseButtonMiddle);
	bool isCanvasHovered = ImGui::IsItemHovered();

	// Handle pan and zoom
	if (isCanvasHovered)
	{
		// Pan with middle mouse button (only if not dragging a node)
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
		{
			ImVec2 delta = ImGui::GetIO().MouseDelta;
			m_graphPanOffset.x += delta.x;
			m_graphPanOffset.y += delta.y;
		}

		// Zoom with mouse wheel
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel != 0.0f)
		{
			m_graphZoom += wheel * 0.1f;
			m_graphZoom = glm::clamp(m_graphZoom, 0.25f, 2.0f);
		}

		// Right-click context menu for adding nodes
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("GraphContextMenu");
		}
	}

	// Context menu for graph
	if (ImGui::BeginPopup("GraphContextMenu"))
	{
		ImGui::Text("Add New Node");
		ImGui::Separator();

		static char newNodeIdBuffer[128] = "";
		ImGui::InputText("Node ID", newNodeIdBuffer, sizeof(newNodeIdBuffer));

		if (ImGui::Button("Create Node"))
		{
			if (newNodeIdBuffer[0] != '\0')
			{
				// Get mouse position in graph space
				ImVec2 mousePos = ImGui::GetMousePosOnOpeningCurrentPopup();
				float graphX    = (mousePos.x - canvasPos.x - m_graphPanOffset.x) / m_graphZoom;
				float graphY    = (mousePos.y - canvasPos.y - m_graphPanOffset.y) / m_graphZoom;

				AddNode(newNodeIdBuffer);

				// Set position to where user clicked
				auto it = m_nodes.find(newNodeIdBuffer);
				if (it != m_nodes.end())
				{
					it->second.visualPosition = glm::vec2(graphX, graphY);
				}

				newNodeIdBuffer[0] = '\0';
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Draw grid
	const float gridStep = 64.0f * m_graphZoom;
	ImU32 gridColor      = IM_COL32(50, 50, 50, 255);

	for (float x = fmodf(m_graphPanOffset.x, gridStep); x < canvasSize.x; x += gridStep)
	{
		drawList->AddLine(ImVec2(canvasPos.x + x, canvasPos.y), ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
		                  gridColor);
	}

	for (float y = fmodf(m_graphPanOffset.y, gridStep); y < canvasSize.y; y += gridStep)
	{
		drawList->AddLine(ImVec2(canvasPos.x, canvasPos.y + y), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
		                  gridColor);
	}

	// Render connections first (behind nodes)
	RenderNodeConnections(context, canvasPos);

	// Track which node is being dragged
	static std::string draggedNodeId = "";
	static glm::vec2 dragStartPos(0.0f, 0.0f);
	static glm::vec2 nodeStartPos(0.0f, 0.0f);

	// Render nodes
	for (const auto& [nodeId, nodeData] : m_nodes)
	{
		ImVec2 screenPos = ImVec2(canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
		                          canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y);

		RenderNode(context, nodeId, screenPos);
	}

	// Handle node dragging
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && isCanvasHovered)
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		// Check if clicked on a node
		bool clickedNode = false;
		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			ImVec2 screenPos = ImVec2(canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
			                          canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y);

			const float nodeWidth  = 200.0f * m_graphZoom;
			const float nodeHeight = 100.0f * m_graphZoom;

			if (mousePos.x >= screenPos.x && mousePos.x <= screenPos.x + nodeWidth && mousePos.y >= screenPos.y &&
			    mousePos.y <= screenPos.y + nodeHeight)
			{
				SelectNode(nodeId);
				clickedNode = true;

				// Start dragging this node
				draggedNodeId = nodeId;
				dragStartPos  = glm::vec2(mousePos.x, mousePos.y);
				nodeStartPos  = nodeData.visualPosition;

				break;
			}
		}

		if (!clickedNode)
		{
			m_selectedNodeId = "";
			draggedNodeId    = "";
		}
	}

	// Update dragged node position
	if (!draggedNodeId.empty() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		glm::vec2 currentMousePos(mousePos.x, mousePos.y);
		glm::vec2 delta = (currentMousePos - dragStartPos) / m_graphZoom;

		auto it = m_nodes.find(draggedNodeId);
		if (it != m_nodes.end())
		{
			it->second.visualPosition = nodeStartPos + delta;
			m_hasUnsavedChanges       = true;
		}
	}

	// Stop dragging
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		draggedNodeId = "";
	}
}

void DialogueEditorWindow::RenderNode(GameContext& context, const std::string& nodeId, ImVec2 position)
{
	auto it = m_nodes.find(nodeId);
	if (it == m_nodes.end())
	{
		return;
	}

	const Dialogue::DialogueNode* node = it->second.node.get();
	ImDrawList* drawList               = ImGui::GetWindowDrawList();

	const float nodeWidth  = 200.0f * m_graphZoom;
	const float nodeHeight = 100.0f * m_graphZoom;
	const float fontSize   = 14.0f * m_graphZoom;

	// Determine node color based on type
	ImU32 bgColor;
	if (nodeId == m_entryNodeId)
	{
		bgColor = IM_COL32(70, 70, 70, 255);  // Gray for entry
	}
	else if (!node->GetChoices().empty())
	{
		bgColor = IM_COL32(50, 100, 200, 255);  // Blue for choices
	}
	else if (!node->GetTargets().empty())
	{
		bgColor = IM_COL32(200, 150, 50, 255);  // Yellow for conditionals
	}
	else if (node->GetNext().has_value())
	{
		bgColor = IM_COL32(50, 150, 50, 255);  // Green for next
	}
	else
	{
		bgColor = IM_COL32(150, 50, 50, 255);  // Red for dead end
	}

	// Highlight if selected
	if (nodeId == m_selectedNodeId)
	{
		drawList->AddRectFilled(ImVec2(position.x - 3, position.y - 3),
		                        ImVec2(position.x + nodeWidth + 3, position.y + nodeHeight + 3),
		                        IM_COL32(255, 255, 0, 255), 5.0f);
	}

	// Draw node background
	drawList->AddRectFilled(position, ImVec2(position.x + nodeWidth, position.y + nodeHeight), bgColor, 5.0f);

	// Draw node border
	drawList->AddRect(position, ImVec2(position.x + nodeWidth, position.y + nodeHeight), IM_COL32(200, 200, 200, 255),
	                  5.0f, 0, 2.0f);

	// Draw node ID (title)
	ImVec2 textPos = ImVec2(position.x + 5, position.y + 5);
	drawList->AddText(ImGui::GetFont(), fontSize, textPos, IM_COL32(255, 255, 255, 255), nodeId.c_str());

	// Draw speaker
	if (node->GetSpeaker().has_value())
	{
		textPos.y += fontSize + 2;
		std::string speakerText = node->GetSpeaker().value();
		drawList->AddText(ImGui::GetFont(), fontSize * 0.85f, textPos, IM_COL32(180, 220, 255, 255),
		                  speakerText.c_str());
	}

	// Draw text preview (first 30 chars)
	if (node->GetText().has_value())
	{
		textPos.y += fontSize;
		std::string textPreview = node->GetText().value();
		if (textPreview.length() > 30)
		{
			textPreview = textPreview.substr(0, 27) + "...";
		}

		drawList->AddText(ImGui::GetFont(), fontSize * 0.75f, textPos, IM_COL32(200, 200, 200, 255),
		                  textPreview.c_str());
	}

	// Draw node info at bottom
	textPos.y = position.y + nodeHeight - fontSize - 5;

	if (node->HasNext())
	{
		drawList->AddText(ImGui::GetFont(), fontSize * 0.7f, textPos, IM_COL32(100, 255, 100, 255),
		                  ("-> " + node->GetNext().value()).c_str());
	}
	else if (!node->GetChoices().empty())
	{
		std::string choicesText = "Choices: " + std::to_string(node->GetChoices().size());
		drawList->AddText(ImGui::GetFont(), fontSize * 0.7f, textPos, IM_COL32(100, 150, 255, 255),
		                  choicesText.c_str());
	}

	if (!node->GetCommands().empty())
	{
		textPos.x           = position.x + nodeWidth - 60;
		std::string cmdText = "C:" + std::to_string(node->GetCommands().size());
		drawList->AddText(ImGui::GetFont(), fontSize * 0.7f, textPos, IM_COL32(255, 200, 100, 255), cmdText.c_str());
	}

	// ========================================================================
	// CONTINUATION DETAILS BOX (below main node)
	// ========================================================================
	const float detailsYOffset  = 5.0f * m_graphZoom;
	ImVec2 detailsPos           = ImVec2(position.x, position.y + nodeHeight + detailsYOffset);
	const float detailsWidth    = nodeWidth;
	const float detailsFontSize = fontSize * 0.7f;
	const float lineHeight      = detailsFontSize + 2.0f;

	// Next Node Details
	if (node->HasNext())
	{
		const std::string& nextId = node->GetNext().value();
		float boxHeight           = lineHeight + 10.0f;

		// Background
		drawList->AddRectFilled(detailsPos, ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
		                        IM_COL32(40, 60, 40, 220),  // Dark green
		                        3.0f);

		// Border
		drawList->AddRect(detailsPos, ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
		                  IM_COL32(100, 255, 100, 200), 3.0f, 0, 1.5f);

		// Text
		ImVec2 textPos       = ImVec2(detailsPos.x + 5, detailsPos.y + 5);
		std::string nextText = "-> " + nextId;
		drawList->AddText(ImGui::GetFont(), detailsFontSize, textPos, IM_COL32(150, 255, 150, 255), nextText.c_str());
	}
	// Choices Details
	else if (!node->GetChoices().empty())
	{
		const auto& choices = node->GetChoices();
		float boxHeight     = (choices.size() * lineHeight) + 10.0f;

		// Background
		drawList->AddRectFilled(detailsPos, ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
		                        IM_COL32(40, 50, 80, 220),  // Dark blue
		                        3.0f);

		// Border
		drawList->AddRect(detailsPos, ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
		                  IM_COL32(100, 150, 255, 200), 3.0f, 0, 1.5f);

		// List choices
		for (size_t i = 0; i < choices.size(); ++i)
		{
			ImVec2 choicePos = ImVec2(detailsPos.x + 5, detailsPos.y + 5 + (i * lineHeight));

			std::string choiceText = "- " + choices[i]->text;
			if (choiceText.length() > 25)
			{
				choiceText = choiceText.substr(0, 22) + "...";
			}
			choiceText += " -> " + choices[i]->targetNode;

			// Truncate target if combined text is too long
			if (choiceText.length() > 35)
			{
				choiceText = choiceText.substr(0, 32) + "...";
			}

			drawList->AddText(ImGui::GetFont(), detailsFontSize, choicePos, IM_COL32(180, 200, 255, 255),
			                  choiceText.c_str());
		}
	}
	// Conditional Targets Details
	else if (!node->GetTargets().empty())
	{
		const auto& targets = node->GetTargets();

		// Calculate height (target + conditions)
		int totalLines = 0;
		for (const auto& target : targets)
		{
			totalLines += 1;                                            // Target line
			totalLines += static_cast<int>(target->conditions.size());  // Condition lines
		}
		float boxHeight = (totalLines * lineHeight) + 10.0f;

		// Background
		drawList->AddRectFilled(detailsPos, ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
		                        IM_COL32(80, 60, 40, 220),  // Dark yellow/orange
		                        3.0f);

		// Border
		drawList->AddRect(detailsPos, ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
		                  IM_COL32(255, 200, 100, 200), 3.0f, 0, 1.5f);

		// List targets and conditions
		int lineNum = 0;
		for (size_t i = 0; i < targets.size(); ++i)
		{
			const auto& target = targets[i];

			// Target node
			ImVec2 targetPos = ImVec2(detailsPos.x + 5, detailsPos.y + 5 + (lineNum * lineHeight));

			std::string targetText = "-> " + target->targetNode;
			if (targetText.length() > 30)
			{
				targetText = targetText.substr(0, 27) + "...";
			}

			drawList->AddText(ImGui::GetFont(), detailsFontSize, targetPos, IM_COL32(255, 220, 150, 255),
			                  targetText.c_str());
			lineNum++;

			// Conditions (indented)
			for (const auto& condition : target->conditions)
			{
				ImVec2 condPos = ImVec2(detailsPos.x + 15,  // Indent
				                        detailsPos.y + 5 + (lineNum * lineHeight));

				// Format condition: "type: param=value"
				std::string condText = "  * " + condition->GetKey();

				// Add key parameters
				const auto& params = condition->GetParams();
				if (params.count("flag"))
				{
					condText += ": " + params.at("flag").AsString();

					if (params.count("op"))
					{
						condText += " " + params.at("op").AsString();
					}

					if (params.count("value"))
					{
						condText += " " + params.at("value").AsString();
					}
				}
				else if (params.count("item"))
				{
					condText += ": " + params.at("item").AsString();
				}
				else if (params.count("level"))
				{
					condText += ": lvl " + params.at("level").AsString();
				}

				// Truncate if too long
				if (condText.length() > 32)
				{
					condText = condText.substr(0, 29) + "...";
				}

				drawList->AddText(ImGui::GetFont(), detailsFontSize, condPos, IM_COL32(200, 180, 120, 255),
				                  condText.c_str());
				lineNum++;
			}
		}
	}
	// ========================================================================
	// END CONTINUATION DETAILS
	// ========================================================================

	// Invisible button for interaction
	ImGui::SetCursorScreenPos(position);
	ImGui::PushID(nodeId.c_str());
	ImGui::InvisibleButton("node", ImVec2(nodeWidth, nodeHeight));

	// Right-click context menu
	if (ImGui::BeginPopupContextItem())
	{
		ImGui::Text("Node: %s", nodeId.c_str());
		ImGui::Separator();

		if (ImGui::MenuItem("Set as Entry Node"))
		{
			m_entryNodeId       = nodeId;
			m_hasUnsavedChanges = true;
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Duplicate"))
		{
			DuplicateNode(nodeId);
		}

		if (ImGui::MenuItem("Delete", nullptr, false, nodeId != m_entryNodeId))
		{
			DeleteNode(nodeId);
		}

		ImGui::EndPopup();
	}

	// Double-click to focus in editor
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		SelectNode(nodeId);
	}

	ImGui::PopID();
}

void DialogueEditorWindow::RenderNodeConnections(GameContext& context, ImVec2 canvasPos)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const float nodeWidth       = 200.0f * m_graphZoom;
	const float nodeHeight      = 100.0f * m_graphZoom;
	const float detailsYOffset  = 5.0f * m_graphZoom;
	const float detailsFontSize = 14.0f * 0.7f * m_graphZoom;
	const float lineHeight      = detailsFontSize + 2.0f;
	const float textYOffset     = 5.0f;  // Padding inside details box

	for (const auto& [nodeId, nodeData] : m_nodes)
	{
		const Dialogue::DialogueNode* node = nodeData.node.get();

		// Calculate node screen position (same as in RenderNode)
		ImVec2 nodeScreenPos = ImVec2(canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
		                              canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y);

		// Details box starts below the main node (same calculation as in RenderNode)
		ImVec2 detailsPos = ImVec2(nodeScreenPos.x, nodeScreenPos.y + nodeHeight + detailsYOffset);

		// Draw "next" connection (green)
		if (node->HasNext())
		{
			auto targetIt = m_nodes.find(node->GetNext().value());
			if (targetIt != m_nodes.end())
			{
				// Start from center of the "next" text line in details box
				ImVec2 fromPos = ImVec2(detailsPos.x + (nodeWidth / 2.0f),
				                        detailsPos.y + textYOffset + (lineHeight / 2.0f)  // Center of text line
				);

				// End at top-center of target node
				ImVec2 targetNodePos =
				    ImVec2(canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
				           canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y);
				ImVec2 toPos = ImVec2(targetNodePos.x + (nodeWidth / 2.0f), targetNodePos.y);

				// Draw line
				drawList->AddLine(fromPos, toPos, IM_COL32(100, 255, 100, 255), 2.0f * m_graphZoom);

				// Draw arrow at end
				ImVec2 dir = ImVec2(toPos.x - fromPos.x, toPos.y - fromPos.y);
				float len  = sqrtf(dir.x * dir.x + dir.y * dir.y);
				if (len > 0.0f)
				{
					dir.x /= len;
					dir.y /= len;

					float arrowSize = 10.0f * m_graphZoom;
					ImVec2 arrowTip = ImVec2(toPos.x - dir.x * arrowSize, toPos.y - dir.y * arrowSize);
					ImVec2 arrowLeft =
					    ImVec2(arrowTip.x - dir.y * arrowSize * 0.5f, arrowTip.y + dir.x * arrowSize * 0.5f);
					ImVec2 arrowRight =
					    ImVec2(arrowTip.x + dir.y * arrowSize * 0.5f, arrowTip.y - dir.x * arrowSize * 0.5f);

					drawList->AddTriangleFilled(toPos, arrowLeft, arrowRight, IM_COL32(100, 255, 100, 255));
				}
			}
		}
		// Draw choice connections (blue)
		else if (!node->GetChoices().empty())
		{
			const auto& choices = node->GetChoices();

			for (size_t i = 0; i < choices.size(); ++i)
			{
				auto targetIt = m_nodes.find(choices[i]->targetNode);
				if (targetIt != m_nodes.end())
				{
					// Start from the specific choice line in details box
					ImVec2 fromPos = ImVec2(detailsPos.x + (nodeWidth / 2.0f),
					                        detailsPos.y + textYOffset + (i * lineHeight) +
					                            (lineHeight / 2.0f)  // Center of this choice's line
					);

					// End at top-center of target node
					ImVec2 targetNodePos =
					    ImVec2(canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
					           canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y);
					ImVec2 toPos = ImVec2(targetNodePos.x + (nodeWidth / 2.0f), targetNodePos.y);

					// Draw line
					drawList->AddLine(fromPos, toPos, IM_COL32(100, 150, 255, 255), 2.0f * m_graphZoom);

					// Draw arrow at end
					ImVec2 dir = ImVec2(toPos.x - fromPos.x, toPos.y - fromPos.y);
					float len  = sqrtf(dir.x * dir.x + dir.y * dir.y);
					if (len > 0.0f)
					{
						dir.x /= len;
						dir.y /= len;

						float arrowSize = 8.0f * m_graphZoom;
						ImVec2 arrowTip = ImVec2(toPos.x - dir.x * arrowSize, toPos.y - dir.y * arrowSize);
						ImVec2 arrowLeft =
						    ImVec2(arrowTip.x - dir.y * arrowSize * 0.5f, arrowTip.y + dir.x * arrowSize * 0.5f);
						ImVec2 arrowRight =
						    ImVec2(arrowTip.x + dir.y * arrowSize * 0.5f, arrowTip.y - dir.x * arrowSize * 0.5f);

						drawList->AddTriangleFilled(toPos, arrowLeft, arrowRight, IM_COL32(100, 150, 255, 255));
					}
				}
			}
		}
		// Draw conditional target connections (yellow)
		else if (!node->GetTargets().empty())
		{
			const auto& targets = node->GetTargets();
			int lineNum         = 0;

			for (size_t i = 0; i < targets.size(); ++i)
			{
				const auto& target = targets[i];
				auto targetIt      = m_nodes.find(target->targetNode);

				if (targetIt != m_nodes.end())
				{
					// Start from the target line (not the conditions)
					ImVec2 fromPos = ImVec2(detailsPos.x + (nodeWidth / 2.0f),
					                        detailsPos.y + textYOffset + (lineNum * lineHeight) +
					                            (lineHeight / 2.0f)  // Center of target line
					);

					// End at top-center of target node
					ImVec2 targetNodePos =
					    ImVec2(canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
					           canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y);
					ImVec2 toPos = ImVec2(targetNodePos.x + (nodeWidth / 2.0f), targetNodePos.y);

					// Draw line
					drawList->AddLine(fromPos, toPos, IM_COL32(255, 200, 100, 255), 2.0f * m_graphZoom);

					// Draw arrow at end
					ImVec2 dir = ImVec2(toPos.x - fromPos.x, toPos.y - fromPos.y);
					float len  = sqrtf(dir.x * dir.x + dir.y * dir.y);
					if (len > 0.0f)
					{
						dir.x /= len;
						dir.y /= len;

						float arrowSize = 8.0f * m_graphZoom;
						ImVec2 arrowTip = ImVec2(toPos.x - dir.x * arrowSize, toPos.y - dir.y * arrowSize);
						ImVec2 arrowLeft =
						    ImVec2(arrowTip.x - dir.y * arrowSize * 0.5f, arrowTip.y + dir.x * arrowSize * 0.5f);
						ImVec2 arrowRight =
						    ImVec2(arrowTip.x + dir.y * arrowSize * 0.5f, arrowTip.y - dir.x * arrowSize * 0.5f);

						drawList->AddTriangleFilled(toPos, arrowLeft, arrowRight, IM_COL32(255, 200, 100, 255));
					}
				}

				// Advance line count (target + all its conditions)
				lineNum += 1;                                            // Target line
				lineNum += static_cast<int>(target->conditions.size());  // Condition lines
			}
		}
	}
}

void DialogueEditorWindow::CalculateGraphLayout()
{
	if (m_nodes.empty() || m_entryNodeId.empty())
	{
		return;
	}

	// Simple hierarchical layout
	std::unordered_map<std::string, int> levels;
	std::unordered_map<int, int> levelCounts;

	// BFS to assign levels
	std::vector<std::string> queue;
	std::set<std::string> visited;

	queue.push_back(m_entryNodeId);
	levels[m_entryNodeId] = 0;
	visited.insert(m_entryNodeId);

	while (!queue.empty())
	{
		std::string current = queue.front();
		queue.erase(queue.begin());

		int currentLevel = levels[current];
		auto nodeIt      = m_nodes.find(current);
		if (nodeIt == m_nodes.end())
		{
			continue;
		}

		const Dialogue::DialogueNode* node = nodeIt->second.node.get();

		// Process connections
		auto processTarget = [&](const std::string& targetId)
		{
			if (visited.find(targetId) == visited.end())
			{
				levels[targetId] = currentLevel + 1;
				visited.insert(targetId);
				queue.push_back(targetId);
			}
		};

		if (node->HasNext())
		{
			processTarget(node->GetNext().value());
		}

		for (const auto& choice : node->GetChoices())
		{
			processTarget(choice->targetNode);
		}

		for (const auto& target : node->GetTargets())
		{
			processTarget(target->targetNode);
		}
	}

	// Calculate positions
	const float levelSpacing = 200.0f;
	const float nodeSpacing  = 250.0f;

	for (auto& [nodeId, nodeData] : m_nodes)
	{
		int level = levels.count(nodeId) ? levels[nodeId] : 0;
		int index = levelCounts[level]++;

		nodeData.visualPosition.x = index * nodeSpacing;
		nodeData.visualPosition.y = level * levelSpacing;
	}
}

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
		                   "Tip: Click a node in the graph view or select from the file panel");
		return;
	}

	auto it = m_nodes.find(m_selectedNodeId);
	if (it == m_nodes.end())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[X] Selected node not found");
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
		ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Entry");
	}

	ImGui::Separator();

	// Basic properties
	RenderNodeBasicInfo(context, node);

	ImGui::Separator();

	// Only show the active continuation section based on what the node has
	bool hasNext    = node->GetNext().has_value();
	bool hasChoices = !node->GetChoices().empty();
	bool hasTargets = !node->GetTargets().empty();

	// Next Node Section
	if (hasNext)
	{
		if (ImGui::CollapsingHeader("Next Node", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::string currentNext = node->GetNext().value();
			if (NodeSelector("Target", currentNext))
			{
				node->SetNext(currentNext);
				m_hasUnsavedChanges = true;
			}

			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Dialogue will automatically advance to this node");
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
		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
		                   "[!] This node has no continuation (dialogue will end here)");
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

		if (ImGui::Selectable("{item_count:item=ITEM}"))
		{
			InsertVariableAtCursor(textBuffer, sizeof(textBuffer), "{item_count:item=}");
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
	{
		currentTypeIndex = 1;
	}
	else if (!node->GetChoices().empty())
	{
		currentTypeIndex = 2;
	}
	else if (!node->GetTargets().empty())
	{
		currentTypeIndex = 3;
	}

	const char* continuationTypes[] = {"None (End)", "Next Node", "Player Choices", "Conditional Targets"};

	int selectedIndex = currentTypeIndex;

	if (ImGui::Combo("##ContinuationType", &selectedIndex, continuationTypes, IM_ARRAYSIZE(continuationTypes)))
	{
		// User changed type
		if (selectedIndex != currentTypeIndex)
		{
			node->ClearNext();
			node->ClearChoices();
			node->ClearTargets();

			// Add the new continuation type
			switch (selectedIndex)
			{
				case 0:  // None
					break;

				case 1:  // Next Node
					// Set next - this takes priority over choices/targets
					node->SetNext("");
					m_hasUnsavedChanges = true;
					break;

				case 2:  // Choices
				{
					// Add a choice - takes priority over targets
					auto newChoice = std::make_unique<Dialogue::Choice>("New choice", "");
					node->AddChoice(std::move(newChoice));
					m_hasUnsavedChanges = true;
				}
				break;

				case 3:  // Targets
				{
					// Add a target - lowest priority
					std::unique_ptr<Dialogue::ConditionalTarget> newTarget =
					    std::make_unique<Dialogue::ConditionalTarget>();
					newTarget->targetNode = "";
					node->AddTarget(std::move(newTarget));
					m_hasUnsavedChanges = true;
				}
				break;
			}
		}
	}

	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: Choose how this dialogue node continues");

	ImGui::Separator();

	// Show current hierarchy
	ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Active continuations:");
	ImGui::Indent();

	if (node->GetNext().has_value())
	{
		ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "- Next Node (highest priority)");
	}

	if (!node->GetChoices().empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "- Choices (%zu)", node->GetChoices().size());
	}

	if (!node->GetTargets().empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "- Targets (%zu)", node->GetTargets().size());
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
		std::string headerLabel = "Choice " + std::to_string(i + 1);  // +": " + choices[i]->text;
		if (headerLabel.length() > 40)
		{
			headerLabel = headerLabel.substr(0, 37) + "...";
		}

		bool isOpen =
		    ImGui::TreeNodeEx(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);

		// Delete button
		ImGui::SameLine();
		bool deleteClicked = ImGui::SmallButton("Delete");

		if (isOpen)
		{
			// Choice text input with unique ID
			char choiceTextBuffer[256];
			strncpy_s(choiceTextBuffer, choices[i]->text.c_str(), sizeof(choiceTextBuffer) - 1);

			std::string textLabel = "Text##choice_text_" + std::to_string(i);
			if (ImGui::InputText(textLabel.c_str(), choiceTextBuffer, sizeof(choiceTextBuffer)))
			{
				choices[i]->text    = choiceTextBuffer;
				m_hasUnsavedChanges = true;
			}

			// Target node selector
			std::string targetNode  = choices[i]->targetNode;
			std::string targetLabel = "Target##choice_target_" + std::to_string(i);

			if (NodeSelector(targetLabel.c_str(), targetNode))
			{
				choices[i]->targetNode = targetNode;
				m_hasUnsavedChanges    = true;
			}

			// Go to target button
			if (!targetNode.empty())
			{
				if (ImGui::SmallButton("Go To"))
				{
					SelectNode(targetNode);
				}
			}

			// Show current values (read-only for now)
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current: \"%s\" -> %s", choices[i]->text.c_str(),
			                   choices[i]->targetNode.c_str());

			ImGui::TreePop();
		}

		ImGui::PopID();

		if (deleteClicked)
		{
			node->RemoveChoice(i);
			m_hasUnsavedChanges = true;
			break;
		}
	}

	// Add choice button
	ImGui::Separator();
	if (ImGui::Button("+ Add Choice", ImVec2(-1, 0)))
	{
		auto newChoice = std::make_unique<Dialogue::Choice>("New choice", "");
		node->AddChoice(std::move(newChoice));
		m_hasUnsavedChanges = true;
	}

	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: Player will choose one of these options");
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
		bool isOpen             = ImGui::TreeNodeEx(("Command: " + std::to_string(i + 1)).c_str(),
		                                            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);

		// Delete button
		ImGui::SameLine();
		bool deleteClicked = ImGui::SmallButton("Delete");

		if (isOpen)
		{
			RenderCommandParameters(context, commands[i].get());
			ImGui::TreePop();
		}

		ImGui::PopID();

		if (deleteClicked)
		{
			node->RemoveCommand(i);
			m_hasUnsavedChanges = true;
			break;
		}
	}

	// Add command button
	ImGui::Separator();
	if (ImGui::Button("+ Add Command", ImVec2(-1, 0)))
	{
		ImGui::OpenPopup("AddCommandPopup");
	}

	if (ImGui::BeginPopup("AddCommandPopup"))
	{
		ImGui::Text("Select Command Type:");
		ImGui::Separator();

		// Blank command - user fills in the type and params themselves
		if (ImGui::Selectable("None - Create your own"))
		{
			std::unordered_map<std::string, Dialogue::DialogueValue> params;
			node->AddCommand(std::make_unique<Dialogue::Command>(std::string(""), params));
			m_hasUnsavedChanges = true;
			ImGui::CloseCurrentPopup();
		}

		// These type names and param keys must match what's registered in
		// assets/Scripts/Dialogue/RegisteredFunctions/Commands.wren - Command::Execute does not null-check its
		// callback, so an unregistered type crashes at runtime rather than failing gracefully.
		if (ImGui::Selectable("setIntFlag - Set integer flag"))
		{
			std::unordered_map<std::string, Dialogue::DialogueValue> params;
			params["flag"]  = Dialogue::DialogueValue("flag_name");
			params["value"] = Dialogue::DialogueValue(0);

			node->AddCommand(std::make_unique<Dialogue::Command>(std::string("setIntFlag"), params));
			m_hasUnsavedChanges = true;
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::Selectable("setFlag - Set boolean flag"))
		{
			std::unordered_map<std::string, Dialogue::DialogueValue> params;
			params["flag"]  = Dialogue::DialogueValue("flag_name");
			params["value"] = Dialogue::DialogueValue(false);

			node->AddCommand(std::make_unique<Dialogue::Command>("setFlag", params));
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

		ImGui::EndPopup();
	}
}

void DialogueEditorWindow::RenderCommandParameters(GameContext& context, Dialogue::Command* command)
{
	const std::string& type = command->GetKey();
	auto& params            = command->GetParams();

	// Flag name
	char typeBuffer[128] = "";
	strncpy_s(typeBuffer, type.c_str(), sizeof(typeBuffer) - 1);
	if (ImGui::InputText("Type", typeBuffer, sizeof(typeBuffer)))
	{
		command->SetKey(typeBuffer);
		m_hasUnsavedChanges = true;
	}

	ImGui::Text("Parameters:");

	std::string deleteParam;
	int i = 0;
	for (auto& param : params)
	{
		// Flag name
		char paramTypeBuffer[128] = "";
		strncpy_s(paramTypeBuffer, param.first.c_str(), sizeof(paramTypeBuffer) - 1);

		std::string paramTypeLabel = "Param Type##param_type_" + std::to_string(i);
		if (ImGui::InputText(paramTypeLabel.c_str(), paramTypeBuffer, sizeof(paramTypeBuffer)))
		{
			m_hasUnsavedChanges = true;
			command->ChangeParameterKey(param.first, paramTypeBuffer);
			return;
		}

		// define the value type variable
		// Determine current type based on what's present
		int currentTypeIndex = -1;  // 0=None, 1=Next, 2=Choices, 3=Targets

		const Dialogue::DialogueValue& paramValue = param.second;

		if (paramValue.IsBool())
		{
			currentTypeIndex = 0;
		}
		else if (paramValue.IsDouble())
		{
			currentTypeIndex = 1;
		}
		else if (paramValue.IsInt())
		{
			currentTypeIndex = 2;
		}
		else if (paramValue.IsString())
		{
			currentTypeIndex = 3;
		}

		const char* ParamTypes[] = {"Bool", "Double", "Int (Will be saved as Double)", "String"};

		int selectedIndex = currentTypeIndex;

		std::string paramValueTypeLabel = "Param Value Type##param_value_type_" + std::to_string(i);
		if (ImGui::Combo(paramValueTypeLabel.c_str(), &selectedIndex, ParamTypes, IM_ARRAYSIZE(ParamTypes)))
		{
			// User changed type
			if (selectedIndex != currentTypeIndex)
			{
				Dialogue::DialogueValue newParamValue;
				switch (selectedIndex)
				{
					case 0:  // bool
						newParamValue = Dialogue::DialogueValue(paramValue.AsBool());
						break;
					case 1:  // double
						newParamValue = Dialogue::DialogueValue(paramValue.AsDouble());
						break;
					case 2:  // int
						newParamValue = Dialogue::DialogueValue(paramValue.AsInt());
						break;
					case 3:  // string
						newParamValue = Dialogue::DialogueValue(paramValue.AsString());
						break;
					default:  // error
						BREAK_MSG("[DIALOGUE EDITOR] selected type %d is not a compatible type", currentTypeIndex);
						break;
				}
				m_hasUnsavedChanges = true;
				command->SetParameter(param.first, newParamValue);
			}
		}

		// change the type value
		std::string valueTypeLabel = "Value##value_" + std::to_string(i);
		switch (selectedIndex)
		{
			case 0:  // bool
			{
				bool boolValue = paramValue.AsBool();
				if (ImGui::Checkbox(valueTypeLabel.c_str(), &boolValue))
				{
					m_hasUnsavedChanges = true;
					command->SetParameter(param.first, Dialogue::DialogueValue(boolValue));
				}
			}
			break;
			case 1:  // double
			{
				float doubleValue = (float)paramValue.AsDouble();
				if (ImGui::InputFloat(valueTypeLabel.c_str(), &doubleValue))
				{
					m_hasUnsavedChanges = true;
					command->SetParameter(param.first, Dialogue::DialogueValue(doubleValue));
				}
			}
			break;
			case 2:  // int
			{
				int intValue = paramValue.AsInt();
				if (ImGui::InputInt(valueTypeLabel.c_str(), &intValue))
				{
					m_hasUnsavedChanges = true;
					command->SetParameter(param.first, Dialogue::DialogueValue(intValue));
				}
			}
			break;
			case 3:  // string
			{
				// Flag name
				char valueBuffer[128] = "";
				strncpy_s(valueBuffer, paramValue.AsString().c_str(), sizeof(valueBuffer) - 1);

				if (ImGui::InputText(valueTypeLabel.c_str(), valueBuffer, sizeof(valueBuffer)))
				{
					m_hasUnsavedChanges = true;
					command->SetParameter(param.first, Dialogue::DialogueValue(valueBuffer));
				}
			}
			break;
			default:  // error
				BREAK_MSG("[DIALOGUE EDITOR] Current type %d is not a compatible type", currentTypeIndex);
				break;
		}

		std::string deleteLabel = "- Delete Param##button_" + std::to_string(i);
		if (ImGui::Button(deleteLabel.c_str()))
		{
			deleteParam = param.first;
		}
		i++;
	}

	if (!deleteParam.empty())
	{
		command->RemoveParameter(deleteParam);
	}

	// add button
	if (ImGui::Button("+ Add Param"))
	{
		std::string newParamName = "New Param";
		int newParamIndex        = 0;
		bool paramAdded          = false;
		do
		{
			std::string key;
			if (newParamIndex == 0)
			{
				key = newParamName;
			}
			else
			{
				key = newParamName + " " + std::to_string(newParamIndex);
			}

			if (!params.count(key))
			{
				paramAdded = true;
				Dialogue::DialogueValue value("");
				command->SetParameter(key, value);
			}
			else
			{
				newParamIndex++;
			}
		} while (!paramAdded);
		m_hasUnsavedChanges = true;
	}
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
		bool isOpen =
		    ImGui::TreeNodeEx(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed);

		// Delete button
		ImGui::SameLine();
		bool deleteTargetClicked = ImGui::SmallButton("Delete");

		if (isOpen)
		{
			// Target node selector
			std::string targetNode  = targets[i]->targetNode;
			std::string targetLabel = "Target Node##target_" + std::to_string(i);

			if (NodeSelector(targetLabel.c_str(), targetNode))
			{
				targets[i]->targetNode = targetNode;
				m_hasUnsavedChanges    = true;
			}

			ImGui::Separator();

			// Conditions
			ImGui::Text("Conditions (ALL must be true):");
			auto& conditions = targets[i]->conditions;

			if (conditions.empty())
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "[!] No conditions - this target will always match");
			}

			for (size_t j = 0; j < conditions.size(); ++j)
			{
				ImGui::PushID(static_cast<int>(j));

				std::string conditionType = conditions[j]->GetKey();
				ImGui::BulletText("Condition: %s", conditionType.c_str());

				ImGui::SameLine();
				bool deleteConditionClicked = ImGui::SmallButton("Delete");

				ImGui::Indent();
				RenderConditionParameters(context, conditions[j].get());
				ImGui::Unindent();

				ImGui::PopID();

				if (deleteConditionClicked)
				{
					conditions.erase(conditions.begin() + j);
					m_hasUnsavedChanges = true;
					break;
				}
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

				if (ImGui::Selectable("empty - Create your own"))
				{
					std::unordered_map<std::string, Dialogue::DialogueValue> params;
					auto condition = std::make_unique<Dialogue::Condition>("", params);
					conditions.push_back(std::move(condition));
					m_hasUnsavedChanges = true;
					ImGui::CloseCurrentPopup();
				}

				// These type names and param keys must match what's registered in
				// assets/Scripts/Dialogue/RegisteredFunctions/Conditions.wren - an unregistered type fails closed
				// (evaluates to false) rather than crashing, but silently won't do what the author intended.
				if (ImGui::Selectable("hasIntFlag - Compare an integer flag"))
				{
					std::unordered_map<std::string, Dialogue::DialogueValue> params;
					params["flag"]  = Dialogue::DialogueValue("flag_name");
					params["op"]    = Dialogue::DialogueValue("==");
					params["value"] = Dialogue::DialogueValue(0);
					auto condition  = std::make_unique<Dialogue::Condition>("hasIntFlag", params);
					conditions.push_back(std::move(condition));
					m_hasUnsavedChanges = true;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::Selectable("hasItem - Check for item"))
				{
					std::unordered_map<std::string, Dialogue::DialogueValue> params;
					params["item"] = Dialogue::DialogueValue("item_name");
					auto condition = std::make_unique<Dialogue::Condition>("hasItem", params);
					conditions.push_back(std::move(condition));
					m_hasUnsavedChanges = true;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::Selectable("notHasItem - Check item is absent"))
				{
					std::unordered_map<std::string, Dialogue::DialogueValue> params;
					params["item"] = Dialogue::DialogueValue("item_name");
					auto condition = std::make_unique<Dialogue::Condition>("notHasItem", params);
					conditions.push_back(std::move(condition));
					m_hasUnsavedChanges = true;
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::TreePop();
		}

		ImGui::PopID();

		if (deleteTargetClicked)
		{
			node->RemoveTarget(i);
			m_hasUnsavedChanges = true;
			break;
		}
	}

	// Add target button
	ImGui::Separator();
	if (ImGui::Button("+ Add Conditional Target", ImVec2(-1, 0)))
	{
		// Create new target with empty conditions
		std::unique_ptr<Dialogue::ConditionalTarget> newTarget = std::make_unique<Dialogue::ConditionalTarget>();
		newTarget->targetNode                                  = "";
		node->AddTarget(std::move(newTarget));
		m_hasUnsavedChanges = true;
	}

	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
	                   "Tip: Targets are evaluated in order. First matching target is used.");
}

void DialogueEditorWindow::RenderConditionParameters(GameContext& context, Dialogue::Condition* condition)
{
	const std::string& type = condition->GetKey();
	const auto& params      = condition->GetParams();

	std::string deleteParam;
	int i = 0;
	for (auto& param : params)
	{
		// Flag name
		char paramTypeBuffer[128] = "";
		strncpy_s(paramTypeBuffer, param.first.c_str(), sizeof(paramTypeBuffer) - 1);

		std::string paramTypeLabel = "Param Type##param_type_" + std::to_string(i);
		if (ImGui::InputText(paramTypeLabel.c_str(), paramTypeBuffer, sizeof(paramTypeBuffer)))
		{
			m_hasUnsavedChanges = true;
			condition->ChangeParameterKey(param.first, paramTypeBuffer);
			return;
		}

		// define the value type variable
		// Determine current type based on what's present
		int currentTypeIndex = -1;  // 0=None, 1=Next, 2=Choices, 3=Targets

		const Dialogue::DialogueValue& paramValue = param.second;

		if (paramValue.IsBool())
		{
			currentTypeIndex = 0;
		}
		else if (paramValue.IsDouble())
		{
			currentTypeIndex = 1;
		}
		else if (paramValue.IsInt())
		{
			currentTypeIndex = 2;
		}
		else if (paramValue.IsString())
		{
			currentTypeIndex = 3;
		}

		const char* ParamTypes[] = {"Bool", "Double", "Int (Will be saved as Double)", "String"};

		int selectedIndex = currentTypeIndex;

		std::string paramValueTypeLabel = "Param Value Type##param_value_type_" + std::to_string(i);
		if (ImGui::Combo(paramValueTypeLabel.c_str(), &selectedIndex, ParamTypes, IM_ARRAYSIZE(ParamTypes)))
		{
			// User changed type
			if (selectedIndex != currentTypeIndex)
			{
				Dialogue::DialogueValue newParamValue;
				switch (selectedIndex)
				{
					case 0:  // bool
						newParamValue = Dialogue::DialogueValue(paramValue.AsBool());
						break;
					case 1:  // double
						newParamValue = Dialogue::DialogueValue(paramValue.AsDouble());
						break;
					case 2:  // int
						newParamValue = Dialogue::DialogueValue(paramValue.AsInt());
						break;
					case 3:  // string
						newParamValue = Dialogue::DialogueValue(paramValue.AsString());
						break;
					default:  // error
						BREAK_MSG("[DIALOGUE EDITOR] selected type %d is not a compatible type", currentTypeIndex);
						break;
				}
				m_hasUnsavedChanges = true;
				condition->SetParameter(param.first, newParamValue);
			}
		}

		// change the type value
		std::string valueTypeLabel = "Value##value_" + std::to_string(i);
		switch (selectedIndex)
		{
			case 0:  // bool
			{
				bool boolValue = paramValue.AsBool();
				if (ImGui::Checkbox(valueTypeLabel.c_str(), &boolValue))
				{
					m_hasUnsavedChanges = true;
					condition->SetParameter(param.first, Dialogue::DialogueValue(boolValue));
				}
			}
			break;
			case 1:  // double
			{
				float doubleValue = (float)paramValue.AsDouble();
				if (ImGui::InputFloat(valueTypeLabel.c_str(), &doubleValue))
				{
					m_hasUnsavedChanges = true;
					condition->SetParameter(param.first, Dialogue::DialogueValue(doubleValue));
				}
			}
			break;
			case 2:  // int
			{
				int intValue = paramValue.AsInt();
				if (ImGui::InputInt(valueTypeLabel.c_str(), &intValue))
				{
					m_hasUnsavedChanges = true;
					condition->SetParameter(param.first, Dialogue::DialogueValue(intValue));
				}
			}
			break;
			case 3:  // string
			{
				// Flag name
				char valueBuffer[128] = "";
				strncpy_s(valueBuffer, paramValue.AsString().c_str(), sizeof(valueBuffer) - 1);

				if (ImGui::InputText(valueTypeLabel.c_str(), valueBuffer, sizeof(valueBuffer)))
				{
					m_hasUnsavedChanges = true;
					condition->SetParameter(param.first, Dialogue::DialogueValue(valueBuffer));
				}
			}
			break;
			default:  // error
				BREAK_MSG("[DIALOGUE EDITOR] Current type %d is not a compatible type", currentTypeIndex);
				break;
		}

		std::string deleteLabel = "- Delete Param##button_" + std::to_string(i);
		if (ImGui::Button(deleteLabel.c_str()))
		{
			deleteParam = param.first;
		}
		i++;
	}

	if (!deleteParam.empty())
	{
		condition->RemoveParameter(deleteParam);
	}

	// add button
	if (ImGui::Button("+ Add Param"))
	{
		std::string newParamName = "New Param";
		int newParamIndex        = 0;
		bool paramAdded          = false;
		do
		{
			std::string key;
			if (newParamIndex == 0)
			{
				key = newParamName;
			}
			else
			{
				key = newParamName + " " + std::to_string(newParamIndex);
			}

			if (!params.count(key))
			{
				paramAdded = true;
				Dialogue::DialogueValue value("");
				condition->SetParameter(key, value);
			}
			else
			{
				newParamIndex++;
			}
		} while (!paramAdded);
		m_hasUnsavedChanges = true;
	}
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool DialogueEditorWindow::NodeSelector(const char* label, std::string& nodeId)
{
	bool changed        = false;
	const char* preview = nodeId.empty() ? "<None>" : nodeId.c_str();

	if (ImGui::BeginCombo(label, preview))
	{
		// Option to clear
		if (ImGui::Selectable("<None>", nodeId.empty()))
		{
			nodeId  = "";
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
				nodeId  = id;
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
	static const char* commonSpeakers[] = {"Greg", "Player", "Narrator", "Mei", "Baron"};

	if (ImGui::BeginCombo("Speaker", buffer))
	{
		if (ImGui::Selectable("<None>"))
		{
			buffer[0] = '\0';
			changed   = true;
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
	size_t varLen     = strlen(variable);

	if (currentLen + varLen < bufferSize - 1)
	{
		strcat_s(buffer, bufferSize, variable);
	}
}

// ============================================================================
// FILE OPERATIONS
// ============================================================================

void DialogueEditorWindow::LoadDialogueFile(GameContext& context, const std::string& filepath,
                                            const std::string& className, const std::string& entryNode)
{
	// Clear existing data
	m_nodes.clear();
	m_selectedNodeId = "";
	m_entryNodeId    = entryNode;
	m_errors.clear();
	m_warnings.clear();
	m_currentClassName = className;
	m_currentFile      = filepath;

	bool success = false;

	Dialogue::DialogueExporter::DialogueSaveFormat format = FormatFromExtension(filepath);
	switch (format)
	{
		case Dialogue::DialogueExporter::DialogueSaveFormat::Wren:
			success = LoadDialogueFileWren(context, filepath, className);
			break;
		case Dialogue::DialogueExporter::DialogueSaveFormat::Json:
			success = LoadDialogueFileJson(context, filepath);
			break;
	}

	if (!success)
	{
		return;
	}

	m_currentSaveFormat = format;

	CalculateGraphLayout();
	m_hasUnsavedChanges = false;
	DEBUG_INFO("Loaded dialogue: %s (%zu nodes)", m_currentClassName.c_str(), m_nodes.size());
}

bool DialogueEditorWindow::LoadDialogueFileWren(GameContext& context, const std::string& filepath,
                                                const std::string& className)
{
	// Extract module path (without extension, relative to assets)
	// Example: "Scripts/Dialogue/ScholarDialogue.wren" -> "dialogue/scholar"
	std::string modulePath = filepath;

	std::replace(modulePath.begin(), modulePath.end(), '\\', '/');

	size_t assetsPos = modulePath.find("Scripts");
	if (assetsPos != std::string::npos)
	{
		modulePath = modulePath.substr(assetsPos + 8);  // Skip "Scripts/"
	}

	if (modulePath.ends_with(".wren"))
	{
		modulePath = modulePath.substr(0, modulePath.length() - 5);
	}

	m_currentModulePath = modulePath;

	WrenVM* vm                = context.GetWrenScriptEngine().GetVM();
	std::string classDataName = "classData_" + std::to_string(m_importCount);
	std::string moduleName    = "loader_" + std::to_string(m_importCount);
	m_importCount++;

	std::stringstream script;
	script << "import \"" << modulePath << "\" for " << m_currentClassName << "\n";
	script << "class " << classDataName << " {\n";
	script << "    static getData() {\n";
	script << "        return " << m_currentClassName << ".getData()\n";
	script << "    }\n";
	script << "}\n";

	wrenEnsureSlots(vm, 1);
	WrenInterpretResult result = wrenInterpret(vm, moduleName.c_str(), script.str().c_str());
	if (result != WREN_RESULT_SUCCESS)
	{
		DEBUG_ERROR("Failed to load Wren dialogue from %s", filepath.c_str());
		m_errors.push_back(ValidationError{ValidationError::Type::InvalidReference, filepath.c_str(), "LOAD_ERROR",
		                                   "Failed to execute Wren script to load dialogue"});
		return false;
	}

	wrenEnsureSlots(vm, 1);
	wrenGetVariable(vm, moduleName.c_str(), classDataName.c_str(), 0);
	WrenHandle* loaderClass   = wrenGetSlotHandle(vm, 0);
	WrenHandle* getDataMethod = wrenMakeCallHandle(vm, "getData()");

	wrenSetSlotHandle(vm, 0, loaderClass);
	wrenCall(vm, getDataMethod);

	wrenReleaseHandle(vm, getDataMethod);
	wrenReleaseHandle(vm, loaderClass);

	ParseDialogueDataFromWren(context, vm, 0);
	return true;
}

bool DialogueEditorWindow::LoadDialogueFileJson(GameContext& context, const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open())
	{
		DEBUG_ERROR("Failed to open JSON dialogue file: %s", filepath.c_str());
		m_errors.push_back(ValidationError{ValidationError::Type::InvalidReference, filepath.c_str(), "LOAD_ERROR",
		                                   "Failed to open JSON dialogue file"});
		return false;
	}

	nlohmann::json root;
	try
	{
		file >> root;
	}
	catch (const nlohmann::json::parse_error& e)
	{
		DEBUG_ERROR("Failed to parse JSON dialogue file %s: %s", filepath.c_str(), e.what());
		m_errors.push_back(ValidationError{ValidationError::Type::InvalidReference, filepath.c_str(), "PARSE_ERROR",
		                                   std::string("JSON parse error: ") + e.what()});
		return false;
	}

	if (!root.is_array())
	{
		DEBUG_ERROR("JSON dialogue file root must be an array: %s", filepath.c_str());
		m_errors.push_back(ValidationError{ValidationError::Type::InvalidReference, filepath.c_str(), "PARSE_ERROR",
		                                   "JSON dialogue root must be an array of nodes"});
		return false;
	}

	ParseDialogueDataFromJson(context, root);
	return true;
}

void DialogueEditorWindow::ParseDialogueDataFromJson(GameContext& context, const nlohmann::json& root)
{
	for (const auto& entry : root)
	{
		if (!entry.contains("node") || !entry.contains("data"))
		{
			continue;
		}

		std::string nodeId = entry["node"].get<std::string>();
		const auto& data   = entry["data"];

		// Create a new node and populate it
		auto node = std::make_unique<Dialogue::DialogueNode>(nodeId);

		if (data.contains("speaker"))
		{
			node->SetSpeaker(data["speaker"].get<std::string>());
		}

		if (data.contains("text"))
		{
			node->SetText(data["text"].get<std::string>());
		}

		if (data.contains("next"))
		{
			node->SetNext(data["next"].get<std::string>());
		}

		if (data.contains("commands") && data["commands"].is_array())
		{
			for (const auto& cmdJson : data["commands"])
			{
				node->AddCommand(ParseCommandFromJson(cmdJson));
			}
		}

		if (data.contains("choices") && data["choices"].is_array())
		{
			for (const auto& choiceJson : data["choices"])
			{
				node->AddChoice(ParseChoiceFromJson(choiceJson));
			}
		}

		if (data.contains("targets") && data["targets"].is_array())
		{
			for (const auto& targetJson : data["targets"])
			{
				node->AddTarget(ParseTargetFromJson(targetJson));
			}
		}

		NodeData nodeData;
		nodeData.node   = std::move(node);
		m_nodes[nodeId] = std::move(nodeData);

		if (!m_nodes.contains(m_entryNodeId))
		{
			m_entryNodeId = m_nodes.begin()->first;
		}
	}
}

std::unique_ptr<Dialogue::Command> DialogueEditorWindow::ParseCommandFromJson(const nlohmann::json& j)
{
	std::string key = j.value("type", "");

	std::unordered_map<std::string, Dialogue::DialogueValue> params;
	if (j.contains("parameters") && j["parameters"].is_array())
	{
		for (const auto& param : j["parameters"])
		{
			std::string paramKey = param.value("type", "");
			Dialogue::DialogueValue paramVal =
			    param.contains("value") ? ParseDialogueValueFromJson(param["value"]) : Dialogue::DialogueValue();
			params[paramKey] = paramVal;
		}
	}

	return std::make_unique<Dialogue::Command>(key, params);
}

std::unique_ptr<Dialogue::Choice> DialogueEditorWindow::ParseChoiceFromJson(const nlohmann::json& j)
{
	auto choice        = std::make_unique<Dialogue::Choice>();
	choice->text       = j.value("text", "");
	choice->targetNode = j.value("target", "");
	return choice;
}

std::unique_ptr<Dialogue::ConditionalTarget> DialogueEditorWindow::ParseTargetFromJson(const nlohmann::json& j)
{
	auto target        = std::make_unique<Dialogue::ConditionalTarget>();
	target->targetNode = j.value("node", "");

	if (j.contains("conditions") && j["conditions"].is_array())
	{
		for (const auto& condJson : j["conditions"])
		{
			std::string key = condJson.value("type", "");

			std::unordered_map<std::string, Dialogue::DialogueValue> params;
			if (condJson.contains("parameters") && condJson["parameters"].is_array())
			{
				for (const auto& param : condJson["parameters"])
				{
					std::string paramKey             = param.value("type", "");
					Dialogue::DialogueValue paramVal = param.contains("value")
					                                       ? ParseDialogueValueFromJson(param["value"])
					                                       : Dialogue::DialogueValue();
					params[paramKey]                 = paramVal;
				}
			}

			target->conditions.push_back(std::make_unique<Dialogue::Condition>(key, params));
		}
	}

	return target;
}

Dialogue::DialogueValue DialogueEditorWindow::ParseDialogueValueFromJson(const nlohmann::json& value)
{
	switch (value.type())
	{
		case nlohmann::json::value_t::boolean:
			return Dialogue::DialogueValue(value.get<bool>());
		case nlohmann::json::value_t::number_integer:
		case nlohmann::json::value_t::number_unsigned:
			return Dialogue::DialogueValue(value.get<int>());
		case nlohmann::json::value_t::number_float:
			return Dialogue::DialogueValue(value.get<double>());
		case nlohmann::json::value_t::string:
			return Dialogue::DialogueValue(value.get<std::string>());
		default:
			return Dialogue::DialogueValue();  // null or unsupported -> empty string
	}
}

void DialogueEditorWindow::SaveDialogueFile(const std::string& filepath,
                                            Dialogue::DialogueExporter::DialogueSaveFormat format)
{
	DEBUG_INFO("Saving dialogue file: %s", filepath.c_str());

	// Convert nodes to raw pointer map for exporter
	std::unordered_map<std::string, Dialogue::DialogueNode*> nodesToExport;
	for (auto& [nodeId, nodeData] : m_nodes)
	{
		nodesToExport[nodeId] = nodeData.node.get();
	}

	// Export to selected format
	std::string output;
	switch (format)
	{
		case Dialogue::DialogueExporter::DialogueSaveFormat::Wren:
			output = Dialogue::DialogueExporter::ExportToWren(nodesToExport, m_currentClassName);
			break;
		case Dialogue::DialogueExporter::DialogueSaveFormat::Json:
			output = Dialogue::DialogueExporter::ExportToJson(nodesToExport);
			break;
	}

	// Write to file
	std::ofstream file(filepath);
	if (file.is_open())
	{
		file << output;
		file.close();
		m_currentFile       = filepath;
		m_currentSaveFormat = format;
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

	m_currentClassName  = className;
	m_currentFile       = "";
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
	nodeData.node           = std::move(entryNode);
	nodeData.visualPosition = glm::vec2(100.0f, 100.0f);

	m_nodes[entryNodeId] = std::move(nodeData);
	m_entryNodeId        = entryNodeId;
	m_selectedNodeId     = entryNodeId;

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

	m_nodes[nodeId]     = std::move(nodeData);
	m_selectedNodeId    = nodeId;
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
	int counter           = 1;
	while (m_nodes.find(newNodeId) != m_nodes.end())
	{
		newNodeId = nodeId + "_copy" + std::to_string(counter++);
	}

	// Create duplicate node
	const Dialogue::DialogueNode* sourceNode = it->second.node.get();
	auto duplicateNode                       = std::make_unique<Dialogue::DialogueNode>(newNodeId);

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

	// Copy commands
	for (const auto& command : sourceNode->GetCommands())
	{
		duplicateNode->AddCommand(std::make_unique<Dialogue::Command>(command->GetKey(), command->GetParams()));
	}

	// Copy conditional targets (deep-copying each target's condition list)
	for (const auto& target : sourceNode->GetTargets())
	{
		auto targetCopy        = std::make_unique<Dialogue::ConditionalTarget>();
		targetCopy->targetNode = target->targetNode;
		for (const auto& condition : target->conditions)
		{
			targetCopy->conditions.push_back(
			    std::make_unique<Dialogue::Condition>(condition->GetKey(), condition->GetParams()));
		}
		duplicateNode->AddTarget(std::move(targetCopy));
	}

	NodeData nodeData;
	nodeData.node           = std::move(duplicateNode);
	nodeData.visualPosition = it->second.visualPosition + glm::vec2(250.0f, 150.0f);

	m_nodes[newNodeId]  = std::move(nodeData);
	m_selectedNodeId    = newNodeId;
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
		warning.type    = ValidationWarning::Type::UnreachableNode;
		warning.nodeId  = "";
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
				error.type       = ValidationError::Type::MissingTargetNode;
				error.nodeId     = nodeId;
				error.message    = "Next node '" + targetId + "' does not exist";
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
				error.type       = ValidationError::Type::MissingTargetNode;
				error.nodeId     = nodeId;
				error.message    = "Choice target '" + choice->targetNode + "' does not exist";
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
				error.type       = ValidationError::Type::MissingTargetNode;
				error.nodeId     = nodeId;
				error.message    = "Conditional target '" + target->targetNode + "' does not exist";
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
			warning.type    = ValidationWarning::Type::DeadEnd;
			warning.nodeId  = nodeId;
			warning.message = "Node has no continuation (no next, choices, or targets)";
			m_warnings.push_back(warning);
		}

		// Check for missing speaker
		if (!node->GetSpeaker().has_value())
		{
			ValidationWarning warning;
			warning.type    = ValidationWarning::Type::MissingSpeaker;
			warning.nodeId  = nodeId;
			warning.message = "Node has no speaker set";
			m_warnings.push_back(warning);
		}

		// Check for empty text
		if (!node->GetText().has_value() || node->GetText().value().empty())
		{
			ValidationWarning warning;
			warning.type    = ValidationWarning::Type::EmptyText;
			warning.nodeId  = nodeId;
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
				warning.type    = ValidationWarning::Type::UnreachableNode;
				warning.nodeId  = nodeId;
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

	m_currentPlaybackResult = ProcessPlaybackNode(entryNodeId);
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
	{
		return;
	}

	auto it = m_nodes.find(m_currentPlaybackResult.nodeId);
	if (it == m_nodes.end())
	{
		return;
	}

	const Dialogue::DialogueNode* node = it->second.node.get();
	if (!node->HasNext())
	{
		return;
	}

	m_currentPlaybackResult = ProcessPlaybackNode(node->GetNext().value());
}

void DialogueEditorWindow::MakePlaybackChoice(GameContext& context, int choiceIndex)
{
	if (!m_isPlaybackActive)
	{
		return;
	}

	auto it = m_nodes.find(m_currentPlaybackResult.nodeId);
	if (it == m_nodes.end())
	{
		return;
	}

	const Dialogue::DialogueNode* node = it->second.node.get();
	const auto& choices                = node->GetChoices();

	if (choiceIndex < 0 || choiceIndex >= static_cast<int>(choices.size()))
	{
		DEBUG_ERROR("Invalid choice index: %d", choiceIndex);
		return;
	}

	m_currentPlaybackResult = ProcessPlaybackNode(choices[choiceIndex]->targetNode);
}

// ============================================================================
// SANDBOXED PLAYBACK EVALUATION
//
// The playback tester never touches real game state (FlagManager, Inventory, Wren callbacks) - it evaluates
// conditions and executes commands against m_playbackIntFlags/m_playbackBoolFlags/m_playbackInventory instead,
// so testing a dialogue can't accidentally grant real items or set real flags. Recognised type names and their
// param keys mirror what's actually registered in assets/Scripts/Dialogue/RegisteredFunctions/{Conditions,
// Commands}.wren (hasIntFlag/hasItem/notHasItem, setIntFlag/setFlag/giveItem/removeItem); the older "intFlag"/
// "setInt"/"setBool" names are kept as aliases for dialogues authored before the editor's own presets were
// corrected to match the real registry. Anything else (cameraShake, pickupEntity, playSound, ...) has no
// sandboxed equivalent and is a no-op here rather than crashing or touching the real game.
// ============================================================================

Dialogue::DialogueResult DialogueEditorWindow::ProcessPlaybackNode(const std::string& nodeId)
{
	auto it = m_nodes.find(nodeId);
	if (it == m_nodes.end())
	{
		DEBUG_ERROR("Playback: node '%s' not found", nodeId.c_str());
		return Dialogue::DialogueResult::NodeNotFound(nodeId);
	}

	m_playbackHistory.push_back(nodeId);

	const Dialogue::DialogueNode* node = it->second.node.get();
	ExecutePlaybackCommands(node->GetCommands());

	Dialogue::DialogueResult result = Dialogue::DialogueResult::Success(nodeId);
	result.speaker                  = node->GetSpeaker();
	result.text                     = node->GetText();

	if (!node->GetTargets().empty())
	{
		// Evaluate targets in order - first one whose conditions are all true (or which has none) wins
		for (const auto& target : node->GetTargets())
		{
			if (target->conditions.empty() || EvaluatePlaybackConditions(target->conditions))
			{
				return ProcessPlaybackNode(target->targetNode);
			}
		}

		// No target matched - dialogue ends here, same as the real DialogueManager
		result.hasEnded = true;
	}
	else if (!node->GetChoices().empty())
	{
		const auto& choices = node->GetChoices();
		for (size_t i = 0; i < choices.size(); ++i)
		{
			result.choices.emplace_back(static_cast<int>(i), choices[i]->text);
		}
	}
	else if (node->HasNext())
	{
		result.shouldAutoAdvance = true;
	}
	else
	{
		result.hasEnded = true;
	}

	return result;
}

bool DialogueEditorWindow::EvaluatePlaybackConditions(const Dialogue::ConditionList& conditions)
{
	for (const auto& condition : conditions)
	{
		if (!EvaluatePlaybackCondition(*condition))
		{
			return false;
		}
	}
	return true;
}

bool DialogueEditorWindow::EvaluatePlaybackCondition(const Dialogue::Condition& condition)
{
	const std::string& type = condition.GetKey();
	const auto& params      = condition.GetParams();

	if (type == "hasIntFlag" || type == "intFlag")
	{
		auto opIt    = params.find("op");
		auto valueIt = params.find("value");
		if (opIt == params.end() || valueIt == params.end())
		{
			return false;
		}

		auto flagIt = params.find("flag");
		auto currentFlagIt =
		    flagIt != params.end() ? m_playbackIntFlags.find(flagIt->second.AsString()) : m_playbackIntFlags.end();
		int flagValue         = currentFlagIt != m_playbackIntFlags.end() ? currentFlagIt->second : 0;
		int compareValue      = valueIt->second.AsInt();
		const std::string& op = opIt->second.AsString();

		if (op == "==" || op == "is")
		{
			return flagValue == compareValue;
		}
		if (op == "!=")
		{
			return flagValue != compareValue;
		}
		if (op == "<")
		{
			return flagValue < compareValue;
		}
		if (op == "<=")
		{
			return flagValue <= compareValue;
		}
		if (op == ">")
		{
			return flagValue > compareValue;
		}
		if (op == ">=")
		{
			return flagValue >= compareValue;
		}

		DEBUG_WARNING("Playback: unknown comparison operator '%s' in hasIntFlag condition", op.c_str());
		return false;
	}

	if (type == "hasItem" || type == "notHasItem")
	{
		auto itemIt = params.find("item");
		if (itemIt == params.end())
		{
			return false;
		}
		const std::string& item = itemIt->second.AsString();
		bool present =
		    std::find(m_playbackInventory.begin(), m_playbackInventory.end(), item) != m_playbackInventory.end();
		return type == "hasItem" ? present : !present;
	}

	// Unrecognised condition type - the real DialogueRegistry would also have no callback registered for it,
	// so fail closed (same as Condition::Evaluate) rather than crash or silently pass.
	DEBUG_WARNING("Playback: condition type '%s' has no sandboxed evaluator - treating as false", type.c_str());
	return false;
}

void DialogueEditorWindow::ExecutePlaybackCommands(const Dialogue::CommandList& commands)
{
	for (const auto& command : commands)
	{
		ExecutePlaybackCommand(*command);
	}
}

void DialogueEditorWindow::ExecutePlaybackCommand(const Dialogue::Command& command)
{
	const std::string& type = command.GetKey();
	const auto& params      = command.GetParams();

	if (type == "setIntFlag" || type == "setInt")
	{
		auto flagIt  = params.find("flag");
		auto valueIt = params.find("value");
		if (flagIt != params.end() && valueIt != params.end())
		{
			m_playbackIntFlags[flagIt->second.AsString()] = valueIt->second.AsInt();
		}
		return;
	}

	if (type == "setFlag" || type == "setBool")
	{
		auto flagIt  = params.find("flag");
		auto valueIt = params.find("value");
		if (flagIt != params.end() && valueIt != params.end())
		{
			m_playbackBoolFlags[flagIt->second.AsString()] = valueIt->second.AsBool();
		}
		return;
	}

	if (type == "giveItem")
	{
		auto itemIt = params.find("item");
		if (itemIt != params.end())
		{
			m_playbackInventory.push_back(itemIt->second.AsString());
		}
		return;
	}

	if (type == "removeItem")
	{
		auto itemIt = params.find("item");
		if (itemIt != params.end())
		{
			auto vecIt = std::find(m_playbackInventory.begin(), m_playbackInventory.end(), itemIt->second.AsString());
			if (vecIt != m_playbackInventory.end())
			{
				m_playbackInventory.erase(vecIt);
			}
		}
		return;
	}

	// Commands with real-world side effects (camera shake, entity pickup, sound, ...) have no sandboxed
	// equivalent - skip rather than touching real game state during a test.
	DEBUG_INFO("Playback: command type '%s' has no sandboxed effect - skipped", type.c_str());
}

Dialogue::DialogueExporter::DialogueSaveFormat DialogueEditorWindow::FormatFromExtension(const std::string& filepath)
{
	if (filepath.size() >= 5 && filepath.substr(filepath.size() - 5) == ".wren")
	{
		return Dialogue::DialogueExporter::DialogueSaveFormat::Wren;
	}
	return Dialogue::DialogueExporter::DialogueSaveFormat::Json;  // default
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
		ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s:", m_currentPlaybackResult.speaker.value().c_str());
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
			return;
		}
	}

	// Auto-advance
	if (m_currentPlaybackResult.shouldAutoAdvance)
	{
		if (ImGui::Button("Continue", ImVec2(-1, 0)))
		{
			ContinuePlayback(context);
			return;
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
		return;
	}

	// Show history
	if (ImGui::CollapsingHeader("History"))
	{
		for (const auto& nodeId : m_playbackHistory)
		{
			ImGui::BulletText("%s", nodeId.c_str());
		}
	}

	// Show sandboxed state so testers can see what conditions/commands actually did without touching real
	// game state
	if (ImGui::CollapsingHeader("Sandbox State"))
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Simulated only - never affects the real game");

		ImGui::Text("Int Flags:");
		if (m_playbackIntFlags.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  (none set)");
		}
		for (const auto& [flag, value] : m_playbackIntFlags)
		{
			ImGui::BulletText("%s = %d", flag.c_str(), value);
		}

		ImGui::Text("Bool Flags:");
		if (m_playbackBoolFlags.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  (none set)");
		}
		for (const auto& [flag, value] : m_playbackBoolFlags)
		{
			ImGui::BulletText("%s = %s", flag.c_str(), value ? "true" : "false");
		}

		ImGui::Text("Inventory:");
		if (m_playbackInventory.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "  (empty)");
		}
		for (const auto& item : m_playbackInventory)
		{
			ImGui::BulletText("%s", item.c_str());
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
				ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[X]");
				ImGui::SameLine();
				ImGui::TextWrapped("[%s] %s", error.nodeId.c_str(), error.message.c_str());

				if (!error.suggestion.empty())
				{
					ImGui::Indent();
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Tip: %s", error.suggestion.c_str());
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
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "[!]");
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

void DialogueEditorWindow::ParseDialogueDataFromWren(GameContext& context, WrenVM* vm, int slot)
{
	auto nodeList = Dialogue::HelperFunctions::GetNodeListFromWren(vm, slot);

	if (nodeList.empty())
	{
		return;
	}

	for (auto& node : nodeList)
	{
		std::string key = node->GetId();
		m_nodes[key]    = {std::move(node), glm::vec2()};
	}

	if (!m_nodes.contains(m_entryNodeId))
	{
		m_entryNodeId = m_nodes.begin()->first;
	}
}
}  // namespace Struktur::Debug
