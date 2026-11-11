# Machine-generated Phase 9 summary

Medians are microseconds per API operation; full min/mean/stddev/p95, statuses,
tolerances, reference values and reliability probes are in measurements.csv.
Blank speedups exclude different algorithms/work, failures or failed accuracy checks.
A ratio above1 means NUMERIX was faster in that particular API workload.

## root

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| polynomial | 0 | bisection / execute | NUMERIX | 0.715 | 2.22e-10 | 1.57e-10 | 35 / 0 | 33 | 0 / 1 | 1.02 |
| polynomial | 0 | bisection / execute | GSL | 0.731 | 2.22e-10 | 1.57e-10 | 34 / 0 | 32 | 0 / 1 | — |
| polynomial | 0 | brent / execute | NUMERIX | 1.146 | 7.68e-14 | 5.43e-14 | 37 / 0 | 35 | 0 / 1 | — |
| polynomial | 0 | brent / execute | GSL | 0.238 | 2.22e-16 | 1.57e-16 | 9 / 0 | 7 | 0 / 1 | — |
| polynomial | 0 | newton / execute | NUMERIX | 0.189 | 0 | 0 | 6 / 5 | 5 | 0 / 1 | 0.732 |
| polynomial | 0 | newton / execute | GSL | 0.138 | 0 | 0 | 6 / 6 | 5 | 0 / 1 | — |
| cosine | 0 | bisection / execute | NUMERIX | 1.047 | 8.87e-11 | 1.2e-10 | 35 / 0 | 33 | 0 / 1 | 1.09 |
| cosine | 0 | bisection / execute | GSL | 1.138 | 8.87e-11 | 1.2e-10 | 34 / 0 | 32 | 0 / 1 | — |
| cosine | 0 | brent / execute | NUMERIX | 1.394 | 1.11e-16 | 1.5e-16 | 32 / 0 | 30 | 0 / 1 | — |
| cosine | 0 | brent / execute | GSL | 0.307 | 0 | 0 | 8 / 0 | 6 | 0 / 1 | — |
| cosine | 0 | newton / execute | NUMERIX | 0.262 | 0 | 0 | 5 / 4 | 4 | 0 / 1 | 0.76 |
| cosine | 0 | newton / execute | GSL | 0.199 | 0 | 0 | 5 / 5 | 4 | 0 / 1 | — |
| flat_cubic | 0 | bisection / execute | NUMERIX | 0.732 | 6.98e-11 | 2.33e-10 | 35 / 0 | 33 | 0 / 1 | 0.89 |
| flat_cubic | 0 | bisection / execute | GSL | 0.652 | 6.98e-11 | 2.33e-10 | 34 / 0 | 32 | 0 / 1 | — |
| flat_cubic | 0 | brent / execute | NUMERIX | 1.442 | 5.27e-12 | 1.76e-11 | 40 / 0 | 38 | 0 / 1 | — |
| flat_cubic | 0 | brent / execute | GSL | 2.621 | 4.58e-11 | 1.53e-10 | 89 / 0 | 87 | 0 / 1 | — |
| flat_cubic | 0 | newton / execute | NUMERIX | 1.063 | 1.03e-10 | 3.44e-10 | 56 / 55 | 55 | 0 / 1 | 0.963 |
| flat_cubic | 0 | newton / execute | GSL | 1.023 | 1.03e-10 | 3.44e-10 | 56 / 56 | 55 | 0 / 1 | — |
| small_scale | 0 | bisection / execute | NUMERIX | 0.713 | 2.22e-10 | 1.57e-10 | 35 / 0 | 33 | 0 / 1 | 1.03 |
| small_scale | 0 | bisection / execute | GSL | 0.735 | 2.22e-10 | 1.57e-10 | 34 / 0 | 32 | 0 / 1 | — |
| small_scale | 0 | brent / execute | NUMERIX | 1.183 | 7.68e-14 | 5.43e-14 | 37 / 0 | 35 | 0 / 1 | — |
| small_scale | 0 | brent / execute | GSL | 0.239 | 2.22e-16 | 1.57e-16 | 9 / 0 | 7 | 0 / 1 | — |
| small_scale | 0 | newton / execute | NUMERIX | 0.192 | 0 | 0 | 6 / 5 | 5 | 0 / 1 | 0.845 |
| small_scale | 0 | newton / execute | GSL | 0.163 | 0 | 0 | 6 / 6 | 5 | 0 / 1 | — |
| large_scale | 0 | bisection / execute | NUMERIX | 0.715 | 2.22e-10 | 1.57e-10 | 35 / 0 | 33 | 0 / 1 | 1.03 |
| large_scale | 0 | bisection / execute | GSL | 0.737 | 2.22e-10 | 1.57e-10 | 34 / 0 | 32 | 0 / 1 | — |
| large_scale | 0 | brent / execute | NUMERIX | 1.184 | 7.68e-14 | 5.43e-14 | 37 / 0 | 35 | 0 / 1 | — |
| large_scale | 0 | brent / execute | GSL | 0.245 | 2.22e-16 | 1.57e-16 | 9 / 0 | 7 | 0 / 1 | — |
| large_scale | 0 | newton / execute | NUMERIX | 0.196 | 0 | 0 | 6 / 5 | 5 | 0 / 1 | 0.809 |
| large_scale | 0 | newton / execute | GSL | 0.158 | 0 | 0 | 6 / 6 | 5 | 0 / 1 | — |
| multiple_bracket | 0 | bisection / execute | NUMERIX | 0.146 | 0 | 0 | 3 / 0 | 1 | 0 / 1 | 0.44 |
| multiple_bracket | 0 | bisection / execute | GSL | 0.064 | 0 | 0 | 3 / 0 | 1 | 0 / 1 | — |
| multiple_bracket | 0 | brent / execute | NUMERIX | 1.117 | 2.22e-16 | 1.11e-16 | 35 / 0 | 33 | 0 / 1 | — |
| multiple_bracket | 0 | brent / execute | GSL | 0.247 | 0 | 0 | 9 / 0 | 7 | 0 / 1 | — |
| multiple_bracket | 0 | newton / execute | NUMERIX | 0.203 | 0 | 0 | 6 / 5 | 5 | 0 / 1 | 0.712 |
| multiple_bracket | 0 | newton / execute | GSL | 0.145 | 0 | 0 | 6 / 6 | 5 | 0 / 1 | — |
| near_endpoint | 0 | bisection / execute | NUMERIX | 0.831 | 4.65e-13 | 0.000465 | 42 / 0 | 40 | 0 / 1 | 0.972 |
| near_endpoint | 0 | bisection / execute | GSL | 0.808 | 4.65e-13 | 0.000465 | 41 / 0 | 39 | 0 / 1 | — |
| near_endpoint | 0 | brent / execute | NUMERIX | 1.334 | 2.07e-25 | 2.07e-16 | 43 / 0 | 41 | 0 / 1 | — |
| near_endpoint | 0 | brent / execute | GSL | 0.101 | 0 | 0 | 4 / 0 | 2 | 0 / 1 | — |
| near_endpoint | 0 | newton / execute | NUMERIX | 0.143 | 0 | 0 | 3 / 2 | 2 | 0 / 1 | 0.575 |
| near_endpoint | 0 | newton / execute | GSL | 0.082 | 0 | 0 | 3 / 3 | 2 | 0 / 1 | — |
| no_bracket | 0 | bisection / execute | NUMERIX | 0.137 | — | — | 2 / 0 | 0 | 2 / 0 | — |
| no_bracket | 0 | bisection / execute | GSL | 0.045 | — | — | 2 / 0 | 0 | 4 / 0 | — |
| no_bracket | 0 | brent / execute | NUMERIX | 0.138 | — | — | 2 / 0 | 0 | 2 / 0 | — |
| no_bracket | 0 | brent / execute | GSL | 0.045 | — | — | 2 / 0 | 0 | 4 / 0 | — |

