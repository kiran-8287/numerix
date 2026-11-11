# Machine-generated Phase 9 summary

Medians are microseconds per API operation; full min/mean/stddev/p95, statuses,
tolerances, reference values and reliability probes are in measurements.csv.
Blank speedups exclude different algorithms/work, failures or failed accuracy checks.
A ratio above1 means NUMERIX was faster in that particular API workload.

## root

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| polynomial | 0 | bisection / execute | NUMERIX | 0.300 | 2.22e-10 | 1.57e-10 | 35 / 0 | 33 | 0 / 1 | 2.12 |
| polynomial | 0 | bisection / execute | GSL | 0.636 | 2.22e-10 | 1.57e-10 | 34 / 0 | 32 | 0 / 1 | — |
| polynomial | 0 | brent / execute | NUMERIX | 0.380 | 7.68e-14 | 5.43e-14 | 18 / 0 | 16 | 0 / 1 | — |
| polynomial | 0 | brent / execute | GSL | 0.200 | 4.13e-14 | 2.92e-14 | 8 / 0 | 6 | 0 / 1 | — |
| polynomial | 0 | newton / execute | NUMERIX | 0.097 | 1.59e-12 | 1.13e-12 | 5 / 4 | 4 | 0 / 1 | 1.13 |
| polynomial | 0 | newton / execute | GSL | 0.110 | 1.59e-12 | 1.13e-12 | 5 / 5 | 4 | 0 / 1 | — |
| cosine | 0 | bisection / execute | NUMERIX | 0.922 | 8.87e-11 | 1.2e-10 | 35 / 0 | 33 | 0 / 1 | 1.08 |
| cosine | 0 | bisection / execute | GSL | 0.998 | 8.87e-11 | 1.2e-10 | 34 / 0 | 32 | 0 / 1 | — |
| cosine | 0 | brent / execute | NUMERIX | 0.899 | 1.11e-16 | 1.5e-16 | 22 / 0 | 20 | 0 / 1 | — |
| cosine | 0 | brent / execute | GSL | 0.254 | 4.77e-15 | 6.46e-15 | 7 / 0 | 5 | 0 / 1 | — |
| cosine | 0 | newton / execute | NUMERIX | 0.185 | 0 | 0 | 5 / 4 | 4 | 0 / 1 | 1.08 |
| cosine | 0 | newton / execute | GSL | 0.200 | 0 | 0 | 5 / 5 | 4 | 0 / 1 | — |
| flat_cubic | 0 | bisection / execute | NUMERIX | 0.299 | 6.98e-11 | 2.33e-10 | 35 / 0 | 33 | 0 / 1 | 1.86 |
| flat_cubic | 0 | bisection / execute | GSL | 0.555 | 6.98e-11 | 2.33e-10 | 34 / 0 | 32 | 0 / 1 | — |
| flat_cubic | 0 | brent / execute | NUMERIX | 0.333 | 0.000413 | 0.00138 | 13 / 0 | 11 | 0 / 0 | — |
| flat_cubic | 0 | brent / execute | GSL | 0.744 | 0.000265 | 0.000883 | 27 / 0 | 25 | 0 / 0 | — |
| flat_cubic | 0 | newton / execute | NUMERIX | 0.308 | 0.000338 | 0.00113 | 19 / 18 | 18 | 0 / 0 | — |
| flat_cubic | 0 | newton / execute | GSL | 0.325 | 0.000338 | 0.00113 | 19 / 19 | 18 | 0 / 0 | — |
| small_scale | 0 | bisection / execute | NUMERIX | 0.298 | 2.22e-10 | 1.57e-10 | 35 / 0 | 33 | 0 / 1 | 2.21 |
| small_scale | 0 | bisection / execute | GSL | 0.657 | 2.22e-10 | 1.57e-10 | 34 / 0 | 32 | 0 / 1 | — |
| small_scale | 0 | brent / execute | NUMERIX | 0.090 | 0.414 | 0.293 | 3 / 0 | 1 | 0 / 0 | — |
| small_scale | 0 | brent / execute | GSL | 0.083 | 0.414 | 0.293 | 3 / 0 | 1 | 0 / 0 | — |
| small_scale | 0 | newton / execute | NUMERIX | 0.050 | 0.414 | 0.293 | 1 / 0 | 0 | 0 / 0 | — |
| small_scale | 0 | newton / execute | GSL | 0.058 | 0.414 | 0.293 | 1 / 1 | 0 | 0 / 0 | — |
| large_scale | 0 | bisection / execute | NUMERIX | 0.310 | 2.22e-10 | 1.57e-10 | 35 / 0 | 33 | 0 / 1 | 2.09 |
| large_scale | 0 | bisection / execute | GSL | 0.648 | 2.22e-10 | 1.57e-10 | 34 / 0 | 32 | 0 / 1 | — |
| large_scale | 0 | brent / execute | NUMERIX | 0.691 | 7.68e-14 | 5.43e-14 | 36 / 0 | 34 | 0 / 1 | — |
| large_scale | 0 | brent / execute | GSL | 0.253 | 2.22e-16 | 1.57e-16 | 9 / 0 | 7 | 0 / 1 | — |
| large_scale | 0 | newton / execute | NUMERIX | 0.112 | 0 | 0 | 6 / 5 | 5 | 0 / 1 | 1.14 |
| large_scale | 0 | newton / execute | GSL | 0.128 | 0 | 0 | 6 / 6 | 5 | 0 / 1 | — |
| multiple_bracket | 0 | bisection / execute | NUMERIX | 0.079 | 0 | 0 | 3 / 0 | 1 | 0 / 1 | 0.98 |
| multiple_bracket | 0 | bisection / execute | GSL | 0.077 | 0 | 0 | 3 / 0 | 1 | 0 / 1 | — |
| multiple_bracket | 0 | brent / execute | NUMERIX | 0.425 | 2.22e-16 | 1.11e-16 | 20 / 0 | 18 | 0 / 1 | — |
| multiple_bracket | 0 | brent / execute | GSL | 0.216 | 4.42e-12 | 2.21e-12 | 8 / 0 | 6 | 0 / 1 | — |
| multiple_bracket | 0 | newton / execute | NUMERIX | 0.111 | 2.09e-14 | 1.04e-14 | 5 / 4 | 4 | 0 / 1 | 1.12 |
| multiple_bracket | 0 | newton / execute | GSL | 0.125 | 2.09e-14 | 1.04e-14 | 5 / 5 | 4 | 0 / 1 | — |
| near_endpoint | 0 | bisection / execute | NUMERIX | 0.326 | 4.65e-13 | 0.000465 | 42 / 0 | 40 | 0 / 1 | 2.12 |
| near_endpoint | 0 | bisection / execute | GSL | 0.692 | 4.65e-13 | 0.000465 | 41 / 0 | 39 | 0 / 1 | — |
| near_endpoint | 0 | brent / execute | NUMERIX | 0.547 | 2.07e-25 | 2.07e-16 | 30 / 0 | 28 | 0 / 1 | — |
| near_endpoint | 0 | brent / execute | GSL | 0.089 | 2.07e-25 | 2.07e-16 | 3 / 0 | 1 | 0 / 1 | — |
| near_endpoint | 0 | newton / execute | NUMERIX | 0.061 | 2.72e-17 | 2.72e-08 | 2 / 1 | 1 | 0 / 1 | 1.18 |
| near_endpoint | 0 | newton / execute | GSL | 0.071 | 2.72e-17 | 2.72e-08 | 2 / 2 | 1 | 0 / 1 | — |
| no_bracket | 0 | bisection / execute | NUMERIX | 0.134 | — | — | 2 / 0 | 0 | 2 / 0 | — |
| no_bracket | 0 | bisection / execute | GSL | 0.058 | — | — | 2 / 0 | 0 | 4 / 0 | — |
| no_bracket | 0 | brent / execute | NUMERIX | 0.134 | — | — | 2 / 0 | 0 | 2 / 0 | — |
| no_bracket | 0 | brent / execute | GSL | 0.066 | — | — | 2 / 0 | 0 | 4 / 0 | — |

