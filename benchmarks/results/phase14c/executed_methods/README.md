# NUMERIX — Methods Run and Their Separate Figures

> This directory contains only methods with recorded timing, accuracy, or analytic trajectory-error evidence. It includes historical root data from Phase 14B, clearly labeled, and bounded Phase 14C pilots. It is not a full seven-module benchmark.

**Coverage:** 31 distinct operations/methods, 60 method-library routes, 66 route/scope records, and 29 copied raw/metadata files.

## Run inventory

| Module | Methods with records | Methods with timing | Methods with accuracy/error evidence | Evidence period |
|---|---:|---:|---:|---|
| Root | 9 | 9 | 9 | Historical Phase 14B |
| Linear Algebra | 4 | 4 | 4 | Phase 14C pilot |
| Differentiation | 6 | 6 | 6 | Phase 14C pilot |
| Interpolation | 4 | 4 | 3 | Phase 14C pilot |
| Integration | 3 | 3 | 3 | Phase 14C pilot |
| Optimization | 2 | 0 | 2 | Phase 14C pilot |
| Ode | 3 | 0 | 3 | Phase 14C pilot |

## Methods recorded

### Root

Methods: Automatic method selection; Bisection; Brent; Multiple-root discovery; Newton-Raphson; Regula Falsi; Safeguarded Newton; Secant; Solver portfolio.

- Timing evidence: Automatic method selection; Bisection; Brent; Multiple-root discovery; Newton-Raphson; Regula Falsi; Safeguarded Newton; Secant; Solver portfolio
- Accuracy/error evidence: Automatic method selection; Bisection; Brent; Multiple-root discovery; Newton-Raphson; Regula Falsi; Safeguarded Newton; Secant; Solver portfolio
- Method-level memory: not measured.
- Evidence is historical Phase 14B v3: 100 problems/method and 31 batches/route. GSL has direct routes for six single-root methods and a heuristic scan for multiple-root discovery; auto-selection and portfolio are NUMERIX-only.

### Linear Algebra

Methods: LU factorization; LU solve; Matrix addition; Matrix multiplication.

- Timing evidence: LU factorization; LU solve; Matrix addition; Matrix multiplication
- Accuracy/error evidence: LU factorization; LU solve; Matrix addition; Matrix multiplication
- Method-level memory: not measured.
- Phase 14C pilot: matrix addition, matrix multiplication, LU factorization, and LU solve; one input per method/library route, 31 batches.

### Differentiation

Methods: Backward difference; Central difference; Five-point difference; Forward difference; Richardson extrapolation; Second derivative.

- Timing evidence: Backward difference; Central difference; Five-point difference; Forward difference; Richardson extrapolation; Second derivative
- Accuracy/error evidence: Backward difference; Central difference; Five-point difference; Forward difference; Richardson extrapolation; Second derivative
- Method-level memory: not measured.
- Phase 14C GSL adapter pilot: three problems/method, 31 batches/route. The second-derivative competitor is a finite-difference wrapper, not a native GSL second-derivative API; piecewise differentiation was not run.

### Interpolation

Methods: Lagrange interpolation; Linear interpolation; Newton construction; Newton evaluation.

- Timing evidence: Lagrange interpolation; Linear interpolation; Newton construction; Newton evaluation
- Accuracy/error evidence: Lagrange interpolation; Linear interpolation; Newton evaluation
- Method-level memory: not measured.
- Phase 14C GSL adapter pilot: three problems/method, 31 batches/route. Newton construction has timing records but no numerical-output accuracy result; construction and evaluation are separate routes.

### Integration

Methods: Adaptive Simpson; Composite Simpson 1/3 rule; Composite trapezoidal rule.

- Timing evidence: Adaptive Simpson; Composite Simpson 1/3 rule; Composite trapezoidal rule
- Accuracy/error evidence: Adaptive Simpson; Composite Simpson 1/3 rule; Composite trapezoidal rule
- Method-level memory: not measured.
- Phase 14C GSL adapter pilot: three problems/method, 31 batches/route. Fixed GSL Gauss-Legendre and adaptive QAG comparisons are best available methods with different work policies; piecewise integration was not run.

### Optimization

Methods: Golden-section bounded minimization; Gradient descent.

