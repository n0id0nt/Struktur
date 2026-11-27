# StrukturJobs - the engine's job system (docs/JobSystem.md). Free of SDL, bgfx and Wren, so the unit tests and the
# job_bench benchmark build on their own: cmake -S src/Engine/Jobs -B build-jobs. The root CMakeLists also pulls it
# in with add_subdirectory().
cmake_minimum_required(VERSION 3.15)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(StrukturJobs CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    set(STRUKTUR_JOBS_STANDALONE TRUE)
    option(STRUKTUR_JOBS_BUILD_TESTS "Build the job system unit tests" ON)
    option(STRUKTUR_JOBS_BUILD_BENCH "Build the job_bench benchmark" ON)
else()
    set(STRUKTUR_JOBS_STANDALONE FALSE)
endif()

# Worker threads. Off by default on web: a browser only runs threads with a configured pthread pool and a page served
# cross-origin isolated. With it off the job system still works - jobs run on the calling thread (inline mode).
if(EMSCRIPTEN)
    option(STRUKTUR_THREADS "Build the job system with worker threads" OFF)
else()
    option(STRUKTUR_THREADS "Build the job system with worker threads" ON)
endif()

get_filename_component(STRUKTUR_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

add_library(StrukturJobs STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/JobSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/JobSystem.cpp
)
target_include_directories(StrukturJobs PUBLIC ${STRUKTUR_REPO_ROOT}/src)
target_compile_features(StrukturJobs PUBLIC cxx_std_20)
if(STRUKTUR_THREADS)
    target_compile_definitions(StrukturJobs PUBLIC STRUKTUR_JOBS_THREADS=1)
    find_package(Threads REQUIRED)
    target_link_libraries(StrukturJobs PUBLIC Threads::Threads)
else()
    target_compile_definitions(StrukturJobs PUBLIC STRUKTUR_JOBS_THREADS=0)
endif()
if(MSVC)
    target_compile_options(StrukturJobs PRIVATE /W4)
endif()

if(STRUKTUR_JOBS_BUILD_TESTS)
    include(FetchContent)
    # doctest (MIT) - same pinned release as the fitness tests.
    FetchContent_Declare(doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.12.tar.gz
        URL_HASH SHA256=73381c7aa4dee704bd935609668cf41880ea7f19fa0504a200e13b74999c2d70
        SOURCE_SUBDIR header-only-not-a-cmake-dir
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(doctest)
    add_executable(StrukturJobsTests
        ${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestMain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Tests/JobSystemTests.cpp
    )
    target_include_directories(StrukturJobsTests PRIVATE ${doctest_SOURCE_DIR}/doctest)
    target_link_libraries(StrukturJobsTests PRIVATE StrukturJobs)
    enable_testing()
    add_test(NAME StrukturJobsTests COMMAND StrukturJobsTests)
endif()

if(STRUKTUR_JOBS_BUILD_BENCH)
    add_executable(job_bench ${STRUKTUR_REPO_ROOT}/tools/job_bench/main.cpp)
    target_link_libraries(job_bench PRIVATE StrukturJobs)
endif()
