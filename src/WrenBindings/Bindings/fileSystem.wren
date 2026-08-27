// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from C++ bindings
// Module: fileSystem

// Enum for the error codes for the file system
class FileSystemError {
    static FileNotFound { 0 }
    static PermissionDenied { 1 }
    static InvalidPath { 2 }
    static ReadError { 3 }
    static WriteError { 4 }
    static MountFailed { 5 }
    static NotInitialised { 6 }
    static Unknown { 7 }
}

// Container for File System Result data
foreign class Result {
    // Get the success of the File System Result
    foreign success
    // Get the error of the File System Result
    foreign error
    // Get the error message of the File System Result
    foreign errorMessage
}

// Container for File System Bytes Result data
foreign class BytesResult {
    // Get the success of the File System Bytes Result
    foreign success
    // Get the success of the File System Bytes Result
    foreign value
    // Get the error of the File System Bytes Result
    foreign error
    // Get the error message of the File System Bytes Result
    foreign errorMessage
}

// Container for File System String Result data
foreign class StringResult {
    // Get the success of the File System String Result
    foreign success
    // Get the success of the File System String Result
    foreign value
    // Get the error of the File System String Result
    foreign error
    // Get the error message of the File System String Result
    foreign errorMessage
}

class FileSystem {
    // Copy a file from the read dir to the write dir
    foreign static seedFromDefaults(arg0,arg1)
    // Read data from a file as bytes
    foreign static readBytes(arg0)
    // Read data from a file as String
    foreign static readString(arg0)
    // Read encrypted data from a file as string
    foreign static readEncrypted(arg0)
    // Write data to a file as bytes
    foreign static writeBytes(arg0,arg1)
    // Write data to a file as String
    foreign static writeString(arg0,arg1)
    // Write encrypted data to a file as string
    foreign static writeEncrypted(arg0,arg1)
    // Syncs in-memory writes to IndexedDB so they persist between sessions: needed for wasm builds
    foreign static syncSaves()
    // Check if a file exists in the file system
    foreign static exists(arg0)
}

