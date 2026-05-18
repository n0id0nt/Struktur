#include "raylib.h"
#include "SystemManager.h"
#include "Engine/GameContext.h"
#include "Debug/Profiling/Profiler.h"

void Struktur::System::SystemManager::Update(GameContext& context)
{
	for (auto& systemId : m_updateSystems)
	{
		auto* system = m_systemMap[systemId].get();
		PROFILE_SCOPE(system->Name());
		system->Update(context);
	}
}

void Struktur::System::SystemManager::Render(GameContext& context)
{
	for (auto& systemId : m_renderSystems)
	{
		auto* system = m_systemMap[systemId].get();
		PROFILE_SCOPE(system->Name());
		system->Update(context);
	}
}
