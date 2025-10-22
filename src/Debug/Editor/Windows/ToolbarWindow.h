#pragma once

#include "EditorWindow.h"

namespace Struktur::Debug
{
    class ToolbarWindow : public EditorWindow
    {
    public:
        ToolbarWindow()
            : EditorWindow("Toolbar")
        {
        }
        
        void Render(GameContext& context) override;
    };
}
