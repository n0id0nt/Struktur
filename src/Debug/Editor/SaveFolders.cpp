#include "SaveFolders.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <system_error>

#include "Debug/Assertions.h"
#include "Engine/Core/FileSystem.h"
#include "nlohmann/json.hpp"

namespace Struktur::Debug
{
// Paths are kept as UTF-8 strings (what PhysFS and tinyfiledialogs use); std::filesystem::path's std::string
// constructor would read them in the ANSI code page, so go through char8_t explicitly.
static std::filesystem::path PathFromUtf8(const std::string& utf8Path)
{
	std::u8string utf8Characters(utf8Path.begin(), utf8Path.end());
	return std::filesystem::path(utf8Characters);
}

static std::string PathToUtf8(const std::filesystem::path& path)
{
	std::u8string utf8Characters = path.generic_u8string();
	return std::string(utf8Characters.begin(), utf8Characters.end());
}

// Absolute, forward-slashed, no trailing separator
static std::string NormaliseFolderPath(const std::filesystem::path& folder)
{
	std::error_code errorCode;
	std::filesystem::path absolutePath = std::filesystem::absolute(folder, errorCode);
	if (errorCode)
	{
		absolutePath = folder;
	}
	std::string normalisedPath = PathToUtf8(absolutePath.lexically_normal());
	while (normalisedPath.size() > 1 && normalisedPath.back() == '/' && normalisedPath[normalisedPath.size() - 2] != ':')
	{
		normalisedPath.pop_back();
	}
	return normalisedPath;
}

static bool EqualsIgnoringCase(const std::string& first, const std::string& second)
{
	if (first.size() != second.size())
	{
		return false;
	}
	for (size_t i = 0; i < first.size(); i++)
	{
		if (std::tolower((unsigned char)first[i]) != std::tolower((unsigned char)second[i]))
		{
			return false;
		}
	}
	return true;
}

static std::string FormatLocalTime(const std::filesystem::file_time_type& fileTime)
{
	auto systemTime  = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
	std::time_t time = std::chrono::system_clock::to_time_t(systemTime);
	std::tm localTime{};
#ifdef _WIN32
	localtime_s(&localTime, &time);
#else
	localtime_r(&time, &localTime);
#endif
	char buffer[32];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTime);
	return buffer;
}

// A version with its raw folder time, for sorting
struct DatedSaveVersion
{
	SaveVersion version;
	std::filesystem::file_time_type modifiedTime;
};

static bool IsNewerVersion(const DatedSaveVersion& first, const DatedSaveVersion& second)
{
	return first.modifiedTime > second.modifiedTime;
}

static int CountFilesIn(const std::filesystem::path& folder)
{
	int fileCount = 0;
	std::error_code errorCode;
	for (std::filesystem::recursive_directory_iterator entry(folder, errorCode), end; !errorCode && entry != end;
	     entry.increment(errorCode))
	{
		if (entry->is_regular_file(errorCode))
		{
			fileCount++;
		}
	}
	return fileCount;
}

static nlohmann::json ReadSettingsFile()
{
	std::ifstream file(SaveFolders::kSettingsFilePath);
	if (!file.is_open())
	{
		return nlohmann::json::object();
	}
	nlohmann::json settings = nlohmann::json::parse(file, nullptr, false);
	if (settings.is_discarded() || !settings.is_object())
	{
		DEBUG_WARNING(std::string("Couldn't read ") + SaveFolders::kSettingsFilePath + ", using default saves folders");
		return nlohmann::json::object();
	}
	return settings;
}

std::string SaveFolders::Initialise(const std::string& projectPath)
{
	s_projectPath = projectPath;

	std::string folder = LoadSavedFolderSetting(projectPath);
	if (folder.empty())
	{
		folder = DefaultSavesFolder(projectPath);
	}

	std::string error;
	std::string currentFolder = PrepareCurrentFolder(folder, error);
	if (currentFolder.empty())
	{
		DEBUG_WARNING("Saves folder " + folder + " isn't usable (" + error + "), using the default");
		folder        = DefaultSavesFolder(projectPath);
		currentFolder = PrepareCurrentFolder(folder, error);
		if (currentFolder.empty())
		{
			DEBUG_ERROR("Couldn't create the default saves folder " + folder + ": " + error);
			currentFolder = PathToUtf8(PathFromUtf8(folder) / kCurrentFolderName);
		}
	}

	s_savesFolder       = folder;
	s_currentSaveFolder = currentFolder;
	DEBUG_INFO("Saving to " + s_currentSaveFolder);
	return s_currentSaveFolder;
}

