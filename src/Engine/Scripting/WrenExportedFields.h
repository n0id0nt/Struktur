#pragma once

#include <string>
#include <vector>
#include <wren.hpp>

namespace Struktur
{
class GameContext;
}

namespace Struktur::Wren
{
struct WrenItem;

#ifdef DEBUG
// A getter (and optionally matching setter) tagged #!export on a Wren class - see
// ExportedFields::DiscoverExportedFields for how these are found. Shared identity/value-access layer:
// WrenScriptComponentRegistry (ECS script components, keyed by class name) and WrenStateManager (state
// instances, keyed by class name too) both discover/cache a class's fields through the same function and read/
// write them on a live instance through the same two calls below, instead of each re-embedding the discovery
// bootstrap or the get/set call pattern.
struct WrenExportedField
{
	std::string name;
	bool hasSetter           = false;
	WrenHandle* getterHandle = nullptr;
	WrenHandle* setterHandle = nullptr;
};

namespace ExportedFields
{
// Walks a class's compile-time Attributes (stock Wren feature, see third_party/wren doc/site/classes.markdown
// "Attributes") to find every getter tagged #!export, and whether a matching setter exists - one
// WrenExportedField per hit. Interprets a small lazily-loaded Wren bootstrap module the first time this runs
// across the whole process (not per class), so it's cheap to call again - callers still cache the result per
// class themselves (both existing callers already do; there's no reason to discover the same class twice).
std::vector<WrenExportedField> DiscoverExportedFields(GameContext& context, WrenHandle* classHandle,
                                                       const std::string& classNameForLogging);

// Reads/writes one exported field on a live instance - any Wren object, not ECS/component-specific. Both
// false on failure (field has no getter/setter handle, or the Wren call itself failed - see DEBUG_ERROR inside
// for which).
bool GetValue(WrenVM* vm, WrenHandle* instanceHandle, const WrenExportedField& field, WrenItem& out_value);
bool SetValue(WrenVM* vm, WrenHandle* instanceHandle, const WrenExportedField& field, const WrenItem& value);

// Releases a class's own getter/setter handles - call before dropping/clearing a cached field list.
void ReleaseFields(WrenVM* vm, std::vector<WrenExportedField>& fields);

// Releases the shared discovery bootstrap's own handles - call once, e.g. when the owning WrenVM shuts down.
void ShutdownBootstrap(WrenVM* vm);
}  // namespace ExportedFields
#endif
}  // namespace Struktur::Wren