## linear

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| dense_well | 8 | pivoted_lu / factor | NUMERIX | 0.470 | — | — | 0 / 0 | 0 | 0 / -1 | 1.69 |
| dense_well | 8 | pivoted_lu / factor | GSL | 0.792 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 8 | pivoted_lu / solve | NUMERIX | 0.256 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 1.02 |
| dense_well | 8 | pivoted_lu / solve | GSL | 0.263 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 8 | pivoted_lu / total | NUMERIX | 0.644 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 1.58 |
| dense_well | 8 | pivoted_lu / total | GSL | 1.017 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 8 | pivoted_lu / factor | NUMERIX | 0.450 | — | — | 0 / 0 | 0 | 0 / -1 | 1.75 |
| dense_small | 8 | pivoted_lu / factor | GSL | 0.787 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 8 | pivoted_lu / solve | NUMERIX | 0.263 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 0.997 |
| dense_small | 8 | pivoted_lu / solve | GSL | 0.262 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 8 | pivoted_lu / total | NUMERIX | 0.644 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | 1.58 |
| dense_small | 8 | pivoted_lu / total | GSL | 1.017 | 4.44e-16 | 4.44e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 8 | pivoted_lu / factor | NUMERIX | 0.447 | — | — | 0 / 0 | 0 | 0 / -1 | 1.75 |
| dense_large | 8 | pivoted_lu / factor | GSL | 0.781 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 8 | pivoted_lu / solve | NUMERIX | 0.267 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | 1.01 |
| dense_large | 8 | pivoted_lu / solve | GSL | 0.268 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 8 | pivoted_lu / total | NUMERIX | 0.643 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | 1.57 |
| dense_large | 8 | pivoted_lu / total | GSL | 1.012 | 3.33e-16 | 3.33e-16 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 8 | pivoted_lu / factor | NUMERIX | 0.474 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 8 | pivoted_lu / factor | GSL | 0.888 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 8 | pivoted_lu / total | NUMERIX | 0.673 | 4.19e-07 | 4.19e-07 | 0 / 0 | 0 | 0 / 0 | — |
| hilbert | 8 | pivoted_lu / total | GSL | 1.115 | 4.39e-07 | 4.39e-07 | 0 / 0 | 0 | 0 / 0 | — |
| dense_well | 32 | pivoted_lu / factor | NUMERIX | 8.710 | — | — | 0 / 0 | 0 | 0 / -1 | 1.15 |
| dense_well | 32 | pivoted_lu / factor | GSL | 10.010 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 32 | pivoted_lu / solve | NUMERIX | 1.378 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 1.01 |
| dense_well | 32 | pivoted_lu / solve | GSL | 1.391 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 32 | pivoted_lu / total | NUMERIX | 9.941 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 1.13 |
| dense_well | 32 | pivoted_lu / total | GSL | 11.198 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 32 | pivoted_lu / factor | NUMERIX | 8.603 | — | — | 0 / 0 | 0 | 0 / -1 | 1.18 |
| dense_small | 32 | pivoted_lu / factor | GSL | 10.121 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 32 | pivoted_lu / solve | NUMERIX | 1.384 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | 0.985 |
| dense_small | 32 | pivoted_lu / solve | GSL | 1.364 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 32 | pivoted_lu / total | NUMERIX | 9.822 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | 1.15 |
| dense_small | 32 | pivoted_lu / total | GSL | 11.287 | 1.22e-15 | 1.22e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 32 | pivoted_lu / factor | NUMERIX | 8.647 | — | — | 0 / 0 | 0 | 0 / -1 | 1.17 |
| dense_large | 32 | pivoted_lu / factor | GSL | 10.103 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 32 | pivoted_lu / solve | NUMERIX | 1.369 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 0.996 |
| dense_large | 32 | pivoted_lu / solve | GSL | 1.364 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 32 | pivoted_lu / total | NUMERIX | 9.910 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | 1.14 |
| dense_large | 32 | pivoted_lu / total | GSL | 11.285 | 8.88e-16 | 8.88e-16 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 32 | pivoted_lu / factor | NUMERIX | 7.334 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 32 | pivoted_lu / factor | GSL | 11.116 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 32 | pivoted_lu / total | NUMERIX | 7.331 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 32 | pivoted_lu / total | GSL | 12.541 | 347 | 347 | 0 / 0 | 0 | 0 / 0 | — |
| dense_well | 64 | pivoted_lu / factor | NUMERIX | 49.242 | — | — | 0 / 0 | 0 | 0 / -1 | 1.11 |
| dense_well | 64 | pivoted_lu / factor | GSL | 54.668 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 64 | pivoted_lu / solve | NUMERIX | 4.666 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.08 |
| dense_well | 64 | pivoted_lu / solve | GSL | 5.058 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 64 | pivoted_lu / total | NUMERIX | 54.821 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.1 |
| dense_well | 64 | pivoted_lu / total | GSL | 60.227 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 64 | pivoted_lu / factor | NUMERIX | 51.495 | — | — | 0 / 0 | 0 | 0 / -1 | 1.1 |
| dense_small | 64 | pivoted_lu / factor | GSL | 56.808 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 64 | pivoted_lu / solve | NUMERIX | 4.775 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | 0.993 |
| dense_small | 64 | pivoted_lu / solve | GSL | 4.741 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 64 | pivoted_lu / total | NUMERIX | 57.499 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.07 |
| dense_small | 64 | pivoted_lu / total | GSL | 61.692 | 1.44e-15 | 1.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 64 | pivoted_lu / factor | NUMERIX | 51.748 | — | — | 0 / 0 | 0 | 0 / -1 | 1.06 |
| dense_large | 64 | pivoted_lu / factor | GSL | 54.647 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 64 | pivoted_lu / solve | NUMERIX | 4.774 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.01 |
| dense_large | 64 | pivoted_lu / solve | GSL | 4.834 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 64 | pivoted_lu / total | NUMERIX | 54.205 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | 1.11 |
| dense_large | 64 | pivoted_lu / total | GSL | 60.169 | 1.55e-15 | 1.55e-15 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 64 | pivoted_lu / factor | NUMERIX | 27.733 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 64 | pivoted_lu / factor | GSL | 58.645 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 64 | pivoted_lu / total | NUMERIX | 28.311 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 64 | pivoted_lu / total | GSL | 65.203 | 563 | 563 | 0 / 0 | 0 | 0 / 0 | — |
| dense_well | 128 | pivoted_lu / factor | NUMERIX | 325.036 | — | — | 0 / 0 | 0 | 0 / -1 | 1.1 |
| dense_well | 128 | pivoted_lu / factor | GSL | 356.113 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_well | 128 | pivoted_lu / solve | NUMERIX | 18.201 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | 1.02 |
| dense_well | 128 | pivoted_lu / solve | GSL | 18.575 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_well | 128 | pivoted_lu / total | NUMERIX | 344.797 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | 1.05 |
| dense_well | 128 | pivoted_lu / total | GSL | 361.416 | 2e-15 | 2e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 128 | pivoted_lu / factor | NUMERIX | 334.086 | — | — | 0 / 0 | 0 | 0 / -1 | 1.02 |
| dense_small | 128 | pivoted_lu / factor | GSL | 341.009 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_small | 128 | pivoted_lu / solve | NUMERIX | 17.766 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.04 |
| dense_small | 128 | pivoted_lu / solve | GSL | 18.420 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_small | 128 | pivoted_lu / total | NUMERIX | 338.887 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | 1.05 |
| dense_small | 128 | pivoted_lu / total | GSL | 356.912 | 2.44e-15 | 2.44e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 128 | pivoted_lu / factor | NUMERIX | 323.553 | — | — | 0 / 0 | 0 | 0 / -1 | 1.05 |
| dense_large | 128 | pivoted_lu / factor | GSL | 341.220 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| dense_large | 128 | pivoted_lu / solve | NUMERIX | 17.754 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | 1.03 |
| dense_large | 128 | pivoted_lu / solve | GSL | 18.319 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | — |
| dense_large | 128 | pivoted_lu / total | NUMERIX | 343.533 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | 1.06 |
| dense_large | 128 | pivoted_lu / total | GSL | 363.433 | 1.78e-15 | 1.78e-15 | 0 / 0 | 0 | 0 / 1 | — |
| hilbert | 128 | pivoted_lu / factor | NUMERIX | 114.742 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 128 | pivoted_lu / factor | GSL | 358.536 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| hilbert | 128 | pivoted_lu / total | NUMERIX | 113.625 | — | — | 0 / 0 | 0 | 7 / 0 | — |
| hilbert | 128 | pivoted_lu / total | GSL | 382.514 | 1.27e+03 | 1.27e+03 | 0 / 0 | 0 | 0 / 0 | — |

