# NUMERIX benchmark protocol

Phase 9 and Phase 10 outputs are historical records and retain their original
CSV schema. Phase 13B outputs are also retained unchanged. New Phase 13C
comparison runs use schema version 2 and are written to `results/phase13c/`.
`scripts/run_benchmarks.py` creates a
unique run ID by default, refuses non-empty output directories, and refuses
paths under the Phase 9 or Phase 10 historical directories.

GSL is an **optional benchmark dependency**. The library, CLI, examples and tests
build without it. No external library sources or binaries are vendored.
See the [Phase 13C/13D comparative evaluation](../documentation/reports/NUMERIX_comparative_evaluation.md)
and the saved [Phase 13C raw results](results/phase13c/). Historical Phase 9
results remain available under `results/phase9/`.

## Build and run

```sh
cmake -S . -B cmake-build-bench -DCMAKE_BUILD_TYPE=Release \
  -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON
cmake --build cmake-build-bench -j2
./cmake-build-bench/numerix_comparison_benchmark --versions
python3 scripts/run_benchmarks.py --build-dir cmake-build-bench \
  --samples 21 --seed 20261112 --compiler gcc \
  --gsl-flags='record the actual GSL build flags here'
```

The run directory contains `measurements.csv` (summary), `raw_timing.csv`
(one row per timing batch), `accuracy.csv` (one untimed result probe associated
with each batch), `workloads.csv` (stable configuration identity and reference
metadata), `comparisons.csv`, `environment.json`, and a generated `summary.md`.
The driver independently verifies the raw timing records and summaries before
writing the comparison and summary reports. Use `--output PATH` to choose an
empty directory outside the historical Phase 9/10 trees.

Install GSL with your package manager, or build its official release locally:

```sh
curl -fLO https://ftp.gnu.org/gnu/gsl/gsl-2.8.tar.gz
tar -xzf gsl-2.8.tar.gz
cd gsl-2.8
./configure --prefix="$PWD/../gsl-local" CC=gcc \
  CFLAGS='-O3 -DNDEBUG' --disable-shared
make -j2
make install
cd ..
# Add -DGSL_ROOT_DIR=/absolute/path/to/gsl-local to the CMake command.
```

The recorded run used locally built static GSL 2.8 and its reference CBLAS,
with GCC 8.5.0 `-O3 -DNDEBUG`, matching NUMERIX Release optimization. Record
distribution compiler/BLAS differences if using packaged GSL. This suite does
not compare against optimized vendor BLAS or claim a comparison with LAPACK.
GSL's licensing applies to executables linked with GSL; it is not a dependency
of the MIT-licensed NUMERIX archive.

With Make:

```sh
make clean
make CFLAGS='-O3 -DNDEBUG' numerix_comparison_benchmark
make benchmark-comparison
# Local install, if needed:
# make GSL_CFLAGS=-I/path/include GSL_LIBS='/path/lib/libgsl.a /path/lib/libgslcblas.a -lm' numerix_comparison_benchmark
```

Rebuild after changing compiler, optimization or GSL flags. Make does not track
flag changes. CMake explicitly reports when GSL is unavailable; `--versions`
also reports it and only NUMERIX rows are produced. A deliberately dependency-free
configuration can use `-DCMAKE_DISABLE_FIND_PACKAGE_GSL=TRUE`.

## Historical Phase 9/10 measurement contract

- The historical Phase 9/10 runs used binary64 (`DBL_MANT_DIG=53`), analytical
  references rounded to double, identical inputs, and no fast-math or
  architecture-specific flags.
- Level 1 is basic, 2 intermediate, 3 difficult and 4 stress. Each CSV row
  records its level, size, methods, tolerances and library status. A status of
  zero is that library's success; other integers belong to that library's enum.
- Input generation and persistent library workspace creation are outside the
  execution timer. Required per-call allocations, validation, resets, data
  copies and destruction remain inside it. LU and interpolation have separate
  factor/solve and construct/evaluate rows. These are **API costs**, including
  different error checking and representation overhead, not pure arithmetic
  kernels. GSL root/minimizer `set` is timed because its endpoint evaluations
  correspond to NUMERIX's in-call initialization; workspace allocation is not.
- There is warm-up followed by powers-of-two batching until at least 2 ms
  (capped at 1,048,576 operations). There are 21 measured batches by default.
  CSV min/median/mean/sample-standard-deviation/p95 describe **batch means per
  operation**, in ns. The p95 is not a per-request latency percentile. Calibration,
  reporting, accuracy calculations and input generation are outside timed batches.
