#include "HierarchyWindow.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
    void HierarchyWindow::Render(GameContext& context)
    {
        if (!m_isVisible)
            return;
            
        ImGui::Begin(m_name.c_str(), &m_isOpen);
        
        ImGui::Text("Scene Objects:");
        ImGui::Separator();
        
        // Placeholder scene hierarchy tree
        if (ImGui::TreeNode("Scene Root"))
        {
            RenderSceneNode("Camera", 0);
            
            if (ImGui::TreeNode("Entities"))
            {
                RenderSceneNode("Player", 1);
                RenderSceneNode("Enemy_1", 2);
                RenderSceneNode("Enemy_2", 3);
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Environment"))
            {
                RenderSceneNode("Ground", 4);
                RenderSceneNode("Sky", 5);
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Lighting"))
            {
                RenderSceneNode("DirectionalLight", 6);
                RenderSceneNode("PointLight_1", 7);
                ImGui::TreePop();
            }
            
            ImGui::TreePop();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Add Object"))
        {
            // Handle add object
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            // Handle delete
        }
        
        ImGui::End();
    }
    
    void HierarchyWindow::RenderSceneNode(const char* name, int id)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | 
                                   ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        
        ImGui::TreeNodeEx((void*)(intptr_t)id, flags, "%s", name);
        
        // Context menu
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Duplicate"))
            {
                // Handle duplicate
            }
            if (ImGui::MenuItem("Delete"))
            {
                // Handle delete
            }
            ImGui::EndPopup();
        }
    }
}
