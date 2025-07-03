cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME "Struktur")
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Disable compiler-specific extensions

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

# Find or fetch libraries
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    
    # Set FetchContent properties for better control
    set(FETCHCONTENT_QUIET FALSE)
    
    # RayLib - Main graphics library
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    
    # EnTT - Entity Component System
    FetchContent_Declare(
        entt
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.12.2
    )
    
    # GLM - OpenGL Mathematics
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    
    # nlohmann/json - JSON library
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    
    # PugiXML - XML library
    FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG v1.14
    )
    
    # RayGUI - Immediate mode GUI for RayLib
    FetchContent_Declare(
        raygui
        GIT_REPOSITORY https://github.com/raysan5/raygui.git
        GIT_TAG 4.0
    )
    
    # Box2D - 2D Physics Engine
    FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG v2.4.1
    )
    
    # Wren - Lightweight scripting language (cross-platform compatible)
    FetchContent_Declare(
        wren
        GIT_REPOSITORY https://github.com/wren-lang/wren.git
        GIT_TAG 0.4.0
    )
    
    # Dear ImGui - Immediate mode GUI (RayLib compatible version)
    FetchContent_Declare(
        imgui
        URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8-docking.tar.gz
    )

    # rlImGui - RayLib ImGui integration
    FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG HEAD
    )

    # Configure library options before making available
    
    # RayLib options
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    
    # GLM options
    set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
    
    # JSON options
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    
    # PugiXML options
    set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    
    # Box2D options
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
    
    # Make libraries available
    FetchContent_MakeAvailable(raylib entt glm nlohmann_json pugixml box2d)
    
    # Handle rlImGui (depends on ImGui)
    FetchContent_GetProperties(rlimgui)
    if(NOT rlimgui_POPULATED)
        FetchContent_Populate(rlimgui)
        add_library(rlimgui STATIC ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
        target_include_directories(rlimgui PUBLIC 
            ${rlimgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}
        )
        target_link_libraries(rlimgui PUBLIC raylib imgui)
    endif()
    
    # Handle Wren (cross-platform scripting)
    FetchContent_GetProperties(wren)
    if(NOT wren_POPULATED)
        FetchContent_Populate(wren)
        
        # Build Wren VM from source
        file(GLOB WREN_SOURCES 
            ${wren_SOURCE_DIR}/src/vm/*.c
            ${wren_SOURCE_DIR}/src/optional/*.c
        )
        
        add_library(wren STATIC ${WREN_SOURCES})
        target_include_directories(wren PUBLIC 
            ${wren_SOURCE_DIR}/src/include
            ${wren_SOURCE_DIR}/src/vm
            ${wren_SOURCE_DIR}/src/optional
        )
        
        # Platform-specific settings for Wren
        if(WIN32)
            target_compile_definitions(wren PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
    endif()

    # Handle RayGUI (header-only)
    FetchContent_GetProperties(raygui)
    if(NOT raygui_POPULATED)
        FetchContent_Populate(raygui)
        add_library(raygui INTERFACE)
        target_include_directories(raygui INTERFACE ${raygui_SOURCE_DIR}/src)
        target_link_libraries(raygui INTERFACE raylib)
    endif()
    
    # Handle Dear ImGui (custom build for rlImGui compatibility)
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
        
        # ImGui core files
        set(IMGUI_SOURCES
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        )
        
        add_library(imgui STATIC ${IMGUI_SOURCES})
        target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
        target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
    endif()
    
endif()


# Source files
set(SOURCES
    src/Main.cpp

    src/Engine/Core/Game.h src/Engine/Core/Game.cpp
    src/Engine/Core/GameContext.h src/Engine/Core/GameContext.cpp
    src/Engine/Core/Gamedata.h src/Engine/Core/GameData.cpp
    src/Engine/Core/Input.h src/Engine/Core/Input.cpp
    src/Engine/Core/ResourcePool.h src/Engine/Core/ResourcePool.cpp
    src/Engine/Core/SystemManager.h src/Engine/Core/SystemManager.cpp

    src/Debug/Assertions.h
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
    pugixml
    raygui
    box2d
    imgui
    rlimgui
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
        set_target_properties(${PROJECT_NAME} PROPERTIES
            WIN32_EXECUTABLE False
        )
    endif()
    
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/assets)
        # Copy assets to build directory
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
            COMMENT "Copying assets to build directory"
        )
    endif()
endif()