## linear

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| dense_well | 8 | pivoted_lu / factor | NUMERIX | 0.485 | — | — | 0 / 0 | 0 | 0 / -1 | 1.67 |
| dense_well | 8 | pivoted_lu / factor | GSL | 0.810 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 8 | pivoted_lu / solve | NUMERIX | 0.270 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 0.968 |
| dense_well | 8 | pivoted_lu / solve | GSL | 0.262 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 8 | pivoted_lu / total | NUMERIX | 0.666 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 1.52 |
| dense_well | 8 | pivoted_lu / total | GSL | 1.013 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 8 | pivoted_lu / factor | NUMERIX | 0.446 | — | — | 0 / 0 | 0 | 0 / -1 | 1.77 |
| dense_small | 8 | pivoted_lu / factor | GSL | 0.789 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 8 | pivoted_lu / solve | NUMERIX | 0.246 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 1.03 |
| dense_small | 8 | pivoted_lu / solve | GSL | 0.254 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 8 | pivoted_lu / total | NUMERIX | 0.640 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 1.55 |
| dense_small | 8 | pivoted_lu / total | GSL | 0.994 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 8 | pivoted_lu / factor | NUMERIX | 0.439 | — | — | 0 / 0 | 0 | 0 / -1 | 1.76 |
| dense_large | 8 | pivoted_lu / factor | GSL | 0.773 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 8 | pivoted_lu / solve | NUMERIX | 0.259 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | 0.992 |
| dense_large | 8 | pivoted_lu / solve | GSL | 0.257 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 8 | pivoted_lu / total | NUMERIX | 0.629 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | 1.56 |
| dense_large | 8 | pivoted_lu / total | GSL | 0.980 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 8 | pivoted_lu / factor | NUMERIX | 0.477 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 8 | pivoted_lu / factor | GSL | 0.895 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 8 | pivoted_lu / total | NUMERIX | 0.675 | 4.19e-07 | 4.19e-07 | 0 / 0 | 0 | 0 / 0 | — |
| hilbert | 8 | pivoted_lu / total | GSL | 1.137 | 4.39e-07 | 4.39e-07 | 0 / 0 | 0 | 0 / 0 | — |
| dense_well | 32 | pivoted_lu / factor | NUMERIX | 8.355 | — | — | 0 / 0 | 0 | 0 / -1 | 1.22 |
| dense_well | 32 | pivoted_lu / factor | GSL | 10.162 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 32 | pivoted_lu / solve | NUMERIX | 1.328 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 0.987 |
| dense_well | 32 | pivoted_lu / solve | GSL | 1.311 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 32 | pivoted_lu / total | NUMERIX | 9.667 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 1.19 |
| dense_well | 32 | pivoted_lu / total | GSL | 11.484 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 32 | pivoted_lu / factor | NUMERIX | 8.205 | — | — | 0 / 0 | 0 | 0 / -1 | 1.26 |
| dense_small | 32 | pivoted_lu / factor | GSL | 10.351 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 32 | pivoted_lu / solve | NUMERIX | 1.329 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | 1 |
| dense_small | 32 | pivoted_lu / solve | GSL | 1.334 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 32 | pivoted_lu / total | NUMERIX | 9.528 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | 1.2 |
| dense_small | 32 | pivoted_lu / total | GSL | 11.453 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 32 | pivoted_lu / factor | NUMERIX | 8.053 | — | — | 0 / 0 | 0 | 0 / -1 | 1.29 |
| dense_large | 32 | pivoted_lu / factor | GSL | 10.357 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 32 | pivoted_lu / solve | NUMERIX | 1.328 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 1 |
| dense_large | 32 | pivoted_lu / solve | GSL | 1.329 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 32 | pivoted_lu / total | NUMERIX | 9.432 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 1.25 |
| dense_large | 32 | pivoted_lu / total | GSL | 11.789 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 32 | pivoted_lu / factor | NUMERIX | 6.977 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 32 | pivoted_lu / factor | GSL | 11.273 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 32 | pivoted_lu / total | NUMERIX | 6.961 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 32 | pivoted_lu / total | GSL | 12.664 | 347 | 347 | 0 / 0 | 0 | 0 / 0 | — |
| dense_well | 64 | pivoted_lu / factor | NUMERIX | 47.515 | — | — | 0 / 0 | 0 | 0 / -1 | 1.17 |
| dense_well | 64 | pivoted_lu / factor | GSL | 55.828 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 64 | pivoted_lu / solve | NUMERIX | 4.528 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.02 |
| dense_well | 64 | pivoted_lu / solve | GSL | 4.628 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 64 | pivoted_lu / total | NUMERIX | 51.729 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.17 |
| dense_well | 64 | pivoted_lu / total | GSL | 60.361 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 64 | pivoted_lu / factor | NUMERIX | 49.925 | — | — | 0 / 0 | 0 | 0 / -1 | 1.13 |
| dense_small | 64 | pivoted_lu / factor | GSL | 56.413 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 64 | pivoted_lu / solve | NUMERIX | 4.474 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.02 |
| dense_small | 64 | pivoted_lu / solve | GSL | 4.561 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 64 | pivoted_lu / total | NUMERIX | 51.645 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.17 |
| dense_small | 64 | pivoted_lu / total | GSL | 60.408 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 64 | pivoted_lu / factor | NUMERIX | 47.445 | — | — | 0 / 0 | 0 | 0 / -1 | 1.18 |
| dense_large | 64 | pivoted_lu / factor | GSL | 55.783 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 64 | pivoted_lu / solve | NUMERIX | 4.526 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1 |
| dense_large | 64 | pivoted_lu / solve | GSL | 4.533 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 64 | pivoted_lu / total | NUMERIX | 52.763 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.22 |
| dense_large | 64 | pivoted_lu / total | GSL | 64.456 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 64 | pivoted_lu / factor | NUMERIX | 28.939 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 64 | pivoted_lu / factor | GSL | 61.399 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 64 | pivoted_lu / total | NUMERIX | 29.301 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 64 | pivoted_lu / total | GSL | 69.837 | 563 | 563 | 0 / 0 | 0 | 0 / 0 | — |
| dense_well | 128 | pivoted_lu / factor | NUMERIX | 317.241 | — | — | 0 / 0 | 0 | 0 / -1 | 1.13 |
| dense_well | 128 | pivoted_lu / factor | GSL | 357.800 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 128 | pivoted_lu / solve | NUMERIX | 17.199 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | 1.05 |
| dense_well | 128 | pivoted_lu / solve | GSL | 17.977 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 128 | pivoted_lu / total | NUMERIX | 329.554 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | 1.14 |
| dense_well | 128 | pivoted_lu / total | GSL | 374.394 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 128 | pivoted_lu / factor | NUMERIX | 307.832 | — | — | 0 / 0 | 0 | 0 / -1 | 1.16 |
| dense_small | 128 | pivoted_lu / factor | GSL | 357.224 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 128 | pivoted_lu / solve | NUMERIX | 17.303 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.07 |
| dense_small | 128 | pivoted_lu / solve | GSL | 18.482 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 128 | pivoted_lu / total | NUMERIX | 339.771 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.12 |
| dense_small | 128 | pivoted_lu / total | GSL | 380.592 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 128 | pivoted_lu / factor | NUMERIX | 310.653 | — | — | 0 / 0 | 0 | 0 / -1 | 1.15 |
| dense_large | 128 | pivoted_lu / factor | GSL | 358.525 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 128 | pivoted_lu / solve | NUMERIX | 17.932 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | 0.987 |
| dense_large | 128 | pivoted_lu / solve | GSL | 17.695 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 128 | pivoted_lu / total | NUMERIX | 328.843 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | 1.15 |
| dense_large | 128 | pivoted_lu / total | GSL | 378.162 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 128 | pivoted_lu / factor | NUMERIX | 108.211 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 128 | pivoted_lu / factor | GSL | 372.423 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 128 | pivoted_lu / total | NUMERIX | 108.973 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 128 | pivoted_lu / total | GSL | 382.641 | 1.27e+03 | 1.27e+03 | 0 / 0 | 0 | 0 / 0 | — |

