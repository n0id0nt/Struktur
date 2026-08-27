#include "LocalizationWindow.h"

#include <algorithm>

#include "Engine/GameContext.h"
#include "Engine/Localization/LocalizationManager.h"

namespace Struktur::Debug
{
LocalizationWindow::LocalizationWindow()
    : EditorWindow("Localization")
{
}

void LocalizationWindow::RenderToolbar(GameContext& context)
{
	Localization::LocalizationManager& localization = context.GetLocalizationManager();

	std::vector<std::string> languages = localization.GetAvailableLanguages();
	std::sort(languages.begin(), languages.end());

	std::string activeLanguage = localization.GetActiveLanguage();
	ImGui::SetNextItemWidth(120.0f);
	if (ImGui::BeginCombo("Active Language", activeLanguage.c_str()))
	{
		for (const std::string& language : languages)
		{
			bool isSelected = language == activeLanguage;
			if (ImGui::Selectable(language.c_str(), isSelected))
			{
				// Changes what the running game actually shows right now (any live UIManager label built from
				// Localization.get()) - a genuine runtime effect, but never touches the JSON files on disk, so
				// it stays within this window's "never edits content" contract.
				localization.SetActiveLanguage(language);
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Switches the live game's active language - handy for previewing, doesn't touch any file");
	}

	ImGui::SameLine();
	if (ImGui::Button("Reload"))
	{
		// Re-reads every language file from disk - lets an external JSON edit show up here (and in the running
		// game) without restarting the process.
		localization.Reload();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Re-reads the localization manifest and every language file from disk");
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputTextWithHint("##KeyFilter", "Filter by key...", m_keyFilter, sizeof(m_keyFilter));

	ImGui::SameLine();
	ImGui::Checkbox("Missing only", &m_missingOnly);

	ImGui::Separator();
}

void LocalizationWindow::RenderTable(GameContext& context, const std::vector<std::string>& languages,
                                     const std::vector<std::string>& keys)
{
	Localization::LocalizationManager& localization = context.GetLocalizationManager();

	int columnCount = 1 + (int)languages.size();
	if (!ImGui::BeginTable("LocalizationTable", columnCount,
	                       ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
	                           ImGuiTableFlags_ScrollY,
	                       ImVec2(0, 400)))
	{
		return;
	}

	ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 260.0f);
	for (const std::string& language : languages)
	{
		ImGui::TableSetupColumn(language.c_str(), ImGuiTableColumnFlags_WidthStretch);
	}
	ImGui::TableHeadersRow();

	for (const std::string& key : keys)
	{
		if (m_keyFilter[0] != '\0' && key.find(m_keyFilter) == std::string::npos)
		{
			continue;
		}

		// Collect every language's raw value up front (rather than per-column) so "missing only" can decide
		// whether to render this row at all before any column has been written.
		std::vector<std::string> values(languages.size());
		std::vector<bool> present(languages.size());
		bool anyMissing = false;
		for (size_t i = 0; i < languages.size(); ++i)
		{
			present[i] = localization.TryGetRawString(languages[i], key, values[i]);
			anyMissing |= !present[i];
		}

		if (m_missingOnly && !anyMissing)
		{
			continue;
		}

		ImGui::PushID(key.c_str());
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(key.c_str());

		for (size_t i = 0; i < languages.size(); ++i)
		{
			ImGui::TableNextColumn();
			if (present[i])
			{
				ImGui::TextWrapped("%s", values[i].c_str());
			}
			else
			{
				ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "(missing)");
			}
		}

		ImGui::PopID();
	}

	ImGui::EndTable();
}

void LocalizationWindow::Render(GameContext& context)
{
	if (!m_isVisible)
	{
		return;
	}

	ImGui::Begin(m_name.c_str(), &m_isOpen);

	Localization::LocalizationManager& localization = context.GetLocalizationManager();

	RenderToolbar(context);

	std::vector<std::string> languages = localization.GetAvailableLanguages();
	std::sort(languages.begin(), languages.end());
	std::vector<std::string> keys = localization.GetAllKeys();

	ImGui::Text("%zu keys across %zu languages", keys.size(), languages.size());
	RenderTable(context, languages, keys);

	ImGui::End();
}
}  // namespace Struktur::Debug
