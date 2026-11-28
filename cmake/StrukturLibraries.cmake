# The engine's libraries, and the functions that put them into a game.
#
# core is always there. The others are listed by a game; a library's requirements are added for it. Each listed library
# defines STRUKTUR_<NAME> for every file of that game, so a class has the same layout everywhere, and a library's use of
# an optional one sits inside that define - see docs/ModuleSplitPlan.md.

set(STRUKTUR_ALL_LIBRARIES ecs wren ui physics dialogue flag fitness)
# Libraries one of these can't build without (besides core).
set(STRUKTUR_REQUIRES_ui ecs)
set(STRUKTUR_REQUIRES_physics ecs)

# struktur_add_libraries(<prefix> [LIBRARIES <name>...] [EDITOR] [ASSETS <folder>] [COMPONENTS <header>])
#
# Creates a static library per engine library, named <prefix>_<name>, compiled for that set: <prefix>_core, the listed
# ones, <prefix>_editor with EDITOR, and <prefix>_app (the startup and frame every game shares) when ecs, wren and ui
# are all in. Sets <prefix>_TARGETS in the caller. ASSETS is the folder the editor opens. COMPONENTS names a game
# header that defines GAME_COMPONENT_LIST, for components scripts can reach.
function(struktur_add_libraries prefix)
    cmake_parse_arguments(ARG "EDITOR" "ASSETS;COMPONENTS" "LIBRARIES" ${ARGN})

    set(libraries ${ARG_LIBRARIES})
    foreach(library IN LISTS ARG_LIBRARIES)
        if(NOT library IN_LIST STRUKTUR_ALL_LIBRARIES)
            message(FATAL_ERROR "Unknown engine library '${library}' (known: ${STRUKTUR_ALL_LIBRARIES})")
        endif()
        list(APPEND libraries ${STRUKTUR_REQUIRES_${library}})
    endforeach()
    if(ARG_EDITOR)
        list(APPEND libraries ecs)
        if(NOT TARGET imgui)
            message(FATAL_ERROR "The editor needs a Debug build with EDITOR on")
        endif()
    endif()
    list(REMOVE_DUPLICATES libraries)

    # What every file of this set is compiled with.
    set(settings ${prefix}_settings)
    add_library(${settings} INTERFACE)
    foreach(library IN LISTS libraries)
        string(TOUPPER ${library} define)
        target_compile_definitions(${settings} INTERFACE STRUKTUR_${define})
    endforeach()
    if(ARG_EDITOR)
        if(NOT ARG_ASSETS)
            set(ARG_ASSETS "assets")
        endif()
        target_compile_definitions(${settings} INTERFACE EDITOR "STRUKTUR_ASSETS_DIR=\"${ARG_ASSETS}\"")
    endif()
    if(ARG_COMPONENTS)
        target_compile_definitions(${settings} INTERFACE "STRUKTUR_GAME_COMPONENT_HEADER=\"${ARG_COMPONENTS}\"")
    endif()
    target_include_directories(${settings} INTERFACE
        ${STRUKTUR_ENGINE_DIR}/src ${STB_IMAGE_INCLUDE_DIR} ${STB_TRUETYPE_INCLUDE_DIR})

    set(targets)

    # A library's sources, given relative to the engine's src folder, and what it links.
    macro(struktur_library name)
        cmake_parse_arguments(LIBRARY "" "" "SOURCES;LINK" ${ARGN})
        set(paths)
        foreach(source IN LISTS LIBRARY_SOURCES)
            list(APPEND paths ${STRUKTUR_ENGINE_DIR}/src/${source})
        endforeach()
        add_library(${prefix}_${name} STATIC ${paths})
        target_link_libraries(${prefix}_${name} PUBLIC ${settings} ${LIBRARY_LINK})
        set_target_properties(${prefix}_${name} PROPERTIES FOLDER "${prefix}")
        list(APPEND targets ${prefix}_${name})
    endmacro()

    # The other libraries of this set that a library also links: its optional ones.
    macro(struktur_optional result)
        set(${result})
        foreach(candidate IN ITEMS ${ARGN})
            if(candidate IN_LIST libraries OR (candidate STREQUAL "editor" AND ARG_EDITOR))
                list(APPEND ${result} ${prefix}_${candidate})
            endif()
        endforeach()
    endmacro()

    set(has_ecs OFF)
    set(has_wren OFF)
    set(has_ui OFF)
    set(has_physics OFF)
    set(has_dialogue OFF)
    foreach(library IN LISTS libraries)
        set(has_${library} ON)
    endforeach()

    # --- core -------------------------------------------------------------------------------------------------------
    set(sources
        Engine/GameContext.h Engine/GameContext.cpp
        Engine/Core/GameData.h Engine/Core/GameData.cpp
        Engine/Core/TimeSystem.h Engine/Core/TimeSystem.cpp
        Engine/Core/FileSystem.h Engine/Core/FileSystem.cpp
        Engine/Core/EngineSettings.h Engine/Core/EngineSettings.cpp
        Engine/Core/CommandLine.h Engine/Core/CommandLine.cpp
        Engine/Platform/Window.h Engine/Platform/Window.cpp
        Engine/Renderer/GraphicsDevice.h Engine/Renderer/GraphicsDevice.cpp
        Engine/Renderer/EmbeddedShaders.h Engine/Renderer/EmbeddedShaders.cpp
        Engine/Renderer/QuadVertex.h
        Engine/Input/Input.h Engine/Input/Input.cpp
        Engine/Input/InputConfigLoader.h Engine/Input/InputConfigLoader.cpp
        Engine/Input/InputMaps.h Engine/Input/InputMaps.cpp
        Engine/Audio/Mixer.h Engine/Audio/Mixer.cpp
        Engine/Localization/LocalizationManager.h Engine/Localization/LocalizationManager.cpp
        Engine/Callback/Callback.h Engine/Callback/Variant.h
        Engine/Callback/FunctionCallback.h Engine/Callback/FunctionCallback.cpp
        Engine/Resource/Resource.h Engine/Resource/Pointers/ResourcePtr.h
        Engine/Resource/Pools/SparseSet.h Engine/Resource/Pools/ResourcePool.h Engine/Resource/Pools/GpuResourcePool.h
        Engine/Resource/Pools/TexturePool.h Engine/Resource/Pools/TexturePool.cpp
        Engine/Resource/Pools/SoundPool.h Engine/Resource/Pools/SoundPool.cpp
        Engine/Resource/Pools/MusicPool.h Engine/Resource/Pools/MusicPool.cpp
        Engine/Resource/Pools/FontPool.h Engine/Resource/Pools/FontPool.cpp
        Engine/Resource/Pools/ShaderPool.h Engine/Resource/Pools/ShaderPool.cpp
        Engine/Resource/ResourceManager.h Engine/Resource/ResourceManager.cpp
        Engine/Resource/SoundResource.h Engine/Resource/SoundResource.cpp
        Engine/Resource/FontResource.h Engine/Resource/FontResource.cpp
        Engine/Resource/MusicResource.h Engine/Resource/MusicResource.cpp
        Engine/Resource/TextureResource.h Engine/Resource/TextureResource.cpp
        Engine/Resource/ShaderResource.h Engine/Resource/ShaderResource.cpp
        Engine/Util/Noise.h Engine/Util/Noise.cpp
        Engine/Util/MathUtil.h Engine/Util/Color.h Engine/Util/Curve.h
        Engine/Text/Font.h Engine/Text/Font.cpp
        Engine/Text/RichText.h Engine/Text/RichText.cpp
        Engine/Event/Event.h Engine/Event/EventManager.h Engine/Event/EventManager.cpp
        Debug/Assertions.h
        Debug/Logger.h Debug/Logger.cpp
        Debug/Profiling/Profiler.h Debug/Profiling/Profiler.cpp)
    struktur_library(core SOURCES ${sources}
        LINK StrukturJobs SDL3_mixer::SDL3_mixer SDL3::SDL3 bgfx bimg bx PhysFS::PhysFS-static EnTT::EnTT glm::glm
             nlohmann_json::nlohmann_json)
    struktur_use_shaders(${prefix}_core CoreShaders)

    # --- ecs --------------------------------------------------------------------------------------------------------
    if(has_ecs)
        set(sources
            Engine/ECS/EcsServices.h Engine/ECS/EcsServices.cpp
            Engine/ECS/SystemManager.h Engine/ECS/SystemManager.cpp
            Engine/ECS/GameObjectManager.h Engine/ECS/GameObjectManager.cpp
            Engine/ECS/CommandList.h Engine/ECS/SystemAccess.h Engine/ECS/SystemSchedule.h
            Engine/ECS/Component/Active.h Engine/ECS/Component/Transform.h Engine/ECS/Component/Identifier.h
            Engine/ECS/System/HierarchySystem.h Engine/ECS/System/HierarchySystem.cpp
            Engine/ECS/System/TransformSystem.h Engine/ECS/System/TransformSystem.cpp
            Engine/ECS/System/SoundSystem.h Engine/ECS/System/SoundSystem.cpp)
        if(has_wren)
            list(APPEND sources
                Engine/ECS/Component/WrenScript.h
                Engine/ECS/ScriptComponents.h Engine/ECS/EcsComponentList.h
                Engine/ECS/WrenScriptComponentRegistry.h Engine/ECS/WrenScriptComponentRegistry.cpp
                Engine/ECS/System/WrenScriptSystem.h Engine/ECS/System/WrenScriptSystem.cpp
                Engine/ECS/System/WrenStateSystem.h Engine/ECS/System/WrenStateSystem.cpp
                Engine/ECS/System/EventSystem.h Engine/ECS/System/EventSystem.cpp
                WrenBindings/WrenGameObject.cpp
                WrenBindings/WrenGameObjectComponents.h WrenBindings/WrenGameObjectComponents.cpp)
        endif()
        struktur_optional(optional physics wren)
        struktur_library(ecs SOURCES ${sources} LINK ${prefix}_core ${optional})
    endif()

    # --- wren -------------------------------------------------------------------------------------------------------
    if(has_wren)
        set(sources
            Engine/Scripting/WrenServices.h Engine/Scripting/WrenServices.cpp
            Engine/Scripting/WrenScriptEngine.h Engine/Scripting/WrenScriptEngine.cpp
            Engine/Scripting/WrenBindingRegistry.h
            Engine/Scripting/WrenCodeGenerator.h Engine/Scripting/WrenCodeGenerator.cpp
            Engine/Scripting/WrenStateManager.h Engine/Scripting/WrenStateManager.cpp
            Engine/Scripting/WrenExportedFields.h Engine/Scripting/WrenExportedFields.cpp
            Engine/Scripting/WrenUtil.h Engine/Scripting/WrenUtil.cpp
            Engine/Scripting/WrenValueWrapper.h
            Engine/Scripting/CallbackHelperFunctions.h Engine/Scripting/CallbackHelperFunctions.cpp
            Engine/Scripting/WrenCallback.h Engine/Scripting/WrenCallback.cpp
            WrenBindings/WrenMath.h WrenBindings/WrenMath.cpp
            WrenBindings/WrenResourceManager.h WrenBindings/WrenResourceManager.cpp
            WrenBindings/WrenFunctionCallback.h WrenBindings/WrenFunctionCallback.cpp
            WrenBindings/WrenFileSystem.h WrenBindings/WrenFileSystem.cpp
            WrenBindings/WrenJobs.h WrenBindings/WrenJobs.cpp
            WrenBindings/WrenLocalization.cpp
            WrenBindings/WrenEvents.cpp
            WrenBindings/WrenApplication.cpp
            WrenBindings/WrenAudio.cpp
            WrenBindings/WrenInput.cpp
            WrenBindings/WrenDebug.cpp
            WrenBindings/WrenSerialisation.cpp)
        struktur_optional(optional ui ecs)
        struktur_library(wren SOURCES ${sources} LINK ${prefix}_core wren ${optional})
    endif()

    # --- ui ---------------------------------------------------------------------------------------------------------
    if(has_ui)
        set(sources
            Engine/UI/UIServices.h Engine/UI/UIServices.cpp
            Engine/UI/UIManager.h Engine/UI/UIManager.cpp
            Engine/UI/FocusNavigator.h Engine/UI/FocusNavigator.cpp
            Engine/UI/UIElement.h Engine/UI/UIElement.cpp
            Engine/UI/TextLayout.h Engine/UI/TextLayout.cpp
            Engine/UI/UILabel.h Engine/UI/UILabel.cpp
            Engine/UI/UIRichLabel.h Engine/UI/UIRichLabel.cpp
            Engine/UI/UIPanel.h Engine/UI/UIPanel.cpp
            Engine/UI/UIColor.h Engine/UI/UIColor.cpp
            Engine/UI/UIGraph.h Engine/UI/UIGraph.cpp
            Engine/UI/UITexture.h Engine/UI/UITexture.cpp
            Engine/UI/InputGlyphs.h Engine/UI/InputGlyphs.cpp
            Engine/UI/UIInputHint.h Engine/UI/UIInputHint.cpp
            Engine/UI/UINineSlice.h Engine/UI/UINineSlice.cpp
            Engine/UI/UIBorder.h Engine/UI/UIBorder.cpp
            Engine/UI/UIClip.h Engine/UI/UIClip.cpp
            Engine/UI/UIScroll.h Engine/UI/UIScroll.cpp
            Engine/Renderer/UIRenderer.h Engine/Renderer/UIRenderer.cpp
            Engine/ECS/System/UISystem.h Engine/ECS/System/UISystem.cpp
            Engine/ECS/System/UIRenderSystem.h Engine/ECS/System/UIRenderSystem.cpp)
        if(has_wren)
            list(APPEND sources WrenBindings/WrenUI.h WrenBindings/WrenUI.cpp)
        endif()
        struktur_optional(optional wren)
        struktur_library(ui SOURCES ${sources} LINK ${prefix}_core ${prefix}_ecs ${optional})
        struktur_use_shaders(${prefix}_ui UIShaders)
    endif()

    # --- physics ----------------------------------------------------------------------------------------------------
    if(has_physics)
        set(sources
            Engine/Physics/PhysicsServices.h Engine/Physics/PhysicsServices.cpp
            Engine/Physics/PhysicsWorld.h Engine/Physics/PhysicsWorld.cpp
            Engine/Physics/ContactListener.h Engine/Physics/ContactListener.cpp
            Engine/Physics/CollisionLayers.h Engine/Physics/CollisionLayers.cpp
            Engine/Physics/PhysicsComponentList.h
            Engine/ECS/Component/PhysicsBody.h
            Engine/ECS/System/PhysicsSystem.h Engine/ECS/System/PhysicsSystem.cpp)
        if(has_wren)
            list(APPEND sources
                WrenBindings/WrenPhysics.h WrenBindings/WrenPhysics.cpp
                WrenBindings/WrenCollisionLayers.cpp
                WrenBindings/WrenPhysicsBody.cpp)
        endif()
        struktur_optional(optional wren)
        struktur_library(physics SOURCES ${sources} LINK ${prefix}_core ${prefix}_ecs box2d ${optional})
    endif()

    # --- dialogue ---------------------------------------------------------------------------------------------------
    if(has_dialogue)
        set(sources
            Engine/Dialogue/DialogueServices.h Engine/Dialogue/DialogueServices.cpp
            Engine/Dialogue/DialogueManager.h Engine/Dialogue/DialogueManager.cpp
            Engine/Dialogue/DialogueNode.h Engine/Dialogue/DialogueNode.cpp
            Engine/Dialogue/DialogueRegistry.h Engine/Dialogue/DialogueRegistry.cpp
            Engine/Dialogue/DialogueValue.h Engine/Dialogue/DialogueValue.cpp
            Engine/Dialogue/Command.h Engine/Dialogue/Command.cpp
            Engine/Dialogue/Condition.h Engine/Dialogue/Condition.cpp
            Engine/Dialogue/TextSystems/VariableSubstitutionSystem.h
            Engine/Dialogue/TextSystems/VariableSubstitutionSystem.cpp
            Engine/Dialogue/DialogueResult.h)
        if(has_wren)
            list(APPEND sources
                Engine/Dialogue/DialogueHelperFunctions.h Engine/Dialogue/DialogueHelperFunctions.cpp
                WrenBindings/WrenDialogue.h WrenBindings/WrenDialogue.cpp
                WrenBindings/WrenVariableSubstitutionSystem.cpp)
        endif()
        struktur_optional(optional wren)
        struktur_library(dialogue SOURCES ${sources} LINK ${prefix}_core ${optional})
    endif()

    # --- flag -------------------------------------------------------------------------------------------------------
    if("flag" IN_LIST libraries)
        set(sources
            Engine/Flag/FlagServices.h Engine/Flag/FlagServices.cpp
            Engine/Flag/FlagManager.h Engine/Flag/FlagManager.cpp)
        if(has_wren)
            list(APPEND sources WrenBindings/WrenFlags.cpp)
        endif()
        struktur_optional(optional wren)
        struktur_library(flag SOURCES ${sources} LINK ${prefix}_core ${optional})
    endif()

    # --- fitness ----------------------------------------------------------------------------------------------------
    if("fitness" IN_LIST libraries)
        set(sources
            Engine/Fitness/FitnessServices.h Engine/Fitness/FitnessServices.cpp
            Engine/Input/TrainerInput.h)
        if(has_ecs)
            list(APPEND sources Engine/ECS/System/FitnessSystem.h Engine/ECS/System/FitnessSystem.cpp)
        endif()
        if(has_wren)
            list(APPEND sources WrenBindings/WrenFitness.cpp)
        endif()
        struktur_optional(optional ecs wren)
        struktur_library(fitness SOURCES ${sources} LINK ${prefix}_core StrukturFitness ${optional})
    endif()

    # --- editor -----------------------------------------------------------------------------------------------------
    if(ARG_EDITOR)
        set(sources
            Debug/Editor/Editor.h Debug/Editor/Editor.cpp
            Debug/Editor/EditorServices.h Debug/Editor/EditorServices.cpp
            Debug/Editor/EditorLayoutManager.h Debug/Editor/EditorLayoutManager.cpp
            Debug/Editor/EditorSettings.h Debug/Editor/EditorSettings.cpp
            Debug/Editor/EditorTheme.h Debug/Editor/EditorTheme.cpp
            Debug/Editor/SaveFolders.h Debug/Editor/SaveFolders.cpp
            Debug/Editor/DebugSystem.h Debug/Editor/DebugSystem.cpp
            Engine/Renderer/DebugRenderer.h Engine/Renderer/DebugRenderer.cpp
            Engine/Renderer/ImGuiRenderer.h Engine/Renderer/ImGuiRenderer.cpp
            Debug/Editor/Windows/EditorWindow.h Debug/Editor/Windows/EditorWindow.cpp
            Debug/Editor/Windows/DebugInfoWindow.h Debug/Editor/Windows/DebugInfoWindow.cpp
            Debug/Editor/Windows/GameViewPortWindow.h Debug/Editor/Windows/GameViewPortWindow.cpp
            Debug/Editor/Windows/HierarchyWindow.h Debug/Editor/Windows/HierarchyWindow.cpp
            Debug/Editor/Windows/ToolbarWindow.h Debug/Editor/Windows/ToolbarWindow.cpp
            Debug/Editor/Windows/InspectorWindow.h Debug/Editor/Windows/InspectorWindow.cpp
            Debug/Editor/Windows/SettingsWindow.h Debug/Editor/Windows/SettingsWindow.cpp
            Debug/Editor/Windows/PreviewWindow.h Debug/Editor/Windows/PreviewWindow.cpp
            Debug/Editor/Windows/FileExplorerWindow.h Debug/Editor/Windows/FileExplorerWindow.cpp
            Debug/Editor/Windows/LogWindow.h Debug/Editor/Windows/LogWindow.cpp
            Debug/Editor/Windows/ProfilerWindow.h Debug/Editor/Windows/ProfilerWindow.cpp
            Debug/Editor/Windows/ResourceManagerWindow.h Debug/Editor/Windows/ResourceManagerWindow.cpp
            Debug/Editor/Windows/LocalizationWindow.h Debug/Editor/Windows/LocalizationWindow.cpp
            Debug/Editor/Windows/SavesWindow.h Debug/Editor/Windows/SavesWindow.cpp
            Debug/Editor/PreviewRenderers/PreviewRenderer.h
            Debug/Editor/PreviewRenderers/TexturePreviewRenderer.h
            Debug/Editor/PreviewRenderers/TexturePreviewRenderer.cpp
            Debug/Editor/PreviewRenderers/TextFilePreviewRenderer.h
            Debug/Editor/PreviewRenderers/TextFilePreviewRenderer.cpp
            Debug/Editor/PreviewRenderers/UnknownFilePreviewRenderer.h
            Debug/Editor/PreviewRenderers/UnknownFilePreviewRenderer.cpp
            Debug/Editor/PreviewRenderers/PreviewHelpers.h)
        if(has_wren)
            list(APPEND sources
                Debug/Editor/ExportedFieldRenderer.h Debug/Editor/ExportedFieldRenderer.cpp
                Debug/Editor/Windows/StateWindow.h Debug/Editor/Windows/StateWindow.cpp)
        endif()
        if(has_ui)
            list(APPEND sources
                Debug/Editor/Windows/UIHierarchyWindow.h Debug/Editor/Windows/UIHierarchyWindow.cpp)
        endif()
        if(has_physics)
            list(APPEND sources Debug/Box2DBgfxDebugDraw.h Debug/Box2DBgfxDebugDraw.cpp)
        endif()
        # The dialogue editor loads and previews dialogue through the scripts.
        if(has_dialogue AND has_wren)
            list(APPEND sources
                Debug/Editor/Windows/DialogueEditorWindow.h Debug/Editor/Windows/DialogueEditorWindow.cpp
                Debug/Editor/Exporters/DialogueExporter.h Debug/Editor/Exporters/DialogueExporter.cpp)
        endif()
        struktur_optional(optional ui physics dialogue wren)
        struktur_library(editor SOURCES ${sources} LINK ${prefix}_core ${prefix}_ecs imgui tinyfiledialogs ${optional})
    endif()

    # --- app: the startup and frame every game shares -----------------------------------------------------------------
    if(has_ecs AND has_wren AND has_ui)
        struktur_optional(optional ${STRUKTUR_ALL_LIBRARIES} editor)
        struktur_library(app SOURCES Engine/App/App.h Engine/App/App.cpp Engine/App/EngineBindings.cpp
            LINK ${prefix}_core ${optional})
    endif()

    set(${prefix}_TARGETS ${targets} PARENT_SCOPE)
