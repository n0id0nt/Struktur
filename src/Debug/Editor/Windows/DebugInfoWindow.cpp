
#include "DebugInfoWindow.h"

#include <raylib.h>

#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"
#include "GameViewportWindow.h"

namespace Struktur::Debug
{
void DebugInfoWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	Core::GameData& gameData = context.GetGameData();

	ImGui::Text("Game Resolution: %dx%d", gameData.gameWidth, gameData.gameHeight);
	ImGui::Text("Application Resolution: %dx%d", gameData.applicationWidth, gameData.applicationHeight);
	ImGui::Separator();

	if (m_viewportWindow)
	{
		ImGui::Text("Viewport Focused: %s", m_viewportWindow->IsViewportFocused() ? "Yes" : "No");
		ImGui::Text("Viewport Hovered: %s", m_viewportWindow->IsViewportHovered() ? "Yes" : "No");
		ImGui::Separator();

		Vector2 gameMousePos = m_viewportWindow->GetGameMousePosition(context);
		if (gameMousePos.x >= 0 && gameMousePos.y >= 0)
		{
			ImGui::Text("Mouse in Game: (%.1f, %.1f)", gameMousePos.x, gameMousePos.y);
		}
		else
		{
			ImGui::Text("Mouse in Game: Outside");
		}
	}

	ImGui::Separator();
	ImGui::Text("FPS: %d", GetFPS());
	ImGui::Text("Frame Time: %.3f ms", GetFrameTime() * 1000.0f);
	ImGui::Separator();

	if (ImGui::Button("Reset"))
	{
		// Handle reset
	}

	ImGui::End();
}
}  // namespace Struktur::Debug
