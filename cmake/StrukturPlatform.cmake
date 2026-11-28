# Language, platform and configuration settings every target of a game shares.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Prevent all subprojects from installing
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
set(SKIP_INSTALL_ALL ON)

option(USE_ASYNCIFY "Enable ASYNCIFY for traditional game loops" OFF)

# Platform detection
if(EMSCRIPTEN)
    set(PLATFORM_WEB TRUE)
    # PLATFORM_WEB/PLATFORM_DESKTOP are preprocessor macros our own code checks via #if defined(PLATFORM_WEB).
    add_compile_definitions(PLATFORM_WEB)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    # Fix missing Emscripten CMake variables
    if(NOT CMAKE_C_COMPILE_OBJECT)
        set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
    endif()
    if(NOT CMAKE_C_ARCHIVE_CREATE)
        set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    endif()
    if(NOT CMAKE_C_ARCHIVE_FINISH)
        set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> <TARGET>")
    endif()

    # Also set CXX variants for completeness
    if(NOT CMAKE_CXX_COMPILE_OBJECT)
        set(CMAKE_CXX_COMPILE_OBJECT "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
    endif()
    if(NOT CMAKE_CXX_ARCHIVE_CREATE)
        set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>")
    endif()
    if(NOT CMAKE_CXX_ARCHIVE_FINISH)
        set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> <TARGET>")
    endif()

    # bx (third_party/bgfx.cmake/bx, vendored - see BX_PLATFORM_POSIX in bx/include/bx/platform.h) treats
    # Emscripten as a full POSIX platform and unconditionally uses pthread_mutex_t/nanosleep/sched_yield/dlopen
    # in os.cpp/mutex.cpp/thread.cpp - those symbols only exist when compiling with pthreads enabled. Applied
    # project-wide (before any add_subdirectory) so every object file agrees on the threading model - a mix of
    # pthread/non-pthread object files fails to link under Emscripten.
    add_compile_options(-pthread)
    add_link_options(-pthread)

    # bx/include/bx/platform.h detects Emscripten via `#elif defined(__wasm__)` (correctly matches) but then
    # computes BX_PLATFORM_EMSCRIPTEN from __EMSCRIPTEN_MAJOR__/_MINOR__/_TINY__ (uppercase) - macros that
    # existed in older Emscripten SDKs. This SDK (emscripten/version.h) only defines the lowercase
    # __EMSCRIPTEN_major__/_minor__/_tiny__ now, so the uppercase names silently evaluate to 0 in arithmetic
    # #if context, making BX_PLATFORM_EMSCRIPTEN (and therefore BX_PLATFORM_POSIX) evaluate false everywhere in
    # bx - it then skips <time.h>/<sched.h>/<dlfcn.h> includes while still trying to use their symbols. Alias
    # the old names to the new ones (re-expanded by the preprocessor once emscripten/version.h's real macros
    # are in scope) rather than patching the vendored submodule.
    add_compile_definitions(
        __EMSCRIPTEN_MAJOR__=__EMSCRIPTEN_major__
        __EMSCRIPTEN_MINOR__=__EMSCRIPTEN_minor__
        __EMSCRIPTEN_TINY__=__EMSCRIPTEN_tiny__
    )

    message(STATUS "Building for Web (Emscripten)")
else()
    set(PLATFORM_DESKTOP TRUE)
    add_compile_definitions(PLATFORM_DESKTOP)
    message(STATUS "Building for Desktop")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG TRUE)
    add_compile_definitions(DEBUG)
    # Defaults on for Debug builds, but can be turned off (-DEDITOR=FALSE) to get a Debug build that mounts assets/
    # live (see App::MountAssets) without compiling in the ImGui editor. The EDITOR define itself is set on the
    # engine libraries of each game (struktur_add_libraries).
    if(NOT DEFINED EDITOR)
        set(EDITOR TRUE)
    endif()
endif()

# Link the MSVC runtime statically
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
endif()
