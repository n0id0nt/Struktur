#include "GameViewportWindow.h"

#include <imgui.h>

#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"

#if defined(PLATFORM_WEB)
	#include "rlImGui.h"
#else
	#include "Engine/Renderer/GraphicsDevice.h"
#endif

namespace Struktur::Debug
{
#if defined(PLATFORM_WEB)
void GameViewportWindow::Initialise(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	m_renderTexture          = ::LoadRenderTexture(gameData.gameWidth, gameData.gameHeight);
	::SetTextureFilter(m_renderTexture.texture, TEXTURE_FILTER_BILINEAR);
}
#else
void GameViewportWindow::Initialise(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	m_frameBuffer =
	    bgfx::createFrameBuffer((uint16_t)gameData.gameWidth, (uint16_t)gameData.gameHeight, bgfx::TextureFormat::BGRA8);
	// Sticky redirect: from here on WorldViewId renders into this instead of the backbuffer, for as long as
	// the editor is running - no per-frame Begin/EndTextureMode push/pop needed (see Editor::BeginUpdateLoop).
	context.GetGraphicsDevice().SetWorldRenderTarget(m_frameBuffer);
}
#endif

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

#if defined(PLATFORM_WEB)
	rlImGuiImageRect(&m_renderTexture.texture, (int)imageSize.x, (int)imageSize.y,
	                 Rectangle{0, 0, (float)m_renderTexture.texture.width, -(float)m_renderTexture.texture.height});
#else
	bgfx::TextureHandle colorTexture = bgfx::getTexture(m_frameBuffer, 0);
	// bgfx's NDC origin convention (and therefore which way a render target's V axis runs) depends on the
	// active backend - flip the display UVs rather than assuming one convention, replacing raylib's
	// negative-height-source-rect trick from before.
	bool flipY = bgfx::getCaps()->originBottomLeft;
	ImVec2 uv0 = flipY ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f);
	ImVec2 uv1 = flipY ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f);
	ImGui::Image((ImTextureID)(intptr_t)colorTexture.idx, imageSize, uv0, uv1);
#endif

	ImGui::End();
	ImGui::PopStyleVar();
}

#if defined(PLATFORM_WEB)
void GameViewportWindow::Shutdown(GameContext& context)
{
	if (m_renderTexture.id != 0)
	{
		::UnloadRenderTexture(m_renderTexture);
		m_renderTexture.id = 0;
	}
}
#else
void GameViewportWindow::Shutdown(GameContext& context)
{
	if (bgfx::isValid(m_frameBuffer))
	{
		context.GetGraphicsDevice().ResetWorldRenderTarget();
		bgfx::destroy(m_frameBuffer);
		m_frameBuffer = BGFX_INVALID_HANDLE;
	}
}
#endif

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
