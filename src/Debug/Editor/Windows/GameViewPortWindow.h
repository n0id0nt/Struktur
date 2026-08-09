#pragma once

#include <raylib.h>

#if !defined(PLATFORM_WEB)
	#include <bgfx/bgfx.h>
#endif

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

	// Get mouse position in game coordinates. Vector2 (a plain raylib POD struct, no rendering dependency) is
	// kept as-is on both platforms rather than introduced as new API surface just for this coordinate pair.
	Vector2 GetGameMousePosition(GameContext& context) const;

#if defined(PLATFORM_WEB)
	// Get render texture (for rendering game content)
	RenderTexture2D& GetRenderTexture()
	{
		return m_renderTexture;
	}
#endif

   private:
#if defined(PLATFORM_WEB)
	RenderTexture2D m_renderTexture;
#else
	// The world view (GraphicsDevice::WorldViewId) renders into this instead of the backbuffer while the
	// editor is active - see GameViewportWindow::Initialise and GraphicsDevice::SetWorldRenderTarget.
	bgfx::FrameBufferHandle m_frameBuffer = BGFX_INVALID_HANDLE;
#endif
	bool m_viewportFocused;
	bool m_viewportHovered;
	ImVec2 m_viewportPos;
	ImVec2 m_viewportSize;
};
}  // namespace Struktur::Debug
