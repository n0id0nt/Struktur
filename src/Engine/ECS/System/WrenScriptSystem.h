#pragma once

#include <unordered_map>
#include <vector>

#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Event/Event.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "entt/entt.hpp"

namespace Struktur
{

class GameContext;

namespace System
{
class WrenScriptSystem : public ISystem
{
   public:
	WrenScriptSystem() {}

	// Initialise a script component (load and instantiate)
	bool InitialiseScript(GameContext& context, entt::entity entity, Component::WrenScript& className);

	// Call Start() on the script
	bool CallStart(GameContext& context, Component::WrenScript& script);

	// Queue a just-created script to be initialised and started at the top of the next Update() - see GameObjectManager::OnScriptConstruct.
	void QueuePendingInitialise(entt::entity entity);

	// Update all scripted entities
	void Update(GameContext& context) override;

	// Destroy a script (call OnDestroy and release handles)
	void DestroyScript(GameContext& context, entt::entity entity, Component::WrenScript& script);

	// Send event to script
	void SendEvent(GameContext& context, entt::entity entity, Component::WrenScript& script,
	               const Event::Event& eventData);

	std::string Name() const override
	{
		return "Wren Script System";
	}

   private:
	std::vector<entt::entity> m_pendingInitialise;
};
}  // namespace System
}  // namespace Struktur
