# Indoor Cycling Software, built on the Struktur engine.
#
#   cmake -S . -B build-windows-debug -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-windows-debug --config Debug
#
# (builds\ has scripts for this; add -DEDITOR=FALSE for a Debug build without the editor.) The tests build on their own:
# see tests\CMakeLists.txt.
cmake_minimum_required(VERSION 3.24)
project(IndoorCyclingSoftware C CXX)

# The engine: this repository while the game lives inside it; the game's engine/ submodule once it moves out. Set it to
# another checkout to build against that one.
set(STRUKTUR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Engine source")
include(${STRUKTUR_DIR}/cmake/Struktur.cmake)
include(cmake/RideLibraries.cmake)

# The procedural road world (src/Renderer/WorldRenderer).
struktur_add_shaders(RideShaders DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    VARYING ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying_rideWorld.def.sc
    SHADERS rideTerrain rideRoad rideProp rideSky ridePost)

struktur_add_game(IndoorCyclingSoftware
    LIBRARIES ecs wren ui fitness
    ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets
    SOURCES
        src/Main.cpp
        src/Game.h                          src/Game.cpp
        src/RideContext.h                   src/RideContext.cpp

        src/RideIntegration/RideService.h   src/RideIntegration/RideService.cpp

        src/Components/WorldComponents.h
        src/Systems/RideSystem.h            src/Systems/RideSystem.cpp
        src/Systems/WorldSystem.h           src/Systems/WorldSystem.cpp
        src/Systems/PacerSystem.h           src/Systems/PacerSystem.cpp
        src/Systems/RiderAnimationSystem.h  src/Systems/RiderAnimationSystem.cpp
        src/Systems/CameraSystem.h          src/Systems/CameraSystem.cpp
        src/Systems/WorldRenderSystem.h     src/Systems/WorldRenderSystem.cpp

        src/World/WorldAssetLoader.h        src/World/WorldAssetLoader.cpp
        src/World/WorldLifecycle.h          src/World/WorldLifecycle.cpp
        src/Renderer/WorldRenderer.h        src/Renderer/WorldRenderer.cpp

        src/Bindings/RideBindings.h
        src/Bindings/WrenRide.cpp
        src/Bindings/WrenWorld.cpp
)
struktur_use_shaders(IndoorCyclingSoftware RideShaders)
target_link_libraries(IndoorCyclingSoftware PRIVATE RideCore RideWorld)
