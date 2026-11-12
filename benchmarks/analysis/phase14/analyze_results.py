#!/usr/bin/env python3
"""Reconstruct Phase 14 summaries and paired comparisons from raw CSV data."""

from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
import sys
from collections import Counter, defaultdict
from pathlib import Path

import numpy as np
from scipy.stats import wilcoxon


ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "benchmarks/problems/phase14/workloads.csv"
METHODS = ROOT / "benchmarks/analysis/phase14/method_mapping.csv"
LIBRARIES = ROOT / "benchmarks/analysis/phase14/libraries.csv"
MODULES = ("root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode")
EXPECTED_LIBRARIES = {
    "root": ["NUMERIX", "GSL", "CMINPACK"],
    "linear_algebra": ["NUMERIX", "GSL", "LAPACK_REFERENCE", "LAPACK_OPENBLAS"],
    "differentiation": ["NUMERIX", "GSL"],
    "interpolation": ["NUMERIX", "GSL"],
    "integration": ["NUMERIX", "GSL"],
    "optimization": ["NUMERIX", "GSL", "NLOPT"],
    "ode": ["NUMERIX", "GSL", "SUNDIALS_ARKODE", "SUNDIALS_CVODE"],
}


def read_csv(path: Path) -> list[dict[str, str]]:
    csv.field_size_limit(sys.maxsize)
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, fields: list[str], rows: list[dict[str, object]]) -> None:
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n", extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def finite_float(value: str) -> float | None:
    try:
        result = float(value)
    except (ValueError, TypeError):
        return None
    return result if math.isfinite(result) else None