- One independent accuracy probe follows each timed batch. `trials`,
  `successes` and `accuracy_successes` count these deterministic probes. They
  measure reproducibility on these inputs, not probabilistic reliability on
  unseen problems. Failed numerical calls are timed and retained in the CSV.
- Callback counters count actual callback invocations, with derivatives separate.
  GSL Newton evaluates a derivative at initialization and at the final iterate;
  NUMERIX counts derivatives in its public combined counter, so the suite uses
  its own wrappers for both. The GSL driver reads a most-recent callback value
  to test residuals without re-evaluating it; if unavailable it calls the counted
  callback. Bisection uses bracket width alone and GSL's collapsed exact-root
  bracket. GSL may report the midpoint of an updated bracket before evaluating
  it, producing a one-iteration difference from NUMERIX.
- Root stopping uses absolute function residual where the NUMERIX method does,
  and/or `abs_tol + rel_tol*abs(x)` for position. These have different units.
  Small function scales and flat roots deliberately expose this limitation.
  Absolute **position error**, not solver status, determines root accuracy.
  Bisection and Newton are equivalent-family comparisons, not bit-identical
  implementations; Brent has different safeguards and is unranked.
- `comparisons.csv` emits `T_GSL/T_NUMERIX` only for successful comparable
  pairs meeting the accuracy check. Factor and construct rows have accuracy
  marked -1 (not applicable); their companion solves/evaluations establish
  correctness. Hilbert rows and different-work pairs have blank speedup fields.
  No aggregate winner or cross-family geometric mean is computed. Values near
  one need to be interpreted against timing spread and machine noise.
- The process is single-threaded. CPU frequency, scheduling, cache state and
  other machine activity are uncontrolled. Environment JSON captures hardware,
  affinity, OS, RAM, compiler versions, exact build files, GSL build flags, source
  hashes and git base/dirty status. Paths in that historical record describe the
  measurement environment and are not required reproduction paths.

## Phase 13B measurement protocol

Schema version 2 records actual floating-point properties per workload:
`storage_bits = sizeof(double) * CHAR_BIT`, `significand_bits = DBL_MANT_DIG`,
`min_exponent = DBL_MIN_EXP`, and `max_exponent = DBL_MAX_EXP`. It does not
assume every supported C platform has binary64.

The stable configuration ID hashes a canonical serialization of category,
case and variant, difficulty, dimensions, input parameters, scale, expected
value, reference-generation method, tolerances, method, phase, and comparison
mode. The same problem, numerical target, and method share an ID across
libraries. The manifest stores the fields alongside the ID so results can be
audited without reversing the hash.

Two comparison modes are distinct:

- `same_algorithm` is reserved for rows classified as API-equivalent in this
  suite. It requires the same mathematical inputs and target, equivalent
  algorithms, and matched precision and stopping settings. Differences in
  callback budgets or setup boundaries must be disclosed before ranking.
- `best_available` compares suitable methods for the same problem and target;
  it measures practical library performance, not the cost of one algorithm.
  Capability-only and excluded/unranked cases have their own labels.

Timing uses monotonic wall-clock time (`CLOCK_MONOTONIC` on POSIX and
`QueryPerformanceCounter` on Windows); CPU time is not collected. Timer
granularity depends on the operating system and host timer implementation; the
runner records the timer API but does not query or claim a measured resolution.
Each library/workload receives one untimed warm-up, then powers-of-two batches
are calibrated to at least 2 ms (capped at 1,048,576 operations) to reduce
timer quantization effects. The default is 21 measured batches. Each batch is
preserved with elapsed batch time, nanoseconds per operation, batch size,
status, precision, seed, and linked accuracy record. A separate deterministic
result probe follows each batch; accuracy probes are not timing repetitions.

Paired library execution uses a reproducible counterbalance schedule. Adjacent
pairs run one NUMERIX-first and one GSL-first; the recorded 64-bit seed selects
the orientation of each block. Every paired raw observation records its actual
order and seed. A library-filtered or NUMERIX-only run is marked unpaired.
Calibration and warm-up are applied separately to both libraries with the same
policy. No outlier is silently removed.

Summary statistics are recalculated from the raw batch records: minimum,
median (middle value, or average of the middle pair), arithmetic mean, sample
standard deviation, nearest-rank p95 (`ceil(0.95*n)`), maximum, raw sample
count, and batch size. The verifier independently recalculates all statistics
and checks missing/duplicate samples, accuracy linkage, workload identity,
batch consistency, execution order, and precision metadata.

The timer encloses the selected API operation and its in-call validation,
resets, required allocations, data copies, and output handling; input generation
and persistent workspace creation remain outside the timer where the API
supports separation. Setup and per-operation costs are reported as separate
phases when present. These are API-level costs, not isolated arithmetic-kernel
times.

