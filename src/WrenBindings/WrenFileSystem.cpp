#include "WrenFileSystem.h"

#include "Debug/Assertions.h"
#include "Engine/GameContext.h"
#include "Engine/Scripting/WrenBindingRegistry.h"

// ============================================================================
// FILE SYSTEM RESULT BINDINGS
// ============================================================================

void wren_FileSystemResultAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(Struktur::FileResult<void>));
}

void wren_FileSystemResultFinalize(void* data)
{
	WrenFileResult* result = static_cast<WrenFileResult*>(data);
	result->~WrenFileResult();
}

// Result.success
void wren_FileSystemResultGetSuccess(WrenVM* vm)
{
	WrenFileResult* result = static_cast<WrenFileResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, result->fileResult.success);
}

// Result.error
void wren_FileSystemResultGetError(WrenVM* vm)
{
	WrenFileResult* result = static_cast<WrenFileResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, static_cast<double>(result->fileResult.error));
}

// Result.errorMessage
void wren_FileSystemResultGetErrorMessage(WrenVM* vm)
{
	WrenFileResult* result = static_cast<WrenFileResult*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, result->fileResult.errorMessage.c_str());
}

// ============================================================================
// FILE SYSTEM BYTES RESULT BINDINGS
// ============================================================================

void wren_FileSystemBytesResultAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenFileResultByte));
}

void wren_FileSystemBytesResultFinalize(void* data)
{
	WrenFileResultByte* result = static_cast<WrenFileResultByte*>(data);
	result->~WrenFileResultByte();
}

// BytesResult.success
void wren_FileSystemBytesResultGetSuccess(WrenVM* vm)
{
	WrenFileResultByte* result = static_cast<WrenFileResultByte*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, result->fileResult.success);
}

// BytesResult.value
void wren_FileSystemBytesResultGetValue(WrenVM* vm)
{
	WrenFileResultByte* result = static_cast<WrenFileResultByte*>(wrenGetSlotForeign(vm, 0));

	if (!result->fileResult.success)
	{
		wrenSetSlotNull(vm, 0);
		return;
	}

	const std::vector<uint8_t>& bytes = result->fileResult.value;

	// Create a Wren List to hold the bytes
	// We need an extra slot for each element we insert
	wrenEnsureSlots(vm, 2);
	wrenSetSlotNewList(vm, 0);

	for (uint8_t byte : bytes)
	{
		wrenSetSlotDouble(vm, 1, static_cast<double>(byte));
		wrenInsertInList(vm, 0, -1, 1);  // -1 = append to end
	}
}

// BytesResult.error
void wren_FileSystemBytesResultGetError(WrenVM* vm)
{
	WrenFileResultByte* result = static_cast<WrenFileResultByte*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, static_cast<double>(result->fileResult.error));
}

// BytesResult.errorMessage
void wren_FileSystemBytesResultGetErrorMessage(WrenVM* vm)
{
	WrenFileResultByte* result = static_cast<WrenFileResultByte*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, result->fileResult.errorMessage.c_str());
}

// ============================================================================
// FILE SYSTEM STRING RESULT BINDINGS
// ============================================================================

void wren_FileSystemStringResultAllocate(WrenVM* vm)
{
	wrenSetSlotNewForeign(vm, 0, 0, sizeof(WrenFileResultString));
}

void wren_FileSystemStringResultFinalize(void* data)
{
	WrenFileResultString* result = static_cast<WrenFileResultString*>(data);
	result->~WrenFileResultString();
}

// StringResult.success
void wren_FileSystemStringResultGetSuccess(WrenVM* vm)
{
	WrenFileResultString* result = static_cast<WrenFileResultString*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotBool(vm, 0, result->fileResult.success);
}

// StringResult.value
void wren_FileSystemStringResultGetValue(WrenVM* vm)
{
	WrenFileResultString* result = static_cast<WrenFileResultString*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, result->fileResult.value.c_str());
}

// StringResult.error
void wren_FileSystemStringResultGetError(WrenVM* vm)
{
	WrenFileResultString* result = static_cast<WrenFileResultString*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotDouble(vm, 0, static_cast<double>(result->fileResult.error));
}

// StringResult.errorMessage
void wren_FileSystemStringResultGetErrorMessage(WrenVM* vm)
{
	WrenFileResultString* result = static_cast<WrenFileResultString*>(wrenGetSlotForeign(vm, 0));
	wrenSetSlotString(vm, 0, result->fileResult.errorMessage.c_str());
}

// ============================================================================
// FILE SYSTEM BINDINGS
// ============================================================================

