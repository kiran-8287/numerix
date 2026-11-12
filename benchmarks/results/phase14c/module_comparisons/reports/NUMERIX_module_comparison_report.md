# NUMERIX Phase 14C — Benchmark Evidence and Completion Report

> **Status: incomplete.** The dependencies, public-method audit, module pilots, analysis tables, and required figure files are assembled. The requested 100-problem-per-module full study and method-specific memory comparison were not completed. The figures distinguish historical results and pilots from missing data; they are not a final seven-module ranking.

## Executive summary

Phase 14C was requested to compare all applicable public numerical operations across seven modules, using 100 distinct problems and 20 families per module, and to report execution time, method-specific memory, and numerical accuracy. The current evidence package meets the audit and artifact requirements, but does **not** meet the full experimental coverage requirement.

- The current API audit contains **50 grouped rows**: root 9, linear algebra 21 (18 numerical operations and 3 lifecycle groups), differentiation 7, interpolation 4, integration 4, optimization 2, and ODE 3. It also lists 6 associated lifecycle/setup APIs separately. The previously omitted `numerix_lu_destroy` is included. Definitions were checked against `include/numerix.h` and `src/`.
- A candidate set of **700 unique historical Phase 14 problems** is checksum-verified: 100/module and 20 source families/module. The Phase 14C manifest has 5,000 method/problem planning rows. This is a candidate workload base, not 700 newly executed Phase 14C cases.
- Normalized figures use 52,824 raw timing observations and 1,696 accuracy records across sources. These totals include **49,600 historical Phase 14B root timing rows**; the remaining timing rows are small Phase 14C pilots. Optimization and ODE have no timing records.
- Exactly **21 required figure types** were generated, each in PNG and SVG (42 image files). All PNGs opened and verified, all SVGs parsed, and no extra graph type was generated. Every memory figure states “not measured.”
- Method-specific memory was not measured for any module. Historical process RSS was not reused as algorithm memory. A Linux/glibc allocator wrapper passed known-allocation, GSL 2.8 vector, and NUMERIX vector positive controls, but method-route visibility for OpenBLAS/LAPACK, NLopt, and SUNDIALS remains unverified.
- The runtime environment is Oracle Linux 8.10, glibc 2.28, GCC 8.5, and an Intel i7-13700. Pilot runs used isolated dependency prefixes. The selected OpenBLAS build reports bundled LAPACK 3.12.0; an older Phase 14 manifest says 3.12.1, so the Phase 14C pilot records the actual selected 3.12.0.

### Module status

| Module | Primary competitor | Inventory rows | Candidate problems | Pilot or historical evidence | Accuracy | Memory |
|---|---|---:|---:|---|---|---|
| Root | GSL 2.8 | 9 (9 numerical, 0 lifecycle) | 100 | Historical Phase 14B data; seven paired routes, two NUMERIX-only routes; not new Phase 14C measurements. | Historical | Not measured |
| Linear Algebra | LAPACK 3.12.0 with OpenBLAS 0.3.34 | 21 (18 numerical, 3 lifecycle) | 100 | Four of 18 numerical operations piloted; four inputs and eight routes. | 0/0 NUMERIX passes | Not measured |
| Differentiation | GSL 2.8 | 7 (7 numerical, 0 lifecycle) | 100 | Six scalar derivative methods piloted on three problems each. | 0/3 NUMERIX passes | Not measured |
| Interpolation | GSL 2.8 | 4 (4 numerical, 0 lifecycle) | 100 | Four methods piloted on three cases each; construction accuracy is N/A. | 0/3 NUMERIX passes | Not measured |
| Integration | GSL 2.8 | 4 (4 numerical, 0 lifecycle) | 100 | Three methods piloted on three problems each; setup and end-to-end routes retained. | 0/3 NUMERIX passes | Not measured |
| Optimization | NLopt 2.10.1 | 2 (2 numerical, 0 lifecycle) | 100 | Correctness-only pilot: one analytic case per method; no runtime data. | 1/1 NUMERIX passes | Not measured |
| Ode | SUNDIALS 7.9.0 (ARKODE ERKStep) | 3 (3 numerical, 0 lifecycle) | 100 | Correctness-only pilot: one two-state system, 21 output points; no accuracy gate or runtime data. | Not evaluated | Not measured |

### Figure index

Each cell links to both image formats. The memory figures state “not measured.”

