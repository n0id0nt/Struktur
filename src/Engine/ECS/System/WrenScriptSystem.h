#pragma once

#include <unordered_map>
#include <vector>

#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/ECS/SystemManager.h"
#include "Engine/Event/Event.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenValueWrapper.h"
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

#ifdef DEBUG
	// Editor support: read/write a #!export-tagged field on a live script instance by name (see
	// WrenScriptComponentRegistry::GetExportedFields for how the field list itself is discovered).
	// Both fail (return false) if the field doesn't exist, isn't exported, or - for the setter - has
	// no matching setter method.
	bool GetExportedFieldValue(GameContext& context, Component::WrenScript& script, const std::string& fieldName,
	                           Wren::WrenItem& out_value);
	bool SetExportedFieldValue(GameContext& context, Component::WrenScript& script, const std::string& fieldName,
	                           const Wren::WrenItem& value);
#endif

	std::string Name() const override
	{
		return "Wren Script System";
	}

   private:
	std::vector<entt::entity> m_pendingInitialise;
};
}  // namespace System
}  // namespace Struktur
