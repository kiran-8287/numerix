# NUMERIX Phase 14 — Expanded Multi-Library Comparative Evaluation

**Study date:** 12 November 2026<br>
**Primary run:** phase14-main-20261112<br>
**Project version:** 0.2.0-rc1<br>
**Study status:** **Partial, with the problem-count floor and paired-run coverage met.**

## 1. Executive summary

The primary run exercised 700 distinct mathematical problems: 100 in each of NUMERIX's seven modules, drawn from 20 families per module. Every configured library/module combination ran the same 100 stable problem IDs. The run contains 2,000 library/problem accuracy records, 62,000 raw wall/CPU timing batches (31 per pair), 8,000 work-metric records, and 2,000 isolated-process RSS probes. There were no launch or output-write failures.

The evidence does not meet every broad method-coverage requirement in the supplied Phase 14 brief. This run measures one selected method family per module and omits several required NUMERIX methods and operation-level phases. CPU and wall timing are retained, but paired inference is based on wall time. Process RSS includes the harness/library baseline and requested heap allocation could not be instrumented. The study is therefore partial and does not claim comprehensive evaluation of every method or a universal winner.

NUMERIX passed accuracy on all 100 root and ODE cases; 82 differentiation, 76 linear-system, 38 interpolation, 95 integration, and 100 optimization cases. NLopt returned accurate minimizers for all 100 optimization cases but exhausted its 4,000-evaluation limit in all 100 and had zero successful convergence statuses. Those NLopt trials do not support a valid speed claim. The main numerical limitations exposed here are differentiation accuracy, global-polynomial interpolation, difficult linear systems, and inverse-square-root endpoint integration.

## 2. NUMERIX overview and repository state

The benchmark used the existing 0.2.0-rc1 public C API and did not modify production algorithms, public headers, mathematical behavior, or project version. NUMERIX currently exposes scalar root finding; vector/matrix operations, dense solves and LU; first/second finite differences; linear/Lagrange/Newton interpolation; finite trapezoid/Simpson/adaptive Simpson quadrature; bounded one-dimensional golden-section and vector gradient descent; and vector-valued fixed-step Euler, Heun and RK4 ODE methods. This run measured only the selected paths below.

The repository was on branch master at commit 2292ee804b02bd82be56803a6eae9d36327ef117 at run time. Phase 13 files are preserved and have a SHA-256 inventory. The full pasted request is copied to [PHASE14_REQUIREMENTS.md](../../PHASE14_REQUIREMENTS.md). The brief forbids a Phase 14 commit, push, tag, or version change.

## 3. Competitor libraries

| Library/configuration | Modules | Measured role |
| --- | --- | --- |
| GSL 2.8 | all seven | Broad native-C comparator; actual methods are listed below. |
| CMINPACK 1.3.15 | root | One-variable MINPACK hybrid solver; best-available local method, no bracket guarantee. |
| Reference LAPACK/LAPACKE 3.12.1 + reference BLAS | linear algebra | Row-major LAPACKE dgesv combined dense solve. |
| LAPACK/LAPACKE 3.12.1 + OpenBLAS 0.3.34 | linear algebra | OpenBLAS-backed LAPACKE dgesv; one runtime thread. |
| NLopt 2.11.0 | optimization | Bounded one-dimensional DIRECT-L best available. |
| SUNDIALS 7.9.0 ARKODE | ODE | Fixed-step Kutta-RK4a tableau verified against classical RK4. |
| SUNDIALS 7.9.0 CVODE | ODE | Adaptive Adams with serial fixed-point solver; best available. |

All competitor builds are optional benchmark dependencies. Versions, licenses, official URLs, build flags, linked BLAS, API type, exclusions and limitations are in the [library capability matrix](../development/phase_14_library_capability_matrix.md) and [libraries.csv](../../benchmarks/analysis/phase14/libraries.csv). ALGLIB and Boost.Math were excluded from the native-C run due to language/API fit; Cuba targets multidimensional integration; Cephes is not a comparable seven-module package.

