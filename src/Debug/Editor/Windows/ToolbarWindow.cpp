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
        
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
