#!/usr/bin/env python3
"""Run method-level root measurements against the frozen Phase 14 root set."""
from __future__ import annotations

import csv
import hashlib
import json
import os
import platform
import random
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
WORKLOADS = ROOT / "benchmarks/problems/phase14/workloads.csv"
INPUTS = ROOT / "benchmarks/problems/phase14/problem_inputs.jsonl"
RUNNER = ROOT / "build/phase14b_root_runner"
OUT = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
RUN_ID = "phase14b-root-20261112-v3"
SEED = 14112026
NUMERIX_METHODS = ["bisection", "regula_falsi", "secant", "newton",
                    "safeguarded_newton", "brent", "auto_selection",
                    "portfolio", "multiple_root_discovery"]
GSL_METHODS = ["bisection", "regula_falsi", "secant", "newton",
               "safeguarded_newton", "brent", "multiple_root_discovery"]


def sha(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def value(p: dict[str, float | int], x: float) -> float:
    m = int(p["_model"]); c = float(p["root"]); a = float(p["scale"])
    k = float(p["shape"]); d = float(p["shift"]); u = x - c
    if m == 0: return a*u
    if m == 1: return a*u*(u-d)
    if m == 2: return a*(u+0.08*k*u**3)
    if m == 3: return a*u*(1.0+0.03*k*u**4)
    if m == 4: return a*__import__("math").sin(k*u)
    if m == 5: return a*__import__("math").expm1(k*u)
    if m == 6: return a*__import__("math").log((x+d)/(c+d)) if x+d>0 and c+d>0 else float("nan")
    if m == 7: return a*u/(1.0+k*u*u)
    if m == 8: return a*(u+0.1*__import__("math").sin(k*u))
    if m == 9: return a*__import__("math").tanh(k*u)
    if m == 10: return a*__import__("math").atan(k*u)
    if m == 11: return a*__import__("math").sinh(k*u)
    if m == 12: return a*u*(1.0+0.05*u+0.02*u*u)
    if m == 13: return a*u*u
    if m == 14: return a*u**3
    if m == 15: return a*u**5
    if m == 16: return a*(u+0.03*u**3)
    if m == 17: return a*1e-12*u
    if m == 18: return a*u*(1.0+0.2*__import__("math").cos(k*u))
    return a*u*(1.0+0.1*u*u)


def main() -> int:
    started_utc = datetime.now(timezone.utc).isoformat()
    if not RUNNER.exists():
        print(f"runner is missing: {RUNNER}", file=sys.stderr); return 2
    OUT.mkdir(parents=True, exist_ok=True)
    if any(OUT.iterdir()):
        raise RuntimeError(f"refusing to mix with existing results in {OUT}")
    with WORKLOADS.open(newline="", encoding="utf-8") as f:
        rows = [r for r in csv.DictReader(f) if r["module"] == "root"]
    if len(rows) != 100:
        raise RuntimeError(f"expected 100 frozen root problems, got {len(rows)}")

    mapping = []
    methods = [("NUMERIX", m) for m in NUMERIX_METHODS]
    methods += [("GSL", m) for m in GSL_METHODS]
    methods += [("CMINPACK", "newton")]
    for row in rows:
        p = json.loads(row["input_parameters_json"])
        flo, fhi = value(p, float(p["lower"])), value(p, float(p["upper"]))
        bracket = (flo == 0 or fhi == 0 or (flo < 0 < fhi) or (fhi < 0 < flo))
        for library, method in methods:
            applicable = True; reason = "within documented method input domain"
            if method in {"bisection", "regula_falsi", "brent", "safeguarded_newton"}:
                applicable = bracket
                reason = "finite sign-changing bracket" if bracket else "not eligible: endpoints do not bracket a sign change"
            elif method == "secant":
                applicable = float(p["initial"]) != float(p["second_initial"])
                reason = "distinct finite starting values" if applicable else "not eligible: identical starts"
            if library == "CMINPACK":
                comp = "best_available: MINPACK hybrid system solver, not scalar Newton"
            elif method in {"auto_selection", "portfolio"}:
                comp = "NUMERIX-only orchestration API"
            elif method == "multiple_root_discovery":
                comp = "best_available: fixed-resolution GSL scan plus Brent refinement; heuristic"
            elif method == "safeguarded_newton" and library == "GSL":
                comp = "best_available: GSL Brent solver, not safeguarded Newton"
            else:
                comp = "direct algorithm-family comparison"
            mapping.append({"run_id": RUN_ID, "problem_id": row["problem_id"],
                "family": row["family"], "method": method, "library": library,
                "applicable": int(applicable), "applicability_reason": reason,
                "comparison_class": "direct" if comp.startswith("direct") else "best_available",
                "comparison_detail": comp, "workload_source": "frozen phase14 workload",
                "input_sha256": row["input_sha256"]})
    with (OUT / "method_workloads.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(mapping[0]), lineterminator="\n")
        w.writeheader(); w.writerows(mapping)

    order = [(lib, method) for lib, method in methods]
    random.Random(SEED).shuffle(order)
    failures = []
    env = os.environ.copy()
    env.update({"OPENBLAS_NUM_THREADS":"1", "OMP_NUM_THREADS":"1", "MKL_NUM_THREADS":"1"})
    with (OUT / "execution_order.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n"); w.writerow(["run_id","seed","order","library","method"])
        for idx,(lib,method) in enumerate(order,1):
            w.writerow([RUN_ID,SEED,idx,lib,method])
    total = len(order)
    for idx,(library,method) in enumerate(order,1):
        # Keep the batch work long enough to resolve small solver calls, while
        # avoiding repeated discovery scans that swamp the measured operation.
        repeats = 1 if method == "multiple_root_discovery" else (2 if method in {"auto_selection","portfolio"} else 4)
        cmd = [str(RUNNER),str(INPUTS),library,method,str(OUT),RUN_ID,"31",str(repeats)]
        result = subprocess.run(cmd,cwd=ROOT,env=env,text=True,capture_output=True)
        if result.returncode:
            failures.append({"library":library,"method":method,"exit_code":result.returncode,
                             "stderr":result.stderr.strip() or "NA"})
        print(f"{idx}/{total} root library-method runs finished: {library}/{method}",flush=True)
    with (OUT / "execution_failures.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=["library","method","exit_code","stderr"],lineterminator="\n")
        w.writeheader();w.writerows(failures)
    meta={"schema_version":"phase14b-root-run-v1","run_id":RUN_ID,
          "started_utc":started_utc,"completed_utc":datetime.now(timezone.utc).isoformat(),
          "git_head":subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,text=True).strip(),
          "git_branch":subprocess.check_output(["git","branch","--show-current"],cwd=ROOT,text=True).strip(),
          "initial_commit":"cbc3d27bb89451f66f938cfe245274f0dccb37b4",
          "platform":platform.platform(),"python":sys.version,
          "compiler":subprocess.run(["cc","--version"],capture_output=True,text=True).stdout.splitlines()[0],
          "build_configuration":"Release-like -O3 -DNDEBUG; C11; -Wall -Wextra -Wpedantic",
          "dependency_versions":{"GSL":"2.8","CMINPACK":"1.3.15","NUMERIX":"0.2.0-rc1"},
          "batches":31,"warmups":2,"repeats_by_method":{"discovery":1,"orchestration":2,"iterative":4},
          "seed":SEED,"input_manifest_sha256":sha(INPUTS),"workload_manifest_sha256":sha(WORKLOADS),
          "libraries_attempted":sorted({lib for lib,_ in methods}),"execution_failures":len(failures),
          "limitations":["Root discovery comparison is heuristic and separately labeled.",
            "The GSL discovery baseline uses a fixed 4096-panel sign-change scan and does not guarantee detection of tangencies.",
            "Root solvers use a common 500-iteration cap in this phase-level adapter; this is lower than the Phase 14 runner's cap to bound difficult method/workload combinations.",
            "Function/derivative counters are enabled only in the untimed probe; they are disabled for warmups and all timing batches.",
            "CPU clock resolution can be zero for short batches; raw values are retained."]}
    (OUT/"run_metadata.json").write_text(json.dumps(meta,indent=2)+"\n",encoding="utf-8")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
