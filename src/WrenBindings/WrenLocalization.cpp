#include "Engine/GameContext.h"
#include "Engine/Localization/LocalizationManager.h"
#include "Engine/Scripting/WrenBindingRegistry.h"
#include "wren.hpp"

// ============================================================================
// LOCALIZATION BINDINGS
// ============================================================================

// Localization.loadManifest(manifestPath) -> Bool
void wren_LocalizationLoadManifest(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	const char* manifestPath = wrenGetSlotString(vm, 1);
	bool success             = localization.LoadFromManifest(manifestPath);
	wrenSetSlotBool(vm, 0, success);
}

// Localization.loadLanguage(languageCode, jsonPath) -> Bool
void wren_LocalizationLoadLanguage(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	const char* languageCode = wrenGetSlotString(vm, 1);
	const char* jsonPath     = wrenGetSlotString(vm, 2);
	bool success             = localization.LoadLanguage(languageCode, jsonPath);
	wrenSetSlotBool(vm, 0, success);
}

// Localization.get(key) -> String
void wren_LocalizationGet(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	const char* key      = wrenGetSlotString(vm, 1);
	std::string value    = localization.GetString(key);
	wrenSetSlotString(vm, 0, value.c_str());
}

// Localization.hasString(key) -> Bool
void wren_LocalizationHasString(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	const char* key = wrenGetSlotString(vm, 1);
	bool exists      = localization.HasString(key);
	wrenSetSlotBool(vm, 0, exists);
}

// Localization.setLanguage(languageCode)
void wren_LocalizationSetLanguage(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	const char* languageCode = wrenGetSlotString(vm, 1);
	localization.SetActiveLanguage(languageCode);
}

// Localization.getLanguage() -> String
void wren_LocalizationGetLanguage(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	wrenSetSlotString(vm, 0, localization.GetActiveLanguage().c_str());
}

// Localization.getAvailableLanguages() -> List<String>
void wren_LocalizationGetAvailableLanguages(WrenVM* vm)
{
	Struktur::GameContext* context = static_cast<Struktur::GameContext*>(wrenGetUserData(vm));
	Struktur::Localization::LocalizationManager& localization = context->GetLocalizationManager();

	std::vector<std::string> languages = localization.GetAvailableLanguages();

	// Extra slot needed for each element inserted - see WrenFileSystem.cpp's own identical pattern.
	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);
	for (const std::string& language : languages)
	{
		wrenSetSlotString(vm, 1, language.c_str());
		wrenInsertInList(vm, 0, -1, 1);
	}
}

WREN_BINDING_MODULE(Localization)
{
	WREN_CLASS_STATIC(registry, "localization", "Localization", "loadManifest(_)", wren_LocalizationLoadManifest,
	                  "Loads every language listed in a localization manifest file");
	WREN_CLASS_STATIC(registry, "localization", "Localization", "loadLanguage(_,_)", wren_LocalizationLoadLanguage,
	                  "Loads (or merges into) one language from its own localization file");
	WREN_CLASS_STATIC(registry, "localization", "Localization", "get(_)", wren_LocalizationGet,
	                  "Gets the localized string for a key in the active language (falling back to the default "
	                  "language, then to \"[key]\", if missing)");
	WREN_CLASS_STATIC(registry, "localization", "Localization", "hasString(_)", wren_LocalizationHasString,
	                  "Checks whether a key has a string in the active or default language");
	WREN_CLASS_STATIC(registry, "localization", "Localization", "setLanguage(_)", wren_LocalizationSetLanguage,
	                  "Sets the active language (must already be loaded)");
	WREN_CLASS_STATIC(registry, "localization", "Localization", "getLanguage()", wren_LocalizationGetLanguage,
	                  "Gets the active language code");
	WREN_CLASS_STATIC(registry, "localization", "Localization", "getAvailableLanguages()",
	                  wren_LocalizationGetAvailableLanguages, "Gets every currently-loaded language code");
}
