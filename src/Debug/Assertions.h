#pragma once

#include <string>
#include <sstream>
#include "raylib.h"

// Platform detection
#ifdef PLATFORM_WEB
    #include <emscripten.h>
#elif PLATFORM_DESKTOP && defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#endif

namespace Struktur
{
    namespace Debug
    {
        
        enum class Level
        {
            LEVEL_INFO,
            LEVEL_WARNING,
            LEVEL_ERROR,
            LEVEL_FATAL
        };
        
        inline int ToRaylibLogLevel(Level level)
        {
            switch (level) {
                case Level::LEVEL_INFO:    return LOG_INFO;
                case Level::LEVEL_WARNING: return LOG_WARNING;
                case Level::LEVEL_ERROR:   return LOG_ERROR;
                case Level::LEVEL_FATAL:   return LOG_FATAL;
                default:                   return LOG_INFO;
            }
        }
        
        inline std::string FormatMessage(const char* file, int line, const char* func, const std::string& message)
        {
            std::stringstream ss;
            
            const char* filename = file;
            const char* lastSlash = strrchr(file, '/');
            const char* lastBackslash = strrchr(file, '\\');
            if (lastSlash || lastBackslash) {
                filename = (lastSlash > lastBackslash) ? lastSlash + 1 : lastBackslash + 1;
            }
            
            ss << "[" << filename << ":" << line << " in " << func << "] " << message;
            return ss.str();
        }
        
        inline void Log(Level level, const char* file, int line, const char* func, const std::string& message)
        {
            std::string formatted = FormatMessage(file, line, func, message);
            TraceLog(ToRaylibLogLevel(level), formatted.c_str());
        }
        
        // Break/halt execution based on platform
        inline void DebugBreak()
        {
            #ifdef PLATFORM_WINDOWS
                __debugbreak();
            #elif defined(PLATFORM_WEB)
                // For web, we can't really break, so we'll throw an exception or use emscripten_force_exit
                EM_ASM({
                    console.error("Debug break triggered!");
                    debugger; // This will break in browser debugger if dev tools are open
                });
            #else
                // For other platforms (Linux, Mac)
                #ifdef __GNUC__
                    __builtin_trap();
                #else
                    raise(SIGTRAP);
                #endif
            #endif
        }
        
        inline bool AssertImpl(bool condition, const char* conditionStr, const char* file, int line, const char* func, const std::string& message = "")
        {
            if (!condition)
            {
                std::stringstream ss;
                ss << "ASSERTION FAILED: " << conditionStr;
                if (!message.empty())
                {
                    ss << " - " << message;
                }
                
                Log(Level::LEVEL_FATAL, file, line, func, ss.str());
                
#ifdef DEBUG
				DebugBreak();
#endif
                
                return false;
            }
            return true;
        }
        
        inline bool BreakImpl(const char* file, int line, const char* func, const std::string& message = "")
        {
			std::stringstream ss;
            ss << "BREAK: ";
			if (!message.empty())
            {
				ss << " - " << message;
			}

			Log(Level::LEVEL_FATAL, file, line, func, ss.str());

#ifdef DEBUG
			DebugBreak();
#endif

			return false;
        }
    }
}

// Macro definitions
#define DEBUG_LOG(level, message) \
    Struktur::Debug::Log(level, __FILE__, __LINE__, __FUNCTION__, message)

// Specific log level macros
#define DEBUG_INFO(message) DEBUG_LOG(Struktur::Debug::Level::LEVEL_INFO, message)
#define DEBUG_WARNING(message) DEBUG_LOG(Struktur::Debug::Level::LEVEL_WARNING, message)
#define DEBUG_ERROR(message) DEBUG_LOG(Struktur::Debug::Level::LEVEL_ERROR, message)
#define DEBUG_FATAL(message) DEBUG_LOG(Struktur::Debug::Level::LEVEL_FATAL, message)

// Always-on assert (works in both debug and release)
#define ASSERT(condition) \
    Struktur::Debug::AssertImpl(condition, #condition, __FILE__, __LINE__, __FUNCTION__)

#define ASSERT_MSG(condition, message) \
    Struktur::Debug::AssertImpl(condition, #condition, __FILE__, __LINE__, __FUNCTION__, message)

#define BREAK \
    Struktur::Debug::BreakImpl(__FILE__, __LINE__, __FUNCTION__)

#define BREAK_MSG(message) \
    Struktur::Debug::BreakImpl(__FILE__, __LINE__, __FUNCTION__, message)

// Usage examples:
/*

int main()
{
    // Initialize Raylib
    InitWindow(800, 600, "Debug Assert Example");
    SetTraceLogLevel(LOG_ALL); // Enable all log levels
    
    int value = 42;
    
    // Basic assert
    DEBUG_ASSERT(value > 0);
    
    // Assert with custom message
    DEBUG_ASSERT_MSG(value < 100, "Value should be less than 100");
    
    // Different log levels
    DEBUG_INFO("This is an info message");
    DEBUG_WARNING("This is a warning message");
    DEBUG_ERROR("This is an error message");
    
    // This will trigger an assertion failure and break in debug mode
    // DEBUG_ASSERT(value > 100);
    
    // Always-on assert (works in release too)
    ASSERT_MSG(value != 0, "Value cannot be zero");
    
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Check console for debug output", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}

*/