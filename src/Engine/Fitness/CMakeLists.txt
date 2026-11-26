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
        ${STRUKTUR_FITNESS_LOGGER_SOURCE}
    )
    target_include_directories(StrukturFitnessTests PRIVATE ${doctest_SOURCE_DIR}/doctest)
    target_link_libraries(StrukturFitnessTests PRIVATE StrukturFitness)

    enable_testing()
    add_test(NAME StrukturFitnessTests COMMAND StrukturFitnessTests)
endif()