Memory figures from the optional allocator wrapper count requested bytes seen
through wrapped `malloc`, `calloc`, `realloc`, and `free` calls. They exclude
stack storage, allocator metadata, unwrapped allocations, and process memory.
RSS or private-memory measurements are whole-process metrics and must be
recorded separately; they are not interchangeable with requested heap bytes.
Cross-platform memory comparability has not been established.

## Test definitions and references

Common root tolerance is `1e-10 + 1e-10*abs(x)`; the endpoint case uses absolute
`1e-12`, relative zero. Root and minimization budgets are 1,000 iterations in both drivers. NUMERIX
also caps callback evaluations at 1,000,000; GSL has no equivalent native cap in
this harness. None of the valid root cases reaches either budget. Difficult invalid cases must return failure, not a root.

| Root case | Mathematical problem and bracket | Newton initial | Reference | Level |
| --- | --- | ---: | ---: | ---: |
| polynomial | `x^2-2`, [0,2] | 1 | sqrt(2) | 1 |
| cosine | `cos(x)-x`, [0,1] | 0.5 | 0.7390851332151606417 | 2 |
| flat_cubic | `(x-0.3)^3`, [0,1] | 0.8 | 0.3 | 3 |
| small_scale | `1e-14*(x^2-2)`, [0,2] | 1 | sqrt(2) | 4 |
| large_scale | `1e14*(x^2-2)`, [0,2] | 1 | sqrt(2) | 4 |
| multiple_bracket | `(x-1)*(x-2)*(x-4)`, [1.5,2.5] | 1.8 | 2 | 2 |
| near_endpoint | `x-1e-9`, [0,1] | 0.5 | 1e-9 | 3 |
| no_bracket | `x^2+1`, [-1,1]; bracketing methods only | n/a | no real root | 3 |

| Category | Inputs, methods and reference | Accuracy check / expected behavior |
| --- | --- | --- |
| Dense LU | n=8,32,64,128; partial pivoting; `A_ii=2s`, `A_ij=0.25s/(i+j+1)` for zero-based i,j; `s=1,1e-12,1e12`; `b_i=sum_j A_ij`; known x=all ones | max solution error <=1e-8; residual infinity norm accumulated in long double outside timing; normwise backward error `norm(r)/(norm(A)*norm(x)+norm(b))`; input b is rounded by double summation |
| Hilbert LU | same n; `A_ij=1/(i+j+1)`; same b and known solution | Stress level 4; numerical rejection and large forward errors are retained; no performance ranking. Rounded b perturbs the exact all-ones problem |
| Differentiation | x=0.7; initial h=1e-3; `x^4`, sin(x), exp(x), `1/(1+100x^2)`; derivatives `4x^3`, cos(x), exp(x), `-200x/(1+100x^2)^2` | error <=1e-7*max(1,abs(reference)); NUMERIX fixed-h Richardson versus GSL adaptive central; unranked |
| Integration | `x^4` on [0,1] ->1/5; sin(x) on [0,pi] ->2; exp(x) on [0,1] ->e-1; `1/(1+100x^2)` on [0,1] ->atan(10)/10; sqrt(x) on [0,1] ->2/3; sin(40x) on [0,1] ->(1-cos(40))/40 | `1e-10+1e-10*abs(reference)` is the common *accuracy check*. NUMERIX adaptive Simpson (depth 32, sum-based initial tolerance) versus GSL QAG21 (`max(abs,rel*abs(I))`); different methods/stopping, unranked. Fixed trapezoid/Simpson use 1024 panels and have no tolerance guarantee. Actual and estimated errors are separate |
| Interpolation | 8 nodes on [0,1], x_i=i/7 or (i/7)^2; y_i=s*(x_i^3-2x_i+1), s=1 or1e10; query0.37; reference s*0.310653 | `1e-10+1e-10*abs(reference)`; Newton vs GSL divided differences, separate setup/evaluation. Lagrange vs GSL polynomial has different representation and is unranked. Two-node linear segment (0,1),(1,0) ->0.63 has no NUMERIX construction object |
| Golden section | `(x-0.3)^2` on [0,2], `exp(x)-x` on [-1,2] (min0), `(x-1.7)^2` on [0,2], `1e12*(x-0.3)^2` on [0,2] | absolute interval tolerance1e-6, relative0; position error <=1e-6; GSL starting point0.5 (1.5 for shifted), NUMERIX starts from interval only; equivalent family with different initialization; final objective recorded |
| ODE | y'=lambda*y, y(0)=1 at several rates and step counts; harmonic (u'=v,v'=-u),(1,0); t in[0,1]; exact exponential or harmonic trajectories | Final infinity error and maximum trajectory error on returned time grid; method-specific tolerances scale with the reference magnitude. GSL `gsl_odeiv2_step_apply` uses fixed-step classical RK4 on the same grid as NUMERIX RK4; both perform four RHS evaluations per step. This is an API-level same-algorithm comparison. GSL adapter stores the full trajectory to match NUMERIX output |

