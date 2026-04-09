#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Struktur
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// Error types
	//////////////////////////////////////////////////////////////////////////////////////////

	enum class FileSystemError
	{
		FileNotFound,
		PermissionDenied,
		InvalidPath,
		ReadError,
		WriteError,
		MountFailed,
		NotInitialised,
		Unknown
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Result type
	//////////////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	struct FileResult
	{
		bool            success = false;
		T               value = {};
		FileSystemError error = FileSystemError::Unknown;
		std::string     errorMessage;

		explicit operator bool() const { return success; }

		static FileResult Ok(T val)
		{
			FileResult r;
			r.success = true;
			r.value = std::move(val);
			return r;
		}

		static FileResult Fail(FileSystemError err, const std::string& msg = "")
		{
			FileResult r;
			r.success = false;
			r.error = err;
			r.errorMessage = msg;
			return r;
		}
	};

	template<>
	struct FileResult<void>
	{
		bool            success = false;
		FileSystemError error = FileSystemError::Unknown;
		std::string     errorMessage;

		explicit operator bool() const { return success; }

		static FileResult Ok()
		{
			FileResult r;
			r.success = true;
			return r;
		}

		static FileResult Fail(FileSystemError err, const std::string& msg = "")
		{
			FileResult r;
			r.success = false;
			r.error = err;
			r.errorMessage = msg;
			return r;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// FileSystem
	//////////////////////////////////////////////////////////////////////////////////////////

	class FileSystem
	{
	public:
		//-----------------------------------------------------------------------------------
		// Lifecycle
		//-----------------------------------------------------------------------------------

		// Must be called first with argv[0] from main()
		static void Init(const std::string& workingDirectory);
		static void Shutdown();

		//-----------------------------------------------------------------------------------
		// Mounting
		//-----------------------------------------------------------------------------------

		// Mount a directory or PAK file into the virtual filesystem
		// append = true  : lower priority (checked after existing mounts)
		// append = false : higher priority (checked before existing mounts, useful for patches/mods)
		static FileResult<void> Mount(const std::string& path, const std::string& mountPoint = "/", bool append = true);

		//-----------------------------------------------------------------------------------
		// Write directory
		//-----------------------------------------------------------------------------------

		// Set where files will be written (saves, config etc.)
		// On desktop use GetSaveDir() to get the correct OS path
		// On WASM use "/saves" (Emscripten virtual path)
		static FileResult<void> SetWriteDir(const std::string& path);

		// Returns the platform-appropriate save directory:
		// Windows : C:/Users/<user>/AppData/Roaming/<organisation>/<appName>/
		// macOS   : ~/Library/Application Support/<appName>/
		// Linux   : ~/.local/share/<appName>/
		static std::string GetSaveDir(const std::string& organisation, const std::string& appName);

		// Copies a file from the PAK to the write directory if it doesn't already exist there
		// Useful for seeding default config files on first run
		static FileResult<void> SeedFromDefaults(const std::string& sourcePath, const std::string& destPath);

		//-----------------------------------------------------------------------------------
		// Reading
		//-----------------------------------------------------------------------------------

		static FileResult<std::vector<uint8_t>> ReadBytes(const std::string& path);
		static FileResult<std::string>          ReadString(const std::string& path);
		static FileResult<std::string>			ReadEncrypted(const std::string& path);

		//-----------------------------------------------------------------------------------
		// Writing
		//-----------------------------------------------------------------------------------

		static FileResult<void> WriteBytes(const std::string& path, const std::vector<uint8_t>& data);
		static FileResult<void> WriteString(const std::string& path, const std::string& data);
		static FileResult<void> WriteEncrypted(const std::string& path, const std::string& data);

		// On WASM: syncs in-memory writes to IndexedDB so they persist between sessions
		// On desktop: no-op
		static void SyncSaves();

		//-----------------------------------------------------------------------------------
		// Utility
		//-----------------------------------------------------------------------------------

		static bool Exists(const std::string& path);

		//-----------------------------------------------------------------------------------
		// Editor only - native file dialogs via tinyfiledialogs
		//-----------------------------------------------------------------------------------

#ifdef EDITOR
		// Opens a native OS folder picker
		// Returns empty string if user cancels
		static std::string OpenFolderDialog(const std::string& title, const std::string& defaultPath = "");

		// Opens a native OS file picker (single file)
		// filters     : e.g. { "*.png", "*.jpg" }
		// filterDesc  : e.g. "Image Files"
		// Returns empty string if user cancels
		static std::string OpenFileDialog(
			const std::string& title,
			const std::string& defaultPath = "",
			const std::vector<std::string>& filters = {},
			const std::string& filterDesc = ""
		);

		// Opens a native OS file picker (multiple files)
		// Returns empty vector if user cancels
		// Multiple paths are split from tinyfd's '|' separated result
		static std::vector<std::string> OpenFileDialogMultiple(
			const std::string& title,
			const std::string& defaultPath = "",
			const std::vector<std::string>& filters = {},
			const std::string& filterDesc = ""
		);

		// Opens a native OS save file dialog
		// Returns empty string if user cancels
		static std::string SaveFileDialog(
			const std::string& title,
			const std::string& defaultPath = "",
			const std::vector<std::string>& filters = {},
			const std::string& filterDesc = ""
		);

		// Opens a native OS message box
		// okCancel = false : ok only
		// okCancel = true  : ok + cancel, returns true if user clicked ok
		static bool MessageBox(
			const std::string& title,
			const std::string& message,
			bool               okCancel = false
		);

		// Opens a native OS text input box
		// Returns empty string if user cancels
		static std::string InputBox(
			const std::string& title,
			const std::string& message,
			const std::string& defaultValue = ""
		);
#endif

	private:
		static FileResult<std::vector<uint8_t>> ReadFile(const std::string& path);

#ifdef EDITOR
		// Converts a vector of filter strings to the const char* array tinyfd expects
		static std::vector<const char*> BuildFilterList(const std::vector<std::string>& filters);
#endif

		static std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& data);
		static std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& data);
	};

} // namespace Struktur