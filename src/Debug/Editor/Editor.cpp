#include "Editor.h"
#include "rlImGui.h"
#include <imgui_internal.h>  // Required for DockBuilder

#include "Engine/GameContext.h"
#include "Engine/Core/GameData.h"

Struktur::Debug::Editor::Editor()
    : m_viewportFocused(false), m_viewportHovered(false)
{
}

void Struktur::Debug::Editor::Initialise(GameContext &context)
{
    Core::GameData& gameData = context.GetGameData();
    m_renderTexture = ::LoadRenderTexture(gameData.gameWidth, gameData.gameHeight);
        
    // Set texture filter to bilinear for better scaling
    ::SetTextureFilter(m_renderTexture.texture, TEXTURE_FILTER_BILINEAR);
}

void Struktur::Debug::Editor::BeginUpdateLoop(GameContext &context)
{
    // Render game to texture
    ::BeginTextureMode(m_renderTexture);
}

void Struktur::Debug::Editor::EndUpdateLoop(GameContext &context)
{
    // End game rendering
    ::EndTextureMode();
}

void Struktur::Debug::Editor::Update(GameContext &context)
{
    // Begin ImGui frame
    ::rlImGuiBegin();
    
    // Setup dockspace and render all editor windows
    RenderEditorLayout(context);
    
    // End ImGui frame
    ::rlImGuiEnd();
}

void Struktur::Debug::Editor::RenderEditorLayout(GameContext &context)
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
    
    // Setup default layout on first run
    SetupDefaultLayout(dockspace_id, viewport->WorkSize);
    
    ImGui::End();
    
    // Render individual windows (these will dock into the dockspace)
    RenderGameViewport(context);
    RenderDebugInfo(context);
    RenderToolbar(context);
}

void Struktur::Debug::Editor::SetupDefaultLayout(ImGuiID dockspace_id, ImVec2 viewportSize)
{
    static bool first_time = true;
    if (!first_time)
        return;
        
    first_time = false;
    
    // Clear any existing layout
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewportSize);
    
    // Split the dockspace into regions
    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
    ImGuiID dock_top_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.05f, nullptr, &dock_main_id);
    
    // Dock windows to their default positions
    ImGui::DockBuilderDockWindow("Game Viewport", dock_main_id);    // Main center area
    ImGui::DockBuilderDockWindow("Debug Info", dock_right_id);      // Right panel
    ImGui::DockBuilderDockWindow("Toolbar", dock_top_id);           // Top toolbar
    
    ImGui::DockBuilderFinish(dockspace_id);
}

void Struktur::Debug::Editor::RenderMenuBar(GameContext &context)
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
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Game Viewport", nullptr, true);
            ImGui::MenuItem("Debug Info", nullptr, true);
            ImGui::MenuItem("Toolbar", nullptr, true);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                // Show about dialog
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void Struktur::Debug::Editor::RenderToolbar(GameContext &context)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::Begin("Toolbar", nullptr, 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse | 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse);
    
    // Play/Pause/Stop buttons
    if (ImGui::Button("Play"))
    {
        // Handle play
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause"))
    {
        // Handle pause
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        // Handle stop
    }
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Tool selection
    if (ImGui::Button("Select"))
    {
        // Handle select tool
    }
    ImGui::SameLine();
    if (ImGui::Button("Move"))
    {
        // Handle move tool
    }
    ImGui::SameLine();
    if (ImGui::Button("Rotate"))
    {
        // Handle rotate tool
    }
    ImGui::SameLine();
    if (ImGui::Button("Scale"))
    {
        // Handle scale tool
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void Struktur::Debug::Editor::RenderGameViewport(GameContext &context)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game Viewport", nullptr, 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // Store focus and hover state
    m_viewportFocused = ImGui::IsWindowFocused();
    m_viewportHovered = ImGui::IsWindowHovered();
    
    // Get available content region
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    
    // Calculate aspect ratio preserving dimensions
    Core::GameData& gameData = context.GetGameData();
    float gameAspect = (float)gameData.gameWidth / (float)gameData.gameHeight;
    float panelAspect = viewportPanelSize.x / viewportPanelSize.y;
    
    ImVec2 imageSize;
    if (panelAspect > gameAspect)
    {
        // Panel is wider than game aspect ratio
        imageSize.y = viewportPanelSize.y;
        imageSize.x = imageSize.y * gameAspect;
    }
    else
    {
        // Panel is taller than game aspect ratio
        imageSize.x = viewportPanelSize.x;
        imageSize.y = imageSize.x / gameAspect;
    }
    
    // Center the image
    ImVec2 cursorPos = ImGui::GetCursorPos();
    cursorPos.x += (viewportPanelSize.x - imageSize.x) * 0.5f;
    cursorPos.y += (viewportPanelSize.y - imageSize.y) * 0.5f;
    ImGui::SetCursorPos(cursorPos);
    
    // Store viewport position and size for mouse calculations
    m_viewportPos = ImGui::GetCursorScreenPos();
    m_viewportSize = imageSize;
    
    // Draw the game texture (flip vertically because OpenGL)
    rlImGuiImageRect(&m_renderTexture.texture, 
        (int)imageSize.x, (int)imageSize.y, 
        Rectangle{0, 0, (float)m_renderTexture.texture.width, -(float)m_renderTexture.texture.height});
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void Struktur::Debug::Editor::RenderDebugInfo(GameContext &context)
{
    ImGui::Begin("Debug Info");
    
    Core::GameData& gameData = context.GetGameData();
    
    ImGui::Text("Game Resolution: %dx%d", gameData.gameWidth, gameData.gameHeight);
    ImGui::Text("Application Resolution: %dx%d", gameData.applicationWidth, gameData.applicationHeight);
    ImGui::Separator();
    
    ImGui::Text("Viewport Focused: %s", m_viewportFocused ? "Yes" : "No");
    ImGui::Text("Viewport Hovered: %s", m_viewportHovered ? "Yes" : "No");
    ImGui::Separator();
    
    // Display mouse position relative to game viewport
    Vector2 gameMousePos = GetGameMousePosition(gameData);
    if (gameMousePos.x >= 0 && gameMousePos.y >= 0)
    {
        ImGui::Text("Mouse in Game: (%.1f, %.1f)", gameMousePos.x, gameMousePos.y);
    }
    else
    {
        ImGui::Text("Mouse in Game: Outside");
    }
    
    ImGui::Separator();
    
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::Text("Frame Time: %.3f ms", GetFrameTime() * 1000.0f);
    
    ImGui::Separator();
    
    if (ImGui::Button("Reset"))
    {
        // Handle reset
    }
    
    ImGui::End();
}

Vector2 Struktur::Debug::Editor::GetGameMousePosition(Core::GameData& gameData) const
{
    if (!m_viewportHovered)
    {
        return Vector2{ -1, -1 };
    }
        
    // Get mouse position relative to ImGui window
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Convert to viewport space
    float relativeX = mousePos.x - m_viewportPos.x;
    float relativeY = mousePos.y - m_viewportPos.y;
    
    // Normalize to 0-1 range
    float normalizedX = relativeX / m_viewportSize.x;
    float normalizedY = relativeY / m_viewportSize.y;
    
    // Convert to game coordinates
    Vector2 gamePos;
    gamePos.x = normalizedX * gameData.gameWidth;
    gamePos.y = normalizedY * gameData.gameHeight;
    
    // Clamp to game bounds
    if (gamePos.x < 0 || gamePos.x >= gameData.gameWidth || 
        gamePos.y < 0 || gamePos.y >= gameData.gameHeight)
    {
        return Vector2{ -1, -1 };
    }
    
    return gamePos;
}
