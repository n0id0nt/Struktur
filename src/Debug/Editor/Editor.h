#pragma once

#include <raylib.h>
#include <imgui.h>

namespace Struktur
{
    class GameContext;
    
    namespace Core
    {
        class GameData;
    }
    
    namespace Debug
    {
        class Editor
        {
        public:
            Editor();
            ~Editor() = default;
            
            // Initialize the editor
            void Initialise(GameContext& context);
            
            // Call before game update/render
            void BeginUpdateLoop(GameContext& context);
            
            // Call after game update/render
            void EndUpdateLoop(GameContext& context);

            void Update(GameContext& context);
            
            // Check if game viewport has focus
            bool IsViewportFocused() const { return m_viewportFocused; }
            bool IsViewportHovered() const { return m_viewportHovered; }
            
            // Get mouse position in game coordinates
            Vector2 GetGameMousePosition(Core::GameData& gameData) const;
            
        private:
            // Render the complete editor layout
            void RenderEditorLayout(GameContext& context);
            
            // Setup the default docking layout
            void SetupDefaultLayout(ImGuiID dockspace_id, ImVec2 viewportSize);
            
            // Render individual windows
            void RenderMenuBar(GameContext& context);
            void RenderToolbar(GameContext& context);
            void RenderGameViewport(GameContext& context);
            void RenderDebugInfo(GameContext& context);
            
        private:
            // Render texture for game
            RenderTexture2D m_renderTexture;
            
            // Viewport state
            bool m_viewportFocused;
            bool m_viewportHovered;
            ImVec2 m_viewportPos;
            ImVec2 m_viewportSize;
        };
    }
}