- Timing evidence: none
- Accuracy/error evidence: Golden-section bounded minimization; Gradient descent
- Method-level memory: not measured.
- Phase 14C correctness-only pilot: one analytic case/method; no timing. NLopt DIRECT-L returned MAXEVAL_REACHED on the golden-section case although the candidate passed the accuracy gate.

### Ode

Methods: Classical RK4 fixed step; Euler fixed step; Heun fixed step.

- Timing evidence: none
- Accuracy/error evidence: Classical RK4 fixed step; Euler fixed step; Heun fixed step
- Method-level memory: not measured.
- Phase 14C correctness-only pilot: Euler, Heun, and RK4 against matching SUNDIALS tableaus on one two-state system and 21 output points. Error was computed, but no pass threshold was set.

## Separate figures

These charts omit methods with no corresponding run evidence. Runtime and accuracy are shown only when measured. Memory figures say not measured.

[Three broad all-module graphs](combined/README.md) combine the executed time routes, accuracy evidence, and per-module memory status.

| Module | Runtime | Memory | Accuracy / error |
|---|---|---|---|
| Root | [PNG](figures/root/root_runtime.png) · [SVG](figures/root/root_runtime.svg) | [PNG](figures/root/root_memory.png) · [SVG](figures/root/root_memory.svg) | [PNG](figures/root/root_accuracy.png) · [SVG](figures/root/root_accuracy.svg) |
| Linear Algebra | [PNG](figures/linear_algebra/linear_algebra_runtime.png) · [SVG](figures/linear_algebra/linear_algebra_runtime.svg) | [PNG](figures/linear_algebra/linear_algebra_memory.png) · [SVG](figures/linear_algebra/linear_algebra_memory.svg) | [PNG](figures/linear_algebra/linear_algebra_accuracy.png) · [SVG](figures/linear_algebra/linear_algebra_accuracy.svg) |
| Differentiation | [PNG](figures/differentiation/differentiation_runtime.png) · [SVG](figures/differentiation/differentiation_runtime.svg) | [PNG](figures/differentiation/differentiation_memory.png) · [SVG](figures/differentiation/differentiation_memory.svg) | [PNG](figures/differentiation/differentiation_accuracy.png) · [SVG](figures/differentiation/differentiation_accuracy.svg) |
| Interpolation | [PNG](figures/interpolation/interpolation_runtime.png) · [SVG](figures/interpolation/interpolation_runtime.svg) | [PNG](figures/interpolation/interpolation_memory.png) · [SVG](figures/interpolation/interpolation_memory.svg) | [PNG](figures/interpolation/interpolation_accuracy.png) · [SVG](figures/interpolation/interpolation_accuracy.svg) |
| Integration | [PNG](figures/integration/integration_runtime.png) · [SVG](figures/integration/integration_runtime.svg) | [PNG](figures/integration/integration_memory.png) · [SVG](figures/integration/integration_memory.svg) | [PNG](figures/integration/integration_accuracy.png) · [SVG](figures/integration/integration_accuracy.svg) |
| Optimization | [PNG](figures/optimization/optimization_runtime.png) · [SVG](figures/optimization/optimization_runtime.svg) | [PNG](figures/optimization/optimization_memory.png) · [SVG](figures/optimization/optimization_memory.svg) | [PNG](figures/optimization/optimization_accuracy.png) · [SVG](figures/optimization/optimization_accuracy.svg) |
| Ode | [PNG](figures/ode/ode_runtime.png) · [SVG](figures/ode/ode_runtime.svg) | [PNG](figures/ode/ode_memory.png) · [SVG](figures/ode/ode_memory.svg) | [PNG](figures/ode/ode_accuracy.png) · [SVG](figures/ode/ode_accuracy.svg) |

## Files

- [`executed_method_routes.csv`](executed_method_routes.csv) — one row per actual method/library/timing-scope record, with sample counts and evidence type.
- [`source_files.csv`](source_files.csv) — copied raw files, original paths, and SHA-256 hashes.
- [`verification.json`](verification.json) — exported method, route, copied-file, and figure counts.
- `data/raw/<module>/<run_id>/` — selected run data only. Root files are historical Phase 14B; other folders are the canonical Phase 14C pilots.

Raw files were copied, not rewritten. Full-study coverage and method-level memory remain incomplete.
