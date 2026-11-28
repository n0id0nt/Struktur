#include "SystemManager.h"

#include "Debug/Profiling/Profiler.h"
#include "Engine/GameContext.h"

void Struktur::System::SystemManager::RunPhase(GameContext& context, const std::vector<std::type_index>& ids,
                                               Phase& phase)
{
	if (phase.builtVersion != m_scheduleVersion)
	{
		phase.builtVersion = m_scheduleVersion;
		// Rebuilt whenever a system is added; declarations are fixed per system type.
		phase.systems.clear();
		std::vector<SystemAccess> accesses;
		for (const std::type_index& id : ids)
		{
			ISystem* system = m_systemMap[id].get();
			phase.systems.push_back(system);
			accesses.push_back(system->Access());
		}
		phase.steps = BuildSchedule(accesses);
		for (const ScheduleStep& step : phase.steps)
		{
			if (!step.mainThread && step.systems.size() > 1)
			{
				std::string names;
				for (size_t index : step.systems)
				{
					names += (names.empty() ? "" : ", ") + phase.systems[index]->Name();
				}
				DEBUG_INFO("Systems scheduled side by side: %s", names.c_str());
			}
		}
	}
	Jobs::JobSystem& jobs = context.GetJobSystem();
	const bool parallel   = m_parallel && jobs.GetWorkerCount() > 0;
	for (const ScheduleStep& step : phase.steps)
	{
		if (step.mainThread || !parallel || step.systems.size() == 1)
		{
			for (size_t index : step.systems)
			{
				ISystem* system = phase.systems[index];
				PROFILE_SCOPE(system->Name());
				system->Update(context);
			}
		}
		else
		{
			// The profiler is main-thread only, so a parallel batch shows as one scope: "Batch: A | B".
			std::string name = "Batch:";
			for (size_t index : step.systems)
			{
				name += " " + phase.systems[index]->Name();
			}
			PROFILE_SCOPE(name);
			for (size_t index : step.systems)
			{
				phase.systems[index]->Prepare(context);
			}
			RunSchedule(std::vector<ScheduleStep>{step}, jobs, true, [&phase, &context](size_t index) { phase.systems[index]->Update(context); },
			            [] {});
		}
		if (!step.mainThread)
		{
			m_commands.Apply(context);
		}
	}
}

void Struktur::System::SystemManager::Update(GameContext& context)
{
	RunPhase(context, m_updateSystems, m_updatePhase);
}

void Struktur::System::SystemManager::Render(GameContext& context)
{
	RunPhase(context, m_renderSystems, m_renderPhase);
}

void Struktur::System::SystemManager::FixedUpdate(GameContext& context)
{
	RunPhase(context, m_fixedUpdateSystems, m_fixedUpdatePhase);
}
