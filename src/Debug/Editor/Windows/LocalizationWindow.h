#pragma once

#include <string>
#include <vector>

#include "EditorWindow.h"

namespace Struktur::Debug
{
// Read-only audit table over Struktur::Localization::LocalizationManager: one row per key, one column per
// loaded language, so a missing translation (e.g. fr.json's still-missing "menu.quit") is visible at a glance
// instead of only surfacing one key at a time via the "log once" warning when a player happens to hit that
// screen in that language - the same coverage-at-a-glance shape the eventual Excel export will have (one row
// per key, one column per language). Never edits strings - translations stay authored externally (JSON today,
// Excel later per the localization plan), matching how ResourceManagerWindow observes ResourceManager without
// ever mutating pool state itself.
class LocalizationWindow : public EditorWindow
{
public:
	LocalizationWindow();

	void Render(GameContext& context) override;

private:
	void RenderToolbar(GameContext& context);
	void RenderTable(GameContext& context, const std::vector<std::string>& languages,
	                 const std::vector<std::string>& keys);

	char m_keyFilter[128] = {};
	bool m_missingOnly    = false;
};
}  // namespace Struktur::Debug
