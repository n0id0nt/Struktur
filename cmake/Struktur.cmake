# The engine's entry point for CMake. A game includes this from its own CMakeLists, after project(), and lists the
# libraries it uses:
#
#   set(STRUKTUR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine CACHE PATH "Engine source")
#   include(${STRUKTUR_DIR}/cmake/Struktur.cmake)
#   struktur_add_game(MyGame LIBRARIES ecs wren ui ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets SOURCES src/Game.cpp)
#
# See StrukturLibraries.cmake for the libraries and struktur_add_game, and StrukturShaders.cmake for a game's shaders.
include_guard(GLOBAL)

get_filename_component(STRUKTUR_ENGINE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
# Where the engine's parts build, inside the including project's build folder.
set(STRUKTUR_BINARY_DIR ${CMAKE_BINARY_DIR}/struktur)

include(${CMAKE_CURRENT_LIST_DIR}/StrukturPlatform.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/StrukturShaders.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/StrukturDependencies.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/StrukturLibraries.cmake)
