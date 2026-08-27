#pragma once

#include <string>
#include <vector>

#include "Engine/Scripting/WrenScriptComponentRegistry.h"
#include "Engine/Scripting/WrenValueWrapper.h"
#include "wren.hpp"

namespace Struktur::Component
{
struct WrenScript
{
	std::string className;
	std::vector<Wren::WrenItem> constructorArgs;

	WrenHandle* instanceHandle                 = nullptr;
	Wren::WrenScriptComponent* scriptComponent = nullptr;

	bool isInitialised = false;
	bool hasError      = false;
	std::string errorMessage;
#ifdef DEBUG
	std::string filePath;
#endif
};

}  // namespace Struktur::Component
