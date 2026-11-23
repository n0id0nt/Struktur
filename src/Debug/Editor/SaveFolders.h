#pragma once

#include <string>
#include <vector>

namespace Struktur::Debug
{
// One saved copy of the current save: a folder next to Current in the saves folder.
struct SaveVersion
{
	std::string name;
	std::string path;
	// Local time the folder was last written, "YYYY-MM-DD HH:MM"
	std::string modifiedTime;
	int fileCount = 0;
};

// The editor's saves folder layout:
//   <saves folder>/Current      - where the game writes while running in the editor (PhysFS write dir, mounted at /)
//   <saves folder>/<version>    - copies of Current, made with SaveCurrentAs
// The saves folder defaults to <project>/../saves and is remembered per project in kSettingsFilePath (next to
// editor_layout.ini in the working directory).
class SaveFolders
{
public:
	static constexpr const char* kSettingsFilePath  = "editor_saves.json";
	static constexpr const char* kCurrentFolderName = "Current";
	// Where RestoreVersion copies a version before swapping it into Current. Folders starting with '.' aren't versions.
	static constexpr const char* kRestoreStagingFolderName = ".Restoring";

	// Picks the project's saves folder and makes sure its Current folder exists. Returns the Current folder for
	// InitialiseGame to set as the write dir and mount. A saves folder from before this layout (save files loose in
	// the folder itself, no Current yet) has those files copied into Current - the originals are left in place.
	static std::string Initialise(const std::string& projectPath);

	static const std::string& GetSavesFolder();
	static const std::string& GetCurrentSaveFolder();

	// Switches the write dir and mount to <folder>/Current and remembers the folder for this project. Only call while
	// the game isn't playing - running scripts would see their save files change underneath them.
	static bool ChangeSavesFolder(const std::string& folder, std::string& outError);

	// Every version folder in the saves folder (not Current), newest first
	static std::vector<SaveVersion> ListVersions();
	// Number of files (recursively) in the Current folder
	static int CountCurrentFiles();

	// Copies Current into a new folder called versionName. Fails, copying nothing, if the name isn't a usable folder
	// name or a version with it already exists.
	static bool SaveCurrentAs(const std::string& versionName, std::string& outError);
	static bool IsValidVersionName(const std::string& versionName, std::string& outError);

	// Permanently deletes everything inside Current (the folder itself stays, so the write dir and mount stay valid).
	// Only call while the game isn't playing.
	static bool ClearCurrent(std::string& outError);
	// Replaces Current's contents with a copy of the version's. The version is copied to a staging folder first, so a
	// failed copy leaves Current untouched. Only call while the game isn't playing.
	static bool RestoreVersion(const std::string& versionName, std::string& outError);
	// Permanently deletes a version folder. Never Current, and only a folder directly inside the saves folder.
	static bool DeleteVersion(const std::string& versionName, std::string& outError);

private:
	// Deletes everything inside Current, after checking that's really the folder it is
	static bool DeleteCurrentContents(std::string& outError);
	static std::string DefaultSavesFolder(const std::string& projectPath);
	static std::string LoadSavedFolderSetting(const std::string& projectPath);
	static void StoreSavedFolderSetting(const std::string& projectPath, const std::string& folder);
	// Creates <folder>/Current (copying loose save files into it the first time). Returns its path, or empty on failure.
	static std::string PrepareCurrentFolder(const std::string& folder, std::string& outError);

	inline static std::string s_projectPath;
	inline static std::string s_savesFolder;
	inline static std::string s_currentSaveFolder;
};
}  // namespace Struktur::Debug