// FileSystem.seedFromDefaults(_,_)
void wren_DialogueManagerSeedFromDefaults(WrenVM* vm)
{
	const char* sourcePath            = wrenGetSlotString(vm, 1);
	const char* DestPath              = wrenGetSlotString(vm, 2);
	Struktur::FileResult<void> result = Struktur::FileSystem::SeedFromDefaults(sourcePath, DestPath);
	wrenGetVariable(vm, "fileSystem", "Result", 1);  // Get class into slot 1
	WrenFileResult* wrenResult = static_cast<WrenFileResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResult)));
	new (wrenResult) WrenFileResult(result);
}

// FileSystem.readBytes(_)
void wren_DialogueManagerReadBytes(WrenVM* vm)
{
	const char* path                                  = wrenGetSlotString(vm, 1);
	Struktur::FileResult<std::vector<uint8_t>> result = Struktur::FileSystem::ReadBytes(path);
	wrenGetVariable(vm, "fileSystem", "ByteResult", 1);  // Get class into slot 1
	WrenFileResultByte* wrenResult =
	    static_cast<WrenFileResultByte*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResultByte)));
	new (wrenResult) WrenFileResultByte(result);
}

// FileSystem.readString(_)
void wren_DialogueManagerReadString(WrenVM* vm)
{
	const char* path                         = wrenGetSlotString(vm, 1);
	Struktur::FileResult<std::string> result = Struktur::FileSystem::ReadString(path);
	wrenGetVariable(vm, "fileSystem", "StringResult", 1);  // Get class into slot 1
	WrenFileResultString* wrenResult =
	    static_cast<WrenFileResultString*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResultString)));
	new (wrenResult) WrenFileResultString(result);
}

// FileSystem.readEncrypted(_)
void wren_DialogueManagerReadEncrypted(WrenVM* vm)
{
	const char* path                         = wrenGetSlotString(vm, 1);
	Struktur::FileResult<std::string> result = Struktur::FileSystem::ReadEncrypted(path);
	wrenGetVariable(vm, "fileSystem", "StringResult", 1);  // Get class into slot 1
	WrenFileResultString* wrenResult =
	    static_cast<WrenFileResultString*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResultString)));
	new (wrenResult) WrenFileResultString(result);
}

// FileSystem.writeBytes(_,_)
void wren_DialogueManagerWriteBytes(WrenVM* vm)
{
	const char* path = wrenGetSlotString(vm, 1);

	// Read the Wren List from slot 2
	// We need an extra slot to read each element out of the list
	wrenEnsureSlots(vm, 4);
	int count = wrenGetListCount(vm, 2);

	std::vector<uint8_t> bytes;
	bytes.reserve(count);

	for (int i = 0; i < count; i++)
	{
		// Extract each element into slot 3
		wrenGetListElement(vm, 2, i, 3);
		uint8_t byte = static_cast<uint8_t>(wrenGetSlotDouble(vm, 3));
		bytes.push_back(byte);
	}

	Struktur::FileResult<void> result = Struktur::FileSystem::WriteBytes(path, bytes);
	wrenGetVariable(vm, "fileSystem", "Result", 1);  // Get class into slot 1
	WrenFileResult* wrenResult = static_cast<WrenFileResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResult)));
	new (wrenResult) WrenFileResult(result);
}

// FileSystem.writeString(_,_)
void wren_DialogueManagerWriteString(WrenVM* vm)
{
	const char* path                  = wrenGetSlotString(vm, 1);
	const char* data                  = wrenGetSlotString(vm, 2);
	Struktur::FileResult<void> result = Struktur::FileSystem::WriteString(path, data);
	wrenGetVariable(vm, "fileSystem", "Result", 1);  // Get class into slot 1
	WrenFileResult* wrenResult = static_cast<WrenFileResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResult)));
	new (wrenResult) WrenFileResult(result);
}

// FileSystem.writeEncrypted(_,_)
void wren_DialogueManagerWriteEncrypted(WrenVM* vm)
{
	const char* path                  = wrenGetSlotString(vm, 1);
	const char* data                  = wrenGetSlotString(vm, 2);
	Struktur::FileResult<void> result = Struktur::FileSystem::WriteEncrypted(path, data);
	wrenGetVariable(vm, "fileSystem", "Result", 1);  // Get class into slot 1
	WrenFileResult* wrenResult = static_cast<WrenFileResult*>(wrenSetSlotNewForeign(vm, 0, 1, sizeof(WrenFileResult)));
	new (wrenResult) WrenFileResult(result);
}

// FileSystem.syncSaves()
void wren_DialogueManagerSyncSaves(WrenVM* vm)
{
	Struktur::FileSystem::SyncSaves();
}