## differentiation

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| quartic_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.047 | 2.01e-13 | 1.46e-13 | 5 / 0 | 0 | 0 / 1 | — |
| quartic_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.119 | 1.97e-11 | 1.44e-11 | 8 / 0 | 0 | 0 / 1 | — |
| sine_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.091 | 1.64e-14 | 2.15e-14 | 5 / 0 | 0 | 0 / 1 | — |
| sine_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.191 | 2.45e-13 | 3.21e-13 | 8 / 0 | 0 | 0 / 1 | — |
| exp_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.095 | 3.03e-13 | 1.51e-13 | 5 / 0 | 0 | 0 / 1 | — |
| exp_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.191 | 5.98e-12 | 2.97e-12 | 8 / 0 | 0 | 0 / 1 | — |
| rational_derivative | 0 | richardson_vs_adaptive / execute | NUMERIX | 0.055 | 1.59e-13 | 2.83e-12 | 5 / 0 | 0 | 0 / 1 | — |
| rational_derivative | 0 | richardson_vs_adaptive / execute | GSL | 0.128 | 1.1e-12 | 1.96e-11 | 8 / 0 | 0 | 0 / 1 | — |

## integration

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 13.622 | 0 | 0 | 473 / 0 | 117 | 0 / 1 | — |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 0.194 | 2.78e-17 | 1.39e-16 | 21 / 0 | 1 | 0 / 1 | — |
| quartic_integral | 1024 | trapezoid / execute | NUMERIX | 10.337 | 3.18e-07 | 1.59e-06 | 1025 / 0 | 1024 | 0 / 0 | — |
| quartic_integral | 1024 | simpson / execute | NUMERIX | 9.268 | 1.21e-13 | 6.06e-13 | 1025 / 0 | 1024 | 0 / 1 | — |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 23.711 | 2.22e-16 | 1.11e-16 | 565 / 0 | 140 | 0 / 1 | — |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 0.317 | 0 | 0 | 21 / 0 | 1 | 0 / 1 | — |
| sine_integral | 1024 | trapezoid / execute | NUMERIX | 19.167 | 1.57e-06 | 7.84e-07 | 1025 / 0 | 1024 | 0 / 0 | — |
| sine_integral | 1024 | simpson / execute | NUMERIX | 19.423 | 9.85e-13 | 4.92e-13 | 1025 / 0 | 1024 | 0 / 1 | — |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 8.854 | 2.22e-16 | 1.29e-16 | 209 / 0 | 51 | 0 / 1 | — |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 0.369 | 0 | 0 | 21 / 0 | 1 | 0 / 1 | — |
| exp_integral | 1024 | trapezoid / execute | NUMERIX | 17.225 | 1.37e-07 | 7.95e-08 | 1025 / 0 | 1024 | 0 / 0 | — |
| exp_integral | 1024 | simpson / execute | NUMERIX | 17.150 | 8.66e-15 | 5.04e-15 | 1025 / 0 | 1024 | 0 / 1 | — |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 26.043 | 2.78e-16 | 1.89e-15 | 809 / 0 | 201 | 0 / 1 | — |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 1.422 | 2.78e-17 | 1.89e-16 | 147 / 0 | 4 | 0 / 1 | — |
| curved_integral | 1024 | trapezoid / execute | NUMERIX | 10.579 | 1.56e-09 | 1.06e-08 | 1025 / 0 | 1024 | 0 / 0 | — |
| curved_integral | 1024 | simpson / execute | NUMERIX | 9.783 | 1.17e-15 | 7.92e-15 | 1025 / 0 | 1024 | 0 / 1 | — |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 26.561 | 4.18e-13 | 6.27e-13 | 829 / 0 | 206 | 0 / 1 | — |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 6.327 | 3.56e-14 | 5.35e-14 | 777 / 0 | 19 | 0 / 1 | — |
| endpoint_sqrt | 1024 | trapezoid / execute | NUMERIX | 10.352 | 6.3e-06 | 9.46e-06 | 1025 / 0 | 1024 | 0 / 0 | — |
| endpoint_sqrt | 1024 | simpson / execute | NUMERIX | 9.544 | 2.48e-06 | 3.72e-06 | 1025 / 0 | 1024 | 0 / 0 | — |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 435.404 | 6.94e-18 | 1.67e-16 | 6729 / 0 | 1681 | 0 / 1 | — |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 2.954 | 4.86e-17 | 1.17e-15 | 147 / 0 | 4 | 0 / 1 | — |
| oscillatory | 1024 | trapezoid / execute | NUMERIX | 22.643 | 5.3e-06 | 0.000127 | 1025 / 0 | 1024 | 0 / 0 | — |
| oscillatory | 1024 | simpson / execute | NUMERIX | 22.445 | 5.39e-10 | 1.29e-08 | 1025 / 0 | 1024 | 0 / 0 | — |

