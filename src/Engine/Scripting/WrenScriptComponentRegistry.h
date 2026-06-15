#pragma once

#include <string>
#include <unordered_map>
#include <wren.hpp>
#ifdef DEBUG
	#include <entt/entt.hpp>
#endif

namespace Struktur
{
class GameContext;

#ifdef DEBUG
namespace Component
{
struct WrenScript;
}
#endif

namespace Wren
{
struct WrenScriptComponent
{
	std::string module;
	std::string className;
	WrenHandle* classHandle           = nullptr;
	WrenHandle* startMethodHandle     = nullptr;
	WrenHandle* updateMethodHandle    = nullptr;
	WrenHandle* onDestroyMethodHandle = nullptr;
	WrenHandle* onEventMethodHandle   = nullptr;
	bool isInitialised                = false;
};

class WrenScriptComponentRegistry
{
   public:
	WrenScriptComponentRegistry()
	    : m_scriptComponents()
	{
	}
	~WrenScriptComponentRegistry() {}

	void RegisterScriptComponent(std::string module, std::string className);
	bool LoadAllScriptComponents(GameContext& context);
	WrenScriptComponent* TryGetScriptComponent(std::string className);
	void Clear(GameContext& context);

#ifdef DEBUG
	// Hot reload support
	void CheckForScriptChanges(GameContext& context);
	void ReloadScript(GameContext& context, entt::entity entity, Component::WrenScript& script);

   private:
	time_t GetFileModificationTime(const std::string& path);

	// Track which scripts have been loaded
	std::unordered_map<std::string, time_t> m_fileModificationTimes;
#endif
	std::unordered_map<std::string, WrenScriptComponent> m_scriptComponents;
};
}  // namespace Wren
}  // namespace Struktur