def rate(numerator: int, denominator: int) -> str:
    return f"{100.0 * numerator / denominator:.6g}" if denominator else "NA"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", help="directory containing raw Phase 14 run CSV files")
    args = parser.parse_args()
    run_dir = (ROOT / args.run_dir).resolve()
    accuracy = read_csv(run_dir / "accuracy.csv")
    raw = read_csv(run_dir / "raw_timing.csv")
    workload = read_csv(MANIFEST)
    method_rows = read_csv(METHODS)
    libraries = read_csv(LIBRARIES)
    try:
        memory = read_csv(run_dir / "memory.csv")
    except FileNotFoundError:
        memory = []
    try:
        work = read_csv(run_dir / "work_metrics.csv")
    except FileNotFoundError:
        work = []
    metadata_path = run_dir / "run_metadata.json"
    metadata = json.loads(metadata_path.read_text(encoding="utf-8")) if metadata_path.exists() else {}

    problems = {row["problem_id"]: row for row in workload}
    eligible_by_module: dict[str, set[str]] = {
        module: set(libs) for module, libs in EXPECTED_LIBRARIES.items()
    }
    accuracy_by_pair = {(r["module"], r["library"], r["problem_id"]): r for r in accuracy}
    accuracy_duplicate_count = len(accuracy) - len(accuracy_by_pair)
    time_by_problem: dict[tuple[str, str, str], list[float]] = defaultdict(list)
    time_by_size: dict[tuple[str, str, str, str, str], list[float]] = defaultdict(list)
    cpu_by_size: dict[tuple[str, str, str, str, str], list[float]] = defaultdict(list)
    time_batch_counts: Counter[tuple[str, str, str]] = Counter()
    raw_batch_keys: set[tuple[str, str, str, str]] = set()
    duplicate_timing_rows = 0
    cpu_zero_elapsed_rows = 0
    cpu_invalid_elapsed_rows = 0
    for row in raw:
        problem_id = row["problem_id"]
        if problem_id not in problems:
            raise ValueError(f"timing row references unknown problem: {problem_id}")
        batch = finite_float(row["wall_elapsed_seconds"])
        cpu_batch = finite_float(row.get("cpu_elapsed_seconds", ""))
        batch_size = int(row["batch_size"])
        if batch is None or batch <= 0 or batch_size < 1:
            raise ValueError(f"invalid wall timing in row for {problem_id}")
        per_call = batch / batch_size
        key = (row["module"], row["library"], problem_id)
        batch_key = (row["run_id"], row["library"], problem_id, row["batch_index"])
        if batch_key in raw_batch_keys:
            duplicate_timing_rows += 1
        raw_batch_keys.add(batch_key)
        time_by_problem[key].append(per_call)
        time_batch_counts[key] += 1
        size_key = (row["module"], row["library"], row["input_size"], row["method"], row["comparison_mode"])
        time_by_size[size_key].append(per_call)
        if cpu_batch is None or cpu_batch < 0:
            cpu_invalid_elapsed_rows += 1
        elif cpu_batch == 0:
            # Keep zero-resolution deltas in raw_timing.csv, but omit them
            # from per-call CPU summaries rather than treating them as a
            # meaningful zero-cost observation.
            cpu_zero_elapsed_rows += 1
        else:
            cpu_by_size[size_key].append(cpu_batch / batch_size)

    accuracy_fields = [
        "module", "library", "eligible_problems", "accuracy_observations", "timing_problem_count",
        "timing_samples", "status_success_count", "status_success_rate_percent", "converged_count",
        "convergence_rate_percent", "finite_count", "accuracy_pass_count", "accuracy_pass_rate_percent",
        "nonfinite_count", "launch_or_write_failures",
    ]
    failures = read_csv(run_dir / "execution_failures.csv") if (run_dir / "execution_failures.csv").exists() else []
    failure_counts = Counter((r["module"], r["library"]) for r in failures)
    modules_libraries = sorted({(m, lib) for m in MODULES for lib in eligible_by_module[m]})
    coverage_rows: list[dict[str, object]] = []
    for module, library in modules_libraries:
        expected_ids = {p["problem_id"] for p in workload if p["module"] == module}
        records = [accuracy_by_pair[(module, library, pid)] for pid in expected_ids if (module, library, pid) in accuracy_by_pair]
        success = sum(r["status_code"] == "0" for r in records)
        converged = sum(r["converged"] == "1" for r in records)
        finite = sum(r["is_finite"] == "1" for r in records)
        accurate = sum(r["accuracy_pass"] == "1" for r in records)
        timing_keys = [key for key in time_by_problem if key[0] == module and key[1] == library]
        timing_samples = sum(len(time_by_problem[key]) for key in timing_keys)
        coverage_rows.append({
            "module": module, "library": library, "eligible_problems": len(expected_ids),
            "accuracy_observations": len(records), "timing_problem_count": len(timing_keys),
            "timing_samples": timing_samples, "status_success_count": success,
            "status_success_rate_percent": rate(success, len(records)),
            "converged_count": converged, "convergence_rate_percent": rate(converged, len(records)),
            "finite_count": finite, "accuracy_pass_count": accurate,
            "accuracy_pass_rate_percent": rate(accurate, len(records)),
            "nonfinite_count": len(records) - finite,
            "launch_or_write_failures": failure_counts[(module, library)],
        })
    write_csv(run_dir / "coverage.csv", accuracy_fields, coverage_rows)

    measurements: list[dict[str, object]] = []
    for (module, library, input_size, method, mode), samples in sorted(time_by_size.items()):
        measurements.append({
            "module": module, "library": library, "method": method, "comparison_mode": mode,
            "input_size": input_size, "batch_samples": len(samples),
            "batch_size": "|".join(sorted({r["batch_size"] for r in raw if
                r["module"] == module and r["library"] == library and r["input_size"] == input_size and r["method"] == method})),
            "min_seconds_per_call": min(samples), "median_seconds_per_call": statistics.median(samples),
            "mean_seconds_per_call": statistics.fmean(samples),
            "stddev_seconds_per_call": statistics.stdev(samples) if len(samples) > 1 else 0.0,
            "p95_seconds_per_call": float(np.percentile(samples, 95)),
            "max_seconds_per_call": max(samples),
            "cpu_batch_samples": len(cpu_by_size[(module, library, input_size, method, mode)]),
            "cpu_min_seconds_per_call": min(cpu_by_size[(module, library, input_size, method, mode)]) if cpu_by_size[(module, library, input_size, method, mode)] else "",
            "cpu_median_seconds_per_call": statistics.median(cpu_by_size[(module, library, input_size, method, mode)]) if cpu_by_size[(module, library, input_size, method, mode)] else "",
            "cpu_mean_seconds_per_call": statistics.fmean(cpu_by_size[(module, library, input_size, method, mode)]) if cpu_by_size[(module, library, input_size, method, mode)] else "",
            "cpu_stddev_seconds_per_call": statistics.stdev(cpu_by_size[(module, library, input_size, method, mode)]) if len(cpu_by_size[(module, library, input_size, method, mode)]) > 1 else (0.0 if cpu_by_size[(module, library, input_size, method, mode)] else ""),
            "cpu_p95_seconds_per_call": float(np.percentile(cpu_by_size[(module, library, input_size, method, mode)], 95)) if cpu_by_size[(module, library, input_size, method, mode)] else "",
            "cpu_max_seconds_per_call": max(cpu_by_size[(module, library, input_size, method, mode)]) if cpu_by_size[(module, library, input_size, method, mode)] else "",
        })
    write_csv(run_dir / "measurements.csv", list(measurements[0]) if measurements else [], measurements)

    work_summary: list[dict[str, object]] = []
    work_missing_value_errors = 0
    for row in work:
        status = row.get("value_status", "")
        if status == "not_collected" and row.get("value", "").strip():
            work_missing_value_errors += 1
        elif status == "measured" and finite_float(row.get("value", "")) is None:
            work_missing_value_errors += 1
        elif status not in ("measured", "not_collected"):
            work_missing_value_errors += 1
    work_groups: dict[tuple[str, str, str, str], list[float]] = defaultdict(list)
    for row in work:
        if row.get("value_status") != "measured":
            continue
        value = finite_float(row.get("value", ""))
        if value is not None:
            work_groups[(row["module"], row["library"], row["metric"], row["unit"])].append(value)
    for (module, library, metric, unit), values in sorted(work_groups.items()):
        work_summary.append({
            "module": module, "library": library, "metric": metric, "unit": unit,
            "problem_count": len(values), "median": statistics.median(values),
            "mean": statistics.fmean(values), "p95": float(np.percentile(values, 95)),
            "maximum": max(values),
        })
    write_csv(run_dir / "work_summary.csv", ["module", "library", "metric", "unit", "problem_count", "median", "mean", "p95", "maximum"], work_summary)

    memory_summary: list[dict[str, object]] = []
    memory_groups: dict[tuple[str, str, str, str], list[float]] = defaultdict(list)
    for row in memory:
        rss = finite_float(row.get("peak_rss_kb", ""))
        if rss is not None:
            memory_groups[(row["module"], row["library"], row["input_size"], row["memory_type"])].append(rss)
    for (module, library, size, kind), values in sorted(memory_groups.items()):
        memory_summary.append({
            "module": module, "library": library, "input_size": size, "memory_type": kind,
            "problem_count": len(values), "median_peak_rss_kb": statistics.median(values),
            "p95_peak_rss_kb": float(np.percentile(values, 95)), "max_peak_rss_kb": max(values),
        })
    write_csv(run_dir / "memory_summary.csv", ["module", "library", "input_size", "memory_type", "problem_count", "median_peak_rss_kb", "p95_peak_rss_kb", "max_peak_rss_kb"], memory_summary)

    mode_lookup = {(r["module"], r["library"]): r["comparison_mode"] for r in method_rows}
    timing_medians = {key: statistics.median(samples) for key, samples in time_by_problem.items()}
    accuracy_index = {(r["module"], r["library"], r["problem_id"]): r for r in accuracy}
    comparisons: list[dict[str, object]] = []
    bootstrap_rng = np.random.default_rng(1402)
    comparisons_to_make = []
    for module, library in modules_libraries:
        if library == "NUMERIX":
            continue
        comparisons_to_make.append((module, library))
    for module, library in comparisons_to_make:
        ids = [p["problem_id"] for p in workload if p["module"] == module]
        ratios_accuracy: list[float] = []
        ratios_status: list[float] = []
        eligible_status = 0
        paired_common = 0
        for problem_id in ids:
            nkey = (module, "NUMERIX", problem_id)
            ckey = (module, library, problem_id)
            if nkey not in timing_medians or ckey not in timing_medians or nkey not in accuracy_index or ckey not in accuracy_index:
                continue
            paired_common += 1
            nacc, cacc = accuracy_index[nkey], accuracy_index[ckey]
            ntime, ctime = timing_medians[nkey], timing_medians[ckey]
            if nacc["accuracy_pass"] == "1" and cacc["accuracy_pass"] == "1":
                ratio = ctime / ntime
                if math.isfinite(ratio) and ratio > 0:
                    ratios_accuracy.append(ratio)
                if nacc["status_code"] == "0" and cacc["status_code"] == "0":
                    eligible_status += 1
                    ratios_status.append(ratio)
        # A timing ratio is a valid performance comparison only when both
        # libraries returned a successful status and both outputs passed the
        # preregistered accuracy gate.  Accurate-looking output from a solver
        # that exhausted its budget remains useful reliability evidence, but
        # it must not be counted as a successful speed comparison.
        if ratios_status:
            ratio_array = np.asarray(ratios_status, dtype=float)
            median_ratio = float(np.median(ratio_array))
            geometric = float(np.exp(np.mean(np.log(ratio_array))))
            boots_median = []
            boots_geo = []
            for _ in range(10000):
                sample = bootstrap_rng.choice(ratio_array, size=len(ratio_array), replace=True)
                boots_median.append(float(np.median(sample)))
                boots_geo.append(float(np.exp(np.mean(np.log(sample)))))
            ci_low, ci_high = np.percentile(boots_median, [2.5, 97.5])
            geo_low, geo_high = np.percentile(boots_geo, [2.5, 97.5])
            try:
                p_value = float(wilcoxon(np.log(ratio_array), zero_method="wilcox", alternative="two-sided").pvalue)
            except ValueError:
                p_value = 1.0
        else:
            median_ratio = geometric = ci_low = ci_high = geo_low = geo_high = p_value = "NA"
        comparisons.append({
            "module": module, "competitor": library,
            "comparison_mode": mode_lookup.get((module, library), "best_available"),
            "speedup_definition": "competitor median seconds per call / NUMERIX median seconds per call; >1 favors NUMERIX",
            "paired_common_problems": paired_common,
            "accuracy_qualified_pairs": len(ratios_accuracy),
            "status_and_accuracy_qualified_pairs": eligible_status,
            "performance_qualified_pairs": len(ratios_status),
            "median_paired_speedup": median_ratio,
            "median_speedup_ci95_low": ci_low, "median_speedup_ci95_high": ci_high,
            "geometric_mean_speedup": geometric,
            "geometric_mean_ci95_low": geo_low, "geometric_mean_ci95_high": geo_high,
            "wilcoxon_p_two_sided": p_value,
        })
    write_csv(run_dir / "comparisons.csv", list(comparisons[0]) if comparisons else [], comparisons)

    expected_pairs = sum(len(EXPECTED_LIBRARIES[m]) * 100 for m in MODULES)
    unique_accuracy_pairs = len({(r["module"], r["library"], r["problem_id"]) for r in accuracy})
    unique_timing_pairs = len(time_by_problem)
    batch_count_values = list(time_batch_counts.values())
    expected_batches = int(metadata.get("timing_batches", 0))
    bad_batch_count_pairs = sum(n != expected_batches for n in batch_count_values) if expected_batches else 0
    expected_memory_pairs = expected_pairs if metadata.get("memory_requested") else 0
    memory_pairs = len({(r.get("module"), r.get("library"), r.get("problem_id")) for r in memory})
    duplicate_work_rows = len(work) - len({(r.get("run_id"), r.get("library"), r.get("problem_id"), r.get("metric")) for r in work})
    missing_pairs = expected_pairs - unique_accuracy_pairs
    checks_ok = (
        len(workload) == 700 and unique_accuracy_pairs == expected_pairs and
        len(accuracy) == unique_accuracy_pairs and not failures and
        duplicate_timing_rows == 0 and bad_batch_count_pairs == 0 and
        work_missing_value_errors == 0 and duplicate_work_rows == 0 and
        cpu_invalid_elapsed_rows == 0 and
        (not metadata.get("memory_requested") or memory_pairs == expected_memory_pairs)
    )
    verification = {
        "schema_version": "phase14-analysis-verification-v1",
        "problem_count": len(workload),
        "problem_count_by_module": {m: sum(p["module"] == m for p in workload) for m in MODULES},
        "eligible_problem_library_pairs_expected": expected_pairs,
        "accuracy_pair_records": len(accuracy),
        "unique_accuracy_problem_library_pairs": unique_accuracy_pairs,
        "timing_rows": len(raw),
        "unique_timing_problem_library_pairs": unique_timing_pairs,
        "expected_batches_per_pair": expected_batches,
        "minimum_batches_per_pair": min(batch_count_values) if batch_count_values else 0,
        "maximum_batches_per_pair": max(batch_count_values) if batch_count_values else 0,
        "duplicate_accuracy_rows": accuracy_duplicate_count,
        "duplicate_timing_rows": duplicate_timing_rows,
        "cpu_timing_rows": len(raw),
        "cpu_zero_elapsed_rows": cpu_zero_elapsed_rows,
        "cpu_invalid_elapsed_rows": cpu_invalid_elapsed_rows,
        "positive_cpu_timing_rows": len(raw) - cpu_zero_elapsed_rows - cpu_invalid_elapsed_rows,
        "pairs_with_wrong_batch_count": bad_batch_count_pairs,
        "memory_rows": len(memory),
        "memory_problem_library_pairs": memory_pairs,
        "expected_memory_problem_library_pairs": expected_memory_pairs,
        "work_metric_rows": len(work),
        "duplicate_work_rows": duplicate_work_rows,
        "work_missing_value_errors": work_missing_value_errors,
        "execution_failures": len(failures),
        "missing_accuracy_pairs": missing_pairs,
        "result": "PASS" if checks_ok else "PARTIAL",
    }
    (run_dir / "analysis_verification.json").write_text(json.dumps(verification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(verification, indent=2, sort_keys=True))
    return 0 if verification["result"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
