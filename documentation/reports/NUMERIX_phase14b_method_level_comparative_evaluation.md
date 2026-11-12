# NUMERIX Phase 14B — Method-Level Comparative Evaluation

**Status: PARTIAL.** The API audit covers all seven modules; method-level measurement and edge-case work currently covers root finding only.

## Executive summary

- 49 public method/operation rows audited across seven modules.
- Root run: 100 frozen problems per method route; 17 library-method routes; 1,700 accuracy records; 52,700 raw timing batches.
- Root edges: 19 cases; 322 measured library-method outcomes; 6,762 timing batches; one CMINPACK discontinuity route is explicitly unsupported/not run.
- Libraries measured for roots: NUMERIX, GSL 2.8, and CMINPACK 1.3.15. LAPACK reference, LAPACK/OpenBLAS, NLopt, ARKODE, and CVODE are present in the capability map but not measured at method level in Phase 14B.
- The study reports no overall library winner. Root discovery is a best-available heuristic comparison.

## Direct root comparisons

The statistical unit is the problem. A pair is retained only when both implementations are applicable, successful, accurate, and have positive wall measurements. Confidence intervals use a 5,000-resample paired problem bootstrap. No hypothesis test or familywise significance claim is made; Holm correction was considered but not applied.

| Method | Eligible pairs | Excluded | Median GSL / NUMERIX | Problem bootstrap 95% interval |
|---|---:|---:|---:|---:|
| bisection | 94 | 6 | 1.08 | [1.07, 1.1] |
| regula_falsi | 95 | 5 | 1.02 | [0.987, 1.04] |
| secant | 85 | 15 | 1.08 | [1.06, 1.09] |
| newton | 90 | 10 | 1.07 | [1.06, 1.08] |
| brent | 95 | 5 | 1.04 | [1.02, 1.06] |

A ratio above one means GSL's median call time was higher on the eligible paired problems. These results are descriptive and do not claim statistical significance.

## Root method findings

- **bisection:** NUMERIX 95/100 successful status and 94/100 accuracy passes; median all-attempt wall call time 1.21 µs.
  GSL: 95/100 successful and 95/100 accuracy passes; median 1.33 µs.
- **regula_falsi:** NUMERIX 95/100 successful status and 95/100 accuracy passes; median all-attempt wall call time 1.39 µs.
  GSL: 95/100 successful and 95/100 accuracy passes; median 1.35 µs.
- **secant:** NUMERIX 100/100 successful status and 100/100 accuracy passes; median all-attempt wall call time 1.46 µs.
  GSL: 100/100 successful and 85/100 accuracy passes; median 1.61 µs.
- **newton:** NUMERIX 100/100 successful status and 100/100 accuracy passes; median all-attempt wall call time 1.46 µs.
  GSL: 100/100 successful and 90/100 accuracy passes; median 1.55 µs.
  CMINPACK best-available hybrid solve: 96/100 successful and 96/100 accuracy passes; median 1.83 µs.
- **safeguarded_newton:** NUMERIX 95/100 successful status and 95/100 accuracy passes; median all-attempt wall call time 1.27 µs.
  GSL: 95/100 successful and 95/100 accuracy passes; median 1.48 µs.
- **brent:** NUMERIX 95/100 successful status and 95/100 accuracy passes; median all-attempt wall call time 1.4 µs.
  GSL: 95/100 successful and 95/100 accuracy passes; median 1.43 µs.
- **auto_selection:** NUMERIX 100/100 successful status and 100/100 accuracy passes; median all-attempt wall call time 3.05 µs.
- **portfolio:** NUMERIX 100/100 successful status and 100/100 accuracy passes; median all-attempt wall call time 5.55 µs.
- **multiple_root_discovery:** NUMERIX 100/100 successful status and 100/100 accuracy passes; median all-attempt wall call time 63.7 µs.
  GSL: 100/100 successful and 94/100 accuracy passes; median 31.9 µs.
- **NUMERIX multiple-root discovery:** 100 detected of 100 reference roots; 0 missed and 0 false positives. GSL uses a fixed 4096-panel sign-change scan, not a root-discovery API.
- **GSL multiple-root discovery:** 94 detected of 100 reference roots; 6 missed and 0 false positives. GSL uses a fixed 4096-panel sign-change scan, not a root-discovery API.

## Edge-case results

The edge ledger records exact inputs, expected behavior, status, accuracy, residual, work, and observation. Correct rejections are not automatically failures. NUMERIX discovery cancellation is distinguished from solver failure; its evaluation cap is not treated as equivalent to GSL because GSL's API does not expose the same per-call budget.

Edge outcomes: [edge_outcomes.csv](../../benchmarks/results/phase14b/analysis/root_edges/edge_outcomes.csv). Raw outcomes: [accuracy.csv](../../benchmarks/results/phase14b/edge_cases/root/accuracy.csv), [raw_timing.csv](../../benchmarks/results/phase14b/edge_cases/root/raw_timing.csv), [method workload map](../../benchmarks/results/phase14b/edge_cases/root/method_workloads.csv). Invalid expression compilation was checked separately in [expression_invalid_probe.csv](../../benchmarks/results/phase14b/edge_cases/root/expression_invalid_probe.csv).

## Measurement protocol

Normal root study: 31 independent batches per problem-method-library route, two warmups, four repeats per batch for iterative algorithms, two for orchestration, and one for discovery. Work counters were captured only in an untimed probe. Edge suite: 21 batches per measured case route. Solver timing excludes process startup, parsing, and result-file I/O. The common root-solver cap is 500 iterations; cap and evaluation-budget fixtures are retained as explicit edge records.

## Figures

All charts have PNG and SVG versions under [benchmarks/results/phase14b/figures/root](../../benchmarks/results/phase14b/figures/root/). The interactive [HTML report](NUMERIX_phase14b_method_level_comparative_evaluation.html) embeds the method-level charts and includes local filters and nested library/method accordions.

## Memory, time-to-accuracy, and limitations

No method-specific heap interception or memory comparison was completed. Phase 14 RSS includes harness/process baseline and is not used as library-specific memory evidence. No tolerance sweeps were performed, so time-to-accuracy is unavailable. CPU samples remain in raw records; zero-resolution samples are excluded from positive CPU summaries.

The linear algebra, differentiation, interpolation, integration, optimization, and ODE method-level benchmark modules remain pending. CMake is absent from the current shell; the Phase 14B adapter compiled manually with GCC 8.5.0, C11, `-O3 -DNDEBUG -Wall -Wextra -Wpedantic`, and pinned local GSL/CMINPACK libraries. The CMake configuration is saved for reproduction where CMake is available.

## Reproducibility artifacts

- Root requirements: [`a.md`](../../a.md)
- Initial method inventory: [`method_coverage.csv`](../../benchmarks/analysis/phase14b/method_coverage.csv)
- Per-library capability map: [`library_method_capabilities.csv`](../../benchmarks/results/phase14b/manifests/library_method_capabilities.csv)
- Root raw run: [`root-method-study-final-v3-20261112`](../../benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/)
- Root analysis: [`analysis/root`](../../benchmarks/results/phase14b/analysis/root/)
- Edge analysis: [`analysis/root_edges`](../../benchmarks/results/phase14b/analysis/root_edges/)
- The Phase 13 checksum inventory and prior Phase 14 data remain preserved. No commit, push, or version change was made.

**Final Phase 14B status: PARTIAL.** The dashboard shows all methods and library capabilities, but six modules, time-to-accuracy, and method-scoped memory remain unfinished.
