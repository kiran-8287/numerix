# NUMERIX Phase 14 Audit and Execution Log

This file tracks the Phase 14 request in [`PHASE14_REQUIREMENTS.md`](../../PHASE14_REQUIREMENTS.md). It is the ordered checklist and source-of-truth progress log for this phase.

## Stage 1 — Repository and historical benchmark audit

**Status: complete.**

- Branch: `master`.
- HEAD at audit: `2292ee804b02bd82be56803a6eae9d36327ef117` (`Complete numerical audit and GSL benchmark study`), also `origin/master` at audit.
- Project version: `0.2.0-rc1`; Phase 14 will not change it.
- The worktree was clean after Phase 13 was pushed. The only initial addition is the requested root copy `PHASE14_REQUIREMENTS.md`; Phase 14 output directories are new and separate.
- The actual public API was inspected in `include/numerix.h`. The seven modules expose scalar root methods; vector/matrix operations, Gaussian/Gauss-Jordan solves, LU factor/solve; first/second finite differences; linear/Lagrange/Newton interpolation; finite trapezoid/Simpson/adaptive Simpson integration; bounded one-dimensional golden-section and vector gradient descent; and vector-valued fixed-step Euler, Heun, and RK4 ODE methods.
- No Phase 9, 10, 13B, or 13C implementation/data was edited during this audit.
- A pre-Phase-14 SHA-256 inventory of 125 Phase 13 result, analysis, report, and script files is stored at `benchmarks/results/phase14/logs/phase13_preservation.sha256`; all 125 entries verified immediately after capture.

### Existing Phase 13 result verification

The existing schema-v2 verifier exited 0 for all nine Phase 13 runs:

| Group | Run summaries | Timing batches | Accuracy probes |
| --- | ---: | ---: | ---: |
| Phase 13B paired smoke | 6 | 18 | 18 |
| Four additional Phase 13B runs | 3 each | 9 each | 9 each |
| Phase 13C main | 520 | 10,920 | 10,920 |
| Phase 13C existing pilot | 222 | 666 | 666 |
| Phase 13C expanded and optimized pilots | 520 each | 1,560 each | 1,560 each |

Exact verifier invocations are preserved in the turn record; they use `python3 scripts/verify_benchmark_results.py <run-directory>`. Phase 13 historical checksums also remain available in `documentation/development/phase13c_phase13b_checksums.sha256` and the Phase 13B completion report.

### Initial build and dependency inventory

- GCC 8.5.0, GNU Fortran, GNU Make, Python, and the bundled CMake executable at `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake` are available. CMake is not on the shell `PATH`.
- GSL 2.8 is locally installed, including an optimized prefix at `/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean`.
- Initial `pkg-config` and dynamic-loader searches found no LAPACKE/LAPACK/OpenBLAS, NLopt, SUNDIALS, or CMINPACK installation. Source builds are feasible in the user-local dependency area; no system installation is required.
- The actual API permits one-dimensional and vector ODE benchmarking, but has no variable-step ODE driver, no second-derivative callback API, and no multidimensional interpolation or integration API. Unsupported comparisons will be identified rather than inferred.

## Ordered execution stages

| Stage | Work | Status | Evidence / next action |
| --- | --- | --- | --- |
| 1 | Repository/API audit and Phase 13 preservation | Complete | This log; 125-file checksum inventory; nine historical run verifiers |
| 2 | Official-library capability, API, version, license, and build research | Complete | Pinned builds and representative C API smoke runs passed for all selected configurations; matrix records actual measured scope |
| 3 | Design at least 100 distinct problems and at least 20 families per module | Complete | `benchmarks/problems/phase14/workloads.csv` and `problem_inputs.jsonl`: 700 distinct IDs, 100/module, 20 families/module, five variants/family; validator passes |
| 4 | Optional C adapters and versioned Phase 14 schema | Complete | `benchmarks/adapters/phase14`; all enabled optional libraries compile; one representative smoke input per configured module/library passed |
| 5 | Pilots and adapter/reference/tolerance validation | Complete | A clean full pilot completed all 2,000 pairs with 42,000 timing rows and no launch failures; a harmonic-oscillator reference defect was corrected and all affected module/library checks passed afterward |
| 6 | Full seven-module, multi-library run | Complete | `benchmarks/results/phase14/main`: 2,000 pairs; 62,000 timing batches (31/pair); 2,000 accuracy records; 8,000 work records; 2,000 RSS probes; zero launch/write failures |
| 7 | Verification and paired statistical analysis | Complete | `analysis_verification.json` PASS; 700 workloads; all pair and batch counts verified; paired speedups now require both status success and accuracy pass; Mode A/B remain separate |
| 8 | Figures and visual inspection | Complete | 22 PNG/SVG pairs; legends, axes, units, and no-estimate rows inspected and corrected |
| 9 | Markdown and HTML comparative report | Complete | Reproducibly generated from saved Phase 14 tables and figures; explicitly reports partial method coverage |
| 10 | Completion report and final audit | Complete (study status: partial) | Completion report records completed work, exact limitations, and verification; no commit/push/tag/version bump |

