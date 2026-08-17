#include "ToolbarWindow.h"

#include "Engine/Game.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
void ToolbarWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::Begin(m_name.c_str(), &m_isOpen,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
	                 ImGuiWindowFlags_NoCollapse);

	// Debug visualization toggles
	auto& debugSettings = context.GetEditor().GetSettings().debugRender;

	// Play/Pause/Stop buttons
	if (ImGui::Button(debugSettings.playingGame ? "Stop" : "Play"))
	{
		// Handle stop
		debugSettings.playingGame = !debugSettings.playingGame;
		debugSettings.pausedGame  = false;
		if (debugSettings.playingGame)
		{
			StartDebugGame(context);
		}
		else
		{
			StopDebugGame(context);
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!debugSettings.playingGame);
	if (ImGui::Button(debugSettings.pausedGame ? "Unpause" : "Pause"))
	{
		// Handle pause
		debugSettings.pausedGame = !debugSettings.pausedGame;
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		debugSettings.playingGame = true;
		StartDebugGame(context);
	}
	ImGui::EndDisabled();

	// ImGui::SameLine();
	// ImGui::Separator();
	// ImGui::SameLine();
	//
	//// Tool selection
	// if (ImGui::Button("Select"))
	//{
	//     // Handle select tool
	// }
	// ImGui::SameLine();
	// if (ImGui::Button("Move"))
	//{
	//     // Handle move tool
	// }
	// ImGui::SameLine();
	// if (ImGui::Button("Rotate"))
	//{
	//     // Handle rotate tool
	// }
	// ImGui::SameLine();
	// if (ImGui::Button("Scale"))
	//{
	//     // Handle scale tool
	// }

	ImGui::PushStyleColor(ImGuiCol_Button, debugSettings.showPhysicsShapes ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f)
	                                                                       : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	if (ImGui::Button("Physics"))
	{
		debugSettings.showPhysicsShapes = !debugSettings.showPhysicsShapes;
		debugSettings.showPhysicsBodies = debugSettings.showPhysicsShapes;
		debugSettings.showPhysicsJoints = debugSettings.showPhysicsShapes;
	}
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Toggle physics debug rendering");
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, debugSettings.showLevelBounds ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f)
	                                                                     : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	if (ImGui::Button("Bounds"))
	{
		debugSettings.showLevelBounds = !debugSettings.showLevelBounds;
	}
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Toggle level boundary rendering");
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button,
	                      debugSettings.showGrid ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	if (ImGui::Button("Grid"))
	{
		debugSettings.showGrid = !debugSettings.showGrid;
	}
	ImGui::PopStyleColor();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Toggle grid rendering");
	}

	ImGui::SameLine();

	ImGui::Text("Audio");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::SliderFloat("##AudioVolume", &debugSettings.audioVolume, 0.0f, 1.0f, "%.2f"))
	{
		context.GetMixer().SetMasterVolume(debugSettings.audioVolume);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Master game audio volume (applies to all currently playing and future sounds/music)");
	}

	ImGui::End();
	ImGui::PopStyleVar();
}
}  // namespace Struktur::Debug
