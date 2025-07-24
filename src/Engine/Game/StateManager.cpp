#include "StateManager.h"

#include "Engine/GameContext.h"

void Struktur::GameResource::StateManager::ChangeState(GameContext& context, std::unique_ptr<IState> newState)
{
    if (m_state.get())
    {
        m_state->Exit(context);
    }
    m_state = std::move(newState);
    m_state->Enter(context);
}

void Struktur::GameResource::StateManager::Update(GameContext& context)
{
    m_state->Update(context);
}

void Struktur::GameResource::StateManager::Render(GameContext& context)
{
    m_state->Render(context);
}

void Struktur::GameResource::StateManager::ReleaseState(GameContext& context)
{
    m_state->Exit(context);
    m_state.reset();
}

std::string Struktur::GameResource::StateManager::GetCurrentStateName()
{
    return m_state->GetStateName();
}

Struktur::GameResource::IState* Struktur::GameResource::StateManager::GetCurrentState()
{
    return m_state.get();
}