### Stage 2 progress

- Capability matrix: `documentation/development/phase_14_library_capability_matrix.md`; machine-readable algorithm mapping: `benchmarks/analysis/phase14/method_mapping.csv`.
- Official source/documentation, C API, license, version, and scope findings are recorded in the capability matrix. ALGLIB and Boost.Math are excluded from the native-C primary study; Cuba is excluded from one-dimensional finite quadrature; Cephes is not a comparable seven-module package.
- Native source builds completed: Reference LAPACK 3.12.1 with reference BLAS/CBLAS/LAPACKE, NLopt 2.11.0, SUNDIALS 7.9.0 (CVODE + ARKODE serial), and CMINPACK 1.3.15.
- OpenBLAS 0.3.34 source is pinned. A Haswell/double-only build attempt failed at shared-library link because the upstream make path still referenced disabled single/complex symbols; the exact log is retained. The corrected all-precision Haswell build completed and its LAPACKE dgesv smoke run passed.
- GSL 2.8 was already built in the user-local optimized prefix. Build logs are in `benchmarks/results/phase14/logs/dependencies/`.

### Stage 3 workload evidence

- Generator: `benchmarks/problems/phase14/prepare_workloads.py` (seed `20261112`).
- Validator: `python3 benchmarks/analysis/phase14/verify_workloads.py` — **PASS**; exactly 700 IDs, 100 per module, 20 families per module, five variants per family, 700 distinct input payload hashes, and all saved input hashes verified.
- Reference policy, accuracy targets, module scope, and mode separation are preregistered in `benchmarks/analysis/phase14/phase14_design.md`.

### Stage 2 and Stage 4 adapter validation

- Optional C adapter build passed for NUMERIX, GSL 2.8, CMINPACK 1.3.15, NLopt 2.11.0, SUNDIALS 7.9.0, reference LAPACK/LAPACKE 3.12.1, and OpenBLAS 0.3.34.
- Representative native C calls passed for every configured module/library pair, including both LAPACKE variants, both SUNDIALS ODE paths, and CMINPACK/NLopt specialists.
- The first full pilot surfaced a GSL Newton adapter segfault for multiple-root cases because `gsl_function_fdf.fdf` was null. The adapter now supplies the combined value/derivative callback. These pilot failures remain preserved in `pilot/full-adapter-pilot-20261112` and are not part of the main benchmark data.
- The same pilot exposed a nested-array boundary defect in the benchmark-only JSON tokenizer. Object/array iteration now respects each container's closing byte offset. Matrix, vector, interpolation, optimization, and ODE inputs then passed representative native runs.
- CVODE in SUNDIALS 7 requires an explicitly attached nonlinear solver in this serial build. The adapter now uses `SUNNonlinSol_FixedPoint` with CVODE Adams and frees it with the solver workspace; both CVODE and fixed-step ARKODE smoke calls passed.
- Work-count CSV now distinguishes measured counts from not-collected metrics instead of writing zero for inapplicable work. GSL differentiation and GSL golden-section setup are classified as best-available because their numerical procedure differs from the current NUMERIX call path.
- The clean full pilot at `benchmarks/results/phase14/pilot/full-final-20261112` has 2,000 accuracy rows, 42,000 raw batches (21 per pair), 8,000 work-metric rows, and zero launch failures. It is pilot evidence only; the primary run used the corrected workload reference and larger timing batches.
- Pilot accuracy status and convergence remain separate. The run showed 82/100 first-derivative accuracy passes for NUMERIX central difference, 95/100 successful NUMERIX adaptive integrations due to inverse-square-root endpoints, 38/100 interpolation accuracy passes for both global-polynomial methods at the preregistered tolerance, and solver-status misses for difficult/singular systems. These observations are retained for the final analysis rather than used to tune the frozen thresholds.
- The pilot found a closed-form reference defect in the harmonic-oscillator workload: the initial-velocity terms omitted factors of frequency. `prepare_workloads.py` now uses the correct analytic solution. The input payload hashes and stable problem IDs did not change; the regenerated manifest and references validate, and all 20 harmonic-oscillator checks across NUMERIX, GSL, ARKODE, and CVODE pass within the saved tolerance.
- NLopt reached the saved minimizer-location tolerance on all 100 primary workloads but exhausted its 4,000-evaluation budget on all cases. The adapter preserves the accurate output/timing and reports convergence failure; the final analysis excludes these rows from status+accuracy-qualified speed estimates.

