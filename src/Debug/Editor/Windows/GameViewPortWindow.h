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
	// The world view (GraphicsDevice::WorldViewId) renders into this instead of the backbuffer while the
	// editor is active - see GameViewportWindow::Initialise and GraphicsDevice::SetWorldRenderTarget.
	bgfx::FrameBufferHandle m_frameBuffer = BGFX_INVALID_HANDLE;
	bool m_viewportFocused;
	bool m_viewportHovered;
	ImVec2 m_viewportPos;
	ImVec2 m_viewportSize;
};
}  // namespace Struktur::Debug
