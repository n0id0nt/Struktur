cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME "Struktur")
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

# Prevent all subprojects from installing
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
set(SKIP_INSTALL_ALL ON)

# Or more specifically:
set(ENTT_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_INSTALL OFF CACHE BOOL "" FORCE)
set(PHYSFS_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

##########################################################################################
# Configurable options
##########################################################################################

# toggle libs to be used with ON and OFF

option(USE_ASYNCIFY "Enable ASYNCIFY for traditional game loops" OFF)

# Platform detection
if(EMSCRIPTEN)
    set(PLATFORM_WEB TRUE)
    # PLATFORM_WEB/PLATFORM_DESKTOP below are real preprocessor macros our own code checks via
    # #if defined(PLATFORM_WEB) throughout - previously these were only ever defined as a side effect of
    # raylib's own CMake (target_compile_definitions(raylib PUBLIC PLATFORM_WEB)) propagating transitively
    # through target_link_libraries(Struktur ... raylib). Now that raylib is gone, define them directly.
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
    # Defaults on for Debug builds, but can be turned off (-DEDITOR=FALSE) to get a Debug build that mounts
    # assets/ live (see InitialiseGame's DEBUG-without-EDITOR branch) without compiling in the ImGui editor.
    if(NOT DEFINED EDITOR)
        set(EDITOR TRUE)
    endif()
    if(EDITOR)
        add_compile_definitions(EDITOR)
    endif()
endif()

##########################################################################################
# MSVC runtime linking
##########################################################################################

# this is here to ensure the runtime is linked staticly for msvc
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
endif()

##########################################################################################
# Dependencies (git submodules)
##########################################################################################

# Ensure submodules are initialised
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --no-fetch
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()

# SDL3 - windowing (Engine/Platform/Window) and audio (SDL3_mixer) on desktop and web.
if(PLATFORM_DESKTOP OR PLATFORM_WEB)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/SDL)

    # Works around SDL3's target_precompile_headers() conflicting with MSVC's /Zi (C2855); harmless under Emscripten.
    set_target_properties(SDL3-static PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

    # SDL3_mixer replaces raylib's raudio; only WAVE/AIFF/VOC/AU are needed (all assets are WAV), so disable the rest.
    set(SDLMIXER_FLAC OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MP3 OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_STB OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_VORBISFILE OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)

    # SDL3_mixer reads BUILD_SHARED_LIBS directly (unlike SDL3's own SDL_SHARED/SDL_STATIC); force static.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/SDL_mixer)
endif()

# bgfx - rendering backend for both platforms now, replacing raylib's rlgl on desktop and raylib's GLFW-
# emscripten path on web. shaderc (the actual shader *compiler*) only ever runs on desktop though - it's a
# native host tool, and cross-compiling it to Emscripten would produce a non-runnable "tool". Web instead
# consumes the already-compiled headers a desktop build already produced on disk (see STRUKTUR_SHADER_OUTPUTS
# below) - build desktop at least once before building web from a clean checkout.
if(PLATFORM_DESKTOP OR PLATFORM_WEB)
    set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
    if(PLATFORM_DESKTOP)
        set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
        set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
    else()
        # Nothing on web ever invokes shaderc/geometryc/texturec/bin2c as build-time tools, so don't even try to
        # build them as Emscripten "host" executables (they wouldn't be runnable during this same build anyway).
        set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(third_party/bgfx.cmake)

    if(PLATFORM_WEB)
        # bx.cmake (third_party/bgfx.cmake/cmake/bx/bx.cmake) unconditionally adds -msse4.2 whenever
        # CMAKE_SYSTEM_PROCESSOR looks like x86 - bgfx.cmake's own CMakeLists re-triggers CMake's compiler/
        # system detection internally (via enable_language()), which re-populates CMAKE_SYSTEM_PROCESSOR from
        # the actual HOST machine (x86_64) even under the Emscripten toolchain, so overriding the variable
        # ourselves beforehand doesn't stick - confirmed by direct inspection, bx.cmake sees "x86" regardless.
        # That flag would need pairing with -msimd128 to satisfy em++, but worse: it also makes bimg's vendored
        # third-party encoder (etcpak) think real x86 SSE/AVX intrinsics are available and try to
        # `#include <x86intrin.h>`, which doesn't exist for wasm32. Stripping it back off post-hoc (rather than
        # patching the vendored bx.cmake) lets bx fall back to its portable/reference SIMD implementation.
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
    set(BGFX_SHADER_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bgfx.cmake/bgfx/src)
    include(${CMAKE_CURRENT_SOURCE_DIR}/third_party/bgfx.cmake/cmake/bgfxToolUtils.cmake)

    # TextureResource decodes images via bimg's already-vendored stb_image rather than re-vendoring our own copy;
    # this header path isn't exposed by bimg's own CMake target, so it's added directly.
    set(STB_IMAGE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bgfx.cmake/bimg/3rdparty/stb)

    # FontResource bakes glyph atlases via bgfx's vendored stb_truetype (pack API) rather than raylib's font
    # loader - this header path isn't exposed by bgfx's own CMake target either, mirroring STB_IMAGE_INCLUDE_DIR
    # above. Note this is a DIFFERENT stb/ dir than bimg's (bgfx.cmake vendors its own copy under bgfx/3rdparty).
    set(STB_TRUETYPE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bgfx.cmake/bgfx/3rdparty/stb)

    set(STRUKTUR_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/Shaders)
    set(STRUKTUR_SHADER_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets/Shaders/compiled)
    set(STRUKTUR_SHADER_OUTPUTS "")

    if(PLATFORM_DESKTOP)
        # NAME must have no dots - shaderc's bin2c step derives a C array name from the shader filename. Default
        # PROFILES (see bgfxToolUtils.cmake) includes 100_es (GLES2/WebGL-compatible) alongside the desktop
        # profiles, for free, on every desktop build - that's what web's bgfx picks up at runtime below.
        function(struktur_compile_shader NAME)
            bgfx_compile_shaders(
                TYPE VERTEX
                SHADERS ${STRUKTUR_SHADER_DIR}/vs_${NAME}.sc
                VARYING_DEF ${STRUKTUR_SHADER_DIR}/varying.def.sc
                OUTPUT_DIR ${STRUKTUR_SHADER_OUTPUT_DIR}
                OUT_FILES_VAR ${NAME}_VS_OUT
                AS_HEADERS
            )
            bgfx_compile_shaders(
                TYPE FRAGMENT
                SHADERS ${STRUKTUR_SHADER_DIR}/fs_${NAME}.sc
                VARYING_DEF ${STRUKTUR_SHADER_DIR}/varying.def.sc
                OUTPUT_DIR ${STRUKTUR_SHADER_OUTPUT_DIR}
                OUT_FILES_VAR ${NAME}_FS_OUT
                AS_HEADERS
            )
            set(STRUKTUR_SHADER_OUTPUTS ${STRUKTUR_SHADER_OUTPUTS} ${${NAME}_VS_OUT} ${${NAME}_FS_OUT} PARENT_SCOPE)
        endfunction()
        struktur_compile_shader(sprite)
        struktur_compile_shader(spriteCoverage)
        struktur_compile_shader(soulEffect)
    else()
        # Web doesn't (can't) invoke shaderc itself - just reference whatever a prior desktop build already
        # generated on disk as plain existing sources (see the PLATFORM_DESKTOP branch above).
        file(GLOB_RECURSE STRUKTUR_SHADER_OUTPUTS ${STRUKTUR_SHADER_OUTPUT_DIR}/*.bin.h)
        if(NOT STRUKTUR_SHADER_OUTPUTS)
            message(WARNING "assets/Shaders/compiled is empty - build build-windows (Desktop) at least once "
                             "before building web, so bgfx has compiled shaders to embed.")
        endif()
    endif()
endif()

# EnTT
add_subdirectory(third_party/entt)

# GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glm)

# nlohmann/json
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/json)

# Box2D
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED    OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/box2d)

# PhysicsFS
set(PHYSFS_BUILD_STATIC TRUE  CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_SHARED FALSE CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_TEST   FALSE CACHE BOOL "" FORCE)
set(PHYSFS_BUILD_DOCS   FALSE CACHE BOOL "" FORCE)
set(PHYSFS_TARGETNAME_UNINSTALL "physfs_uninstall" CACHE STRING "" FORCE)
add_subdirectory(third_party/physfs)

# Wren (manual build - same as before, just different source path)
file(GLOB WREN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wren/src/vm/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wren/src/optional/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WrenExtensions/Modules/Reflect/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WrenExtensions/Modules/Trace/*.c
)
add_library(wren STATIC ${WREN_SOURCES})
target_compile_definitions(wren PUBLIC WREN_OPT_META=1 WREN_OPT_RANDOM=1 WREN_OPT_REFLECT=1)
target_include_directories(wren PUBLIC
    third_party/wren/src/include
    third_party/wren/src/optional
    ${CMAKE_CURRENT_SOURCE_DIR}/WrenExtensions/Modules
)
target_include_directories(wren PRIVATE
    third_party/wren/src/vm
    third_party/wren/src/optional
)
if(WIN32)
    target_compile_definitions(wren PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Editor-only libs
if(EDITOR)
    # ImGui
    set(IMGUI_SOURCES
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_demo.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
    )
    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC third_party/imgui)
    target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

    if(PLATFORM_DESKTOP OR PLATFORM_WEB)
        # Input/platform plumbing comes from ImGui's own official SDL3 backend (rendering is our own
        # hand-rolled bgfx backend, see Engine/Renderer/ImGuiRenderer - the "ForOther" variant here doesn't
        # need a paired renderer backend).
        target_sources(imgui PRIVATE third_party/imgui/backends/imgui_impl_sdl3.cpp)
        target_include_directories(imgui PUBLIC third_party/imgui/backends)
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    endif()

    # tinyfiledialogs
    add_library(tinyfiledialogs STATIC third_party/tinyfiledialogs/tinyfiledialogs.c)
    target_include_directories(tinyfiledialogs PUBLIC third_party/tinyfiledialogs)
endif()

if(PLATFORM_DESKTOP OR PLATFORM_WEB)
    # bgfx/SDL3-backed renderer, shared by both platforms now (see the bgfx block above).
    set(RENDERER_SOURCES
        src/Engine/Platform/Window.h                src/Engine/Platform/Window.cpp
        src/Engine/Renderer/GraphicsDevice.h        src/Engine/Renderer/GraphicsDevice.cpp
        src/Engine/Renderer/EmbeddedShaders.h        src/Engine/Renderer/EmbeddedShaders.cpp
        src/Engine/Renderer/QuadVertex.h
        src/Engine/Renderer/TileChunk.h
        src/Engine/Renderer/TileChunkBuilder.h      src/Engine/Renderer/TileChunkBuilder.cpp
        src/Engine/Renderer/UIRenderer.h            src/Engine/Renderer/UIRenderer.cpp
        ${STRUKTUR_SHADER_OUTPUTS}
    )
    source_group("Shaders/Compiled" FILES ${STRUKTUR_SHADER_OUTPUTS})
endif()

if(DEBUG)
    set(DEBUG_SOURCES
        src/Debug/Box2DBgfxDebugDraw.h              src/Debug/Box2DBgfxDebugDraw.cpp

        src/Engine/Renderer/DebugRenderer.h         src/Engine/Renderer/DebugRenderer.cpp

        src/Engine/Scripting/WrenCodeGenerator.h    src/Engine/Scripting/WrenCodeGenerator.cpp
    )
endif()

if(EDITOR)
    # Debug: Use source assets directly
    set(EDITOR_SOURCES
        src/Engine/ECS/System/DebugSystem.h         src/Engine/ECS/System/DebugSystem.cpp

        src/Debug/Editor/Editor.h                   src/Debug/Editor/Editor.cpp
        src/Debug/Editor/EditorLayoutManager.h      src/Debug/Editor/EditorLayoutManager.cpp
        src/Debug/Editor/EditorSettings.h           src/Debug/Editor/EditorSettings.cpp
        src/Debug/Editor/EditorTheme.h              src/Debug/Editor/EditorTheme.cpp
        src/Debug/Editor/Exporters/DialogueExporter.h src/Debug/Editor/Exporters/DialogueExporter.cpp

        src/Debug/Editor/EditorSettings.h           src/Debug/Editor/EditorSettings.cpp
        
        src/Engine/Renderer/ImGuiRenderer.h         src/Engine/Renderer/ImGuiRenderer.cpp

        src/Debug/Editor/Windows/EditorWindow.h     src/Debug/Editor/Windows/EditorWindow.cpp
        src/Debug/Editor/Windows/DebugInfoWindow.h  src/Debug/Editor/Windows/DebugInfoWindow.cpp
        src/Debug/Editor/Windows/GameViewPortWindow.h src/Debug/Editor/Windows/GameViewPortWindow.cpp
        src/Debug/Editor/Windows/HierarchyWindow.h  src/Debug/Editor/Windows/HierarchyWindow.cpp
        src/Debug/Editor/Windows/UIHierarchyWindow.h src/Debug/Editor/Windows/UIHierarchyWindow.cpp
        src/Debug/Editor/Windows/ToolbarWindow.h    src/Debug/Editor/Windows/ToolbarWindow.cpp
        src/Debug/Editor/Windows/InspectorWindow.h  src/Debug/Editor/Windows/InspectorWindow.cpp
        src/Debug/Editor/Windows/SettingsWindow.h   src/Debug/Editor/Windows/SettingsWindow.cpp
        src/Debug/Editor/Windows/PreviewWindow.h    src/Debug/Editor/Windows/PreviewWindow.cpp
        src/Debug/Editor/Windows/FileExplorerWindow.h src/Debug/Editor/Windows/FileExplorerWindow.cpp
        src/Debug/Editor/Windows/LogWindow.h        src/Debug/Editor/Windows/LogWindow.cpp
        src/Debug/Editor/Windows/ProfilerWindow.h   src/Debug/Editor/Windows/ProfilerWindow.cpp
        src/Debug/Editor/Windows/DialogueEditorWindow.h src/Debug/Editor/Windows/DialogueEditorWindow.cpp

        src/Debug/Editor/PreviewRenderers/PreviewRenderer.h
        src/Debug/Editor/PreviewRenderers/TexturePreviewRenderer.h src/Debug/Editor/PreviewRenderers/TexturePreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/ShaderPreviewRenderer.h  src/Debug/Editor/PreviewRenderers/ShaderPreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/SpritePreviewRenderer.h  src/Debug/Editor/PreviewRenderers/SpritePreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/TextFilePreviewRenderer.h src/Debug/Editor/PreviewRenderers/TextFilePreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/UnknownFilePreviewRenderer.h src/Debug/Editor/PreviewRenderers/UnknownFilePreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/PreviewHelpers.h
    )
endif()

# Wren Scripting: Use source assets directly
set(WREN_BINDING_SOURCES 
    src/WrenBindings/ComponentListXMacro.h
    src/WrenBindings/WrenAnimation.h            src/WrenBindings/WrenAnimation.cpp
    src/WrenBindings/WrenGameObjectComponents.h src/WrenBindings/WrenGameObjectComponents.cpp
    src/WrenBindings/WrenMath.h                 src/WrenBindings/WrenMath.cpp
    src/WrenBindings/WrenPhysics.h              src/WrenBindings/WrenPhysics.cpp
                                                src/WrenBindings/WrenCollisionLayers.cpp
    src/WrenBindings/WrenResourceManager.h      src/WrenBindings/WrenResourceManager.cpp
    src/WrenBindings/WrenUI.h                   src/WrenBindings/WrenUI.cpp
    src/WrenBindings/WrenDialogue.h             src/WrenBindings/WrenDialogue.cpp
    src/WrenBindings/WrenFunctionCallback.h     src/WrenBindings/WrenFunctionCallback.cpp
    src/WrenBindings/WrenFileSystem.h           src/WrenBindings/WrenFileSystem.cpp
                                                src/WrenBindings/WrenFlags.cpp
                                                src/WrenBindings/WrenEvents.cpp
                                                src/WrenBindings/WrenApplication.cpp
                                                src/WrenBindings/WrenGameObject.cpp
                                                src/WrenBindings/WrenInput.cpp
                                                src/WrenBindings/WrenDebug.cpp
                                                src/WrenBindings/WrenSerialisation.cpp
                                                src/WrenBindings/WrenVariableSubstitutionSystem.cpp
)

# Source files
set(SOURCES
    src/Main.cpp

    src/Engine/Game.h                           src/Engine/Game.cpp
    src/Engine/GameContext.h                    src/Engine/GameContext.cpp

    src/Engine/Core/Gamedata.h                  src/Engine/Core/GameData.cpp
    src/Engine/Core/TimeSystem.h                src/Engine/Core/TimeSystem.cpp
    src/Engine/Core/FileSystem.h                src/Engine/Core/FileSystem.cpp

    src/Engine/Input/Input.h                    src/Engine/Input/Input.cpp
    src/Engine/Input/InputConfigLoader.h        src/Engine/Input/InputConfigLoader.cpp
    src/Engine/Input/InputMaps.h                src/Engine/Input/InputMaps.cpp

    src/Engine/Audio/Mixer.h                    src/Engine/Audio/Mixer.cpp

    src/Engine/Flag/FlagManager.h               src/Engine/Flag/FlagManager.cpp

    src/Engine/Callback/Callback.h 
    src/Engine/Callback/Variant.h
    src/Engine/Callback/CallbackHelperFunctions.h src/Engine/Callback/CallbackHelperFunctions.cpp
    src/Engine/Callback/FunctionCallback.h      src/Engine/Callback/FunctionCallback.cpp
    src/Engine/Callback/WrenCallback.h          src/Engine/Callback/WrenCallback.cpp

    src/Engine/Dialogue/DialogueManager.h       src/Engine/Dialogue/DialogueManager.cpp
    src/Engine/Dialogue/DialogueNode.h          src/Engine/Dialogue/DialogueNode.cpp
    src/Engine/Dialogue/DialogueRegistry.h      src/Engine/Dialogue/DialogueRegistry.cpp
    src/Engine/Dialogue/DialogueValue.h         src/Engine/Dialogue/DialogueValue.cpp
    src/Engine/Dialogue/Command.h               src/Engine/Dialogue/Command.cpp
    src/Engine/Dialogue/Condition.h             src/Engine/Dialogue/Condition.cpp
    src/Engine/Dialogue/DialogueHelperFunctions.h src/Engine/Dialogue/DialogueHelperFunctions.cpp
    src/Engine/Dialogue/TextSystems/VariableSubstitutionSystem.h src/Engine/Dialogue/TextSystems/VariableSubstitutionSystem.cpp
    src/Engine/Dialogue/DialogueResult.h

    src/Engine/Resource/Resource.h
    src/Engine/Resource/Pointers/ResourcePtr.h
    src/Engine/Resource/Pools/SparseSet.h
    src/Engine/Resource/Pools/ResourcePool.h
    src/Engine/Resource/Pools/GpuResourcePool.h
    src/Engine/Resource/Pools/TexturePool.h     src/Engine/Resource/Pools/TexturePool.cpp
    src/Engine/Resource/Pools/SoundPool.h       src/Engine/Resource/Pools/SoundPool.cpp
    src/Engine/Resource/Pools/MusicPool.h       src/Engine/Resource/Pools/MusicPool.cpp
    src/Engine/Resource/Pools/FontPool.h        src/Engine/Resource/Pools/FontPool.cpp
    src/Engine/Resource/Pools/ShaderPool.h      src/Engine/Resource/Pools/ShaderPool.cpp
    src/Engine/Resource/ResourceManager.h       src/Engine/Resource/ResourceManager.cpp
    src/Engine/Resource/SoundResource.h         src/Engine/Resource/SoundResource.cpp
    src/Engine/Resource/FontResource.h          src/Engine/Resource/FontResource.cpp
    src/Engine/Resource/MusicResource.h         src/Engine/Resource/MusicResource.cpp
    src/Engine/Resource/TextureResource.h       src/Engine/Resource/TextureResource.cpp
    src/Engine/Resource/ShaderResource.h        src/Engine/Resource/ShaderResource.cpp
    
    src/Engine/ECS/SystemManager.h              src/Engine/ECS/SystemManager.cpp
    src/Engine/ECS/GameObjectManager.h          src/Engine/ECS/GameObjectManager.cpp
    
    src/Engine/ECS/Component/active.h
    src/Engine/ECS/Component/PhysicsBody.h
    src/Engine/ECS/Component/Sprite.h
    src/Engine/ECS/Component/SpriteAnimation.h
    src/Engine/ECS/Component/Transform.h
    src/Engine/ECS/Component/TileMap.h
    src/Engine/ECS/Component/Identifier.h
    src/Engine/ECS/Component/Camera.h
    src/Engine/ECS/Component/Shader.h
    src/Engine/ECS/Component/WrenScript.h

    src/Engine/ECS/System/HierarchySystem.h     src/Engine/ECS/System/HierarchySystem.cpp
    src/Engine/ECS/System/PhysicsSystem.h       src/Engine/ECS/System/PhysicsSystem.cpp
    src/Engine/ECS/System/SpriteRenderSystem.h  src/Engine/ECS/System/SpriteRenderSystem.cpp
    src/Engine/ECS/System/AnimationSystem.h     src/Engine/ECS/System/AnimationSystem.cpp
    src/Engine/ECS/System/TransformSystem.h     src/Engine/ECS/System/TransformSystem.cpp
    src/Engine/ECS/System/CameraSystem.h        src/Engine/ECS/System/CameraSystem.cpp
    src/Engine/ECS/System/UISystem.h            src/Engine/ECS/System/UISystem.cpp
    src/Engine/ECS/System/UIRenderSystem.h      src/Engine/ECS/System/UIRenderSystem.cpp
    src/Engine/ECS/System/ShaderSystem.h        src/Engine/ECS/System/ShaderSystem.cpp
    src/Engine/ECS/System/WrenScriptSystem.h    src/Engine/ECS/System/WrenScriptSystem.cpp
    src/Engine/ECS/System/WrenStateSystem.h     src/Engine/ECS/System/WrenStateSystem.cpp
    src/Engine/ECS/System/SoundSystem.h         src/Engine/ECS/System/SoundSystem.cpp
    src/Engine/ECS/System/EventSystem.h         src/Engine/ECS/System/EventSystem.cpp

    src/Engine/Physics/PhysicsWorld.h           src/Engine/Physics/PhysicsWorld.cpp
    src/Engine/Physics/ContactListener.h        src/Engine/Physics/ContactListener.cpp
    src/Engine/Physics/CollisionLayers.h        src/Engine/Physics/CollisionLayers.cpp
    src/Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.h src/Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.cpp

    src/Engine/FileLoading/LevelParser.h        src/Engine/FileLoading/LevelParser.cpp

    src/Engine/UI/UIManager.h                   src/Engine/UI/UIManager.cpp
    src/Engine/UI/FocusNavigator.h              src/Engine/UI/FocusNavigator.cpp
    src/Engine/UI/UIElement.h                   src/Engine/UI/UIElement.cpp
    src/Engine/UI/TextLayout.h                  src/Engine/UI/TextLayout.cpp
    src/Engine/UI/UILabel.h                     src/Engine/UI/UILabel.cpp
    src/Engine/UI/UIRichLabel.h                 src/Engine/UI/UIRichLabel.cpp
    src/Engine/UI/UIPanel.h                     src/Engine/UI/UIPanel.cpp
    src/Engine/UI/UIColor.h                     src/Engine/UI/UIColor.cpp
    src/Engine/UI/UITexture.h                   src/Engine/UI/UITexture.cpp
    src/Engine/UI/UIBorder.h                    src/Engine/UI/UIBorder.cpp

    src/Engine/World/Level.h                    src/Engine/World/Level.cpp
    src/Engine/World/TileMap.h                  src/Engine/World/TileMap.cpp
    src/Engine/World/Camera.h                   src/Engine/World/Camera.cpp
    src/Engine/World/RenderLayer.h

    src/Engine/Renderer/WorldRenderer.h         src/Engine/Renderer/WorldRenderer.cpp

    src/Engine/Scripting/WrenScriptEngine.h     src/Engine/Scripting/WrenScriptEngine.cpp
    src/Engine/Scripting/WrenBindingRegistry.h  src/Engine/Scripting/WrenBindingRegistry.cpp
    src/Engine/Scripting/WrenStateManager.h     src/Engine/Scripting/WrenStateManager.cpp
    src/Engine/Scripting/WrenScriptComponentRegistry.h src/Engine/Scripting/WrenScriptComponentRegistry.cpp
    src/Engine/Scripting/WrenUtil.h             src/Engine/Scripting/WrenUtil.cpp
    src/Engine/Scripting/WrenValueWrapper.h

    src/Engine/Util/Noise.h                     src/Engine/Util/Noise.cpp
    src/Engine/Util/MathUtil.h
    src/Engine/Util/Color.h
    src/Engine/Util/Curve.h

    src/Engine/Text/Font.h                      src/Engine/Text/Font.cpp
    src/Engine/Text/RichText.h                  src/Engine/Text/RichText.cpp

    src/Engine/Event/Event.h
    src/Engine/Event/EventManager.h             src/Engine/Event/EventManager.cpp

    src/Debug/Assertions.h
    src/Debug/Logger.h                          src/Debug/Logger.cpp
    src/Debug/Profiling/Profiler.h              src/Debug/Profiling/Profiler.cpp

    ${WREN_BINDING_SOURCES}
    ${DEBUG_SOURCES}
    ${EDITOR_SOURCES}
    ${RENDERER_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(PLATFORM_DESKTOP OR PLATFORM_WEB)
    target_include_directories(${PROJECT_NAME} PRIVATE ${STB_IMAGE_INCLUDE_DIR} ${STB_TRUETYPE_INCLUDE_DIR} ${STRUKTUR_SHADER_OUTPUT_DIR})
endif()

# Link Libraries
target_link_libraries(${PROJECT_NAME}
    EnTT::EnTT
    glm::glm
    wren
    nlohmann_json::nlohmann_json
    box2d
    PhysFS::PhysFS-static
    $<$<OR:$<BOOL:${PLATFORM_DESKTOP}>,$<BOOL:${PLATFORM_WEB}>>:SDL3_mixer::SDL3_mixer>
    $<$<OR:$<BOOL:${PLATFORM_DESKTOP}>,$<BOOL:${PLATFORM_WEB}>>:SDL3::SDL3>
    $<$<OR:$<BOOL:${PLATFORM_DESKTOP}>,$<BOOL:${PLATFORM_WEB}>>:bgfx>
    $<$<OR:$<BOOL:${PLATFORM_DESKTOP}>,$<BOOL:${PLATFORM_WEB}>>:bimg>
    $<$<OR:$<BOOL:${PLATFORM_DESKTOP}>,$<BOOL:${PLATFORM_WEB}>>:bx>
    $<$<BOOL:${EDITOR}>:imgui>
    $<$<BOOL:${EDITOR}>:tinyfiledialogs>
)

# Platform-specific settings
if(PLATFORM_WEB)
    if(USE_ASYNCIFY)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1")
        message(STATUS "Building web version WITH ASYNCIFY")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ASSERTIONS=1 -s WASM=1 -s GL_ENABLE_GET_PROC_ADDRESS=1 -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1")
        message(STATUS "Building web version WITHOUT ASYNCIFY")
    endif()
    
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=['HEAPF32','HEAPU8','HEAPU16','HEAPU32','HEAP32','HEAP8','FS']")
    # Export C++ functions for JavaScript
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS='[\"_main\",\"_RequestFullscreenFromC\",\"_ExitFullscreenFromC\",\"_IsFullscreenSupported\"]'")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]'")
    
    # Assets folder
    if(PROJECT_ASSETS_FOLDER)
        set(ASSETS_DIR "${PROJECT_ASSETS_FOLDER}")
    else()
        set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
    endif()

    # Preload assets fozr web builds
    if(IS_DIRECTORY ${ASSETS_DIR})
        file(GLOB_RECURSE ASSET_FILES "${ASSETS_DIR}/*")
        if(ASSET_FILES)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${ASSETS_DIR}@/assets\"")
            message(STATUS "Preloading assets from: ${ASSETS_DIR}")
        endif()
    endif()

    # Include favicon if it exists
    if(EXISTS ${CMAKE_SOURCE_DIR}/favicon.ico)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_SOURCE_DIR}/favicon.ico@/favicon.ico\"")
        message(STATUS "Including favicon.ico")
    endif()


    # Shell file
    if(DEFINED WEB_SHELL_FILE)
        set(SHELL_FILE "${WEB_SHELL_FILE}")
    else()
        set(SHELL_FILE "${CMAKE_SOURCE_DIR}/web/shell.html")
    endif()

    if(EXISTS "${SHELL_FILE}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file \"${SHELL_FILE}\"")
        message(STATUS "Using shell: ${SHELL_FILE}")
    else()
        message(STATUS "No shell file found, using Emscripten default")
    endif()

    # Include web assets (favicons, etc.)
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/web/assets)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_SOURCE_DIR}/web/assets@/\"")
        message(STATUS "Including web assets")
    endif()
    
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
elseif(PLATFORM_DESKTOP)
    # Desktop settings
    if(WIN32)
		if(CMAKE_BUILD_TYPE STREQUAL "Release")
			set_target_properties(${PROJECT_NAME} PROPERTIES
				WIN32_EXECUTABLE True
			)
			set(WINDOWS_EXECUTABLE TRUE)
			add_compile_definitions(WINDOWS_EXECUTABLE)
		else()
			set_target_properties(${PROJECT_NAME} PROPERTIES
				WIN32_EXECUTABLE False
			)
		endif()
    endif()

endif()

install(TARGETS ${PROJECT_NAME} DESTINATION .)