## 4. Experimental methodology

The workload seed is 20261112. Each module has 20 named families and five distinct parameterized inputs per family. Stable IDs, SHA-256-linked input data, references, tolerances, difficulty, expected behavior, eligible libraries and method metadata are saved. References use closed forms, known constructed solutions, 70-digit mpmath, or independent SciPy DOP853 with a halved-step check for nonlinear ODEs. A harmonic-oscillator reference formula was corrected during pilot validation before the primary run; IDs and input hashes were retained.

The primary build used gcc (GCC) 8.5.0 20210514 (Red Hat 8.5.0-30.0.1), -O3 -DNDEBUG, no -ffast-math, double precision, and single-threaded libraries. The machine was a01-109-119l (Linux-5.15.0-326.222.4.el8uek.x86_64-x86_64-with-glibc2.28; x86_64, 4 logical CPUs). Each pair had 31 timing batches after warm-up/calibration; library execution order was deterministically shuffled with seed 20261112. Raw rows contain CLOCK_MONOTONIC wall time and CLOCK_PROCESS_CPUTIME_ID CPU time; per-call summaries are in measurements.csv. Paired inference uses wall time.

Mode A means algorithm-equivalent; Mode B means best-available but algorithmically different. Categories are not pooled. The speed ratio is competitor median wall time / NUMERIX median wall time, so above 1 favors NUMERIX. Only pairs where both libraries returned successful status and both passed the frozen accuracy gate contribute to speed estimates. Confidence intervals are 95% paired-problem bootstrap intervals (10,000 resamples); two-sided Wilcoxon tests on log ratios are exploratory and not adjusted for multiple comparisons.

## 5. Benchmark coverage

| Module | Problems | Families | Library/problem pairs | Accuracy records | Timing batches |
| --- | --- | --- | --- | --- | --- |
| Root finding | 100 | 20 | 300 | 300 | 9300 |
| Linear algebra | 100 | 20 | 400 | 400 | 12400 |
| Numerical differentiation | 100 | 20 | 200 | 200 | 6200 |
| Interpolation | 100 | 20 | 200 | 200 | 6200 |
| Numerical integration | 100 | 20 | 200 | 200 | 6200 |
| Optimization | 100 | 20 | 300 | 300 | 9300 |
| ODE solving | 100 | 20 | 400 | 400 | 12400 |

Every configured competitor has 100 common IDs in its applicable module. Status, convergence and accuracy are separate measurements.

| Module | Library | Observed | Status | Converged | Accurate | Timing batches |
| --- | --- | --- | --- | --- | --- | --- |
| Numerical differentiation | GSL 2.8 | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| Numerical differentiation | NUMERIX | 100 | 100/100 (100%) | 100/100 (100%) | 82/100 (82%) | 3100 |
| Numerical integration | GSL 2.8 | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| Numerical integration | NUMERIX | 100 | 95/100 (95%) | 95/100 (95%) | 95/100 (95%) | 3100 |
| Interpolation | GSL 2.8 | 100 | 100/100 (100%) | 100/100 (100%) | 38/100 (38%) | 3100 |
| Interpolation | NUMERIX | 100 | 100/100 (100%) | 100/100 (100%) | 38/100 (38%) | 3100 |
| Linear algebra | GSL 2.8 | 100 | 95/100 (95%) | 95/100 (95%) | 76/100 (76%) | 3100 |
| Linear algebra | LAPACK/LAPACKE + OpenBLAS | 100 | 95/100 (95%) | 95/100 (95%) | 76/100 (76%) | 3100 |
| Linear algebra | Reference LAPACK/LAPACKE | 100 | 95/100 (95%) | 95/100 (95%) | 76/100 (76%) | 3100 |
| Linear algebra | NUMERIX | 100 | 87/100 (87%) | 87/100 (87%) | 76/100 (76%) | 3100 |
| ODE solving | GSL 2.8 | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| ODE solving | NUMERIX | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| ODE solving | SUNDIALS ARKODE 7.9.0 | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| ODE solving | SUNDIALS CVODE 7.9.0 | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| Optimization | GSL 2.8 | 100 | 95/100 (95%) | 95/100 (95%) | 100/100 (100%) | 3100 |
| Optimization | NLopt 2.11.0 | 100 | 0/100 (0%) | 0/100 (0%) | 100/100 (100%) | 3100 |
| Optimization | NUMERIX | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| Root finding | CMINPACK 1.3.15 | 100 | 96/100 (96%) | 96/100 (96%) | 100/100 (100%) | 3100 |
| Root finding | GSL 2.8 | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |
| Root finding | NUMERIX | 100 | 100/100 (100%) | 100/100 (100%) | 100/100 (100%) | 3100 |

