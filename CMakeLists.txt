cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME "Struktur")
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##########################################################################################
# Configurable options
##########################################################################################

# toggle libs to be used with ON and OFF

option(USE_ASYNCIFY "Enable ASYNCIFY for traditional game loops" OFF)

# Platform detection
if(EMSCRIPTEN)
    set(PLATFORM_WEB TRUE)
    message(STATUS "Building for Web (Emscripten)")
else()
    set(PLATFORM_DESKTOP TRUE)
    message(STATUS "Building for Desktop")
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
        GIT_TAG 0.9.9.8
    )
    
    # Lua - Scripting language
    FetchContent_Declare(
        lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.6
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
    
    # Sol2 - Lua C++ binding (depends on Lua)
    FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG v3.3.0
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
    
    # Handle Lua separately (needs custom build)
    FetchContent_GetProperties(lua)
    if(NOT lua_POPULATED)
        FetchContent_Populate(lua)
        
        # Build Lua as a static library (compatible with Sol2)
        set(LUA_LIB_SOURCES
            ${lua_SOURCE_DIR}/lapi.c
            ${lua_SOURCE_DIR}/lcode.c
            ${lua_SOURCE_DIR}/lctype.c
            ${lua_SOURCE_DIR}/ldebug.c
            ${lua_SOURCE_DIR}/ldo.c
            ${lua_SOURCE_DIR}/ldump.c
            ${lua_SOURCE_DIR}/lfunc.c
            ${lua_SOURCE_DIR}/lgc.c
            ${lua_SOURCE_DIR}/llex.c
            ${lua_SOURCE_DIR}/lmem.c
            ${lua_SOURCE_DIR}/lobject.c
            ${lua_SOURCE_DIR}/lopcodes.c
            ${lua_SOURCE_DIR}/lparser.c
            ${lua_SOURCE_DIR}/lstate.c
            ${lua_SOURCE_DIR}/lstring.c
            ${lua_SOURCE_DIR}/ltable.c
            ${lua_SOURCE_DIR}/ltm.c
            ${lua_SOURCE_DIR}/lundump.c
            ${lua_SOURCE_DIR}/lvm.c
            ${lua_SOURCE_DIR}/lzio.c
            ${lua_SOURCE_DIR}/lauxlib.c
            ${lua_SOURCE_DIR}/lbaselib.c
            ${lua_SOURCE_DIR}/lcorolib.c
            ${lua_SOURCE_DIR}/ldblib.c
            ${lua_SOURCE_DIR}/liolib.c
            ${lua_SOURCE_DIR}/lmathlib.c
            ${lua_SOURCE_DIR}/loadlib.c
            ${lua_SOURCE_DIR}/loslib.c
            ${lua_SOURCE_DIR}/lstrlib.c
            ${lua_SOURCE_DIR}/ltablib.c
            ${lua_SOURCE_DIR}/lutf8lib.c
            ${lua_SOURCE_DIR}/linit.c
        )
        
        add_library(lua STATIC ${LUA_LIB_SOURCES})
        target_include_directories(lua PUBLIC ${lua_SOURCE_DIR})
        
        # Lua compile definitions
        target_compile_definitions(lua PRIVATE LUA_BUILD_AS_DLL=0)
        
        # Platform-specific settings
        if(WIN32)
            target_compile_definitions(lua PRIVATE 
                LUA_USE_WINDOWS
                _CRT_SECURE_NO_WARNINGS
            )
        elseif(UNIX AND NOT EMSCRIPTEN)
            target_compile_definitions(lua PRIVATE LUA_USE_POSIX)
            target_link_libraries(lua PRIVATE m dl)
        endif()
        
        # Ensure C linkage for C++ (needed for Sol2)
        set_target_properties(lua PROPERTIES
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
        )
    endif()

    # Handle Sol2 (header-only, needs Lua)
    FetchContent_GetProperties(sol2)
    if(NOT sol2_POPULATED)
        FetchContent_Populate(sol2)
        add_library(sol2 INTERFACE)
        target_include_directories(sol2 INTERFACE ${sol2_SOURCE_DIR}/include)
        target_link_libraries(sol2 INTERFACE lua)
        target_compile_features(sol2 INTERFACE cxx_std_17)
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
    src/main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link Libraries
target_link_libraries(${PROJECT_NAME} 
    raylib
    EnTT::EnTT
    glm::glm
    lua
    nlohmann_json::nlohmann_json
    pugixml
    raygui
    box2d
    sol2
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