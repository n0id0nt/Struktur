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
        private:
            
            // Track which scripts have been loaded
            //TODO make this a debug only feature
            std::unordered_map<std::string, time_t> fileModificationTimes;
            
        public:
            WrenScriptSystem() {}
            
            // Initialise a script component (load and instantiate)
            bool InitialiseScript(GameContext& context, entt::entity entity, 
                                Component::WrenScript& script);
            
            // Call Create() on the script
            bool CallCreate(GameContext& context, entt::entity entity, 
                        Component::WrenScript& script);
            
            // Update all scripted entities
            void Update(GameContext& context) override;
            
            // Destroy a script (call OnDestroy and release handles)
            void DestroyScript(GameContext& context, entt::entity entity, 
                            Component::WrenScript& script);
            
            // Send event to script
            void SendEvent(GameContext& context, entt::entity entity, 
                        Component::WrenScript& script, const std::string& eventType,
                        const std::unordered_map<std::string, double>& eventData);
            
            // Hot reload support
            void CheckForScriptChanges(GameContext& context);
            void ReloadScript(GameContext& context, entt::entity entity, 
                            Component::WrenScript& script);
            
        private:
            // Helper to get method handle
            WrenHandle* GetMethodHandle(WrenVM* vm, WrenHandle* classHandle, 
                                    const char* signature);
            
            // Helper to get file modification time
            time_t GetFileModificationTime(const std::string& path);
        };

    }
}
