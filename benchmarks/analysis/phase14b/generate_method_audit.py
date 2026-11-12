#!/usr/bin/env python3
"""Generate a verified-method inventory and Phase 14-to-14B gap audit."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "benchmarks/analysis/phase14b"
DOC = ROOT / "documentation/development/phase_14b_method_coverage_matrix.md"

# Inventory follows include/numerix.h and the implementation directories.
ROWS = [
("root","Bisection","numerix_root_bisection","src/algorithms/root_finding/root_finding.c","Not measured in Phase 14 main; primary path selected Brent/Newton","GSL bisection","Direct family comparison","Bracket validity, endpoints, stagnation, nonfinite callback, budget limit."),
("root","Regula Falsi","numerix_root_regula_falsi","src/algorithms/root_finding/root_finding.c","Not measured in Phase 14 main","GSL false-position","Direct family comparison","Bracket validity, endpoint root, flat endpoint, iteration exhaustion."),
("root","Secant","numerix_root_secant","src/algorithms/root_finding/root_finding.c","Not measured in Phase 14 main","GSL secant","Direct family comparison","Coincident starts, same-sign starts, zero denominator, poor starts."),
("root","Newton-Raphson","numerix_root_newton","src/algorithms/root_finding/root_finding.c","Measured for selected root families only","GSL Newton","Direct family comparison","Zero/near-zero derivative, poor guess, repeated/tangential root."),
("root","Safeguarded Newton","numerix_root_safeguarded_newton","src/algorithms/root_finding/root_finding.c","Not measured in Phase 14 main","GSL derivative solver; no exact safeguarded-Brent API","Best available only","Invalid/non-sign-changing bracket, zero derivative, endpoint root."),
("root","Brent","numerix_root_brent","src/algorithms/root_finding/root_finding.c","Measured for selected sign-bracket families","GSL Brent","Direct family comparison","Endpoint roots, same-sign endpoints, adjacent floats, midpoint stagnation."),
("root","Automatic method selection","numerix_root_solve_auto","src/intelligence/root_execution.c","Not measured as an independent method","No equivalent automatic recommendation path","NUMERIX-only orchestration study","Insufficient metadata, unsupported representation, fallback behavior."),
("root","Solver portfolio","numerix_root_solve_portfolio","src/intelligence/root_execution.c","Not measured as an independent method","No equivalent portfolio API","NUMERIX-only orchestration study","Attempt exhaustion, disagreement, duplicate work, candidate limits."),
("root","Multiple-root discovery","numerix_root_find_all","src/root_discovery/root_discovery.c","Not measured in Phase 14 main","GSL scan plus local root solvers; no equivalent completeness API","Best available; heuristic completeness only","Tangencies, close roots, oscillation, domain gaps, unresolved regions."),
("linear_algebra","Vector lifecycle","numerix_vector_create / numerix_vector_destroy","src/algorithms/linear_algebra/linear_algebra.c","Not measured separately; solve setup includes allocations","GSL vector allocation/free; BLAS has no lifecycle equivalent","API/allocation comparison only","Sizes 1 and large; invalid size; allocation failure and cleanup."),
("linear_algebra","Vector copy","numerix_vector_copy","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL vector memcpy; CBLAS copy","Direct operation family","Lengths 1 to large; aliasing; invalid/nonfinite data."),
("linear_algebra","Vector addition","numerix_vector_add","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL vector add; CBLAS daxpy wrapper","Direct operation family","Lengths 1 to large; unequal dimensions; overflow."),
("linear_algebra","Vector subtraction","numerix_vector_subtract","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL vector sub; CBLAS daxpy wrapper","Direct operation family","Lengths 1 to large; unequal dimensions; overflow."),
("linear_algebra","Vector scaling","numerix_vector_scale","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL vector scale; CBLAS dscal","Direct operation family","Zero/negative/extreme scalar; overflow and underflow."),
("linear_algebra","Vector dot product","numerix_vector_dot","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL BLAS dot; CBLAS ddot","Direct operation family","Cancellation, mixed scale, unequal lengths, overflow."),
("linear_algebra","Vector 2-norm","numerix_vector_norm2","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL/BLAS nrm2","Direct operation family","Subnormal and near-DBL_MAX components; empty/invalid vector."),
("linear_algebra","Vector infinity norm","numerix_vector_norm_inf","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL vector max-absolute; CBLAS iamax + abs","Direct operation family","All zero, mixed signs, nonfinite input."),
("linear_algebra","Matrix lifecycle","numerix_matrix_create / numerix_matrix_destroy","src/algorithms/linear_algebra/linear_algebra.c","Not measured separately; solve setup includes allocations","GSL matrix allocation/free; LAPACK has no lifecycle equivalent","API/allocation comparison only","2x2 and larger, overflow dimension, allocation failure/cleanup."),
("linear_algebra","Matrix copy","numerix_matrix_copy","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL matrix memcpy","Direct operation family","Rectangular shapes, size scaling, invalid dimensions."),
("linear_algebra","Matrix addition","numerix_matrix_add","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL matrix add; BLAS wrapper","Direct operation family","Rectangular/square shapes, mismatch, overflow."),
("linear_algebra","Matrix subtraction","numerix_matrix_subtract","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL matrix sub; BLAS wrapper","Direct operation family","Rectangular shapes, mismatch, overflow."),
("linear_algebra","Matrix scaling","numerix_matrix_scale","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL matrix scale; BLAS dscal wrapper","Direct operation family","Zero/negative/extreme scalar; overflow."),
("linear_algebra","Matrix multiplication","numerix_matrix_multiply","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL BLAS dgemm; CBLAS dgemm","Direct operation family","Rectangular products, mismatched sizes, cancellation, scaling."),
("linear_algebra","Matrix transpose","numerix_matrix_transpose","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL matrix transpose","Direct operation family","Rectangular dimensions, layout and size scaling."),
("linear_algebra","Matrix-vector multiply","numerix_matrix_vector_multiply","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL BLAS dgemv; CBLAS dgemv","Direct operation family","Rectangular shapes, dimension mismatch, cancellation."),
("linear_algebra","Gaussian elimination solve","numerix_linear_solve_gaussian","src/algorithms/linear_algebra/linear_algebra.c","Measured as dense combined solve","GSL LU solve; LAPACKE dgesv","Best available; pivoting policies differ","Singular/ill-conditioned, scaled rows, overflow, residual."),
("linear_algebra","Gauss-Jordan solve","numerix_linear_solve_gauss_jordan","src/algorithms/linear_algebra/linear_algebra.c","Not measured in Phase 14 main","GSL/LAPACK LU solve","Best available; no direct Gauss-Jordan API","Pivoting, singularity, scaling, residual and forward error."),
("linear_algebra","LU factorization","numerix_lu_decompose","src/algorithms/linear_algebra/linear_algebra.c","Not measured separately","GSL LU; LAPACKE dgetrf","Direct factorization family","Pivot-required matrices, singularity, factor residual, scaling."),
("linear_algebra","LU solve","numerix_lu_solve","src/algorithms/linear_algebra/linear_algebra.c","Not measured separately; main solve refactors each call","GSL LU solve; LAPACKE dgetrs","Direct solve family with precomputed factors","Repeated RHS, factor validity, residual, dimensions."),
("differentiation","Forward difference","numerix_diff_forward","src/algorithms/differentiation/differentiation.c","Not measured in Phase 14 main","GSL gsl_deriv_forward","Best available; stencil and step policy differ","Step sweep, domain boundary, coordinate collapse, cancellation."),
("differentiation","Backward difference","numerix_diff_backward","src/algorithms/differentiation/differentiation.c","Not measured in Phase 14 main","GSL gsl_deriv_backward","Best available; stencil and step policy differ","Step sweep, domain boundary, coordinate collapse, cancellation."),
("differentiation","Central difference","numerix_diff_central","src/algorithms/differentiation/differentiation.c","Measured in Phase 14 main","GSL gsl_deriv_central","Best available; GSL uses adaptive five-point scheme","Step sweep, nonsmoothness, large/small coordinates."),
("differentiation","Five-point difference","numerix_diff_central_five_point","src/algorithms/differentiation/differentiation.c","Not measured in Phase 14 main","GSL gsl_deriv_central","Best available; step adjustment differs","Step sweep, roundoff, nonsmoothness."),
("differentiation","Richardson extrapolation","numerix_diff_richardson","src/algorithms/differentiation/differentiation.c","Not measured in Phase 14 main","GSL derivative helpers; no same public Richardson API verified","Best available only","Step sweep, smoothness, evaluation cost, cancellation."),
("differentiation","Second derivative","numerix_diff_second","src/algorithms/differentiation/differentiation.c","Not measured in Phase 14 main","No direct GSL second-derivative API; constructed finite-difference baseline only","Best available/wrapper only","Step sweep, cancellation, derivative near zero, nonsmooth inputs."),
("differentiation","Piecewise central difference","numerix_piecewise_diff_central","src/piecewise/adapters.c","Not measured in Phase 14 main","No equivalent GSL boundary-aware piecewise API","NUMERIX-specific boundary behavior","At/near joins, stencil crossing, jump and removable boundaries."),
("interpolation","Linear interpolation","numerix_interp_linear","src/algorithms/interpolation/interpolation.c","Not measured in Phase 14 main","GSL gsl_interp_linear","Direct functional comparison","One/two points, duplicate x, endpoints, extrapolation."),
("interpolation","Lagrange interpolation","numerix_interp_lagrange","src/algorithms/interpolation/interpolation.c","Not measured in Phase 14 main","GSL polynomial interpolation","Best available; representation differs","Node clustering, duplicate nodes, extrapolation, scale."),
("interpolation","Newton construction","numerix_interp_newton_create","src/algorithms/interpolation/interpolation.c","Construction included in combined Phase 14 polynomial path","GSL gsl_interp_polynomial initialization","Best available construction phase","Duplicate/near-duplicate nodes, increasing node count, overflow."),
("interpolation","Newton evaluation","numerix_interp_newton_evaluate","src/algorithms/interpolation/interpolation.c","Evaluation measured with construction in Phase 14 path","GSL gsl_interp_eval","Best available evaluation phase","Independent validation grid, extrapolation, high degree, conditioning."),
("integration","Composite trapezoidal rule","numerix_integrate_trapezoid","src/algorithms/integration/integration.c","Not measured in Phase 14 main","No GSL composite trapezoid API; GSL fixed Gauss-Legendre is not equivalent","NUMERIX-only plus best-available quadrature","Panels, reversed/zero interval, narrow peak, discontinuity."),
("integration","Composite Simpson 1/3 rule","numerix_integrate_simpson","src/algorithms/integration/integration.c","Not measured in Phase 14 main","No GSL composite Simpson API; QAG is not equivalent","NUMERIX-only plus best-available quadrature","Odd/even panels, zero/reversed interval, endpoint behavior."),
("integration","Adaptive Simpson","numerix_integrate_adaptive_simpson","src/algorithms/integration/integration.c","Measured in Phase 14 main","GSL gsl_integration_qag","Best available; different algorithms","Tolerance sweep, depth/budget exhaustion, singular endpoints."),
("integration","Piecewise trapezoidal integration","numerix_piecewise_integrate_trapezoid","src/piecewise/adapters.c","Not measured in Phase 14 main","No GSL boundary-aware segmented API equivalent","NUMERIX-specific boundary handling","Gaps, joins, included/excluded bounds, jumps."),
("optimization","Golden-section bounded minimization","numerix_optimize_golden_section","src/algorithms/optimization/optimization.c","Measured in Phase 14 main","GSL gsl_min_fminimizer_brent; NLopt derivative-free methods","Best available; GSL Brent is not pure golden section","Endpoint minima, flat/narrow/multimodal, invalid bounds."),
("optimization","Gradient descent","numerix_optimize_gradient_descent","src/algorithms/optimization/optimization.c","Not measured in Phase 14 main","GSL multimin steepest descent; NLopt gradient methods","Best available; line-search/termination differ","Ill-scaled variables, zero gradient, oversized step, stagnation."),
("ode","Euler fixed step","numerix_ode_euler","src/algorithms/ode/ode.c","Not measured in Phase 14 main","GSL odeiv2 step_euler","Direct family comparison","Zero/backward interval, oversized step, instability, nonfinite stage."),
("ode","Heun fixed step","numerix_ode_heun","src/algorithms/ode/ode.c","Not measured in Phase 14 main","GSL rk2 family; tableau equivalence must be verified","Direct only if tableau verified; otherwise best available","Order checks, step sweep, backward interval, nonfinite stage."),
("ode","Classical RK4 fixed step","numerix_ode_rk4","src/algorithms/ode/ode.c","Measured in Phase 14 main","GSL odeiv2 rk4; SUNDIALS ARKODE Kutta-RK4a","Direct fixed-step comparison","Trajectory-wide error, step sweep, vector dimensions, stability."),
]

FIELDS = ["module", "method", "public_symbol", "implementation_source",
          "phase14_coverage", "competitor_equivalent", "comparison_class",
          "required_edge_cases"]

def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / "method_coverage.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(FIELDS)
        w.writerows(ROWS)
    lines = [
        "# Phase 14B initial public-method coverage audit", "",
        "This inventory follows include/numerix.h, the implementations under src/, the Phase 14 method map/adapters, and existing tests. Lifecycle APIs are included because Phase 14B explicitly requests create/destroy measurements. General expression, diagnostics, execution-management, and recommendation-support APIs are identified as supporting APIs rather than additional core numerical algorithms.", "",
        f"**Identified method/operation rows:** {len(ROWS)}. Phase 14 measured one selected primary path per module; remaining method rows are Phase 14B coverage gaps.", "",
        "The machine-readable matrix is [method_coverage.csv](../../benchmarks/analysis/phase14b/method_coverage.csv). Direct comparison classifications remain provisional until the Phase 14B method runs verify formulas/tableaux and stopping rules. Best-available comparisons must remain separate.", "",
        "| Module | Public method/operation | NUMERIX symbol | Source | Phase 14 coverage | Candidate competitor | Comparison class | Required edge tests |",
        "| --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in ROWS:
        module, method, symbol, source, coverage, competitor, classification, edges = row
        lines.append(f"| {module} | {method} | {symbol} | {source} | {coverage} | {competitor} | {classification} | {edges} |")
    lines += [
        "", "## API boundary notes", "",
        "- Root auto-selection, portfolio execution, and multiple-root discovery are included because they provide public root-solving/discovery behavior. Piecewise differentiation and integration are included because they have distinct boundary-aware numerical semantics.",
        "- General expression compilation/evaluation, diagnostics/sensitivity, managed execution, generic context/counter utilities, and algorithm-recommendation metadata support numerical operations but are not counted as separate core methods. The phase will disclose this boundary explicitly.",
        "- The original Phase 14 700-problem manifest and outputs remain read-only. Phase 14B uses its own manifest, raw-data hierarchy, analysis, figures, and report.",
        "- This initial audit must be revised if source/API inspection during implementation changes a capability or method classification.",
    ]
    DOC.parent.mkdir(parents=True, exist_ok=True)
    DOC.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {len(ROWS)} API method rows")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
