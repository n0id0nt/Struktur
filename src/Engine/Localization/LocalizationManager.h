#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Struktur::Localization
{
// Key -> localized-string lookup, one language resident per entry in m_languages, loaded once via
// LoadFromManifest (or LoadLanguage directly). Deliberately narrow: this only resolves a key to the string
// authored for the active language - it does NOT parse `{varName|modifiers}` placeholders itself. That's a
// separate, already-working concern (InteractState.wren's DialogueManagerHelper.processString, which calls
// through to VariableSubstitutionSystem::ApplyModifiers) - a caller feeds GetString()'s result into that
// pipeline exactly as it would any other templated string today. See the design plan for the full reasoning.
//
// Mirrors Flag::FlagManager's shape (owned by GameContext, thin Wren forwarding shims in WrenLocalization.cpp)
// and Input's "log once, safe default" pattern for anything missing (Input::WarnMissingBindingOnce) - a
// missing key or language is a real content bug worth surfacing, but should never crash or go silently blank.
class LocalizationManager
{
public:
	// Reads a manifest (see the design plan for its schema: {"defaultLanguage": "...", "languages": [{"code",
	// "name", "file"}, ...]}) and calls LoadLanguage for every entry - the shipped-build-safe way to discover
	// available languages, since FileSystem::ListDirectory/IsDirectory are #ifdef EDITOR-only and compiled out
	// of shipped builds entirely (see FileSystem.h). Sets the active language to the manifest's
	// "defaultLanguage" if that language loaded successfully.
	bool LoadFromManifest(const std::string& manifestPath);

	// Loads one language's flat {"language": "...", "strings": {"key": "value", ...}} file, merging into
	// whatever's already loaded for that code (a second call for the same code overwrites individual keys,
	// same "last write wins" semantics as Input::CreateButtonBinding et al.). Malformed JSON or a missing file
	// logs and returns false rather than throwing - same hardened-parse pattern InputConfigLoader uses.
	bool LoadLanguage(const std::string& languageCode, const std::string& jsonPath);

	// No-op (with a logged warning) if languageCode was never loaded - deliberately doesn't clear the previous
	// active language, so a bad SetActiveLanguage call can't leave GetString() resolving against nothing.
	// Re-runs LoadFromManifest against whatever path the last successful LoadFromManifest call used - lets an
	// editor tool (see LocalizationWindow) pick up edits made to the JSON files on disk without restarting the
	// process. False (with a logged warning, no-op) if LoadFromManifest was never called successfully.
	bool Reload();

	void SetActiveLanguage(const std::string& languageCode);
	const std::string& GetActiveLanguage() const
	{
		return m_activeLanguageCode;
	}
	std::vector<std::string> GetAvailableLanguages() const;

	// Every key with a string in at least one loaded language, sorted - the row set for a "key x language" audit
	// table (see LocalizationWindow). Deliberately the union, not just the default language's keys, so a key
	// that's (incorrectly) only translated in a non-default language still shows up as a row.
	std::vector<std::string> GetAllKeys() const;

	// Raw single-language lookup with no active/default fallback and no "log once" bookkeeping - unlike
	// GetString, this is only for tooling (LocalizationWindow) that wants to know exactly what's authored for
	// one specific language, not what a player would actually see. Returns false (outValue untouched) if
	// languageCode isn't loaded or has no string for key.
	bool TryGetRawString(const std::string& languageCode, const std::string& key, std::string& outValue) const;

	// Active language first; falls back to m_defaultLanguageCode if the key's missing there too; falls back to
	// "[key]" (visibly wrong, never blank, never crashes - the same reasoning as Input's own missing-binding
	// fallback) if it's missing from every loaded language. Logs once per missing key (not every call - a
	// missing key is very plausibly read every frame from a UI label or dialogue line). Returns by value (not
	// const&, unlike most of this class's other simple getters) since the fallback case constructs a new
	// string on the spot - matches Flag::FlagManager::GetStringFlag's own by-value return for the same reason.
	std::string GetString(const std::string& key) const;
	bool HasString(const std::string& key) const;

private:
	std::string m_defaultLanguageCode = "en";
	std::string m_activeLanguageCode  = "en";
	// languageCode -> (key -> localized string)
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_languages;
	mutable std::set<std::string> m_warnedMissingKeys;
	std::string m_lastManifestPath;  // empty until LoadFromManifest succeeds at least once - see Reload
};
}  // namespace Struktur::Localization
