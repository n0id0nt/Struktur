#pragma once

#include <bgfx/bgfx.h>

#include <glm/glm.hpp>

#include "EditorWindow.h"

namespace Struktur::Debug
{
class GameViewportWindow : public EditorWindow
{
public:
	GameViewportWindow()
	    : EditorWindow("Game Viewport"),
	      m_viewportFocused(false),
	      m_viewportHovered(false)
	{
	}

	void Initialise(GameContext& context) override;
	void Render(GameContext& context) override;
	void Shutdown(GameContext& context) override;

	// Viewport state
	bool IsViewportFocused() const
	{
		return m_viewportFocused;
	}
	bool IsViewportHovered() const
	{
		return m_viewportHovered;
	}

	// Get mouse position in game coordinates.
	glm::vec2 GetGameMousePosition(GameContext& context) const;

private:
	// (Re)creates m_frameBuffer at the given size and redirects World/Debug/UI/DebugUI into it (see
	// GraphicsDevice::SetWorldRenderTarget) - shared by Initialise and ResizeGameViewport so there's one place
	// that owns "destroy the old one first if it exists, then create+redirect".
	void CreateFrameBuffer(GameContext& context, int width, int height);
	// Changes the game's logical resolution (Core::GameData::gameWidth/Height) AND the physical framebuffer/bgfx
	// view rect to match - both are required (see CreateFrameBuffer's own comment on why gameData alone isn't
	// enough): the framebuffer's pixel size and view rect are only ever set at creation time, not re-derived from
	// gameData every frame the way UIRenderer::SetupView's orthographic projection is.
	void ResizeGameViewport(GameContext& context, int width, int height);
	// Draws the aspect-ratio preset + custom-size buttons floating over the rendered frame's top-left corner -
	// called from Render() after ImGui::Image so it paints on top of the already-drawn frame within the same
	// window, rather than pushing the image down as a toolbar row.
	void RenderSizeControls(GameContext& context);

	// The world view (GraphicsDevice::WorldViewId) renders into this instead of the backbuffer while the
	// editor is active - see GameViewportWindow::Initialise and GraphicsDevice::SetWorldRenderTarget.
	bgfx::FrameBufferHandle m_frameBuffer = BGFX_INVALID_HANDLE;
	bool m_viewportFocused;
	bool m_viewportHovered;
	ImVec2 m_viewportPos;
	ImVec2 m_viewportSize;

	// Pending values for the "Custom..." size popup's width/height inputs - seeded from the current game
	// resolution when the popup opens (see RenderSizeControls), not applied until "Apply" is pressed.
	int m_customWidth  = 0;
	int m_customHeight = 0;
};
}  // namespace Struktur::Debug