// FileSystem.exists(_)
void wren_DialogueManagerExists(WrenVM* vm)
{
	const char* path = wrenGetSlotString(vm, 1);
	bool exists      = Struktur::FileSystem::Exists(path);
	wrenSetSlotBool(vm, 0, exists);
}

// ============================================================================
// BINDING REGISTRATION
// ============================================================================

WREN_BINDING_MODULE(FileSystem)
{
	WREN_ENUM(registry, "fileSystem", FileSystemError, "Enum for the error codes for the file system",
	          WREN_ENUM_PAIR("FileNotFound", Struktur::FileSystemError::FileNotFound),
	          WREN_ENUM_PAIR("PermissionDenied", Struktur::FileSystemError::PermissionDenied),
	          WREN_ENUM_PAIR("InvalidPath", Struktur::FileSystemError::InvalidPath),
	          WREN_ENUM_PAIR("ReadError", Struktur::FileSystemError::ReadError),
	          WREN_ENUM_PAIR("WriteError", Struktur::FileSystemError::WriteError),
	          WREN_ENUM_PAIR("MountFailed", Struktur::FileSystemError::MountFailed),
	          WREN_ENUM_PAIR("NotInitialised", Struktur::FileSystemError::NotInitialised),
	          WREN_ENUM_PAIR("Unknown", Struktur::FileSystemError::Unknown), );

	// DialogueData foreign class
	WREN_FOREIGN_CLASS(registry, "fileSystem", "Result", wren_FileSystemResultAllocate, wren_FileSystemResultFinalize,
	                   "Container for File System Result data");

	WREN_CLASS_METHOD(registry, "fileSystem", "Result", "success", wren_FileSystemResultGetSuccess,
	                  "Get the success of the File System Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "Result", "error", wren_FileSystemResultGetError,
	                  "Get the error of the File System Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "Result", "errorMessage", wren_FileSystemResultGetErrorMessage,
	                  "Get the error message of the File System Result");

	// DialogueData foreign class
	WREN_FOREIGN_CLASS(registry, "fileSystem", "BytesResult", wren_FileSystemBytesResultAllocate,
	                   wren_FileSystemBytesResultFinalize, "Container for File System Bytes Result data");

	WREN_CLASS_METHOD(registry, "fileSystem", "BytesResult", "success", wren_FileSystemBytesResultGetSuccess,
	                  "Get the success of the File System Bytes Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "BytesResult", "value", wren_FileSystemBytesResultGetValue,
	                  "Get the success of the File System Bytes Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "BytesResult", "error", wren_FileSystemBytesResultGetError,
	                  "Get the error of the File System Bytes Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "BytesResult", "errorMessage", wren_FileSystemBytesResultGetErrorMessage,
	                  "Get the error message of the File System Bytes Result");

	// DialogueData foreign class
	WREN_FOREIGN_CLASS(registry, "fileSystem", "StringResult", wren_FileSystemStringResultAllocate,
	                   wren_FileSystemStringResultFinalize, "Container for File System String Result data");

	WREN_CLASS_METHOD(registry, "fileSystem", "StringResult", "success", wren_FileSystemStringResultGetSuccess,
	                  "Get the success of the File System String Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "StringResult", "value", wren_FileSystemStringResultGetValue,
	                  "Get the success of the File System String Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "StringResult", "error", wren_FileSystemStringResultGetError,
	                  "Get the error of the File System String Result");
	WREN_CLASS_METHOD(registry, "fileSystem", "StringResult", "errorMessage",
	                  wren_FileSystemStringResultGetErrorMessage,
	                  "Get the error message of the File System String Result");

	// FileSystem static methods
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "seedFromDefaults(_,_)",
	                  wren_DialogueManagerSeedFromDefaults, "Copy a file from the read dir to the write dir");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "readBytes(_)", wren_DialogueManagerReadBytes,
	                  "Read data from a file as bytes");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "readString(_)", wren_DialogueManagerReadString,
	                  "Read data from a file as String");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "readEncrypted(_)", wren_DialogueManagerReadEncrypted,
	                  "Read encrypted data from a file as string");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "writeBytes(_,_)", wren_DialogueManagerWriteBytes,
	                  "Write data to a file as bytes");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "writeString(_,_)", wren_DialogueManagerWriteString,
	                  "Write data to a file as String");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "writeEncrypted(_,_)", wren_DialogueManagerWriteEncrypted,
	                  "Write encrypted data to a file as string");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "syncSaves()", wren_DialogueManagerSyncSaves,
	                  "Syncs in-memory writes to IndexedDB so they persist between sessions: needed for wasm builds");
	WREN_CLASS_STATIC(registry, "fileSystem", "FileSystem", "exists(_)", wren_DialogueManagerExists,
	                  "Check if a file exists in the file system");
}
