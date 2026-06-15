#include "EventSystem.h"

#include "Engine/ECS/System/WrenScriptSystem.h"
#include "Engine/GameContext.h"

void Struktur::System::EventSystem::Update(GameContext& context)
{
	auto& registry                 = context.GetRegistry();
	auto& stateManager             = context.GetWrenStateManager();
	auto& systemManager            = context.GetSystemManager();
	auto& eventManager             = context.GetEventManager();
	WrenScriptSystem& scriptSystem = systemManager.GetSystem<WrenScriptSystem>();
	auto view                      = registry.view<Component::WrenScript>();

	for (auto& event : eventManager.Drain())
	{
		stateManager.SendEvent(context, event);

		for (auto entity : view)
		{
			auto& script = view.get<Component::WrenScript>(entity);
			scriptSystem.SendEvent(context, entity, script, event);
		}
	}
}
