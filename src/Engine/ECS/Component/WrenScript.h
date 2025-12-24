#pragma once

#include "wren.hpp"
#include <string>
#include <vector>

#include "Engine/Scripting/WrenValueWrapper.h"
#include "Engine/Scripting/WrenScriptComponentRegistry.h"

namespace Struktur::Component
{
struct WrenScript
{
    std::string className;
    std::vector<Wren::Item> constructorArgs;
    
    WrenHandle* instanceHandle = nullptr;
    Wren::WrenScriptComponent* scriptComponent = nullptr;
    
    bool isInitialised = false;
    bool hasError = false;
    std::string errorMessage;
};

} // namespace Struktur::Component
