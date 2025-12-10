#pragma once

#include <string>
#include <sstream>
#include <cstdio>
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
        
        // Variadic template function for printf-style formatting
        template<typename... Args>
        inline std::string FormatString(const char* format, Args&&... args)
        {
            // Calculate required buffer size
            int size = snprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1;
            if (size <= 0) return std::string(format);
            
            // Allocate buffer and format string
            std::string result(size, '\0');
            snprintf(&result[0], size, format, std::forward<Args>(args)...);
            result.resize(size - 1); // Remove null terminator
            return result;
        }
        
        // Overload for no arguments (just return the format string)
        inline std::string FormatString(const char* format)
        {
            return std::string(format);
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
        
        // Overload for string message (backward compatibility)
        inline void Log(Level level, const char* file, int line, const char* func, const std::string& message)
        {
            std::string formatted = FormatMessage(file, line, func, message);
            TraceLog(ToRaylibLogLevel(level), formatted.c_str());
        }
        
        // Overload for const char* message (backward compatibility)
        inline void Log(Level level, const char* file, int line, const char* func, const char* message)
        {
            std::string formatted = FormatMessage(file, line, func, std::string(message));
            TraceLog(ToRaylibLogLevel(level), formatted.c_str());
        }
        
        // Template version of Log that accepts format arguments
        template<typename... Args>
        inline void Log(Level level, const char* file, int line, const char* func, const char* format, Args&&... args)
        {
            std::string message = FormatString(format, std::forward<Args>(args)...);
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
        
        // Template version for formatted assert messages
        template<typename... Args>
        inline bool AssertImpl(bool condition, const char* conditionStr, const char* file, int line, const char* func, const char* format, Args&&... args)
        {
            if (!condition)
            {
                std::string message = FormatString(format, std::forward<Args>(args)...);
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
        
        // Template version for formatted break messages
        template<typename... Args>
        inline bool BreakImpl(const char* file, int line, const char* func, const char* format, Args&&... args)
        {
            std::string message = FormatString(format, std::forward<Args>(args)...);
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
#define DEBUG_LOG(level, ...) \
    Struktur::Debug::Log(level, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// Specific log level macros
#define DEBUG_INFO(...) DEBUG_LOG(Struktur::Debug::Level::LEVEL_INFO, __VA_ARGS__)
#define DEBUG_WARNING(...) DEBUG_LOG(Struktur::Debug::Level::LEVEL_WARNING, __VA_ARGS__)
#define DEBUG_ERROR(...) DEBUG_LOG(Struktur::Debug::Level::LEVEL_ERROR, __VA_ARGS__)
#define DEBUG_FATAL(...) DEBUG_LOG(Struktur::Debug::Level::LEVEL_FATAL, __VA_ARGS__)

// Always-on assert (works in both debug and release)
#define ASSERT(condition) \
    Struktur::Debug::AssertImpl(condition, #condition, __FILE__, __LINE__, __FUNCTION__)

#define ASSERT_MSG(condition, ...) \
    Struktur::Debug::AssertImpl(condition, #condition, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define BREAK \
    Struktur::Debug::BreakImpl(__FILE__, __LINE__, __FUNCTION__)

#define BREAK_MSG(...) \
    Struktur::Debug::BreakImpl(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// Usage examples:
/*

int main()
{
    // Initialise Raylib
    InitWindow(800, 600, "Debug Assert Example");
    SetTraceLogLevel(LOG_ALL); // Enable all log levels
    
    int value = 42;
    const char* name = "TestObject";
    
    // Basic assert
    ASSERT(value > 0);
    
    // Assert with formatted message
    ASSERT_MSG(value < 100, "Value %d should be less than 100", value);
    
    // Different log levels with formatting
    DEBUG_INFO("This is an info message");
    DEBUG_WARNING("Warning: Value is %d", value);
    DEBUG_ERROR("Error in object: %s", name);
    
    // Printf-style formatting
    DEBUG_WARNING("Wren binding not found: %s.%s%s (static=%d)",
                  "module", "ClassName", "signature", 1);
    
    // Break with formatted message
    // BREAK_MSG("Critical error at position (%d, %d)", 10, 20);
    
    // Always-on assert (works in release too)
    ASSERT_MSG(value != 0, "Value cannot be zero (got %d)", value);
    
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
