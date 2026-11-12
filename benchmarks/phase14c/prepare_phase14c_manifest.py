#!/usr/bin/env python3
"""Prepare an isolated Phase 14C audit manifest from preserved Phase 14 inputs.

This records candidate workload reuse and method coverage; it does not create
Phase 14C measurements or rewrite any prior-phase files.
"""
from __future__ import annotations

import csv
import hashlib
import json
import re
import shutil
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "benchmarks/results/phase14c/module_comparisons/manifests"
OLD_WORKLOAD = ROOT / "benchmarks/problems/phase14/workloads.csv"
OLD_INPUTS = ROOT / "benchmarks/problems/phase14/problem_inputs.jsonl"
OLD_METHODS = ROOT / "benchmarks/analysis/phase14b/method_coverage.csv"
ROOT_METHODS = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/method_workloads.csv"
HEADER = ROOT / "include/numerix.h"

PRIMARY = {
    "root": ("GSL", "2.8"),
    "linear_algebra": ("LAPACK with OpenBLAS", "OpenBLAS 0.3.34; bundled LAPACK 3.12.0"),
    "differentiation": ("GSL", "2.8"),
    "interpolation": ("GSL", "2.8"),
    "integration": ("GSL", "2.8"),
    "optimization": ("NLopt", "2.10.1"),
    "ode": ("SUNDIALS", "7.9.0"),
}
EXPECTED_METHODS = {
    # Phase 14B's 49-row historical method list is the baseline; the current
    # public API audit adds the separately listed LU destruction lifecycle.
    "root": 9, "linear_algebra": 21, "differentiation": 7,
    "interpolation": 4, "integration": 4, "optimization": 2, "ode": 3,
}
SPECIAL_INPUTS = {
    "numerix_piecewise_diff_central": "Requires frozen piecewise-function fixtures; scalar Phase 14 differentiation cases are not piecewise objects.",
    "numerix_piecewise_integrate_trapezoid": "Requires frozen piecewise interval fixtures; scalar Phase 14 integration cases are not piecewise objects.",
}


def rows(path: Path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, fieldnames, items):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(items)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def method_mapping(module, old_row):
    operation = old_row["method"]
    symbols = set(re.findall(r"\bnumerix_[A-Za-z0-9_]+", old_row["public_symbol"]))
    if module == "linear_algebra":
        source = ROOT / "benchmarks/phase14c/linear_algebra/linear_algebra_method_inventory.csv"
        for item in rows(source):
            if item["operation"] == operation:
                return item["competitor_api"], item["comparison_class"]
    elif module in {"differentiation", "interpolation", "integration"}:
        source = ROOT / "benchmarks/results/phase14c/module_comparisons/manifests/gsl_scalar_modules_inventory.csv"
        if source.exists():
            for item in rows(source):
                item_symbols = set(re.findall(r"\bnumerix_[A-Za-z0-9_]+", item["public_api"]))
                if item_symbols == symbols:
                    return item["competitor_api"], item["comparison_type"]
    elif module in {"optimization", "ode"}:
        source = ROOT / "benchmarks/phase14c/optimization_ode_inventory.csv"
        if source.exists():
            for item in rows(source):
                if item["operation"] == operation:
                    return item["competitor_route"], item["comparison_class"]
    return old_row["competitor_equivalent"], old_row["comparison_class"]


def normalize_class(value, kind):
    key = value.lower().replace("-", "_")
    if kind == "lifecycle" or "allocation_only" in key or "lifecycle" in key:
        return "lifecycle_only"
    if "not_comparable" in key or "numerix_specific" in key or "no equivalent" in key:
        return "not_comparable"
    if "numerix_only" in key or "orchestration" in key:
        return "numerix_only"
    if "best_available" in key or "best available" in key or "wrapper" in key:
        return "best_available"
    if "direct" in key:
        return "direct"
    return "comparison_class_needs_review"