## 6. Results by module

### Root finding

**Methods measured:** NUMERIX Brent, or Newton with analytic derivative for designated multiple-root families; GSL Brent/Newton; CMINPACK hybrd1 as a one-variable nonlinear system.

**NUMERIX:** status 100/100; converged 100/100; accurate 100/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| CMINPACK 1.3.15 | B: best available | 100 | 100 | 96 | 1.76 (1.59, 1.94) |
| GSL 2.8 | A: equivalent | 100 | 100 | 100 | 0.958 (0.796, 1.05) |
GSL's pooled median interval spans parity. CMINPACK is a local nonlinear-system comparison, not a bracketed-root equivalent.

### Linear algebra

**Methods measured:** Dense Gaussian-elimination solve through NUMERIX, GSL LU decomposition/solve, and LAPACKE dgesv with reference BLAS or OpenBLAS.

**NUMERIX:** status 87/100; converged 87/100; accurate 76/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| GSL 2.8 | B: best available | 100 | 76 | 76 | 0.95 (0.716, 1.06) |
| LAPACK/LAPACKE + OpenBLAS | B: best available | 100 | 76 | 76 | 0.868 (0.478, 0.98) |
| Reference LAPACK/LAPACKE | B: best available | 100 | 76 | 76 | 0.833 (0.753, 0.944) |
All paths are combined dense solves. This run does not isolate factorization, solve-only, BLAS primitives, or a full backward-error comparison. Row-major LAPACKE conversion is included end to end.

### Numerical differentiation

**Methods measured:** NUMERIX two-point central difference and GSL gsl_deriv_central. First derivatives only in this measured path.

**NUMERIX:** status 100/100; converged 100/100; accurate 82/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| GSL 2.8 | B: best available | 100 | 82 | 82 | 1.51 (1.33, 1.64) |
NUMERIX accuracy is 82/100 under the preregistered gate. Speed comparison uses only successful and accurate pairs and does not imply greater overall accuracy.

### Interpolation

**Methods measured:** NUMERIX global Newton polynomial evaluation and GSL gsl_interp_polynomial evaluation.

**NUMERIX:** status 100/100; converged 100/100; accurate 38/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| GSL 2.8 | B: best available | 100 | 38 | 38 | 1.07 (0.99, 1.12) |
Both global-polynomial implementations pass only 38/100 under the strict frozen gate. Conclusions apply only to that passing subset.

### Numerical integration

**Methods measured:** NUMERIX adaptive Simpson and GSL QAG with a 61-point Gauss-Kronrod rule.

**NUMERIX:** status 95/100; converged 95/100; accurate 95/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| GSL 2.8 | B: best available | 100 | 95 | 95 | 0.35 (0.308, 0.49) |
NUMERIX has 95 successful/accurate cases; inverse-square-root endpoint stress cases account for its main misses. GSL QAG and adaptive Simpson are different algorithms.

### Optimization

**Methods measured:** NUMERIX bounded 1D golden section; GSL grid bracket scan plus golden section; NLopt bounded 1D DIRECT-L.

