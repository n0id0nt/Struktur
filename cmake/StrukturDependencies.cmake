# Third-party dependencies (the engine's git submodules) and the engine's two standalone libraries, jobs and fitness.

# Fetch only the submodules that are missing: updating one that's already there would move a checkout you're working on.
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${STRUKTUR_ENGINE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule status --recursive
        WORKING_DIRECTORY ${STRUKTUR_ENGINE_DIR}
        OUTPUT_VARIABLE submodule_status
    )
    string(REPLACE "\n" ";" submodule_lines "${submodule_status}")
    foreach(line IN LISTS submodule_lines)
        # A leading '-' marks a submodule that hasn't been initialised.
        if(line MATCHES "^-[0-9a-f]+ ([^ ]+)")
            set(missing ${CMAKE_MATCH_1})
            get_filename_component(missing_parent "${STRUKTUR_ENGINE_DIR}/${missing}" DIRECTORY)
            get_filename_component(missing_name "${missing}" NAME)
            message(STATUS "Fetching submodule ${missing}")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${missing_name}
                WORKING_DIRECTORY ${missing_parent}
            )
        endif()
    endforeach()
endif()

set(third_party ${STRUKTUR_ENGINE_DIR}/third_party)
set(third_party_build ${STRUKTUR_BINARY_DIR}/third_party)

set(ENTT_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_INSTALL OFF CACHE BOOL "" FORCE)
set(PHYSFS_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# SDL3 - windowing (Engine/Platform/Window) and audio (SDL3_mixer) on desktop and web.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory(${third_party}/SDL ${third_party_build}/SDL)

# Works around SDL3's target_precompile_headers() conflicting with MSVC's /Zi (C2855); harmless under Emscripten.
set_target_properties(SDL3-static PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

# SDL3_mixer: sound effects are WAV and music is OGG (stb_vorbis, header-only - no external lib), so everything else
# is disabled.
set(SDLMIXER_FLAC OFF CACHE BOOL "" FORCE)
set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MP3 OFF CACHE BOOL "" FORCE)
set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_STB ON CACHE BOOL "" FORCE)
set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)

# SDL3_mixer reads BUILD_SHARED_LIBS directly (unlike SDL3's own SDL_SHARED/SDL_STATIC); force static.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${third_party}/SDL_mixer ${third_party_build}/SDL_mixer)

# bgfx - the rendering backend for both platforms. shaderc (the shader *compiler*) only ever runs on desktop - it's a
# native host tool. Web consumes the headers a desktop build already compiled (see StrukturShaders.cmake), so build
# desktop at least once before building web from a clean checkout.
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
if(PLATFORM_DESKTOP)
    set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
else()
    # Nothing on web ever invokes shaderc/geometryc/texturec/bin2c as build-time tools, so don't even try to build
    # them as Emscripten "host" executables (they wouldn't be runnable during this same build anyway).
    set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
endif()
add_subdirectory(${third_party}/bgfx.cmake ${third_party_build}/bgfx.cmake)

if(PLATFORM_WEB)
    # bx.cmake (third_party/bgfx.cmake/cmake/bx/bx.cmake) unconditionally adds -msse4.2 whenever
    # CMAKE_SYSTEM_PROCESSOR looks like x86 - bgfx.cmake's own CMakeLists re-triggers CMake's compiler/system
    # detection internally (via enable_language()), which re-populates CMAKE_SYSTEM_PROCESSOR from the actual HOST
    # machine (x86_64) even under the Emscripten toolchain, so overriding the variable ourselves beforehand doesn't
    # stick - confirmed by direct inspection, bx.cmake sees "x86" regardless. That flag would need pairing with
    # -msimd128 to satisfy em++, but worse: it also makes bimg's vendored third-party encoder (etcpak) think real
    # x86 SSE/AVX intrinsics are available and try to `#include <x86intrin.h>`, which doesn't exist for wasm32.
    # Stripping it back off post-hoc (rather than patching the vendored bx.cmake) lets bx fall back to its
    # portable/reference SIMD implementation.
    get_target_property(BX_COMPILE_OPTIONS bx COMPILE_OPTIONS)
    if(BX_COMPILE_OPTIONS)
        list(FILTER BX_COMPILE_OPTIONS EXCLUDE REGEX "msse4\\.2")
        set_target_properties(bx PROPERTIES COMPILE_OPTIONS "${BX_COMPILE_OPTIONS}")
    endif()
    get_target_property(BX_INTERFACE_COMPILE_OPTIONS bx INTERFACE_COMPILE_OPTIONS)
    if(BX_INTERFACE_COMPILE_OPTIONS)
        list(FILTER BX_INTERFACE_COMPILE_OPTIONS EXCLUDE REGEX "msse4\\.2")
        set_target_properties(bx PROPERTIES INTERFACE_COMPILE_OPTIONS "${BX_INTERFACE_COMPILE_OPTIONS}")
    endif()
endif()

# add_subdirectory() (rather than find_package()) skips bgfx.cmake's installed Config.cmake, so the
# variable/function it would have set up (BGFX_SHADER_INCLUDE_PATH, bgfx_compile_shaders()) are set up by hand.
set(BGFX_SHADER_INCLUDE_PATH ${third_party}/bgfx.cmake/bgfx/src)
include(${third_party}/bgfx.cmake/cmake/bgfxToolUtils.cmake)

