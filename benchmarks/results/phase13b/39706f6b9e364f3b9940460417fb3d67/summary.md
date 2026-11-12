# Phase 13B benchmark summary

Schema version 2 stores every timing batch in raw_timing.csv; accuracy.csv contains separate untimed probes.
Summary values are independently checked against those records by verify_benchmark_results.py.
The 95th percentile uses nearest rank, ceil(0.95*n). All samples remain in the data; no outliers are removed.
Memory columns measure requested heap allocations observed by the instrumented allocator, not process memory or RSS.

| Configuration | Category / case | Size | Method / phase | Library | Mode | Median ns | Min–max ns | Raw batches | Batch size | Status / accuracy | Order |
| --- | --- | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| cfg-1bac02a7c2cc02de | root / polynomial | 0 | bisection / execute | NUMERIX | best_available | 747 | 732–748 | 3 | 4096 | 0 / 1 | NUMERIX_FIRST |
| cfg-1bac02a7c2cc02de | root / polynomial | 0 | bisection / execute | GSL | best_available | 861 | 856–904 | 3 | 4096 | 0 / 1 | NUMERIX_FIRST |
| cfg-5b4665767d0edd1b | root / polynomial | 0 | brent / execute | GSL | best_available | 271 | 263–296 | 3 | 8192 | 0 / 1 | GSL_FIRST |
| cfg-5b4665767d0edd1b | root / polynomial | 0 | brent / execute | NUMERIX | best_available | 1.15e+03 | 1.15e+03–1.23e+03 | 3 | 2048 | 0 / 1 | GSL_FIRST |
| cfg-25e70c7d73eb1bcb | root / polynomial | 0 | newton / execute | GSL | best_available | 195 | 177–215 | 3 | 16384 | 0 / 1 | GSL_FIRST |
| cfg-25e70c7d73eb1bcb | root / polynomial | 0 | newton / execute | NUMERIX | best_available | 224 | 216–232 | 3 | 16384 | 0 / 1 | GSL_FIRST |

## Linked records

- workloads.csv carries input parameters, tolerances, reference method, precision, library, and comparison mode for each configuration.
- raw_timing.csv carries one row per measured batch, including the recorded seed and actual paired order.
- accuracy.csv links each timing sample to a separate untimed probe.
- environment.json records the toolchain, build flags, run ID, and protocol.
- comparisons.csv includes direct ratios only for successful rows classified as same-algorithm and passing their accuracy checks.

Requested heap allocation metrics are separately named in measurements.csv. They exclude stack storage, allocator metadata, untracked library allocations, and process RSS.
