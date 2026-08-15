#pragma once

#include <string>
#include <unordered_map>
#include <vector>
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
#ifdef DEBUG
// A getter (and optionally matching setter) tagged #!export on a script class - see
// WrenScriptComponentRegistry::ResolveExportedFields for how these are discovered. Editor-only:
// the inspector uses these to show/edit a script's exported variables, similar to Godot's @export.
struct WrenExportedField
{
	std::string name;
	bool hasSetter            = false;
	WrenHandle* getterHandle  = nullptr;
	WrenHandle* setterHandle  = nullptr;
};
#endif

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
	void EnsureExportReflectionBootstrap(GameContext& context);

	// Track which scripts have been loaded
	std::unordered_map<std::string, time_t> m_fileModificationTimes;

	// Handles for the small Wren-side helper module that walks a class's Attributes to find
	// #!export-tagged getters (see WrenScriptComponentRegistry.cpp) - loaded once, lazily.
	WrenHandle* m_exportReflectionClassHandle = nullptr;
	WrenHandle* m_exportReflectionCallHandle  = nullptr;
#endif
	std::unordered_map<std::string, WrenScriptComponent> m_scriptComponents;
};
}  // namespace Wren
}  // namespace Struktur