| Module | Runtime | Memory | Accuracy |
|---|---|---|---|
| Root | [PNG](../figures/root/root_runtime.png) · [SVG](../figures/root/root_runtime.svg) | [PNG](../figures/root/root_memory.png) · [SVG](../figures/root/root_memory.svg) | [PNG](../figures/root/root_accuracy.png) · [SVG](../figures/root/root_accuracy.svg) |
| Linear Algebra | [PNG](../figures/linear_algebra/linear_algebra_runtime.png) · [SVG](../figures/linear_algebra/linear_algebra_runtime.svg) | [PNG](../figures/linear_algebra/linear_algebra_memory.png) · [SVG](../figures/linear_algebra/linear_algebra_memory.svg) | [PNG](../figures/linear_algebra/linear_algebra_accuracy.png) · [SVG](../figures/linear_algebra/linear_algebra_accuracy.svg) |
| Differentiation | [PNG](../figures/differentiation/differentiation_runtime.png) · [SVG](../figures/differentiation/differentiation_runtime.svg) | [PNG](../figures/differentiation/differentiation_memory.png) · [SVG](../figures/differentiation/differentiation_memory.svg) | [PNG](../figures/differentiation/differentiation_accuracy.png) · [SVG](../figures/differentiation/differentiation_accuracy.svg) |
| Interpolation | [PNG](../figures/interpolation/interpolation_runtime.png) · [SVG](../figures/interpolation/interpolation_runtime.svg) | [PNG](../figures/interpolation/interpolation_memory.png) · [SVG](../figures/interpolation/interpolation_memory.svg) | [PNG](../figures/interpolation/interpolation_accuracy.png) · [SVG](../figures/interpolation/interpolation_accuracy.svg) |
| Integration | [PNG](../figures/integration/integration_runtime.png) · [SVG](../figures/integration/integration_runtime.svg) | [PNG](../figures/integration/integration_memory.png) · [SVG](../figures/integration/integration_memory.svg) | [PNG](../figures/integration/integration_accuracy.png) · [SVG](../figures/integration/integration_accuracy.svg) |
| Optimization | [PNG](../figures/optimization/optimization_runtime.png) · [SVG](../figures/optimization/optimization_runtime.svg) | [PNG](../figures/optimization/optimization_memory.png) · [SVG](../figures/optimization/optimization_memory.svg) | [PNG](../figures/optimization/optimization_accuracy.png) · [SVG](../figures/optimization/optimization_accuracy.svg) |
| Ode | [PNG](../figures/ode/ode_runtime.png) · [SVG](../figures/ode/ode_runtime.svg) | [PNG](../figures/ode/ode_memory.png) · [SVG](../figures/ode/ode_memory.svg) | [PNG](../figures/ode/ode_accuracy.png) · [SVG](../figures/ode/ode_accuracy.svg) |

## Competitors and dependency setup

The fixed competitor mapping was preserved: GSL 2.8 for root finding, differentiation, interpolation, and integration; LAPACK with one-thread OpenBLAS 0.3.34 for linear algebra; NLopt 2.10.1 for optimization; and SUNDIALS 7.9.0 for ODE solving. GSL, OpenBLAS/LAPACK, and SUNDIALS were available in isolated user prefixes. NLopt 2.10.1 was built and installed alongside the existing 2.11.0 prefix at `~/.local/numerix-deps/prefix/phase14c/nlopt-2.10.1`; no system-wide package install was made. Markdown 3.7 was installed in the user Python environment to render the HTML report; Matplotlib 3.9.1 and Pillow 10.4.0 were already available.

Exact prefixes, source hashes/commits, build versions, and the historical LAPACK metadata discrepancy are recorded in [`library_versions.json`](../manifests/library_versions.json) and [`benchmark_environment.json`](../manifests/benchmark_environment.json).

## Method inventory and workload audit

The definitive grouped method list is [`method_coverage.csv`](../manifests/method_coverage.csv). Its comparison classes are: 20 direct, 23 best available, 2 NUMERIX-only, 2 not comparable, and 3 lifecycle-only. No row remains unclassified. Automatic root selection and solver portfolio have no invented GSL measurements. Piecewise differentiation/integration operations are preserved as not comparable to GSL boundary-aware APIs. A separate [`auxiliary_lifecycle_coverage.csv`](../manifests/auxiliary_lifecycle_coverage.csv) lists report, interpolant, piecewise-object, and ODE-solution cleanup/setup APIs.

The 700 problem definitions and input sidecar were copied byte-for-byte into the Phase 14C manifest area after checking each input hash and unique ID. The manifest script checks current public symbols and implementation definitions, identifies the stale Phase 14B portfolio source field, and records candidate applicability as planned where no adapter has run. Full workload coverage is **not claimed**.

## Module findings

### Root finding — historical Phase 14B evidence

GSL 2.8 is the selected competitor. Phase 14B v3 has 100 problems per NUMERIX root method, 31 batches per timed route, and the full nine-entry NUMERIX root inventory. Seven public methods have GSL routes; automatic method selection and solver portfolio are NUMERIX-only. Multiple-root discovery uses a fixed GSL scan and remains heuristic, not an equivalent completeness guarantee. CMINPACK rows in the historical root source were excluded from the fixed GSL comparison.