**NUMERIX:** status 100/100; converged 100/100; accurate 100/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| GSL 2.8 | B: best available | 100 | 100 | 95 | 2.32 (2.25, 2.39) |
| NLopt 2.11.0 | B: best available | 100 | 100 | 0 | not estimable: no qualifying pairs |
NLopt passes the location accuracy gate for 100 returned points but reports zero successful/converged statuses because all runs hit the 4,000-evaluation cap. It has no status+accuracy-qualified speed estimate. GSL has five status failures despite 100 accuracy passes.

### ODE solving

**Methods measured:** Fixed-step classical RK4 through NUMERIX, GSL odeiv2, and SUNDIALS ARKODE Kutta-RK4a; adaptive CVODE Adams is reported separately.

**NUMERIX:** status 100/100; converged 100/100; accurate 100/100. Each configured competitor completed all 100 input IDs.

| Competitor | Mode | Common IDs | Accuracy-qualified | Status + accuracy | Median speedup (95% CI) |
| --- | --- | --- | --- | --- | --- |
| GSL 2.8 | A: equivalent | 100 | 100 | 100 | 1.95 (1.88, 2.02) |
| SUNDIALS ARKODE 7.9.0 | A: equivalent | 100 | 100 | 100 | 5.77 (5.47, 6.1) |
| SUNDIALS CVODE 7.9.0 | B: best available | 100 | 100 | 100 | 4.68 (4.14, 5.16) |
Fixed-mesh RK4 comparisons use the matched classical tableau. Adaptive CVODE is Mode B and remains separate. All implementations pass all 100 status, convergence and accuracy checks.

## 7. Cross-module paired performance results

Only successful-status and accuracy-passing pairs enter these ratios. Above 1 favors NUMERIX. n counts distinct problem pairs; a missing estimate means no qualifying pairs, not zero speed.

| Module | Competitor | Mode | n | Median ratio | 95% problem CI | Wilcoxon p |
| --- | --- | --- | --- | --- | --- | --- |
| Numerical differentiation | GSL 2.8 | B | 82 | 1.51 | [1.33, 1.64] | 1.76e-12 |
| Numerical integration | GSL 2.8 | B | 95 | 0.35 | [0.308, 0.49] | 1.27e-08 |
| Interpolation | GSL 2.8 | B | 38 | 1.07 | [0.99, 1.12] | 0.0792 |
| Linear algebra | GSL 2.8 | B | 76 | 0.95 | [0.716, 1.06] | 0.183 |
| Linear algebra | LAPACK/LAPACKE + OpenBLAS | B | 76 | 0.868 | [0.478, 0.98] | 0.00306 |
| Linear algebra | Reference LAPACK/LAPACKE | B | 76 | 0.833 | [0.753, 0.944] | 6.01e-05 |
| ODE solving | GSL 2.8 | A | 100 | 1.95 | [1.88, 2.02] | 3.9e-18 |
| ODE solving | SUNDIALS ARKODE 7.9.0 | A | 100 | 5.77 | [5.47, 6.1] | 3.9e-18 |
| ODE solving | SUNDIALS CVODE 7.9.0 | B | 100 | 4.68 | [4.14, 5.16] | 3.9e-18 |
| Optimization | GSL 2.8 | B | 95 | 2.32 | [2.25, 2.39] | 1.88e-16 |
| Optimization | NLopt 2.11.0 | B | 0 | — | not estimable | — |
| Root finding | CMINPACK 1.3.15 | B | 96 | 1.76 | [1.59, 1.94] | 1.15e-12 |
| Root finding | GSL 2.8 | A | 100 | 0.958 | [0.796, 1.05] | 0.00647 |

These results are algorithm- and workload-specific. GSL was faster on the accuracy/status-qualified adaptive-integration subset; NUMERIX was faster on the qualified differentiation subset. Neither result generalizes to a whole library.