def evidence_scope(module: str, public_api: str, kind: str) -> tuple[str, str]:
    symbols = set(re.findall(r"\bnumerix_[A-Za-z0-9_]+", public_api))
    if module == "root":
        return "historical Phase 14B v3 only; not new Phase 14C data", "historical_only"
    if module == "linear_algebra":
        piloted = {
            "numerix_matrix_add", "numerix_matrix_multiply",
            "numerix_lu_decompose", "numerix_lu_solve",
        }
        if symbols & piloted:
            return "Phase 14C pilot: four workloads, 31 batches per route; not full study", "pilot_runtime_accuracy"
        return "No Phase 14C measurement for this operation", "not_measured"
    if module in {"differentiation", "interpolation", "integration"}:
        if symbols & {"numerix_piecewise_diff_central", "numerix_piecewise_integrate_trapezoid"}:
            return "No Phase 14C measurement; no verified GSL equivalent", "not_measured_not_comparable"
        return "Phase 14C adapter pilot: three problems per module, 31 batches per route; not full study", "pilot_runtime_accuracy"
    if module == "optimization":
        return "Phase 14C correctness-only pilot: one analytic case per method; no timing or memory", "pilot_correctness_only"
    if module == "ode":
        return "Phase 14C correctness-only pilot: one two-state case, full 21-point trajectory; no timing or memory", "pilot_correctness_only"
    return "No Phase 14C measurement", "not_measured"


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    old = rows(OLD_METHODS)
    header_text = HEADER.read_text(encoding="utf-8")
    if len(old) != 49:
        raise SystemExit(f"Expected 49 Phase 14B audit rows; found {len(old)}")
    historical_counts = Counter(row["module"] for row in old)
    if dict(historical_counts) != {**EXPECTED_METHODS, "linear_algebra": 20}:
        raise SystemExit(f"Unexpected Phase 14B baseline coverage counts: {dict(historical_counts)}")

    inventory = []
    for row in old:
        module = row["module"]
        symbols = re.findall(r"\bnumerix_[A-Za-z0-9_]+", row["public_symbol"])
        if not symbols:
            raise SystemExit(f"Missing symbol/source in inventory row {row['method']}")
        absent = [name for name in symbols if name not in header_text]
        if absent:
            raise SystemExit(f"API audit mismatch for {row['method']}: declarations missing from header={absent}")
        implementation_sources = set()
        for name in symbols:
            definition = re.compile(r"(?m)^\s*(?:numerix_status|void|double|size_t|const\s+char\s*\*)\s+" + re.escape(name) + r"\s*\(")
            matches = [path for path in (ROOT / "src").rglob("*.c") if definition.search(path.read_text(encoding="utf-8", errors="replace"))]
            if not matches:
                raise SystemExit(f"No current C implementation definition found for {name}")
            implementation_sources.update(str(path.relative_to(ROOT)) for path in matches)
        source_correction = ""
        audited_sources = "|".join(sorted(implementation_sources))
        if audited_sources != row["implementation_source"]:
            source_correction = f"Phase14B source field `{row['implementation_source']}` corrected by current definition scan."
        kind = "lifecycle" if "lifecycle" in row["method"].lower() else "numerical"
        comp_method, comp_class = method_mapping(module, row)
        comparison = normalize_class(comp_class, kind)
        primary_name, primary_version = PRIMARY[module]
        if kind == "numerical" and not comp_method:
            comparison = "not_comparable"
            comp_method = "No verified equivalent; retain NUMERIX outcome"
        if module == "root":
            history = "Phase 14B v3 method-level run exists; historical only"
        else:
            phase14 = row["phase14_coverage"].lower()
            history = "Selected method path exists in Phase 14 main" if "measured" in phase14 else "No matching method-level historical route verified"
        scope, status = evidence_scope(module, row["public_symbol"], kind)
        inventory.append({
            "module": module,
            "operation": row["method"],
            "public_api": row["public_symbol"],
            "kind": kind,
            "implementation_source": audited_sources,
            "phase14c_primary_competitor": primary_name,
            "primary_competitor_version": primary_version,
            "competitor_method": comp_method,
            "comparison_type": comparison,
            "phase14_coverage_note": row["phase14_coverage"],
            "historical_data_status": history,
            "phase14c_status": status,
            "phase14c_evidence_scope": scope,
            "notes": (row["required_edge_cases"] + (" " + source_correction if source_correction else "")),
        })

    # Reconcile current public API inventory entries omitted by the historical
    # list, while keeping non-numerical lifecycle work explicitly classified.
    la_path = ROOT / "benchmarks/phase14c/linear_algebra/linear_algebra_method_inventory.csv"
    audited_la = rows(la_path)
    known_la_operations = {row["operation"] for row in inventory if row["module"] == "linear_algebra"}
    for item in audited_la:
        if item["operation"] in known_la_operations:
            continue
        symbols = re.findall(r"\bnumerix_[A-Za-z0-9_]+", item["public_api"])
        if not symbols:
            raise SystemExit(f"Missing current public symbol for supplemental API row {item['operation']}")
        implementation_sources = set()
        for name in symbols:
            if name not in header_text:
                raise SystemExit(f"API audit mismatch for {item['operation']}: {name} not in public header")
            definition = re.compile(r"(?m)^\s*(?:numerix_status|void|double|size_t|const\s+char\s*\*)\s+" + re.escape(name) + r"\s*\(")
            matches = [path for path in (ROOT / "src").rglob("*.c") if definition.search(path.read_text(encoding="utf-8", errors="replace"))]
            if not matches:
                raise SystemExit(f"No current C implementation definition found for supplemental API {name}")
            implementation_sources.update(str(path.relative_to(ROOT)) for path in matches)
        inventory.append({
            "module": "linear_algebra",
            "operation": item["operation"],
            "public_api": item["public_api"],
            "kind": item["kind"],
            "implementation_source": "|".join(sorted(implementation_sources)),
            "phase14c_primary_competitor": item["competitor"],
            "primary_competitor_version": PRIMARY["linear_algebra"][1],
            "competitor_method": item["competitor_api"],
            "comparison_type": normalize_class(item["comparison_class"], item["kind"]),
            "phase14_coverage_note": "Not separately measured in Phase 14; lifecycle-only API.",
            "historical_data_status": "No matching lifecycle measurement verified",
            "phase14c_status": "lifecycle_scope_pending",
            "phase14c_evidence_scope": "No Phase 14C lifecycle measurement",
            "notes": item["notes"],
        })

    counts = Counter(row["module"] for row in inventory)
    if len(inventory) != 50 or dict(counts) != EXPECTED_METHODS:
        raise SystemExit(f"Unexpected audited method coverage counts: {dict(counts)}")
    coverage_path = OUT / "method_coverage.csv"
    write_csv(coverage_path, list(inventory[0]), inventory)

    # Lifecycle/setup APIs associated with numerical outputs are inventoried
    # separately so they are not mistaken for additional numerical algorithms.
    auxiliary_lifecycle = [
        {"module": "root", "public_api": "numerix_root_search_report_destroy", "operation": "Release multiple-root discovery report", "related_numerical_api": "numerix_root_find_all", "status": "lifecycle_only_not_separately_measured"},
        {"module": "interpolation", "public_api": "numerix_interpolant_destroy", "operation": "Release Newton interpolant storage", "related_numerical_api": "numerix_interp_newton_create / numerix_interp_newton_evaluate", "status": "cleanup_exercised_in_pilot; lifecycle_cost_not_separately_measured"},
        {"module": "differentiation + integration", "public_api": "numerix_piecewise_create", "operation": "Create piecewise function object", "related_numerical_api": "numerix_piecewise_diff_central / numerix_piecewise_integrate_trapezoid", "status": "not_measured"},
        {"module": "differentiation + integration", "public_api": "numerix_piecewise_add_expression", "operation": "Add expression segment to piecewise function", "related_numerical_api": "numerix_piecewise_diff_central / numerix_piecewise_integrate_trapezoid", "status": "not_measured"},
        {"module": "differentiation + integration", "public_api": "numerix_piecewise_destroy", "operation": "Release piecewise function object", "related_numerical_api": "numerix_piecewise_diff_central / numerix_piecewise_integrate_trapezoid", "status": "not_measured"},
        {"module": "ode", "public_api": "numerix_ode_solution_destroy", "operation": "Release ODE trajectory storage", "related_numerical_api": "numerix_ode_euler / numerix_ode_heun / numerix_ode_rk4", "status": "cleanup_exercised_in_pilot; lifecycle_cost_not_separately_measured"},
    ]
    for item in auxiliary_lifecycle:
        name = item["public_api"]
        if name not in header_text:
            raise SystemExit(f"Auxiliary lifecycle API missing from public header: {name}")
        definition = re.compile(r"(?m)^\s*(?:numerix_status|void|double|size_t|const\s+char\s*\*)\s+" + re.escape(name) + r"\s*\(")
        matches = [path for path in (ROOT / "src").rglob("*.c") if definition.search(path.read_text(encoding="utf-8", errors="replace"))]
        if not matches:
            raise SystemExit(f"No current implementation definition found for auxiliary lifecycle API {name}")
        item["implementation_source"] = "|".join(sorted(str(path.relative_to(ROOT)) for path in matches))
        item["comparison_type"] = "lifecycle_only"
        item["competitor_equivalent"] = "not applicable as a numerical method"
    write_csv(OUT / "auxiliary_lifecycle_coverage.csv", list(auxiliary_lifecycle[0]), auxiliary_lifecycle)

    # Reuse the 700 immutable Phase 14 inputs as a candidate workload set, not
    # as new measurements. Keep exact original bytes and checksums.
    base = rows(OLD_WORKLOAD)
    inputs = {}
    for line in OLD_INPUTS.read_text(encoding="utf-8").splitlines():
        item = json.loads(line)
        if item["problem_id"] in inputs:
            raise SystemExit(f"Duplicate sidecar problem ID {item['problem_id']}")
        inputs[item["problem_id"]] = item
    if len(base) != 700 or len({r["problem_id"] for r in base}) != 700 or set(inputs) != {r["problem_id"] for r in base}:
        raise SystemExit("Phase 14 workload must contain 700 unique inputs linked to its JSONL")
    for row in base:
        item = inputs[row["problem_id"]]
        canonical = json.dumps(item["input_parameters"], sort_keys=True, separators=(",", ":"))
        if hashlib.sha256(canonical.encode()).hexdigest() != row["input_sha256"] or item["input_sha256"] != row["input_sha256"]:
            raise SystemExit(f"Input checksum mismatch: {row['problem_id']}")
    by_module = Counter(row["module"] for row in base)
    if any(by_module[module] != 100 for module in PRIMARY):
        raise SystemExit(f"Unexpected per-module workload counts: {dict(by_module)}")
    input_copy = OUT / "problem_inputs.jsonl"
    workload_copy = OUT / "workload_manifest.csv"
    shutil.copyfile(OLD_INPUTS, input_copy)
    shutil.copyfile(OLD_WORKLOAD, workload_copy)

    # Route-by-problem applicability is a plan. Root rows are sourced from the
    # actual Phase 14B applicability table. Other routes are explicitly marked
    # planned, with piecewise APIs held out until their own fixtures exist.
    applicability = []
    root_rows = rows(ROOT_METHODS)
    root_by_id = {(r["problem_id"], r["method"], r["library"]): r for r in root_rows}
    if len(root_rows) != 1700:
        raise SystemExit(f"Expected 1700 historical root route rows; found {len(root_rows)}")
    for item in inventory:
        module = item["module"]
        symbols = set(re.findall(r"\bnumerix_[A-Za-z0-9_]+", item["public_api"]))
        if module == "root":
            method = {
                "Bisection": "bisection", "Regula Falsi": "regula_falsi", "Secant": "secant",
                "Newton-Raphson": "newton", "Safeguarded Newton": "safeguarded_newton",
                "Brent": "brent", "Automatic method selection": "auto_selection",
                "Solver portfolio": "portfolio", "Multiple-root discovery": "multiple_root_discovery",
            }[item["operation"]]
            candidate = [r for (pid, m, lib), r in root_by_id.items() if m == method and lib == "NUMERIX"]
            for r in candidate:
                applicability.append({
                    "module": module, "problem_id": r["problem_id"], "operation": item["operation"],
                    "public_api": item["public_api"], "applicability_status": "historically_verified_applicable" if r["applicable"] == "1" else "historically_not_applicable",
                    "reason": r["applicability_reason"], "source_run_id": r["run_id"], "input_sha256": r["input_sha256"],
                    "phase14c_measurement_status": "not_new_phase14c_measurement",
                })
            continue
        for workload in base:
            if workload["module"] != module:
                continue
            special = next((SPECIAL_INPUTS[s] for s in symbols if s in SPECIAL_INPUTS), None)
            if item["kind"] == "lifecycle":
                app_status, reason = "planned_lifecycle_scope", "Measure allocation/create/destroy separately; this row is not a numerical method."
            elif special:
                app_status, reason = "requires_supplemental_fixture", special
            elif module == "ode":
                app_status, reason = "requires_full_trajectory_reference", "Existing historical ODE reference is final-state only; Phase 14C needs frozen references at all output points."
            else:
                app_status, reason = "candidate_applicable_not_yet_executed", "Same frozen Phase 14 problem definition; method-specific API dispatch and adapter pilot remain required."
            applicability.append({
                "module": module, "problem_id": workload["problem_id"], "operation": item["operation"],
                "public_api": item["public_api"], "applicability_status": app_status,
                "reason": reason, "source_run_id": "phase14-main-20261112",
                "input_sha256": workload["input_sha256"],
                "phase14c_measurement_status": "planned_only_no_phase14c_result",
            })
    coverage_cols = list(applicability[0])
    write_csv(OUT / "method_workload_coverage.csv", coverage_cols, applicability)

    source_paths = [
        HEADER,
        OLD_METHODS,
        ROOT / "benchmarks/phase14c/linear_algebra/linear_algebra_method_inventory.csv",
        ROOT / "benchmarks/results/phase14c/module_comparisons/manifests/gsl_scalar_modules_inventory.csv",
        ROOT / "benchmarks/phase14c/optimization_ode_inventory.csv",
        OLD_WORKLOAD,
        OLD_INPUTS,
        ROOT_METHODS,
        ROOT / "benchmarks/results/phase14/main/raw_timing.csv",
        ROOT / "benchmarks/results/phase14/main/accuracy.csv",
        ROOT / "benchmarks/results/phase14/main/memory.csv",
        ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/raw_timing.csv",
        ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/accuracy.csv",
    ]
    checksums = []
    for path in source_paths:
        if path.is_file():
            if path == HEADER or "linear_algebra_method_inventory" in path.name or "gsl_scalar_modules_inventory" in path.name or "optimization_ode_inventory" in path.name:
                role = "current_api_or_method_inventory_source"
            elif path == OLD_METHODS:
                role = "historical_method_inventory_input"
            else:
                role = "historical_measurement_or_workload_source_not_new_phase14c_data"
            checksums.append({"path": str(path.relative_to(ROOT)), "sha256": sha256(path), "role": role})
    write_csv(OUT / "source_references.csv", ["path", "sha256", "role"], checksums)
    (OUT / "source_checksums.txt").write_text(
        "".join(f"{item['sha256']}  {item['path']}  [{item['role']}]\n" for item in checksums),
        encoding="utf-8",
    )
    provenance = {
        "schema_version": "phase14c-workload-intake-v1",
        "status": "candidate workload set verified; full Phase 14C study not run",
        "source_run_id": "phase14-main-20261112",
        "source_manifest_sha256": sha256(OLD_WORKLOAD),
        "source_input_sidecar_sha256": sha256(OLD_INPUTS),
        "copied_manifest_sha256": sha256(workload_copy),
        "copied_input_sidecar_sha256": sha256(input_copy),
        "problem_count": len(base),
        "module_counts": dict(by_module),
        "methods_in_inventory": len(inventory),
        "module_method_counts": dict(counts),
        "method_problem_applicability_rows": len(applicability),
        "source_is_historical": True,
        "phase14c_pilot_measurements_exist": True,
        "full_study_measurement_rows": 0,
        "limitations": [
            "Phase 14 inputs are a candidate frozen workload base, not new Phase 14C measurements.",
            "Piecewise derivative and integral adapters need supplemental piecewise fixtures.",
            "ODE Phase 14C accuracy needs full-trajectory references at every requested output point.",
            "Every non-root method still requires exact API dispatch verification and a pilot before the final run.",
        ],
    }
    (OUT / "workload_intake_verification.json").write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"method_rows": len(inventory), "counts": dict(counts), "candidate_problems": len(base), "applicability_rows": len(applicability), "full_study_measurement_rows": 0, "phase14c_pilot_measurements_exist": True}, indent=2))


if __name__ == "__main__":
    main()
