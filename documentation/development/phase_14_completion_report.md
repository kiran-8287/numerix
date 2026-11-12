# NUMERIX Phase 14 Completion Report

**Phase status: PARTIAL — primary run and verification are complete; broad method-level coverage requirements remain unmet.**

## Executive summary

The work completed a reproducible seven-module Phase 14 primary benchmark against GSL and specialist libraries. It contains 700 distinct problem instances (100 per module, 20 families per module), 2,000 configured library/problem pairs, 62,000 timing batches (31 per pair), 2,000 accuracy records, 8,000 computational-work records, and 2,000 isolated-process RSS probes. There were no launch/write failures.

The run met the minimum distinct problem count and current adapter pairing coverage. It is not a complete method-by-method evaluation of NUMERIX's public API. The benchmark omits multiple methods and operation phases requested in the pasted brief; requested heap allocation is unavailable, and RSS is dominated by the process/library baseline. These gaps are documented in the full comparative report. No Phase 14 commit, push, tag, or version bump was made.

## Libraries and measured coverage

| Module | Distinct problems | Configured libraries (100 IDs each) |
| --- | ---: | --- |
| Root finding | 100 | NUMERIX, GSL, CMINPACK |
| Linear algebra | 100 | NUMERIX, GSL, reference LAPACK/LAPACKE + reference BLAS, LAPACK/LAPACKE + OpenBLAS |
| Numerical differentiation | 100 | NUMERIX, GSL |
| Interpolation | 100 | NUMERIX, GSL |
| Numerical integration | 100 | NUMERIX, GSL |
| Optimization | 100 | NUMERIX, GSL, NLopt |
| ODE solving | 100 | NUMERIX, GSL, SUNDIALS ARKODE, SUNDIALS CVODE |

The library capability matrix records versions, APIs, licenses, source/documentation URLs, compiler/build options, comparison classes, and exclusions. All specialist dependencies are optional and remain outside the NUMERIX production library.

## Main reliability and accuracy results

| Module | Library | Status success | Converged | Accuracy pass |
| --- | --- | ---: | ---: | ---: |
| Root | NUMERIX | 100/100 | 100/100 | 100/100 |
| Root | GSL | 100/100 | 100/100 | 100/100 |
| Root | CMINPACK | 96/100 | 96/100 | 100/100 |
| Linear algebra | NUMERIX | 87/100 | 87/100 | 76/100 |
| Linear algebra | GSL | 95/100 | 95/100 | 76/100 |
| Linear algebra | Reference LAPACK | 95/100 | 95/100 | 76/100 |
| Linear algebra | LAPACK + OpenBLAS | 95/100 | 95/100 | 76/100 |
| Differentiation | NUMERIX | 100/100 | 100/100 | 82/100 |
| Differentiation | GSL | 100/100 | 100/100 | 100/100 |
| Interpolation | NUMERIX | 100/100 | 100/100 | 38/100 |
| Interpolation | GSL | 100/100 | 100/100 | 38/100 |
| Integration | NUMERIX | 95/100 | 95/100 | 95/100 |
| Integration | GSL | 100/100 | 100/100 | 100/100 |
| Optimization | NUMERIX | 100/100 | 100/100 | 100/100 |
| Optimization | GSL | 95/100 | 95/100 | 100/100 |
| Optimization | NLopt | 0/100 | 0/100 | 100/100 |
| ODE | NUMERIX, GSL, ARKODE, CVODE | 100/100 each | 100/100 each | 100/100 each |

NLopt returned minimizer locations that met the accuracy gate in all 100 cases, but every run exhausted its 4,000-evaluation cap. Its output accuracy and failed convergence status remain separate; there is no status-and-accuracy-qualified NLopt speed estimate.

## Paired performance findings

The speed ratio is competitor median wall time divided by NUMERIX median wall time; values above one favor NUMERIX. Ratios require both implementations to report successful status and pass accuracy. Confidence intervals resample paired problem IDs.

