# NUMERIX Phase 13B Completion Report

Date: 2026-11-12<br>
Scope: correctness and benchmark measurement-gate fixes<br>
Version remains `0.2.0-rc1`; no commit or push was made.

### Post-completion GSL update — 2026-11-12

After the original Phase 13B validation, GSL 2.8 was built from the official release source and installed user-locally at `/home/142401034/.local/numerix-deps/gsl-2.8`. A clean CMake configuration found it using `GSL_ROOT_DIR`, and `numerix_comparison_benchmark --versions` reported `GSL=2.8`. The benchmark target built and linked successfully.

A paired smoke run was then recorded at [run 39706f6b](../../benchmarks/results/phase13b/39706f6b9e364f3b9940460417fb3d67/summary.md): three polynomial-root methods, three samples per library and method, six workload summaries, 18 raw timing batches, and 18 linked accuracy probes. Both `NUMERIX_FIRST` and `GSL_FIRST` orders occurred; all records passed the schema v2 verifier. This confirms the paired measurement path works. It is a smoke run, not a performance conclusion or the full benchmark matrix. The earlier baseline and validation notes below accurately describe GSL's absence during the original Phase 13B work.

## A. Baseline

- Branch: `master`.
- Initial HEAD: `6c2ef314bbe0b563dbf2353dd70a3b1ee2748574`, matching the Phase 13A audit baseline.
- Initial tracked worktree: clean. One pre-existing user-created untracked file, `documentation/development/phase_13a_benchmark_readiness_audit.md`, was preserved unchanged.
- Initial GCC 8.5.0 Debug build passed the 33 then-registered CTest cases. A fuller source and build inventory is in [phase_13b_baseline.md](phase_13b_baseline.md).
- Environment: Linux; GCC 8.5.0, Clang 21.1.8, GNU Make 4.2.1, Python 3.9.25, bundled CMake/CTest 4.4.4, Valgrind 3.22.0. GSL was not installed. The bundled CMake executable was used because CMake and CTest were not on `PATH`.

The expected branch, commit, project version, and modular build layout matched the audit. No branch switch or reset was needed.

## B. Finding-by-finding results

