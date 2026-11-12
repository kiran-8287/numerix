# NUMERIX Project Showcase — Content Verification Report

**Date:** 2026-11-12  
**Scope:** `documentation/project_showcase/` only  
**Library version:** 0.2.0-rc1  
**Host environment:** Linux x86_64, GCC 8.5.0, Clang 21.1.8, CMake 4.4.4

This report lists every feature included in the showcase, its source/API
reference, implementation status, limitations, and whether each C example was
compiled and executed successfully. It does not certify the library beyond the
separately recorded build/test reports.

## Features included

| # | Feature | Source / API | Status | Limitations noted | Example compiled & executed |
|---|---------|--------------|--------|-------------------|-----------------------------|
| 1 | Mathematical expression input | `src/expression/` — `numerix_expression_compile/evaluate/destroy`, callback context/adapter | Verified | Only `x` variable; no user functions/comparisons/modulo; NAN on failure | Yes (`tutorial_integration`) |
| 2 | C callback input | `include/numerix.h` — `numerix_function`, `numerix_root_newton` etc. | Verified | Callback cannot return a status directly; NAN surfaces failures | Yes (`tutorial_callback_root`) |
| 3 | Expression vs callback | `numerix_expression_callback` adapter | Verified | Not every API accepts expressions (linear algebra, ODE, multivariable) | Yes |
| 4 | Piecewise functions | `src/piecewise/` — create/add_expression/evaluate/analyze_boundary/integrate/diff | Verified | Probing cannot prove all discontinuities; confidence levels; gaps rejected | Yes (`numerix_piecewise_demo`) |
| 5 | Algorithm recommendation | `src/intelligence/recommendation.c` — `numerix_recommend_algorithm` | Verified | Not a guarantee of fastest/most accurate | Yes (`numerix_intelligence_demo`) |
| 6 | Automatic selection & execution | `src/intelligence/root_execution.c` — `numerix_root_solve_auto`, `_portfolio` | Verified | Fallback not universal; classified failures only | Yes (`numerix_intelligence_demo`) |
| 7 | Failure classification & fallback | `numerix_root_selection_report`, `numerix_algorithm_attempt` | Verified | Some failures do not trigger fallback | Yes (fallback demo) |
| 8 | Root-solving portfolio | `src/algorithms/root_finding/root_finding.c` — six methods | Verified | Per-method limitations documented | Yes |
| 9 | Multiple-root discovery | `src/root_discovery/root_discovery.c` — `numerix_root_find_all` | Verified | Heuristic; cannot guarantee completeness; multiplicity is an estimate | Yes (`numerix_root_discovery_demo`) |
| 10 | Execution lifecycle & monitoring | `src/execution/managed_execution.c` — state, progress, summary | Verified | Pause/resume at Bisection step boundaries only | Yes (`numerix_managed_execution_demo`) |
| 11 | Step-by-step execution | `numerix_execution_step_callback` — Bisection steps | Verified | Bisection-focused | Yes (`numerix_managed_execution_demo`) |
| 12 | Resource limits & cancellation | `numerix_options`, `numerix_managed_execution_request_cancel` | Verified | Cooperative cancellation; not real-time | Yes (cancellation demo) |
| 13 | Numerical diagnostics | `src/diagnostics/` — `numerix_diagnostics_analyze` | Verified | Only supported metrics set; estimates not proofs | Yes (`numerix_cli --root`) |
| 14 | Forward/backward error & conditioning | `numerix_diagnostics_forward_error_scalar`, `root_condition` | Verified | Condition estimates are estimates | Yes |
| 15 | Sensitivity & uncertainty | `numerix_diagnostics_scalar_sensitivity`, multivariable, propagation | Verified | Independent inputs; no correlations; scalar objective only | Yes |
| 16 | Reliability & edge-case handling | `include/numerix.h` status codes | Verified | Allocation-fault injection not exhaustive | Yes (invalid bracket) |
| 17 | Memory management & ownership | `numerix_*_create/destroy`, `numerix_allocate/deallocate` | Verified | No custom allocator/shared library/stable ABI | Yes |
| 18 | Portability, build & install | `CMakeLists.txt`, `Makefile`, install guide | Verified (Linux GCC/Clang) | Windows/macOS unvalidated | N/A |
| 19 | Testing & verification | CTest 35 tests; sanitizers; static analysis | Verified (2026-11-12 report) | Historical/current counts not combined | N/A |
| 20 | Benchmarking context | Phase 14B report | Partial | Root methods only; six modules pending | N/A |

## C examples compiled and executed

The following examples were written, compiled against `libnumerix.a` with
`gcc -std=c11 -Wall -Wextra -Iinclude`, and executed. Outputs shown on the
website match actual program output:

1. `tutorial_callback_root.c` — Newton on x²−4 → root 2.000000000000, 6 iterations. **PASS**
2. `tutorial_integration.c` — sin(x) integral 0→π ≈ 2.0000000000. **PASS**
3. `tutorial_differentiation.c` — derivative of x² at 2 → 4.00000000. **PASS**
4. `tutorial_linear_algebra.c` — 2×2 Gaussian solve → [1.8, 1.4]. **PASS**
5. `tutorial_ode.c` — RK4 oscillator → final y ≈ [0.999858, -0.016809]. **PASS**

The following existing demo executables were run and their outputs used:

- `numerix_expression_demo` — verified outputs (x²+3x−5, sin+cos, root, integral, derivative). **PASS**
- `numerix_piecewise_demo` — verified outputs (evaluations, boundary, integral, derivative, jump, removable). **PASS**
- `numerix_intelligence_demo` — verified outputs (recommendation, auto solve, portfolio, fallback). **PASS**
- `numerix_managed_execution_demo` — verified outputs (Bisection steps, root, iterations, evaluations). **PASS**
- `numerix_root_discovery_demo` — verified outputs (3 roots, tangential root, piecewise search, cancellation). **PASS**
- `numerix_cli` — verified `--eval`, `--root`, `--roots`, `--piecewise-demo`. **PASS**

## Preservation checks

- `documentation/presentation/` is **unchanged** (not modified or deleted).
- No NUMERIX production source code, public API, tests, or version was modified.
- No benchmark data, CSV, JSON, dashboard, or comparison selector was copied into `project_showcase/`.
- `project_showcase/assets/` contains only `showcase.css` and `showcase.js`.

## Remaining platform notes

- Linux GCC 8.5 and Clang 21.1 (Debug/Release) validated: 35/35 CTest, 2/2 consumers.
- Clang ASan/UBSan: 35/35.
- Windows MinGW/MSVC and macOS: configured in CI, not yet tested (unvalidated).
- Sanitizer/GCC-fanalyzer limitations documented in cross-platform build report.