const std::string& SaveFolders::GetSavesFolder()
{
	return s_savesFolder;
}

const std::string& SaveFolders::GetCurrentSaveFolder()
{
	return s_currentSaveFolder;
}

bool SaveFolders::ChangeSavesFolder(const std::string& folder, std::string& outError)
{
	std::string newSavesFolder = NormaliseFolderPath(PathFromUtf8(folder));
	if (newSavesFolder == s_savesFolder)
	{
		return true;
	}

	std::string newCurrentFolder = PrepareCurrentFolder(newSavesFolder, outError);
	if (newCurrentFolder.empty())
	{
		return false;
	}

	FileResult<void> unmountResult = FileSystem::Unmount(s_currentSaveFolder);
	if (!unmountResult)
	{
		DEBUG_WARNING("Couldn't unmount the old save folder " + s_currentSaveFolder + ": " + unmountResult.errorMessage);
	}

	FileResult<void> writeDirResult = FileSystem::SetWriteDir(newCurrentFolder);
	FileResult<void> mountResult    = writeDirResult ? FileSystem::Mount(newCurrentFolder, "/", false) : writeDirResult;
	if (!writeDirResult || !mountResult)
	{
		outError = "Couldn't switch to " + newCurrentFolder + ": " +
		           (writeDirResult ? mountResult.errorMessage : writeDirResult.errorMessage);
		// Put the old folder back so saving keeps working
		FileSystem::SetWriteDir(s_currentSaveFolder);
		FileSystem::Mount(s_currentSaveFolder, "/", false);
		return false;
	}

	s_savesFolder       = newSavesFolder;
	s_currentSaveFolder = newCurrentFolder;
	StoreSavedFolderSetting(s_projectPath, s_savesFolder);
	DEBUG_INFO("Saving to " + s_currentSaveFolder);
	return true;
}

std::vector<SaveVersion> SaveFolders::ListVersions()
{
	std::vector<DatedSaveVersion> datedVersions;

	std::error_code errorCode;
	for (std::filesystem::directory_iterator entry(PathFromUtf8(s_savesFolder), errorCode), end;
	     !errorCode && entry != end; entry.increment(errorCode))
	{
		if (!entry->is_directory(errorCode))
		{
			continue;
		}
		std::string name = PathToUtf8(entry->path().filename());
		if (EqualsIgnoringCase(name, kCurrentFolderName) || name.empty() || name.front() == '.')
		{
			continue;
		}
		DatedSaveVersion datedVersion;
		datedVersion.modifiedTime         = entry->last_write_time(errorCode);
		datedVersion.version.name         = name;
		datedVersion.version.path         = PathToUtf8(entry->path());
		datedVersion.version.modifiedTime = FormatLocalTime(datedVersion.modifiedTime);
		datedVersion.version.fileCount    = CountFilesIn(entry->path());
		datedVersions.push_back(datedVersion);
	}

	std::sort(datedVersions.begin(), datedVersions.end(), IsNewerVersion);
	std::vector<SaveVersion> versions;
	for (const DatedSaveVersion& datedVersion : datedVersions)
	{
		versions.push_back(datedVersion.version);
	}
	return versions;
}

int SaveFolders::CountCurrentFiles()
{
	return CountFilesIn(PathFromUtf8(s_currentSaveFolder));
}

bool SaveFolders::SaveCurrentAs(const std::string& versionName, std::string& outError)
{
	if (!IsValidVersionName(versionName, outError))
	{
		return false;
	}

	std::filesystem::path destination = PathFromUtf8(s_savesFolder) / PathFromUtf8(versionName);
	std::error_code errorCode;
	if (std::filesystem::exists(destination, errorCode))
	{
		outError = "A save called \"" + versionName + "\" already exists";
		return false;
	}
	if (!std::filesystem::create_directory(destination, errorCode))
	{
		outError = "Couldn't create " + PathToUtf8(destination) + ": " + errorCode.message();
		return false;
	}

	std::filesystem::copy(PathFromUtf8(s_currentSaveFolder), destination, std::filesystem::copy_options::recursive,
	                      errorCode);
	if (errorCode)
	{
		outError = "Couldn't copy the current save: " + errorCode.message() + " (partial copy left in " +
		           PathToUtf8(destination) + ")";
		return false;
	}

	DEBUG_INFO("Saved current save as " + PathToUtf8(destination));
	return true;
}

