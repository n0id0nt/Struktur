#pragma once

#include <memory>

#include "Engine/Scripting/WrenScriptEngine.h"
#include "Engine/Scripting/WrenStateManager.h"

namespace Struktur::Wren
{
// The Wren library's services. A game's context holds one and returns its members.
struct Services
{
	Services();
	// Destroys the services, in order.
	void Shutdown();

	std::unique_ptr<WrenScriptEngine> scriptEngine;
	std::unique_ptr<WrenStateManager> stateManager;
};
}  // namespace Struktur::Wren
