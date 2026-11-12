#!/usr/bin/env python3
"""Create deterministic root edge fixtures and their expected behavior ledger."""
from __future__ import annotations

import csv
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "benchmarks/results/phase14b/manifests/root_edge_cases.csv"
INPUTS = ROOT / "benchmarks/results/phase14b/workloads/root_edge_cases.jsonl"
_ROOT_REFERENCE = object()


def case(name: str, model: int, root: float, lower: float, upper: float,
         initial: float, second: float, *, scale: float=1.0, shape: float=1.0,
         shift: float=1.0, reference: object=_ROOT_REFERENCE, special: str="",
         expected: str, notes: str, atol: float=1e-10, rtol: float=1e-10) -> dict:
    p={"_model":model,"_module":"root","root":root,"scale":scale,
       "shape":shape,"shift":shift,"lower":lower,"upper":upper,
       "initial":initial,"second_initial":second}
    if special=="maxiter":name="MAXITER1-"+name
    if special=="evalcap":name="EVALCAP1-"+name
    if special=="cancel":name="CANCEL-"+name
    return {"problem_id":"P14-ROOT-EDGE-"+name,"family":"edge_case",
      "input_parameters":p,"reference_value":root if reference is _ROOT_REFERENCE else reference,
      "absolute_tolerance":atol,"relative_tolerance":rtol,"special":special,
      "expected_behavior":expected,"notes":notes}


