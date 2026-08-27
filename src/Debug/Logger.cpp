#include "Logger.h"

#include <cstdio>

namespace
{
Struktur::Debug::LogCallback g_logCallback;

const char* LevelName(int level)
{
	switch (level)
	{
		case Struktur::Debug::LOG_TRACE:
			return "TRACE";
		case Struktur::Debug::LOG_DEBUG:
			return "DEBUG";
		case Struktur::Debug::LOG_INFO:
			return "INFO";
		case Struktur::Debug::LOG_WARNING:
			return "WARNING";
		case Struktur::Debug::LOG_ERROR:
			return "ERROR";
		case Struktur::Debug::LOG_FATAL:
			return "FATAL";
		default:
			return "LOG";
	}
}
}  // namespace

void Struktur::Debug::Log(int level, const char* message)
{
	std::fprintf(level >= LOG_ERROR ? stderr : stdout, "%s: %s\n", LevelName(level), message);
	if (g_logCallback)
	{
		g_logCallback(level, message);
	}
}

void Struktur::Debug::SetLogCallback(LogCallback callback)
{
	g_logCallback = std::move(callback);
}
