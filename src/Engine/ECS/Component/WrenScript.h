#pragma once

#include "wren.hpp"
#include <string>
#include <vector>

#include "Engine/Scripting/WrenValueWrapper.h"

namespace Struktur::Component
{
struct WrenScript
{
    std::string scriptPath;                    // Path to .wren file
    std::string className;                     // Wren class name
    std::vector<Wren::Item> constructorArgs;   // Optional constructor arguments (e.g., NPC name)
    
    //TODO Clean up what we actually need handles for
    WrenHandle* classHandle = nullptr;          // Handle to Wren class
    WrenHandle* instanceHandle = nullptr;       // Handle to Wren instance
    WrenHandle* createMethodHandle = nullptr;   // Cached handle to Create()
    WrenHandle* updateMethodHandle = nullptr;   // Cached handle to Update(dt)
    WrenHandle* onDestroyMethodHandle = nullptr;// Cached handle to OnDestroy()
    WrenHandle* onEventMethodHandle = nullptr;  // Cached handle to OnEvent(event)
    
    bool isInitialised = false;                 // Initialization status
    bool hasError = false;                      // Error flag for fallback behavior
    std::string errorMessage;                   // Last error message
};

} // namespace Struktur::Component
