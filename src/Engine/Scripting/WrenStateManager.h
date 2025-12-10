#pragma once

#include "WrenScriptEngine.h"
#include <string>

namespace Struktur
{

    class GameContext;

    namespace Wren
    {
        // C++ bridge to Wren state management system
        // Holds handle to Wren Game instance and delegates all calls to Wren
        class WrenStateManager
        {
        public:
            WrenStateManager() 
                : m_rootStateInstanceHandle(nullptr)
                , m_updateMethodHandle(nullptr)
                , m_renderMethodHandle(nullptr)
                , m_isInitialised(false)
            {}
            
            ~WrenStateManager() {}
            
            // Initialise the Wren state system
            // Loads Main.wren, calls Boot.initialise(initialStateName)
            // Returns Game instance handle
            // Called before the game window is created and will initial the project settings.
            bool Initialise(GameContext& context);

            // Called after the all the systems are initialised 
            void Start(GameContext& context);
            
            // Update the game state (calls Game.update(dt) in Wren)
            void Update(GameContext& context);
            
            // Optional: Render (calls Game.render() in Wren)
            void Render(GameContext& context);
            
            // Send event to current state
            void SendEvent(GameContext& context, const std::string& eventType, const std::unordered_map<std::string, double>& eventData);
            
            // Clean up
            void Shutdown(GameContext& context);
            
            // Query state (for debugging)
            std::string GetCurrentStateName(GameContext& context);
            
            // Getters
            bool IsInitialised() const { return m_isInitialised; }
            WrenHandle* GetGameHandle() const { return m_rootStateInstanceHandle; }

        private:
            WrenHandle* m_rootStateInstanceHandle;  // Handle to Wren Game object
            WrenHandle* m_updateMethodHandle;       // Cached Game.update(_) method
            WrenHandle* m_renderMethodHandle;       // Cached Game.render() method
            WrenHandle* m_startMethodHandle;        // Cached Game.start() method
            WrenHandle* m_quitMethodHandle;         // Cached Game.quit() method
            
            bool m_isInitialised;
            
        };
    }
}
