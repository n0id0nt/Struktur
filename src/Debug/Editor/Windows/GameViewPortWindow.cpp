#include "GameViewportWindow.h"

#include <algorithm>
#include <imgui.h>

#include "Engine/Core/GameData.h"
#include "Engine/GameContext.h"
#include "Engine/Renderer/GraphicsDevice.h"

namespace Struktur::Debug
{
namespace
{
struct AspectPreset
{
	const char* label;
	int width;
	int height;
};

// Common aspect-ratio presets for the viewport-size overlay buttons - width held at a fixed base (matching
// Main.wren's own default game width) with height derived per ratio, so switching between them reads as "same
// width, taller/shorter" instead of jumping around in both dimensions at once.
constexpr int kPresetBaseWidth = 1280;
constexpr AspectPreset kAspectPresets[] = {
    {"16:9", kPresetBaseWidth, kPresetBaseWidth * 9 / 16},
    {"4:3", kPresetBaseWidth, kPresetBaseWidth * 3 / 4},
    {"21:9", kPresetBaseWidth, kPresetBaseWidth * 9 / 21},
};
}  // namespace

void GameViewportWindow::Initialise(GameContext& context)
{
	Core::GameData& gameData = context.GetGameData();
	m_customWidth            = gameData.gameWidth;
	m_customHeight           = gameData.gameHeight;
	CreateFrameBuffer(context, gameData.gameWidth, gameData.gameHeight);
}

void GameViewportWindow::CreateFrameBuffer(GameContext& context, int width, int height)
{
	if (bgfx::isValid(m_frameBuffer))
	{
		bgfx::destroy(m_frameBuffer);
	}
	m_frameBuffer = bgfx::createFrameBuffer((uint16_t)width, (uint16_t)height, bgfx::TextureFormat::BGRA8);
	// Sticky redirect: from here on WorldViewId (and Debug/UI/DebugUI, which share it) renders into this instead
	// of the backbuffer, for as long as the editor is running - see GraphicsDevice::SetWorldRenderTarget.
	context.GetGraphicsDevice().SetWorldRenderTarget(m_frameBuffer, (uint16_t)width, (uint16_t)height);
}

void GameViewportWindow::ResizeGameViewport(GameContext& context, int width, int height)
{
	width  = std::max(width, 1);
	height = std::max(height, 1);

	Core::GameData& gameData = context.GetGameData();
	gameData.gameWidth       = width;
	gameData.gameHeight      = height;
	// UIRenderer::SetupView's orthographic projection and WorldRenderer's camera projection already re-derive
	// from gameData.gameWidth/gameHeight every frame, so updating those two fields is enough for logical layout -
	// but the framebuffer's actual pixel size and bgfx view rect are only ever set once (at creation), so those
	// need to be explicitly rebuilt here too or the physical render target would stay at its old resolution while
	// everything drawn into it assumes the new one, visibly misaligning/mis-scaling the whole frame.
	CreateFrameBuffer(context, width, height);
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

	bgfx::TextureHandle colorTexture = bgfx::getTexture(m_frameBuffer, 0);
	// bgfx's NDC origin convention (and therefore which way a render target's V axis runs) depends on the
	// active backend - flip the display UVs rather than assuming one convention.
	bool flipY = bgfx::getCaps()->originBottomLeft;
	ImVec2 uv0 = flipY ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f);
	ImVec2 uv1 = flipY ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f);
	ImGui::Image((ImTextureID)(intptr_t)colorTexture.idx, imageSize, uv0, uv1);

	RenderSizeControls(context);

	ImGui::End();
	ImGui::PopStyleVar();
}

void GameViewportWindow::RenderSizeControls(GameContext& context)
{
	// Floats over the rendered frame's top-left corner (m_viewportPos, just set above) rather than pushing the
	// image down as a toolbar row - drawn after ImGui::Image so it paints on top within this same window.
	// ImGui::Image itself never consumes clicks (unlike ImageButton), so these buttons receive input normally
	// with no z-order/hit-testing tricks needed despite visually overlapping it.
	ImGui::SetCursorScreenPos(ImVec2(m_viewportPos.x + 8.0f, m_viewportPos.y + 8.0f));
	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 0.9f));

	for (const AspectPreset& preset : kAspectPresets)
	{
		if (ImGui::Button(preset.label))
		{
			ResizeGameViewport(context, preset.width, preset.height);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%dx%d", preset.width, preset.height);
		}
		ImGui::SameLine();
	}

	if (ImGui::Button("Custom..."))
	{
		Core::GameData& gameData = context.GetGameData();
		m_customWidth            = gameData.gameWidth;
		m_customHeight           = gameData.gameHeight;
		ImGui::OpenPopup("CustomViewportSize");
	}

	ImGui::PopStyleColor(3);

	if (ImGui::BeginPopup("CustomViewportSize"))
	{
		ImGui::SetNextItemWidth(80.0f);
		ImGui::InputInt("Width", &m_customWidth);
		ImGui::SetNextItemWidth(80.0f);
		ImGui::InputInt("Height", &m_customHeight);
		if (ImGui::Button("Apply"))
		{
			ResizeGameViewport(context, m_customWidth, m_customHeight);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::EndGroup();
}

void GameViewportWindow::Shutdown(GameContext& context)
{
	if (bgfx::isValid(m_frameBuffer))
	{
		context.GetGraphicsDevice().ResetWorldRenderTarget();
		bgfx::destroy(m_frameBuffer);
		m_frameBuffer = BGFX_INVALID_HANDLE;
	}
}

glm::vec2 GameViewportWindow::GetGameMousePosition(GameContext& context) const
{
	if (!m_viewportHovered)
	{
		return glm::vec2{-1, -1};
	}

	Core::GameData& gameData = context.GetGameData();
	ImVec2 mousePos          = ImGui::GetMousePos();

	float relativeX = mousePos.x - m_viewportPos.x;
	float relativeY = mousePos.y - m_viewportPos.y;

	float normalizedX = relativeX / m_viewportSize.x;
	float normalizedY = relativeY / m_viewportSize.y;

	glm::vec2 gamePos;
	gamePos.x = normalizedX * gameData.gameWidth;
	gamePos.y = normalizedY * gameData.gameHeight;

	if (gamePos.x < 0 || gamePos.x >= gameData.gameWidth || gamePos.y < 0 || gamePos.y >= gameData.gameHeight)
	{
		return glm::vec2{-1, -1};
	}

	return gamePos;
}
}  // namespace Struktur::Debug