## Stage 6 — Primary benchmark execution

- Primary run: `benchmarks/results/phase14/main`, run ID `phase14-main-20261112`; started `2026-11-12T01:53:49Z`, finished `2026-11-12T02:01:43Z` UTC.
- The runner deterministically shuffled configured library order within each problem using seed `20261112`; all dependency thread settings were set to one. Batch count was 31 for every pair. Batch sizes by module are recorded in `run_metadata.json`.
- Results include 2,000 accuracy records, 62,000 raw timing rows with monotonic wall and process CPU durations, 8,000 work rows, 2,000 isolated-process peak-RSS rows, and an empty execution-failure table. No Phase 13 artifacts were rewritten.
- Primary benchmark data were not rerun after analysis-only corrections; only summary tables and visualizations were regenerated from the retained raw records.

## Stage 7 — Verification and statistical analysis

- `python3 benchmarks/analysis/phase14/analyze_results.py benchmarks/results/phase14/main` — **PASS**: 700 problems, 2,000 expected and observed library/problem pairs, 62,000 unique timing batches, 31 batches/pair, 2,000 memory rows, 8,000 work rows, no duplicates/missing values/launch failures.
- Speedup eligibility was tightened during final audit to require both successful solver status and accuracy pass for both libraries. Accuracy-qualified counts remain separate; NLopt therefore has no valid speed estimate (0 status+accuracy pairs), despite 100 accurate returned locations.
- `comparisons.csv` reports paired-problem median/geometric-mean ratios and 95% bootstrap intervals, with Mode A algorithm-equivalent and Mode B best-available comparisons not pooled. Wall time is the inferential measure; CPU time is retained and summarized separately.
- `python3 benchmarks/analysis/phase14/verify_workloads.py` — **PASS**: 700 distinct payloads, 100/module, 20 families/module, five variants/family, all input hashes and reference checks valid.

## Stage 8 — Figures

- `plot_results.py` regenerated 22 figures in PNG and SVG under `benchmarks/results/phase14/main/figures`.
- Final visual inspection covered the paired-speedup forest plot, coverage/status panels, runtime scalability, memory/RSS, and module-specific plots. The speedup graph title/annotations identify status+accuracy qualification and explicitly mark NLopt as having no valid speed estimate. The RSS graph includes a legend and discloses the shared-library/harness baseline.

## Stage 9 — Comparative report

- The reproducible generator is `benchmarks/analysis/phase14/generate_report.py`.
- Outputs: `documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.md` and `.html`.
- Both reports call the result partial: the minimum distinct-problem and current paired-run coverage is met, but broad method-level requirements are not. Missing method groups and memory limitations are enumerated.

## Stage 10 — Regression checks and final audit

- `make test` — **PASS**, including release/accuracy checks, all seven Phase 1 module tests, expression, audit, piecewise, intelligence, execution, diagnostics, root discovery, Phase 8 integration, Phase 13B regression, and benchmark measurement tests.
- CMake Release configure/build with `BUILD_TESTING=ON` in `/tmp/numerix-phase14-regression` — **PASS**; `ctest --output-on-failure` — **PASS**, 35/35 tests.
- Python phase14 scripts compile; workload validation and raw-result analysis pass. The full Phase 13 SHA-256 inventory verifies all 125 entries.
- Valgrind 3.22.0 with full definite-leak checking passed on one NUMERIX adapter path for each of the seven modules. GCC Address/UndefinedSanitizer configure was attempted but could not link because the environment lacks the expected `libasan.so.5.0.0` and `libubsan.so.1.0.0` runtime files.
- No production source/header, public API, or version change was made. Phase 14 changes remain uncommitted and unpushed as required.
- Remaining acceptance gaps: all-method/method-by-method benchmark coverage, vector/matrix and LU operation decomposition, second/higher-order differentiation comparisons, interpolation alternatives, fixed integration and optimization/ODE methods, trajectory-wide ODE accuracy, and heap allocation instrumentation. The overall study is therefore **partial**, not fully comprehensive.

## Constraints carried forward

- Phase 13 artifacts are read-only and must match `phase13_preservation.sha256` at the end.
- Competitor dependencies remain optional and must not enter the NUMERIX production library's public API or runtime dependencies.
- No benchmark result, reference, tolerance, or eligibility rule may be chosen after looking at which library wins.
- Keep solver status separate from accuracy pass; preserve failures and missing values explicitly.
- The user explicitly forbids a Phase 14 commit, push, or version bump in the supplied brief. Do not perform one.
- If the 100-problem/module minimum or any required competitor cannot be completed, report the exact achieved coverage and leave the study status partial rather than implying full acceptance.