## 8. Accuracy analysis

Accuracy is tested against independent references using absolute and relative tolerances recorded in the manifest. Solver success and reported error estimates are not substitutes for actual error.

See the per-library table in Section 5, [accuracy.csv](../../benchmarks/results/phase14/main/accuracy.csv), and module figures below. Linear failures include singular/ill-conditioned systems; both global polynomial implementations have low pass rates at this strict tolerance; NUMERIX misses some differentiation and endpoint integration cases. ODE references were corrected before the main run and all 20 harmonic checks passed in validation.

## 9. Performance and computational effort

The run retained 62,000 raw batches with both clocks. 61,943 positive CPU batch measurements are included in per-call summaries in measurements.csv; 57 zero-resolution CPU timer deltas are retained in raw_timing.csv but excluded from those summaries. There are 94 grouped method/size summaries. Each of 2,000 library/problem pairs has 31 batches.

Function evaluations, iterations and ODE right-hand-side evaluations are recorded where exposed by the API. Missing and inapplicable work metrics are explicitly marked; they are not written as zero. Work summaries provide medians, means, p95 and maxima. There is no complete linear algebra operation decomposition or interpolation construction/evaluation timing split.

## 10. Memory analysis

There are 2,000 isolated-process getrusage(RUSAGE_SELF).ru_maxrss observations in KiB. Typical measurements are about 9.7 MiB and nearly flat because the process and shared-library baseline dominates. RSS includes the harness and setup; it is not solver-only memory. Requested heap bytes, allocation counts and live heap are unavailable and blank, not zero. These data do not establish a memory winner.

## 11. Reliability and convergence

The main run has 0 launch/write failures, and all planned adapter/problem outputs are represented. Numerical status failures remain in counts and raw records. NLopt's 0/100 successful statuses and 0/100 convergence are distinct from its 100/100 accurate returned locations.

## 12. Scalability

Input dimensions vary for selected workloads such as dense matrix order, interpolation nodes and ODE step counts. The overview plots size curves where the input has a meaningful size variable. Fixed-size scalar workloads use independent workload index and are not presented as size-scaling curves. This method-level scope cannot establish broad asymptotic behavior.

## 13. Main observations

- NUMERIX has strong status/accuracy outcomes in measured root, bounded optimization and ODE paths, while differentiation, interpolation, difficult linear systems and endpoint integration expose correctness work.
- GSL is the broad baseline. Specialist libraries add scoped comparisons; LAPACK/LAPACKE + OpenBLAS is accurately described as the LAPACKE path backed by OpenBLAS.
- NLopt's accurate output is not a converged result under the configured budget.
- RSS mostly measures process baseline. Heap instrumentation is still needed for useful library-memory attribution.
- Fixed-step RK4 and adaptive/best-available comparisons remain separate.

## 14. Limitations and unmet requirements

The primary run meets the 700 distinct-problem floor, 20 families/module, applicable competitors, and 100 paired inputs/current adapter. It does not comprehensively measure all public methods requested in the brief:

- Root: selected Brent/Newton paths only; no method-by-method bisection, regula falsi, secant, safeguarded-method, or multiple-root discovery recall study.
- Linear algebra: combined dense solve only; no vector/matrix primitives, NUMERIX LU factorization, factor-only/solve-only phases, or full backward-error comparison.
- Differentiation: first derivative central difference only; no second derivative, NUMERIX five-point/Richardson paths, or method-by-method coverage.
- Interpolation: global Newton polynomial only; no linear/Lagrange comparisons or construction/evaluation timing split.
- Integration: adaptive Simpson only; no fixed trapezoid/Simpson methods or separately preregistered common tolerance sweep.
- Optimization: one-dimensional golden section only; no NUMERIX gradient descent. NLopt's budget exhaustion leaves no successful speed estimate.
- ODE: fixed RK4 plus adaptive CVODE; no NUMERIX Euler/Heun comparison, and main accuracy gate uses endpoint rather than full trajectory norm.
- CPU time is retained and summarized but inferential comparisons use wall time. Memory is RSS-only; heap allocation instrumentation is unavailable.
- One Linux host, compiler and thread configuration do not establish cross-platform performance.

