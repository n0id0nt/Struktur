#include "StateManager.h"

#include <format>

#include "Engine/GameContext.h"
#include "Debug/Assertions.h"

void Struktur::GameResource::StateManager::ChangeState(GameContext& context, std::unique_ptr<IState> newState)
{
    if (m_state.get())
    {
        DEBUG_INFO(std::format("[State Manager] Exiting: {}", m_state->GetStateName()));
        m_state->Exit(context, *this);
    }
    m_state = std::move(newState);
    if (m_state.get())
    {
        m_state->Enter(context, *this);
        DEBUG_INFO(std::format("[State Manager] Entering: {}", m_state->GetStateName()));
    }
    else
    {
        DEBUG_INFO("[State Manager] State set to null");
    }
}

void Struktur::GameResource::StateManager::Update(GameContext& context)
{
    m_state->Update(context, *this);
}

void Struktur::GameResource::StateManager::Render(GameContext& context)
{
    m_state->Render(context, *this);
}

void Struktur::GameResource::StateManager::ReleaseState(GameContext& context)
{
    m_state->Exit(context, *this);
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