endfunction()

# struktur_add_game(<name> [LIBRARIES <name>...] SOURCES <file>... ASSETS <folder> [COMPONENTS <header>])
#
# The game's executable, with the engine compiled for its libraries (and the editor, when EDITOR is on). ASSETS is the
# game's asset folder: the editor opens it and a web build packs it. Its src folder is on the include path.
function(struktur_add_game name)
    cmake_parse_arguments(GAME "" "ASSETS;COMPONENTS" "LIBRARIES;SOURCES" ${ARGN})

    set(editor)
    if(EDITOR)
        set(editor EDITOR)
    endif()
    struktur_add_libraries(struktur LIBRARIES ${GAME_LIBRARIES} ${editor} ASSETS ${GAME_ASSETS}
        COMPONENTS ${GAME_COMPONENTS})

    add_executable(${name} ${GAME_SOURCES})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${name} PRIVATE ${struktur_TARGETS})
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${name})

    if(PLATFORM_WEB)
        set(flags "-s ASSERTIONS=1 -s WASM=1 -s GL_ENABLE_GET_PROC_ADDRESS=1 -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1")
        if(USE_ASYNCIFY)
            string(APPEND flags " -s ASYNCIFY")
        endif()
        string(APPEND flags " -s EXPORTED_RUNTIME_METHODS=['HEAPF32','HEAPU8','HEAPU16','HEAPU32','HEAP32','HEAP8','FS']")
        # PROJECT_ASSETS_FOLDER overrides the folder that gets packed.
        if(PROJECT_ASSETS_FOLDER)
            set(GAME_ASSETS "${PROJECT_ASSETS_FOLDER}")
        endif()
        string(APPEND flags " --preload-file \"${GAME_ASSETS}@/assets\"")
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico)
            string(APPEND flags " --preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico@/favicon.ico\"")
        endif()
        if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web/assets)
            string(APPEND flags " --preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/web/assets@/\"")
        endif()
        # The page around the game: WEB_SHELL_FILE, the game's web/shell.html, or the engine's.
        set(shell "${STRUKTUR_ENGINE_DIR}/web/shell.html")
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
            set(shell "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
        endif()
        if(DEFINED WEB_SHELL_FILE)
            set(shell "${WEB_SHELL_FILE}")
        endif()
        string(APPEND flags " --shell-file \"${shell}\"")
        set_target_properties(${name} PROPERTIES SUFFIX ".html" LINK_FLAGS "${flags}")
    elseif(WIN32)
        # A release build has no console window.
        if(CMAKE_BUILD_TYPE STREQUAL "Release")
            set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE True)
            target_compile_definitions(${name} PRIVATE WINDOWS_EXECUTABLE)
        endif()
    endif()

    install(TARGETS ${name} DESTINATION .)
endfunction()

# struktur_add_check(<name> [LIBRARIES <name>...] [EDITOR])
#
# Compiles the engine for one set of libraries and links all of it into an empty program, so that code using a library
# the set leaves out fails to compile or link. The engine's own CMakeLists lists the sets.
function(struktur_add_check name)
    cmake_parse_arguments(CHECK "EDITOR" "" "LIBRARIES" ${ARGN})
    set(editor)
    if(CHECK_EDITOR)
        set(editor EDITOR)
    endif()
    struktur_add_libraries(check_${name} LIBRARIES ${CHECK_LIBRARIES} ${editor})
    add_executable(check_${name} ${STRUKTUR_ENGINE_DIR}/tests/LibraryCheck.cpp)
    target_link_libraries(check_${name} PRIVATE ${check_${name}_TARGETS})
    set_target_properties(check_${name} PROPERTIES FOLDER "check_${name}")
    # Every object of the engine, used or not, so nothing unresolved can hide.
    if(MSVC)
        foreach(target IN LISTS check_${name}_TARGETS)
            target_link_options(check_${name} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${target}>")
        endforeach()
    endif()
endfunction()
