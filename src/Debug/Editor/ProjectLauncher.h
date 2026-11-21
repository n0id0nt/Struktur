#pragma once

#include <string>
#include <vector>

namespace Struktur
{
class GameContext;
}

namespace Struktur::Debug
{
// One remembered project folder - the directory mounted at "/" (it holds Scripts/Main.wren).
struct LauncherProject
{
	std::string name;
	std::string path;
};

// Editor-startup project picker, drawn with ImGui in the editor window before any project is mounted (replaces
// the old native "Select Project Directory" dialog). Keeps the list of previously used projects in
// kProjectListFilePath, next to editor_layout.ini in the working directory. Removing a project only takes it off
// that list - no files are ever touched.
class ProjectLauncher
{
public:
	static constexpr const char* kProjectListFilePath = "editor_projects.json";

	// Runs the launcher's own frame loop until a project is opened or the window is closed. Needs the window,
	// graphics device, ImGui context and ImGuiRenderer initialised already. Returns the chosen project's folder,
	// or an empty string if the window was closed.
	std::string Run(GameContext& context);

private:
	void LoadProjectList();
	void SaveProjectList() const;
	// Seeds the list with the repo's assets/ folder (relative to the working directory) on first run
	void SeedDefaultProject();

	void RenderLauncher();
	void RenderProjectTable();
	// Drawn inside each table row's ID scope, where that row's Remove button opened the popup
	void RenderRemoveConfirmation();

	void AddProjectFromFolderPicker();
	void RemoveProject(int projectIndex);
	// Moves the project to the top of the list (most recently used first), saves, and marks it as chosen
	void OpenProject(int projectIndex);

	std::vector<LauncherProject> m_projects;
	std::string m_chosenProjectPath;
	std::string m_statusMessage;
	int m_pendingRemovalIndex = -1;
};
}  // namespace Struktur::Debug