## differentiation

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| quartic_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.048 | 2.01e-13 | 1.46e-13 | 5 / 0 | 0 | 0 / 1 | — |
| quartic_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.114 | 1.97e-11 | 1.44e-11 | 8 / 0 | 0 | 0 / 1 | — |
| sine_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.090 | 1.64e-14 | 2.15e-14 | 5 / 0 | 0 | 0 / 1 | — |
| sine_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.184 | 2.45e-13 | 3.21e-13 | 8 / 0 | 0 | 0 / 1 | — |
| exp_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.091 | 3.03e-13 | 1.51e-13 | 5 / 0 | 0 | 0 / 1 | — |
| exp_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.188 | 5.98e-12 | 2.97e-12 | 8 / 0 | 0 | 0 / 1 | — |
| rational_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.054 | 1.59e-13 | 2.83e-12 | 5 / 0 | 0 | 0 / 1 | — |
| rational_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.125 | 1.1e-12 | 1.96e-11 | 8 / 0 | 0 | 0 / 1 | — |

## integration

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 2.711 | 2.78e-17 | 1.39e-16 | 257 / 0 | 127 | 0 / 1 | — |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 0.189 | 2.78e-17 | 1.39e-16 | 21 / 0 | 1 | 0 / 1 | — |
| quartic_integral | 1024 | trapezoid / execute | NUMERIX | 6.292 | 3.18e-07 | 1.59e-06 | 1025 / 0 | 1024 | 0 / 0 | — |
| quartic_integral | 1024 | simpson / execute | NUMERIX | 6.447 | 1.21e-13 | 6.06e-13 | 1025 / 0 | 1024 | 0 / 1 | — |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 7.923 | 5.55e-15 | 2.78e-15 | 369 / 0 | 183 | 0 / 1 | — |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 0.309 | 0 | 0 | 21 / 0 | 1 | 0 / 1 | — |
| sine_integral | 1024 | trapezoid / execute | NUMERIX | 14.878 | 1.57e-06 | 7.84e-07 | 1025 / 0 | 1024 | 0 / 0 | — |
| sine_integral | 1024 | simpson / execute | NUMERIX | 15.667 | 9.85e-13 | 4.92e-13 | 1025 / 0 | 1024 | 0 / 1 | — |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 2.721 | 6.66e-16 | 3.88e-16 | 129 / 0 | 63 | 0 / 1 | — |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 0.361 | 0 | 0 | 21 / 0 | 1 | 0 / 1 | — |
| exp_integral | 1024 | trapezoid / execute | NUMERIX | 13.456 | 1.37e-07 | 7.95e-08 | 1025 / 0 | 1024 | 0 / 0 | — |
| exp_integral | 1024 | simpson / execute | NUMERIX | 13.998 | 8.44e-15 | 4.91e-15 | 1025 / 0 | 1024 | 0 / 1 | — |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 6.710 | 1.38e-14 | 9.38e-14 | 573 / 0 | 285 | 0 / 1 | — |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 1.365 | 2.78e-17 | 1.89e-16 | 147 / 0 | 4 | 0 / 1 | — |
| curved_integral | 1024 | trapezoid / execute | NUMERIX | 6.352 | 1.56e-09 | 1.06e-08 | 1025 / 0 | 1024 | 0 / 0 | — |
| curved_integral | 1024 | simpson / execute | NUMERIX | 6.484 | 1.17e-15 | 7.92e-15 | 1025 / 0 | 1024 | 0 / 1 | — |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0.833 | — | — | 67 / 0 | 32 | 6 / 0 | — |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 6.123 | 3.56e-14 | 5.35e-14 | 777 / 0 | 19 | 0 / 1 | — |
| endpoint_sqrt | 1024 | trapezoid / execute | NUMERIX | 6.367 | 6.3e-06 | 9.46e-06 | 1025 / 0 | 1024 | 0 / 0 | — |
| endpoint_sqrt | 1024 | simpson / execute | NUMERIX | 6.345 | 2.48e-06 | 3.72e-06 | 1025 / 0 | 1024 | 0 / 0 | — |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 93.189 | 6.59e-16 | 1.58e-14 | 3817 / 0 | 1907 | 0 / 1 | — |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 2.797 | 4.86e-17 | 1.17e-15 | 147 / 0 | 4 | 0 / 1 | — |
| oscillatory | 1024 | trapezoid / execute | NUMERIX | 20.858 | 5.3e-06 | 0.000127 | 1025 / 0 | 1024 | 0 / 0 | — |
| oscillatory | 1024 | simpson / execute | NUMERIX | 21.177 | 5.39e-10 | 1.29e-08 | 1025 / 0 | 1024 | 0 / 0 | — |

