#!/usr/bin/env python3
"""Run the root method adapter on the dedicated edge-case suite."""
from __future__ import annotations

import csv
import json
import os
import random
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
RUNNER=ROOT/"build/phase14b_root_runner"
INPUTS=ROOT/"benchmarks/results/phase14b/workloads/root_edge_cases.jsonl"
MANIFEST=ROOT/"benchmarks/results/phase14b/manifests/root_edge_cases.csv"
OUT=ROOT/"benchmarks/results/phase14b/edge_cases/root"
RUN_ID="phase14b-root-edges-20261112-v1"
NUMERIX=["bisection","regula_falsi","secant","newton","safeguarded_newton","brent","auto_selection","portfolio","multiple_root_discovery"]
GSL=["bisection","regula_falsi","secant","newton","safeguarded_newton","brent","multiple_root_discovery"]


def f(p:dict,x:float)->float:
    m=p["_model"];c=p["root"];u=x-c;a=p["scale"];k=p["shape"];d=p["shift"]
    if m==100:return -1.0 if x<c else 1.0
    if m==101:return float("nan")
    if m==0:return a*u
    if m==1:return a*u*(u-d)
    if m==6:
        try:return a*__import__("math").log((x+d)/(c+d))
        except (ValueError,ZeroDivisionError):return float("nan")
    if m==11:
        try:return a*__import__("math").sinh(k*u)
        except OverflowError:return float("inf") if u>0 else float("-inf")
    if m==13:return a*u*u
    if m==17:return a*1e-12*u
    return float("nan")


