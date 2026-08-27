#include "FileSystem.h"

#include <physfs.h>

#include "Debug/Assertions.h"

#ifdef PLATFORM_WEB
	#include <emscripten.h>
#endif

#ifdef EDITOR
	#include <tinyfiledialogs.h>
#endif
#include <filesystem>

namespace Struktur
{
//////////////////////////////////////////////////////////////////////////////////////////
// Internal helpers
//////////////////////////////////////////////////////////////////////////////////////////

static FileSystemError TranslatePhysFSError(PHYSFS_ErrorCode code)
{
	switch (code)
	{
		case PHYSFS_ERR_NOT_FOUND:
			return FileSystemError::FileNotFound;
		case PHYSFS_ERR_PERMISSION:
			return FileSystemError::PermissionDenied;
		case PHYSFS_ERR_BAD_FILENAME:
			return FileSystemError::InvalidPath;
		case PHYSFS_ERR_IO:
			return FileSystemError::ReadError;
		case PHYSFS_ERR_NOT_INITIALIZED:
			return FileSystemError::NotInitialised;
		default:
			return FileSystemError::Unknown;
	}
}

static std::string GetPhysFSErrorMessage(PHYSFS_ErrorCode code)
{
	return std::string(PHYSFS_getErrorByCode(code));
}

// PHYSFS_getLastErrorCode() clears the stored error once read, so callers that need both the translated
// FileSystemError AND the human-readable message must capture the code exactly once - calling
// TranslatePhysFSError()/GetPhysFSErrorMessage() as two separate PHYSFS_getLastErrorCode() reads (as this used
// to) meant the second read always saw PHYSFS_ERR_OK ("no error"), masking the real failure reason.
struct PhysFSError
{
	FileSystemError error;
	std::string message;
};

static PhysFSError GetLastPhysFSError()
{
	PHYSFS_ErrorCode code = PHYSFS_getLastErrorCode();
	return {TranslatePhysFSError(code), GetPhysFSErrorMessage(code)};
}

//////////////////////////////////////////////////////////////////////////////////////////
// Lifecycle
//////////////////////////////////////////////////////////////////////////////////////////

void FileSystem::Init(const std::string& workingDirectory)
{
	PHYSFS_init(workingDirectory.c_str());
}

void FileSystem::Shutdown()
{
	PHYSFS_deinit();
}

std::string FileSystem::GetWorkingDirectory()
{
	return std::filesystem::current_path().string();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Mounting
//////////////////////////////////////////////////////////////////////////////////////////

FileResult<void> FileSystem::Mount(const std::string& path, const std::string& mountPoint, bool append)
{
	if (PHYSFS_mount(path.c_str(), mountPoint.c_str(), append ? 1 : 0) == 0)
	{
		auto physfsError = GetLastPhysFSError();
		return FileResult<void>::Fail(physfsError.error, physfsError.message);
	}

	return FileResult<void>::Ok();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Write directory
//////////////////////////////////////////////////////////////////////////////////////////

FileResult<void> FileSystem::SetWriteDir(const std::string& path)
{
	// PhysicsFS can only create directories inside an already-set write dir
	// So we set it first to the parent, create the subdir, then set it to the full path
	std::filesystem::path fsPath(path);
	std::filesystem::path parent = fsPath.parent_path();
	std::string dirName          = fsPath.filename().string();

	// Set parent as write dir temporarily so we can create the child
	if (PHYSFS_setWriteDir(parent.string().c_str()) != 0)
	{
		PHYSFS_mkdir(dirName.c_str());
	}

	if (PHYSFS_setWriteDir(path.c_str()) == 0)
	{
		auto physfsError = GetLastPhysFSError();
		return FileResult<void>::Fail(physfsError.error, physfsError.message);
	}

	return FileResult<void>::Ok();
}

std::string FileSystem::GetSaveDir(const std::string& organisation, const std::string& appName)
{
	const char* dir = PHYSFS_getPrefDir(organisation.c_str(), appName.c_str());
	ASSERT(dir);
	return std::string(dir);
}

FileResult<void> FileSystem::SeedFromDefaults(const std::string& sourcePath, const std::string& destPath)
{
	// Already exists in write dir - don't overwrite the user's version
	if (Exists(destPath))
	{
		return FileResult<void>::Ok();
	}

	// Read from PAK
	auto result = ReadBytes(sourcePath);
	if (!result)
	{
		return FileResult<void>::Fail(result.error, "SeedFromDefaults: could not read source " + sourcePath + " - " +
		                                                result.errorMessage);
	}

	// Write to save dir (plain text - user needs to be able to edit this)
	auto writeResult = WriteBytes(destPath, result.value);
	if (!writeResult)
	{
		return FileResult<void>::Fail(writeResult.error, "SeedFromDefaults: could not write dest " + destPath + " - " +
		                                                     writeResult.errorMessage);
	}

	DEBUG_INFO("FILESYSTEM: Seeded default file %s -> %s", sourcePath.c_str(), destPath.c_str());

	return FileResult<void>::Ok();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Reading
//////////////////////////////////////////////////////////////////////////////////////////

FileResult<std::vector<uint8_t>> FileSystem::ReadBytes(const std::string& path)
{
	return ReadFile(path);
}

FileResult<std::string> FileSystem::ReadString(const std::string& path)
{
	auto result = ReadFile(path);
	if (!result)
	{
		return FileResult<std::string>::Fail(result.error, result.errorMessage);
	}

	return FileResult<std::string>::Ok(std::string(result.value.begin(), result.value.end()));
}

FileResult<std::vector<uint8_t>> FileSystem::ReadFile(const std::string& path)
{
	PHYSFS_File* file = PHYSFS_openRead(path.c_str());
	if (!file)
	{
		auto physfsError = GetLastPhysFSError();
		return FileResult<std::vector<uint8_t>>::Fail(physfsError.error, physfsError.message);
	}

	PHYSFS_sint64 size = PHYSFS_fileLength(file);
	if (size < 0)
	{
		PHYSFS_close(file);
		return FileResult<std::vector<uint8_t>>::Fail(FileSystemError::ReadError,
		                                              "Could not determine file size: " + path);
	}

	std::vector<uint8_t> buffer(size);
	PHYSFS_sint64 bytesRead = PHYSFS_readBytes(file, buffer.data(), size);
	PHYSFS_close(file);

	if (bytesRead < size)
	{
		return FileResult<std::vector<uint8_t>>::Fail(FileSystemError::ReadError,
		                                              GetPhysFSErrorMessage(PHYSFS_getLastErrorCode()));
	}

	return FileResult<std::vector<uint8_t>>::Ok(std::move(buffer));
}

FileResult<std::string> FileSystem::ReadEncrypted(const std::string& path)
{
	auto result = ReadBytes(path);
	if (!result)
	{
		return FileResult<std::string>::Fail(result.error, result.errorMessage);
	}

	std::vector<uint8_t> decrypted = Decrypt(result.value);
	return FileResult<std::string>::Ok(std::string(decrypted.begin(), decrypted.end()));
}

//////////////////////////////////////////////////////////////////////////////////////////
// Writing
//////////////////////////////////////////////////////////////////////////////////////////

FileResult<void> FileSystem::WriteBytes(const std::string& path, const std::vector<uint8_t>& data)
{
	PHYSFS_File* file = PHYSFS_openWrite(path.c_str());
	if (!file)
	{
		auto physfsError = GetLastPhysFSError();
		return FileResult<void>::Fail(physfsError.error, physfsError.message);
	}

	PHYSFS_sint64 bytesWritten = PHYSFS_writeBytes(file, data.data(), data.size());
	PHYSFS_close(file);

	if (bytesWritten < (PHYSFS_sint64)data.size())
	{
		return FileResult<void>::Fail(FileSystemError::WriteError, GetPhysFSErrorMessage(PHYSFS_getLastErrorCode()));
	}

	return FileResult<void>::Ok();
}

FileResult<void> FileSystem::WriteString(const std::string& path, const std::string& data)
{
	return WriteBytes(path, std::vector<uint8_t>(data.begin(), data.end()));
}

FileResult<void> FileSystem::WriteEncrypted(const std::string& path, const std::string& data)
{
	std::vector<uint8_t> raw(data.begin(), data.end());
	std::vector<uint8_t> encrypted = Encrypt(raw);
	return WriteBytes(path, encrypted);
}

void FileSystem::SyncSaves()
{
#if defined(PLATFORM_WEB)
	// Sync FROM memory TO IndexedDB (false = persist direction)
	// Must be called after every save on WASM or data will be lost on tab close
	EM_ASM(FS.syncfs(
	    false, function(err) {
		    if (err)
		    {
			    console.error('[Struktur] Failed to sync saves to IndexedDB:', err);
		    }
		    else
		    {
			    console.log('[Struktur] Saves synced to IndexedDB');
		    }
	    }););
#endif
	// No-op on desktop - writes go directly to the OS filesystem
}

//////////////////////////////////////////////////////////////////////////////////////////
// Utility
//////////////////////////////////////////////////////////////////////////////////////////

bool FileSystem::Exists(const std::string& path)
{
	return PHYSFS_exists(path.c_str()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Encryption
//////////////////////////////////////////////////////////////////////////////////////////

// Keep this secret - changing it will invalidate all existing saves
// TODO Move this to a git ignored header
static constexpr uint8_t k_encryptionKey[] = {0x4B, 0x72, 0x69, 0x63, 0x6B, 0x65, 0x74, 0x47,
                                              0x61, 0x6D, 0x65, 0x53, 0x61, 0x76, 0x65, 0x31};
static constexpr size_t k_keyLength        = sizeof(k_encryptionKey);

std::vector<uint8_t> FileSystem::Encrypt(const std::vector<uint8_t>& data)
{
	std::vector<uint8_t> result(data.size());
	for (size_t i = 0; i < data.size(); i++)
	{
		result[i] = data[i] ^ k_encryptionKey[i % k_keyLength];
	}
	return result;
}

std::vector<uint8_t> FileSystem::Decrypt(const std::vector<uint8_t>& data)
{
	// XOR is symmetric - decryption is identical to encryption
	return Encrypt(data);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Editor - tinyfiledialogs wrappers
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR

FileResult<uint64_t> FileSystem::GetFileSize(const std::string& path)
{
	PHYSFS_Stat stat;
	if (!PHYSFS_stat(path.c_str(), &stat) || stat.filesize < 0)
	{
		auto physfsError = GetLastPhysFSError();
		return FileResult<uint64_t>::Fail(physfsError.error, physfsError.message);
	}
	return FileResult<uint64_t>::Ok((uint64_t)stat.filesize);
}

std::vector<std::string> FileSystem::ListDirectory(const std::string& path)
{
	std::vector<std::string> result;
	// "" means "the mount root" by this codebase's own path convention (no leading slash anywhere - see
	// FileExplorerWindow) - PHYSFS itself wants "/" for that, not "".
	char** files = PHYSFS_enumerateFiles(path.empty() ? "/" : path.c_str());
	if (files)
	{
		for (char** entry = files; *entry != nullptr; ++entry)
		{
			result.emplace_back(*entry);
		}
		PHYSFS_freeList(files);
	}
	return result;
}

bool FileSystem::IsDirectory(const std::string& path)
{
	PHYSFS_Stat stat;
	if (!PHYSFS_stat(path.c_str(), &stat))
	{
		return false;
	}
	return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

std::string FileSystem::OpenFolderDialog(const std::string& title, const std::string& defaultPath)
{
	const char* result = tinyfd_selectFolderDialog(title.c_str(), defaultPath.empty() ? nullptr : defaultPath.c_str());
	return result ? std::string(result) : std::string();
}

std::string FileSystem::OpenFileDialog(const std::string& title, const std::string& defaultPath,
                                       const std::vector<std::string>& filters, const std::string& filterDesc)
{
	auto filterList = BuildFilterList(filters);

	const char* result = tinyfd_openFileDialog(title.c_str(), defaultPath.empty() ? nullptr : defaultPath.c_str(),
	                                           (int)filterList.size(), filterList.empty() ? nullptr : filterList.data(),
	                                           filterDesc.empty() ? nullptr : filterDesc.c_str(),
	                                           0  // single selection
	);
	return result ? std::string(result) : std::string();
}

std::vector<std::string> FileSystem::OpenFileDialogMultiple(const std::string& title, const std::string& defaultPath,
                                                            const std::vector<std::string>& filters,
                                                            const std::string& filterDesc)
{
	auto filterList = BuildFilterList(filters);

	const char* result = tinyfd_openFileDialog(title.c_str(), defaultPath.empty() ? nullptr : defaultPath.c_str(),
	                                           (int)filterList.size(), filterList.empty() ? nullptr : filterList.data(),
	                                           filterDesc.empty() ? nullptr : filterDesc.c_str(),
	                                           1  // multi selection
	);

	// tinyfd returns multiple paths separated by '|'
	std::vector<std::string> paths;
	if (!result)
	{
		return paths;
	}

	std::string resultStr(result);
	size_t pos = 0;
	while ((pos = resultStr.find('|')) != std::string::npos)
	{
		paths.push_back(resultStr.substr(0, pos));
		resultStr.erase(0, pos + 1);
	}
	paths.push_back(resultStr);
	return paths;
}

std::string FileSystem::SaveFileDialog(const std::string& title, const std::string& defaultPath,
                                       const std::vector<std::string>& filters, const std::string& filterDesc)
{
	auto filterList = BuildFilterList(filters);

	const char* result = tinyfd_saveFileDialog(title.c_str(), defaultPath.empty() ? nullptr : defaultPath.c_str(),
	                                           (int)filterList.size(), filterList.empty() ? nullptr : filterList.data(),
	                                           filterDesc.empty() ? nullptr : filterDesc.c_str());
	return result ? std::string(result) : std::string();
}

bool FileSystem::MessageBox(const std::string& title, const std::string& message, bool okCancel)
{
	int result = tinyfd_messageBox(title.c_str(), message.c_str(), okCancel ? "okcancel" : "ok", "info",
	                               1  // default button = ok
	);
	return result == 1;
}

std::string FileSystem::InputBox(const std::string& title, const std::string& message, const std::string& defaultValue)
{
	const char* result =
	    tinyfd_inputBox(title.c_str(), message.c_str(), defaultValue.empty() ? nullptr : defaultValue.c_str());
	return result ? std::string(result) : std::string();
}

std::vector<const char*> FileSystem::BuildFilterList(const std::vector<std::string>& filters)
{
	std::vector<const char*> filterList;
	filterList.reserve(filters.size());
	for (const auto& f : filters)
	{
		filterList.push_back(f.c_str());
	}
	return filterList;
}

#endif  // EDITOR

}  // namespace Struktur

//////////////////////////////////////////////////////////////////////////////////////////
// WASM extern C callback
// Called by JS once IndexedDB has synced saves into memory
// Hook this up to whatever triggers your save game loading
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(PLATFORM_WEB)
extern "C"
{
	void OnFilesystemReady()
	{
		// Safe to read save files now - IndexedDB has been synced into memory
		// Trigger your save game load here, for example:
		// GameSaveManager::Load();
	}
}
#endif
