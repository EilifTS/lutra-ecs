#include <benchmark/benchmark.h>

#include "BECS.h"

BENCHMARK(BenchmarkECS)->RangeMultiplier(2)->Range(8, 1 << 16);
BENCHMARK(BenchmarkECSSTD)->RangeMultiplier(2)->Range(8, 1 << 16);

BENCHMARK_MAIN();
