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
set(RAYLIB_INSTALL OFF CACHE BOOL "" FORCE)

##########################################################################################
# Configurable options
##########################################################################################

# toggle libs to be used with ON and OFF

option(USE_ASYNCIFY "Enable ASYNCIFY for traditional game loops" OFF)

# Platform detection
if(EMSCRIPTEN)
    set(PLATFORM_WEB TRUE)
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

    message(STATUS "Building for Web (Emscripten)")
else()
    set(PLATFORM_DESKTOP TRUE)
    message(STATUS "Building for Desktop")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG TRUE)
    add_compile_definitions(DEBUG)
    set(EDITOR TRUE)
    add_compile_definitions(EDITOR)
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

# RayLib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)

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

# Wren (manual build — same as before, just different source path)
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

    # rlImGui
    add_library(rlimgui STATIC third_party/rlimgui/rlImGui.cpp)
    target_include_directories(rlimgui PUBLIC third_party/rlimgui third_party/imgui)
    target_link_libraries(rlimgui PUBLIC raylib imgui)

    # tinyfiledialogs
    add_library(tinyfiledialogs STATIC third_party/tinyfiledialogs/tinyfiledialogs.c)
    target_include_directories(tinyfiledialogs PUBLIC third_party/tinyfiledialogs)
endif()

if(DEBUG)
    set(DEBUG_SOURCES 
        src/Debug/Box2DDebugRenderer.h

        src/Engine/ECS/System/DebugSystem.h         src/Engine/ECS/System/DebugSystem.cpp

        src/Engine/Scripting/WrenCodeGenerator.h    src/Engine/Scripting/WrenCodeGenerator.cpp
    )
endif()

