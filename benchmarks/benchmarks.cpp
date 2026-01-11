#include <benchmark/benchmark.h>

#include "BECS.h"

BENCHMARK(ECSIterationSTD);
BENCHMARK(ECSIterationNormal)->Args({ 100 });
BENCHMARK(ECSIterationChunked)->Args({ 100 });
BENCHMARK(ECSIterationNormal)->Args({ 90 });
BENCHMARK(ECSIterationChunked)->Args({ 90 });
BENCHMARK(ECSIterationNormal)->Args({ 70 });
BENCHMARK(ECSIterationChunked)->Args({ 70 });
BENCHMARK(ECSIterationNormal)->Args({ 50 });
BENCHMARK(ECSIterationChunked)->Args({ 50 });
BENCHMARK(ECSIterationNormal)->Args({ 30 });
BENCHMARK(ECSIterationChunked)->Args({ 30 });
BENCHMARK(ECSIterationNormal)->Args({ 10 });
BENCHMARK(ECSIterationChunked)->Args({ 10 });
BENCHMARK(ECSIterationNormal)->Args({ 1 });
BENCHMARK(ECSIterationChunked)->Args({ 1 });
BENCHMARK(ECSIterationRndNormal)->Args({ 100 });
BENCHMARK(ECSIterationRndChunked)->Args({ 100 });
BENCHMARK(ECSIterationRndNormal)->Args({ 90 });
BENCHMARK(ECSIterationRndChunked)->Args({ 90 });
BENCHMARK(ECSIterationRndNormal)->Args({ 70 });
BENCHMARK(ECSIterationRndChunked)->Args({ 70 });
BENCHMARK(ECSIterationRndNormal)->Args({ 50 });
BENCHMARK(ECSIterationRndChunked)->Args({ 50 });
BENCHMARK(ECSIterationRndNormal)->Args({ 30 });
BENCHMARK(ECSIterationRndChunked)->Args({ 30 });
BENCHMARK(ECSIterationRndNormal)->Args({ 10 });
BENCHMARK(ECSIterationRndChunked)->Args({ 10 });
BENCHMARK(ECSIterationRndNormal)->Args({ 1 });
BENCHMARK(ECSIterationRndChunked)->Args({ 1 });

BENCHMARK_MAIN();
