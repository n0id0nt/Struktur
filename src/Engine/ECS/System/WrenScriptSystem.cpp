#include "WrenScriptSystem.h"
#include "Engine/GameContext.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace Struktur::System {

bool WrenScriptSystem::InitializeScript(GameContext& context, entt::entity entity, 
                                       Component::WrenScript& script)
{
    Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
    WrenVM* vm = scriptEngine.GetVM();
    if (!vm)
    {
        DEBUG_ERROR("Wren VM not initialized");
        script.hasError = true;
        script.errorMessage = "Wren VM not initialized";
        return false;
    }
    
    // Load the script file
    std::ifstream file(script.scriptPath);
    if (!file.is_open()) {
        DEBUG_ERROR("Failed to open script file: %s", script.scriptPath.c_str());
        script.hasError = true;
        script.errorMessage = "Failed to open script file";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    // Interpret the script
    WrenInterpretResult result = wrenInterpret(vm, script.scriptPath.c_str(), source.c_str());
    
    if (result != WREN_RESULT_SUCCESS)
    {
        DEBUG_ERROR("Failed to compile script: %s", script.scriptPath.c_str());
        script.hasError = true;
        script.errorMessage = "Compilation failed";
        return false;
    }
    
    // Get the class
    wrenEnsureSlots(vm, 1);
    wrenGetVariable(vm, script.scriptPath.c_str(), script.className.c_str(), 0);
    script.classHandle = wrenGetSlotHandle(vm, 0);
    
    // Instantiate the class
    int numberOfSlots = script.constructorArgs.empty() ? 2 : 3;
    wrenEnsureSlots(vm, numberOfSlots);
    wrenSetSlotHandle(vm, 0, script.classHandle);
    
    // Pass entity ID and constructor args to constructor
    wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
    
    // Call constructor: new(entity) or new(entity, args)
    WrenHandle* constructMethod;
    if (!script.constructorArgs.empty())
    {
        // If constructor args exist, create signature with 2 params
        wrenSetSlotString(vm, 2, script.constructorArgs.c_str());
        constructMethod = wrenMakeCallHandle(vm, "new(_,_)");
    }
    else
    {
        constructMethod = wrenMakeCallHandle(vm, "new(_)");
    }
    
    WrenInterpretResult callResult = wrenCall(vm, constructMethod);
    wrenReleaseHandle(vm, constructMethod);
    
    if (callResult != WREN_RESULT_SUCCESS)
    {
        DEBUG_ERROR("Failed to instantiate Wren class: %s", script.className.c_str());
        script.hasError = true;
        script.errorMessage = "Instantiation failed";
        return false;
    }
    
    // Store instance handle (it's in slot 0 after constructor call)
    script.instanceHandle = wrenGetSlotHandle(vm, 0);
    
    // Cache method handles for performance
    script.createMethodHandle = GetMethodHandle(vm, script.classHandle, "Create(_)");
    script.updateMethodHandle = GetMethodHandle(vm, script.classHandle, "Update(_)");
    script.onDestroyMethodHandle = GetMethodHandle(vm, script.classHandle, "OnDestroy()");
    script.onEventMethodHandle = GetMethodHandle(vm, script.classHandle, "OnEvent(_)");
    
    script.isInitialized = true;
    fileModificationTimes[script.scriptPath] = GetFileModificationTime(script.scriptPath);
    
    DEBUG_INFO("Initialized Wren script: %s (%s)", script.scriptPath.c_str(), script.className.c_str());
    
    return true;
}

bool WrenScriptSystem::CallCreate(GameContext& context, entt::entity entity, 
                                 Component::WrenScript& script)
{
    if (!script.isInitialized || script.hasError)
    {
        return false;
    }
    
    if (!script.createMethodHandle)
    {
        DEBUG_WARNING("Script %s does not have a Create method", script.className.c_str());
        return true; // Not an error, just doesn't have the method
    }

    Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
    WrenVM* vm = scriptEngine.GetVM();
    
    wrenEnsureSlots(vm, 2);
    wrenSetSlotHandle(vm, 0, script.instanceHandle);
    wrenSetSlotDouble(vm, 1, static_cast<double>(entity));
    
    WrenInterpretResult result = wrenCall(vm, script.createMethodHandle);
    
    if (result != WREN_RESULT_SUCCESS)
    {
        DEBUG_ERROR("Error calling Create() on script: %s", script.className.c_str());
        script.hasError = true;
        script.errorMessage = "Create() call failed";
        return false;
    }
    
    return true;
}

void WrenScriptSystem::Update(GameContext& context)
{
    Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
    WrenVM* vm = scriptEngine.GetVM();
    if (!vm) return;
    
    auto& registry = context.GetRegistry();
    auto view = registry.view<Component::WrenScript>();
    
    for (auto entity : view)
    {
        auto& script = view.get<Component::WrenScript>(entity);
        
        // Skip if not initialized or has error
        if (script.hasError)
        {
            continue;
        }
        
        // Initialize if needed
        if (!script.isInitialized && !script.instanceHandle)
        {
            if (!InitializeScript(context, entity, script)) {
                continue;
            }
            CallCreate(context, entity, script);
        }
        
        // Skip if no update method
        if (!script.updateMethodHandle)
        {
            continue;
        }
        
        // Call Update(dt)
        wrenEnsureSlots(vm, 2);
        wrenSetSlotHandle(vm, 0, script.instanceHandle);
        wrenSetSlotDouble(vm, 1, context.GetGameData().deltaTime);
        
        WrenInterpretResult result = wrenCall(vm, script.updateMethodHandle);
        
        if (result != WREN_RESULT_SUCCESS)
        {
            DEBUG_ERROR("Error calling Update() on entity %d script: %s", 
                       static_cast<int>(entity), script.className.c_str());
            script.hasError = true;
            script.errorMessage = "Update() call failed";
        }
    }
}

void WrenScriptSystem::DestroyScript(GameContext& context, entt::entity entity, 
                                    Component::WrenScript& script)
{
    if (!script.isInitialized)
    {
        return;
    }
    
    Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
    WrenVM* vm = scriptEngine.GetVM();
    if (!vm) return;
    
    // Call OnDestroy if it exists
    if (script.onDestroyMethodHandle && script.instanceHandle)
    {
        wrenEnsureSlots(vm, 1);
        wrenSetSlotHandle(vm, 0, script.instanceHandle);
        
        WrenInterpretResult result = wrenCall(vm, script.onDestroyMethodHandle);
        
        if (result != WREN_RESULT_SUCCESS) {
            DEBUG_WARNING("Error calling OnDestroy() on script: %s", script.className.c_str());
        }
    }
    
    // Release all handles
    if (script.classHandle) wrenReleaseHandle(vm, script.classHandle);
    if (script.instanceHandle) wrenReleaseHandle(vm, script.instanceHandle);
    if (script.createMethodHandle) wrenReleaseHandle(vm, script.createMethodHandle);
    if (script.updateMethodHandle) wrenReleaseHandle(vm, script.updateMethodHandle);
    if (script.onDestroyMethodHandle) wrenReleaseHandle(vm, script.onDestroyMethodHandle);
    if (script.onEventMethodHandle) wrenReleaseHandle(vm, script.onEventMethodHandle);
    
    script.classHandle = nullptr;
    script.instanceHandle = nullptr;
    script.createMethodHandle = nullptr;
    script.updateMethodHandle = nullptr;
    script.onDestroyMethodHandle = nullptr;
    script.onEventMethodHandle = nullptr;
    script.isInitialized = false;
}

void WrenScriptSystem::SendEvent(GameContext& context, entt::entity entity, 
                                Component::WrenScript& script, const std::string& eventType,
                                const std::unordered_map<std::string, double>& eventData)
{
    if (!script.isInitialized || script.hasError || !script.onEventMethodHandle)
    {
        return;
    }
    
    Wren::WrenScriptEngine& scriptEngine = context.GetWrenScriptEngine();
    WrenVM* vm = scriptEngine.GetVM();
    
    // Create event map in Wren
    wrenEnsureSlots(vm, 3);
    wrenSetSlotHandle(vm, 0, script.instanceHandle);
    wrenSetSlotNewMap(vm, 1);
    
    // Add event type
    wrenSetSlotString(vm, 2, "type");
    wrenSetSlotString(vm, 3, eventType.c_str());
    wrenSetMapValue(vm, 1, 2, 3);
    
    // Add event data
    for (const auto& [key, value] : eventData)
    {
        wrenSetSlotString(vm, 2, key.c_str());
        wrenSetSlotDouble(vm, 3, value);
        wrenSetMapValue(vm, 1, 2, 3);
    }
    
    // Call OnEvent(eventMap)
    WrenInterpretResult result = wrenCall(vm, script.onEventMethodHandle);
    
    if (result != WREN_RESULT_SUCCESS)
    {
        DEBUG_ERROR("Error calling OnEvent() on script: %s", script.className.c_str());
    }
}

void WrenScriptSystem::CheckForScriptChanges(GameContext& context)
{
    auto& registry = context.GetRegistry();
    auto view = registry.view<Component::WrenScript>();
    
    for (auto entity : view)
    {
        auto& script = view.get<Component::WrenScript>(entity);
        
        if (!script.isInitialized) continue;
        
        time_t currentModTime = GetFileModificationTime(script.scriptPath);
        
        auto it = fileModificationTimes.find(script.scriptPath);
        if (it != fileModificationTimes.end() && currentModTime > it->second)
        {
            DEBUG_INFO("Script file changed, reloading: %s", script.scriptPath.c_str());
            ReloadScript(context, entity, script);
            fileModificationTimes[script.scriptPath] = currentModTime;
        }
    }
}

void WrenScriptSystem::ReloadScript(GameContext& context, entt::entity entity, 
                                   Component::WrenScript& script)
{
    // Destroy old script
    DestroyScript(context, entity, script);
    
    // Reset error state
    script.hasError = false;
    script.errorMessage.clear();
    
    // Reinitialize
    if (InitializeScript(context, entity, script))
    {
        CallCreate(context, entity, script);
        DEBUG_INFO("Successfully reloaded script: %s", script.scriptPath.c_str());
    }
    else
    {
        DEBUG_ERROR("Failed to reload script: %s", script.scriptPath.c_str());
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

WrenHandle* WrenScriptSystem::GetMethodHandle(WrenVM* vm, WrenHandle* classHandle, 
                                             const char* signature)
{
    wrenEnsureSlots(vm, 1);
    wrenSetSlotHandle(vm, 0, classHandle);
    
    WrenHandle* methodHandle = wrenMakeCallHandle(vm, signature);
    
    // Note: We can't easily check if the method exists without trying to call it
    // So we return the handle and let the caller handle null checks
    
    return methodHandle;
}

time_t WrenScriptSystem::GetFileModificationTime(const std::string& path)
{
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0)
    {
        return fileInfo.st_mtime;
    }
    return 0;
}

} // namespace Struktur::System
