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
        private:
            WrenHandle* m_rootStateInstanceHandle;   // Handle to Wren Game object
            WrenHandle* m_updateMethodHandle;   // Cached Game.update(_) method
            WrenHandle* m_renderMethodHandle;   // Cached Game.render() method (optional)
            
            bool m_isInitialized;
            
        public:
            WrenStateManager() 
                : m_rootStateInstanceHandle(nullptr)
                , m_updateMethodHandle(nullptr)
                , m_renderMethodHandle(nullptr)
                , m_isInitialized(false)
            {}
            
            ~WrenStateManager() {}
            
            // Initialize the Wren state system
            // Loads Boot.wren, calls Boot.initialize(initialStateName)
            // Returns Game instance handle
            bool Initialize(GameContext& context, const std::string& initialStateName);
            
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
            bool IsInitialized() const { return m_isInitialized; }
            WrenHandle* GetGameHandle() const { return m_rootStateInstanceHandle; }
        };
    }
}