The paired ratio is `GSL per-call median / NUMERIX per-call median`; values above one favor NUMERIX. These are descriptive historical summaries, not hypothesis tests or Phase 14C results:

| Method | GSL / NUMERIX median time ratio | Problem bootstrap 95% interval | GSL accuracy passes | NUMERIX accuracy passes |
|---|---:|---:|---:|---:|
| Bisection | 1.081 | [1.069, 1.098] | —/— | 94/100 |
| Regula Falsi | 1.025 | [0.991, 1.034] | —/— | 95/100 |
| Secant | 1.065 | [1.056, 1.079] | —/— | 100/100 |
| Newton-Raphson | 1.077 | [1.064, 1.094] | —/— | 100/100 |
| Safeguarded Newton | 1.139 | [1.126, 1.166] | —/— | 95/100 |
| Brent | 1.037 | [1.021, 1.058] | —/— | 95/100 |
| Multiple-root discovery | 0.535 | [0.524, 0.798] | —/— | 100/100 |

Historical accuracy-pass counts (denominator includes the attempted method records) are:

| Method | NUMERIX | GSL |
|---|---:|---:|
| Bisection | 94/100 | —/— |
| Regula Falsi | 95/100 | —/— |
| Secant | 100/100 | —/— |
| Newton-Raphson | 100/100 | —/— |
| Safeguarded Newton | 95/100 | —/— |
| Brent | 95/100 | —/— |
| Multiple-root discovery | 100/100 | —/— |

Root memory is unavailable at method scope. Historical source and run hashes are retained in the manifests; the original Phase 14B files were not relabeled or rewritten.

### Linear algebra

The audited inventory has 18 numerical operations and three lifecycle groups. The accepted pilot run, `phase14c-la-pilot-20261112T055827Z`, covers matrix addition, matrix multiplication, LU factorization, and LU solve with four problem definitions, eight library routes, 31 batches per route, 8/8 accuracy passes, and 2/2 setup checks. The route-specific competitor/NUMERIX per-call ratios are 1.172 for matrix addition, 0.348 for matrix multiplication, 0.363 for LU factorization, and 1.768 for LU solve, each on **one** workload. These ratios only describe those four pilot inputs and cannot establish which library is generally faster. DGETRF end-to-end includes row-major/column-major conversion; LU solve uses each library's saved factors.

The remaining 14 numerical operations were not piloted. The 8 pilot memory rows are explicitly unavailable. OpenBLAS ran with one thread. The runtime build actually used LAPACK 3.12.0 despite the old phase-level manifest's 3.12.1 value.

### Numerical differentiation

The three-problem GSL 2.8 pilot recorded 1,116 timing rows: 36 routes/problems/accuracy records with 31 timing batches per method/problem/library route. All 36 calls returned successful finite results; 30/36 met the strict configured accuracy target. Forward and backward NUMERIX difference records missed that pilot target (0/3 each), while GSL passed those six records. Central, five-point, Richardson, and second-derivative records passed in the pilot. The second-derivative competitor is a wrapper finite-difference baseline, not a native GSL second-derivative API. The piecewise central-difference method was audited but not measured and has no verified GSL equivalent.

This result is useful for identifying step-size and accuracy issues in the adapter. Three problems do not support a module-wide speed or accuracy conclusion.

### Interpolation

The pilot covered linear interpolation, Lagrange evaluation, Newton construction, and Newton evaluation on three cases each. It retained 744 timing rows with 31 batches per route. All 18 numerical evaluation calls returned status success, but only 4/18 met the accuracy target: linear 0/6, Lagrange 2/6, Newton evaluation 2/6 across both libraries. The six setup records for Newton construction have no numerical-output accuracy result and are marked N/A. Construction and evaluation costs are separate routes. These three cases do not support a general ranking.

### Numerical integration

The pilot covered composite trapezoid, composite Simpson, and adaptive Simpson on three inputs each and retained steady/setup and end-to-end measurements separately. All 18 accuracy records had successful statuses; 14/18 met the configured gate. The fixed GSL route uses 65-point Gauss-Legendre, not a composite trapezoid/Simpson algorithm; adaptive Simpson versus QAG also uses different work policies. Those are best-available comparisons and cannot be read as identical-method speed contests. Piecewise trapezoidal integration was inventoried but not measured.

### Optimization

NLopt 2.10.1 passed runtime version enforcement. The one-case correctness pilot generated four records, all passing the independent minimizer/objective gate. Golden-section versus DIRECT-L used a common 200-objective-call budget, but the NUMERIX API has no matching initial-point control. NLopt DIRECT-L returned native status 5 (`MAXEVAL_REACHED`), recorded as `limit_reached` and `converged=0` even though its candidate passed the accuracy gate. Gradient descent and L-BFGS both converged on the pilot case; objective and gradient call counts are kept separate. No timing or memory values were collected.

