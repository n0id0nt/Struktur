# The ride's logic and road world, free of SDL, bgfx and Wren, so the tests build them without the rest of the game.
# Needs STRUKTUR_DIR and the engine's StrukturFitness and StrukturJobs targets.
#
#   RideWorld  the procedural road world behind the ride screens: route spline, elevation, biomes, chunk meshes, rider
#              animation, asset manifest and placeholders. The bgfx renderer lives in the game.
#   RideCore   the ride screens' logic: zones, speed model, workouts, ERG/SIM control, recording, FIT.
include(FetchContent)
# pugixml (MIT) - workout (.zwo) parsing. Pinned release tarball + hash; pugixml.cpp is compiled straight into
# RideCore (SOURCE_SUBDIR keeps pugixml's own CMake project, and its install rules, out of the build).
FetchContent_Declare(pugixml
    URL https://github.com/zeux/pugixml/releases/download/v1.14/pugixml-1.14.tar.gz
    URL_HASH SHA256=2f10e276870c64b1db6809050a75e11a897a8d7456c4be5c6b2e35a11168a015
    SOURCE_SUBDIR not-a-cmake-dir
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(pugixml)

get_filename_component(ride_source "${CMAKE_CURRENT_LIST_DIR}/../src" ABSOLUTE)

set(WORLD_DIR ${ride_source}/World)
add_library(RideWorld STATIC
    ${WORLD_DIR}/Noise.h               ${WORLD_DIR}/Noise.cpp
    ${WORLD_DIR}/WorldSettings.h       ${WORLD_DIR}/WorldSettings.cpp
    ${WORLD_DIR}/Route.h               ${WORLD_DIR}/Route.cpp
    ${WORLD_DIR}/ElevationProfile.h    ${WORLD_DIR}/ElevationProfile.cpp
    ${WORLD_DIR}/Biome.h               ${WORLD_DIR}/Biome.cpp
    ${WORLD_DIR}/ChunkBuilder.h        ${WORLD_DIR}/ChunkBuilder.cpp
    ${WORLD_DIR}/ChunkStreamer.h       ${WORLD_DIR}/ChunkStreamer.cpp
    ${WORLD_DIR}/RiderAnimation.h      ${WORLD_DIR}/RiderAnimation.cpp
    ${WORLD_DIR}/SpriteDescriptor.h    ${WORLD_DIR}/SpriteDescriptor.cpp
    ${WORLD_DIR}/AssetManifest.h       ${WORLD_DIR}/AssetManifest.cpp
    ${WORLD_DIR}/Placeholder.h         ${WORLD_DIR}/Placeholder.cpp
    ${WORLD_DIR}/World.h               ${WORLD_DIR}/World.cpp
    ${WORLD_DIR}/Camera.h
    ${WORLD_DIR}/RoadFollow.h          ${WORLD_DIR}/RoadFollow.cpp
    ${WORLD_DIR}/Pacer.h               ${WORLD_DIR}/Pacer.cpp
)
target_include_directories(RideWorld PUBLIC ${ride_source} ${STRUKTUR_DIR}/src ${STRUKTUR_DIR}/third_party/glm)
# The job system builds the road's chunks.
target_link_libraries(RideWorld PUBLIC nlohmann_json::nlohmann_json StrukturJobs)
target_compile_features(RideWorld PUBLIC cxx_std_20)
find_package(Threads)
if(Threads_FOUND)
    target_link_libraries(RideWorld PUBLIC Threads::Threads)
endif()

set(RIDE_DIR ${ride_source}/Ride)
add_library(RideCore STATIC
    ${RIDE_DIR}/Zones.h                 ${RIDE_DIR}/Zones.cpp
    ${RIDE_DIR}/SpeedModel.h            ${RIDE_DIR}/SpeedModel.cpp
    ${RIDE_DIR}/Workout.h               ${RIDE_DIR}/WorkoutLoader.cpp
    ${RIDE_DIR}/WorkoutRunner.h         ${RIDE_DIR}/WorkoutRunner.cpp
    ${RIDE_DIR}/TrainerController.h     ${RIDE_DIR}/TrainerController.cpp
    ${RIDE_DIR}/RideLog.h               ${RIDE_DIR}/RideLog.cpp
    ${RIDE_DIR}/FitWriter.h             ${RIDE_DIR}/FitWriter.cpp
    ${RIDE_DIR}/RideSettings.h          ${RIDE_DIR}/RideSettings.cpp
    ${RIDE_DIR}/RideSession.h           ${RIDE_DIR}/RideSession.cpp
    ${pugixml_SOURCE_DIR}/src/pugixml.cpp
)
target_include_directories(RideCore PUBLIC ${ride_source} ${pugixml_SOURCE_DIR}/src)
# Drives a Fitness::Trainer.
target_link_libraries(RideCore PUBLIC StrukturFitness RideWorld nlohmann_json::nlohmann_json)
target_compile_features(RideCore PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(RideWorld PRIVATE /W4)
    target_compile_options(RideCore PRIVATE /W4)
    # Third-party source compiled into this target: keep its warnings out of our /W4 output.
    set_source_files_properties(${pugixml_SOURCE_DIR}/src/pugixml.cpp PROPERTIES COMPILE_OPTIONS "/W0")
endif()
