#include "WrenServices.h"

namespace Struktur::Wren
{
Services::Services()
{
	scriptEngine = std::make_unique<WrenScriptEngine>();
	stateManager = std::make_unique<WrenStateManager>();
}

void Services::Shutdown()
{
	scriptEngine.reset();
	stateManager.reset();
}
}  // namespace Struktur::Wren
