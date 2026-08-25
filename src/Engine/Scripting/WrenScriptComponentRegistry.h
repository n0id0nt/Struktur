#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <wren.hpp>

#include "Engine/Scripting/WrenExportedFields.h"
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
#ifdef DEBUG
	std::vector<WrenExportedField> exportedFields;
	bool exportedFieldsResolved = false;
#endif
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

	// Editor support: lazily discovers and caches #!export-tagged getters/setters for a script class.
	// Empty (never null) if the class has no exports or isn't loaded.
	const std::vector<WrenExportedField>& GetExportedFields(GameContext& context, const std::string& className);

private:
	time_t GetFileModificationTime(const std::string& path);
	void ResolveExportedFields(GameContext& context, WrenScriptComponent& scriptComponent);

	// Track which scripts have been loaded
	std::unordered_map<std::string, time_t> m_fileModificationTimes;
#endif
	std::unordered_map<std::string, WrenScriptComponent> m_scriptComponents;
};
}  // namespace Wren
}  // namespace Struktur
