#include "Editor.h"
#include "rlImGui.h"
#include <imgui_internal.h>

#include "Engine/GameContext.h"
#include "Engine/Core/GameData.h"

// Include all window types
#include "Debug/Editor/Windows/GameViewportWindow.h"
#include "Debug/Editor/Windows/DebugInfoWindow.h"
#include "Debug/Editor/Windows/ToolbarWindow.h"
#include "Debug/Editor/Windows/HierarchyWindow.h"
#include "Debug/Editor/Windows/InspectorWindow.h"
#include "Debug/Editor/Windows/SettingsWindow.h"
#include "Debug/Editor/Windows/PreviewWindow.h"
#include "Debug/Editor/Windows/FileExplorerWindow.h"
#include "Debug/Editor/EditorTheme.h"

namespace Struktur::Debug
{
    Editor::Editor()
        : m_gameViewport(nullptr)
        , m_currentLayout("Default")
        , m_showDemoWindow(false)
    {
    }
    
    void Editor::Initialise(GameContext& context)
    {
        // Create and register all editor windows
        
        // Game viewport (main rendering window)
        auto gameViewport = std::make_shared<GameViewportWindow>();
        m_gameViewport = gameViewport.get();
        RegisterWindow(gameViewport);
        
        // Hierarchy window (must be created before inspector)
        auto hierarchy = std::make_shared<HierarchyWindow>();
        auto hierarchyPtr = hierarchy.get();
        RegisterWindow(hierarchy);
        
        // Debug info panel
        auto debugInfo = std::make_shared<DebugInfoWindow>(m_gameViewport);
        RegisterWindow(debugInfo);
        
        // Toolbar
        auto toolbar = std::make_shared<ToolbarWindow>();
        RegisterWindow(toolbar);
        
        // Settings window
        auto settings = std::make_shared<SettingsWindow>();
        RegisterWindow(settings);

        // Preview window (must be created before inspector and file explorer)
        auto previewWindow = std::make_shared<PreviewWindow>();
        auto previewPtr = previewWindow.get();
        RegisterWindow(previewWindow);

        // File Explorer (depends on preview window)
        auto fileExplorer = std::make_shared<FileExplorerWindow>(previewPtr, "assets");
        RegisterWindow(fileExplorer);
                
        // Inspector window (depends on hierarchy)
        auto inspector = std::make_shared<InspectorWindow>(hierarchyPtr, previewPtr);
        RegisterWindow(inspector);
        
        // Initialize all windows
        for (auto& window : m_windows)
        {
            window->Initialize(context);
        }
        
        // Setup layouts
        m_layoutManager.CreateDefaultLayout();
        
        // Apply dark theme
        EditorTheme::ApplyDarkTheme();
    }
    
    void Editor::Shutdown(GameContext& context)
    {
        // Shutdown all windows
        for (auto& window : m_windows)
        {
            window->Shutdown(context);
        }
        
        m_windows.clear();
        m_windowMap.clear();
    }
    
    void Editor::BeginUpdateLoop(GameContext& context)
    {
        // Begin rendering to game viewport texture
        if (m_gameViewport)
        {
            ::BeginTextureMode(m_gameViewport->GetRenderTexture());
        }
    }
    
    void Editor::EndUpdateLoop(GameContext& context)
    {
        // End rendering to game viewport texture
        ::EndTextureMode();
    }
    
    void Editor::Update(GameContext& context)
    {
        // Begin ImGui frame
        ::rlImGuiBegin();
        
        // Setup dockspace and render all editor windows
        RenderEditorLayout(context);
        
        // Render demo window if enabled
        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
        
        // End ImGui frame
        ::rlImGuiEnd();
    }
    
    void Editor::RegisterWindow(std::shared_ptr<EditorWindow> window)
    {
        m_windows.push_back(window);
        m_windowMap[window->GetName()] = window.get();
    }
    
    EditorWindow* Editor::GetWindow(const std::string& name)
    {
        auto it = m_windowMap.find(name);
        if (it != m_windowMap.end())
        {
            return it->second;
        }
        return nullptr;
    }
    
    void Editor::RenderEditorLayout(GameContext& context)
    {
        Core::GameData& gameData = context.GetGameData();
        
        // Create fullscreen dockspace window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
        ImGui::PopStyleVar(3);
        
        // Render menu bar
        RenderMenuBar(context);
        
        // Create dockspace
        ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        
        // Setup layout
        m_layoutManager.SetupLayout(dockspace_id, m_currentLayout, viewport->WorkSize);
        
        ImGui::End();
        
        // Render all registered windows
        for (auto& window : m_windows)
        {
            if (window->IsVisible())
            {
                window->Render(context);
            }
        }
    }
    
    void Editor::RenderMenuBar(GameContext& context)
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    // Handle new file
                }
                if (ImGui::MenuItem("Open", "Ctrl+O"))
                {
                    // Handle open file
                }
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    // Handle save
                }
                ImGui::Separator();
                
                if (ImGui::MenuItem("Save Layout"))
                {
                    m_layoutManager.SaveLayout("editor_layout.ini");
                }
                if (ImGui::MenuItem("Load Layout"))
                {
                    m_layoutManager.LoadLayout("editor_layout.ini");
                }
                
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    // Handle exit
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z"))
                {
                    // Handle undo
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y"))
                {
                    // Handle redo
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Settings", "Ctrl+,"))
                {
                    // Open settings window
                    auto* settingsWindow = GetWindow("Editor Settings");
                    if (settingsWindow)
                    {
                        settingsWindow->SetVisible(true);
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View"))
            {
                RenderViewMenu();
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Layout"))
            {
                RenderLayoutMenu();
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Theme"))
            {
                if (ImGui::MenuItem("Dark"))
                {
                    EditorTheme::ApplyDarkTheme();
                }
                if (ImGui::MenuItem("Light"))
                {
                    EditorTheme::ApplyLightTheme();
                }
                if (ImGui::MenuItem("Classic"))
                {
                    EditorTheme::ApplyClassicTheme();
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("ImGui Demo"))
                {
                    m_showDemoWindow = !m_showDemoWindow;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("About"))
                {
                    // Show about dialog
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
    }
    
    void Editor::RenderViewMenu()
    {
        // Toggle visibility of each window
        for (auto& window : m_windows)
        {
            bool isVisible = window->IsVisible();
            if (ImGui::MenuItem(window->GetName().c_str(), nullptr, &isVisible))
            {
                window->SetVisible(isVisible);
            }
        }
    }
    
    void Editor::RenderLayoutMenu()
    {
        if (ImGui::MenuItem("Reset to Default"))
        {
            m_layoutManager.ResetToLayout("Default");
            m_currentLayout = "Default";
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Save Current Layout"))
        {
            m_layoutManager.SaveLayout("editor_layout.ini");
        }
        
        if (ImGui::MenuItem("Load Saved Layout"))
        {
            m_layoutManager.LoadLayout("editor_layout.ini");
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Reset All Layouts"))
        {
            m_layoutManager.ResetAllLayouts();
        }
    }
}