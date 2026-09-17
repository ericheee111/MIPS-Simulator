# Parser/execution microbenchmark

Measured on the same Debian container with GCC 14.2.0, `-std=c++11 -O2 -DNDEBUG`, without LTO. Each cell is the median of three separate process invocations. The original implementation is commit `74731fa1af9b1997bcbaa3b01eb88fe7d3486677`; both versions use the same `benchmark.cpp`. Raw samples are in `results-2026-09-17.json`.

The input is N-1 decimal `li` instructions and a `j main`. Parse time includes lexing, assembly and obtaining the machine. Execution time measures one million synchronous steps, **excluding** the controller, snapshots and GUI. This intentionally exposes the original repeated suffix scan and is not a representative application benchmark.

| Instructions | Original parse (ms) | Refactored parse (ms) | Original 1M steps (ms) | Refactored 1M steps (ms) |
|---|---:|---:|---:|---:|
| 500 | 84.390 | 0.398 | 48.087 | 10.066 |
| 1000 | 286.843 | 0.818 | 42.538 | 8.740 |
| 2000 | 1363.578 | 3.311 | 50.495 | 8.757 |

The shared, virtualized environment has visible timing variance. These samples demonstrate improvement for this workload; they do not establish a universal speedup or statistical confidence interval. No CI timing threshold is imposed.

```sh
cmake -S . -B build-bench -DBUILD_TESTING=OFF -DMIPS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --parallel 2
./build-bench/mips_benchmark 2000
```

For an exact comparison, compile the benchmark with the recorded flags and each implementation's own parser/lexer/VM sources; do not combine old headers with new implementation files.

## Follow-up controller / observation workload

`controller_benchmark.cpp` uses the same fixed jump loop and three 250 ms trials at
1 KiB, 1 MiB and 16 MiB, with zero/four concurrent observers. The previous runtime
is built from the source bundle commit `8c78610` with `MIPS_BENCH_LEGACY`; the candidate
uses bounded register observations. Both builds use GCC 14.2, C++11 and Release
`-O3 -DNDEBUG` on the same host. Raw CSV and candidate source hashes accompany the
results in `controller-measurements.json`.

This deliberately compares full-snapshot requests with the new lightweight requests:
it measures the relevant API/workflow change, not an isolated scheduler speedup.
Observers only need execution state, so copying all memory was unnecessary work.
No GUI throughput, scheduling percentile, general speedup or long-term guarantee is
inferred from these three samples. The deterministic fairness test, not a timing
threshold, is the acceptance gate. The benchmark's sleep bounds its workload duration;
commands still use futures for completion. Re-run with the same load to compare.
