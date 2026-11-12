#!/usr/bin/env python3
"""Summarize per-method root edge outcomes without hiding rejected inputs."""
from __future__ import annotations

import csv
import json
from collections import Counter, defaultdict
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
DATA=ROOT/"benchmarks/results/phase14b/edge_cases/root"
OUT=ROOT/"benchmarks/results/phase14b/analysis/root_edges"


def read(path:Path)->list[dict[str,str]]:
    with path.open(newline="",encoding="utf-8") as f:return list(csv.DictReader(f))


def main()->int:
    OUT.mkdir(parents=True,exist_ok=True)
    rows=read(DATA/"accuracy.csv");planned=read(DATA/"method_workloads.csv")
    keys={(r["library"],r["requested_method"],r["problem_id"]) for r in rows}
    groups=defaultdict(list)
    for r in rows:groups[(r["library"],r["requested_method"])].append(r)
    summaries=[]
    for (library,method),items in sorted(groups.items()):
        all_attempts=len(items);applicable=sum(r["applicable"]=="1" for r in items)
        succeeded=sum(r["status"]=="0" and r["converged"]=="1" for r in items)
        passed=sum(r["accuracy_pass"]=="1" for r in items)
        joint=sum(r["status"]=="0" and r["converged"]=="1" and r["accuracy_pass"]=="1" for r in items)
        statuses=Counter(r["status"] for r in items)
        summaries.append({"library":library,"method":method,"attempted_edge_cases":all_attempts,
          "applicable_edge_cases":applicable,"successful_status":succeeded,"accuracy_passes":passed,
          "successful_and_accurate":joint,"failed_or_rejected":all_attempts-succeeded,
          "status_code_counts":json.dumps(dict(statuses),sort_keys=True),
          "correct_rejections_recorded":"see edge_outcomes.csv; expected-behavior adjudication is per case"})
    with (OUT/"method_summary.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=list(summaries[0]),lineterminator="\n");w.writeheader();w.writerows(summaries)
    by_id={r["problem_id"]:r for r in read(ROOT/"benchmarks/results/phase14b/manifests/root_edge_cases.csv")}
    details=[]
    for r in rows:
        m=by_id.get(r["problem_id"],{})
        observation=""
        status=int(r["status"])
        if "MAXITER1" in r["problem_id"]:
            observation="Iteration-limit fixture; success is valid only if the requested tolerance was reached within one iteration."
        elif "EVALCAP1" in r["problem_id"] and r["library"]=="NUMERIX":
            observation="NUMERIX reached the configured one-evaluation resource limit; other libraries do not share this budget contract."
        elif "CANCEL" in r["problem_id"] and r["requested_method"]=="multiple_root_discovery":
            observation="NUMERIX discovery progress callback cancellation is recorded as a cancelled search, not a numerical failure."
        elif "discontinuous_jump" in r["problem_id"]:
            observation="Any sign-change-based root claim is inaccurate because the discontinuous callback has no zero."
        elif "reversed_interval" in r["problem_id"]:
            observation="A reversed discovery interval is rejected; open Newton/secant APIs do not use the bracket."
        elif "zero_derivative_nonroot" in r["problem_id"] and r["requested_method"]=="newton":
            observation="Zero derivative away from a root must not be reported as an accurate root."
        elif "identical_secant_starts" in r["problem_id"] and r["requested_method"]=="secant":
            observation="Coincident secant starts exercise the zero-denominator guard."
        if status!=0 and r["applicable"]=="0":
            category="rejected_or_failed_outside_assumptions"
        elif status!=0:
            category="failed_or_resource_limited"
        elif r["accuracy_pass"]=="0":
            category="successful_status_but_inaccurate_or_incomplete"
        else:
            category="successful_and_accurate"
        details.append({"library":r["library"],"method":r["requested_method"],"problem_id":r["problem_id"],
          "edge_case":m.get("edge_case",""),"applicable":r["applicable"],"status":r["status"],
          "converged":r["converged"],"accuracy_pass":r["accuracy_pass"],"absolute_error":r["absolute_error"],
          "residual":r["residual"],"function_evaluations":r["function_evaluations"],
          "expected_roots":r["expected_roots"],"discovered_roots":r["discovered_roots"],
          "missed_roots":r["missed_roots"],"false_positives":r["false_positives"],
          "outcome_category":category,"expected_behavior":m.get("expected_behavior",""),"observation":observation})
    with (OUT/"edge_outcomes.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=list(details[0]),lineterminator="\n");w.writeheader();w.writerows(details)
    missing=[r for r in planned if r["measurement_status"]!="measured"]
    verify={"accuracy_rows":len(rows),"timing_rows":len(read(DATA/"raw_timing.csv")),
      "planned_method_case_pairs":len(planned),"measured_pairs":len(keys),"unmeasured_pairs":missing,
      "unique_accuracy_keys":len(keys)==len(rows),"timing_batches_per_measured_pair":21,
      "expression_probe_passed":read(DATA/"expression_invalid_probe.csv")[0]["passed"]=="1",
      "interpretation":"Correct rejection is not automatically a failure; see outcome_category and case expectations."}
    (OUT/"analysis_verification.json").write_text(json.dumps(verify,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"accuracy_rows":len(rows),"timing_rows":verify["timing_rows"],
       "planned_pairs":len(planned),"measured_pairs":len(keys),"unmeasured":len(missing)},indent=2))
    return 0 if verify["unique_accuracy_keys"] and verify["expression_probe_passed"] else 1


if __name__=="__main__":raise SystemExit(main())
