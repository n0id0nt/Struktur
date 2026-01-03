#include "WrenScriptEngine.h"
#include "Engine/GameContext.h"
#include <filesystem>

namespace Struktur::Wren {

void WrenScriptEngine::Initialise(GameContext& context)
{   
    WrenConfiguration config;
    wrenInitConfiguration(&config);
    
    config.errorFn = OnWrenError;
    config.writeFn = OnWrenWrite;
    config.loadModuleFn = OnLoadModule;
    config.bindForeignMethodFn = OnBindForeignMethod;
    config.bindForeignClassFn = OnBindForeignClass;
    
    m_vm = wrenNewVM(&config);
    wrenSetUserData(m_vm, &context);
    
    DEBUG_INFO("Wren VM initialised");
    DEBUG_INFO("Registered %zu method bindings", Wren::GetMethodBindings().size());
    DEBUG_INFO("Registered %zu class bindings", Wren::GetClassBindings().size());
    DEBUG_INFO("Registered %zu enum bindings", Wren::GetEnumBindings().size());
    DEBUG_INFO("Registered %zu constant bindings", Wren::GetConstantBindings().size());
}

void WrenScriptEngine::Shutdown()
{
    if (m_vm)
    {
        wrenFreeVM(m_vm);
        m_vm = nullptr;
    }
}

bool WrenScriptEngine::InterpretString(const char* module, const char* source)
{
    if (!m_vm)
    {
        DEBUG_ERROR("Wren VM not initialised");
        return false;
    }
    
    WrenInterpretResult result = wrenInterpret(m_vm, module, source);
    
    if (result == WREN_RESULT_COMPILE_ERROR)
    {
        DEBUG_ERROR("Wren compile error in module: %s", module);
        return false;
    }
    else if (result == WREN_RESULT_RUNTIME_ERROR)
    {
        DEBUG_ERROR("Wren runtime error in module: %s", module);
        return false;
    }
    else if (result != WREN_RESULT_SUCCESS)
    {
        DEBUG_ERROR("Wren error in module: %s", module);
        return false;
    }
    
    return true;
}

bool WrenScriptEngine::InterpretFile(const char* path)
{
    if (!m_vm)
    {
        DEBUG_ERROR("Wren VM not initialised");
        return false;
    }
    
    // Load file
    std::ifstream file(path);
    if (!file.is_open())
    {
        DEBUG_ERROR("Failed to open Wren file: %s", path);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    // Extract module name from path (remove extension)
    std::filesystem::path filepath(path);
    std::string module = filepath.stem().string();
    
    return InterpretString(module.c_str(), source.c_str());
}

// ============================================================================
// CALLBACK IMPLEMENTATIONS
// ============================================================================

void WrenScriptEngine::OnWrenError(WrenVM* vm, WrenErrorType type, const char* module, int line, const char* message)
{
    switch (type)
    {
    case WREN_ERROR_COMPILE:
        DEBUG_ERROR("[Wren Compile] %s:%d: %s", module, line, message);
        break;
    case WREN_ERROR_RUNTIME:
        DEBUG_ERROR("[Wren Runtime] %s", message);
        break;
    case WREN_ERROR_STACK_TRACE:
        DEBUG_ERROR("[Wren Trace] %s:%d: %s", module, line, message);
        break;
    }
}

void WrenScriptEngine::OnWrenWrite(WrenVM* vm, const char* text)
{
    if (strcmp(text, "\n") == 0) return;
    DEBUG_INFO("[Wren Print] %s", text);
}

WrenLoadModuleResult WrenScriptEngine::OnLoadModule(WrenVM* vm, const char* name)
{
    WrenLoadModuleResult result = {};
    
	// Built-in optional modules
    if (strcmp(name, "meta") == 0 || strcmp(name, "random") == 0)
    {
        result.source = NULL;
        return result;
    }
    
    // YOUR CUSTOM MODULE
    if (strcmp(name, "reflect") == 0)
    {
        result.source = wrenReflectSource();
        result.onComplete = NULL;  // Source is static, no cleanup needed
        return result;
    }

    // Try multiple paths for module loading
    std::vector<std::string> searchPaths = {
        std::string("assets/scripts/") + name + ".wren",
        std::string("assets/scripts/bindings/") + name + ".wren",
    };
    
    std::string source;
    bool found = false;
    
    for (const auto& path : searchPaths)
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            source = buffer.str();
            found = true;
            DEBUG_INFO("Loaded Wren module: %s from %s", name, path.c_str());
            break;
        }
    }
    
    if (!found)
    {
        DEBUG_ERROR("Failed to load Wren module: %s", name);
        return result;
    }
    
    // Allocate memory for Wren (Wren will free it)
    char* source_copy = new char[source.size() + 1];
    strcpy(source_copy, source.c_str());
    
    result.source = source_copy;
    result.onComplete = [](WrenVM* vm, const char* name, WrenLoadModuleResult result) {
        delete[] result.source;
    };
    
    return result;
}

WrenForeignMethodFn WrenScriptEngine::OnBindForeignMethod(WrenVM *vm, const char *module, const char *className, bool isStatic, const char *signature)
{
    WrenForeignMethodFn method;
    if (strcmp(module, "reflect") == 0)
    {
        method = wrenReflectBindForeignMethod(vm, className, isStatic, signature);
    }
    else if (!isStatic && strncmp(signature, "init ", 5) == 0)
    {
        method = Wren::FindClass(module, className).allocate;
    }
    else
    {
        method = Wren::FindMethod(module, className, isStatic, signature);
    }

    // check for inherited methods
    if (!method)
    {
        for (const auto& binding : GetClassBindings())
        {
            if (binding.moduleName == module && binding.className == className)
            {
                if (!binding.parentClassName.empty())
                {
                    method = Wren::FindMethod(module, binding.parentClassName.c_str(), isStatic, signature);
                }
                break;
            }
        }
    }

    if (!method)
    {
        DEBUG_WARNING("Wren binding not found: %s.%s.%s (static=%d)", module, className, signature, isStatic);
    }

    return method;
}

WrenForeignClassMethods WrenScriptEngine::OnBindForeignClass(WrenVM *vm, const char *module, const char *className)
{
    return Wren::FindClass(module, className);
}
}
