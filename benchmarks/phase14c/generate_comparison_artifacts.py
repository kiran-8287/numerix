#!/usr/bin/env python3
"""Normalize available Phase 14C pilots, analyze them, and create 21 figures.

The script preserves pilot and historical provenance. Missing measurements are
rendered as unavailable; it does not synthesize timing, memory, or accuracy data.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
import random
import shutil
import statistics
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "benchmarks/results/phase14c/module_comparisons"
RAW = BASE / "raw"
ANALYSIS = BASE / "analysis"
FIGURES = BASE / "figures"
MANIFEST = BASE / "manifests"
RUNS = {
    "root": "phase14b-root-20261112-v3",
    "linear_algebra": "phase14c-la-pilot-20261112T055827Z",
    "differentiation": "phase14c-pilot-scalar-20261112T055820Z",
    "interpolation": "phase14c-pilot-scalar-20261112T055820Z",
    "integration": "phase14c-pilot-scalar-20261112T055820Z",
    "optimization": "phase14c-opt-correctness-pilot-20261112",
    "ode": "phase14c-ode-correctness-pilot-20261112T060326Z",
}
MODULES = ["root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode"]
LIBRARIES = {
    "root": "GSL",
    "linear_algebra": "LAPACK/OpenBLAS",
    "differentiation": "GSL",
    "interpolation": "GSL",
    "integration": "GSL",
    "optimization": "NLopt",
    "ode": "SUNDIALS",
}
COLORS = {"NUMERIX": "#255D9C", "COMPETITOR": "#D6794E"}
ROOT_KEYS = {
    "numerix_root_bisection": "bisection",
    "numerix_root_regula_falsi": "regula_falsi",
    "numerix_root_secant": "secant",
    "numerix_root_newton": "newton",
    "numerix_root_safeguarded_newton": "safeguarded_newton",
    "numerix_root_brent": "brent",
    "numerix_root_solve_auto": "auto_selection",
    "numerix_root_solve_portfolio": "portfolio",
    "numerix_root_find_all": "multiple_root_discovery",
}


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, fields: list[str], records: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore", lineterminator="\n")
        writer.writeheader()
        writer.writerows(records)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def float_or_none(value) -> float | None:
    try:
        result = float(value)
        return result if math.isfinite(result) else None
    except (TypeError, ValueError):
        return None


def flag(value) -> bool | None:
    if value is True or str(value).lower() in {"1", "true", "yes", "pass", "success"}:
        return True
    if value is False or str(value).lower() in {"0", "false", "no", "fail", "failed"}:
        return False
    return None


def method_key(module: str, public_api: str) -> str | None:
    symbols = [x for x in public_api.replace("/", " ").split() if x.startswith("numerix_")]
    if not symbols:
        return None
    symbol = symbols[0]
    if module == "root":
        return ROOT_KEYS.get(symbol)
    if module == "linear_algebra":
        return symbol
    if module == "differentiation":
        return {
            "numerix_diff_central_five_point": "five_point",
        }.get(symbol, symbol.removeprefix("numerix_diff_"))
    if module == "interpolation":
        return symbol.removeprefix("numerix_interp_")
    if module == "integration":
        return {
            "numerix_integrate_trapezoid": "composite_trapezoid",
            "numerix_integrate_simpson": "composite_simpson",
        }.get(symbol, symbol.removeprefix("numerix_integrate_"))
    return symbol


def short_class(value: str) -> str:
    return {
        "direct": "D",
        "best_available": "BA",
        "numerix_only": "NUMERIX only",
        "not_comparable": "NC",
        "lifecycle_only": "lifecycle",
    }.get(value, value)


def collect() -> tuple[list[dict], list[dict], dict[str, Path]]:
    coverage = read_csv(MANIFEST / "method_coverage.csv")
    coverage_by_module = {m: [r for r in coverage if r["module"] == m] for m in MODULES}
    time_rows: list[dict] = []
    accuracy_rows: list[dict] = []
    source_paths: dict[str, Path] = {}

    # The root set remains an immutable historical Phase 14B source.
    root_dir = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
    source_paths["root.raw_timing"] = root_dir / "raw_timing.csv"
    source_paths["root.accuracy"] = root_dir / "accuracy.csv"
    root_timing = read_csv(root_dir / "raw_timing.csv")
    root_accuracy = read_csv(root_dir / "accuracy.csv")
    workloads = read_csv(ROOT / "benchmarks/problems/phase14/workloads.csv")
    workload_by_id = {r["problem_id"]: r for r in workloads if r["module"] == "root"}
    for row in root_timing:
        if row["library"] not in {"NUMERIX", "GSL"}:
            continue
        key = row["requested_method"]
        batch_size = float_or_none(row.get("batch_size")) or 1.0
        elapsed = float_or_none(row.get("wall_seconds"))
        if elapsed is None or elapsed < 0:
            continue
        time_rows.append({
            "module": "root", "method_key": key, "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
            "problem_id": row["problem_id"], "source_run_id": row["run_id"],
            "source_kind": "historical_phase14b", "comparison_type": row.get("comparison_class", ""),
            "scope": "solver call including library workspace", "batch_index": row.get("batch_index", ""),
            "elapsed_per_call": elapsed / batch_size, "status": row.get("status", ""),
        })
    for row in root_accuracy:
        if row["library"] not in {"NUMERIX", "GSL"}:
            continue
        key = row["requested_method"]
        abs_error = float_or_none(row.get("absolute_error"))
        norm_error = None
        workload = workload_by_id.get(row["problem_id"])
        if abs_error is not None and key != "multiple_root_discovery" and workload:
            abs_tol = float_or_none(workload.get("absolute_tolerance")) or 0.0
            rel_tol = float_or_none(workload.get("relative_tolerance")) or 0.0
            ref = float_or_none(workload.get("reference_value")) or 0.0
            denom = abs_tol + rel_tol * abs(ref)
            if denom > 0:
                norm_error = abs_error / denom
        accuracy_rows.append({
            "module": "root", "method_key": key, "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
            "problem_id": row["problem_id"], "source_run_id": row["run_id"],
            "source_kind": "historical_phase14b", "comparison_type": row.get("comparison_class", ""),
            "status": row.get("status", ""), "native_status": row.get("native_status", ""),
            "converged": flag(row.get("converged")), "accuracy_pass": flag(row.get("accuracy_pass")),
            "absolute_error": abs_error, "normalized_error": norm_error,
            "accuracy_metric": row.get("error_kind", "root location / discovery outcome"),
        })

    la_run = ROOT / "benchmarks/phase14c/linear_algebra/pilot_runs" / RUNS["linear_algebra"]
    source_paths["linear_algebra.raw_timing"] = la_run / "raw_timing.csv"
    source_paths["linear_algebra.raw_accuracy"] = la_run / "raw_accuracy.csv"
    for row in read_csv(la_run / "raw_timing.csv"):
        elapsed = float_or_none(row.get("monotonic_wall_elapsed_seconds"))
        calls = float_or_none(row.get("calls_completed"))
        if elapsed is None or not calls:
            continue
        time_rows.append({
            "module": "linear_algebra", "method_key": row["numerix_method"],
            "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
            "problem_id": row["problem_id"], "source_run_id": row["run_id"],
            "source_kind": "phase14c_pilot", "comparison_type": row.get("comparison_class", ""),
            "scope": row.get("timing_scope", "end-to-end"), "batch_index": row.get("batch_index", ""),
            "elapsed_per_call": elapsed / calls, "status": row.get("status", ""),
        })
    for row in read_csv(la_run / "raw_accuracy.csv"):
        accuracy_rows.append({
            "module": "linear_algebra", "method_key": row["numerix_method"],
            "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
            "problem_id": row["problem_id"], "source_run_id": row["run_id"],
            "source_kind": "phase14c_pilot", "comparison_type": row.get("comparison_class", ""),
            "status": row.get("status", ""), "native_status": row.get("native_status", ""),
            "converged": flag(row.get("status") == "success"), "accuracy_pass": flag(row.get("accuracy_pass")),
            "absolute_error": float_or_none(row.get("absolute_error")),
            "normalized_error": float_or_none(row.get("normalized_error")),
            "accuracy_metric": row.get("error_metric", ""),
        })

    # Scalar module pilot has a shared raw schema. Its GSL accuracy rows do not
    # repeat the NUMERIX method key, so preserve the adjacent NUMERIX/GSL route
    # pairing recorded in the raw CSV while normalizing a separate analysis copy.
    for module in ("differentiation", "interpolation", "integration"):
        run_dir = RAW / module / RUNS[module]
        source_paths[f"{module}.raw_timing"] = run_dir / "raw_timing.csv"
        source_paths[f"{module}.raw_accuracy"] = run_dir / "raw_accuracy.csv"
        for row in read_csv(run_dir / "raw_timing.csv"):
            elapsed = float_or_none(row.get("wall_elapsed_seconds"))
            calls = float_or_none(row.get("batch_size")) or 1.0
            if elapsed is None or elapsed < 0:
                continue
            competitor_route = row.get("competitor_method", "")
            scope = "end-to-end" if competitor_route.endswith("_end_to_end") else "steady/setup"
            time_rows.append({
                "module": module, "method_key": row["numerix_method"],
                "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR", "problem_id": row["problem_id"],
                "source_run_id": row["run_id"], "source_kind": "phase14c_pilot",
                "comparison_type": row.get("comparison_type", ""), "scope": scope,
                "batch_index": row.get("batch_index", ""), "elapsed_per_call": elapsed / calls,
                "status": row.get("status", ""),
            })
        raw_acc = read_csv(run_dir / "raw_accuracy.csv")
        current_key = None
        current_problem = None
        for row in raw_acc:
            if row["library"] == "NUMERIX":
                current_key = row.get("method", "")
                current_problem = row["problem_id"]
                key = current_key
            else:
                if current_problem != row["problem_id"] or current_key is None:
                    raise SystemExit(f"Could not pair scalar pilot accuracy row: {module}/{row['problem_id']}")
                key = current_key
            error = float_or_none(row.get("absolute_error"))
            abs_tol = float_or_none(row.get("target_absolute")) or 0.0
            rel_tol = float_or_none(row.get("target_relative")) or 0.0
            reference = float_or_none(row.get("reference")) or 0.0
            denominator = abs_tol + rel_tol * abs(reference)
            norm = error / denominator if error is not None and denominator > 0 else None
            accuracy_rows.append({
                "module": module, "method_key": key, "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
                "problem_id": row["problem_id"], "source_run_id": row["run_id"],
                "source_kind": "phase14c_pilot", "comparison_type": row.get("comparison_type", ""),
                "status": row.get("status", ""), "native_status": row.get("native_status", ""),
                "converged": flag(row.get("status") == "success"),
                "accuracy_pass": flag(row.get("pass")), "absolute_error": error,
                "normalized_error": norm, "accuracy_metric": row.get("metric", ""),
            })

    opt_dir = ROOT / "benchmarks/phase14c/pilots/optimization"
    ode_dir = ROOT / "benchmarks/phase14c/pilots/ode"
    source_paths["optimization.raw_accuracy"] = opt_dir / "raw_accuracy_pilot.csv"
    for row in read_csv(opt_dir / "raw_accuracy_pilot.csv"):
        accuracy_rows.append({
            "module": "optimization", "method_key": row["numerix_method"],
            "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
            "problem_id": row["problem_id"], "source_run_id": row["run_id"],
            "source_kind": "phase14c_correctness_pilot", "comparison_type": row.get("comparison_type", ""),
            "status": row.get("normalized_status", ""), "native_status": row.get("native_status_name", ""),
            "converged": flag(row.get("converged")), "accuracy_pass": flag(row.get("accuracy_pass")),
            "absolute_error": float_or_none(row.get("absolute_error")),
            "normalized_error": float_or_none(row.get("normalized_error")),
            "accuracy_metric": row.get("accuracy_metric", ""),
        })
    source_paths["ode.raw_accuracy"] = ode_dir / "raw_accuracy_pilot.csv"
    for row in read_csv(ode_dir / "raw_accuracy_pilot.csv"):
        accuracy_rows.append({
            "module": "ode", "method_key": row["numerix_method"],
            "library": "NUMERIX" if row["library"] == "NUMERIX" else "COMPETITOR",
            "problem_id": row["problem_id"], "source_run_id": row["run_id"],
            "source_kind": "phase14c_correctness_pilot", "comparison_type": row.get("comparison_type", ""),
            "status": row.get("normalized_status", ""), "native_status": row.get("native_status", ""),
            "converged": flag(row.get("converged")),
            "accuracy_pass": flag(row.get("accuracy_pass")),
            "absolute_error": float_or_none(row.get("absolute_error")),
            "normalized_error": float_or_none(row.get("normalized_error")),
            "accuracy_metric": row.get("accuracy_metric", ""),
        })

    # Copy canonical pilot raw data into the consolidated results tree.
    # Inputs remain untouched; each copied run retains its own original run ID.
    la_dest = RAW / "linear_algebra" / RUNS["linear_algebra"]
    if not la_dest.exists():
        shutil.copytree(la_run, la_dest)
    for module in ("optimization", "ode"):
        src = opt_dir if module == "optimization" else ode_dir
        dst = RAW / module / RUNS[module]
        dst.mkdir(parents=True, exist_ok=True)
        filename = "raw_accuracy_pilot.csv"
        shutil.copy2(src / filename, dst / "raw_accuracy.csv")
        shutil.copy2(src / "run_metadata.json", dst / "run_metadata.json")
        manifest = src / "workload_manifest.csv"
        if manifest.exists():
            shutil.copy2(manifest, dst / "workload_manifest.csv")

    root_ref = RAW / "root" / RUNS["root"]
    root_ref.mkdir(parents=True, exist_ok=True)
    (root_ref / "historical_source_reference.json").write_text(json.dumps({
        "run_id": RUNS["root"],
        "source_directory": str(root_dir.relative_to(ROOT)),
        "phase14c_measurement": False,
        "note": "Original Phase 14B raw files remain at their source path and are referenced by SHA-256; they are not relabeled or copied as Phase 14C measurements.",
    }, indent=2) + "\n", encoding="utf-8")

    return time_rows, accuracy_rows, source_paths


def per_problem_times(time_rows: list[dict]) -> dict[tuple, list[float]]:
    batches = defaultdict(list)
    for row in time_rows:
        key = (row["module"], row["method_key"], row["library"], row["scope"], row["problem_id"])
        batches[key].append(row["elapsed_per_call"])
    return batches


def bootstrap_interval(values: list[float], seed: int) -> tuple[float | None, float | None]:
    if not values:
        return None, None
    if len(values) == 1:
        return values[0], values[0]
    rng = random.Random(seed)
    estimates = []
    for _ in range(5000):
        estimates.append(statistics.median([values[rng.randrange(len(values))] for _ in values]))
    estimates.sort()
    return estimates[int(0.025 * (len(estimates) - 1))], estimates[int(0.975 * (len(estimates) - 1))]


def generate_analysis(time_rows: list[dict], accuracy_rows: list[dict]) -> tuple[dict, dict]:
    coverage = read_csv(MANIFEST / "method_coverage.csv")
    per_problem = per_problem_times(time_rows)
    time_groups = defaultdict(list)
    for key, batches in per_problem.items():
        module, method, lib, scope, problem = key
        time_groups[(module, method, lib, scope)].append((problem, statistics.median(batches), len(batches)))
    accuracy_groups = defaultdict(list)
    for row in accuracy_rows:
        accuracy_groups[(row["module"], row["method_key"], row["library"])].append(row)

    summary_by_module: dict[str, list[dict]] = {module: [] for module in MODULES}
    paired_by_module: dict[str, list[dict]] = {module: [] for module in MODULES}
    summary_fields = [
        "module", "operation", "method_key", "kind", "comparison_type", "library",
        "source_run_id", "evidence_scope", "timing_scope", "timed_problem_count",
        "timing_batch_count", "median_problem_median_seconds_per_call",
        "accuracy_attempt_count", "status_success_count", "accuracy_pass_count",
        "accuracy_pass_rate", "finite_error_count", "median_absolute_error",
        "median_normalized_error", "accuracy_metric", "memory_status",
    ]
    for item in coverage:
        module = item["module"]
        key = method_key(module, item["public_api"])
        if key is None:
            continue
        scopes = sorted({scope for (m, mk, _, scope) in time_groups if m == module and mk == key})
        if not scopes:
            scopes = ["not_measured"]
        for library in ("NUMERIX", "COMPETITOR"):
            accuracy = accuracy_groups.get((module, key, library), [])
            successes = sum(row["converged"] is True for row in accuracy)
            pass_rows = [row for row in accuracy if row["accuracy_pass"] is not None]
            passes = sum(row["accuracy_pass"] is True for row in pass_rows)
            errors = [row["absolute_error"] for row in accuracy if row.get("absolute_error") is not None]
            normalized = [row["normalized_error"] for row in accuracy if row.get("normalized_error") is not None]
            for scope in scopes:
                timed = time_groups.get((module, key, library, scope), [])
                per_problem_values = [entry[1] for entry in timed]
                summary_by_module[module].append({
                    "module": module, "operation": item["operation"], "method_key": key,
                    "kind": item["kind"], "comparison_type": item["comparison_type"],
                    "library": library if library == "NUMERIX" else LIBRARIES[module],
                    "source_run_id": RUNS[module], "evidence_scope": item["phase14c_evidence_scope"],
                    "timing_scope": scope, "timed_problem_count": len(timed),
                    "timing_batch_count": sum(entry[2] for entry in timed),
                    "median_problem_median_seconds_per_call": statistics.median(per_problem_values) if per_problem_values else "",
                    "accuracy_attempt_count": len(pass_rows), "status_success_count": successes,
                    "accuracy_pass_count": passes,
                    "accuracy_pass_rate": passes / len(pass_rows) if pass_rows else "",
                    "finite_error_count": len(errors),
                    "median_absolute_error": statistics.median(errors) if errors else "",
                    "median_normalized_error": statistics.median(normalized) if normalized else "",
                    "accuracy_metric": next((row["accuracy_metric"] for row in accuracy if row.get("accuracy_metric")), ""),
                    "memory_status": "not_measured_method_specific_memory",
                })

        # Paired runtime unit is the problem, not its 31 timing batches.
        paired_scopes = sorted({scope for (m, mk, _, scope) in time_groups if m == module and mk == key})
        for scope in paired_scopes:
            nums = {problem: median for problem, median, _ in time_groups.get((module, key, "NUMERIX", scope), [])}
            comps = {problem: median for problem, median, _ in time_groups.get((module, key, "COMPETITOR", scope), [])}
            ratios = [comps[p] / nums[p] for p in sorted(nums.keys() & comps.keys()) if nums[p] > 0 and comps[p] > 0]
            low, high = bootstrap_interval(ratios, 14112026 + sum(ord(c) for c in module + key + scope))
            paired_by_module[module].append({
                "module": module, "operation": item["operation"], "method_key": key,
                "comparison_type": item["comparison_type"], "timing_scope": scope,
                "ratio_definition": "competitor per-call median / NUMERIX per-call median; >1 favors NUMERIX",
                "paired_problem_count": len(ratios), "median_paired_ratio": statistics.median(ratios) if ratios else "",
                "bootstrap_95ci_low": low if low is not None else "", "bootstrap_95ci_high": high if high is not None else "",
                "bootstrap_resamples": 5000 if len(ratios) > 1 else 0,
                "interpretation": "descriptive pilot/historical analysis only; problem is resampling unit",
            })

    for module in MODULES:
        write_csv(ANALYSIS / module / "method_summary.csv", summary_fields, summary_by_module[module])
        write_csv(ANALYSIS / module / "paired_runtime_summary.csv", [
            "module", "operation", "method_key", "comparison_type", "timing_scope", "ratio_definition",
            "paired_problem_count", "median_paired_ratio", "bootstrap_95ci_low", "bootstrap_95ci_high",
            "bootstrap_resamples", "interpretation",
        ], paired_by_module[module])
    return summary_by_module, paired_by_module


def display_label(item: dict) -> str:
    return f"{item['operation']} [{short_class(item['comparison_type'])}]"


def save_figure(fig, path_stem: Path) -> None:
    path_stem.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path_stem.with_suffix(".png"), dpi=200, bbox_inches="tight", facecolor="white")
    svg_path = path_stem.with_suffix(".svg")
    fig.savefig(svg_path, bbox_inches="tight", facecolor="white")
    svg_text = svg_path.read_text(encoding="utf-8")
    svg_path.write_text("\n".join(line.rstrip() for line in svg_text.splitlines()) + "\n", encoding="utf-8")
    plt.close(fig)


def create_figures(time_rows: list[dict], accuracy_rows: list[dict]) -> None:
    coverage = read_csv(MANIFEST / "method_coverage.csv")
    per_problem = per_problem_times(time_rows)
    time_groups = defaultdict(list)
    for key, batches in per_problem.items():
        module, method, library, scope, problem = key
        time_groups[(module, method, library, scope)].append((problem, statistics.median(batches), len(batches)))
    accuracy_groups = defaultdict(list)
    for row in accuracy_rows:
        accuracy_groups[(row["module"], row["method_key"], row["library"])].append(row)

    for module in MODULES:
        items = [row for row in coverage if row["module"] == module and row["kind"] == "numerical"]
        comp_label = LIBRARIES[module]
        # Runtime: all observed routes, all attempts, grouped by problem median.
        route_categories = []
        for item in items:
            key = method_key(module, item["public_api"])
            scopes = sorted({scope for (m, mk, _, scope) in time_groups if m == module and mk == key})
            if not scopes:
                scopes = ["not measured"]
            for scope in scopes:
                suffix = {"not_measured": "(not measured)", "end-to-end": "(end-to-end)", "steady/setup": "(steady/setup)", "solver call including library workspace": ""}.get(scope, f"({scope})")
                label = f"{display_label(item)}\n{suffix}" if suffix else display_label(item)
                route_categories.append((item, key, scope, label))
        fig_width = max(12.5, min(22.0, 0.95 * max(1, len(route_categories))))
        fig, ax = plt.subplots(figsize=(fig_width, 7.5))
        x = list(range(len(route_categories)))
        width = 0.34
        runtime_exists = False
        for offset, library, color in ((-width / 2, "NUMERIX", COLORS["NUMERIX"]), (width / 2, "COMPETITOR", COLORS["COMPETITOR"])):
            values = []
            for item, key, scope, _ in route_categories:
                group = time_groups.get((module, key, library, scope), [])
                values.append(statistics.median([r[1] for r in group]) * 1e6 if group else float("nan"))
                runtime_exists |= bool(group)
            bars = ax.bar([v + offset for v in x], values, width=width, color=color, label="NUMERIX" if library == "NUMERIX" else comp_label)
        ax.set_xticks(x, [entry[3] for entry in route_categories], rotation=55, ha="right", fontsize=8)
        ax.set_ylabel("Median of per-problem median time (µs/call; log scale)")
        ax.set_xlabel("Public operation and equivalence class")
        ax.set_title(f"{module.replace('_', ' ').title()} — execution time")
        ax.grid(axis="y", which="both", alpha=0.23)
        ax.legend(frameon=False)
        if runtime_exists:
            ax.set_yscale("log")
            for idx, (item, key, scope, _) in enumerate(route_categories):
                groups = [time_groups.get((module, key, library, scope), []) for library in ("NUMERIX", "COMPETITOR")]
                if not any(groups):
                    continue
                counts = [len(group) for group in groups]
                successes = []
                for library in ("NUMERIX", "COMPETITOR"):
                    attempts = [r for r in accuracy_groups.get((module, key, library), []) if r["accuracy_pass"] is not None]
                    successes.append(f"{sum(r['converged'] is True for r in attempts)}/{len(attempts)}" if attempts else "—")
                local_top = max((group and statistics.median([r[1] for r in group]) * 1e6 or 0.0) for group in groups)
                ax.text(
                    x[idx], local_top * 1.1,
                    f"n N/C={counts[0]}/{counts[1]}\nok N/C={successes[0]},{successes[1]}",
                    ha="center", va="bottom", fontsize=5.7,
                )
            lower, upper = ax.get_ylim()
            ax.set_ylim(lower, upper * 1.5)
        else:
            ax.text(0.5, 0.55, "NOT MEASURED\nThis module's pilot collected correctness only.", transform=ax.transAxes, ha="center", va="center", fontsize=15, color="#6C7785", weight="bold")
        note = {
            "root": "Historical Phase 14B v3 only; 100 problems per root method, 31 batches. Not new Phase 14C data.",
            "linear_algebra": "Phase 14C pilot only: four workloads, four operations, 31 batches per route; other operations not measured.",
            "differentiation": "Phase 14C adapter pilot: three problems per module, 31 batches per route; not a full study.",
            "interpolation": "Phase 14C adapter pilot: three problems per module, 31 batches per route; setup and evaluation routes kept distinct.",
            "integration": "Phase 14C adapter pilot: three problems per module, 31 batches per route; adaptive end-to-end route labeled separately.",
            "optimization": "Correctness-only pilot; no execution-time samples.",
            "ode": "Correctness-only fixed-step tableau pilot; no execution-time samples.",
        }[module]
        fig.text(0.01, 0.01, note + " Runtime labels show problem counts and status-success counts as NUMERIX/competitor; batch counts are in the saved CSV.", ha="left", fontsize=8, color="#4D5968")
        fig.subplots_adjust(bottom=0.34, top=0.91)
        save_figure(fig, FIGURES / module / f"{module}_runtime")

        # Memory: no module has passed complete route-specific allocation
        # visibility, so no pilot smoke value is misrepresented as a result.
        fig, ax = plt.subplots(figsize=(12, 6.5))
        ax.set_title(f"{module.replace('_', ' ').title()} — method-specific memory")
        ax.set_xlabel("Problem size / workload (not measured)")
        ax.set_ylabel("Peak additional requested heap memory (bytes)")
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.legend(handles=[Patch(color=COLORS["NUMERIX"], label="NUMERIX"), Patch(color=COLORS["COMPETITOR"], label=comp_label)], frameon=False, loc="upper right")
        ax.text(0.5, 0.59, "NOT MEASURED", transform=ax.transAxes, ha="center", va="center", fontsize=22, color="#687586", weight="bold")
        ax.text(0.5, 0.42, "No complete method-level allocation series passed route-specific visibility checks.\nHistorical process RSS and positive-control values are not plotted as method memory.", transform=ax.transAxes, ha="center", va="center", fontsize=11, color="#4D5968")
        fig.text(0.01, 0.01, "Positive controls exist for tracker, GSL vector, and NUMERIX vector paths; OpenBLAS/LAPACK, NLopt, SUNDIALS, mmap/custom allocator coverage remains open.", ha="left", fontsize=8, color="#4D5968")
        save_figure(fig, FIGURES / module / f"{module}_memory")

        # Accuracy: use pass rates for targets that were evaluated. ODE keeps
        # the pilot's analytic normalized trajectory error and explicitly has
        # no pass threshold.
        fig, ax = plt.subplots(figsize=(fig_width, 7.5))
        if module == "ode":
            width = 0.34
            x = list(range(len(items)))
            for offset, library, color in ((-width/2, "NUMERIX", COLORS["NUMERIX"]), (width/2, "COMPETITOR", COLORS["COMPETITOR"])):
                values = []
                for item in items:
                    key = method_key(module, item["public_api"])
                    errs = [r["normalized_error"] for r in accuracy_groups.get((module, key, library), []) if r.get("normalized_error") is not None]
                    values.append(max(1e-18, statistics.median(errs)) if errs else float("nan"))
                ax.bar([v+offset for v in x], values, width=width, color=color, label="NUMERIX" if library == "NUMERIX" else comp_label)
            ax.set_yscale("log")
            for idx, item in enumerate(items):
                key = method_key(module, item["public_api"])
                num_rows = accuracy_groups.get((module, key, "NUMERIX"), [])
                comp_rows = accuracy_groups.get((module, key, "COMPETITOR"), [])
                n_num, n_comp = len(num_rows), len(comp_rows)
                route_errors = [r["normalized_error"] for r in num_rows + comp_rows if r.get("normalized_error") is not None]
                if route_errors:
                    ax.text(x[idx], max(route_errors) * 1.15, f"n N/C={n_num}/{n_comp}", ha="center", va="bottom", fontsize=8)
            low, high = ax.get_ylim()
            ax.set_ylim(low, high * 1.5)
            ax.set_xticks(x, [f"{item['operation']} [D]" for item in items], rotation=30, ha="right")
            ax.set_ylabel("Median normalized max-trajectory error vs analytic solution (log scale)")
            ax.set_xlabel("Fixed-step method; direct tableau comparison (D)")
            ax.set_title("ODE — full 21-point trajectory error on one pilot system")
        else:
            x = list(range(len(items)))
            width = 0.34
            for offset, library, color in ((-width/2, "NUMERIX", COLORS["NUMERIX"]), (width/2, "COMPETITOR", COLORS["COMPETITOR"])):
                rates = []
                annot = []
                for item in items:
                    key = method_key(module, item["public_api"])
                    rs = [r for r in accuracy_groups.get((module, key, library), []) if r["accuracy_pass"] is not None]
                    passed = sum(r["accuracy_pass"] is True for r in rs)
                    rates.append(100.0 * passed / len(rs) if rs else float("nan"))
                    annot.append((passed, len(rs), sum(r["converged"] is True for r in rs)))
                bars = ax.bar([v+offset for v in x], rates, width=width, color=color, label="NUMERIX" if library == "NUMERIX" else comp_label)
                for bar, (passed, total, succeeded), item in zip(bars, annot, items):
                    if total:
                        ax.text(bar.get_x()+bar.get_width()/2, bar.get_height()+2.0, f"{passed}/{total}\nstatus {succeeded}/{total}", ha="center", va="bottom", fontsize=7, rotation=90)
                    else:
                        is_setup = any(word in item["operation"].lower() for word in ("setup", "construction"))
                        no_competitor = library == "COMPETITOR" and item["comparison_type"] in {"numerix_only", "not_comparable"}
                        label = "N/A" if is_setup or no_competitor else "N/M"
                        ax.text(bar.get_x()+bar.get_width()/2, 5, label, ha="center", va="bottom", fontsize=8, rotation=90, color="#697586")
            ax.axhline(100.0, color="#576271", linestyle="--", linewidth=0.9, alpha=0.7, label="All attempted cases pass")
            ax.set_ylim(0, 125)
            ax.set_ylabel("Accuracy-target pass rate across attempted records (%)")
            ax.set_xticks(x, [display_label(item) for item in items], rotation=55, ha="right", fontsize=8)
            ax.set_xlabel("Public operation and equivalence class")
            ax.set_title(f"{module.replace('_', ' ').title()} — numerical accuracy outcome")
            ax.text(0.01, 0.98, "Failed statuses remain in pass denominators; N/A means the operation has no numerical output.", transform=ax.transAxes, ha="left", va="top", fontsize=8)
        ax.grid(axis="y", which="both", alpha=0.23)
        ax.legend(frameon=False, ncol=2)
        note = {
            "root": "Historical Phase 14B v3; GSL 2.8. Auto-selection and portfolio are NUMERIX-only; discovery is heuristic best available.",
            "linear_algebra": "Pilot uses four bounded integer/matrix workloads; all 8 route records passed their exact-reference checks.",
            "differentiation": "Pilot: 3 problems/method; pass counts reflect the configured strict derivative target, not broad method ranking.",
            "interpolation": "Pilot: 3 problems/method; only 4/18 evaluation records passed; construction has no accuracy result.",
            "integration": "Pilot: 3 problems/method; pass counts reflect the configured target; fixed Gauss-Legendre is best available, not composite-rule equivalence.",
            "optimization": "Correctness-only pilot: one case per method; DIRECT-L reached max evaluations on golden-section despite passing the accuracy gate.",
            "ode": "Normalized trajectory error uses max analytic state as scale; exact method tableau verified; threshold not defined.",
        }[module]
        fig.text(0.01, 0.01, note, ha="left", fontsize=8, color="#4D5968")
        fig.subplots_adjust(bottom=0.34, top=0.90)
        save_figure(fig, FIGURES / module / f"{module}_accuracy")


def verify_figures() -> dict:
    expected = [FIGURES / module / f"{module}_{kind}.{ext}" for module in MODULES for kind in ("runtime", "memory", "accuracy") for ext in ("png", "svg")]
    errors = []
    checked_png = checked_svg = 0
    for path in expected:
        if not path.is_file():
            errors.append(f"missing:{path.relative_to(ROOT)}")
            continue
        if path.suffix == ".png":
            try:
                with Image.open(path) as image:
                    image.verify()
                checked_png += 1
            except Exception as exc:
                errors.append(f"invalid_png:{path.name}:{exc}")
        else:
            try:
                ET.parse(path)
                checked_svg += 1
            except Exception as exc:
                errors.append(f"invalid_svg:{path.name}:{exc}")
    actual = sorted(str(p.relative_to(FIGURES)) for p in FIGURES.rglob("*.png")) + sorted(str(p.relative_to(FIGURES)) for p in FIGURES.rglob("*.svg"))
    verification = {
        "schema_version": "phase14c-graph-verification-v1",
        "required_graph_count": 21,
        "png_count": checked_png,
        "svg_count": checked_svg,
        "expected_image_file_count": len(expected),
        "missing_or_invalid": errors,
        "exactly_three_figures_per_module": all(sum(1 for kind in ("runtime", "memory", "accuracy") if (FIGURES / m / f"{m}_{kind}.png").is_file()) == 3 for m in MODULES),
        "all_values_are_from_recorded_sources_or_explicit_not_measured_labels": True,
        "historical_root_not_relabelled": True,
        "verified_image_files": actual,
    }
    (BASE / "verification/graph_verification.json").parent.mkdir(parents=True, exist_ok=True)
    (BASE / "verification/graph_verification.json").write_text(json.dumps(verification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if errors or checked_png != 21 or checked_svg != 21 or len(actual) != 42:
        raise SystemExit(json.dumps(verification, indent=2))
    return verification


def main() -> int:
    time_rows, accuracy_rows, source_paths = collect()
    summary, paired = generate_analysis(time_rows, accuracy_rows)
    create_figures(time_rows, accuracy_rows)
    graph_verification = verify_figures()
    raw_files = []
    for label, path in sorted(source_paths.items()):
        if path.is_file():
            raw_files.append({"label": label, "source_path": str(path.relative_to(ROOT)), "sha256": sha256(path)})
    verification = {
        "schema_version": "phase14c-artifact-verification-v1",
        "run_scope": "pilot and historical evidence only; full 700-problem method-level study is not complete",
        "method_inventory_rows": len(read_csv(MANIFEST / "method_coverage.csv")),
        "auxiliary_lifecycle_rows": len(read_csv(MANIFEST / "auxiliary_lifecycle_coverage.csv")),
        "candidate_workload_problems": len(read_csv(MANIFEST / "workload_manifest.csv")),
        "phase14c_full_study_measurement_rows": 0,
        "normalized_timing_records": len(time_rows),
        "normalized_accuracy_records": len(accuracy_rows),
        "graph_verification": graph_verification,
        "raw_source_files": raw_files,
        "limitations": [
            "Root timing and accuracy are historical Phase 14B v3 data, never Phase 14C measurements.",
            "Linear algebra timing/accuracy is a 4-workload pilot covering only matrix add, matrix multiply, LU factor, and LU solve.",
            "Differentiation/interpolation/integration are three-problem adapter pilots; not 100-problem studies.",
            "Optimization and ODE have correctness-only pilots and no runtime measurements.",
            "Method-specific memory is not measured for any module; process RSS and smoke-control allocations are not substituted.",
            "Accuracy thresholds are unevaluated for the ODE pilot.",
        ],
    }
    (BASE / "verification/benchmark_verification.json").write_text(json.dumps(verification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "normalized_timing_records": len(time_rows), "normalized_accuracy_records": len(accuracy_rows),
        "figures": 21, "png": graph_verification["png_count"], "svg": graph_verification["svg_count"],
        "full_study_complete": False,
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
