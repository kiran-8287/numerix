#!/usr/bin/env python3
"""Summarize Phase 14B root data with problem-level paired timing analysis."""
from __future__ import annotations

import csv
import hashlib
import json
import random
import statistics
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DATA = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
OUT = ROOT / "benchmarks/results/phase14b/analysis/root"
RUN_ID = "phase14b-root-20261112-v3"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def sha(path: Path) -> str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda:f.read(1024*1024),b""):
            h.update(b)
    return h.hexdigest()


def percentile(values: list[float], q: float) -> float:
    xs=sorted(values)
    if not xs:return float("nan")
    pos=(len(xs)-1)*q; lo=int(pos); hi=min(lo+1,len(xs)-1)
    return xs[lo]+(xs[hi]-xs[lo])*(pos-lo)


def bootstrap_median(values: list[float], seed: int) -> tuple[float,float]:
    if not values:return float("nan"),float("nan")
    rng=random.Random(seed); estimates=[]; n=len(values)
    for _ in range(5000):
        sample=[values[rng.randrange(n)] for _ in range(n)]
        estimates.append(statistics.median(sample))
    return percentile(estimates,0.025),percentile(estimates,0.975)


def main() -> int:
    OUT.mkdir(parents=True,exist_ok=True)
    accuracy=read_csv(DATA/"accuracy.csv")
    timing=read_csv(DATA/"raw_timing.csv")
    workloads=read_csv(DATA/"method_workloads.csv")
    times: dict[tuple[str,str,str],list[float]]=defaultdict(list)
    cpu: dict[tuple[str,str,str],list[float]]=defaultdict(list)
    for r in timing:
        key=(r["library"],r["requested_method"],r["problem_id"])
        size=float(r["batch_size"])
        times[key].append(float(r["wall_seconds"])/size)
        c=float(r["cpu_seconds"])/size
        if c>0:cpu[key].append(c)
    by_key={(r["library"],r["requested_method"],r["problem_id"]):r for r in accuracy}
    groups: dict[tuple[str,str],list[dict[str,str]]]=defaultdict(list)
    for r in accuracy:groups[(r["library"],r["requested_method"])].append(r)
    summary=[]
    for (library,method),rows in sorted(groups.items()):
        all_wall=[]; all_cpu=[]; all_errors=[]; raw_wall=[]; raw_cpu=[]
        successful=accurate=eligible=joint=0; failures=Counter()
        for r in rows:
            key=(library,method,r["problem_id"])
            if key in times:all_wall.append(statistics.median(times[key]))
            if cpu[key]:all_cpu.append(statistics.median(cpu[key]))
            raw_wall.extend(times.get(key,[]))
            raw_cpu.extend(cpu.get(key,[]))
            try:
                error=float(r["absolute_error"])
                if error>=0 and error<float("inf"):all_errors.append(error)
            except ValueError:pass
            successful+=r["status"]=="0" and r["converged"]=="1"
            accurate+=r["accuracy_pass"]=="1"
            joint+=r["status"]=="0" and r["converged"]=="1" and r["accuracy_pass"]=="1"
            eligible+=r["applicable"]=="1"
            if r["status"]!="0":failures[r["status"]]+=1
        summary.append({"run_id":RUN_ID,"library":library,"method":method,
          "attempted_problems":len(rows),"applicable_problems":eligible,
          "status_successes":successful,"accuracy_passes":accurate,
          "successful_and_accurate":joint,"failure_status_counts":json.dumps(dict(failures),sort_keys=True),
          "success_rate_all_attempts":successful/len(rows) if rows else "",
          "accuracy_pass_rate_all_attempts":accurate/len(rows) if rows else "",
          "successful_and_accurate_rate_all_attempts":joint/len(rows) if rows else "",
          "median_wall_seconds_per_call":statistics.median(raw_wall) if raw_wall else "",
          "mean_wall_seconds_per_call":statistics.mean(raw_wall) if raw_wall else "",
          "sd_wall_seconds_per_call":statistics.stdev(raw_wall) if len(raw_wall)>1 else "",
          "min_wall_seconds_per_call":min(raw_wall) if raw_wall else "",
          "p95_wall_seconds_per_call":percentile(raw_wall,.95) if raw_wall else "",
          "max_wall_seconds_per_call":max(raw_wall) if raw_wall else "",
          "median_problem_median_wall_seconds_per_call":statistics.median(all_wall) if all_wall else "",
          "p95_problem_median_wall_seconds_per_call":percentile(all_wall,.95) if all_wall else "",
          "median_positive_cpu_seconds_per_call":statistics.median(raw_cpu) if raw_cpu else "",
          "mean_positive_cpu_seconds_per_call":statistics.mean(raw_cpu) if raw_cpu else "",
          "sd_positive_cpu_seconds_per_call":statistics.stdev(raw_cpu) if len(raw_cpu)>1 else "",
          "min_positive_cpu_seconds_per_call":min(raw_cpu) if raw_cpu else "",
          "max_positive_cpu_seconds_per_call":max(raw_cpu) if raw_cpu else "",
          "zero_cpu_batch_count":sum(float(t["cpu_seconds"])==0 for t in timing if t["library"]==library and t["requested_method"]==method),
          "timing_batch_sample_count":len(raw_wall),"positive_cpu_batch_sample_count":len(raw_cpu),
          "median_absolute_error":statistics.median(all_errors) if all_errors else "",
          "timing_problem_count":len(all_wall),"positive_cpu_problem_count":len(all_cpu),
          "timing_batches_per_problem":31,"warmups_per_problem":2})
    with (OUT/"method_summary.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=list(summary[0]),lineterminator="\n");w.writeheader();w.writerows(summary)

    direct=["bisection","regula_falsi","secant","newton","brent"]
    comparisons=[]
    for method in direct:
        pairs=[];excluded=0
        for r in [x for x in workloads if x["method"]==method and x["library"]=="NUMERIX"]:
            pid=r["problem_id"]
            n=by_key.get(("NUMERIX",method,pid));g=by_key.get(("GSL",method,pid))
            if not n or not g or n["applicable"]!="1" or g["applicable"]!="1":
                excluded+=1;continue
            if n["status"]!="0" or g["status"]!="0" or n["accuracy_pass"]!="1" or g["accuracy_pass"]!="1":
                excluded+=1;continue
            nt=times.get(("NUMERIX",method,pid),[]);gt=times.get(("GSL",method,pid),[])
            if not nt or not gt:excluded+=1;continue
            nm=statistics.median(nt);gm=statistics.median(gt)
            if nm<=0 or gm<=0:excluded+=1;continue
            pairs.append((pid,gm/nm))
        ratios=[x[1] for x in pairs];lo,hi=bootstrap_median(ratios,SEED_FOR(method=method))
        comparisons.append({"run_id":RUN_ID,"method":method,"comparison_class":"direct algorithm-family",
          "ratio_definition":"GSL median wall time / NUMERIX median wall time; >1 favors NUMERIX",
          "eligible_paired_problems":len(pairs),"excluded_problems":excluded,
          "median_paired_speed_ratio":statistics.median(ratios) if ratios else "",
          "bootstrap_95ci_low":lo if ratios else "","bootstrap_95ci_high":hi if ratios else "",
          "bootstrap_resamples":5000 if ratios else 0,
          "paired_problem_ids":"|".join(pid for pid,_ in pairs),
          "inference_note":"Exploratory paired problem bootstrap; no hypothesis test or familywise significance claim. Holm correction was considered but not applied because this descriptive study does not claim significance."})
    with (OUT/"paired_comparisons.csv").open("w",newline="",encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=list(comparisons[0]),lineterminator="\n");w.writeheader();w.writerows(comparisons)

    planned={(r["library"],r["method"]) for r in workloads}
    observed={(r["library"],r["requested_method"]) for r in accuracy}
    verify={"schema_version":"phase14b-root-analysis-v2","run_id":RUN_ID,
      "accuracy_rows":len(accuracy),"raw_timing_rows":len(timing),"planned_method_library_pairs":len(planned),
      "observed_method_library_pairs":len(observed),"unobserved_pairs":[list(x) for x in sorted(planned-observed)],
      "accuracy_rows_unique":len({(r["library"],r["requested_method"],r["problem_id"]) for r in accuracy})==len(accuracy),
      "timing_rows_per_pair_problem":31,"raw_timing_checksum_sha256":sha(DATA/"raw_timing.csv"),
      "accuracy_checksum_sha256":sha(DATA/"accuracy.csv"),"workload_map_checksum_sha256":sha(DATA/"method_workloads.csv"),
      "statistical_unit":"problem; each problem contributes the median of 31 batch measurements",
      "time_to_accuracy":"not measured in this root run; thresholds were evaluated at one configured tolerance",
      "memory":"not measured at method scope; Phase 14 process RSS is not reused as per-method allocation evidence"}
    (OUT/"analysis_verification.json").write_text(json.dumps(verify,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({"accuracy_rows":len(accuracy),"timing_rows":len(timing),
        "planned_pairs":len(planned),"observed_pairs":len(observed),"unobserved":verify["unobserved_pairs"]},indent=2))
    return 0 if planned==observed and verify["accuracy_rows_unique"] else 1


def SEED_FOR(method: str) -> int:
    return 14112026 + sum(ord(c) for c in method)


if __name__=="__main__":
    raise SystemExit(main())
