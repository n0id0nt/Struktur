# StrukturFitness - smart trainer / heart rate / power meter support (see docs/SmartTrainer.md).
#
# Deliberately free of SDL, bgfx and Wren so the unit tests and the trainer_probe tool link it on their own. Can be
# configured standalone (cmake -S src/Engine/Fitness) for a fast test loop, or pulled in by the root CMakeLists via
# add_subdirectory().
cmake_minimum_required(VERSION 3.15)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(StrukturFitness CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set(STRUKTUR_FITNESS_STANDALONE TRUE)
    option(STRUKTUR_BUILD_TESTS "Build the StrukturFitness unit tests" ON)
    option(STRUKTUR_BUILD_TOOLS "Build trainer_probe" ON)
else()
    set(STRUKTUR_FITNESS_STANDALONE FALSE)
endif()

# Repo root - Debug/Assertions.h and nlohmann/json live outside this folder.
get_filename_component(STRUKTUR_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

if(STRUKTUR_FITNESS_STANDALONE AND NOT TARGET nlohmann_json::nlohmann_json)
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    add_subdirectory(${STRUKTUR_REPO_ROOT}/third_party/json ${CMAKE_BINARY_DIR}/third_party/json)
endif()

set(FITNESS_SOURCES
    Protocol/Bytes.h
    Protocol/BleUuid.h              Protocol/BleUuid.cpp
    Protocol/Ftms.h                 Protocol/Ftms.cpp
    Protocol/HeartRate.h            Protocol/HeartRate.cpp
    Protocol/CyclingPower.h         Protocol/CyclingPower.cpp
    Protocol/CadenceCalculator.h    Protocol/CadenceCalculator.cpp

    Core/ThreadSafeQueue.h
    Core/Backoff.h

    Ble/IBleCentral.h               Ble/BleCommon.cpp
    Ble/NullBleCentral.h
    Ble/FakeBleCentral.h            Ble/FakeBleCentral.cpp
    Ble/BleCentralFactory.h         Ble/BleCentralFactory.cpp
    Ble/FakeDevices.h               Ble/FakeDevices.cpp
    Ble/SimulatedBleCentral.h       Ble/SimulatedBleCentral.cpp

    Devices/DeviceTypes.h           Devices/DeviceTypes.cpp
    Devices/BleLink.h               Devices/BleLink.cpp
    Devices/ControlPointQueue.h     Devices/ControlPointQueue.cpp
    Devices/FtmsTrainer.h           Devices/FtmsTrainer.cpp
    Devices/BleSensors.h            Devices/BleSensors.cpp
    Devices/DeviceDiscovery.h       Devices/DeviceDiscovery.cpp
    Devices/DeviceRegistry.h        Devices/DeviceRegistry.cpp

    FitnessManager.h                FitnessManager.cpp
)

# Windows desktop: real BLE through the WinRT APIs (C++/WinRT headers ship with the Windows 10+ SDK). Every other
# platform (web, Linux, macOS) gets NullBleCentral until a backend is written - see docs/SmartTrainerLibraries.md.
if(WIN32 AND NOT EMSCRIPTEN)
    set(STRUKTUR_FITNESS_WINRT TRUE)
    list(APPEND FITNESS_SOURCES Ble/WinRtBleCentral.h Ble/WinRtBleCentral.cpp)
else()
    set(STRUKTUR_FITNESS_WINRT FALSE)
endif()

add_library(StrukturFitness STATIC ${FITNESS_SOURCES})
# Includes are written repo-style ("Engine/Fitness/...", "Debug/Assertions.h"), so expose src/ itself.
target_include_directories(StrukturFitness PUBLIC ${STRUKTUR_REPO_ROOT}/src)
target_link_libraries(StrukturFitness PUBLIC nlohmann_json::nlohmann_json)
target_compile_features(StrukturFitness PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(StrukturFitness PRIVATE /W4)
endif()
if(STRUKTUR_FITNESS_WINRT)
    target_compile_definitions(StrukturFitness PRIVATE STRUKTUR_FITNESS_WINRT)
    # C++/WinRT projection headers produce very large objects; windowsapp is the umbrella import lib for WinRT.
    set_source_files_properties(Ble/WinRtBleCentral.cpp PROPERTIES COMPILE_OPTIONS "/bigobj")
    target_link_libraries(StrukturFitness PRIVATE windowsapp)
endif()

# StrukturRide - the ride screens' logic (zones, speed model, workouts, ERG/SIM control, recording, FIT). Lives
# beside StrukturFitness because it drives a Fitness::Trainer; still free of SDL/bgfx/Wren so it unit-tests here.
include(FetchContent)
# pugixml (MIT) - workout (.zwo) parsing. Pinned release tarball + hash; pugixml.cpp is compiled straight into
# StrukturRide (SOURCE_SUBDIR keeps pugixml's own CMake project, and its install rules, out of the build).
FetchContent_Declare(pugixml
    URL https://github.com/zeux/pugixml/releases/download/v1.14/pugixml-1.14.tar.gz
    URL_HASH SHA256=2f10e276870c64b1db6809050a75e11a897a8d7456c4be5c6b2e35a11168a015
    SOURCE_SUBDIR not-a-cmake-dir
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(pugixml)

# StrukturWorld - the procedural road world behind the ride screens (route spline, elevation, biomes, chunk
# meshes, rider animation, asset manifest + placeholders). Pure C++: the bgfx renderer lives in the engine.
set(WORLD_DIR ${STRUKTUR_REPO_ROOT}/src/Engine/World)
set(WORLD_SOURCES
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
    ${WORLD_DIR}/World.h           ${WORLD_DIR}/World.cpp
)
# The job system (chunk builds, ride autosaves). The root build adds it first; standalone, pull it in here.
if(NOT TARGET StrukturJobs)
    add_subdirectory(${STRUKTUR_REPO_ROOT}/src/Engine/Jobs ${CMAKE_CURRENT_BINARY_DIR}/StrukturJobs)
endif()

add_library(StrukturWorld STATIC ${WORLD_SOURCES})
target_include_directories(StrukturWorld PUBLIC ${STRUKTUR_REPO_ROOT}/src)
target_link_libraries(StrukturWorld PUBLIC nlohmann_json::nlohmann_json StrukturJobs)
target_compile_features(StrukturWorld PUBLIC cxx_std_20)
find_package(Threads)
if(Threads_FOUND)
    target_link_libraries(StrukturWorld PUBLIC Threads::Threads)
endif()
if(MSVC)
    target_compile_options(StrukturWorld PRIVATE /W4)
endif()

set(RIDE_DIR ${STRUKTUR_REPO_ROOT}/src/Engine/Ride)
set(RIDE_SOURCES
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
add_library(StrukturRide STATIC ${RIDE_SOURCES})
target_include_directories(StrukturRide PUBLIC ${STRUKTUR_REPO_ROOT}/src ${pugixml_SOURCE_DIR}/src)
target_link_libraries(StrukturRide PUBLIC StrukturFitness StrukturWorld nlohmann_json::nlohmann_json)
target_compile_features(StrukturRide PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(StrukturRide PRIVATE /W4)
    # Third-party source compiled into this target: keep its warnings out of our /W4 output.
    set_source_files_properties(${pugixml_SOURCE_DIR}/src/pugixml.cpp PROPERTIES COMPILE_OPTIONS "/W0")
endif()

# Debug/Logger.cpp is compiled into the game executable already; standalone consumers (tests, probe) add it to
# their own sources instead of this library, so the game never sees a duplicate definition.
set(STRUKTUR_FITNESS_LOGGER_SOURCE ${STRUKTUR_REPO_ROOT}/src/Debug/Logger.cpp)

if(STRUKTUR_BUILD_TESTS)
    include(FetchContent)
    # doctest (MIT) - pinned release tarball + hash. Only the single header is used; SOURCE_SUBDIR points at a folder
    # with no CMakeLists so doctest's own project (install/test targets) is never added to this build.
    FetchContent_Declare(doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.12.tar.gz
        URL_HASH SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70
        SOURCE_SUBDIR header-only-not-a-cmake-dir
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(doctest)

    add_executable(StrukturFitnessTests
        Tests/TestMain.cpp
        Tests/ProtocolTests.cpp
        Tests/DeviceTests.cpp
        Tests/RideCoreTests.cpp
        Tests/RideControlTests.cpp
        Tests/RideWorldTests.cpp
        ${STRUKTUR_FITNESS_LOGGER_SOURCE}
    )
    target_include_directories(StrukturFitnessTests PRIVATE ${doctest_SOURCE_DIR}/doctest)
    target_link_libraries(StrukturFitnessTests PRIVATE StrukturFitness StrukturRide StrukturWorld)
    # Sample/malformed workout files the tests load from disk.
    target_compile_definitions(StrukturFitnessTests PRIVATE
        STRUKTUR_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests/Data"
        STRUKTUR_WORKOUTS_DIR="${STRUKTUR_REPO_ROOT}/assets/Workouts"
        STRUKTUR_ASSETS_DIR="${STRUKTUR_REPO_ROOT}/assets")

    enable_testing()
    add_test(NAME StrukturFitnessTests COMMAND StrukturFitnessTests)
endif()

if(STRUKTUR_BUILD_TOOLS)
    add_executable(trainer_probe
        ${STRUKTUR_REPO_ROOT}/tools/trainer_probe/main.cpp
        ${STRUKTUR_FITNESS_LOGGER_SOURCE}
    )
    target_link_libraries(trainer_probe PRIVATE StrukturFitness)
endif()
