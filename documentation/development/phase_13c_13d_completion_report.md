# NUMERIX Phase 13C + 13D Completion Report

**Date:** 12 November 2026<br>
**Status:** **COMPLETE WITH DOCUMENTED LIMITATIONS**<br>
**Detailed evaluation:** [NUMERIX_comparative_evaluation.md](../reports/NUMERIX_comparative_evaluation.md)<br>
**HTML evaluation:** [NUMERIX_comparative_evaluation.html](../reports/NUMERIX_comparative_evaluation.html)

## A. Environment

| Item | Value |
| --- | --- |
| Branch / baseline commit | `master` / `6c2ef314bbe0b563dbf2353dd70a3b1ee2748574` |
| Project version | `0.2.0-rc1` (unchanged) |
| OS / architecture | Oracle Linux Server 8.10; Linux 5.15; x86_64 |
| CPU | Intel Core i5-7400; 4 online cores; 1 socket; no SMT |
| Compiler / flags | GCC 8.5.0; Release `-O3 -DNDEBUG`; no fast-math |
| GSL | 2.8, locally built with GCC `-O3 -DNDEBUG`, bundled CBLAS |
| CMake / CTest | CMake 4.4.4 |
| Python / analysis | Python 3.9.25; NumPy 1.26.4; Matplotlib 3.9.4; SciPy 1.13.1; Markdown 3.8.2 |
| Floating point | binary64 `double`: 64 storage bits, 53 significand bits, exponent bounds −1021 to 1024 |
| Timer | `CLOCK_MONOTONIC`; wall time only |
| Schema | Benchmark schema v2 |

The GSL optimized prefix is `/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean`. Both the CMake and Make comparison benchmark builds reported `GSL=2.8`.

The repository worktree was already dirty with Phase 13B implementation and evidence when this work started. Those files remain uncommitted and were used by the Phase 13C benchmark. The main run's `environment.json` captures the working-tree status, and [source_fingerprints.sha256](../../benchmarks/results/phase13c/main-21samples-seed20261112/source_fingerprints.sha256) fingerprints the relevant C sources, headers, build files, and benchmark scripts. The commit hash alone is not a complete source identifier for this run. No commit or push was made.

## B. Benchmark coverage

The study plan contains 114 core mathematical configurations: 16 root, 30 linear algebra, 15 differentiation, 12 interpolation, 16 integration, 10 optimization, and 15 ODE. All 114 were attempted and completed in the workload manifest. Sixteen additional NUMERIX-only capability probes were recorded separately.

| Record type | Count |
| --- | ---: |
| Workload summary rows | 520 (299 NUMERIX, 221 GSL) |
| Solver status success / failure | 500 / 20 |
| Accuracy-passing summary rows | 374 |
| Paired method/phase workloads | 221 |
| Eligible direct pairings | 95 |
| Eligible best-available or approximate pairings | 67 |
| Excluded/ineligible paired workloads | 59 |
| Raw timing batches | 10,920 |
| Separate accuracy probes | 10,920 |
| Timing batches per library/workload summary | 21 |

Pairing counts refer to method/phase rows and not unique problem configurations. Each run has distinct raw-timing and accuracy rows; the verifier confirmed their IDs, summary statistics, precision metadata, ordering, and links.

## C. Library coverage and exclusions

| Category | NUMERIX coverage | GSL 2.8 coverage | Classification / exclusions |
| --- | --- | --- | --- |
| Root | Bisection, Newton, Brent | Bisection, Newton, Brent | Bisection/Newton direct; Brent approximate because safeguards differ. No-bracket/tangent rejection and oscillatory Newton failure retained and excluded from speed ratios. |
| Linear algebra | Pivoted LU; factor, solve, total phases | LU routines; corresponding phases | Direct for eligible systems. Factor-only has no output accuracy metric; solve and total phases are accuracy-gated. Ill-conditioned factorization/accuracy failures remain visible. |
| Differentiation | Richardson finite difference | Adaptive central difference | Best available; methods and work estimates differ. |
| Interpolation | Linear, Newton, Lagrange | Linear and polynomial interpolation | One direct linear-segment case; polynomial evaluation comparisons approximate. Setup and evaluation are separated. |
| Integration | Adaptive Simpson, trapezoid, Simpson | QAG 21-point adaptive integration | Adaptive methods best available; fixed NUMERIX rules are capability-only and accuracy failures are not used for claims. |
| Optimization | Golden-section minimization | Golden-section minimization | Direct one-dimensional comparison. No multidimensional optimizer comparison. |
| ODE | Fixed RK4, Euler, Heun | Fixed-step RK4 adapter | RK4 direct only when both trajectory accuracy gates pass; Euler/Heun capability-only. GSL `step_apply` is fixed-step and not adaptive. |

