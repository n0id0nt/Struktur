#include "EditorCamera.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>

#include "Debug/Editor/Windows/GameViewportWindow.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
namespace
{
constexpr float MIN_ZOOM = 0.05f;
constexpr float MAX_ZOOM = 16.f;
// Zoom factor per mouse-wheel notch.
constexpr float ZOOM_STEP    = 1.15f;
constexpr float FRAME_MARGIN = 0.95f;
}  // namespace

void EditorCamera::HandleInput(GameContext& context, const GameViewportWindow& viewport)
{
	ImGuiIO& io              = ImGui::GetIO();
	Core::GameData& gameData = context.GetGameData();
	glm::vec2 viewSize{(float)gameData.gameWidth, (float)gameData.gameHeight};
	glm::vec2 viewportSize = viewport.GetViewportSize();
	if (viewportSize.x <= 0.f || viewportSize.y <= 0.f)
	{
		return;
	}
	// The viewport image is the game framebuffer scaled to fit the panel.
	float gamePixelsPerScreenPixel = viewSize.x / viewportSize.x;

	bool panHeld = ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle);
	if (!panHeld)
	{
		m_isPanning = false;
	}
	else if (viewport.IsViewportHovered() &&
	         (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
	{
		m_isPanning = true;
	}

	if (m_isPanning)
	{
		glm::vec2 screenDelta{io.MouseDelta.x, io.MouseDelta.y};
		m_view.target -= screenDelta * gamePixelsPerScreenPixel / m_view.zoom;
	}

	glm::vec2 mouseGamePos = viewport.GetGameMousePosition(context);
	if (io.MouseWheel != 0.f && mouseGamePos.x >= 0.f)
	{
		// Keep the world point under the cursor fixed while zooming.
		glm::vec2 fromCentre = mouseGamePos - viewSize / 2.f;
		glm::vec2 worldPoint = m_view.target + fromCentre / m_view.zoom;
		m_view.zoom          = std::clamp(m_view.zoom * std::pow(ZOOM_STEP, io.MouseWheel), MIN_ZOOM, MAX_ZOOM);
		m_view.target        = worldPoint - fromCentre / m_view.zoom;
	}
}

void EditorCamera::Apply(GameContext& context) const
{
	Core::GameData& gameData      = context.GetGameData();
	World::Camera& camera         = context.GetCamera();
	camera.target                 = m_view.target;
	camera.offset                 = glm::vec2(gameData.gameWidth / 2.f, gameData.gameHeight / 2.f);
	camera.zoom                   = m_view.zoom;
	camera.rotation               = 0.f;
	camera.previousCameraPosition = m_view.target;
}

void EditorCamera::FrameRect(const glm::vec2& position, const glm::vec2& size, const glm::vec2& viewSize)
{
	m_view.target = position + size / 2.f;
	m_view.zoom   = size.x > 0.f && size.y > 0.f
	                    ? std::clamp(std::min(viewSize.x / size.x, viewSize.y / size.y) * FRAME_MARGIN, MIN_ZOOM,
	                                 MAX_ZOOM)
	                    : 1.f;
}
}  // namespace Struktur::Debug