| Finding | Status | Reproduction and fix | Regression and remaining limitation |
| --- | --- | --- | --- |
| ROOT-01 | **FIXED AND VERIFIED** | Baseline Bisection returned `LIMIT_REACHED` with its two endpoint calls consumed although the bracket already met position tolerance. It now checks the bound before a new callback, reuses an endpoint residual if the rounded midpoint equals an endpoint, and otherwise marks an unevaluated residual `NAN`. | `test_phase13b_regressions.c` checks a two-call success, an interval needing subdivision, exact midpoint accounting, and adjacent representable endpoints. High-level root selection accepts a finite positional estimate with unavailable residual. Residual remains unavailable when the returned midpoint was not sampled. |
| LA-01 | **FIXED AND VERIFIED** | Baseline rejected `A=[1e-310], b=[1e-310]` as singular. LU-backed Gaussian and Gauss-Jordan now use scaled partial pivoting: each row is scaled by its largest original matrix entry; a pivot quality at or below `n * DBL_EPSILON` is rejected. | Regressions cover `1e-310`, the smallest subnormal, ordinary and large scales, mixed row scales, exact singularity, near-singularity, and residuals. This is a scaled-pivot heuristic, not a condition estimate or forward-error guarantee. `absolute_tolerance` remains unused for pivot acceptance; this is documented. |
| LA-02 | **FIXED AND VERIFIED** | Baseline finite inputs could produce an infinite LU factor while decomposition returned success. LU now checks elimination multipliers, products, and updated entries as they are formed; numerical failure destroys partial output and leaves a cleared factorization. | Overflow and cleanup are checked in `test_phase13b_regressions.c`. The allocation-fault build injects failure at each of LU's three allocations and checks for no live bytes and cleared output. |
| DIFF-01 | **FIXED AND VERIFIED** | Baseline `f(x)=x`, `x=1e16`, `h=0.25` returned success with derivative zero because stencil coordinates collapsed. Every requested stencil coordinate is now checked for finiteness and pairwise distinctness before callbacks. | Tests cover forward, backward, central, five-point, Richardson, and second derivative formulas, large coordinates, local ULPs, `nextafter`, subnormals, and valid distinct stencils. The solver intentionally does not enlarge the user's step; a collapsed stencil returns `INVALID_INPUT`. |
| ODE-01 | **FIXED AND VERIFIED** | Baseline Euler integration at `x=1e16`, `h=0.25` returned success with two points at the same time. Solvers now require each outer step to advance; RK4 also requires a finite midpoint distinct from both endpoints. | Regressions cover Euler, Heun, RK4, vector states, forward and reverse steps, stagnation, midpoint collapse, and valid near-ULP steps. Failure destroys the partial history and returns the failure status with zero points. Zero-step calls remain invalid as before. |
| INTERP-01 | **FIXED AND VERIFIED** | Baseline interpolation with opposite finite endpoints near `1e308` failed from an overflowing intermediate although the midpoint result was zero. Linear interpolation now uses exponent-scaled differences, products, sums, and quotient, normalizing differences by powers of two. | Tests cover the extreme midpoint, adjacent large abscissas, ordinary and tiny scales, subnormals, both extrapolation directions, endpoint-near queries, and truly unrepresentable results. Other polynomial interpolation formulas retain their existing overflow limitations. |
| DIAG-01 | **FIXED AND VERIFIED** | Baseline differentiation placed numeric zero in the generic residual field despite having no residual metric. Differentiation and interpolation now use `NAN` for unavailable residuals. Diagnostic reports use existing `has_*` flags and initialize unavailable values to `NAN`; CLI and examples print “residual unavailable” when needed. | Regression checks differentiate an identity callback, preserve actual zero root and linear-system residuals, and confirm diagnostic flags. No diagnostic serializer exists in this repository. The generic `numerix_result` has no availability bit, so callers must use `isfinite(residual)`; documented without changing structure layout. |
| BMARK-01 | **FIXED AND VERIFIED** | Replaced the ambiguous precision field with storage width, significand precision, and exponent bounds, using `sizeof(double)`, `CHAR_BIT`, `DBL_MANT_DIG`, `DBL_MIN_EXP`, and `DBL_MAX_EXP`. | The C test compares fields with the compiler macros. Current host reports 64 storage bits, 53 significand bits, min exponent -1021, max exponent 1024. Metadata does not assume all platforms use binary64. |
| BMARK-02 | **FIXED AND LIVE-SMOKE VERIFIED** | A seeded, deterministic counterbalance schedule gives opposite first-library order to adjacent paired cases. Run seed and actual order are recorded. Warm-up is one untimed operation per library/workload; calibration uses powers-of-two batches until 2 ms, capped at 1,048,576 operations. | Schedule tests verify repeatability, both first orders, seed-dependent schedules, and balanced blocks. The post-installation GSL smoke run recorded both first-library orders across paired NUMERIX/GSL polynomial-root cases. The smoke is too small for comparative performance claims. |
| BMARK-03 | **FIXED AND VERIFIED** | Schema v2 preserves raw timing batches, separate accuracy probes, workload manifests, and derived summary records. The verifier independently reconstructs statistics and checks IDs, precision, order, batches, statuses, and accuracy links while retaining the legacy Phase 9/10 verifier. | Integration tests reject missing or duplicate samples, invalid times, wrong summary statistics, malformed precision, missing order, identity mismatch, and missing accuracy rows. The post-installation run verified six summaries, 18 raw timing batches, and 18 accuracy probes for both libraries. |

## C. Numerical changes

The LU and Gauss-Jordan pivot decision is now relative to each row's original scale, which permits uniformly tiny and subnormal systems while rejecting zero and relatively negligible pivots. LU failure is detected at the arithmetic stage that produces the invalid value, and all partial allocations are released.

