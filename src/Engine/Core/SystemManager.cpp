#include "raylib.h"
#include "SystemManager.h"
#include "Engine/Core/GameContext.h"

void Struktur::Core::SystemManager::Update(GameContext &context)
{
    for (auto& system : m_updateSystems)
    {
        system->Update(context);
    }

    ::BeginDrawing();
    ::ClearBackground(BLACK);

    for (auto& system : m_renderSystems)
    {
        system->Update(context);
    }

    ::EndDrawing();
}