## interpolation

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| uniform_cubic | 8 | newton / construct | NUMERIX | 0.171 | — | — | 0 / 0 | 0 | 0 / -1 | 1.02 |
| uniform_cubic | 8 | newton / construct | GSL | 0.174 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| uniform_cubic | 8 | newton / evaluate | NUMERIX | 0.086 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | 0.587 |
| uniform_cubic | 8 | newton / evaluate | GSL | 0.051 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| uniform_cubic | 8 | lagrange / evaluate | NUMERIX | 0.180 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | — |
| uniform_cubic | 8 | lagrange / evaluate | GSL | 0.048 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| nonuniform_cubic | 8 | newton / construct | NUMERIX | 0.166 | — | — | 0 / 0 | 0 | 0 / -1 | 1.01 |
| nonuniform_cubic | 8 | newton / construct | GSL | 0.167 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| nonuniform_cubic | 8 | newton / evaluate | NUMERIX | 0.083 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | 0.567 |
| nonuniform_cubic | 8 | newton / evaluate | GSL | 0.047 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| nonuniform_cubic | 8 | lagrange / evaluate | NUMERIX | 0.176 | 9.99e-16 | 3.22e-15 | 0 / 0 | 0 | 0 / 1 | — |
| nonuniform_cubic | 8 | lagrange / evaluate | GSL | 0.048 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| scaled_cubic | 8 | newton / construct | NUMERIX | 0.162 | — | — | 0 / 0 | 0 | 0 / -1 | 1.05 |
| scaled_cubic | 8 | newton / construct | GSL | 0.171 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| scaled_cubic | 8 | newton / evaluate | NUMERIX | 0.083 | 4.77e-07 | 1.53e-16 | 0 / 0 | 0 | 0 / 1 | 0.572 |
| scaled_cubic | 8 | newton / evaluate | GSL | 0.048 | 4.77e-07 | 1.53e-16 | 0 / 0 | 0 | 0 / 1 | — |
| scaled_cubic | 8 | lagrange / evaluate | NUMERIX | 0.177 | 9.54e-07 | 3.07e-16 | 0 / 0 | 0 | 0 / 1 | — |
| scaled_cubic | 8 | lagrange / evaluate | GSL | 0.048 | 4.77e-07 | 1.53e-16 | 0 / 0 | 0 | 0 / 1 | — |
| linear_segment | 2 | linear / evaluate | NUMERIX | 0.058 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | 0.645 |
| linear_segment | 2 | linear / evaluate | GSL | 0.037 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | — |

