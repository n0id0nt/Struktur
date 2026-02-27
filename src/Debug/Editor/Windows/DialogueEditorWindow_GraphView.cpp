#include "DialogueEditorWindow.h"

#include <set>
#include <imgui.h>
#include <imgui_internal.h>

namespace Struktur::Debug
{
	void DialogueEditorWindow::RenderGraphView(GameContext& context)
	{
		ImGui::Text("Dialogue Graph");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Pan: Middle Mouse, Zoom: Scroll, Right-Click: Add Node, Drag: Move Nodes)");

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
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();

		// Create invisible button for the entire canvas to capture input
		ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
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
					float graphX = (mousePos.x - canvasPos.x - m_graphPanOffset.x) / m_graphZoom;
					float graphY = (mousePos.y - canvasPos.y - m_graphPanOffset.y) / m_graphZoom;

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
		RenderNodeConnections(context, canvasPos);

		// Track which node is being dragged
		static std::string draggedNodeId = "";
		static glm::vec2 dragStartPos(0.0f, 0.0f);
		static glm::vec2 nodeStartPos(0.0f, 0.0f);

		// Render nodes
		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			ImVec2 screenPos = ImVec2(
				canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
				canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
			);

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

					// Start dragging this node
					draggedNodeId = nodeId;
					dragStartPos = glm::vec2(mousePos.x, mousePos.y);
					nodeStartPos = nodeData.visualPosition;