### ODE solving

SUNDIALS 7.9.0 ARKODE ERKStep tableaus were checked against NUMERIX Euler, Heun, and classical RK4. One two-state analytic system was evaluated at all 21 output points. Relative max trajectory errors (scaled by the maximum analytic state on the grid) were about 8.95e-2 for Euler, 3.09e-3 for Heun, and 1.53e-6 for RK4; NUMERIX and SUNDIALS trajectories differed by at most 2.66e-15, 3.55e-15, and 7.11e-15 respectively. SUNDIALS reports one extra startup RHS evaluation (21/41/81 versus NUMERIX 20/40/80). The pilot did not define an accuracy threshold, so accuracy pass/fail is `not_evaluated`. No timing or memory values were collected.

## Memory measurement and status

The interposer tracks requested bytes through `malloc`, `calloc`, `realloc`, `aligned_alloc`, `memalign`, `posix_memalign`, and `free`. It uses a static pointer table and compiles under `-Wall -Wextra -Wpedantic -Werror`. The synthetic positive control reports 970 B peak / 1,090 B total requested with success/failure realloc and aligned-allocation controls; GSL 2.8 `gsl_vector_alloc(100)` reports 856 B; NUMERIX `vector_add` on 100 values reports 816 B. These are **visibility controls**, not algorithm memory results.

Direct `mmap`, custom allocators, some libc allocation paths, and separately loaded `LD_PRELOAD` behavior are not validated. Positive controls through OpenBLAS/LAPACK, NLopt, and SUNDIALS are outstanding. Therefore all seven method-memory graphs state “not measured,” and every memory conclusion remains unavailable.

## Graphs, raw data, and reproducibility

Each module has exactly the three requested figures: runtime, method-specific memory, and numerical accuracy. Each figure is saved as PNG and SVG. The artifact generator is [`generate_comparison_artifacts.py`](../../../../phase14c/generate_comparison_artifacts.py); the audit/workload manifest generator is [`prepare_phase14c_manifest.py`](../../../../phase14c/prepare_phase14c_manifest.py). Raw pilot CSVs and run metadata are preserved by run ID under `../raw/`; historical root data remain in their original Phase 14B directory and are referenced by checksum.

The separately filtered [executed-method inventory and figures](../../executed_methods/README.md) include only routes with run records. They include historical root findings, bounded timing pilots, and correctness-only pilots with their evidence type labeled.

Three combined all-module figures for [runtime, memory, and accuracy](../../executed_methods/combined/README.md) provide the broad overview. Their captions identify unavailable data and the limits on cross-module comparisons.

Processed per-method summaries and paired problem-level timing ratios are under `../analysis/<module>/`. The statistical unit is the problem; timing batches are first reduced to a within-problem median. Bootstrap intervals resample problem ratios, but three-problem and single-problem pilots are too small to support broad inference. All accuracy failures, not-applicable setup records, convergence/native statuses, and missing routes remain visible in the CSV summaries or figure annotations.

Verification files:

- [`benchmark_verification.json`](../verification/benchmark_verification.json) — raw-source counts and study limits.
- [`graph_verification.json`](../verification/graph_verification.json) — exactly 21 figures, 21 verified PNGs, 21 valid SVGs.
- [`workload_intake_verification.json`](../manifests/workload_intake_verification.json) — 700 candidate inputs and 5,000 method/problem planning rows.
- [`source_checksums.txt`](../manifests/source_checksums.txt) — checksums for the source, raw, processed, and figure artifacts.
- [`methodology_audit.md`](../../../../phase14c/methodology_audit.md) — independent fairness, statistics, provenance, and memory review.

The existing presentation directory was not modified during Phase 14C. No dashboard was created, and no repository changes were committed or pushed.

## What remains before this can be called a complete benchmark

1. Run every applicable method on the frozen 100-problem/20-family set in every module, including all remaining linear-algebra routes and the NUMERIX-only piecewise inputs.
2. Establish and record independent high-precision/analytic references, common accuracy targets, applicability, native status, and work counts for every attempted route. Define an ODE accuracy gate before classifying pass/fail.
3. Validate allocator visibility through each actual GSL, OpenBLAS/LAPACK, NLopt, and SUNDIALS route; then collect isolated per-operation memory samples with symmetric input/output/workspace scopes. Keep an unavailable status whenever visibility fails.
4. Reconcile full raw timing, accuracy, memory, and process status against the frozen method/workload manifest; record timer resolution and CPU affinity; regenerate figures and report from the final data.

The current package is reproducible pilot evidence and a verified inventory. It does not answer the requested final all-module “which is faster, smaller, or more accurate” question.