## optimization

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| convex_minimum | 0 | golden_section / execute | NUMERIX | 0.335 | 3.44e-08 | 1.15e-07 | 33 / 0 | 31 | 0 / 1 | 2.23 |
| convex_minimum | 0 | golden_section / execute | GSL | 0.746 | 1.76e-08 | 5.85e-08 | 43 / 0 | 40 | 0 / 1 | — |
| nonlinear_minimum | 0 | golden_section / execute | NUMERIX | 0.568 | 3e-08 | — | 33 / 0 | 31 | 0 / 1 | 1.89 |
| nonlinear_minimum | 0 | golden_section / execute | GSL | 1.075 | 4.4e-08 | — | 45 / 0 | 42 | 0 / 1 | — |
| shifted_minimum | 0 | golden_section / execute | NUMERIX | 0.334 | 3.44e-08 | 2.02e-08 | 33 / 0 | 31 | 0 / 1 | 2.16 |
| shifted_minimum | 0 | golden_section / execute | GSL | 0.721 | 1.76e-08 | 1.03e-08 | 43 / 0 | 40 | 0 / 1 | — |
| scaled_minimum | 0 | golden_section / execute | NUMERIX | 0.333 | 3.44e-08 | 1.15e-07 | 33 / 0 | 31 | 0 / 1 | 2.19 |
| scaled_minimum | 0 | golden_section / execute | GSL | 0.730 | 1.76e-08 | 5.85e-08 | 43 / 0 | 40 | 0 / 1 | — |

## ode

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| exponential_growth | 100 | rk4_vs_step_doubling / execute | NUMERIX | 3.591 | 2.25e-10 | 8.26e-11 | 400 / 0 | 100 | 0 / 1 | — |
| exponential_growth | 100 | rk4_vs_step_doubling / execute | GSL | 9.291 | 1.41e-11 | 5.19e-12 | 1100 / 0 | 100 | 0 / 1 | — |
| exponential_growth | 100 | euler / execute | NUMERIX | 0.824 | 0.0135 | 0.00495 | 100 / 0 | 100 | 0 / 1 | — |
| exponential_growth | 100 | heun / execute | NUMERIX | 1.731 | 4.5e-05 | 1.65e-05 | 200 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | rk4_vs_step_doubling / execute | NUMERIX | 3.472 | 3.67e-10 | 2.71e-09 | 400 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | rk4_vs_step_doubling / execute | GSL | 8.946 | 2.27e-11 | 1.68e-10 | 1100 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | euler / execute | NUMERIX | 0.817 | 0.00272 | 0.0201 | 100 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | heun / execute | NUMERIX | 1.723 | 1.83e-05 | 0.000135 | 200 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | rk4_vs_step_doubling / execute | NUMERIX | 3.829 | 6.97e-11 | 1.29e-10 | 400 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | rk4_vs_step_doubling / execute | GSL | 10.509 | 4.37e-12 | 8.09e-12 | 1100 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | euler / execute | NUMERIX | 1.284 | 0.0042 | 0.00777 | 100 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | heun / execute | NUMERIX | 1.898 | 1.4e-05 | 2.58e-05 | 200 / 0 | 100 | 0 / 1 | — |

## capability

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| expression_evaluation | 0 | expression / evaluate | NUMERIX | 0.086 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | — |
| automatic_execution | 0 | automatic / execute | NUMERIX | 1.105 | 7.68e-14 | 5.43e-14 | 20 / 0 | 16 | 0 / 1 | — |
| managed_execution | 0 | managed / execute | NUMERIX | 1.810 | 7.68e-14 | 5.43e-14 | 20 / 0 | 16 | 0 / 1 | — |
| diagnostic_execution | 0 | diagnostics / execute | NUMERIX | 1.222 | 7.68e-14 | 5.43e-14 | 21 / 0 | 16 | 0 / 1 | — |
| discovery_one | 16 | discovery / execute | NUMERIX | 5.646 | 0 | 0 | 113 / 0 | 61 | 0 / 1 | — |
| discovery_empty | 16 | discovery / execute | NUMERIX | 1.689 | 0 | — | 33 / 0 | 0 | 0 / 1 | — |
| discovery_three | 16 | discovery / execute | NUMERIX | 12.430 | 0 | 0 | 252 / 0 | 176 | 0 / 1 | — |
| discovery_repeated | 16 | discovery / execute | NUMERIX | 4.620 | 0 | 0 | 101 / 0 | 37 | 0 / 1 | — |
| discovery_one | 32 | discovery / execute | NUMERIX | 5.795 | 0 | 0 | 124 / 0 | 47 | 0 / 1 | — |
| discovery_empty | 32 | discovery / execute | NUMERIX | 3.051 | 0 | — | 65 / 0 | 0 | 0 / 1 | — |
| discovery_three | 32 | discovery / execute | NUMERIX | 15.976 | 0 | 0 | 317 / 0 | 202 | 0 / 1 | — |
| discovery_repeated | 32 | discovery / execute | NUMERIX | 6.437 | 0 | 0 | 152 / 0 | 53 | 0 / 1 | — |
| discovery_one | 64 | discovery / execute | NUMERIX | 9.117 | 0 | 0 | 196 / 0 | 55 | 0 / 1 | — |
| discovery_empty | 64 | discovery / execute | NUMERIX | 5.832 | 0 | — | 129 / 0 | 0 | 0 / 1 | — |
| discovery_three | 64 | discovery / execute | NUMERIX | 16.773 | 0 | 0 | 353 / 0 | 181 | 0 / 1 | — |
| discovery_repeated | 64 | discovery / execute | NUMERIX | 9.241 | 0 | 0 | 213 / 0 | 50 | 0 / 1 | — |

## Allocation scope: setup + one operation + destruction

Requested heap bytes, including nested linked-object allocations; inputs, stack,
allocator metadata and process RSS are excluded. All figures are measured,
not formulas. Setup bytes were live just before the operation.