## interpolation

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| uniform_cubic | 8 | newton / construct | NUMERIX | 0.166 | — | — | 0 / 0 | 0 | 0 / -1 | 1.02 |
| uniform_cubic | 8 | newton / construct | GSL | 0.169 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| uniform_cubic | 8 | newton / evaluate | NUMERIX | 0.084 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | 0.585 |
| uniform_cubic | 8 | newton / evaluate | GSL | 0.049 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| uniform_cubic | 8 | lagrange / evaluate | NUMERIX | 0.185 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | — |
| uniform_cubic | 8 | lagrange / evaluate | GSL | 0.049 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| nonuniform_cubic | 8 | newton / construct | NUMERIX | 0.169 | — | — | 0 / 0 | 0 | 0 / -1 | 1.03 |
| nonuniform_cubic | 8 | newton / construct | GSL | 0.174 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| nonuniform_cubic | 8 | newton / evaluate | NUMERIX | 0.085 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | 0.569 |
| nonuniform_cubic | 8 | newton / evaluate | GSL | 0.048 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| nonuniform_cubic | 8 | lagrange / evaluate | NUMERIX | 0.184 | 9.99e-16 | 3.22e-15 | 0 / 0 | 0 | 0 / 1 | — |
| nonuniform_cubic | 8 | lagrange / evaluate | GSL | 0.048 | 1.11e-16 | 3.57e-16 | 0 / 0 | 0 | 0 / 1 | — |
| scaled_cubic | 8 | newton / construct | NUMERIX | 0.166 | — | — | 0 / 0 | 0 | 0 / -1 | 1.04 |
| scaled_cubic | 8 | newton / construct | GSL | 0.173 | — | — | 0 / 0 | 0 | 0 / -1 | — |
| scaled_cubic | 8 | newton / evaluate | NUMERIX | 0.084 | 4.77e-07 | 1.53e-16 | 0 / 0 | 0 | 0 / 1 | 0.578 |
| scaled_cubic | 8 | newton / evaluate | GSL | 0.049 | 4.77e-07 | 1.53e-16 | 0 / 0 | 0 | 0 / 1 | — |
| scaled_cubic | 8 | lagrange / evaluate | NUMERIX | 0.183 | 9.54e-07 | 3.07e-16 | 0 / 0 | 0 | 0 / 1 | — |
| scaled_cubic | 8 | lagrange / evaluate | GSL | 0.049 | 4.77e-07 | 1.53e-16 | 0 / 0 | 0 | 0 / 1 | — |
| linear_segment | 2 | linear / evaluate | NUMERIX | 0.058 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | 0.636 |
| linear_segment | 2 | linear / evaluate | GSL | 0.037 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | — |

