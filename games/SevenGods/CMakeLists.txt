# Seven Gods, built on the Struktur engine.
#
#   cmake -S . -B build-windows-debug -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-windows-debug --config Debug
#
# (builds\ has scripts for this; add -DEDITOR=FALSE for a Debug build without the editor.)
cmake_minimum_required(VERSION 3.24)
project(SevenGods C CXX)

# The engine: this repository while the game lives inside it; the game's engine/ submodule once it moves out. Set it to
# another checkout to build against that one.
set(STRUKTUR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Engine source")
include(${STRUKTUR_DIR}/cmake/Struktur.cmake)

struktur_add_game(SevenGods
    LIBRARIES ecs wren ui physics world2d dialogue flag
    ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets
    SOURCES
        src/Main.cpp
        src/Game.h                src/Game.cpp
        src/SevenGodsContext.h    src/SevenGodsContext.cpp
)
