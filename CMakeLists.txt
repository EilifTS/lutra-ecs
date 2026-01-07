cmake_minimum_required (VERSION 3.16)
project(lutra-ecs LANGUAGES CXX)

option(LUTRA_ECS_BUILD_BENCHMARKS "Build benchmarks" ON)
option(LUTRA_ECS_BUILD_TESTS "Build tests" ON)

# Define library files
file(GLOB LUTRA_ECS_INCLUDES
    include/lutra-ecs/*.h
)

# Create library
add_library(lutra-ecs INTERFACE ${LUTRA_ECS_INCLUDES})

# Finish up library
target_include_directories(lutra-ecs INTERFACE "include")

# Testing dependencies
include(FetchContent)

if (LUTRA_ECS_BUILD_BENCHMARKS OR LUTRA_ECS_BUILD_TESTS)
    FetchContent_Declare(GTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(GTest)
endif()

if (LUTRA_ECS_BUILD_BENCHMARKS)
    FetchContent_Declare(benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    FetchContent_MakeAvailable(benchmark)
endif()

# Benchmarks
if (LUTRA_ECS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Tests
if (LUTRA_ECS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