## optimization

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| convex_minimum | 0 | golden_section / execute | NUMERIX | 0.323 | 3.44e-08 | 1.15e-07 | 33 / 0 | 31 | 0 / 1 | 2.3 |
| convex_minimum | 0 | golden_section / execute | GSL | 0.744 | 1.76e-08 | 5.85e-08 | 43 / 0 | 40 | 0 / 1 | — |
| nonlinear_minimum | 0 | golden_section / execute | NUMERIX | 0.571 | 3e-08 | — | 33 / 0 | 31 | 0 / 1 | 1.87 |
| nonlinear_minimum | 0 | golden_section / execute | GSL | 1.068 | 4.4e-08 | — | 45 / 0 | 42 | 0 / 1 | — |
| shifted_minimum | 0 | golden_section / execute | NUMERIX | 0.320 | 3.44e-08 | 2.02e-08 | 33 / 0 | 31 | 0 / 1 | 2.31 |
| shifted_minimum | 0 | golden_section / execute | GSL | 0.740 | 1.76e-08 | 1.03e-08 | 43 / 0 | 40 | 0 / 1 | — |
| scaled_minimum | 0 | golden_section / execute | NUMERIX | 0.322 | 3.44e-08 | 1.15e-07 | 33 / 0 | 31 | 0 / 1 | 2.32 |
| scaled_minimum | 0 | golden_section / execute | GSL | 0.746 | 1.76e-08 | 5.85e-08 | 43 / 0 | 40 | 0 / 1 | — |

