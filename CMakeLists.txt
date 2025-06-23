cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME "Struktur")
project(${PROJECT_NAME} CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##########################################################################################
# Configurable options
##########################################################################################

# toggle libs to be used with ON and OFF

option(USE_RAYLIB "Use raylib" ON)
option(USE_ASYNCIFY "Enable ASYNCIFY for traditional game loops" OFF)
option(USE_PHYSFS "Use PhysicsFS for asset management" OFF)

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

##########################################################################################
# Add dependencies with FetchContent
##########################################################################################

include(FetchContent)

##########################################################################################
# Raylib Setup
##########################################################################################

if(USE_RAYLIB)
   #set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
   #set(BUILD_GAMES OFF CACHE BOOL "" FORCE) # don't build the supplied example games
   #set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
   #set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE) 

    set(dep raylib)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
    )
    
    FetchContent_MakeAvailable(raylib)
endif()

# Source files
set(SOURCES
    src/main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link Libraries
target_link_libraries(${PROJECT_NAME} raylib)

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