def main() -> int:
    lo=1.0; hi=math.nextafter(math.nextafter(1.0,math.inf),math.inf)
    adjacent=math.nextafter(1.0,math.inf)
    cases=[
      case("lower_endpoint",0,0.0,0.0,1.0,0.3,0.6,expected="Accept exact root at lower bracket endpoint.",notes="Endpoint value is exactly zero."),
      case("upper_endpoint",0,1.0,0.0,1.0,0.3,0.6,expected="Accept exact root at upper bracket endpoint.",notes="Endpoint value is exactly zero."),
      case("no_root_bracket",0,2.0,0.0,1.0,0.3,0.6,expected="Reject bracketed methods; open methods may converge only if starts reach the root.",notes="True root lies outside the supplied interval."),
      case("same_sign_tangency",13,0.5,0.0,1.0,0.25,0.75,expected="Bracketed solvers reject same-sign endpoints; discovery may report a tangential root.",notes="Even-multiplicity root without sign change."),
      case("adjacent_float_midpoint",0,adjacent,lo,hi,1.0,hi,expected="Terminate with a representable endpoint/root or report stagnation without looping.",notes="The interval is only two ULPs wide and midpoint arithmetic can stagnate."),
      case("tiny_bracket",0,1e-150,-1e-150,3e-150,0.0,2e-150,expected="Find the small-magnitude root within scale-appropriate tolerance.",notes="Finite tiny-scale interval with a 1e-160 absolute root tolerance.",atol=1e-160,rtol=1e-8),
      case("huge_bracket",0,1e150,-1e150,2e150,0.0,1.5e150,expected="Converge without overflow in interval handling.",notes="Large finite domain."),
      case("zero_derivative_at_root",13,0.0,-1.0,1.0,0.0,0.5,expected="Newton may accept exact zero residual despite zero derivative; other methods follow their own rules.",notes="Repeated root with derivative zero at the exact initial root."),
      case("zero_derivative_nonroot",1,0.0,-0.5,1.5,0.5,1.25,shift=1.0,expected="Newton must fail safely when derivative is zero away from a root.",notes="The initial point is the quadratic vertex; two roots lie in the interval, endpoints have same sign."),
      case("near_zero_derivative",17,1.0,0.0,2.0,1.1,1.4,scale=1.0,expected="Report convergence and residual accurately for a flat scaled function.",notes="Derivative magnitude is 1e-12."),
      case("nonfinite_domain_callback",6,0.0,-2.0,1.0,-0.5,0.5,shift=1.0,expected="Report callback/domain failure or a documented safe rejection.",notes="Logarithm callback is non-finite for part of the supplied interval."),
      case("infinite_endpoint_callback",11,0.0,-1000.0,1000.0,-1.0,1.0,expected="Handle infinite endpoint values without undefined arithmetic.",notes="Sinh callback overflows at both interval endpoints."),
      case("discontinuous_jump",100,0.0,-1.0,1.0,-0.5,0.5,reference=None,expected="Do not claim a root from a sign change across a jump.",notes="Synthetic callback jumps from -1 to +1 and has no zero."),
      case("iteration_exhaustion",0,0.123456789,-1.0,1.0,-0.8,0.6,special="maxiter",expected="Return the configured iteration-limit status after one iteration if not converged.",notes="Runner enforces a one-iteration cap for this case."),
      case("evaluation_exhaustion",0,0.123456789,-1.0,1.0,-0.8,0.6,special="evalcap",expected="NUMERIX stops when its one-evaluation budget is exhausted; libraries without callback budgets are marked non-equivalent.",notes="The Phase 14B adapter sets NUMERIX maximum_function_evaluations=1."),
      case("discovery_cancellation",0,0.123456789,-1.0,1.0,-0.8,0.6,special="cancel",expected="NUMERIX discovery cancels through its progress callback after eight sampled function evaluations.",notes="Other libraries without a compatible cancellation callback are not directly comparable."),
      case("closely_spaced_roots",1,0.0,-0.1,0.1,-0.05,0.05,shift=1e-6,expected="Discovery should report both roots when resolvable; bracketed methods reject same-sign endpoints.",notes="Two roots are separated by 1e-6."),
      case("identical_secant_starts",0,0.3,-1.0,1.0,0.2,0.2,expected="Secant rejects coincident starts without division by zero.",notes="Only the secant method depends on two distinct starting values."),
      case("reversed_interval",0,0.0,1.0,-1.0,0.0,0.5,expected="Reject invalid lower/upper ordering.",notes="Input bracket is reversed."),
    ]
    INPUTS.parent.mkdir(parents=True,exist_ok=True)
    with INPUTS.open("w",encoding="utf-8") as f:
        for c in cases:
            row={k:v for k,v in c.items() if k not in {"special","expected_behavior","notes"}}
            f.write(json.dumps(row,sort_keys=True,separators=(",",":"))+"\n")
    fields=["problem_id","edge_case","module","model","input_parameters_json","reference_value",
            "expected_behavior","notes","applicability_note","status_interpretation"]
    with MANIFEST.open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=fields,lineterminator="\n");w.writeheader()
        for c in cases:
            model=c["input_parameters"]["_model"]
            app="The discontinuity has no true root; invalid-expression compilation is outside callback root APIs." if model==100 else "See per-method applicability and expected behavior."
            w.writerow({"problem_id":c["problem_id"],"edge_case":c["problem_id"].removeprefix("P14-ROOT-EDGE-"),
              "module":"root","model":model,"input_parameters_json":json.dumps(c["input_parameters"],sort_keys=True),
              "reference_value":"not applicable" if c["reference_value"] is None else c["reference_value"],
              "expected_behavior":c["expected_behavior"],"notes":c["notes"],"applicability_note":app,
              "status_interpretation":"Correct rejection, numerical failure, cancellation, resource exhaustion, unsupported, and inaccurate success must remain distinct."})
        w.writerow({"problem_id":"P14-ROOT-EDGE-invalid_expression","edge_case":"invalid_expression",
          "module":"root","model":"supporting_expression_api","input_parameters_json":"NA",
          "reference_value":"not applicable","expected_behavior":"Expression compilation rejects malformed source with a parse status; callback root methods do not accept expression text directly.",
          "notes":"See expression_invalid_probe.csv; this validates the public expression input boundary separately from callback solver timing.",
          "applicability_note":"Supporting API probe; not a root algorithm timing case.",
          "status_interpretation":"Correct invalid-input rejection."})
    print(f"wrote {len(cases)} edge cases")
    return 0


if __name__=="__main__":raise SystemExit(main())