bool SaveFolders::IsValidVersionName(const std::string& versionName, std::string& outError)
{
	if (versionName.empty())
	{
		outError = "Enter a name for the save";
		return false;
	}
	if (versionName.front() == ' ' || versionName.back() == ' ' || versionName.back() == '.')
	{
		outError = "The name can't start or end with a space, or end with a dot";
		return false;
	}
	for (char character : versionName)
	{
		if ((unsigned char)character < 32 || std::string("<>:\"/\\|?*").find(character) != std::string::npos)
		{
			outError = "The name can't contain < > : \" / \\ | ? *";
			return false;
		}
	}
	if (versionName == "." || versionName == ".." || EqualsIgnoringCase(versionName, kCurrentFolderName))
	{
		outError = "\"" + versionName + "\" is reserved";
		return false;
	}
	return true;
}

bool SaveFolders::ClearCurrent(std::string& outError)
{
	if (!DeleteCurrentContents(outError))
	{
		return false;
	}
	DEBUG_INFO("Cleared " + s_currentSaveFolder);
	return true;
}

bool SaveFolders::RestoreVersion(const std::string& versionName, std::string& outError)
{
	std::filesystem::path versionFolder = PathFromUtf8(s_savesFolder) / PathFromUtf8(versionName);
	std::filesystem::path stagingFolder = PathFromUtf8(s_savesFolder) / kRestoreStagingFolderName;
	std::filesystem::path currentFolder = PathFromUtf8(s_currentSaveFolder);
	std::error_code errorCode;
	if (versionName.empty() || EqualsIgnoringCase(versionName, kCurrentFolderName) ||
	    !std::filesystem::is_directory(versionFolder, errorCode))
	{
		outError = "There's no saved version called \"" + versionName + "\"";
		return false;
	}

	// 1. Copy the version aside - if this fails, Current hasn't been touched
	std::filesystem::remove_all(stagingFolder, errorCode);
	std::filesystem::copy(versionFolder, stagingFolder, std::filesystem::copy_options::recursive, errorCode);
	if (errorCode)
	{
		outError = "Couldn't copy \"" + versionName + "\": " + errorCode.message() + " - the current save is unchanged";
		std::error_code cleanupError;
		std::filesystem::remove_all(stagingFolder, cleanupError);
		return false;
	}

	// 2. Empty Current, 3. move the staged files in (a rename on the same drive)
	if (!DeleteCurrentContents(outError))
	{
		std::error_code cleanupError;
		std::filesystem::remove_all(stagingFolder, cleanupError);
		return false;
	}
	for (std::filesystem::directory_iterator entry(stagingFolder, errorCode), end; !errorCode && entry != end;
	     entry.increment(errorCode))
	{
		std::error_code moveError;
		std::filesystem::rename(entry->path(), currentFolder / entry->path().filename(), moveError);
		if (moveError)
		{
			outError = "Couldn't move " + PathToUtf8(entry->path().filename()) + " into the current save: " +
			           moveError.message() + " - the rest is still in " + PathToUtf8(stagingFolder);
			return false;
		}
	}
	std::filesystem::remove_all(stagingFolder, errorCode);

	DEBUG_INFO("Restored \"" + versionName + "\" into " + s_currentSaveFolder);
	return true;
}

bool SaveFolders::DeleteVersion(const std::string& versionName, std::string& outError)
{
	// The same names ListVersions shows - and IsValidVersionName rules out separators, so the folder can only be a
	// direct child of the saves folder.
	std::string nameError;
	if (!IsValidVersionName(versionName, nameError) || versionName.front() == '.')
	{
		outError = "\"" + versionName + "\" isn't a saved version";
		return false;
	}
	std::filesystem::path versionFolder = PathFromUtf8(s_savesFolder) / PathFromUtf8(versionName);
	std::error_code errorCode;
	if (s_savesFolder.empty() || !std::filesystem::is_directory(versionFolder, errorCode))
	{
		outError = "There's no saved version called \"" + versionName + "\"";
		return false;
	}

	std::filesystem::remove_all(versionFolder, errorCode);
	if (errorCode)
	{
		outError = "Couldn't delete \"" + versionName + "\": " + errorCode.message();
		return false;
	}
	DEBUG_INFO("Deleted save version " + PathToUtf8(versionFolder));
	return true;
}

