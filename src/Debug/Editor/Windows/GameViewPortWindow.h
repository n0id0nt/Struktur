#pragma once

#include "EditorWindow.h"
#include <raylib.h>

namespace Struktur::Debug
{
    class GameViewportWindow : public EditorWindow
    {
    public:
        GameViewportWindow()
            : EditorWindow("Game Viewport")
            , m_viewportFocused(false)
            , m_viewportHovered(false)
        {
        }
        
        void Initialize(GameContext& context) override;
        void Render(GameContext& context) override;
        void Shutdown(GameContext& context) override;
        
        // Viewport state
        bool IsViewportFocused() const { return m_viewportFocused; }
        bool IsViewportHovered() const { return m_viewportHovered; }
        
        // Get mouse position in game coordinates
        Vector2 GetGameMousePosition(GameContext& context) const;
        
        // Get render texture (for rendering game content)
        RenderTexture2D& GetRenderTexture() { return m_renderTexture; }
        
    private:
        RenderTexture2D m_renderTexture;
        bool m_viewportFocused;
        bool m_viewportHovered;
        ImVec2 m_viewportPos;
        ImVec2 m_viewportSize;
    };
}