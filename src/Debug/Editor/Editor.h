#pragma once

#include <raylib.h>
#include <imgui.h>
#include <memory>
#include <vector>
#include <unordered_map>

#include "Debug/Editor/Windows/EditorWindow.h"
#include "Debug/Editor/EditorLayoutManager.h"

namespace Struktur
{
    class GameContext;
    
    namespace Debug
    {
        class GameViewportWindow;
        
        class Editor
        {
        public:
            Editor();
            ~Editor() = default;
            
            // Initialize the editor
            void Initialise(GameContext& context);
            
            // Shutdown the editor
            void Shutdown(GameContext& context);
            
            // Call before game update/render
            void BeginUpdateLoop(GameContext& context);
            
            // Call after game update/render
            void EndUpdateLoop(GameContext& context);
            
            // Update editor UI (call after EndUpdateLoop)
            void Update(GameContext& context);
            
            // Register a new editor window
            void RegisterWindow(std::shared_ptr<EditorWindow> window);
            
            // Get a window by name
            EditorWindow* GetWindow(const std::string& name);
            
            // Get the game viewport window
            GameViewportWindow* GetGameViewport() { return m_gameViewport; }
            
        private:
            // Render the complete editor layout
            void RenderEditorLayout(GameContext& context);
            
            // Render individual components
            void RenderMenuBar(GameContext& context);
            void RenderViewMenu();
            void RenderLayoutMenu();
            
        private:
            // Window management
            std::vector<std::shared_ptr<EditorWindow>> m_windows;
            std::unordered_map<std::string, EditorWindow*> m_windowMap;
            
            // Special windows (kept as raw pointers for quick access)
            GameViewportWindow* m_gameViewport;
            
            // Layout management
            EditorLayoutManager m_layoutManager;
            std::string m_currentLayout;
            
            // Editor state
            bool m_showDemoWindow;
        };
    }
}