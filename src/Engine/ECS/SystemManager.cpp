#include "raylib.h"
#include "SystemManager.h"
#include "Engine/GameContext.h"

void Struktur::System::SystemManager::Update(GameContext &context)
{
    for (auto& system : m_updateSystems)
    {
        m_systemMap[system]->Update(context);
    }
}

void Struktur::System::SystemManager::Render(GameContext &context)
{
    for (auto& system : m_renderSystems)
    {
        m_systemMap[system]->Update(context);
    }
}
