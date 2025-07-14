#include "raylib.h"
#include "SystemManager.h"
#include "Engine/GameContext.h"

void Struktur::Core::SystemManager::Update(GameContext &context)
{
    for (auto& system : m_updateSystems)
    {
        m_systemMap[system]->Update(context);
    }

    ::BeginDrawing();
    ::ClearBackground(BLACK);

    for (auto& system : m_renderSystems)
    {
        m_systemMap[system]->Update(context);
    }

    ::EndDrawing();
}
