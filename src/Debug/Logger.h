#pragma once

#include <functional>

namespace Struktur
{
namespace Debug
{
// Mirrors raylib's TraceLogLevel values verbatim - LogWindow's existing level-name/color/filter mapping keys
// off these numbers directly, so keeping them identical avoids touching that mapping at all.
enum LogLevel
{
	LOG_ALL = 0,  // Display all logs
	LOG_TRACE,    // Trace logging, intended for internal use only
	LOG_DEBUG,    // Debug logging, used for internal debugging, it should be disabled on release builds
	LOG_INFO,     // Info logging, used for program execution info
	LOG_WARNING,  // Warning logging, used on recoverable failures
	LOG_ERROR,    // Error logging, used on unrecoverable failures
	LOG_FATAL,    // Fatal logging, used to abort program: exit(EXIT_FAILURE)
	LOG_NONE      // Disable logging
};

using LogCallback = std::function<void(int level, const char* message)>;

// Replaces raylib's ::TraceLog - writes "LEVEL: message" to stdout and forwards to the optional callback below
// (message is already fully formatted by the caller - see Assertions.h's Log() - so no printf-style args here).
void Log(int level, const char* message);

// Replaces raylib's ::SetTraceLogCallback - lets LogWindow capture every Log() call into its own view instead
// of (or in addition to) stdout.
void SetLogCallback(LogCallback callback);
}  // namespace Debug
}  // namespace Struktur