if(EDITOR)
    # Debug: Use source assets directly
    set(EDITOR_SOURCES 
        src/Debug/Editor/Editor.h                   src/Debug/Editor/Editor.cpp
        src/Debug/Editor/EditorLayoutManager.h      src/Debug/Editor/EditorLayoutManager.cpp
        src/Debug/Editor/EditorSettings.h           src/Debug/Editor/EditorSettings.cpp
        src/Debug/Editor/Exporters/DialogueExporter.h src/Debug/Editor/Exporters/DialogueExporter.cpp

        src/Debug/Editor/EditorSettings.h           src/Debug/Editor/EditorSettings.cpp
        
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
                                                    src/Debug/Editor/Windows/DialogueEditorWindow_GraphView.cpp
                                                    src/Debug/Editor/Windows/DialogueEditorWindow_Operations.cpp
                                                    src/Debug/Editor/Windows/DialogueEditorWindow_NodeEditor.cpp

        src/Debug/Editor/PreviewRenderers/PreviewRenderer.h
        src/Debug/Editor/PreviewRenderers/TexturePreviewRenderer.h src/Debug/Editor/PreviewRenderers/TexturePreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/ShaderPreviewRenderer.h  src/Debug/Editor/PreviewRenderers/ShaderPreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/SpritePreviewRenderer.h  src/Debug/Editor/PreviewRenderers/SpritePreviewRenderer.cpp
        src/Debug/Editor/PreviewRenderers/TextFilePreviewRenderer.h
        src/Debug/Editor/PreviewRenderers/UnknownFilePreviewRenderer.h
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
    src/WrenBindings/WrenResourceManager.h      src/WrenBindings/WrenResourceManager.cpp
    src/WrenBindings/WrenUI.h                   src/WrenBindings/WrenUI.cpp
    src/WrenBindings/WrenDialogue.h             src/WrenBindings/WrenDialogue.cpp
    src/WrenBindings/WrenFunctionCallback.h     src/WrenBindings/WrenFunctionCallback.cpp
    src/WrenBindings/WrenFileSystem.h           src/WrenBindings/WrenFileSystem.cpp
                                                src/WrenBindings/WrenFlags.cpp
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

    src/Engine/Resource/ResourcePool.h
    src/Engine/Resource/Resource.h
    src/Engine/Resource/ResourcePtr.h
    src/Engine/Resource/ResourceManager.h       src/Engine/Resource/ResourceManager.cpp
    src/Engine/Resource/SoundResource.h         src/Engine/Resource/SoundResource.cpp
    src/Engine/Resource/FontResource.h          src/Engine/Resource/FontResource.cpp
    src/Engine/Resource/MusicResource.h         src/Engine/Resource/MusicResource.cpp
    src/Engine/Resource/TextureResource.h       src/Engine/Resource/TextureResource.cpp
    src/Engine/Resource/ShaderResource.h        src/Engine/Resource/ShaderResource.cpp
    
    src/Engine/ECS/SystemManager.h              src/Engine/ECS/SystemManager.cpp
    src/Engine/ECS/GameObjectManager.h          src/Engine/ECS/GameObjectManager.cpp
    
    src/Engine/ECS/Component/PhysicsBody.h
    src/Engine/ECS/Component/Player.h
    src/Engine/ECS/Component/Sprite.h
    src/Engine/ECS/Component/SpriteAnimation.h
    src/Engine/ECS/Component/Transform.h
    src/Engine/ECS/Component/TileMap.h
    src/Engine/ECS/Component/Identifier.h
    src/Engine/ECS/Component/Camera.h
    src/Engine/ECS/Component/Shader.h
    src/Engine/ECS/Component/WrenScript.h

    src/Engine/ECS/System/HierrarchySystem.h    src/Engine/ECS/System/HierrarchySystem.cpp
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

    src/Engine/Physics/PhysicsWorld.h           src/Engine/Physics/PhysicsWorld.cpp
    src/Engine/Physics/ContactListener.h        src/Engine/Physics/ContactListener.cpp
    src/Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.h src/Engine/Physics/CollisionShapeGenerators/TileMapCollisionBodyGenerator.cpp

    src/Engine/FileLoading/LevelParser.h        src/Engine/FileLoading/LevelParser.cpp

    src/Engine/UI/UIManager.h                   src/Engine/UI/UIManager.cpp
    src/Engine/UI/FocusNavigator.h              src/Engine/UI/FocusNavigator.cpp
    src/Engine/UI/UIElement.h                   src/Engine/UI/UIElement.cpp
    src/Engine/UI/UILabel.h                     src/Engine/UI/UILabel.cpp
    src/Engine/UI/UIPanel.h                     src/Engine/UI/UIPanel.cpp

    src/Engine/Game/Level.h                     src/Engine/Game/Level.cpp
    src/Engine/Game/TileMap.h                   src/Engine/Game/TileMap.cpp
    src/Engine/Game/Camera.h                    src/Engine/Game/Camera.cpp
    src/Engine/Game/Curve.h

    src/Engine/Scripting/WrenScriptEngine.h     src/Engine/Scripting/WrenScriptEngine.cpp
    src/Engine/Scripting/WrenBindingRegistry.h  src/Engine/Scripting/WrenBindingRegistry.cpp
    src/Engine/Scripting/WrenStateManager.h     src/Engine/Scripting/WrenStateManager.cpp
    src/Engine/Scripting/WrenScriptComponentRegistry.h src/Engine/Scripting/WrenScriptComponentRegistry.cpp
    src/Engine/Scripting/WrenUtil.h             src/Engine/Scripting/WrenUtil.cpp
    src/Engine/Scripting/WrenValueWrapper.h

    src/Engine/Util/Noise.h                     src/Engine/Util/Noise.cpp

    src/Debug/Assertions.h
    src/Debug/Profiling/Profiler.h              src/Debug/Profiling/Profiler.cpp

    ${WREN_BINDING_SOURCES}
    ${DEBUG_SOURCES}
    ${EDITOR_SOURCES}
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link Libraries
target_link_libraries(${PROJECT_NAME} 
    raylib
    EnTT::EnTT
    glm::glm
    wren
    nlohmann_json::nlohmann_json
    box2d
    PhysFS::PhysFS-static
    $<$<BOOL:${EDITOR}>:imgui>
    $<$<BOOL:${EDITOR}>:rlimgui>
    $<$<BOOL:${EDITOR}>:tinyfiledialogs>
)

# Platform-specific settings
if(PLATFORM_WEB)
    if(USE_ASYNCIFY)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1")
        message(STATUS "Building web version WITH ASYNCIFY")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s GL_ENABLE_GET_PROC_ADDRESS=1 -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1")
        message(STATUS "Building web version WITHOUT ASYNCIFY")
    endif()
    
    # Export C++ functions for JavaScript
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS='[\"_main\",\"_RequestFullscreenFromC\",\"_ExitFullscreenFromC\",\"_IsFullscreenSupported\"]'")
    #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]'")
    
    # Preload assets for web builds
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/assets)
        file(GLOB_RECURSE ASSET_FILES "${CMAKE_SOURCE_DIR}/assets/*")
        if(ASSET_FILES)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_SOURCE_DIR}/assets@/assets\"")
            message(STATUS "Preloading assets directory for web build")
        else()
            message(STATUS "Assets directory exists but is empty")
        endif()
    else()
        message(STATUS "No assets directory found - creating example structure")
    endif()
    
    # Include favicon if it exists
    if(EXISTS ${CMAKE_SOURCE_DIR}/favicon.ico)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_SOURCE_DIR}/favicon.ico@/favicon.ico\"")
        message(STATUS "Including favicon.ico")
    endif()
    
    # Include web assets (favicons, etc.)
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/web/assets)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_SOURCE_DIR}/web/assets@/\"")
        message(STATUS "Including web assets")
    endif()
    
    # Use custom shell if it exists
    if(EXISTS ${CMAKE_SOURCE_DIR}/web/shell.html)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file \"${CMAKE_SOURCE_DIR}/web/shell.html\"")
        message(STATUS "Using custom shell.html")
    else()
        message(STATUS "Using default Emscripten shell")
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