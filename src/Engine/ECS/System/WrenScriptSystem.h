#pragma once

#include "entt/entt.hpp"
#include <unordered_map>

#include "Engine/ECS/Component/WrenScript.h"
#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/ECS/SystemManager.h"

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
            
            // Update all scripted entities
            void Update(GameContext& context) override;
            
            // Destroy a script (call OnDestroy and release handles)
            void DestroyScript(GameContext& context, entt::entity entity, Component::WrenScript& script);
            
            // Send event to script
            void SendEvent(GameContext& context, entt::entity entity, Component::WrenScript& script, const std::string& eventType,
                const std::unordered_map<std::string, double>& eventData);
        };
    }
}
