#!/usr/bin/env python3
"""Validate pilot record coverage and basic measurement invariants only."""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import math
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
RESULTS = REPO / "benchmarks/results/phase14c/module_comparisons"
MODULES = {"differentiation": (1116, 36, 36), "interpolation": (744, 24, 24), "integration": (1116, 18, 36)}
GSL_PREFIX = "/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean/lib"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="") as f:
        return list(csv.DictReader(f))


def reference_for(module: str, problem_id: str, method: str, parameters: dict) -> float:
    if module == "differentiation":
        x = float(parameters["x"])
        second = method in {"second", "three_point_second_derivative_wrapper"}
        if problem_id == "D-PILOT-SIN-01": return -math.sin(x) if second else math.cos(x)
        if problem_id == "D-PILOT-EXP-01": return (0.04 if second else 0.2) * math.exp(0.2 * x)
        return 6.0 * x if second else 3.0 * x * x - 2.0
    if module == "interpolation":
        q = float(parameters["query"])
        if problem_id == "I-PILOT-CUBIC-01": return q * q * q - 2.0 * q + 1.0
        if problem_id == "I-PILOT-EXP-01": return math.exp(q)
        return 1.0 / (1.0 + q * q)
    if problem_id == "Q-PILOT-SIN-01": return 2.0
    if problem_id == "Q-PILOT-EXP-01": return math.e - 1.0
    return 0.4


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_id")
    args = parser.parse_args()
    run_id = args.run_id
    checks: dict[str, object] = {"run_id": run_id, "scope": "pilot_only_not_final_verification", "modules": {}, "errors": []}
    manifest_path = RESULTS / "manifests" / f"pilot_workload_{run_id}.csv"
    checksum_path = RESULTS / "manifests" / f"pilot_workload_checksum_{run_id}.txt"
    manifest = read_csv(manifest_path)
    ids = [r["problem_id"] for r in manifest]
    checks["manifest_unique_problem_ids"] = len(ids) == len(set(ids))
    checks["manifest_problems_by_module"] = dict(Counter(r["module"] for r in manifest))
    recorded_hash = checksum_path.read_text().split()[0]
    checks["manifest_checksum_recorded_before_measurement"] = hashlib.sha256(manifest_path.read_bytes()).hexdigest() == recorded_hash
    manifest_by_id = {r["problem_id"]: r for r in manifest}
    linkage = (RESULTS / "manifests" / f"gsl_linkage_{run_id}.txt").read_text()
    checks["isolated_gsl_2_8_linkage"] = GSL_PREFIX in linkage and "libgsl.so.28" in linkage
    for module, expected in MODULES.items():
        folder = RESULTS / "raw" / module / run_id
        timing = read_csv(folder / "raw_timing.csv")
        accuracy = read_csv(folder / "raw_accuracy.csv")
        memory = read_csv(folder / "raw_memory.csv")
        grouped = Counter((r["problem_id"], r["library"], r["numerix_method"], r["competitor_method"]) for r in timing)
        batch_counts_ok = bool(grouped) and all(v == 31 for v in grouped.values())
        timing_ok = all(math.isfinite(float(r["wall_elapsed_seconds"])) and float(r["wall_elapsed_seconds"]) > 0
                        and math.isfinite(float(r["cpu_elapsed_seconds"])) and float(r["cpu_elapsed_seconds"]) >= 0
                        and int(r["batch_size"]) > 0 and int(r["work_counter"]) >= 0
                        for r in timing)
        memory_ok = all(r["measurement_status"] == "unavailable" and not r["peak_additional_live_requested_bytes"]
                        and not r["total_requested_bytes"] for r in memory)
        accuracy_status_ok = all(r["status"] in {"success", "not_applicable"} for r in accuracy)
        accuracy_finite_ok = all((r["status"] == "not_applicable" and r["finite_result"] == "not_applicable")
                                 or (r["status"] == "success" and r["finite_result"] == "1") for r in accuracy)
        version_ok = all(r["library"] != "GSL" or r["library_version"] == "2.8" for r in timing + accuracy + memory)
        order_ok = all(
            (int(r["batch_index"]) % 2 == 1 and
             ((r["library"] == "NUMERIX" and r["batch_order"] == "NUMERIX_first") or
              (r["library"] == "GSL" and r["batch_order"] == "GSL_second"))) or
            (int(r["batch_index"]) % 2 == 0 and
             ((r["library"] == "NUMERIX" and r["batch_order"] == "NUMERIX_second") or
              (r["library"] == "GSL" and r["batch_order"] == "GSL_first")))
            for r in timing)
        reference_ok = True
        pass_flags_ok = True
        for row in accuracy:
            if row["status"] == "not_applicable":
                continue
            manifest_row = manifest_by_id[row["problem_id"]]
            reference = reference_for(module, row["problem_id"], row["method"], json.loads(manifest_row["serialized_parameters"]))
            reference_ok &= math.isclose(float(row["reference"]), reference, rel_tol=2e-15, abs_tol=2e-15)
            finite_success = row["status"] == "success" and row["finite_result"] == "1"
            error = float(row["absolute_error"]) if finite_success else math.inf
            tolerance = float(row["target_absolute"]) + float(row["target_relative"]) * abs(float(row["reference"]))
            expected_pass = finite_success and error <= tolerance
            pass_flags_ok &= row["pass"] == ("1" if expected_pass else "0")
        module_result = {
            "timing_rows": len(timing), "expected_timing_rows": expected[0],
            "accuracy_rows": len(accuracy), "expected_accuracy_rows": expected[1],
            "memory_rows": len(memory), "expected_memory_rows": expected[2],
            "timing_route_groups": len(grouped), "all_route_groups_have_31_batches": batch_counts_ok,
            "library_order_counterbalanced": order_ok,
            "all_times_finite_positive_and_work_nonnegative": timing_ok,
            "memory_explicitly_unavailable_not_zero": memory_ok,
            "accuracy_status_and_finite_fields_consistent": accuracy_status_ok and accuracy_finite_ok,
            "references_recomputed_from_analytic_formulas": reference_ok,
            "accuracy_pass_flags_recomputed": pass_flags_ok,
            "exact_versions_consistent": version_ok,
        }
        expected_rows_ok = (len(timing), len(accuracy), len(memory)) == expected
        if not expected_rows_ok: checks["errors"].append(f"{module}: row counts differ from expected {expected}")
        for label, ok in [("batches", batch_counts_ok), ("order", order_ok), ("timing", timing_ok), ("memory", memory_ok),
                          ("accuracy", accuracy_status_ok and accuracy_finite_ok), ("references", reference_ok),
                          ("pass_flags", pass_flags_ok), ("versions", version_ok)]:
            if not ok: checks["errors"].append(f"{module}: {label} check failed")
        checks["modules"][module] = module_result
    checks["passed_pilot_schema_checks"] = len(checks["errors"]) == 0
    checks["limitations"] = [
        "three problems per module only; the 100-problem/20-family target is unmet",
        "allocator instrumentation was not implemented or validated; all memory values are unavailable",
        "adaptive Simpson and QAG work limits are not matched in this pilot",
        "fixed composite integration versus Gauss-Legendre is best-available, not direct",
        "not a final performance comparison; do not generalize pilot output",
    ]
    target = RESULTS / "verification" / f"pilot_verification_{run_id}.json"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(checks, indent=2) + "\n")
    print(json.dumps({"verification": str(target.relative_to(REPO)), "passed": checks["passed_pilot_schema_checks"], "errors": checks["errors"]}))
    if checks["errors"]: raise SystemExit(1)


if __name__ == "__main__":
    main()
