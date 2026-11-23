#include "SavesWindow.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Debug/Editor/Editor.h"
#include "Engine/Core/FileSystem.h"
#include "Engine/GameContext.h"

namespace Struktur::Debug
{
static const ImVec4 STATUS_ERROR_COLOR   = ImVec4(1.0f, 0.45f, 0.4f, 1.0f);
static const ImVec4 STATUS_SUCCESS_COLOR = ImVec4(0.5f, 0.9f, 0.5f, 1.0f);

SavesWindow::SavesWindow()
    : EditorWindow("Saves")
{
}

void SavesWindow::Render(GameContext& context)
{
	if (!ImGui::Begin(m_name.c_str(), &m_isOpen))
	{
		// Collapsed or a hidden tab - re-read the folder when it's shown again
		m_needsRefresh = true;
		ImGui::End();
		return;
	}
	if (m_needsRefresh)
	{
		RefreshVersions();
	}
	m_isPlaying = context.GetEditor().GetSettings().debugRender.playingGame;

	RenderSavesFolder(context);
	ImGui::Separator();
	RenderSaveCurrent();
	ImGui::Separator();
	RenderVersionTable();
	RenderConfirmations();

	ImGui::End();
}

void SavesWindow::RenderSavesFolder(GameContext& context)
{
	ImGui::TextDisabled("Saves folder");
	ImGui::TextWrapped("%s", SaveFolders::GetSavesFolder().c_str());

	// Swapping the folder under a running game would change its save files mid-play
	ImGui::BeginDisabled(m_isPlaying);
	if (ImGui::Button("Choose Folder..."))
	{
		ChooseSavesFolder();
	}
	ImGui::EndDisabled();
	if (m_isPlaying && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Stop the game to change the saves folder");
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh"))
	{
		RefreshVersions();
	}

	ImGui::TextDisabled("Current save (the editor writes here)");
	ImGui::TextWrapped("%s  (%d files)", SaveFolders::GetCurrentSaveFolder().c_str(), m_currentFileCount);

	ImGui::BeginDisabled(m_isPlaying || m_currentFileCount == 0);
	if (ImGui::Button("Clear Current..."))
	{
		m_shouldOpenClear = true;
	}
	ImGui::EndDisabled();
	if (m_isPlaying && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Stop the game to clear the current save");
	}
}

void SavesWindow::RenderSaveCurrent()
{
	ImGui::TextDisabled("Save the current save as a new version");
	ImGui::SetNextItemWidth(-ImGui::CalcTextSize("Save Current").x - ImGui::GetStyle().ItemSpacing.x -
	                        ImGui::GetStyle().FramePadding.x * 2.0f);
	bool pressedEnter =
	    ImGui::InputTextWithHint("##VersionName", "Save name", &m_versionName, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::SameLine();
	if (ImGui::Button("Save Current") || pressedEnter)
	{
		SaveCurrentVersion();
	}

	if (!m_statusMessage.empty())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, m_isStatusError ? STATUS_ERROR_COLOR : STATUS_SUCCESS_COLOR);
		ImGui::TextWrapped("%s", m_statusMessage.c_str());
		ImGui::PopStyleColor();
	}
}

void SavesWindow::RenderVersionTable()
{
	ImGui::TextDisabled("Saved versions (%d)", (int)m_versions.size());
	if (m_versions.empty())
	{
		ImGui::TextDisabled("None yet.");
		return;
	}

	// Columns fit their content and scroll sideways rather than squeezing names - the buttons sit right after the
	// name so they stay in view in a narrow window, with the date and file count to their right.
	ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollX |
	                             ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
	if (!ImGui::BeginTable("SaveVersions", 4, tableFlags))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(1, 1);
	ImGui::TableSetupColumn("Name");
	ImGui::TableSetupColumn("");
	ImGui::TableSetupColumn("Saved");
	ImGui::TableSetupColumn("Files");
	ImGui::TableHeadersRow();

	for (const SaveVersion& version : m_versions)
	{
		ImGui::PushID(version.name.c_str());
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(version.name.c_str());
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", version.path.c_str());
		}
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginDisabled(m_isPlaying);
		if (ImGui::SmallButton("Restore..."))
		{
			m_pendingRestoreName = version.name;
			m_shouldOpenRestore  = true;
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip(m_isPlaying ? "Stop the game to restore a save"
			                              : "Replace the current save with this version");
		}
		// Versions aren't read by the running game, so this works while playing too
		ImGui::SameLine();
		if (ImGui::SmallButton("Delete..."))
		{
			m_pendingDeleteName = version.name;
			m_shouldOpenDelete  = true;
		}
		ImGui::TableSetColumnIndex(2);
		ImGui::TextUnformatted(version.modifiedTime.c_str());
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%d", version.fileCount);
		ImGui::PopID();
	}
	ImGui::EndTable();
}

