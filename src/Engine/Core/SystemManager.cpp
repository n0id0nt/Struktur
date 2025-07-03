#include "SystemManager.h"
#include "Engine/Core/GameContext.h"

void Struktur::Core::SystemManager::Update(GameContext &context)
{
    for (auto& system : m_updateSystems)
    {
        system->Update(context);
    }
}

void Struktur::Core::SystemManager::Render(GameContext &context)
{
    for (auto& system : m_renderSystems)
    {
        system->Update(context);
    }
}
