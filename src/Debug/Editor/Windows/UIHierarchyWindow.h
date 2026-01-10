#pragma once

#include "EditorWindow.h"
#include "Engine/UI/UIElement.h"

namespace Struktur::Debug
{
    class UIHierarchyWindow : public EditorWindow
    {
    public:
        UIHierarchyWindow()
            : EditorWindow("UI Hierarchy")
            , m_selectedElement(nullptr)
        {
        }
        
        void Render(GameContext& context) override;
        
        // Get/Set selected UI element
        UI::UIElement* GetSelectedElement() const { return m_selectedElement; }
        void SetSelectedElement(UI::UIElement* element) { m_selectedElement = element; }
        
    private:
        // Render the entire UI hierarchy starting from root elements
        void RenderUIHierarchy(GameContext& context);
        
        // Recursively render a single UI element node and its children
        void RenderUIElementNode(UI::UIElement* element);
        
        // Render context menu for a UI element
        void RenderElementContextMenu(UI::UIElement* element);
        
    private:
        UI::UIElement* m_selectedElement; // Currently selected UI element in hierarchy
    };
}