GSL offers neither the same fixed composite Simpson/trapezoid API nor an
explicit Euler/Heun equivalent in this comparison; those rows are NUMERIX-only.
No forced gradient-descent comparison is made because GSL's available algorithms
and line searches differ. Differences are recorded rather than hidden.

NUMERIX-only workloads include evaluating a precompiled `sin(x)+x^2` at 0.7,
automatic execution, managed execution and execution plus diagnostics for
`x^2-2` on[0,2]. Discovery tests `x^2-2`, `x^2+1`, `(x-1)*(x-2)*(x-4)` and
`(x-0.3)^2` on[0,5] at 16,32,64 panels. Expected detected counts are1,0,3,1;
counts and maximum returned residual are checked/reported. This finite sample
does not establish discovery completeness or correctness for arbitrary roots.
Existing root-discovery tests cover poles, boundaries, cancellation and limits.

## Allocation measurement and fault injection

On Linux with a GNU-compatible linker and **static** GSL/CBLAS, add:

```sh
cmake -S . -B cmake-build-bench -DNUMERIX_BENCHMARK_ALLOCATIONS=ON
cmake --build cmake-build-bench -j2
./cmake-build-bench/numerix_comparison_memory --memory > /tmp/allocations.csv
ctest --test-dir cmake-build-bench --output-on-failure
```

The separate memory binary wraps malloc/calloc/realloc/free in linked objects.
It measures one setup/operation/destruction after input generation, including
nested NUMERIX solvers/diagnostics, GSL workspaces and the GSL adapter's ODE
trajectory. It reports successful allocation count, cumulative requested bytes,
peak simultaneously live **requested heap bytes**, persistent setup bytes, and
bytes live after cleanup. It excludes input arrays, stack, allocator metadata,
libc-internal allocations and process RSS. Freeing a temporary buffer makes
its live bytes fall; cumulative allocated bytes do not fall. Setup is included
in memory even where excluded from execution timing. Shared GSL is rejected for
this probe because linker wrapping cannot observe calls inside a shared library.
Tracker metadata is fixed static storage and is outside the measurements.
Capacity overflow aborts the run; failed allocation attempts are not successes.

Discovery CSV also reports the original direct-array/workspace metrics, with
their Phase 8 scope unchanged. These exclude the nested allocations now captured
by the independent probe. `numerix_allocation_failure_tests` injects a one-time
failure at **every allocation index** observed in expression compilation and
six discovery workloads, and verifies cleanup. This closes selected failure
paths, not every allocation path in the whole library.

`/usr/bin/time -v` can separately record process peak RSS for a filtered run:

```sh
rss_dir=$(mktemp -d)
/usr/bin/time -v ./cmake-build-bench/numerix_comparison_benchmark \
  --samples 3 --case dense_well --library NUMERIX \
  --output-dir "$rss_dir" --run-id rss > "$rss_dir/measurements.csv"
```

RSS is for the entire process and harness, including all selected sizes and
phases. Do not compare it with requested heap bytes or call it per-operation
library memory.

## Other validation

```sh
python3 scripts/validate_release.py --output /tmp/numerix-validation
make benchmark-diagnostics
make benchmark-root-discovery
```

Validation requires GCC, Clang, CMake, CTest and optionally Valgrind/Cppcheck.
Use `--cmake`, `--ctest`, `--analyzer-gcc`, `--cppcheck` for local tools.
The script records actual exits; analyzer exit0 only means it ran. Diagnostics
require manual triage, as documented in the final report. Keep large build
artifacts outside version control. No Windows/MSVC result is inferred from a
Linux build.

## Phase 10 update

Historical phase9/ results retain the earlier stopping rules. Current root
comparisons use position/contraction evidence, with no small-residual shortcut
in either driver. Current adaptive Simpson uses a global conservative error
sum and `max(abs,rel*abs(I))`. See [tolerance semantics](../documentation/api/tolerances.md)
and [the accuracy report](https://github.com/kiran-8287/numerix/blob/master/documentation/development/phase10_accuracy_report.md).
Results under phase10/ are a new measured baseline, not a speed-optimization claim.