					break;
				}
			}

			if (!clickedNode)
			{
				m_selectedNodeId = "";
				draggedNodeId = "";
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
				m_hasUnsavedChanges = true;
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

		// ========================================================================
		// CONTINUATION DETAILS BOX (below main node)
		// ========================================================================
		const float detailsYOffset = 5.0f * m_graphZoom;
		ImVec2 detailsPos = ImVec2(position.x, position.y + nodeHeight + detailsYOffset);
		const float detailsWidth = nodeWidth;
		const float detailsFontSize = fontSize * 0.7f;
		const float lineHeight = detailsFontSize + 2.0f;

		// Next Node Details
		if (node->HasNext())
		{
			const std::string& nextId = node->GetNext().value();
			float boxHeight = lineHeight + 10.0f;

			// Background
			drawList->AddRectFilled(
				detailsPos,
				ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
				IM_COL32(40, 60, 40, 220),  // Dark green
				3.0f
			);

			// Border
			drawList->AddRect(
				detailsPos,
				ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
				IM_COL32(100, 255, 100, 200),
				3.0f,
				0,
				1.5f
			);

			// Text
			ImVec2 textPos = ImVec2(detailsPos.x + 5, detailsPos.y + 5);
			std::string nextText = "→ " + nextId;
			drawList->AddText(
				ImGui::GetFont(),
				detailsFontSize,
				textPos,
				IM_COL32(150, 255, 150, 255),
				nextText.c_str()
			);
		}
		// Choices Details
		else if (!node->GetChoices().empty())
		{
			const auto& choices = node->GetChoices();
			float boxHeight = (choices.size() * lineHeight) + 10.0f;

			// Background
			drawList->AddRectFilled(
				detailsPos,
				ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
				IM_COL32(40, 50, 80, 220),  // Dark blue
				3.0f
			);

			// Border
			drawList->AddRect(
				detailsPos,
				ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
				IM_COL32(100, 150, 255, 200),
				3.0f,
				0,
				1.5f
			);

			// List choices
			for (size_t i = 0; i < choices.size(); ++i)
			{
				ImVec2 choicePos = ImVec2(
					detailsPos.x + 5,
					detailsPos.y + 5 + (i * lineHeight)
				);

				std::string choiceText = "• " + choices[i]->text;
				if (choiceText.length() > 25)
				{
					choiceText = choiceText.substr(0, 22) + "...";
				}
				choiceText += " → " + choices[i]->targetNode;

				// Truncate target if combined text is too long
				if (choiceText.length() > 35)
				{
					choiceText = choiceText.substr(0, 32) + "...";
				}

				drawList->AddText(
					ImGui::GetFont(),
					detailsFontSize,
					choicePos,
					IM_COL32(180, 200, 255, 255),
					choiceText.c_str()
				);
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
				totalLines += 1;  // Target line
				totalLines += static_cast<int>(target->conditions.size());  // Condition lines
			}
			float boxHeight = (totalLines * lineHeight) + 10.0f;

			// Background
			drawList->AddRectFilled(
				detailsPos,
				ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
				IM_COL32(80, 60, 40, 220),  // Dark yellow/orange
				3.0f
			);

			// Border
			drawList->AddRect(
				detailsPos,
				ImVec2(detailsPos.x + detailsWidth, detailsPos.y + boxHeight),
				IM_COL32(255, 200, 100, 200),
				3.0f,
				0,
				1.5f
			);

			// List targets and conditions
			int lineNum = 0;
			for (size_t i = 0; i < targets.size(); ++i)
			{
				const auto& target = targets[i];

				// Target node
				ImVec2 targetPos = ImVec2(
					detailsPos.x + 5,
					detailsPos.y + 5 + (lineNum * lineHeight)
				);

				std::string targetText = "→ " + target->targetNode;
				if (targetText.length() > 30)
				{
					targetText = targetText.substr(0, 27) + "...";
				}

				drawList->AddText(
					ImGui::GetFont(),
					detailsFontSize,
					targetPos,
					IM_COL32(255, 220, 150, 255),
					targetText.c_str()
				);
				lineNum++;

				// Conditions (indented)
				for (const auto& condition : target->conditions)
				{
					ImVec2 condPos = ImVec2(
						detailsPos.x + 15,  // Indent
						detailsPos.y + 5 + (lineNum * lineHeight)
					);

					// Format condition: "type: param=value"
					std::string condText = "  ⟡ " + condition->GetKey();

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

					drawList->AddText(
						ImGui::GetFont(),
						detailsFontSize,
						condPos,
						IM_COL32(200, 180, 120, 255),
						condText.c_str()
					);
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
				m_entryNodeId = nodeId;
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

		const float nodeWidth = 200.0f * m_graphZoom;
		const float nodeHeight = 100.0f * m_graphZoom;
		const float detailsYOffset = 5.0f * m_graphZoom;
		const float detailsFontSize = 14.0f * 0.7f * m_graphZoom;
		const float lineHeight = detailsFontSize + 2.0f;
		const float textYOffset = 5.0f;  // Padding inside details box

		for (const auto& [nodeId, nodeData] : m_nodes)
		{
			const Dialogue::DialogueNode* node = nodeData.node.get();

			// Calculate node screen position (same as in RenderNode)
			ImVec2 nodeScreenPos = ImVec2(
				canvasPos.x + (nodeData.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
				canvasPos.y + (nodeData.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
			);

			// Details box starts below the main node (same calculation as in RenderNode)
			ImVec2 detailsPos = ImVec2(
				nodeScreenPos.x,
				nodeScreenPos.y + nodeHeight + detailsYOffset
			);

			// Draw "next" connection (green)
			if (node->HasNext())
			{
				auto targetIt = m_nodes.find(node->GetNext().value());
				if (targetIt != m_nodes.end())
				{
					// Start from center of the "next" text line in details box
					ImVec2 fromPos = ImVec2(
						detailsPos.x + (nodeWidth / 2.0f),
						detailsPos.y + textYOffset + (lineHeight / 2.0f)  // Center of text line
					);

					// End at top-center of target node
					ImVec2 targetNodePos = ImVec2(
						canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
						canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
					);
					ImVec2 toPos = ImVec2(
						targetNodePos.x + (nodeWidth / 2.0f),
						targetNodePos.y
					);

					// Draw line
					drawList->AddLine(fromPos, toPos, IM_COL32(100, 255, 100, 255), 2.0f * m_graphZoom);

					// Draw arrow at end
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
			else if (!node->GetChoices().empty())
			{
				const auto& choices = node->GetChoices();

				for (size_t i = 0; i < choices.size(); ++i)
				{
					auto targetIt = m_nodes.find(choices[i]->targetNode);
					if (targetIt != m_nodes.end())
					{
						// Start from the specific choice line in details box
						ImVec2 fromPos = ImVec2(
							detailsPos.x + (nodeWidth / 2.0f),
							detailsPos.y + textYOffset + (i * lineHeight) + (lineHeight / 2.0f)  // Center of this choice's line
						);

						// End at top-center of target node
						ImVec2 targetNodePos = ImVec2(
							canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
							canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
						);
						ImVec2 toPos = ImVec2(
							targetNodePos.x + (nodeWidth / 2.0f),
							targetNodePos.y
						);

						// Draw line
						drawList->AddLine(fromPos, toPos, IM_COL32(100, 150, 255, 255), 2.0f * m_graphZoom);

						// Draw arrow at end
						ImVec2 dir = ImVec2(toPos.x - fromPos.x, toPos.y - fromPos.y);
						float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
						if (len > 0.0f)
						{
							dir.x /= len;
							dir.y /= len;

							float arrowSize = 8.0f * m_graphZoom;
							ImVec2 arrowTip = ImVec2(toPos.x - dir.x * arrowSize, toPos.y - dir.y * arrowSize);
							ImVec2 arrowLeft = ImVec2(arrowTip.x - dir.y * arrowSize * 0.5f, arrowTip.y + dir.x * arrowSize * 0.5f);
							ImVec2 arrowRight = ImVec2(arrowTip.x + dir.y * arrowSize * 0.5f, arrowTip.y - dir.x * arrowSize * 0.5f);

							drawList->AddTriangleFilled(toPos, arrowLeft, arrowRight, IM_COL32(100, 150, 255, 255));
						}
					}
				}
			}
			// Draw conditional target connections (yellow)
			else if (!node->GetTargets().empty())
			{
				const auto& targets = node->GetTargets();
				int lineNum = 0;

				for (size_t i = 0; i < targets.size(); ++i)
				{
					const auto& target = targets[i];
					auto targetIt = m_nodes.find(target->targetNode);

					if (targetIt != m_nodes.end())
					{
						// Start from the target line (not the conditions)
						ImVec2 fromPos = ImVec2(
							detailsPos.x + (nodeWidth / 2.0f),
							detailsPos.y + textYOffset + (lineNum * lineHeight) + (lineHeight / 2.0f)  // Center of target line
						);

						// End at top-center of target node
						ImVec2 targetNodePos = ImVec2(
							canvasPos.x + (targetIt->second.visualPosition.x * m_graphZoom) + m_graphPanOffset.x,
							canvasPos.y + (targetIt->second.visualPosition.y * m_graphZoom) + m_graphPanOffset.y
						);
						ImVec2 toPos = ImVec2(
							targetNodePos.x + (nodeWidth / 2.0f),
							targetNodePos.y
						);

						// Draw line
						drawList->AddLine(fromPos, toPos, IM_COL32(255, 200, 100, 255), 2.0f * m_graphZoom);

						// Draw arrow at end
						ImVec2 dir = ImVec2(toPos.x - fromPos.x, toPos.y - fromPos.y);
						float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
						if (len > 0.0f)
						{
							dir.x /= len;
							dir.y /= len;

							float arrowSize = 8.0f * m_graphZoom;
							ImVec2 arrowTip = ImVec2(toPos.x - dir.x * arrowSize, toPos.y - dir.y * arrowSize);
							ImVec2 arrowLeft = ImVec2(arrowTip.x - dir.y * arrowSize * 0.5f, arrowTip.y + dir.x * arrowSize * 0.5f);
							ImVec2 arrowRight = ImVec2(arrowTip.x + dir.y * arrowSize * 0.5f, arrowTip.y - dir.x * arrowSize * 0.5f);

							drawList->AddTriangleFilled(toPos, arrowLeft, arrowRight, IM_COL32(255, 200, 100, 255));
						}
					}

					// Advance line count (target + all its conditions)
					lineNum += 1;  // Target line
					lineNum += static_cast<int>(target->conditions.size());  // Condition lines
				}
			}
		}
	}

	void DialogueEditorWindow::CalculateGraphLayout()
	{
		if (m_nodes.empty() || m_entryNodeId.empty())
			return;

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
				processTarget(choice->targetNode);
			}

			for (const auto& target : node->GetTargets())
			{
				processTarget(target->targetNode);
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