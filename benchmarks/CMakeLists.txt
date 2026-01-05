file(GLOB BENCHMARK_SOURCES
    benchmarks.cpp
)

add_executable(benchmarks ${BENCHMARK_SOURCES})
target_compile_features(benchmarks PRIVATE cxx_std_20)
target_include_directories(benchmarks PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(benchmarks PRIVATE benchmark::benchmark lutra-ecs)