## ode

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| exponential_growth | 100 | rk4_vs_step_doubling / execute | NUMERIX | 3.537 | 2.25e-10 | 8.26e-11 | 400 / 0 | 100 | 0 / 1 | — |
| exponential_growth | 100 | rk4_vs_step_doubling / execute | GSL | 8.844 | 1.41e-11 | 5.19e-12 | 1100 / 0 | 100 | 0 / 1 | — |
| exponential_growth | 100 | euler / execute | NUMERIX | 0.854 | 0.0135 | 0.00495 | 100 / 0 | 100 | 0 / 1 | — |
| exponential_growth | 100 | heun / execute | NUMERIX | 1.805 | 4.5e-05 | 1.65e-05 | 200 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | rk4_vs_step_doubling / execute | NUMERIX | 3.467 | 3.67e-10 | 2.71e-09 | 400 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | rk4_vs_step_doubling / execute | GSL | 8.991 | 2.27e-11 | 1.68e-10 | 1100 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | euler / execute | NUMERIX | 0.850 | 0.00272 | 0.0201 | 100 / 0 | 100 | 0 / 1 | — |
| exponential_decay | 100 | heun / execute | NUMERIX | 1.809 | 1.83e-05 | 0.000135 | 200 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | rk4_vs_step_doubling / execute | NUMERIX | 4.089 | 6.97e-11 | 1.29e-10 | 400 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | rk4_vs_step_doubling / execute | GSL | 10.637 | 4.37e-12 | 8.09e-12 | 1100 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | euler / execute | NUMERIX | 0.991 | 0.0042 | 0.00777 | 100 / 0 | 100 | 0 / 1 | — |
| harmonic | 100 | heun / execute | NUMERIX | 1.875 | 1.4e-05 | 2.58e-05 | 200 / 0 | 100 | 0 / 1 | — |

