#pragma once

#include <string>
#include <vector>

#include "Debug/Editor/SaveFolders.h"
#include "EditorWindow.h"

namespace Struktur::Debug
{
// Manages the editor's game saves (see SaveFolders): pick the saves folder the editor writes to, and copy the
// Current save into a named version folder beside it. The version list is re-read when the window opens, after a
// save or folder change, and on Refresh - not every frame.
class SavesWindow : public EditorWindow
{
public:
	SavesWindow();

	void Render(GameContext& context) override;

private:
	void RenderSavesFolder(GameContext& context);
	void RenderSaveCurrent();
	void RenderVersionTable();
	// The Clear, Restore and Delete confirmations - opened here, at window level, from flags the buttons set
	void RenderConfirmations();

	void ChooseSavesFolder();
	void SaveCurrentVersion();
	void ClearCurrentSave();
	void RestorePendingVersion();
	void DeletePendingVersion();
	void RefreshVersions();
	void SetStatus(const std::string& message, bool isError);

	std::vector<SaveVersion> m_versions;
	int m_currentFileCount = 0;
	bool m_needsRefresh    = true;

	std::string m_versionName;
	std::string m_statusMessage;
	bool m_isStatusError = false;

	// Clearing or overwriting Current under a running game would pull its save files away mid-play
	bool m_isPlaying           = false;
	bool m_shouldOpenClear     = false;
	bool m_shouldOpenRestore   = false;
	std::string m_pendingRestoreName;
	bool m_shouldOpenDelete = false;
	std::string m_pendingDeleteName;
};
}  // namespace Struktur::Debug
