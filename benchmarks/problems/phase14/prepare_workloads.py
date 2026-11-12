#!/usr/bin/env python3
"""Generate the preregistered Phase 14 workload and independent references.

The emitted manifest contains 20 formula/structure families x 5 distinct
parameter instances for each of the seven public NUMERIX modules.  Python is
used only to prepare inputs/references; benchmark adapters consume the saved
input records through their native C APIs.
"""

from __future__ import annotations

import csv
import hashlib
import json
import math
import random
import sys
from pathlib import Path

import mpmath as mp
import numpy as np
from scipy.integrate import solve_ivp


ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent / "workloads.csv"
SEED = 20261112
mp.mp.dps = 70


def compact(value):
    if isinstance(value, np.ndarray):
        return value.tolist()
    if isinstance(value, (np.floating, np.integer)):
        return value.item()
    if isinstance(value, float):
        return value if math.isfinite(value) else str(value)
    if isinstance(value, dict):
        return {str(k): compact(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [compact(v) for v in value]
    return value


def row(module, family_i, variant_i, model, description, params, size,
        reference, reference_method, atol, rtol, difficulty,
        expected="success", eligible="numerix|gsl", modes="best_available"):
    family = FAMILIES[module][family_i]
    full_input = compact(params)
    full_input["_model"] = model
    full_input["_module"] = module
    manifest_input = dict(full_input)
    if module == "linear_algebra":
        for key in ("matrix", "rhs", "x_reference"):
            manifest_input.pop(key, None)
    if module == "interpolation":
        for key in ("nodes", "values", "validation_x", "validation_y"):
            manifest_input.pop(key, None)
    canonical_input = json.dumps(full_input, sort_keys=True, separators=(",", ":"))
    input_hash = hashlib.sha256(canonical_input.encode("utf-8")).hexdigest()
    result = {
        "problem_id": f"P14-{module.upper()}-{family_i + 1:02d}-{variant_i + 1:02d}",
        "module": module,
        "family": family,
        "family_index": family_i + 1,
        "variant_index": variant_i + 1,
        "model": model,
        "mathematical_description": description,
        "input_parameters_json": json.dumps(manifest_input, sort_keys=True, separators=(",", ":")),
        "input_data_file": "problem_inputs.jsonl",
        "input_sha256": input_hash,
        "input_size": size,
        "reference_value": json.dumps(compact(reference), separators=(",", ":")),
        "reference_method": reference_method,
        "absolute_tolerance": f"{atol:.17g}",
        "relative_tolerance": f"{rtol:.17g}",
        "difficulty": difficulty,
        "expected_status": expected,
        "eligible_libraries": eligible,
        "comparison_modes": modes,
        "seed": SEED + family_i * 101 + variant_i,
    }
    result["_input_data"] = {
        "problem_id": result["problem_id"],
        "family": family,
        "input_parameters": full_input,
        "reference_value": compact(reference),
        "absolute_tolerance": atol,
        "relative_tolerance": rtol,
        "input_sha256": input_hash,
    }
    return result


FAMILIES = {
    "root": ["linear", "quadratic_isolated", "cubic_monotone", "quartic_shape", "sine", "exponential", "logarithmic", "rational", "mixed_sine", "tanh_flat", "arctangent", "sinh_scaled", "asymmetric_polynomial", "double_root", "triple_root", "fifth_order_flat", "steep_scaled", "tiny_scaled", "cosine_perturbed", "boundary_root"],
    "linear_algebra": ["diagonal", "strict_diagonal_dominance", "symmetric_positive_definite", "tridiagonal", "toeplitz", "hilbert_conditioning", "vandermonde_conditioning", "row_scaled", "column_scaled", "random_dense", "near_singular", "singular", "permutation", "block_diagonal", "arrowhead", "low_rank_plus_diagonal", "cauchy", "symmetric_indefinite", "pivot_required", "geometric_spectrum"],
    "differentiation": ["cubic_polynomial", "sine", "cosine", "exponential", "logarithm", "square_root", "rational_quadratic", "gaussian", "hyperbolic_tangent", "arctangent", "x_exp_decay", "chirp_sine", "exponential_sine", "log_quadratic", "rational_linear", "quintic_polynomial", "regularized_norm", "hyperbolic_cosine", "inverse_hyperbolic_sine", "exponential_complement"],
    "interpolation": ["low_degree_polynomial", "sine_uniform", "cosine_nonuniform", "exponential", "logarithm", "square_root", "runge_uniform", "runge_chebyshev", "gaussian_clustered", "high_frequency_sine", "cubic_smooth", "rational", "tanh", "quartic", "sparse_nodes", "dense_nodes", "scaled_small", "scaled_large", "endpoint_curvature", "mixed_smooth"],
    "integration": ["quartic_polynomial", "sine", "cosine", "exponential", "log1p", "rational", "narrow_gaussian", "broad_gaussian", "damped_oscillation", "sqrt_endpoint", "inverse_sqrt_endpoint", "log_endpoint", "logistic_step", "rational_near_pole", "absolute_value_kink", "small_scale", "large_scale", "tanh_transition", "high_frequency_sine_squared", "exp_cosine"],
    "optimization": ["quadratic", "quartic", "asymmetric_quartic", "cosine_perturbed_bowl", "multimodal_nonnegative", "flat_quartic", "narrow_bowl", "wide_bowl", "exponential_bowl", "log_cosh_bowl", "rational_bowl", "boundary_lower", "boundary_upper", "scaled_objective_small", "scaled_objective_large", "high_frequency_ripple", "shouldered_bowl", "smooth_absolute_bowl", "steep_bowl", "two_scale_bowl"],
    "ode": ["constant_rhs", "exponential_growth_decay", "affine_linear", "polynomial_forcing", "quadratic_forcing", "harmonic_oscillator", "damped_oscillator", "coupled_diagonal", "logistic", "linear_sinusoid_forcing", "quadratic_state", "gaussian_forcing", "sinusoid_rhs", "cosine_rhs", "exponential_forcing", "lotka_volterra", "van_der_pol", "sir", "stiff_diagonal", "nonlinear_pendulum"],
}


def root_rows():
    rows = []
    for fi, family in enumerate(FAMILIES["root"]):
        for vi in range(5):
            r = -1.75 + 0.73 * vi + 0.17 * fi
            k = 0.6 + 0.31 * (vi + 1)
            scale = (10.0 ** ((vi % 3) - 1)) * (1.0 + 0.15 * fi)
            shift = 1.25 + 0.1 * (vi + fi % 3)
            if fi == 6:
                r = 0.4 + 0.2 * vi
                shift = 2.5
            lo, hi = r - 0.8, r + 0.8
            if fi == 1:
                lo, hi = r - 0.25 * shift, r + 0.25 * shift
            if fi == 19:
                lo, hi = r, r + 0.9
            if fi in (13, 14, 15):
                lo, hi = r - 0.6, r + 0.6
            p = {"root": r, "scale": scale, "shape": k, "shift": shift,
                 "lower": lo, "upper": hi, "initial": r + 0.23,
                 "second_initial": r + 0.51, "derivative_available": True}
            expr = [
                f"{scale:.6g}*(x-{r:.6g})",
                f"{scale:.6g}*(x-{r:.6g})*(x-{r + shift:.6g})",
                f"{scale:.6g}*((x-{r:.6g})+{0.08*k:.6g}*(x-{r:.6g})^3)",
                f"{scale:.6g}*(x-{r:.6g})*(1+{0.03*k:.6g}*(x-{r:.6g})^4)",
                f"{scale:.6g}*sin({k:.6g}*(x-{r:.6g}))",
                f"{scale:.6g}*expm1({k:.6g}*(x-{r:.6g}))",
                f"{scale:.6g}*log((x+{shift:.6g})/({r+shift:.6g}))",
                f"{scale:.6g}*(x-{r:.6g})/(1+{k:.6g}*(x-{r:.6g})^2)",
                f"{scale:.6g}*((x-{r:.6g})+0.1*sin({k:.6g}*(x-{r:.6g})))",
                f"{scale:.6g}*tanh({k:.6g}*(x-{r:.6g}))",
                f"{scale:.6g}*atan({k:.6g}*(x-{r:.6g}))",
                f"{scale:.6g}*sinh({k:.6g}*(x-{r:.6g}))",
                f"{scale:.6g}*(x-{r:.6g})*(1+0.05*(x-{r:.6g})+0.02*(x-{r:.6g})^2)",
                f"{scale:.6g}*(x-{r:.6g})^2",
                f"{scale:.6g}*(x-{r:.6g})^3",
                f"{scale:.6g}*(x-{r:.6g})^5",
                f"{scale:.6g}*((x-{r:.6g})+0.03*(x-{r:.6g})^3)",
                f"{scale*1e-12:.6g}*(x-{r:.6g})",
                f"{scale:.6g}*(x-{r:.6g})*(1+0.2*cos({k:.6g}*(x-{r:.6g})))",
                f"{scale:.6g}*(x-{r:.6g})*(1+0.1*(x-{r:.6g})^2)",
            ][fi]
            modes = "best_available"
            eligible = "numerix|gsl|cminpack"
            rows.append(row("root", fi, vi, fi, expr, p, 1, r,
                            "analytical root", 2e-9, 2e-9,
                            "hard" if fi in (13, 14, 15, 17, 19) else ("moderate" if vi in (2, 3) else "easy"),
                            eligible=eligible, modes=modes))
    return rows


def linear_rows():
    rows = []
    sizes = [8, 16, 32, 64, 128]
    for fi, family in enumerate(FAMILIES["linear_algebra"]):
        for vi in range(5):
            n = sizes[(fi + vi) % len(sizes)]
            rng = np.random.default_rng(SEED + fi * 101 + vi)
            x = rng.uniform(-1.0, 1.0, n)
            kind = fi
            if kind == 0:
                a = np.diag(np.geomspace(0.25, 4.0, n))
            elif kind == 1:
                a = rng.normal(0.0, 0.2 / n, (n, n)); np.fill_diagonal(a, 1.0 + np.sum(np.abs(a), axis=1))
            elif kind == 2:
                q, _ = np.linalg.qr(rng.normal(size=(n, n))); a = q @ np.diag(np.geomspace(1.0, 1e-5, n)) @ q.T
            elif kind == 3:
                a = np.diag(np.full(n, 3.0)) + np.diag(np.full(n-1, -0.7), 1) + np.diag(np.full(n-1, -0.7), -1)
            elif kind == 4:
                a = np.fromfunction(lambda i,j: 1.0/(1.0 + np.abs(i-j)), (n,n), dtype=float) + np.eye(n)
            elif kind == 5:
                nn = min(n, 24); a = np.fromfunction(lambda i,j: 1.0/(i+j+1.0), (nn,nn), dtype=float); n=nn; x=x[:n]
            elif kind == 6:
                n=min(n,16); x=x[:n]; a=np.vander(np.linspace(0.2,1.0,n), N=n, increasing=True)
            elif kind in (7,8):
                a = np.eye(n) * np.geomspace(1.0, 1e-4, n)[:,None]
                if kind == 8: a = np.eye(n) * np.geomspace(1e-4, 1.0, n)[None,:]
            elif kind == 9:
                a = rng.normal(size=(n,n)) / math.sqrt(n) + np.eye(n) * 2.0
            elif kind == 10:
                a = np.diag(np.geomspace(1.0, 1e-10, n)) + np.diag(np.full(n-1, 1e-4), 1)
            elif kind == 11:
                a = np.eye(n); a[-1] = a[0]
            elif kind == 12:
                a = np.eye(n)[rng.permutation(n)]
            elif kind == 13:
                a = np.zeros((n,n)); half=n//2; a[:half,:half]=np.eye(half)*2; a[half:,half:]=np.eye(n-half)*3
            elif kind == 14:
                a = np.eye(n)*2.0; a[0,1:]=0.2; a[1:,0]=0.2
            elif kind == 15:
                v=rng.normal(size=n); a=np.outer(v,v)/n + np.eye(n)*0.5
            elif kind == 16:
                u=np.arange(1,n+1,dtype=float); a=1.0/(u[:,None]+u[None,:]+0.2)
            elif kind == 17:
                q,_=np.linalg.qr(rng.normal(size=(n,n))); a=q @ np.diag(np.linspace(-2.0,2.0,n)) @ q.T
            elif kind == 18:
                a=rng.normal(size=(n,n)); a += np.eye(n)*0.3
                a[-1]=0; a[-1,0]=1.0
            else:
                a=np.diag(np.geomspace(1.0,1e-8,n))
            b=a @ x
            singular = kind == 11
            condition = float(np.linalg.cond(a)) if not singular else math.inf
            params = {"n": int(n), "matrix": a.tolist(), "rhs": b.tolist(),
                      "x_reference": x.tolist(), "condition_number_reference": condition,
                      "structure": family, "operation": "dense_linear_solve"}
            rows.append(row("linear_algebra", fi, vi, fi, family.replace("_", " "), params, n,
                            x.tolist(), "constructed exact solution x; b=A*x in float64; residual independently recomputed",
                            1e-12, 1e-9, "hard" if kind in (5,6,10,11,16,19) else ("moderate" if n>=64 else "easy"),
                            expected="singular_expected" if singular else "success",
                            eligible="numerix|gsl|lapack_reference|lapack_openblas",
                            modes="algorithm_equivalent|best_available"))
    return rows


def diff_eval(model, x, p):
    a=p[0]
    if model==0: return x**3+a*x*x+0.3*x, 3*x*x+2*a*x+0.3
    if model==1: return math.sin(a*x), a*math.cos(a*x)
    if model==2: return math.cos(a*x), -a*math.sin(a*x)
    if model==3: return math.exp(a*x), a*math.exp(a*x)
    if model==4: return math.log1p(a*x), a/(1+a*x)
    if model==5: return math.sqrt(1+a*x), a/(2*math.sqrt(1+a*x))
    if model==6: return 1/(1+a*x*x), -2*a*x/(1+a*x*x)**2
    if model==7:
        u=x-p[1]; y=math.exp(-a*u*u); return y,-2*a*u*y
    if model==8: return math.tanh(a*x), a/(math.cosh(a*x)**2)
    if model==9: return math.atan(a*x), a/(1+a*a*x*x)
    if model==10: return x*math.exp(-a*x), math.exp(-a*x)*(1-a*x)
    if model==11: return math.sin(a*x*x), 2*a*x*math.cos(a*x*x)
    if model==12:
        y=math.exp(a*math.sin(x)); return y,a*math.cos(x)*y
    if model==13: return math.log1p(a*x*x),2*a*x/(1+a*x*x)
    if model==14: return x/(1+a*x*x),(1-a*x*x)/(1+a*x*x)**2
    if model==15: return x**5+a*x*x,5*x**4+2*a*x
    if model==16: return math.sqrt(x*x+a),x/math.sqrt(x*x+a)
    if model==17: return math.cosh(a*x),a*math.sinh(a*x)
    if model==18: return math.asinh(a*x),a/math.sqrt(1+a*a*x*x)
    y=1-math.exp(-a*x); return y,a*math.exp(-a*x)


def diff_function_mp(model, x, a, center):
    if model==0:return x**3+a*x*x+mp.mpf("0.3")*x
    if model==1:return mp.sin(a*x)
    if model==2:return mp.cos(a*x)
    if model==3:return mp.exp(a*x)
    if model==4:return mp.log1p(a*x)
    if model==5:return mp.sqrt(1+a*x)
    if model==6:return 1/(1+a*x*x)
    if model==7:return mp.exp(-a*(x-center)**2)
    if model==8:return mp.tanh(a*x)
    if model==9:return mp.atan(a*x)
    if model==10:return x*mp.exp(-a*x)
    if model==11:return mp.sin(a*x*x)
    if model==12:return mp.exp(a*mp.sin(x))
    if model==13:return mp.log1p(a*x*x)
    if model==14:return x/(1+a*x*x)
    if model==15:return x**5+a*x*x
    if model==16:return mp.sqrt(x*x+a)
    if model==17:return mp.cosh(a*x)
    if model==18:return mp.asinh(a*x)
    return 1-mp.exp(-a*x)


def differentiation_rows():
    rows=[]
    for fi,family in enumerate(FAMILIES["differentiation"]):
        for vi in range(5):
            a=0.3+0.35*(vi+1)+0.015*fi
            x=-0.7+0.31*vi+0.08*fi
            if fi in (4,5,13,16): x=0.2+0.11*vi
            if fi==4: a=0.2+0.1*vi
            if fi==5: a=0.3+0.1*vi
            if fi==16: a=0.01+0.02*vi
            h=[1e-2,1e-3,1e-4,1e-5,1e-6][(fi+vi)%5]
            if vi==4 and fi in (10,11,12,15,17): h=1e-7
            center=0.17+0.03*vi
            xx=mp.mpf(str(x)); aa=mp.mpf(str(a)); cc=mp.mpf(str(center))
            deriv=mp.diff(lambda z: diff_function_mp(fi,z,aa,cc),xx)
            deriv_check=mp.diff(lambda z: diff_function_mp(fi,z,aa,cc),xx,addprec=30)
            if abs(deriv-deriv_check)>mp.mpf("1e-55")*max(1,abs(deriv)):
                raise ArithmeticError(f"high-precision derivative did not stabilize: {family} variant {vi}")
            params={"x":x,"step":h,"a":a,"center":center,"derivative_order":1}
            rows.append(row("differentiation",fi,vi,fi,f"d/dx {family.replace('_',' ')} at x={x:.6g}",params,1,mp.nstr(deriv,60),
                            "mpmath arbitrary-precision differentiation (70 dps), cross-checked at 100 dps",
                            2e-7,2e-6,"hard" if h<=1e-6 else ("moderate" if vi>=3 else "easy"),
                            eligible="numerix|gsl",modes="algorithm_equivalent|best_available"))
    return rows


def interp_eval(model,x,a):
    if model==0:return 1+a*x+0.2*x*x-0.03*x**3
    if model==1:return math.sin(a*x)
    if model==2:return math.cos(a*x)+0.1*x
    if model==3:return math.exp(a*x)
    if model==4:return math.log1p(a*x)
    if model==5:return math.sqrt(1+a*x)
    if model in (6,7):return 1/(1+25*x*x)
    if model==8:return math.exp(-a*(x-0.2)**2)
    if model==9:return math.sin(a*x)
    if model==10:return x**3-0.4*x*x+0.1*x
    if model==11:return 1/(1+a*x*x)
    if model==12:return math.tanh(a*x)
    if model==13:return x**4-0.2*x**2+0.5
    if model==14:return math.sin(a*x)+x*x
    if model==15:return math.cos(a*x)+x**3
    if model==16:return 1e-8*math.sin(a*x)
    if model==17:return 1e8*math.sin(a*x)
    if model==18:return math.sqrt(max(0.0,x+1.01))
    return math.exp(-a*x)*math.cos(a*x)


def interpolation_rows():
    rows=[]; counts=[4,6,8,12,16]
    for fi,family in enumerate(FAMILIES["interpolation"]):
        for vi in range(5):
            n=counts[(fi+vi)%5]; a=0.8+0.25*vi+0.03*fi
            lo,hi=-1.0,1.0
            distribution=(fi//4)%5
            if fi in (4,5,18): lo,hi=0.0,1.0
            if distribution==0: nodes=np.linspace(lo,hi,n)
            elif distribution==1:
                nodes=np.sort((lo+hi)/2+(hi-lo)/2*np.cos(np.linspace(math.pi,0,n)))
            elif distribution==2:
                u=np.linspace(-1,1,n); nodes=np.sort((lo+hi)/2+(hi-lo)/2*np.sign(u)*np.abs(u)**1.8)
            elif distribution==3:
                u=np.linspace(0,1,n); nodes=lo+(hi-lo)*u**1.6
            else:
                u=np.linspace(0,1,n); nodes=lo+(hi-lo)*(0.5+0.5*np.tanh(2.5*(2*u-1))/math.tanh(2.5))
            ys=np.array([interp_eval(fi,float(x),a) for x in nodes])
            query=np.linspace(lo+(hi-lo)*0.01,hi-(hi-lo)*0.01,101).tolist()
            truth=[interp_eval(fi,x,a) for x in query]
            params={"n":n,"nodes":nodes.tolist(),"values":ys.tolist(),"validation_x":query,
                    "validation_y":truth,"a":a,"lower":lo,"upper":hi,"distribution":distribution,
                    "interpolation_modes":["piecewise_linear","global_polynomial"]}
            rows.append(row("interpolation",fi,vi,fi,f"{family.replace('_',' ')} on {distribution} node distribution",
                            params,n,truth[50],"direct function evaluation on an independent 101-point validation grid",
                            1e-8,1e-7,"hard" if distribution in (2,4) or n>=12 else ("moderate" if n>=8 else "easy"),
                            eligible="numerix|gsl",modes="algorithm_equivalent|best_available"))
    return rows


def integ_mp(model,x,a,b,c):
    if model==0:return x**4+a*x*x+b*x
    if model==1:return mp.sin(a*x)
    if model==2:return mp.cos(a*x)
    if model==3:return mp.exp(a*x)
    if model==4:return mp.log(1+a*x)
    if model==5:return 1/(1+(a*x)**2)
    if model==6:return mp.exp(-((x-c)/a)**2)
    if model==7:return mp.exp(-((x-c)/a)**2)
    if model==8:return mp.sin(a*x)/(1+x*x)
    if model==9:return mp.sqrt(x)
    if model==10:return 1/mp.sqrt(x)
    if model==11:return mp.log(x)
    if model==12:return 1/(1+mp.exp(-a*(x-c)))
    if model==13:return 1/(1+a*x)
    if model==14:return abs(x-c)
    if model==15:return 1e-8*mp.sin(a*x)
    if model==16:return 1e8*(x*x+1)
    if model==17:return mp.tanh(a*(x-c))
    if model==18:return mp.sin(a*x)**2
    return mp.exp(-a*x)*mp.cos(b*x)


def integration_rows():
    rows=[]
    for fi,family in enumerate(FAMILIES["integration"]):
        for vi in range(5):
            a=0.5+0.7*(vi+1)+0.03*fi; b=0.1+0.07*vi; c=0.35+0.05*vi
            lo,hi=0.0,1.0
            if fi==11: lo=1e-8
            if fi==13: a=0.4+0.1*vi
            if fi in (6,7): a=0.06+0.03*vi if fi==6 else 0.8+0.4*vi
            if fi==10: lo=0.0
            reference=float(mp.quad(lambda z: integ_mp(fi,z,a,b,c), [lo,c,hi] if lo<c<hi else [lo,hi]))
            params={"a":a,"b":b,"center":c,"lower":lo,"upper":hi,
                    "requested_abs_tolerance":1e-8,"requested_rel_tolerance":1e-8,
                    "fixed_panels":256,"max_depth":24}
            expected="success" if fi!=10 else "endpoint_singularity_stress"
            rows.append(row("integration",fi,vi,fi,f"integral of {family.replace('_',' ')} on [{lo:g},{hi:g}]",
                            params,256,reference,"70-decimal mpmath quadrature, split at known kink/peak where applicable",
                            2e-7,2e-7,"hard" if fi in (6,10,11,13,18) else ("moderate" if fi in (8,12,14) else "easy"),expected=expected,
                            eligible="numerix|gsl",modes="best_available"))
    return rows


def opt_eval(model,x,p):
    m,s,r,k,lo,hi=p
    u=x-m
    if model==0:return s*u*u
    if model==1:return s*u**4
    if model==2:return s*(u*u+0.3*u**3+0.2*u**4)
    if model==3:return s*(u*u+r*(1-math.cos(k*u)))
    if model==4:return s*(u*u+r*(1-math.cos(k*u)))
    if model==5:return s*u**4+0.01*u*u
    if model==6:return s*u*u+0.01*u**4
    if model==7:return s*u*u
    if model==8:return s*(math.exp(0.5*u)+math.exp(-0.5*u)-2)
    if model==9:return s*(math.log(math.cosh(u))+0.5*u*u)
    if model==10:return s*u*u/(1+0.1*u*u)
    if model in (11,12):return s*u*u
    if model==13:return 1e-8*u*u
    if model==14:return 1e8*u*u
    if model==15:return s*(u*u+r*(1-math.cos(k*u)))
    if model==16:return s*(u*u+0.2*(1-math.cos(2*u)))
    if model==17:return s*(math.sqrt(u*u+1e-4)-0.01+u*u)
    if model==18:return s*math.expm1(abs(u))**2
    return s*(u*u+0.01*u**4+0.05*(1-math.cos(10*u)))


def optimization_rows():
    rows=[]
    for fi,family in enumerate(FAMILIES["optimization"]):
        for vi in range(5):
            m=-0.7+0.28*vi+0.035*fi; s=0.3+0.4*vi+0.05*fi
            ripple=0.02+0.01*vi; k=2.0+vi+fi%4
            lo,hi=m-1.1-0.1*vi,m+1.2+0.05*fi
            if fi==11:
                lo=-1.0-0.1*vi; hi=1.5; m=lo
            if fi==12:
                lo=-1.5; hi=1.0+0.1*vi; m=hi
            p=[m,s,ripple,k,lo,hi]
            initial=lo+0.27*(hi-lo)
            params={"model":fi,"parameters":p,"lower":lo,"upper":hi,"initial":initial,
                    "reference_minimizer":m,"gradient_available":True}
            val=opt_eval(fi,m,p)
            rows.append(row("optimization",fi,vi,fi,f"bounded one-dimensional {family.replace('_',' ')}",
                            params,1,val,"analytical minimizer; independently checked derivative and interval endpoints",
                            2e-7,2e-7,"hard" if fi in (4,5,11,12,15,17) else ("moderate" if vi>=3 else "easy"),
                            eligible="numerix|gsl|nlopt",modes="algorithm_equivalent|best_available"))
    return rows


def ode_rhs(model,t,y,p):
    a,b,c=p
    if model==0:return np.array([a])
    if model==1:return np.array([a*y[0]])
    if model==2:return np.array([a*y[0]+b])
    if model==3:return np.array([a*t])
    if model==4:return np.array([a*t*t+b])
    if model in (5,6):return np.array([y[1],-a*a*y[0]-(2*b*a*y[1] if model==6 else 0.0)])
    if model==7:return np.array([a*y[0],-b*y[1]])
    if model==8:return np.array([a*y[0]*(1-y[0]/b)])
    if model==9:return np.array([a*y[0]+b*math.cos(c*t)])
    if model==10:return np.array([a*y[0]*y[0]])
    if model==11:return np.array([a*math.exp(-b*t*t)])
    if model==12:return np.array([a*math.sin(b*t)])
    if model==13:return np.array([a*math.cos(b*t)])
    if model==14:return np.array([a*math.exp(b*t)])
    if model==15:return np.array([a*y[0]-b*y[0]*y[1],-c*y[1]+a*y[0]*y[1]])
    if model==16:return np.array([y[1],a*(1-y[0]*y[0])*y[1]-y[0]])
    if model==17:
        beta=a; return np.array([-b*y[0]*y[1],b*y[0]*y[1]-c*y[1],c*y[1]])
    if model==18:return np.array([-a*y[0],-b*y[1]])
    return np.array([y[1],-a*math.sin(y[0])-b*y[1]])


def ode_exact(model,t,y0,p):
    a,b,c=p
    if model==0:return np.array([y0[0]+a*t])
    if model==1:return np.array([y0[0]*math.exp(a*t)])
    if model==2:return np.array([(y0[0]+b/a)*math.exp(a*t)-b/a])
    if model==3:return np.array([y0[0]+a*t*t/2])
    if model==4:return np.array([y0[0]+a*t**3/3+b*t])
    if model==5:
        # Harmonic oscillator y'' = -a^2 y with state [y, y'].
        return np.array([
            y0[0]*math.cos(a*t)+(y0[1]/a)*math.sin(a*t),
            -a*y0[0]*math.sin(a*t)+y0[1]*math.cos(a*t),
        ])
    if model==6:
        w=a; z=b
        d=math.sqrt(max(1e-12,1-z*z)); wd=w*d; e=math.exp(-z*w*t)
        q=(y0[1]+z*w*y0[0])/wd
        yy=e*(y0[0]*math.cos(wd*t)+q*math.sin(wd*t))
        vv=e*(-z*w*(y0[0]*math.cos(wd*t)+q*math.sin(wd*t))-y0[0]*wd*math.sin(wd*t)+q*wd*math.cos(wd*t))
        return np.array([yy,vv])
    if model==7:return np.array([y0[0]*math.exp(a*t),y0[1]*math.exp(-b*t)])
    if model==8:
        k=b; return np.array([k/(1+(k/y0[0]-1)*math.exp(-a*t))])
    if model==9:
        den=a*a+c*c
        integ=(a*math.exp(a*t)-a*math.cos(c*t)+c*math.sin(c*t))/den
        return np.array([math.exp(a*t)*y0[0]+b*integ])
    if model==10:return np.array([y0[0]/(1-a*y0[0]*t)])
    if model==11:
        return np.array([y0[0]+a*math.sqrt(math.pi)/(2*math.sqrt(b))*math.erf(math.sqrt(b)*t)])
    if model==12:return np.array([y0[0]+a*(1-math.cos(b*t))/b])
    if model==13:return np.array([y0[0]+a*math.sin(b*t)/b])
    if model==14:return np.array([y0[0]+a*(math.exp(b*t)-1)/b])
    return None


def ode_rows():
    rows=[]
    for fi,family in enumerate(FAMILIES["ode"]):
        for vi in range(5):
            a=0.2+0.13*(vi+1)+0.01*(fi%5); b=0.15+0.04*vi; c=0.8+0.2*vi
            t0=0.0; tf=0.4+0.15*vi; h=tf/(40+8*vi)
            if fi in (15,16,17,19): tf=0.3+0.08*vi; h=tf/(120+20*vi)
            if fi==18: a=2.0+vi*0.5; b=15.0+vi*5.0; tf=0.4; h=tf/(80+20*vi)
            dim=2 if fi in (5,6,7,15,16,18,19) else (3 if fi==17 else 1)
            y0=np.array([0.7+0.03*vi,0.2-0.02*vi,0.1+0.01*vi][:dim])
            if fi==8: y0[0]=0.25+0.03*vi; b=1.0+0.1*vi
            p=[a,b,c]
            ref=ode_exact(fi,tf,y0,p)
            ref_method="closed-form analytical solution"
            if ref is None:
                rhs=lambda t,y: ode_rhs(fi,t,y,p)
                coarse=solve_ivp(rhs,(t0,tf),y0,method="DOP853",rtol=2e-13,atol=2e-15,dense_output=False,max_step=h/6)
                fine=solve_ivp(rhs,(t0,tf),y0,method="DOP853",rtol=2e-13,atol=2e-15,dense_output=False,max_step=h/12)
                if not coarse.success or not fine.success:
                    raise ArithmeticError(f"DOP853 reference integration failed: {family} variant {vi}")
                delta=float(np.max(np.abs(coarse.y[:,-1]-fine.y[:,-1])))
                if delta>2e-10*max(1.0,float(np.max(np.abs(fine.y[:,-1])))):
                    raise ArithmeticError(f"DOP853 reference did not converge: {family} variant {vi}: {delta}")
                ref=fine.y[:,-1]
                ref_method=f"independent SciPy DOP853, rtol=2e-13, atol=2e-15; max_step halved from {h/6:.3g} to {h/12:.3g}, endpoint delta={delta:.3g}"
            params={"model":fi,"parameters":p,"initial_state":y0.tolist(),"t0":t0,"tf":tf,
                    "fixed_step":h,"fixed_steps":int(round((tf-t0)/h)),"dimension":dim,
                    "adaptive_rtol":1e-8,"adaptive_atol":1e-10}
            rows.append(row("ode",fi,vi,fi,f"{family.replace('_',' ')} over [{t0:g},{tf:g}]",
                            params,int(dim),np.asarray(ref).tolist(),ref_method,2e-7,2e-6,
                            "hard" if fi in (16,17,18,19) else ("moderate" if fi in (5,6,7,15) else "easy"),
                            eligible="numerix|gsl|sundials",modes="algorithm_equivalent|best_available"))
    return rows


def build():
    modules=[root_rows(),linear_rows(),differentiation_rows(),interpolation_rows(),integration_rows(),optimization_rows(),ode_rows()]
    all_rows=[r for group in modules for r in group]
    columns=[key for key in all_rows[0] if not key.startswith("_")]
    OUT.parent.mkdir(parents=True,exist_ok=True)
    with OUT.open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=columns,lineterminator="\n")
        w.writeheader(); w.writerows({k:r[k] for k in columns} for r in all_rows)
    input_path=OUT.parent / "problem_inputs.jsonl"
    with input_path.open("w",encoding="utf-8",newline="\n") as f:
        for r in all_rows:
            f.write(json.dumps(r["_input_data"],sort_keys=True,separators=(",", ":"))+"\n")
    counts={m:sum(r["module"]==m for r in all_rows) for m in FAMILIES}
    family_counts={m:len({r["family"] for r in all_rows if r["module"]==m}) for m in FAMILIES}
    if any(v<100 for v in counts.values()) or any(v<20 for v in family_counts.values()):
        raise SystemExit(f"coverage failure: {counts}; families={family_counts}")
    print(json.dumps({"manifest":str(OUT),"inputs":str(input_path),"problems":len(all_rows),"counts":counts,"families":family_counts},indent=2))


if __name__=="__main__":
    build()
