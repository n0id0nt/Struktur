// DialogueEditorWindow_GraphView.cpp
// Graph view rendering for dialogue editor
// Append this to DialogueEditorWindow.cpp or include as needed

#include "DialogueEditorWindow.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace Struktur::Debug
{
	void DialogueEditorWindow::RenderGraphView(GameContext& context)
	{
		ImGui::Text("Dialogue Graph");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Pan: Middle Mouse, Zoom: Scroll)");

		if (m_nodes.empty())
		{
			ImGui::TextWrapped("No nodes to display. Create or load a dialogue to see the graph.");
			return;
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();

		// Handle pan and zoom
		if (ImGui::IsWindowHovered())
		{
			// Pan with middle mouse button
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
		}

		// Draw grid
		const float gridStep = 64.0f * m_graphZoom;
		ImU32 gridColor = IM_COL32(50, 50, 50, 255);
		
		for (float x = fmodf(m_graphPanOffset.x, gridStep); x < canvasSize.x; x += gridStep)
		{
			drawList->AddLine(
				ImVec2(canvasPos.x + x, canvasPos.y),
				ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y),
				gridColor
			);
		}
		
		for (float y = fmodf(m_graphPanOffset.y, gridStep); y < canvasSize.y; y += gridStep)
		{
			drawList->AddLine(
				ImVec2(canvasPos.x, canvasPos.y + y),
				ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y),
				gridColor
			);
		}

		// Render connections first (behind nodes)
		RenderNodeConnections(context);

		// Render nodes
		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			ImVec2 screenPos = ImVec2(
				canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
				canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
			);

			RenderNode(context, nodeId, screenPos);
		}

		// Handle node selection on click
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			
			// Check if clicked on a node
			bool clickedNode = false;
			for (const auto& [nodeId, nodeData] : m_nodes)
			{
				ImVec2 screenPos = ImVec2(
					canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
					canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
				);

				const float nodeWidth = 200.0f * m_graphZoom;
				const float nodeHeight = 100.0f * m_graphZoom;

				if (mousePos.x >= screenPos.x && mousePos.x <= screenPos.x + nodeWidth &&
					mousePos.y >= screenPos.y && mousePos.y <= screenPos.y + nodeHeight)
				{
					SelectNode(nodeId);
					clickedNode = true;
					break;
				}
			}

			if (!clickedNode)
			{
				m_selectedNodeId = "";
			}
		}
	}

	void DialogueEditorWindow::RenderNode(GameContext& context, const std::string& nodeId, ImVec2 position)
	{
		auto it = m_nodes.find(nodeId);
		if (it == m_nodes.end())
			return;

		const Dialogue::DialogueNode* node = it->second.node.get();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const float nodeWidth = 200.0f * m_graphZoom;
		const float nodeHeight = 100.0f * m_graphZoom;
		const float fontSize = 14.0f * m_graphZoom;

		// Determine node color based on type
		ImU32 bgColor;
		if (nodeId == m_entryNodeId)
		{
			bgColor = IM_COL32(50, 150, 50, 255);  // Green for entry
		}
		else if (!node->GetChoices().empty())
		{
			bgColor = IM_COL32(50, 100, 200, 255);  // Blue for choices
		}
		else if (!node->GetTargets().empty())
		{
			bgColor = IM_COL32(200, 150, 50, 255);  // Yellow for conditionals
		}
		else if (!node->GetNext().has_value() && node->GetChoices().empty())
		{
			bgColor = IM_COL32(150, 50, 50, 255);  // Red for dead end
		}
		else
		{
			bgColor = IM_COL32(70, 70, 70, 255);  // Gray for normal
		}

		// Highlight if selected
		if (nodeId == m_selectedNodeId)
		{
			drawList->AddRectFilled(
				ImVec2(position.x - 3, position.y - 3),
				ImVec2(position.x + nodeWidth + 3, position.y + nodeHeight + 3),
				IM_COL32(255, 255, 0, 255),
				5.0f
			);
		}

		// Draw node background
		drawList->AddRectFilled(
			position,
			ImVec2(position.x + nodeWidth, position.y + nodeHeight),
			bgColor,
			5.0f
		);

		// Draw node border
		drawList->AddRect(
			position,
			ImVec2(position.x + nodeWidth, position.y + nodeHeight),
			IM_COL32(200, 200, 200, 255),
			5.0f,
			0,
			2.0f
		);

		// Draw node ID (title)
		ImVec2 textPos = ImVec2(position.x + 5, position.y + 5);
		drawList->AddText(
			ImGui::GetFont(),
			fontSize,
			textPos,
			IM_COL32(255, 255, 255, 255),
			nodeId.c_str()
		);

		// Draw speaker
		if (node->GetSpeaker().has_value())
		{
			textPos.y += fontSize + 2;
			std::string speakerText = "🎭 " + node->GetSpeaker().value();
			drawList->AddText(
				ImGui::GetFont(),
				fontSize * 0.85f,
				textPos,
				IM_COL32(180, 220, 255, 255),
				speakerText.c_str()
			);
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
			
			drawList->AddText(
				ImGui::GetFont(),
				fontSize * 0.75f,
				textPos,
				IM_COL32(200, 200, 200, 255),
				textPreview.c_str()
			);
		}

		// Draw node info at bottom
		textPos.y = position.y + nodeHeight - fontSize - 5;
		
		if (node->HasNext())
		{
			drawList->AddText(
				ImGui::GetFont(),
				fontSize * 0.7f,
				textPos,
				IM_COL32(100, 255, 100, 255),
				("➜ " + node->GetNext().value()).c_str()
			);
		}
		else if (!node->GetChoices().empty())
		{
			std::string choicesText = "Choices: " + std::to_string(node->GetChoices().size());
			drawList->AddText(
				ImGui::GetFont(),
				fontSize * 0.7f,
				textPos,
				IM_COL32(100, 150, 255, 255),
				choicesText.c_str()
			);
		}

		if (!node->GetCommands().empty())
		{
			textPos.x = position.x + nodeWidth - 60;
			std::string cmdText = "⚙️ " + std::to_string(node->GetCommands().size());
			drawList->AddText(
				ImGui::GetFont(),
				fontSize * 0.7f,
				textPos,
				IM_COL32(255, 200, 100, 255),
				cmdText.c_str()
			);
		}
	}

	void DialogueEditorWindow::RenderNodeConnections(GameContext& context)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();

		const float nodeWidth = 200.0f * m_graphZoom;
		const float nodeHeight = 100.0f * m_graphZoom;

		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			const Dialogue::DialogueNode* node = nodeData.node.get();
			
			ImVec2 fromPos = ImVec2(
				canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x + nodeWidth / 2,
				canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y + nodeHeight
			);

			// Draw "next" connection (green)
			if (node->HasNext())
			{
				auto targetIt = m_nodes.find(node->GetNext().value());
				if (targetIt != m_nodes.end())
				{
					ImVec2 toPos = ImVec2(
						canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x + nodeWidth / 2,
						canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
					);

					drawList->AddLine(fromPos, toPos, IM_COL32(100, 255, 100, 255), 2.0f * m_graphZoom);
					
					// Draw arrow
					ImVec2 dir = ImVec2(toPos.x - fromPos.x, toPos.y - fromPos.y);
					float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
					if (len > 0.0f)
					{
						dir.x /= len;
						dir.y /= len;
						
						float arrowSize = 10.0f * m_graphZoom;
						ImVec2 arrowTip = ImVec2(toPos.x - dir.x * arrowSize, toPos.y - dir.y * arrowSize);
						ImVec2 arrowLeft = ImVec2(arrowTip.x - dir.y * arrowSize * 0.5f, arrowTip.y + dir.x * arrowSize * 0.5f);
						ImVec2 arrowRight = ImVec2(arrowTip.x + dir.y * arrowSize * 0.5f, arrowTip.y - dir.x * arrowSize * 0.5f);
						
						drawList->AddTriangleFilled(toPos, arrowLeft, arrowRight, IM_COL32(100, 255, 100, 255));
					}
				}
			}

			// Draw choice connections (blue)
			for (const auto& choice : node->GetChoices())
			{
				auto targetIt = m_nodes.find(choice.targetNode);
				if (targetIt != m_nodes.end())
				{
					ImVec2 toPos = ImVec2(
						canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x + nodeWidth / 2,
						canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
					);

					drawList->AddLine(fromPos, toPos, IM_COL32(100, 150, 255, 255), 2.0f * m_graphZoom);
				}
			}

			// Draw target connections (yellow)
			for (const auto& target : node->GetTargets())
			{
				auto targetIt = m_nodes.find(target.targetNode);
				if (targetIt != m_nodes.end())
				{
					ImVec2 toPos = ImVec2(
						canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x + nodeWidth / 2,
						canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
					);

					drawList->AddLine(fromPos, toPos, IM_COL32(255, 200, 100, 255), 2.0f * m_graphZoom);
				}
			}
		}
	}

	void DialogueEditorWindow::CalculateGraphLayout()
	{
		if (m_nodes.empty() || m_entryNodeId.empty())
			return;

		// Simple hierarchical layout
		std::map<std::string, int> levels;
		std::map<int, int> levelCounts;

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
			auto nodeIt = m_nodes.find(current);
			if (nodeIt == m_nodes.end())
				continue;

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
				processTarget(choice.targetNode);
			}

			for (const auto& target : node->GetTargets())
			{
				processTarget(target.targetNode);
			}
		}

		// Calculate positions
		const float levelSpacing = 200.0f;
		const float nodeSpacing = 250.0f;

		for (auto& [nodeId, nodeData] : m_nodes)
		{
			int level = levels.count(nodeId) ? levels[nodeId] : 0;
			int index = levelCounts[level]++;

			nodeData.visualPosition.x = index * nodeSpacing;
			nodeData.visualPosition.y = level * levelSpacing;
		}
	}
}
