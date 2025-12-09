#pragma once

#include "wren.hpp"
#include <string>

namespace Struktur::Component
{
struct WrenScript
{
    std::string scriptPath;           // Path to .wren file
    std::string className;            // Wren class name
    std::string constructorArgs;      // Optional constructor arguments (e.g., NPC name)
    
    //TODO Clean up what we actually need handles for
    WrenHandle* classHandle;          // Handle to Wren class
    WrenHandle* instanceHandle;       // Handle to Wren instance
    WrenHandle* createMethodHandle;   // Cached handle to Create()
    WrenHandle* updateMethodHandle;   // Cached handle to Update(dt)
    WrenHandle* onDestroyMethodHandle;// Cached handle to OnDestroy()
    WrenHandle* onEventMethodHandle;  // Cached handle to OnEvent(event)
    
    bool isInitialized;               // Initialization status
    bool hasError;                    // Error flag for fallback behavior
    std::string errorMessage;         // Last error message
    
    WrenScript()
        : classHandle(nullptr)
        , instanceHandle(nullptr)
        , createMethodHandle(nullptr)
        , updateMethodHandle(nullptr)
        , onDestroyMethodHandle(nullptr)
        , onEventMethodHandle(nullptr)
        , isInitialized(false)
        , hasError(false)
    {}
    
    WrenScript(const std::string& path, const std::string& cls, const std::string& args = "")
        : scriptPath(path)
        , className(cls)
        , constructorArgs(args)
        , classHandle(nullptr)
        , instanceHandle(nullptr)
        , createMethodHandle(nullptr)
        , updateMethodHandle(nullptr)
        , onDestroyMethodHandle(nullptr)
        , onEventMethodHandle(nullptr)
        , isInitialized(false)
        , hasError(false)
    {}
};

} // namespace Struktur::Component