Differentiation validates the whole requested stencil before invoking the callback. ODE integration rejects a time coordinate that does not advance and rejects collapsed RK4 midpoint stages. Linear interpolation uses scaled arithmetic to avoid avoidable intermediate overflow and returns `NUMERICAL_FAILURE` when the final value is outside the finite range.

Bisection may now return from a bracket-width bound without spending another callback. A residual is finite only when it is known at the returned position; otherwise it is `NAN`. Existing diagnostic availability flags distinguish unavailable values from true zero. No public structure fields or function signatures changed. The meaning of the existing LU `threshold` field is now the documented relative cutoff.

## D. Benchmark infrastructure changes

Schema v2 stores:

- `raw_timing.csv`: one row per timing batch, including run/configuration IDs, sample index, library, method, pair/order/seed, elapsed time, unit, batch size, status, precision, and linked accuracy record.
- `accuracy.csv`: separate untimed probes linked to timing observations.
- `workloads.csv`: parameters, size, tolerance, precision, reference method, comparison mode, library, and configuration ID.
- `measurements.csv`: summaries derived from the raw batches; `comparisons.csv` contains direct ratios only for eligible same-algorithm rows.

Summary definitions are min, median (averaging the middle pair for even counts), mean, sample standard deviation, nearest-rank p95 `ceil(0.95*n)`, max, sample count, and batch size. The C statistics routine uses scaled Welford accumulation; the Python verifier recomputes from saved raw samples. The stable configuration ID hashes a canonical serialization of case/category, variant, difficulty, dimensions, input parameters, scale, expected value, reference-generation method, tolerances, method, phase, and mode.

The documented protocol separates same-algorithm and best-available comparisons. Timing uses monotonic wall time, retains outliers, and includes selected API work such as in-call allocation and output handling; persistent setup is separated where available. The run environment records the timer API and states that host timer granularity is not queried; calibration targets 2 ms batches. Memory columns are explicitly requested heap bytes observed by the allocator wrapper, not RSS or total process memory. Cross-platform memory comparability is not claimed.

The original final-source, NUMERIX-only smoke run is stored at [Phase 13B run 6370ec5c](../../benchmarks/results/phase13b/6370ec5c70dd4d058893cfc213a349d5/summary.md). It records three polynomial-root workload summaries, nine raw timing batches, and nine separate accuracy probes with seed `20261112`. After GSL was installed, the paired smoke run [39706f6b](../../benchmarks/results/phase13b/39706f6b9e364f3b9940460417fb3d67/summary.md) recorded three root methods for each library and both execution orders. Earlier schema smoke runs from implementation are also retained under the Phase 13B results directory.

## E. Test results

All commands below exited 0. CMake and CTest were invoked as `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake` and `/tmp/numerix-phase9-tools/python/cmake/data/bin/ctest`.

For each CMake configuration below, the build and test commands were `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake --build <listed build directory> --parallel 4` and `/tmp/numerix-phase9-tools/python/cmake/data/bin/ctest --test-dir <listed build directory> --output-on-failure`. The sanitizer CTest command additionally used the listed `ASAN_OPTIONS` and `UBSAN_OPTIONS` environment variables.