| Case | n | Method / phase | Library | allocations | cumulative B | peak live B | setup B | after destroy B |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |
| polynomial | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| polynomial | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| polynomial | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| polynomial | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| polynomial | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| polynomial | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| cosine | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| cosine | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| cosine | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| cosine | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| cosine | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| cosine | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| flat_cubic | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| flat_cubic | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| flat_cubic | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| flat_cubic | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| flat_cubic | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| flat_cubic | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| small_scale | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| small_scale | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| small_scale | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| small_scale | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| small_scale | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| small_scale | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| large_scale | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| large_scale | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| large_scale | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| large_scale | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| large_scale | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| large_scale | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| multiple_bracket | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| multiple_bracket | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| multiple_bracket | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| multiple_bracket | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| multiple_bracket | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| multiple_bracket | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| near_endpoint | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| near_endpoint | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| near_endpoint | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| near_endpoint | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| near_endpoint | 0 | newton / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| near_endpoint | 0 | newton / execute | GSL | 2 | 48 | 48 | 48 | 0 |
| no_bracket | 0 | bisection / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| no_bracket | 0 | bisection / execute | GSL | 2 | 64 | 64 | 64 | 0 |
| no_bracket | 0 | brent / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| no_bracket | 0 | brent / execute | GSL | 2 | 112 | 112 | 112 | 0 |
| dense_well | 8 | pivoted_lu / factor | NUMERIX | 2 | 576 | 576 | 0 | 0 |
| dense_well | 8 | pivoted_lu / factor | GSL | 8 | 744 | 744 | 0 | 0 |
| dense_well | 8 | pivoted_lu / solve | NUMERIX | 4 | 656 | 656 | 576 | 0 |
| dense_well | 8 | pivoted_lu / solve | GSL | 11 | 864 | 776 | 656 | 0 |
| dense_well | 8 | pivoted_lu / total | NUMERIX | 4 | 656 | 656 | 0 | 0 |
| dense_well | 8 | pivoted_lu / total | GSL | 11 | 864 | 776 | 0 | 0 |
| dense_small | 8 | pivoted_lu / factor | NUMERIX | 2 | 576 | 576 | 0 | 0 |
| dense_small | 8 | pivoted_lu / factor | GSL | 8 | 744 | 744 | 0 | 0 |
| dense_small | 8 | pivoted_lu / solve | NUMERIX | 4 | 656 | 656 | 576 | 0 |
| dense_small | 8 | pivoted_lu / solve | GSL | 11 | 864 | 776 | 656 | 0 |
| dense_small | 8 | pivoted_lu / total | NUMERIX | 4 | 656 | 656 | 0 | 0 |
| dense_small | 8 | pivoted_lu / total | GSL | 11 | 864 | 776 | 0 | 0 |
| dense_large | 8 | pivoted_lu / factor | NUMERIX | 2 | 576 | 576 | 0 | 0 |
| dense_large | 8 | pivoted_lu / factor | GSL | 8 | 744 | 744 | 0 | 0 |
| dense_large | 8 | pivoted_lu / solve | NUMERIX | 4 | 656 | 656 | 576 | 0 |
| dense_large | 8 | pivoted_lu / solve | GSL | 11 | 864 | 776 | 656 | 0 |
| dense_large | 8 | pivoted_lu / total | NUMERIX | 4 | 656 | 656 | 0 | 0 |
| dense_large | 8 | pivoted_lu / total | GSL | 11 | 864 | 776 | 0 | 0 |
| hilbert | 8 | pivoted_lu / factor | NUMERIX | 2 | 576 | 576 | 0 | 0 |
| hilbert | 8 | pivoted_lu / factor | GSL | 8 | 744 | 744 | 0 | 0 |
| hilbert | 8 | pivoted_lu / total | NUMERIX | 4 | 656 | 656 | 0 | 0 |
| hilbert | 8 | pivoted_lu / total | GSL | 11 | 864 | 776 | 0 | 0 |
| dense_well | 32 | pivoted_lu / factor | NUMERIX | 2 | 8448 | 8448 | 0 | 0 |
| dense_well | 32 | pivoted_lu / factor | GSL | 8 | 8712 | 8712 | 0 | 0 |
| dense_well | 32 | pivoted_lu / solve | NUMERIX | 4 | 8720 | 8720 | 8448 | 0 |
| dense_well | 32 | pivoted_lu / solve | GSL | 11 | 9024 | 8840 | 8528 | 0 |
| dense_well | 32 | pivoted_lu / total | NUMERIX | 4 | 8720 | 8720 | 0 | 0 |
| dense_well | 32 | pivoted_lu / total | GSL | 11 | 9024 | 8840 | 0 | 0 |
| dense_small | 32 | pivoted_lu / factor | NUMERIX | 2 | 8448 | 8448 | 0 | 0 |
| dense_small | 32 | pivoted_lu / factor | GSL | 8 | 8712 | 8712 | 0 | 0 |
| dense_small | 32 | pivoted_lu / solve | NUMERIX | 4 | 8720 | 8720 | 8448 | 0 |
| dense_small | 32 | pivoted_lu / solve | GSL | 11 | 9024 | 8840 | 8528 | 0 |
| dense_small | 32 | pivoted_lu / total | NUMERIX | 4 | 8720 | 8720 | 0 | 0 |
| dense_small | 32 | pivoted_lu / total | GSL | 11 | 9024 | 8840 | 0 | 0 |
| dense_large | 32 | pivoted_lu / factor | NUMERIX | 2 | 8448 | 8448 | 0 | 0 |
| dense_large | 32 | pivoted_lu / factor | GSL | 8 | 8712 | 8712 | 0 | 0 |
| dense_large | 32 | pivoted_lu / solve | NUMERIX | 4 | 8720 | 8720 | 8448 | 0 |
| dense_large | 32 | pivoted_lu / solve | GSL | 11 | 9024 | 8840 | 8528 | 0 |
| dense_large | 32 | pivoted_lu / total | NUMERIX | 4 | 8720 | 8720 | 0 | 0 |
| dense_large | 32 | pivoted_lu / total | GSL | 11 | 9024 | 8840 | 0 | 0 |
| hilbert | 32 | pivoted_lu / factor | NUMERIX | 2 | 8448 | 8448 | 0 | 0 |
| hilbert | 32 | pivoted_lu / factor | GSL | 8 | 8712 | 8712 | 0 | 0 |
| hilbert | 32 | pivoted_lu / total | NUMERIX | 2 | 8448 | 8448 | 0 | 0 |
| hilbert | 32 | pivoted_lu / total | GSL | 11 | 9024 | 8840 | 0 | 0 |
| dense_well | 64 | pivoted_lu / factor | NUMERIX | 2 | 33280 | 33280 | 0 | 0 |
| dense_well | 64 | pivoted_lu / factor | GSL | 8 | 33672 | 33672 | 0 | 0 |
| dense_well | 64 | pivoted_lu / solve | NUMERIX | 4 | 33808 | 33808 | 33280 | 0 |
| dense_well | 64 | pivoted_lu / solve | GSL | 11 | 34240 | 33928 | 33360 | 0 |
| dense_well | 64 | pivoted_lu / total | NUMERIX | 4 | 33808 | 33808 | 0 | 0 |
| dense_well | 64 | pivoted_lu / total | GSL | 11 | 34240 | 33928 | 0 | 0 |
| dense_small | 64 | pivoted_lu / factor | NUMERIX | 2 | 33280 | 33280 | 0 | 0 |
| dense_small | 64 | pivoted_lu / factor | GSL | 8 | 33672 | 33672 | 0 | 0 |
| dense_small | 64 | pivoted_lu / solve | NUMERIX | 4 | 33808 | 33808 | 33280 | 0 |
| dense_small | 64 | pivoted_lu / solve | GSL | 11 | 34240 | 33928 | 33360 | 0 |
| dense_small | 64 | pivoted_lu / total | NUMERIX | 4 | 33808 | 33808 | 0 | 0 |
| dense_small | 64 | pivoted_lu / total | GSL | 11 | 34240 | 33928 | 0 | 0 |
| dense_large | 64 | pivoted_lu / factor | NUMERIX | 2 | 33280 | 33280 | 0 | 0 |
| dense_large | 64 | pivoted_lu / factor | GSL | 8 | 33672 | 33672 | 0 | 0 |
| dense_large | 64 | pivoted_lu / solve | NUMERIX | 4 | 33808 | 33808 | 33280 | 0 |
| dense_large | 64 | pivoted_lu / solve | GSL | 11 | 34240 | 33928 | 33360 | 0 |
| dense_large | 64 | pivoted_lu / total | NUMERIX | 4 | 33808 | 33808 | 0 | 0 |
| dense_large | 64 | pivoted_lu / total | GSL | 11 | 34240 | 33928 | 0 | 0 |
| hilbert | 64 | pivoted_lu / factor | NUMERIX | 2 | 33280 | 33280 | 0 | 0 |
| hilbert | 64 | pivoted_lu / factor | GSL | 8 | 33672 | 33672 | 0 | 0 |
| hilbert | 64 | pivoted_lu / total | NUMERIX | 2 | 33280 | 33280 | 0 | 0 |
| hilbert | 64 | pivoted_lu / total | GSL | 11 | 34240 | 33928 | 0 | 0 |
| dense_well | 128 | pivoted_lu / factor | NUMERIX | 2 | 132096 | 132096 | 0 | 0 |
| dense_well | 128 | pivoted_lu / factor | GSL | 8 | 132744 | 132744 | 0 | 0 |
| dense_well | 128 | pivoted_lu / solve | NUMERIX | 4 | 133136 | 133136 | 132096 | 0 |
| dense_well | 128 | pivoted_lu / solve | GSL | 11 | 133824 | 133256 | 132176 | 0 |
| dense_well | 128 | pivoted_lu / total | NUMERIX | 4 | 133136 | 133136 | 0 | 0 |
| dense_well | 128 | pivoted_lu / total | GSL | 11 | 133824 | 133256 | 0 | 0 |
| dense_small | 128 | pivoted_lu / factor | NUMERIX | 2 | 132096 | 132096 | 0 | 0 |
| dense_small | 128 | pivoted_lu / factor | GSL | 8 | 132744 | 132744 | 0 | 0 |
| dense_small | 128 | pivoted_lu / solve | NUMERIX | 4 | 133136 | 133136 | 132096 | 0 |
| dense_small | 128 | pivoted_lu / solve | GSL | 11 | 133824 | 133256 | 132176 | 0 |
| dense_small | 128 | pivoted_lu / total | NUMERIX | 4 | 133136 | 133136 | 0 | 0 |
| dense_small | 128 | pivoted_lu / total | GSL | 11 | 133824 | 133256 | 0 | 0 |
| dense_large | 128 | pivoted_lu / factor | NUMERIX | 2 | 132096 | 132096 | 0 | 0 |
| dense_large | 128 | pivoted_lu / factor | GSL | 8 | 132744 | 132744 | 0 | 0 |
| dense_large | 128 | pivoted_lu / solve | NUMERIX | 4 | 133136 | 133136 | 132096 | 0 |
| dense_large | 128 | pivoted_lu / solve | GSL | 11 | 133824 | 133256 | 132176 | 0 |
| dense_large | 128 | pivoted_lu / total | NUMERIX | 4 | 133136 | 133136 | 0 | 0 |
| dense_large | 128 | pivoted_lu / total | GSL | 11 | 133824 | 133256 | 0 | 0 |
| hilbert | 128 | pivoted_lu / factor | NUMERIX | 2 | 132096 | 132096 | 0 | 0 |
| hilbert | 128 | pivoted_lu / factor | GSL | 8 | 132744 | 132744 | 0 | 0 |
| hilbert | 128 | pivoted_lu / total | NUMERIX | 2 | 132096 | 132096 | 0 | 0 |
| hilbert | 128 | pivoted_lu / total | GSL | 11 | 133824 | 133256 | 0 | 0 |
| quartic_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| quartic_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0 | 0 | 0 | 0 | 0 |
| sine_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| sine_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0 | 0 | 0 | 0 | 0 |
| exp_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| exp_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0 | 0 | 0 | 0 | 0 |
| rational_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| rational_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0 | 0 | 0 | 0 | 0 |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| quartic_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| quartic_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| sine_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| sine_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| exp_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| exp_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| curved_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| curved_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| endpoint_sqrt | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| endpoint_sqrt | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| oscillatory | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| oscillatory | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| uniform_cubic | 8 | newton / construct | NUMERIX | 2 | 128 | 128 | 0 | 0 |
| uniform_cubic | 8 | newton / construct | GSL | 5 | 256 | 256 | 0 | 0 |
| uniform_cubic | 8 | newton / evaluate | NUMERIX | 2 | 128 | 128 | 128 | 0 |
| uniform_cubic | 8 | newton / evaluate | GSL | 5 | 256 | 256 | 256 | 0 |
| uniform_cubic | 8 | lagrange / evaluate | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| uniform_cubic | 8 | lagrange / evaluate | GSL | 5 | 256 | 256 | 256 | 0 |
| nonuniform_cubic | 8 | newton / construct | NUMERIX | 2 | 128 | 128 | 0 | 0 |
| nonuniform_cubic | 8 | newton / construct | GSL | 5 | 256 | 256 | 0 | 0 |
| nonuniform_cubic | 8 | newton / evaluate | NUMERIX | 2 | 128 | 128 | 128 | 0 |
| nonuniform_cubic | 8 | newton / evaluate | GSL | 5 | 256 | 256 | 256 | 0 |
| nonuniform_cubic | 8 | lagrange / evaluate | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| nonuniform_cubic | 8 | lagrange / evaluate | GSL | 5 | 256 | 256 | 256 | 0 |
| scaled_cubic | 8 | newton / construct | NUMERIX | 2 | 128 | 128 | 0 | 0 |
| scaled_cubic | 8 | newton / construct | GSL | 5 | 256 | 256 | 0 | 0 |
| scaled_cubic | 8 | newton / evaluate | NUMERIX | 2 | 128 | 128 | 128 | 0 |
| scaled_cubic | 8 | newton / evaluate | GSL | 5 | 256 | 256 | 256 | 0 |
| scaled_cubic | 8 | lagrange / evaluate | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| scaled_cubic | 8 | lagrange / evaluate | GSL | 5 | 256 | 256 | 256 | 0 |
| linear_segment | 2 | linear / evaluate | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| linear_segment | 2 | linear / evaluate | GSL | 1 | 40 | 40 | 40 | 0 |
| convex_minimum | 0 | golden_section / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| convex_minimum | 0 | golden_section / execute | GSL | 2 | 80 | 80 | 80 | 0 |
| nonlinear_minimum | 0 | golden_section / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| nonlinear_minimum | 0 | golden_section / execute | GSL | 2 | 80 | 80 | 80 | 0 |
| shifted_minimum | 0 | golden_section / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| shifted_minimum | 0 | golden_section / execute | GSL | 2 | 80 | 80 | 80 | 0 |
| scaled_minimum | 0 | golden_section / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| scaled_minimum | 0 | golden_section / execute | GSL | 2 | 80 | 80 | 80 | 0 |
| exponential_growth | 100 | rk4_vs_step_doubling / execute | NUMERIX | 7 | 1656 | 1656 | 0 | 0 |
| exponential_growth | 100 | rk4_vs_step_doubling / execute | GSL | 9 | 1720 | 1720 | 104 | 0 |
| exponential_growth | 100 | euler / execute | NUMERIX | 4 | 1632 | 1632 | 0 | 0 |
| exponential_growth | 100 | heun / execute | NUMERIX | 5 | 1640 | 1640 | 0 | 0 |
| exponential_decay | 100 | rk4_vs_step_doubling / execute | NUMERIX | 7 | 1656 | 1656 | 0 | 0 |
| exponential_decay | 100 | rk4_vs_step_doubling / execute | GSL | 9 | 1720 | 1720 | 104 | 0 |
| exponential_decay | 100 | euler / execute | NUMERIX | 4 | 1632 | 1632 | 0 | 0 |
| exponential_decay | 100 | heun / execute | NUMERIX | 5 | 1640 | 1640 | 0 | 0 |
| harmonic | 100 | rk4_vs_step_doubling / execute | NUMERIX | 7 | 2504 | 2504 | 0 | 0 |
| harmonic | 100 | rk4_vs_step_doubling / execute | GSL | 9 | 2568 | 2568 | 144 | 0 |
| harmonic | 100 | euler / execute | NUMERIX | 4 | 2456 | 2456 | 0 | 0 |
| harmonic | 100 | heun / execute | NUMERIX | 5 | 2472 | 2472 | 0 | 0 |
| expression_evaluation | 0 | expression / evaluate | NUMERIX | 8 | 648 | 640 | 296 | 0 |
| automatic_execution | 0 | automatic / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| managed_execution | 0 | managed / execute | NUMERIX | 2 | 10616 | 10616 | 0 | 0 |
| diagnostic_execution | 0 | diagnostics / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| discovery_one | 16 | discovery / execute | NUMERIX | 4 | 31048 | 31048 | 0 | 0 |
| discovery_empty | 16 | discovery / execute | NUMERIX | 3 | 904 | 904 | 0 | 0 |
| discovery_three | 16 | discovery / execute | NUMERIX | 4 | 31048 | 31048 | 0 | 0 |
| discovery_repeated | 16 | discovery / execute | NUMERIX | 5 | 31144 | 31144 | 0 | 0 |
| discovery_one | 32 | discovery / execute | NUMERIX | 4 | 31560 | 31560 | 0 | 0 |
| discovery_empty | 32 | discovery / execute | NUMERIX | 3 | 1416 | 1416 | 0 | 0 |
| discovery_three | 32 | discovery / execute | NUMERIX | 5 | 31656 | 31656 | 0 | 0 |
| discovery_repeated | 32 | discovery / execute | NUMERIX | 5 | 31656 | 31656 | 0 | 0 |
| discovery_one | 64 | discovery / execute | NUMERIX | 4 | 32584 | 32584 | 0 | 0 |
| discovery_empty | 64 | discovery / execute | NUMERIX | 3 | 2440 | 2440 | 0 | 0 |
| discovery_three | 64 | discovery / execute | NUMERIX | 5 | 32680 | 32680 | 0 | 0 |
| discovery_repeated | 64 | discovery / execute | NUMERIX | 5 | 32680 | 32680 | 0 | 0 |
