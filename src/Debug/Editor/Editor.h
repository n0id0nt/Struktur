#pragma once
#include "raylib.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>  // THIS IS REQUIRED FOR DOCKBUILDER!

namespace Struktur
{
    class GameContext;

    namespace Debug
    {
        class Editor
        {
        public:
            Editor();

            void Initialise(GameContext& context);

            void BeginUpdateLoop(GameContext& context);
            void EndUpdateLoop(GameContext& context);

        private:
            ImGuiID RenderGameView(GameContext& context);
            
            ::RenderTexture2D m_renderTexture;
        };
    }
}