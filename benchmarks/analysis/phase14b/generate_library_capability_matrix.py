#!/usr/bin/env python3
"""Expand the API audit into a per-library method capability/measurement map."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
METHODS=ROOT/"benchmarks/analysis/phase14b/method_coverage.csv"
OUT=ROOT/"benchmarks/results/phase14b/manifests/library_method_capabilities.csv"
DOC=ROOT/"documentation/development/phase_14b_library_capability_matrix.md"
LIBS=["NUMERIX","GSL","CMINPACK","LAPACK_REFERENCE","LAPACK_OPENBLAS","NLOPT","SUNDIALS_ARKODE","SUNDIALS_CVODE"]

# Module-scoped methods/API routes taken from the Phase 14 native adapters and
# pinned dependency capability matrix. A best-available route is not treated as
# the same algorithm.
GSL={
"root":{"Bisection":("direct_equivalent","gsl_root_fsolver_bisection"),
 "Regula Falsi":("direct_equivalent","gsl_root_fsolver_falsepos"),
 "Secant":("direct_equivalent","gsl_root_fdfsolver_secant"),
 "Newton-Raphson":("direct_equivalent","gsl_root_fdfsolver_newton"),
 "Safeguarded Newton":("best_available","GSL Brent solver"),
 "Brent":("direct_equivalent","gsl_root_fsolver_brent"),
 "Multiple-root discovery":("best_available","fixed-resolution sign scan plus Brent refinement; heuristic")},
"linear_algebra":{"Vector lifecycle":("available_supporting_api","gsl_vector_alloc/free"),
 "Vector copy":("direct_equivalent","gsl_vector_memcpy"),"Vector addition":("direct_equivalent","gsl_vector_add"),
 "Vector subtraction":("direct_equivalent","gsl_vector_sub"),"Vector scaling":("direct_equivalent","gsl_vector_scale"),
 "Vector dot product":("direct_equivalent","gsl_blas_ddot"),"Vector 2-norm":("direct_equivalent","gsl_blas_dnrm2"),
 "Vector infinity norm":("approximate_wrapper","GSL vector max-absolute operation"),
 "Matrix lifecycle":("available_supporting_api","gsl_matrix_alloc/free"),"Matrix copy":("direct_equivalent","gsl_matrix_memcpy"),
 "Matrix addition":("direct_equivalent","gsl_matrix_add"),"Matrix subtraction":("direct_equivalent","gsl_matrix_sub"),
 "Matrix scaling":("direct_equivalent","gsl_matrix_scale"),"Matrix multiplication":("direct_equivalent","gsl_blas_dgemm"),
 "Matrix transpose":("direct_equivalent","gsl_matrix_transpose_memcpy"),"Matrix-vector multiply":("direct_equivalent","gsl_blas_dgemv"),
 "Gaussian elimination solve":("best_available","GSL pivoted LU decomposition and solve"),
 "Gauss-Jordan solve":("not_supported","no selected Gauss-Jordan API"),"LU factorization":("direct_equivalent","gsl_linalg_LU_decomp"),
 "LU solve":("direct_equivalent","gsl_linalg_LU_solve")},
"differentiation":{"Forward difference":("best_available","gsl_deriv_forward"),"Backward difference":("best_available","gsl_deriv_backward"),
 "Central difference":("best_available","gsl_deriv_central"),"Five-point difference":("best_available","gsl_deriv_central adaptive stencil"),
 "Richardson extrapolation":("approximate_wrapper","no same public GSL Richardson method selected"),
 "Second derivative":("approximate_wrapper","constructed finite-difference stencil; no direct GSL second derivative API"),
 "Piecewise central difference":("not_supported","no boundary-aware GSL piecewise API")},
"interpolation":{"Linear interpolation":("direct_equivalent","gsl_interp_linear"),
 "Lagrange interpolation":("best_available","GSL polynomial interpolation"),
 "Newton construction":("best_available","gsl_interp_polynomial initialization"),
 "Newton evaluation":("best_available","gsl_interp_eval on polynomial representation")},
"integration":{"Composite trapezoidal rule":("not_supported","no selected GSL composite trapezoid routine"),
 "Composite Simpson 1/3 rule":("not_supported","no selected GSL composite Simpson routine"),
 "Adaptive Simpson":("best_available","gsl_integration_qag (Gauss-Kronrod 61-point rule)"),
 "Piecewise trapezoidal integration":("not_supported","no boundary-aware GSL piecewise adapter")},
"optimization":{"Golden-section bounded minimization":("best_available","GSL Brent minimizer"),
 "Gradient descent":("best_available","GSL multimin steepest-descent family")},
"ode":{"Euler fixed step":("direct_equivalent","gsl_odeiv2_step_euler"),
 "Heun fixed step":("approximate_equivalent","GSL rk2 family; Heun tableau equivalence not verified"),
 "Classical RK4 fixed step":("direct_equivalent","gsl_odeiv2_step_rk4")}}

CMINPACK={("root","Newton-Raphson"):("best_available","cminpack hybrd1 one-variable nonlinear-system solve")}
LAPACK={
"Vector scaling":("direct_equivalent","CBLAS dscal"),"Vector dot product":("direct_equivalent","CBLAS ddot"),"Vector 2-norm":("direct_equivalent","CBLAS dnrm2"),
"Matrix multiplication":("direct_equivalent","CBLAS dgemm"),"Matrix-vector multiply":("direct_equivalent","CBLAS dgemv"),
"Gaussian elimination solve":("best_available","LAPACKE dgesv"),"LU factorization":("direct_equivalent","LAPACKE dgetrf"),
"LU solve":("direct_equivalent","LAPACKE dgetrs")}
NLOPT={"Golden-section bounded minimization":("best_available","bounded derivative-free NLopt method (DIRECT_L)"),
       "Gradient descent":("best_available","gradient-based NLopt optimizer family; stopping and line search differ")}
ARKODE={"Classical RK4 fixed step":("direct_equivalent","ARKODE Kutta-RK4a verified tableau")}
CVODE={"Classical RK4 fixed step":("best_available","CVODE adaptive Adams/BDF is not fixed-step RK4"),
       "Euler fixed step":("best_available","CVODE adaptive integration; not a fixed-step Euler call"),
       "Heun fixed step":("best_available","CVODE adaptive integration; not a fixed-step Heun call")}


def main()->int:
    with METHODS.open(newline="",encoding="utf-8") as f:methods=list(csv.DictReader(f))
    rows=[]
    for m in methods:
        module=m["module"];name=m["method"]
        for lib in LIBS:
            status="not_applicable";api="NA";note="Library does not target this NUMERIX module."
            if lib=="NUMERIX":
                status="direct_equivalent";api=m["public_symbol"];note="Implemented in NUMERIX public API."
                measured="method-measured" if module=="root" else "not-yet-measured"
            elif lib=="GSL" and name in GSL.get(module,{}):
                status,api=GSL[module][name];note="Capability from pinned GSL API/source audit; Phase 14B execution status is recorded separately."
                measured="method-measured" if module=="root" else "not-yet-measured"
            elif lib=="CMINPACK" and (module,name) in CMINPACK:
                status,api=CMINPACK[(module,name)];note="One-variable MINPACK hybrid solve; not scalar Newton algorithm equivalence."
                measured="method-measured" if module=="root" else "not-yet-measured"
            elif lib in {"LAPACK_REFERENCE","LAPACK_OPENBLAS"} and module=="linear_algebra" and name in LAPACK:
                status,api=LAPACK[name];note="Capability is available through the Phase 14 LAPACKE/CBLAS build; method-level Phase 14B measurements are pending."
                measured="not-yet-measured"
            elif lib=="NLOPT" and module=="optimization" and name in NLOPT:
                status,api=NLOPT[name];note="Best-available optimizer route; not algorithm-identical."
                measured="not-yet-measured"
            elif lib=="SUNDIALS_ARKODE" and module=="ode" and name in ARKODE:
                status,api=ARKODE[name];note="Phase 14 pinned adapter supports fixed Kutta-RK4a."
                measured="not-yet-measured"
            elif lib=="SUNDIALS_CVODE" and module=="ode" and name in CVODE:
                status,api=CVODE[name];note="Adaptive ODE method; compare only as best-available, not fixed-step algorithm identity."
                measured="not-yet-measured"
            elif lib=="GSL" and module in GSL:
                status="not_supported";note="No direct API or selected best-available adapter route is recorded for this method."
                measured="not-measured"
            elif lib=="CMINPACK" and module!="root":
                status="not_applicable";note="CMINPACK is a nonlinear-system solver, not a method for this module."
                measured="not-applicable"
            elif lib=="CMINPACK" and module=="root":
                status="not_supported";note="No corresponding CMINPACK method route is selected for this root method."
                measured="not-measured"
            elif lib.startswith("LAPACK") and module=="linear_algebra":
                status="not_supported";note="No direct LAPACKE/CBLAS operation route is selected for this method."
                measured="not-measured"
            elif lib.startswith("LAPACK") and module!="linear_algebra":
                status="not_applicable";note="LAPACK/BLAS is a linear algebra library."
                measured="not-applicable"
            elif lib=="NLOPT" and module!="optimization":
                status="not_applicable";note="NLopt provides optimization routines only."
                measured="not-applicable"
            elif lib.startswith("SUNDIALS") and module!="ode":
                status="not_applicable";note="SUNDIALS is used here for ODE integration only."
                measured="not-applicable"
            elif lib.startswith("SUNDIALS") and module=="ode":
                status="not_supported";note="No exact or selected best-available route is recorded for this method."
                measured="not-measured"
            else:
                measured="not-measured"
            rows.append({"module":module,"method":name,"public_symbol":m["public_symbol"],
              "library":lib,"capability_status":status,"library_api_or_route":api,
              "phase14b_measurement_status":measured,"note":note})
    OUT.parent.mkdir(parents=True,exist_ok=True)
    with OUT.open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]),lineterminator="\n");w.writeheader();w.writerows(rows)
    lines=["# Phase 14B per-library method capability and measurement matrix","",
      "The machine-readable matrix has one row per verified NUMERIX public method and candidate library. Capability and measurement status are separate: API availability does not mean a Phase 14B measurement exists. Direct algorithm comparisons and best-available routes remain distinct. The Phase 14B root runner measured NUMERIX, GSL, and CMINPACK where applicable; the other six modules have not yet been benchmarked in this execution.","",
      "The mapping follows the pinned Phase 14 capability audit in [phase_14_library_capability_matrix.md](phase_14_library_capability_matrix.md), native adapter APIs, and the NUMERIX public-method inventory. GSL Heun/RK2 equivalence is deliberately left approximate because the tableau equivalence has not been verified.","",
      "| Module | Method | NUMERIX | GSL | CMINPACK | LAPACK reference | LAPACK + OpenBLAS | NLopt | ARKODE | CVODE |","|---|---|---|---|---|---|---|---|---|---|"]
    for m in methods:
        cells=[]
        for lib in LIBS:
            r=next(x for x in rows if x["module"]==m["module"] and x["method"]==m["method"] and x["library"]==lib)
            cells.append(r["capability_status"].replace("_"," "))
        lines.append("| "+" | ".join([m["module"],m["method"]]+cells)+" |")
    DOC.write_text("\n".join(lines)+"\n",encoding="utf-8")
    print(f"wrote {len(rows)} per-library capability records")
    return 0


if __name__=="__main__":raise SystemExit(main())
