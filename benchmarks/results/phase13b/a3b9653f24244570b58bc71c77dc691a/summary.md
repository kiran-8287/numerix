# Phase 13B benchmark summary

Schema version 2 stores every timing batch in raw_timing.csv; accuracy.csv contains separate untimed probes.
Summary values are independently checked against those records by verify_benchmark_results.py.
The 95th percentile uses nearest rank, ceil(0.95*n). All samples remain in the data; no outliers are removed.
Memory columns measure requested heap allocations observed by the instrumented allocator, not process memory or RSS.

| Configuration | Category / case | Size | Method / phase | Library | Mode | Median ns | Min–max ns | Raw batches | Batch size | Status / accuracy | Order |
| --- | --- | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| cfg-1bac02a7c2cc02de | root / polynomial | 0 | bisection / execute | NUMERIX | best_available | 1.6e+03 | 1.54e+03–1.64e+03 | 3 | 2048 | 0 / 1 | GSL_UNAVAILABLE |
| cfg-5b4665767d0edd1b | root / polynomial | 0 | brent / execute | NUMERIX | best_available | 2.68e+03 | 2.61e+03–2.81e+03 | 3 | 1024 | 0 / 1 | GSL_UNAVAILABLE |
| cfg-25e70c7d73eb1bcb | root / polynomial | 0 | newton / execute | NUMERIX | best_available | 474 | 460–514 | 3 | 8192 | 0 / 1 | GSL_UNAVAILABLE |

## Linked records

- workloads.csv carries input parameters, tolerances, reference method, precision, library, and comparison mode for each configuration.
- raw_timing.csv carries one row per measured batch, including the recorded seed and actual paired order.
- accuracy.csv links each timing sample to a separate untimed probe.
- environment.json records the toolchain, build flags, run ID, and protocol.
- comparisons.csv includes direct ratios only for successful rows classified as same-algorithm and passing their accuracy checks.

Requested heap allocation metrics are separately named in measurements.csv. They exclude stack storage, allocator metadata, untracked library allocations, and process RSS.
