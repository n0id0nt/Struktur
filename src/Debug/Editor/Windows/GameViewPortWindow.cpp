#include "GameViewportWindow.h"

#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"
#include "rlImGui.h"

namespace Struktur::Debug
{
void GameViewportWindow::Initialise(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	m_renderTexture          = ::LoadRenderTexture(gameData.gameWidth, gameData.gameHeight);
	::SetTextureFilter(m_renderTexture.texture, TEXTURE_FILTER_BILINEAR);
}

void GameViewportWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin(m_name.c_str(), &m_isOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	m_viewportFocused = ImGui::IsWindowFocused();
	m_viewportHovered = ImGui::IsWindowHovered();

	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

	Core::GameData& gameData = context.GetGameData();
	float gameAspect         = (float)gameData.gameWidth / (float)gameData.gameHeight;
	float panelAspect        = viewportPanelSize.x / viewportPanelSize.y;

	ImVec2 imageSize;
	if (panelAspect > gameAspect)
	{
		imageSize.y = viewportPanelSize.y;
		imageSize.x = imageSize.y * gameAspect;
	}
	else
	{
		imageSize.x = viewportPanelSize.x;
		imageSize.y = imageSize.x / gameAspect;
	}

	ImVec2 cursorPos = ImGui::GetCursorPos();
	cursorPos.x += (viewportPanelSize.x - imageSize.x) * 0.5f;
	cursorPos.y += (viewportPanelSize.y - imageSize.y) * 0.5f;
	ImGui::SetCursorPos(cursorPos);

	m_viewportPos  = ImGui::GetCursorScreenPos();
	m_viewportSize = imageSize;

	rlImGuiImageRect(&m_renderTexture.texture, (int)imageSize.x, (int)imageSize.y,
	                 Rectangle{0, 0, (float)m_renderTexture.texture.width, -(float)m_renderTexture.texture.height});

	ImGui::End();
	ImGui::PopStyleVar();
}

void GameViewportWindow::Shutdown(GameContext& context)
{
	if (m_renderTexture.id != 0)
	{
		::UnloadRenderTexture(m_renderTexture);
		m_renderTexture.id = 0;
	}
}

Vector2 GameViewportWindow::GetGameMousePosition(GameContext& context) const
{
	if (!m_viewportHovered)
	{
		return Vector2{-1, -1};
	}

	Core::GameData& gameData = context.GetGameData();
	ImVec2 mousePos          = ImGui::GetMousePos();

	float relativeX = mousePos.x - m_viewportPos.x;
	float relativeY = mousePos.y - m_viewportPos.y;

	float normalizedX = relativeX / m_viewportSize.x;
	float normalizedY = relativeY / m_viewportSize.y;

	Vector2 gamePos;
	gamePos.x = normalizedX * gameData.gameWidth;
	gamePos.y = normalizedY * gameData.gameHeight;

	if (gamePos.x < 0 || gamePos.x >= gameData.gameWidth || gamePos.y < 0 || gamePos.y >= gameData.gameHeight)
	{
		return Vector2{-1, -1};
	}

	return gamePos;
}
}  // namespace Struktur::Debug