These gaps are why the study remains partial. A follow-on should add method-level workload/run matrices, operation-specific linear algebra timing/error, more differentiation and interpolation alternatives, fixed quadrature and optimization/ODE coverage, trajectory-wide ODE error, and allocation instrumentation.

## 15. Recommendations

1. Improve NUMERIX central-difference accuracy under curvature and floating-point-scale stress.
2. Investigate interpolation conditioning and node strategy while retaining the observed low pass rate as a correctness signal.
3. Investigate singular endpoint handling in adaptive Simpson.
4. Improve status/error reporting for singular and ill-conditioned systems.
5. Use a separately preregistered realistic NLopt budget; do not reinterpret the current exhausted runs as converged.
6. Extend the benchmark to every public method and add heap instrumentation before category-wide conclusions.

## 16. Reproducibility and artifacts

From repository root, validate the saved workload; preparation can regenerate the Phase 14 manifest and input sidecar:

    python3 benchmarks/analysis/phase14/verify_workloads.py

Regenerate analysis, figures and reports without rerunning timed library measurements:

    python3 benchmarks/analysis/phase14/analyze_results.py benchmarks/results/phase14/main
    python3 benchmarks/analysis/phase14/plot_results.py benchmarks/results/phase14/main
    python3 benchmarks/analysis/phase14/generate_report.py benchmarks/results/phase14/main

Analysis requires NumPy/SciPy, plotting requires NumPy/Matplotlib, and HTML conversion uses Python-Markdown. Adapter build details are in [README](../../benchmarks/adapters/phase14/README.md), [runner](../../benchmarks/analysis/phase14/run_benchmark.py), and the [execution log](../development/phase_14_audit_and_execution_log.md).

Evidence: [manifest](../../benchmarks/problems/phase14/workloads.csv), [input sidecar](../../benchmarks/problems/phase14/problem_inputs.jsonl), [raw timing](../../benchmarks/results/phase14/main/raw_timing.csv), [accuracy](../../benchmarks/results/phase14/main/accuracy.csv), [work metrics](../../benchmarks/results/phase14/main/work_metrics.csv), [RSS](../../benchmarks/results/phase14/main/memory.csv), [run metadata](../../benchmarks/results/phase14/main/run_metadata.json), [verification](../../benchmarks/results/phase14/main/analysis_verification.json), [coverage](../../benchmarks/results/phase14/main/coverage.csv), and [comparisons](../../benchmarks/results/phase14/main/comparisons.csv).

## Figures

### Differentiation error-estimate calibration

![Differentiation error-estimate calibration](../../benchmarks/results/phase14/main/figures/differentiation_error_estimate_calibration.png)

[SVG](../../benchmarks/results/phase14/main/figures/differentiation_error_estimate_calibration.svg)

### Differentiation error versus step

![Differentiation error versus step](../../benchmarks/results/phase14/main/figures/differentiation_error_vs_step.png)

[SVG](../../benchmarks/results/phase14/main/figures/differentiation_error_vs_step.svg)

### Differentiation error versus work

![Differentiation error versus work](../../benchmarks/results/phase14/main/figures/differentiation_error_vs_work.png)

[SVG](../../benchmarks/results/phase14/main/figures/differentiation_error_vs_work.svg)

### Integration error-estimate calibration

![Integration error-estimate calibration](../../benchmarks/results/phase14/main/figures/integration_error_estimate_calibration.png)

[SVG](../../benchmarks/results/phase14/main/figures/integration_error_estimate_calibration.svg)

### Integration error versus work

![Integration error versus work](../../benchmarks/results/phase14/main/figures/integration_error_vs_work.png)