# TextureResource decodes images via bimg's already-vendored stb_image, and FontResource bakes glyph atlases via
# bgfx's vendored stb_truetype. Neither header path is exposed by those CMake targets (and they're different stb/
# folders: bgfx.cmake vendors its own copy under bgfx/3rdparty).
set(STB_IMAGE_INCLUDE_DIR ${third_party}/bgfx.cmake/bimg/3rdparty/stb)
set(STB_TRUETYPE_INCLUDE_DIR ${third_party}/bgfx.cmake/bgfx/3rdparty/stb)

# EnTT
add_subdirectory(${third_party}/entt ${third_party_build}/entt)

# GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(${third_party}/glm ${third_party_build}/glm)

# nlohmann/json
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(${third_party}/json ${third_party_build}/json)

# Box2D
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED    OFF CACHE BOOL "" FORCE)
add_subdirectory(${third_party}/box2d ${third_party_build}/box2d)

# PhysicsFS
set(PHYSFS_BUILD_STATIC TRUE  CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_SHARED FALSE CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST   FALSE CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS   FALSE CACHE BOOL "" FORCE)
set(PHYSFS_TARGETNAME_UNINSTALL "physfs_uninstall" CACHE STRING "" FORCE)
add_subdirectory(${third_party}/physfs ${third_party_build}/physfs)

# StrukturJobs - the job system (docs/JobSystem.md), part of the core library. STRUKTUR_THREADS (off on web by default)
# decides whether it starts worker threads; its tests and job_bench build when src/Engine/Jobs is configured on its own.
set(STRUKTUR_JOBS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(STRUKTUR_JOBS_BUILD_BENCH OFF CACHE BOOL "" FORCE)
add_subdirectory(${STRUKTUR_ENGINE_DIR}/src/Engine/Jobs ${STRUKTUR_BINARY_DIR}/Jobs)

# StrukturFitness - BLE smart trainer / heart rate / power meter support (docs/SmartTrainer.md), the heart of the
# fitness library. Its unit tests and the trainer_probe tool are off here; configure src/Engine/Fitness on its own to
# build them.
option(STRUKTUR_BUILD_TESTS "Build the StrukturFitness unit tests" OFF)
option(STRUKTUR_BUILD_TOOLS "Build trainer_probe" OFF)
add_subdirectory(${STRUKTUR_ENGINE_DIR}/src/Engine/Fitness ${STRUKTUR_BINARY_DIR}/Fitness)

# Wren (manual build)
file(GLOB WREN_SOURCES
    ${third_party}/wren/src/vm/*.c
    ${third_party}/wren/src/optional/*.c
    ${STRUKTUR_ENGINE_DIR}/WrenExtensions/Modules/Reflect/*.c
    ${STRUKTUR_ENGINE_DIR}/WrenExtensions/Modules/Trace/*.c
)
add_library(wren STATIC ${WREN_SOURCES})
target_compile_definitions(wren PUBLIC WREN_OPT_META=1 WREN_OPT_RANDOM=1 WREN_OPT_REFLECT=1)
target_include_directories(wren PUBLIC
    ${third_party}/wren/src/include
    ${third_party}/wren/src/optional
    ${STRUKTUR_ENGINE_DIR}/WrenExtensions/Modules
)
target_include_directories(wren PRIVATE
    ${third_party}/wren/src/vm
    ${third_party}/wren/src/optional
)
if(WIN32)
    target_compile_definitions(wren PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Editor-only libs
if(EDITOR)
    # ImGui
    add_library(imgui STATIC
        ${third_party}/imgui/imgui.cpp
        ${third_party}/imgui/imgui_demo.cpp
        ${third_party}/imgui/imgui_draw.cpp
        ${third_party}/imgui/imgui_tables.cpp
        ${third_party}/imgui/imgui_widgets.cpp
        ${third_party}/imgui/misc/cpp/imgui_stdlib.cpp
    )
    target_include_directories(imgui PUBLIC ${third_party}/imgui)
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

    # Input/platform plumbing comes from ImGui's own official SDL3 backend (rendering is our own bgfx backend, see
    # Engine/Renderer/ImGuiRenderer - the "ForOther" variant doesn't need a paired renderer backend).
    target_sources(imgui PRIVATE ${third_party}/imgui/backends/imgui_impl_sdl3.cpp)
    target_include_directories(imgui PUBLIC ${third_party}/imgui/backends)
    target_link_libraries(imgui PUBLIC SDL3::SDL3)

    # tinyfiledialogs
    add_library(tinyfiledialogs STATIC ${third_party}/tinyfiledialogs/tinyfiledialogs.c)
    target_include_directories(tinyfiledialogs PUBLIC ${third_party}/tinyfiledialogs)
endif()

# The shaders the core and UI libraries embed.
struktur_add_shaders(CoreShaders DIRECTORY ${STRUKTUR_ENGINE_DIR}/shaders/core
    VARYING ${STRUKTUR_ENGINE_DIR}/shaders/varying.def.sc SHADERS sprite)
struktur_add_shaders(UIShaders DIRECTORY ${STRUKTUR_ENGINE_DIR}/shaders/ui
    VARYING ${STRUKTUR_ENGINE_DIR}/shaders/varying.def.sc SHADERS spriteCoverage richTextAnim)