def main()->int:
    OUT.mkdir(parents=True,exist_ok=True)
    unexpected=[p.name for p in OUT.iterdir() if p.name!="expression_invalid_probe.csv"]
    if unexpected:raise RuntimeError(f"refusing to mix edge measurements into {OUT}: {unexpected}")
    cases=[json.loads(s) for s in INPUTS.read_text(encoding="utf-8").splitlines() if s.strip()]
    source=INPUTS.parent/"root_edge_cases_cminpack_eligible.jsonl"
    with source.open("w",encoding="utf-8") as stream:
        for c in cases:
            if c["input_parameters"]["_model"] not in (100,101):stream.write(json.dumps(c,separators=(",",":"))+"\n")
    methods=[("NUMERIX",m) for m in NUMERIX]+[("GSL",m) for m in GSL]+[("CMINPACK","newton")]
    mapping=[]
    with MANIFEST.open(newline="",encoding="utf-8") as stream:edge_rows=list(csv.DictReader(stream))
    by_id={r["problem_id"]:r for r in edge_rows}
    for c in cases:
        p=c["input_parameters"];pid=c["problem_id"];fl=f(p,p["lower"]);fu=f(p,p["upper"])
        sign_bracket=(fl==0 or fu==0 or (fl<0<fu) or (fu<0<fl))
        for lib,method in methods:
            applicable=True;reason="method accepts this callback/input category"
            if method in {"bisection","regula_falsi","brent","safeguarded_newton"}:
                applicable=sign_bracket and p["lower"]<p["upper"] and p["_model"] not in (100,101)
                reason="continuous finite sign-changing bracket" if applicable else "outside continuous sign-bracket assumptions"
            elif method=="secant":
                applicable=p["initial"]!=p["second_initial"] and p["_model"]!=101
                reason="two distinct starts" if applicable else "coincident starts or non-finite callback"
            elif method=="newton":
                applicable=p["_model"]!=101
                reason="finite callback and derivative expected" if applicable else "callback is non-finite throughout"
            if lib=="CMINPACK" and p["_model"] in (100,101):
                applicable=False;reason="CMINPACK callback path in this adapter does not support synthetic discontinuity/non-finite fixture"
            if lib=="CMINPACK":comp="best_available: MINPACK hybrid solver, not scalar Newton"
            elif method in {"auto_selection","portfolio"}:comp="NUMERIX-only orchestration API"
            elif method=="multiple_root_discovery":comp="best_available root-set scan; heuristic"
            elif lib=="GSL" and method=="safeguarded_newton":comp="GSL Brent; best available, not same algorithm"
            else:comp="direct algorithm-family comparison"
            mapping.append({"run_id":RUN_ID,"problem_id":pid,"edge_case":by_id[pid]["edge_case"],
              "library":lib,"method":method,"applicable":int(applicable),"applicability_reason":reason,
              "comparison_class":"direct" if comp.startswith("direct") else "best_available",
              "comparison_detail":comp,"expected_behavior":by_id[pid]["expected_behavior"],
              "measurement_status":"planned"})
    with (OUT/"method_workloads.csv").open("w",newline="",encoding="utf-8") as stream:
        w=csv.DictWriter(stream,fieldnames=list(mapping[0]),lineterminator="\n");w.writeheader();w.writerows(mapping)
    order=list(methods);random.Random(14112026).shuffle(order);failures=[]
    env=os.environ.copy();env.update({"OMP_NUM_THREADS":"1","OPENBLAS_NUM_THREADS":"1","MKL_NUM_THREADS":"1"})
    with (OUT/"execution_order.csv").open("w",newline="",encoding="utf-8") as stream:
        w=csv.writer(stream,lineterminator="\n");w.writerow(["run_id","order","library","method"])
        w.writerows([[RUN_ID,i+1,l,m] for i,(l,m) in enumerate(order)])
    for i,(lib,method) in enumerate(order,1):
        input_path=source if lib=="CMINPACK" else INPUTS
        cmd=[str(RUNNER),str(input_path),lib,method,str(OUT),RUN_ID,"21","1"]
        result=subprocess.run(cmd,cwd=ROOT,env=env,text=True,capture_output=True)
        if result.returncode:failures.append({"library":lib,"method":method,"code":result.returncode,"stderr":result.stderr.strip() or "NA"})
        print(f"{i}/{len(order)} edge library-method runs complete: {lib}/{method}",flush=True)
    with (OUT/"execution_failures.csv").open("w",newline="",encoding="utf-8") as stream:
        w=csv.DictWriter(stream,fieldnames=["library","method","code","stderr"],lineterminator="\n");w.writeheader();w.writerows(failures)
    # Mark all planned combinations, including explicitly unsupported/unrun ones.
    with (OUT/"accuracy.csv").open(newline="",encoding="utf-8") as stream:acc=list(csv.DictReader(stream))
    keys={(r["library"],r["requested_method"],r["problem_id"]) for r in acc}
    for r in mapping:
        r["measurement_status"]="measured" if (r["library"],r["method"],r["problem_id"]) in keys else "unsupported/not run"
    with (OUT/"method_workloads.csv").open("w",newline="",encoding="utf-8") as stream:
        w=csv.DictWriter(stream,fieldnames=list(mapping[0]),lineterminator="\n");w.writeheader();w.writerows(mapping)
    metadata={"schema_version":"phase14b-root-edge-v1","run_id":RUN_ID,"batches":21,
      "timed_repeats_per_batch":1,"warmups":2,"cases":len(cases),"planned_library_method_runs":len(order),
      "accuracy_rows":len(acc),"execution_failures":len(failures),
      "limitations":["One-method callback budgets are enforced by NUMERIX; the GSL API has no equivalent per-call evaluation cap.",
        "CMINPACK outputs are omitted on synthetic discontinuity and always-nonfinite callback fixtures because its adapter uses the frozen smooth model callback.",
        "Invalid-expression compilation is a supporting expression API check, not a callback root-solver input; see the separate expression probe record."]}
    (OUT/"run_metadata.json").write_text(json.dumps(metadata,indent=2)+"\n",encoding="utf-8")
    return 1 if failures else 0


if __name__=="__main__":raise SystemExit(main())