[SVG](../../benchmarks/results/phase14/main/figures/integration_error_vs_work.svg)

### Integration evaluations versus tolerance

![Integration evaluations versus tolerance](../../benchmarks/results/phase14/main/figures/integration_evaluations_vs_tolerance.png)

[SVG](../../benchmarks/results/phase14/main/figures/integration_evaluations_vs_tolerance.svg)

### Interpolation error versus node count

![Interpolation error versus node count](../../benchmarks/results/phase14/main/figures/interpolation_error_vs_node_count.png)

[SVG](../../benchmarks/results/phase14/main/figures/interpolation_error_vs_node_count.svg)

### Linear conditioning and residual

![Linear conditioning and residual](../../benchmarks/results/phase14/main/figures/linear_algebra_conditioning_and_residual.png)

[SVG](../../benchmarks/results/phase14/main/figures/linear_algebra_conditioning_and_residual.svg)

### ODE accuracy by family

![ODE accuracy by family](../../benchmarks/results/phase14/main/figures/ode_accuracy_by_family.png)

[SVG](../../benchmarks/results/phase14/main/figures/ode_accuracy_by_family.svg)

### ODE error versus RHS evaluations

![ODE error versus RHS evaluations](../../benchmarks/results/phase14/main/figures/ode_error_vs_work.png)

[SVG](../../benchmarks/results/phase14/main/figures/ode_error_vs_work.svg)

### Optimization error versus work

![Optimization error versus work](../../benchmarks/results/phase14/main/figures/optimization_error_vs_work.png)

[SVG](../../benchmarks/results/phase14/main/figures/optimization_error_vs_work.svg)

### Optimization gap, work and runtime

![Optimization gap, work and runtime](../../benchmarks/results/phase14/main/figures/optimization_gap_work_runtime.png)

[SVG](../../benchmarks/results/phase14/main/figures/optimization_gap_work_runtime.svg)

### Coverage by module and library

![Coverage by module and library](../../benchmarks/results/phase14/main/figures/overview_01_coverage.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_01_coverage.svg)

### Status, convergence and accuracy

![Status, convergence and accuracy](../../benchmarks/results/phase14/main/figures/overview_02_status_convergence_accuracy.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_02_status_convergence_accuracy.svg)

### Status and convergence counts

![Status and convergence counts](../../benchmarks/results/phase14/main/figures/overview_03_reliability.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_03_reliability.svg)

### Status and accuracy-qualified speedups

![Status and accuracy-qualified speedups](../../benchmarks/results/phase14/main/figures/overview_04_paired_speedup.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_04_paired_speedup.svg)

### Runtime by module and available size axis

![Runtime by module and available size axis](../../benchmarks/results/phase14/main/figures/overview_05_runtime_scalability.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_05_runtime_scalability.svg)

### Isolated-process peak RSS

![Isolated-process peak RSS](../../benchmarks/results/phase14/main/figures/overview_06_memory_scalability.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_06_memory_scalability.svg)

### Module-specific error distributions

![Module-specific error distributions](../../benchmarks/results/phase14/main/figures/overview_07_accuracy_distribution.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_07_accuracy_distribution.svg)

### Wall time versus observed error

![Wall time versus observed error](../../benchmarks/results/phase14/main/figures/overview_08_time_to_accuracy.png)

[SVG](../../benchmarks/results/phase14/main/figures/overview_08_time_to_accuracy.svg)

### Root accuracy by difficulty

![Root accuracy by difficulty](../../benchmarks/results/phase14/main/figures/root_accuracy_by_difficulty.png)

[SVG](../../benchmarks/results/phase14/main/figures/root_accuracy_by_difficulty.svg)

### Root error versus function evaluations

![Root error versus function evaluations](../../benchmarks/results/phase14/main/figures/root_error_vs_work.png)

[SVG](../../benchmarks/results/phase14/main/figures/root_error_vs_work.svg)