| Module/comparison | Qualified pairs | Median ratio (95% CI) | Reading |
| --- | ---: | ---: | --- |
| Root — GSL, equivalent | 100 | 0.958 (0.796–1.046) | Median interval spans parity; geometric-mean result is more GSL-favorable. |
| Root — CMINPACK, best available | 96 | 1.763 (1.594–1.940) | NUMERIX faster on these valid local-solver pairs. |
| Linear algebra — GSL | 76 | 0.950 (0.716–1.056) | No clear median direction. |
| Linear algebra — reference LAPACK | 76 | 0.833 (0.753–0.944) | LAPACK faster on these combined dense solves. |
| Linear algebra — LAPACK + OpenBLAS | 76 | 0.868 (0.478–0.980) | OpenBLAS-backed LAPACKE faster on these combined solves. |
| Differentiation — GSL, best available | 82 | 1.510 (1.335–1.641) | NUMERIX faster on the subset where both results are accurate. |
| Interpolation — GSL, best available | 38 | 1.067 (0.990–1.118) | Interval spans parity; low accuracy pass count limits the result. |
| Integration — GSL, best available | 95 | 0.350 (0.308–0.490) | GSL faster on this adaptive quadrature workload. |
| Optimization — GSL, best available | 95 | 2.317 (2.247–2.388) | NUMERIX faster on status- and accuracy-qualified cases. |
| Optimization — NLopt | 0 | Not estimable | All 100 trials exhausted the evaluation budget. |
| ODE — GSL, fixed RK4 equivalent | 100 | 1.953 (1.883–2.024) | NUMERIX faster on the matched fixed-step path. |
| ODE — ARKODE, fixed RK4 equivalent | 100 | 5.767 (5.473–6.102) | NUMERIX faster on this adapter-level matched path. |
| ODE — CVODE, best available | 100 | 4.678 (4.141–5.160) | NUMERIX faster here; CVODE is adaptive and method-mismatched. |

These results are confined to the tested methods and inputs. They do not establish a universal winner.

## Measurements, reports, and figures

- Raw wall and CPU time: 62,000 batches, 31 per library/problem pair.
- Accuracy/status: 2,000 library/problem observations; failures remain in the data.
- Computational work: 8,000 records, with missing/inapplicable values explicitly distinguished from zero.
- Memory: 2,000 isolated-process peak RSS values, typically about 9.7 MiB; this includes the harness and shared-library baseline. Requested heap/allocation instrumentation is unavailable.
- Figures: 22 plots, each in PNG and SVG, under benchmarks/results/phase14/main/figures/.
- Full report: [Markdown](../reports/NUMERIX_expanded_multi_library_comparative_evaluation.md) and [HTML](../reports/NUMERIX_expanded_multi_library_comparative_evaluation.html).
- Reproducible report generator: benchmarks/analysis/phase14/generate_report.py.

## Verification

- Workload validation: **PASS** — 700 unique problem/input payloads, 100 per module, 20 families per module, five variants per family, references and input hashes checked.
- Primary analysis: **PASS** — 2,000 expected/observed pairs, 62,000 timing rows, exactly 31 batches per pair, 2,000 memory rows, 8,000 work records, no duplicate rows, missing pairs, or launch failures.
- Phase 13 preservation: **PASS** — all 125 entries in benchmarks/results/phase14/logs/phase13_preservation.sha256 verify.
- make test: **PASS**, including module tests and release, accuracy, Phase 13B, and benchmark measurement regressions.
- CMake Release build with BUILD_TESTING=ON: **PASS**; CTest: **35/35 passed**.
- Phase 14 Python scripts compile; workload validation and raw-result analysis pass. Report regeneration is byte-for-byte deterministic; all 22 PNG and 22 SVG files parse, and all 61 local HTML links resolve.
- Valgrind 3.22.0 leak/error checks passed on one NUMERIX adapter path in each of the seven modules. Address/UndefinedSanitizer configuration was attempted but the GCC runtime link failed because the environment lacks the expected libasan/libubsan runtime files.
- git diff --check: included in the final audit; no production numerical source, public API, or version change is included.

## Remaining scope gaps

The following mandatory method groups were not included in the primary run:

- Root methods beyond selected Brent/Newton paths and no multiple-root discovery-recall study.
- Vector/matrix primitives, NUMERIX LU factorization, separate factorization/solve timings, and comprehensive backward-error analysis.
- Second derivatives and NUMERIX higher-order/Richardson differentiation paths.
- Linear/Lagrange interpolation alternatives and construction/evaluation phase separation.
- Fixed trapezoid/Simpson integration.
- NUMERIX gradient-descent optimization.
- NUMERIX Euler/Heun ODE paths and trajectory-wide error scoring.
- Heap allocation measurement.

The Phase 14 study remains **partial** until those method-level requirements are addressed in a separately preregistered extension. Current work is uncommitted and unpushed, as directed by the saved brief.