## capability

| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| expression_evaluation | 0 | expression / evaluate | NUMERIX | 0.090 | 0 | 0 | 0 / 0 | 0 | 0 / 1 | — |
| automatic_execution | 0 | automatic / execute | NUMERIX | 1.975 | 7.68e-14 | 5.43e-14 | 39 / 0 | 35 | 0 / 1 | — |
| managed_execution | 0 | managed / execute | NUMERIX | 3.007 | 7.68e-14 | 5.43e-14 | 39 / 0 | 35 | 0 / 1 | — |
| diagnostic_execution | 0 | diagnostics / execute | NUMERIX | 2.088 | 7.68e-14 | 5.43e-14 | 40 / 0 | 35 | 0 / 1 | — |
| discovery_one | 16 | discovery / execute | NUMERIX | 7.548 | 0 | 0 | 137 / 0 | 85 | 0 / 1 | — |
| discovery_empty | 16 | discovery / execute | NUMERIX | 1.727 | 0 | — | 33 / 0 | 0 | 0 / 1 | — |
| discovery_three | 16 | discovery / execute | NUMERIX | 14.071 | 0 | 0 | 262 / 0 | 186 | 0 / 1 | — |
| discovery_repeated | 16 | discovery / execute | NUMERIX | 4.518 | 0 | 0 | 101 / 0 | 37 | 0 / 1 | — |
| discovery_one | 32 | discovery / execute | NUMERIX | 6.702 | 0 | 0 | 135 / 0 | 58 | 0 / 1 | — |
| discovery_empty | 32 | discovery / execute | NUMERIX | 3.075 | 0 | — | 65 / 0 | 0 | 0 / 1 | — |
| discovery_three | 32 | discovery / execute | NUMERIX | 17.904 | 0 | 0 | 324 / 0 | 209 | 0 / 1 | — |
| discovery_repeated | 32 | discovery / execute | NUMERIX | 6.687 | 0 | 0 | 152 / 0 | 53 | 0 / 1 | — |
| discovery_one | 64 | discovery / execute | NUMERIX | 9.414 | 0 | 0 | 197 / 0 | 56 | 0 / 1 | — |
| discovery_empty | 64 | discovery / execute | NUMERIX | 5.936 | 0 | — | 129 / 0 | 0 | 0 / 1 | — |
| discovery_three | 64 | discovery / execute | NUMERIX | 18.150 | 0 | 0 | 354 / 0 | 182 | 0 / 1 | — |
| discovery_repeated | 64 | discovery / execute | NUMERIX | 9.283 | 0 | 0 | 213 / 0 | 50 | 0 / 1 | — |

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
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 3 | 21504 | 12288 | 0 | 0 |
| quartic_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| quartic_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| quartic_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 4 | 46080 | 24576 | 0 | 0 |
| sine_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| sine_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| sine_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 2 | 9216 | 6144 | 0 | 0 |
| exp_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| exp_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| exp_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 4 | 46080 | 24576 | 0 | 0 |
| curved_integral | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| curved_integral | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| curved_integral | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 4 | 46080 | 24576 | 0 | 0 |
| endpoint_sqrt | 1024 | adaptive_simpson_vs_qag21 / execute | GSL | 7 | 480088 | 480088 | 480088 | 0 |
| endpoint_sqrt | 1024 | trapezoid / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| endpoint_sqrt | 1024 | simpson / execute | NUMERIX | 0 | 0 | 0 | 0 | 0 |
| oscillatory | 1024 | adaptive_simpson_vs_qag21 / execute | NUMERIX | 7 | 390144 | 196608 | 0 | 0 |
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
