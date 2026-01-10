#include "UIHierarchyWindow.h"
#include "Engine/GameContext.h"
#include "Engine/UI/UIManager.h"

namespace Struktur::Debug
{
    void UIHierarchyWindow::Render(GameContext& context)
    {
        if (!m_isVisible)
            return;
        
        ImGui::Begin(m_name.c_str(), &m_isOpen);
        
        ImGui::Text("UI Elements:");
        ImGui::Separator();
        
        // Render the UI hierarchy
        RenderUIHierarchy(context);
        
        ImGui::Separator();
        
        // Toolbar buttons
        if (ImGui::Button("Deselect"))
        {
            m_selectedElement = nullptr;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Focus Selected"))
        {
            if (m_selectedElement && m_selectedElement->IsFocusable())
            {
                context.GetUIManager().SetFocus(m_selectedElement);
            }
        }
        
        // Show simple selected element info
        if (m_selectedElement != nullptr)
        {
            ImGui::Separator();
            ImGui::Text("Selected: %s", m_selectedElement->GetId().empty() ? "[Unnamed]" : m_selectedElement->GetId().c_str());
            ImGui::TextDisabled("See Inspector window for details");
        }
        
        ImGui::End();
    }
    
    void UIHierarchyWindow::RenderUIHierarchy(GameContext& context)
    {
        UI::UIManager& uiManager = context.GetUIManager();
        
        // Get all root-level UI elements (elements without parents)
        const auto& elements = uiManager.GetElements();
        
        if (elements.empty())
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No UI elements");
            return;
        }
        
        // Render only root elements (children will be rendered recursively)
        for (const auto& element : elements)
        {
            if (element && element->GetParent() == nullptr)
            {
                RenderUIElementNode(element.get());
            }
        }
    }
    
    void UIHierarchyWindow::RenderUIElementNode(UI::UIElement* element)
    {
        if (!element)
            return;
        
        // Determine node flags
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | 
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        
        // Check if this element has children
        const auto& children = element->GetChildren();
        bool hasChildren = !children.empty();
        
        // If no children, make it a leaf node (no arrow)
        if (!hasChildren)
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        
        // Highlight if selected
        if (m_selectedElement == element)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        
        // Create unique ID for the tree node using element pointer
        void* nodeId = (void*)element;
        
        // Build label with ID and visibility/enabled status
        std::string label = element->GetId().empty() ? "[Unnamed]" : element->GetId();
        
        // Add status indicators
        if (!element->IsVisible())
            label += " [Hidden]";
        if (!element->IsEnabled())
            label += " [Disabled]";
        if (element->IsFocusable())
            label += " [Focusable]";
        
        // Render the tree node
        bool nodeOpen = ImGui::TreeNodeEx(nodeId, flags, "%s", label.c_str());
        
        // Handle selection on click
        if (ImGui::IsItemClicked())
        {
            m_selectedElement = element;
        }
        
        // Render context menu
        RenderElementContextMenu(element);
        
        // If node is open and has children, render them
        if (nodeOpen)
        {
            if (hasChildren)
            {
                for (const auto& child : children)
                {
                    RenderUIElementNode(child.get());
                }
            }
            
            ImGui::TreePop();
        }
    }
    
    void UIHierarchyWindow::RenderElementContextMenu(UI::UIElement* element)
    {
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::Text("Element: %s", element->GetId().c_str());
            ImGui::Separator();
            
            if (ImGui::MenuItem("Select"))
            {
                m_selectedElement = element;
            }
            
            ImGui::Separator();
            
            bool visible = element->IsVisible();
            if (ImGui::MenuItem("Toggle Visibility", nullptr, &visible))
            {
                element->SetVisible(visible);
            }
            
            bool enabled = element->IsEnabled();
            if (ImGui::MenuItem("Toggle Enabled", nullptr, &enabled))
            {
                element->SetEnabled(enabled);
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Copy ID"))
            {
                ImGui::SetClipboardText(element->GetId().c_str());
            }
            
            ImGui::EndPopup();
        }
    }
}