| Configuration | Command/configuration | Result |
| --- | --- | --- |
| GCC Debug | `cmake -S . -B /tmp/numerix-phase13b-gcc-debug -DCMAKE_BUILD_TYPE=Debug -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON -DNUMERIX_BENCHMARK_ALLOCATIONS=OFF`; `cmake --build /tmp/numerix-phase13b-gcc-debug --parallel 4`; `ctest --test-dir /tmp/numerix-phase13b-gcc-debug --output-on-failure` | 36/36 passed, including raw benchmark verifier and Phase 13B regressions. |
| GCC Release | `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake -S . -B /tmp/numerix-phase13b-gcc-release -DCMAKE_BUILD_TYPE=Release -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON`; `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake --build /tmp/numerix-phase13b-gcc-release --parallel 4`; `/tmp/numerix-phase9-tools/python/cmake/data/bin/ctest --test-dir /tmp/numerix-phase13b-gcc-release --output-on-failure` | 36/36 passed. |
| Clang Debug | `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake -S . -B /tmp/numerix-phase13b-clang-debug -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON`; same build and CTest commands with `clang-debug` as the build directory | 36/36 passed. |
| Clang Release | `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake -S . -B /tmp/numerix-phase13b-clang-release -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON`; same build and CTest commands with `clang-release` as the build directory | 36/36 passed. |
| Clang ASan + UBSan | `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake -S . -B /tmp/numerix-phase13b-sanitizers -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON -DCMAKE_DISABLE_FIND_PACKAGE_GSL=TRUE -DCMAKE_C_FLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'`; `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake --build /tmp/numerix-phase13b-sanitizers --parallel 4`; CTest with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` | 36/36 passed; no sanitizer findings. |
| GCC allocation fault injection | `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake -S . -B /tmp/numerix-phase13b-allocation -DCMAKE_BUILD_TYPE=Debug -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON -DNUMERIX_BENCHMARK_ALLOCATIONS=ON -DCMAKE_DISABLE_FIND_PACKAGE_GSL=TRUE`; `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake --build /tmp/numerix-phase13b-allocation --parallel 4`; CTest in that directory | 37/37 passed, including per-allocation LU failure cleanup. |
| Make | `make -j4 all test` (GCC, `-O2`, C11 and warning flags) | Exit 0; production library, CLI, examples, category tests, Phase 13B tests, benchmark statistics, release regressions, and accuracy tests passed. |
| Installed API consumers | `cmake --install /tmp/numerix-phase13b-gcc-release --prefix /tmp/numerix-phase13b-install`; `cmake -S tests/installed_consumer -B /tmp/numerix-phase13b-consumer -DNUMERIX_INSTALL_PREFIX=/tmp/numerix-phase13b-install`; build and CTest in that directory | C and C++ consumers passed, 2/2. |
| Valgrind | Phase 13B regressions, release regressions, accuracy tests, and benchmark measurement tests with definite/indirect leak errors enabled | All four exited 0. |
| Clang static analyzer | Nine changed numerical, intelligence, and benchmark C translation units | Completed with no diagnostics after removing one dead-store warning. |
| Historical verifier | `python3 scripts/verify_benchmark_results.py` on Phase 9 and Phase 10 directories | Both passed; 222 observations verified for each legacy dataset. |
| Schema v2 smoke | `python3 scripts/run_benchmarks.py --build-dir /tmp/numerix-phase13b-gcc-debug --samples 3 --seed 20261112 --case polynomial` | Passed; three summaries, nine raw timing batches, nine accuracy probes. |
| GSL paired smoke after local installation | Configure with `-DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON -DGSL_ROOT_DIR=/home/142401034/.local/numerix-deps/gsl-2.8`; build `numerix_comparison_benchmark`; run `python3 scripts/run_benchmarks.py --build-dir /tmp/numerix-phase13b-gsl-smoke --samples 3 --seed 20261112 --case polynomial`; independently verify the recorded run | Passed; GSL 2.8 detected and linked; six workload summaries, 18 raw timing batches, 18 accuracy probes; both first-library orders recorded. Smoke scope only. |
| Diff hygiene | `git diff --check` | Passed. |

`cppcheck` was unavailable. GCC 8.5.0 predates GCC's `-fanalyzer`; GCC static analysis was not run. No native Windows/MSVC, MinGW, or macOS build was attempted.

## F. Historical data integrity

Phase 9 and Phase 10 CSV files were read only. Their final SHA-256 values match the baseline capture:

| Historical file | SHA-256 |
| --- | --- |
| Phase 9 `allocations.csv` | `2aebebcd35f6dd49c3b6d756bf31f72fcecf9b5e29f8d6467c84bbcdb45bfcd9` |
| Phase 9 `comparisons.csv` | `ba16b5df2b90b2a1b085ecb2978131f82e5d444eabcd11c4d6796e4c4947188a` |
| Phase 9 `measurements.csv` | `b460bbc3ff1c755ad56dcca7ffd541fd70a23d7bf1ec0451ea2271bbfa50064e` |
| Phase 9 `numerix_only.csv` | `6ffebc5bbbe9ce23de211e5ad667c7debdeb2c35ee67586e904fbfe0836da7ff` |
| Phase 10 `allocations.csv` | `bf32583c442f98a062b5049cbbf741f8126532f96ceb23de079d842e3f7ad75f` |
| Phase 10 `comparisons.csv` | `97e91fba8421ba7b49618687b75d808e3fe9b7683fe7a837afc38c004f09ae55` |
| Phase 10 `measurements.csv` | `5ffdfe38eb9904ec86294e7b72be6da374fa29add826700505f0964adb098a36` |

All new run artifacts are separated under `benchmarks/results/phase13b/`. The run driver requires an empty destination and rejects destinations inside the Phase 9 or Phase 10 trees; the C harness also uses exclusive file creation.

## G. Changed files

- Build and benchmark: `CMakeLists.txt`, `Makefile`, `benchmarks/comparison.c`, new `benchmarks/measurement.c` and `.h`, `benchmarks/README.md`.
- Numerical implementation: `src/algorithms/root_finding/root_finding.c`, `linear_algebra.c`, `differentiation.c`, `interpolation.c`, `ode.c`, `src/intelligence/root_execution.c`, and `portfolio.c`.
- Public contract and user output: `include/numerix.h`, `cli/main.c`, `examples/managed_execution_demo.c`, `examples/root_discovery_demo.c`, and API documentation for diagnostics, roots, linear algebra, differentiation, interpolation, and ODEs.
- Benchmark tools: `scripts/run_benchmarks.py`, `scripts/verify_benchmark_results.py`, and `scripts/summarize_benchmarks.py`.
- Tests: new `tests/test_phase13b_regressions.c`, `test_benchmark_measurement.c`, and `test_benchmark_verification.py`; updates to `tests/test_accuracy.c` and `tests/test_allocation_failures.c`.
- Reports and data: `documentation/development/phase_13b_baseline.md`, this report, and the Phase 13B smoke runs under `benchmarks/results/phase13b/`.

The pre-existing Phase 13A audit file is preserved and was not modified.

## H. Remaining limitations

- GSL 2.8 is now installed user-locally and the paired measurement path passed the three-method polynomial-root smoke. The run has only three samples per library/method and does not establish comparative performance. A full, prespecified benchmark matrix is still needed before making performance claims.
- Windows/MSVC and macOS runtime/build portability remain unvalidated. The implementation uses C11 facilities, but a native build is still needed to confirm platform behavior, including exclusive CSV creation and timer paths.
- Scaled pivoting is heuristic; ill-conditioned systems can still have large forward error. It intentionally does not use `absolute_tolerance` as the pivot threshold.
- A requested finite-difference step is rejected when floating-point coordinates collapse; the library does not silently choose a larger step.
- Only linear interpolation arithmetic received the exponent-scaled correction. Other polynomial interpolation methods can still overflow for extreme data.
- Requested heap allocation counts omit stack storage, allocator metadata, unwrapped allocations, and process memory. Cross-platform memory comparisons are not validated.
- `cppcheck` and GCC `-fanalyzer` were unavailable. No 400-configuration run, new library integration, comparison charts, commit, or push was performed.

## I. Final recommendation

**CONDITIONALLY READY FOR PHASE 13C.** All listed Phase 13B correctness changes pass the Linux GCC/Clang Debug and Release suites, sanitizer suite, allocation-fault suite, C/C++ installed-consumer checks, and focused regressions. GSL 2.8 is now installed user-locally, and the paired smoke confirms both recorded execution orders and verifier acceptance. The smoke is not a performance result; run the planned benchmark matrix before making comparative claims. Validate Windows/MSVC separately before treating the Windows support target as verified. Phase 13C and the 400-configuration benchmark were not started.
