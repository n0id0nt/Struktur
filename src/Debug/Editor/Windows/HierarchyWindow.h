#pragma once

#include "EditorWindow.h"

namespace Struktur::Debug
{
    class HierarchyWindow : public EditorWindow
    {
    public:
        HierarchyWindow()
            : EditorWindow("Hierarchy")
        {
        }
        
        void Render(GameContext& context) override;
        
    private:
        void RenderSceneNode(const char* name, int id);
    };
}