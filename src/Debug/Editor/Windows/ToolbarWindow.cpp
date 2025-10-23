#include "ToolbarWindow.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
    void ToolbarWindow::Render(GameContext& context)
    {
        if (!m_isVisible)
            return;
            
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::Begin(m_name.c_str(), &m_isOpen, 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoScrollWithMouse |
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

        // Debug visualization toggles
        auto& debugSettings = context.GetEditor().GetSettings().debugRender;
        
        ImGui::PushStyleColor(ImGuiCol_Button, 
            debugSettings.showPhysicsShapes ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Physics"))
        {
            debugSettings.showPhysicsShapes = !debugSettings.showPhysicsShapes;
            debugSettings.showPhysicsBodies = debugSettings.showPhysicsShapes;
            debugSettings.showPhysicsJoints = debugSettings.showPhysicsShapes;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Toggle physics debug rendering");
        }
        
        ImGui::SameLine();
        
        ImGui::PushStyleColor(ImGuiCol_Button, 
            debugSettings.showLevelBounds ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Bounds"))
        {
            debugSettings.showLevelBounds = !debugSettings.showLevelBounds;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Toggle level boundary rendering");
        }
        
        ImGui::SameLine();
        
        ImGui::PushStyleColor(ImGuiCol_Button, 
            debugSettings.showGrid ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("Grid"))
        {
            debugSettings.showGrid = !debugSettings.showGrid;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Toggle grid rendering");
        }
        
        ImGui::SameLine();
        
        ImGui::PushStyleColor(ImGuiCol_Button, 
            debugSettings.showFPS ? ImVec4(0.4f, 0.7f, 0.4f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("FPS"))
        {
            debugSettings.showFPS = !debugSettings.showFPS;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Toggle FPS counter");
        }
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
