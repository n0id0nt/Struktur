# The ride's unit tests, without the game: a fast test loop.
#
#   cmake -S tests -B build-tests -G "Visual Studio 17 2022"
#   cmake --build build-tests --config Debug
#   build-tests\Debug\RideTests.exe
cmake_minimum_required(VERSION 3.24)
project(RideTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(STRUKTUR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "Engine source")
get_filename_component(game_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
add_subdirectory(${STRUKTUR_DIR}/third_party/json ${CMAKE_BINARY_DIR}/third_party/json)
# StrukturFitness, which brings in StrukturJobs once RideLibraries asks for it.
add_subdirectory(${STRUKTUR_DIR}/src/Engine/Jobs ${CMAKE_BINARY_DIR}/Jobs)
add_subdirectory(${STRUKTUR_DIR}/src/Engine/Fitness ${CMAKE_BINARY_DIR}/Fitness)
include(${game_dir}/cmake/RideLibraries.cmake)

include(FetchContent)
# doctest (MIT) - the same pinned release as the engine's tests.
FetchContent_Declare(doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.12.tar.gz
    URL_HASH SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70
    SOURCE_SUBDIR header-only-not-a-cmake-dir
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(doctest)

add_executable(RideTests
    TestMain.cpp
    RideCoreTests.cpp
    RideControlTests.cpp
    RideWorldTests.cpp
    ${STRUKTUR_DIR}/src/Debug/Logger.cpp
)
target_include_directories(RideTests PRIVATE ${doctest_SOURCE_DIR}/doctest)
target_link_libraries(RideTests PRIVATE StrukturFitness RideCore RideWorld)
# Sample and malformed workout files the tests load from disk, and the game's own workouts and assets.
target_compile_definitions(RideTests PRIVATE
    STRUKTUR_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Data"
    STRUKTUR_WORKOUTS_DIR="${game_dir}/assets/Workouts"
    STRUKTUR_ASSETS_DIR="${game_dir}/assets")

enable_testing()
add_test(NAME RideTests COMMAND RideTests)
