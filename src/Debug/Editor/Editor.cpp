#include "Editor.h"
#include "rlImGui.h"

#include "Engine/GameContext.h"
#include "Engine/Core/GameData.h"

Struktur::Debug::Editor::Editor()
{
    
}

void Struktur::Debug::Editor::Initialise(GameContext &context)
{
    const int gameWidth = 1280;
    const int gameHeight = 720;

    m_renderTexture = LoadRenderTexture(gameWidth, gameHeight);
        
    // Set texture filter to bilinear for better scaling
    ::SetTextureFilter(m_renderTexture.texture, TEXTURE_FILTER_BILINEAR);
}

void Struktur::Debug::Editor::BeginUpdateLoop(GameContext &context)
{
    ::BeginTextureMode(m_renderTexture);
}

void Struktur::Debug::Editor::EndUpdateLoop(GameContext &context)
{
    ::EndTextureMode();
    
    ::rlImGuiBegin();
    ImGuiID gameView_id = RenderGameView(context);

     // Create dockspace
    //ImGuiViewport* viewport_imgui = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(960, 1080));
    ImGui::SetNextWindowViewport(gameView_id);
    
    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    host_window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    host_window_flags |= ImGuiWindowFlags_NoBackground;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    ImGui::Begin("DockSpace", nullptr, host_window_flags);
    ImGui::PopStyleVar(3);
    
    // Create dockspace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    static bool first_time = true;
    if (first_time)
    {
        first_time = false;
        
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(960, 1080));
        
        // Split the dockspace
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        
        // Dock windows
        ImGui::DockBuilderDockWindow("Game Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Debug Info", dock_right_id);
        
        ImGui::DockBuilderFinish(dockspace_id);
    }
    ImGui::End();
    // You can add other debug windows here
    ImGui::Begin("Debug Info");
    //ImGui::Text("Game Resolution: %dx%d", gameWidth, gameHeight);
    //ImGui::Text("Player Position: (%.1f, %.1f)", playerPos.x, playerPos.y);
    //ImGui::Text("Viewport Focused: %s", viewport.IsViewportFocused() ? "Yes" : "No");
    //ImGui::Text("Viewport Hovered: %s", viewport.IsViewportHovered() ? "Yes" : "No");
    ImGui::Separator();
    
    //if (mousePos.x >= 0 && mousePos.y >= 0)
    //{
    //    ImGui::Text("Mouse in Game: (%.1f, %.1f)", mousePos.x, mousePos.y);
    //}
    //else
    {
        ImGui::Text("Mouse in Game: Outside");
    }
    
    ImGui::Separator();
    //ImGui::SliderFloat("Player Speed", &playerSpeed, 100.0f, 1000.0f);
    
    if (ImGui::Button("Reset Player Position"))
    {
        //playerPos = { gameWidth / 2.0f, gameHeight / 2.0f };
    }
    
    ImGui::End();

    ::rlImGuiEnd();
}

ImGuiID Struktur::Debug::Editor::RenderGameView(GameContext &context)
{
    // Create ImGui window for the game viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game Viewport", nullptr, 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // Store focus and hover state
    //m_viewportFocused = ImGui::IsWindowFocused();
    //m_viewportHovered = ImGui::IsWindowHovered();
    
    // Get available content region
    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    
    // Calculate aspect ratio preserving dimensions
    Core::GameData& gameData = context.GetGameData();
    float gameAspect = (float)gameData.screenWidth / (float)gameData.screenHeight;
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
    //m_viewportPos = ImGui::GetCursorScreenPos();
    //m_viewportSize = imageSize;
    
    // Draw the game texture (flip vertically because OpenGL)
    rlImGuiImageRect(&m_renderTexture.texture, 
            (int)imageSize.x, (int)imageSize.y, 
            Rectangle{0, 0, (float)m_renderTexture.texture.width, -(float)m_renderTexture.texture.height});
    
    ImGui::End();
    ImGui::PopStyleVar();
    return ImGui::GetID("Game Viewport");
}
