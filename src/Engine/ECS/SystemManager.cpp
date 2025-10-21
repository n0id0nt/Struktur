#include "raylib.h"
#include "SystemManager.h"
#include "Engine/GameContext.h"

void Struktur::System::SystemManager::Update(GameContext &context)
{
    for (auto& system : m_updateSystems)
    {
        m_systemMap[system]->Update(context);
    }

    ::BeginDrawing();
#ifdef DEBUG
    ::ClearBackground(BLACK);
    Debug::Editor& editor = context.GetEditor();
    editor.BeginUpdateLoop(context);
#endif
    ::ClearBackground(BLACK);
    for (auto& system : m_renderSystems)
    {
        m_systemMap[system]->Update(context);
    }
#ifdef DEBUG
    editor.EndUpdateLoop(context);
#endif
    ::EndDrawing();
}
