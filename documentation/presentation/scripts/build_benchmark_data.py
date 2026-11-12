#!/usr/bin/env python3
"""Build local presentation chart data from the preserved Phase 14B root outputs.

This script only writes documentation/presentation/data. It never modifies the
original benchmark runs or analysis artifacts.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
RUN_ID = "phase14b-root-20261112-v3"
RAW_DIR = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
ANALYSIS_DIR = ROOT / "benchmarks/results/phase14b/analysis/root"
COVERAGE_FILE = ROOT / "benchmarks/analysis/phase14b/method_coverage.csv"
OUT_DIR = ROOT / "documentation/presentation/data"

MODULES = [
    ("root", "Root Finding"),
    ("linear_algebra", "Linear Algebra"),
    ("differentiation", "Differentiation"),
    ("interpolation", "Interpolation"),
    ("integration", "Integration"),
    ("optimization", "Optimization"),
    ("ode", "Ordinary Differential Equations"),
]


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def number(value: str) -> float | None:
    if value == "":
        return None
    parsed = float(value)
    return parsed if math.isfinite(parsed) else None


def integer(value: str) -> int:
    return int(value) if value else 0


def verify_sources(verification: dict, timing_path: Path,
                   accuracy_path: Path, workload_path: Path) -> None:
    expected = {
        "raw_timing_checksum_sha256": timing_path,
        "accuracy_checksum_sha256": accuracy_path,
        "workload_map_checksum_sha256": workload_path,
    }
    for checksum_key, path in expected.items():
        actual = sha256(path)
        if actual != verification[checksum_key]:
            raise ValueError(f"Source checksum mismatch for {path.relative_to(ROOT)}")


def main() -> int:
    timing_path = RAW_DIR / "raw_timing.csv"
    accuracy_path = RAW_DIR / "accuracy.csv"
    workload_path = RAW_DIR / "method_workloads.csv"
    verification_path = ANALYSIS_DIR / "analysis_verification.json"
    summary_path = ANALYSIS_DIR / "method_summary.csv"
    paired_path = ANALYSIS_DIR / "paired_comparisons.csv"

    verification = json.loads(verification_path.read_text(encoding="utf-8"))
    if verification["run_id"] != RUN_ID:
        raise ValueError("Unexpected analysis run ID")
    verify_sources(verification, timing_path, accuracy_path, workload_path)

    timing_rows = read_csv(timing_path)
    accuracy_rows = read_csv(accuracy_path)
    workload_rows = read_csv(workload_path)
    summaries = read_csv(summary_path)
    paired_rows = read_csv(paired_path)
    coverage_rows = read_csv(COVERAGE_FILE)

    if len(timing_rows) != verification["raw_timing_rows"]:
        raise ValueError("Raw timing row count differs from verified analysis")
    if len(accuracy_rows) != verification["accuracy_rows"]:
        raise ValueError("Accuracy row count differs from verified analysis")
    if len(summaries) != verification["observed_method_library_pairs"]:
        raise ValueError("Unexpected number of library/method summary rows")
    if len(paired_rows) != 5:
        raise ValueError("Expected five direct GSL/NUMERIX paired comparisons")

    wall_by_problem: dict[tuple[str, str, str], list[float]] = defaultdict(list)
    timing_counts: dict[tuple[str, str, str], int] = defaultdict(int)
    for row in timing_rows:
        key = (row["library"], row["requested_method"], row["problem_id"])
        batch_size = float(row["batch_size"])
        if batch_size <= 0:
            raise ValueError("Non-positive timing batch size")
        wall_by_problem[key].append(float(row["wall_seconds"]) / batch_size)
        timing_counts[key] += 1

    accuracy_by_route: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in accuracy_rows:
        accuracy_by_route[(row["library"], row["requested_method"])].append(row)

    workload_by_route: dict[tuple[str, str], dict[str, str]] = {}
    for row in workload_rows:
        workload_by_route.setdefault(
            (row["library"], row["method"]), row)

    paired_by_method = {row["method"]: row for row in paired_rows}
    methods = []
    for summary in sorted(summaries, key=lambda r: (r["method"], r["library"])):
        library = summary["library"]
        method = summary["method"]
        route = workload_by_route.get((library, method))
        if route is None:
            raise ValueError(f"Missing workload classification for {library}/{method}")

        route_class = route["comparison_class"]
        comparison_class = route_class
        comparison_detail = route["comparison_detail"]
        if method == "multiple_root_discovery":
            comparison_class = "best_available"
            if library == "NUMERIX":
                comparison_detail = (
                    "NUMERIX heuristic discovery compared with the GSL fixed-grid "
                    "sign-change scan plus Brent refinement."
                )
            elif library == "GSL":
                comparison_detail = (
                    "GSL fixed 4096-panel sign-change scan plus Brent refinement; "
                    "a best-available heuristic baseline, not a native discovery API."
                )
        elif method == "safeguarded_newton" and library in ("NUMERIX", "GSL"):
            # The NUMERIX route is safeguarded Newton, but GSL's route is Brent.
            # Mark the cross-library comparison as best-available on both rows.
            comparison_class = "best_available"
            comparison_detail = (
                "NUMERIX safeguarded Newton compared with the GSL Brent route; "
                "best-available cross-library comparison, not equivalent methods."
                if library == "NUMERIX" else
                "GSL Brent route compared with NUMERIX safeguarded Newton; "
                "best-available cross-library comparison, not equivalent methods."
            )

        problem_points = []
        for accuracy in sorted(
                accuracy_by_route[(library, method)],
                key=lambda r: r["problem_id"]):
            key = (library, method, accuracy["problem_id"])
            samples = wall_by_problem.get(key, [])
            median_seconds = statistics.median(samples) if samples else None
            problem_points.append({
                "problem_id": accuracy["problem_id"],
                "family": accuracy["family"],
                "applicable": accuracy["applicable"] == "1",
                "status_success": accuracy["status"] == "0",
                "accuracy_pass": accuracy["accuracy_pass"] == "1",
                "absolute_error": number(accuracy["absolute_error"]),
                "median_wall_microseconds": (
                    median_seconds * 1_000_000 if median_seconds is not None else None),
                "timing_batches": timing_counts.get(key, 0),
            })

        paired = paired_by_method.get(method)
        # Paired inference is only assigned to the two matching direct routes.
        if paired and library not in ("NUMERIX", "GSL"):
            paired = None
        methods.append({
            "module": "root",
            "module_label": "Root Finding",
            "method": method,
            "library": library,
            "comparison_class": comparison_class,
            "route_comparison_class": route_class,
            "comparison_detail": comparison_detail,
            "attempted_problems": integer(summary["attempted_problems"]),
            "applicable_problems": integer(summary["applicable_problems"]),
            "status_successes": integer(summary["status_successes"]),
            "accuracy_passes": integer(summary["accuracy_passes"]),
            "successful_and_accurate": integer(summary["successful_and_accurate"]),
            "success_rate_all_attempts": number(summary["success_rate_all_attempts"]),
            "accuracy_pass_rate_all_attempts": number(summary["accuracy_pass_rate_all_attempts"]),
            "successful_and_accurate_rate_all_attempts": number(summary["successful_and_accurate_rate_all_attempts"]),
            "median_wall_seconds_per_call": number(summary["median_wall_seconds_per_call"]),
            "median_wall_microseconds_per_call": (
                (number(summary["median_wall_seconds_per_call"]) or 0.0) * 1_000_000),
            "p95_problem_median_wall_seconds_per_call": number(summary["p95_problem_median_wall_seconds_per_call"]),
            "timing_batch_sample_count": integer(summary["timing_batch_sample_count"]),
            "median_absolute_error": number(summary["median_absolute_error"]),
            "paired_comparison": ({
                "competitor": "GSL",
                "baseline": "NUMERIX",
                "ratio_definition": paired["ratio_definition"],
                "eligible_paired_problems": integer(paired["eligible_paired_problems"]),
                "excluded_problems": integer(paired["excluded_problems"]),
                "median_competitor_over_numerix": number(paired["median_paired_speed_ratio"]),
                "bootstrap_95ci": [
                    number(paired["bootstrap_95ci_low"]),
                    number(paired["bootstrap_95ci_high"]),
                ],
                "bootstrap_resamples": integer(paired["bootstrap_resamples"]),
                "inference_note": paired["inference_note"],
            } if paired else None),
            "problem_points": problem_points,
        })

    inventory_counts = defaultdict(int)
    operations_by_module: dict[str, list[dict[str, str]]] = defaultdict(list)
    numerix_root_methods = {
        row["method"] for row in summaries if row["library"] == "NUMERIX"
    }
    root_method_labels = {
        "Bisection": "bisection",
        "Regula Falsi": "regula_falsi",
        "Secant": "secant",
        "Newton-Raphson": "newton",
        "Safeguarded Newton": "safeguarded_newton",
        "Brent": "brent",
        "Automatic method selection": "auto_selection",
        "Solver portfolio": "portfolio",
        "Multiple-root discovery": "multiple_root_discovery",
    }
    for row in coverage_rows:
        module = row["module"]
        inventory_counts[module] += 1
        route_name = root_method_labels.get(row["method"], "")
        measured = module == "root" and route_name in numerix_root_methods
        operations_by_module[module].append({
            "operation": row["method"],
            "public_symbol": row["public_symbol"],
            "implementation_source": row["implementation_source"],
            "phase14_coverage": row["phase14_coverage"],
            "competitor_equivalent": row["competitor_equivalent"],
            "comparison_class": row["comparison_class"],
            "method_level_status": "measured" if measured else "pending",
        })
    if len(operations_by_module["root"]) != len(numerix_root_methods):
        raise ValueError("Root method audit does not match measured NUMERIX routes")

    coverage = []
    for module, label in MODULES:
        measured = module == "root"
        coverage.append({
            "module": module,
            "label": label,
            "audited_method_rows": inventory_counts[module],
            "method_level_status": "measured" if measured else "pending",
            "operations": operations_by_module[module],
            "note": (
                "Phase 14B measured root routes only; this does not represent all root methods."
                if measured else
                "Method-level benchmark comparison is pending; capability inventory is not a measurement."
            ),
        })

    metadata = json.loads((RAW_DIR / "run_metadata.json").read_text(encoding="utf-8"))
    payload = {
        "schema_version": "numerix-presentation-benchmark-v1",
        "meta": {
            "study": "Phase 14B method-level root study",
            "run_id": verification["run_id"],
            "run_date": "2026-11-12",
            "git_head": metadata["git_head"],
            "platform": metadata["platform"],
            "compiler": metadata["compiler"],
            "build_configuration": metadata["build_configuration"],
            "dependencies": metadata["dependency_versions"],
            "accuracy_rows": verification["accuracy_rows"],
            "raw_timing_rows": verification["raw_timing_rows"],
            "library_method_routes": verification["observed_method_library_pairs"],
            "batches_per_problem_route": verification["timing_rows_per_pair_problem"],
            "statistical_unit": verification["statistical_unit"],
            "sources": {
                "method_summary": summary_path.relative_to(ROOT).as_posix(),
                "paired_comparisons": paired_path.relative_to(ROOT).as_posix(),
                "accuracy": accuracy_path.relative_to(ROOT).as_posix(),
                "raw_timing": timing_path.relative_to(ROOT).as_posix(),
                "method_workloads": workload_path.relative_to(ROOT).as_posix(),
                "method_coverage": COVERAGE_FILE.relative_to(ROOT).as_posix(),
                "verification": verification_path.relative_to(ROOT).as_posix(),
            },
        },
        "coverage": coverage,
        "methods": methods,
        "paired_comparisons": [
            method["paired_comparison"] | {"method": method["method"]}
            for method in methods
            if method["library"] == "NUMERIX" and method["paired_comparison"] is not None
        ],
        "limitations": {
            "memory_measurements_available": False,
            "time_to_accuracy_sweep_available": False,
            "tolerance_sweep_available": False,
            "other_six_module_method_level_results_available": False,
            "interpretation": [
                "Runtime/error points use one configured tolerance and are not time-to-accuracy curves.",
                "Paired speed ratios include only problems where both libraries succeeded and passed the accuracy gate.",
                "Ratios and bootstrap intervals are descriptive; no hypothesis test or significance claim was made.",
                "The GSL multiple-root route is a fixed-grid scan plus Brent refinement, not an equivalent native root-discovery API.",
                "CMINPACK's Newton-labelled route is a best-available MINPACK hybrid system solver, not scalar Newton.",
                "Phase 14 process RSS is not a method-specific memory comparison and is not repurposed here.",
            ],
        },
    }

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    json_path = OUT_DIR / "phase14b_root_comparisons.json"
    js_path = OUT_DIR / "phase14b_root_comparisons.js"
    serialized = json.dumps(payload, indent=2, ensure_ascii=False, allow_nan=False) + "\n"
    json_path.write_text(serialized, encoding="utf-8")
    js_literal = json.dumps(payload, separators=(",", ":"), ensure_ascii=False, allow_nan=False)
    js_literal = (js_literal.replace("<", "\\u003c")
                  .replace(">", "\\u003e")
                  .replace("&", "\\u0026")
                  .replace("\u2028", "\\u2028")
                  .replace("\u2029", "\\u2029"))
    js_path.write_text(
        "// Generated locally by scripts/build_benchmark_data.py; do not edit.\n"
        f"window.NUMERIX_BENCHMARK_DATA = {js_literal};\n",
        encoding="utf-8",
    )
    print(f"Wrote {json_path.relative_to(ROOT)}")
    print(f"Wrote {js_path.relative_to(ROOT)}")
    print(f"Routes: {len(methods)}; paired direct methods: {len(payload['paired_comparisons'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
