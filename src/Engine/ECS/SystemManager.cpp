#include "raylib.h"
#include "SystemManager.h"
#include "Engine/GameContext.h"

void Struktur::System::SystemManager::Update(GameContext &context)
{
    //DEBUG_INFO("Starting Update Systems");
    for (auto& system : m_updateSystems)
    {
        //DEBUG_INFO("Update Systems");
        m_systemMap[system]->Update(context);
    }

    //DEBUG_INFO("Starting Render Systems");
    ::BeginDrawing();
    ::ClearBackground(BLACK);

    for (auto& system : m_renderSystems)
    {
        //DEBUG_INFO("Render Systems");
        m_systemMap[system]->Update(context);
    }

    ::EndDrawing();
}