Investigated but not integrated: LAPACK/OpenBLAS, NLopt, SUNDIALS, Eigen, and Boost.Math were not detected in the environment. GSL covered the required priority-one categories; each additional library would require adapters and comparison validation, so those integrations were deferred. No Windows or macOS benchmark was run.

## D. Results summary

All speed ratios are GSL median runtime divided by NUMERIX median runtime; above 1 favors NUMERIX. Ratios are not combined into a universal library score.

- **Direct results:** root category ratio 0.913 (95% problem-bootstrap interval 0.803–1.023, unresolved); linear aggregate 0.684 (0.614–0.769), driven by faster GSL factor-and-solve time while solve-only was near equal and slightly favored NUMERIX; golden section 2.141 (1.996–2.258) favored NUMERIX; fixed RK4 2.675 (2.637–2.723) favored NUMERIX on eight accuracy-eligible configurations. One direct interpolation case favored GSL but is insufficient for a category claim.
- **Best available / approximate:** NUMERIX Richardson differentiation was 1.89× faster than GSL adaptive central difference and had lower median error in all 15 configurations. GSL QAG was about 38× faster than NUMERIX adaptive Simpson on 16 eligible configurations. GSL Brent was about 3.9× faster in the approximate comparison. GSL polynomial evaluation was about 1.7× faster than NUMERIX Newton and 4.5× faster than NUMERIX Lagrange evaluation.
- **Accuracy:** per-category median errors, pass counts, and failures are in the detailed report and source CSVs. NUMERIX passed all applicable differentiation and interpolation rows; GSL passed 16/16 applicable adaptive integration rows. Fixed low-order NUMERIX integration rules missed 18 of 32 applicable records. ODE trajectory accuracy passed 24/45 NUMERIX method rows and 12/15 GSL RK4 rows; GSL had lower median trajectory error in all 15 paired problem configurations.
- **Reliability:** 20 of 520 summary rows had nonzero solver status. They arose from expected root bracket rejection, one oscillatory Newton convergence failure per library, and NUMERIX ill-conditioned LU factorization rejection. Returned-but-inaccurate outputs are separately counted and excluded from performance claims.
- **Memory:** direct eligible pairs show median peak live requested heap of 0 vs 56 B (root), 33,808 vs 33,928 B (linear), 0 vs 40 B (one interpolation case), 0 vs 80 B (optimization), and 2,080 vs 2,144 B (ODE), NUMERIX vs GSL. These are wrapped heap-request statistics, not process RSS. Integration memory is best-available and not directly ranked.
- **Scalability:** dense LU measurements span `n=8` to `n=256`; polynomial evaluation spans multiple node counts; RK4 spans 10, 100, and 500 steps. Graphs show total LU cost increasingly favors GSL with size, while solve-only is close; ODE runtime grows with steps and only accuracy-passing cases qualify for direct timing ratios.

The report [NUMERIX_comparative_evaluation.md](../reports/NUMERIX_comparative_evaluation.md) contains per-category results, statistical intervals, figures, accuracy and reliability tables, methodology, and limitations.

## E. Generated artifacts

- Main raw run: `benchmarks/results/phase13c/main-21samples-seed20261112/` — summary, raw timing, accuracy probes, workload/comparison manifests, environment metadata, and source fingerprints.
- Verified pilots: `benchmarks/results/phase13c/pilot-existing-3samples/`, `pilot-expanded-3samples/`, and `pilot-o3-3samples/`.
- Allocation instrumentation: `benchmarks/results/phase13c/memory-instrumented-o3/`.
- Finalized configuration plan: `benchmarks/analysis/phase13c/workload_plan.csv`.
- Analysis script: `scripts/analyze_phase13c.py`.
- Plan preparation script: `scripts/prepare_phase13c_workload_plan.py`.
- HTML rendering script: `scripts/render_phase13c_report.py`.
- Analysis tables: `benchmarks/analysis/phase13c/tables/` (12 CSV tables).
- Graphs: `benchmarks/analysis/phase13c/figures/` (17 figures, PNG and SVG).
- Comparative report: `documentation/reports/NUMERIX_comparative_evaluation.md` and `.html`.
- This completion report: `documentation/development/phase_13c_13d_completion_report.md`.
- Phase 13B preservation checksums: `documentation/development/phase13c_phase13b_checksums.sha256` (35 files).