bool SaveFolders::DeleteCurrentContents(std::string& outError)
{
	// Guard against ever emptying the wrong folder: it must be the Current folder inside the saves folder
	std::filesystem::path currentFolder = PathFromUtf8(s_currentSaveFolder);
	std::error_code errorCode;
	if (s_currentSaveFolder.empty() || !EqualsIgnoringCase(PathToUtf8(currentFolder.filename()), kCurrentFolderName) ||
	    PathToUtf8(currentFolder.parent_path()) != s_savesFolder || !std::filesystem::is_directory(currentFolder, errorCode))
	{
		outError = "The current save folder isn't where it should be: " + s_currentSaveFolder;
		return false;
	}

	std::vector<std::filesystem::path> entries;
	for (std::filesystem::directory_iterator entry(currentFolder, errorCode), end; !errorCode && entry != end;
	     entry.increment(errorCode))
	{
		entries.push_back(entry->path());
	}
	if (errorCode)
	{
		outError = "Couldn't read " + s_currentSaveFolder + ": " + errorCode.message();
		return false;
	}
	for (const std::filesystem::path& entryPath : entries)
	{
		std::filesystem::remove_all(entryPath, errorCode);
		if (errorCode)
		{
			outError = "Couldn't delete " + PathToUtf8(entryPath) + ": " + errorCode.message();
			return false;
		}
	}
	return true;
}

std::string SaveFolders::DefaultSavesFolder(const std::string& projectPath)
{
	if (projectPath.empty())
	{
		return NormaliseFolderPath(std::filesystem::path("saves"));
	}
	return NormaliseFolderPath(PathFromUtf8(projectPath) / ".." / "saves");
}

std::string SaveFolders::LoadSavedFolderSetting(const std::string& projectPath)
{
	nlohmann::json settings = ReadSettingsFile();
	if (!settings.contains("savesFolders") || !settings["savesFolders"].is_object())
	{
		return std::string();
	}
	const nlohmann::json& savesFolders = settings["savesFolders"];
	if (!savesFolders.contains(projectPath) || !savesFolders[projectPath].is_string())
	{
		return std::string();
	}
	return savesFolders[projectPath].get<std::string>();
}

void SaveFolders::StoreSavedFolderSetting(const std::string& projectPath, const std::string& folder)
{
	nlohmann::json settings = ReadSettingsFile();
	if (!settings.contains("savesFolders") || !settings["savesFolders"].is_object())
	{
		settings["savesFolders"] = nlohmann::json::object();
	}
	settings["savesFolders"][projectPath] = folder;

	std::ofstream file(kSettingsFilePath, std::ios::trunc);
	if (!file.is_open())
	{
		DEBUG_WARNING(std::string("Couldn't write ") + kSettingsFilePath);
		return;
	}
	file << settings.dump(4);
}

std::string SaveFolders::PrepareCurrentFolder(const std::string& folder, std::string& outError)
{
	std::filesystem::path savesFolder   = PathFromUtf8(folder);
	std::filesystem::path currentFolder = savesFolder / kCurrentFolderName;
	std::error_code errorCode;
	if (std::filesystem::is_directory(currentFolder, errorCode))
	{
		return PathToUtf8(currentFolder);
	}

	std::filesystem::create_directories(currentFolder, errorCode);
	if (errorCode)
	{
		outError = "Couldn't create " + PathToUtf8(currentFolder) + ": " + errorCode.message();
		return std::string();
	}

	// First time with this layout: the game used to save straight into the saves folder - carry those files over
	int copiedFiles = 0;
	for (std::filesystem::directory_iterator entry(savesFolder, errorCode), end; !errorCode && entry != end;
	     entry.increment(errorCode))
	{
		std::error_code copyError;
		if (entry->is_regular_file(copyError) &&
		    std::filesystem::copy_file(entry->path(), currentFolder / entry->path().filename(), copyError))
		{
			copiedFiles++;
		}
	}
	if (copiedFiles > 0)
	{
		DEBUG_INFO("Copied " + std::to_string(copiedFiles) + " existing save files into " + PathToUtf8(currentFolder));
	}
	return PathToUtf8(currentFolder);
}
}  // namespace Struktur::Debug