void SavesWindow::RenderConfirmations()
{
	const char* clearPopupId   = "Clear current save?";
	const char* restorePopupId = "Restore save?";
	const char* deletePopupId  = "Delete saved version?";
	if (m_shouldOpenClear)
	{
		ImGui::OpenPopup(clearPopupId);
		m_shouldOpenClear = false;
	}
	if (m_shouldOpenRestore)
	{
		ImGui::OpenPopup(restorePopupId);
		m_shouldOpenRestore = false;
	}
	if (m_shouldOpenDelete)
	{
		ImGui::OpenPopup(deletePopupId);
		m_shouldOpenDelete = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImVec2 buttonSize(140.0f, 0.0f);

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(clearPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Delete all %d files in the current save?", m_currentFileCount);
		ImGui::TextDisabled("%s", SaveFolders::GetCurrentSaveFolder().c_str());
		ImGui::TextUnformatted("This can't be undone. Saved versions aren't touched.");
		ImGui::Spacing();
		if (ImGui::Button("Clear", buttonSize))
		{
			ClearCurrentSave();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", buttonSize) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(restorePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Replace the current save with \"%s\"?", m_pendingRestoreName.c_str());
		ImGui::Text("The %d files in the current save will be deleted.", m_currentFileCount);
		ImGui::TextUnformatted("Save it as a version first if you want to keep it.");
		ImGui::Spacing();
		if (ImGui::Button("Restore", buttonSize))
		{
			RestorePendingVersion();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", buttonSize) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(deletePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Delete the saved version \"%s\"?", m_pendingDeleteName.c_str());
		ImGui::TextUnformatted("Its folder and files are deleted. This can't be undone.");
		ImGui::TextUnformatted("The current save isn't touched.");
		ImGui::Spacing();
		if (ImGui::Button("Delete", buttonSize))
		{
			DeletePendingVersion();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", buttonSize) || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void SavesWindow::ChooseSavesFolder()
{
	std::string pickedFolder = FileSystem::OpenFolderDialog("Choose Saves Folder", SaveFolders::GetSavesFolder());
	if (pickedFolder.empty())
	{
		return;
	}

	std::string error;
	if (!SaveFolders::ChangeSavesFolder(pickedFolder, error))
	{
		SetStatus(error, true);
		return;
	}
	SetStatus("Now saving to " + SaveFolders::GetCurrentSaveFolder(), false);
	RefreshVersions();
}

void SavesWindow::SaveCurrentVersion()
{
	std::string error;
	if (!SaveFolders::SaveCurrentAs(m_versionName, error))
	{
		SetStatus(error, true);
		RefreshVersions();
		return;
	}
	SetStatus("Saved \"" + m_versionName + "\"", false);
	m_versionName.clear();
	RefreshVersions();
}

void SavesWindow::ClearCurrentSave()
{
	std::string error;
	if (!SaveFolders::ClearCurrent(error))
	{
		SetStatus(error, true);
	}
	else
	{
		SetStatus("Cleared the current save", false);
	}
	RefreshVersions();
}

void SavesWindow::RestorePendingVersion()
{
	std::string error;
	if (!SaveFolders::RestoreVersion(m_pendingRestoreName, error))
	{
		SetStatus(error, true);
	}
	else
	{
		SetStatus("Restored \"" + m_pendingRestoreName + "\" into the current save", false);
	}
	m_pendingRestoreName.clear();
	RefreshVersions();
}

void SavesWindow::DeletePendingVersion()
{
	std::string error;
	if (!SaveFolders::DeleteVersion(m_pendingDeleteName, error))
	{
		SetStatus(error, true);
	}
	else
	{
		SetStatus("Deleted \"" + m_pendingDeleteName + "\"", false);
	}
	m_pendingDeleteName.clear();
	RefreshVersions();
}

void SavesWindow::RefreshVersions()
{
	m_versions         = SaveFolders::ListVersions();
	m_currentFileCount = SaveFolders::CountCurrentFiles();
	m_needsRefresh     = false;
}

void SavesWindow::SetStatus(const std::string& message, bool isError)
{
	m_statusMessage = message;
	m_isStatusError = isError;
}
}  // namespace Struktur::Debug