The analysis script regenerates all tables and graphs from saved raw data; report rendering does not rerun the numerical study.

## F. Verification

| Command / check | Exit / result |
| --- | --- |
| `python3 scripts/verify_benchmark_results.py benchmarks/results/phase13c/main-21samples-seed20261112` | 0; schema v2, 520 summaries, 10,920 raw timing batches, 10,920 accuracy probes verified |
| `python3 scripts/verify_benchmark_results.py benchmarks/results/phase13c/pilot-o3-3samples` | 0; schema v2, 520 summaries, 1,560 raw batches, 1,560 accuracy probes verified |
| GSL CMake Release build: `cmake --build /tmp/numerix-phase13c-gsl-o3 --parallel 4` | 0 |
| GSL CTest: `ctest --test-dir /tmp/numerix-phase13c-gsl-o3 --output-on-failure` | 0; 36/36 passed |
| `make -j4 all test` | 0; Make tests passed |
| Isolated Make GSL benchmark build in `/tmp/numerix-phase13c-make-check` with GSL 2.8 include/library overrides | 0; executable reported `GSL=2.8` |
| Static-GSL memory-probe rebuild with explicit `libgsl.a` and `libgslcblas.a`, followed by `numerix_comparison_memory --memory` | 0; reproduced 520 rows matching the preserved allocation metrics, ignoring run ID |
| `python3 -m py_compile scripts/analyze_phase13c.py scripts/prepare_phase13c_workload_plan.py scripts/render_phase13c_report.py tests/test_phase13c_analysis.py` | 0 |
| `python3 tests/test_phase13c_analysis.py` | 0; 4 tests passed |
| `python3 scripts/render_phase13c_report.py` | 0; generated the HTML report |
| Analysis/graph generation via `python3 scripts/analyze_phase13c.py ...` | 0; all 12 tables and 17 PNG/SVG figures regenerated from saved results |
| `sha256sum -c documentation/development/phase13c_phase13b_checksums.sha256` | 0; all 35 Phase 13B files unchanged |
| `sha256sum -c benchmarks/results/phase13c/main-21samples-seed20261112/source_fingerprints.sha256` | 0; all 45 source/build/script fingerprints match |
| Phase 9/10 SHA-256 values listed in `phase_13b_completion_report.md` | Verified to match the historical report values |
| `git diff --check` | 0 at final source/document check |

The CMake and Make builds used the current dirty Phase 13B worktree contents. The isolated Make GSL check copied source files to `/tmp` so it did not overwrite workspace build products.

## G. Limitations

- One Oracle Linux host, one CPU model, GCC 8.5, and one optimized build; no Windows, macOS, alternate compiler, or multi-host comparison.
- Wall-clock timing only; CPU frequency, load, and scheduling were not controlled.
- GSL is the only integrated external library. Other candidates were deferred.
- Some methods have no equivalent API; best-available and approximate results are kept distinct from direct results.
- Accuracy thresholds and error scales are method- and category-specific. Failed statuses and failed accuracy checks reduce eligible pair counts.
- Requested heap bytes and wrapped allocation counts omit RSS, stack, allocator metadata, and unwrapped allocations.
- The active uncommitted worktree is needed to reconstruct the exact run; the recorded base commit alone is insufficient.
- This is not a broad multithreaded, sparse-matrix, multidimensional-optimization, or high-dimensional ODE evaluation.

## H. Final recommendation

**COMPLETE WITH DOCUMENTED LIMITATIONS.** Phase 13C ran the planned GSL 2.8 comparison across all seven categories; Phase 13D generated verified statistical tables, graphs, a full Markdown report, and an HTML rendering. The study is reproducible from preserved raw data and scripts. It supports method-specific conclusions, not a universal ranking of NUMERIX against GSL. Historical Phase 9, Phase 10, and Phase 13B result files were preserved; the 35 Phase 13B checksums and the historical Phase 9/10 checksums match their saved values. No version change, commit, or push was made.
