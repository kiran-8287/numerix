#!/usr/bin/env python3
"""Rebuild Phase 13C tables and figures from verified schema-v2 observations."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import statistics

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import TwoSlopeNorm
import numpy as np


LIBRARIES = ("NUMERIX", "GSL")
CORE_CATEGORIES = ("root", "linear", "differentiation", "interpolation",
                   "integration", "optimization", "ode")
PROBLEM_FIELDS = (
    "category", "case", "variant", "difficulty", "input_size", "lower",
    "upper", "initial", "step", "scale", "absolute_tolerance",
    "relative_tolerance", "expected_value", "reference_method",
)


def read_csv(path):
    with Path(path).open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path, rows, fields=None):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if fields is None:
        fields = list(rows[0]) if rows else []
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def problem_id(row):
    payload = json.dumps([row.get(field, "") for field in PROBLEM_FIELDS],
                         ensure_ascii=True, separators=(",", ":"))
    return "problem-" + hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def comparison_class(label, mode):
    if label == "api_equivalent":
        return "DIRECT"
    if label == "ill_conditioned_unranked" or mode == "excluded_unranked_case":
        return "INCOMPARABLE"
    if label == "numerix_only" or mode == "capability_only":
        return "NOT_SUPPORTED"
    if label in {"different_safeguards", "equivalent_representation",
                 "different_representation", "different_work"}:
        return "APPROXIMATE"
    if mode == "best_available" or label == "different_algorithm":
        return "BEST_AVAILABLE"
    return "INCOMPARABLE"


def geometric_mean(values):
    values = [float(value) for value in values if math.isfinite(float(value)) and value > 0]
    if not values:
        return float("nan")
    return math.exp(sum(math.log(value) for value in values) / len(values))


def bootstrap_gmean_ci(values, seed, repeats=4000):
    values = np.asarray([x for x in values if math.isfinite(x) and x > 0], dtype=float)
    if len(values) < 2:
        return float("nan"), float("nan")
    rng = np.random.default_rng(seed)
    logs = np.log(values)
    draws = rng.integers(0, len(logs), size=(repeats, len(logs)))
    estimates = np.exp(np.mean(logs[draws], axis=1))
    low, high = np.quantile(estimates, [0.025, 0.975])
    return float(low), float(high)


def bootstrap_ratio_ci(numerix_samples, gsl_samples, seed, repeats=2000):
    n_values = np.asarray(numerix_samples, dtype=float)
    g_values = np.asarray(gsl_samples, dtype=float)
    if len(n_values) < 3 or len(g_values) < 3:
        return float("nan"), float("nan")
    rng = np.random.default_rng(seed)
    n_draws = rng.integers(0, len(n_values), size=(repeats, len(n_values)))
    g_draws = rng.integers(0, len(g_values), size=(repeats, len(g_values)))
    n_medians = np.median(n_values[n_draws], axis=1)
    g_medians = np.median(g_values[g_draws], axis=1)
    ratios = g_medians / n_medians
    low, high = np.quantile(ratios[np.isfinite(ratios) & (ratios > 0)], [0.025, 0.975])
    return float(low), float(high)


def safe_float(value):
    try:
        result = float(value)
    except (TypeError, ValueError):
        return float("nan")
    return result


def classify_outcome(row):
    if row["status"] != "0":
        if (row["category"] == "root" and row["case"] in {"no_bracket", "tangent_root"}
                and row["method"] in {"bisection", "brent"}):
            return "expected_bracket_rejection"
        if row["category"] == "root" and row["case"] == "oscillatory_root":
            return "newton_convergence_failure"
        if row["category"] == "linear" and row["case"] == "hilbert":
            return "ill_conditioned_factorization_failure"
        return "solver_failure"
    if row["accuracy_pass"] == "0":
        if row["category"] == "linear" and row["case"] == "hilbert":
            return "ill_conditioned_accuracy_failure"
        if row["category"] == "linear" and row["case"].startswith("near_rank_"):
            return "near_singular_accuracy_failure"
        if row["category"] == "integration" and row["method"] in {"trapezoid", "simpson"}:
            return "fixed_rule_accuracy_failure"
        if row["category"] == "ode":
            return "discretization_accuracy_failure"
        return "accuracy_failure"
    if row["accuracy_pass"] == "-1":
        return "accuracy_not_applicable"
    return "successful_and_accurate"


def create_problem_map(workload_rows):
    result = {}
    for row in workload_rows:
        result[row["configuration_id"]] = problem_id(row)
    return result


def make_pairs(measurements, workloads, raw_timing):
    problems = create_problem_map(workloads)
    by_pair = {}
    for row in measurements:
        if row["pair_id"]:
            by_pair.setdefault(row["pair_id"], {})[row["library"]] = row
    samples = {}
    for row in raw_timing:
        samples.setdefault((row["configuration_id"], row["library"]), []).append(
            safe_float(row["elapsed_ns_per_operation"]))
    pairs = []
    for pair_id, members in by_pair.items():
        if set(members) != set(LIBRARIES):
            continue
        numerix, gsl = members["NUMERIX"], members["GSL"]
        label = numerix["comparison_label"]
        mode = numerix["comparison_mode"]
        classification = comparison_class(label, mode)
        n_median = safe_float(numerix["median_ns"])
        g_median = safe_float(gsl["median_ns"])
        n_ok = numerix["status"] == "0" and numerix["accuracy_pass"] == "1"
        g_ok = gsl["status"] == "0" and gsl["accuracy_pass"] == "1"
        eligible = (classification in {"DIRECT", "APPROXIMATE", "BEST_AVAILABLE"}
                    and n_ok and g_ok and n_median > 0 and g_median > 0)
        ratio = g_median / n_median if eligible else float("nan")
        reason = "eligible"
        if classification in {"INCOMPARABLE", "NOT_SUPPORTED"}:
            reason = classification.lower()
        elif numerix["status"] != "0" or gsl["status"] != "0":
            reason = "one or both solvers returned failure status"
        elif not n_ok or not g_ok:
            reason = "one or both outputs failed the accuracy gate or have no accuracy result"
        elif not math.isfinite(ratio) or ratio <= 0:
            reason = "invalid timing median"
        seed = int(hashlib.sha256(pair_id.encode("utf-8")).hexdigest()[:8], 16)
        low, high = (float("nan"), float("nan"))
        if eligible:
            low, high = bootstrap_ratio_ci(
                samples.get((pair_id, "NUMERIX"), []),
                samples.get((pair_id, "GSL"), []), seed)
        pairs.append({
            "problem_id": problems.get(pair_id, ""),
            "pair_id": pair_id,
            "category": numerix["category"],
            "case": numerix["case"],
            "input_size": numerix["input_size"],
            "method": numerix["method"],
            "phase": numerix["phase"],
            "comparison_class": classification,
            "comparison_label": label,
            "comparison_mode": mode,
            "numerix_status": numerix["status"],
            "gsl_status": gsl["status"],
            "numerix_accuracy_pass": numerix["accuracy_pass"],
            "gsl_accuracy_pass": gsl["accuracy_pass"],
            "numerix_median_ns": n_median,
            "gsl_median_ns": g_median,
            "numerix_function_evaluations": numerix.get("function_evaluations", ""),
            "gsl_function_evaluations": gsl.get("function_evaluations", ""),
            "numerix_derivative_evaluations": numerix.get("derivative_evaluations", ""),
            "gsl_derivative_evaluations": gsl.get("derivative_evaluations", ""),
            "numerix_iterations": numerix.get("iterations", ""),
            "gsl_iterations": gsl.get("iterations", ""),
            "speedup_gsl_over_numerix": ratio,
            "bootstrap_ratio_95_low": low,
            "bootstrap_ratio_95_high": high,
            "eligible_for_claim": int(eligible),
            "exclusion_reason": reason,
        })
    return pairs, samples, problems


def summarize_category_performance(pairs, classification):
    output = []
    for category in CORE_CATEGORIES:
        selected = [row for row in pairs if row["category"] == category
                    and row["comparison_class"] == classification
                    and row["eligible_for_claim"] == 1]
        problem_values = {}
        for row in selected:
            problem_values.setdefault(row["problem_id"], []).append(
                row["speedup_gsl_over_numerix"])
        per_problem = [geometric_mean(values) for values in problem_values.values()]
        estimate = geometric_mean(per_problem)
        seed = sum(ord(ch) for ch in category + classification) + 13013
        low, high = bootstrap_gmean_ci(per_problem, seed)
        output.append({
            "category": category,
            "comparison_class": classification,
            "problem_configurations": len(problem_values),
            "eligible_method_phase_pairs": len(selected),
            "gmean_speedup_gsl_over_numerix": estimate,
            "problem_bootstrap_95_low": low,
            "problem_bootstrap_95_high": high,
            "interpretation": "ratio > 1 favors NUMERIX; < 1 favors GSL",
        })
    return output


def aggregate_accuracy(measurements, workloads, accuracy_rows):
    problem_map = create_problem_map(workloads)
    by_config = {}
    for row in accuracy_rows:
        key = (row["configuration_id"], row["library"])
        by_config.setdefault(key, []).append(row)
    results = []
    for row in measurements:
        key = (row["configuration_id"], row["library"])
        probes = by_config.get(key, [])
        abs_errors = [safe_float(p["absolute_error"]) for p in probes]
        rel_errors = [safe_float(p["relative_error"]) for p in probes]
        abs_errors = [x for x in abs_errors if math.isfinite(x)]
        rel_errors = [x for x in rel_errors if math.isfinite(x)]
        results.append({
            "problem_id": problem_map.get(row["configuration_id"], ""),
            "configuration_id": row["configuration_id"],
            "category": row["category"], "case": row["case"],
            "method": row["method"], "phase": row["phase"],
            "library": row["library"], "status": row["status"],
            "accuracy_pass": row["accuracy_pass"],
            "probe_count": len(probes),
            "accuracy_probe_passes": sum(p["accuracy_pass"] == "1" for p in probes),
            "median_absolute_error": statistics.median(abs_errors) if abs_errors else "",
            "maximum_absolute_error": max(abs_errors) if abs_errors else "",
            "median_relative_error": statistics.median(rel_errors) if rel_errors else "",
            "maximum_relative_error": max(rel_errors) if rel_errors else "",
            "function_evaluations": row["function_evaluations"],
            "derivative_evaluations": row["derivative_evaluations"],
            "iterations": row["iterations"],
            "residual": row["residual"], "estimated_error": row["estimated_error"],
            "backward_error": row["backward_error"],
            "trajectory_error": row["trajectory_error"],
        })
    return results


def summarize_coverage(plan, workloads, pairs):
    plan = [row for row in plan if row["category"] in CORE_CATEGORIES]
    by_actual = {}
    for row in workloads:
        if row["category"] in CORE_CATEGORIES:
            by_actual.setdefault(problem_id(row), set()).add(
                (row["configuration_id"], row["library"]))
    direct = {}
    eligible_other = {}
    excluded = {}
    for row in pairs:
        direct.setdefault(row["category"], set())
        eligible_other.setdefault(row["category"], set())
        excluded.setdefault(row["category"], 0)
        if row["comparison_class"] == "DIRECT" and row["eligible_for_claim"] == 1:
            direct[row["category"]].add(row["pair_id"])
        elif row["comparison_class"] in {"BEST_AVAILABLE", "APPROXIMATE"} and row["eligible_for_claim"] == 1:
            eligible_other[row["category"]].add(row["pair_id"])
        else:
            excluded[row["category"]] += 1
    output = []
    for category in CORE_CATEGORIES:
        planned_rows = [row for row in plan if row["category"] == category]
        ids = {row["problem_id"] for row in planned_rows}
        attempted = ids.intersection(by_actual)
        completed = sum(
            len(by_actual.get(row["problem_id"], set())) ==
            int(row["planned_workload_records"]) for row in planned_rows)
        output.append({
            "category": category,
            "planned_problem_configurations": len(planned_rows),
            "attempted_problem_configurations": len(attempted),
            "completed_problem_configurations": completed,
            "eligible_direct_method_phase_pairs": len(direct.get(category, set())),
            "eligible_best_available_or_approximate_pairs": len(eligible_other.get(category, set())),
            "excluded_or_ineligible_paired_workloads": excluded.get(category, 0),
        })
    return output


def summarize_reliability(measurements, workloads):
    problem_map = create_problem_map(workloads)
    output = []
    for category in (*CORE_CATEGORIES, "capability"):
        for library in LIBRARIES:
            rows = [r for r in measurements if r["category"] == category and
                    r["library"] == library]
            if not rows:
                continue
            failures = [r for r in rows if r["status"] != "0"]
            accuracy_eligible = [r for r in rows if r["status"] == "0" and
                                 r["accuracy_pass"] in {"0", "1"}]
            pass_count = sum(r["accuracy_pass"] == "1" for r in accuracy_eligible)
            outcome_counts = {}
            for row in rows:
                outcome = classify_outcome(row)
                outcome_counts[outcome] = outcome_counts.get(outcome, 0) + 1
            output.append({
                "category": category, "library": library,
                "workload_method_phase_records": len(rows),
                "distinct_problem_configurations": len({
                    problem_map.get(r["configuration_id"], "") for r in rows}),
                "solver_success_records": sum(r["status"] == "0" for r in rows),
                "solver_failure_records": len(failures),
                "solver_success_rate": sum(r["status"] == "0" for r in rows) / len(rows),
                "accuracy_applicable_records": len(accuracy_eligible),
                "accuracy_pass_records": pass_count,
                "accuracy_pass_rate": pass_count / len(accuracy_eligible)
                if accuracy_eligible else "",
                "expected_bracket_rejections": outcome_counts.get("expected_bracket_rejection", 0),
                "convergence_failures": outcome_counts.get("newton_convergence_failure", 0),
                "ill_conditioned_failures": outcome_counts.get("ill_conditioned_factorization_failure", 0) +
                    outcome_counts.get("ill_conditioned_accuracy_failure", 0) +
                    outcome_counts.get("near_singular_accuracy_failure", 0),
                "accuracy_failures": sum(v for k, v in outcome_counts.items()
                                          if k.endswith("accuracy_failure")),
            })
    return output


def summarize_algorithm_performance(pairs):
    groups = {}
    for row in pairs:
        if row["eligible_for_claim"] != 1:
            continue
        key = (row["category"], row["comparison_class"], row["method"], row["phase"])
        groups.setdefault(key, []).append(row)
    output = []
    for key, rows in sorted(groups.items()):
        problem_values = {}
        for row in rows:
            problem_values.setdefault(row["problem_id"], []).append(
                row["speedup_gsl_over_numerix"])
        values = [geometric_mean(v) for v in problem_values.values()]
        low, high = bootstrap_gmean_ci(values, sum(ord(ch) for ch in "|".join(key)) + 13014)
        output.append({
            "category": key[0], "comparison_class": key[1],
            "method": key[2], "phase": key[3],
            "problem_configurations": len(problem_values),
            "eligible_pairs": len(rows),
            "gmean_speedup_gsl_over_numerix": geometric_mean(values),
            "problem_bootstrap_95_low": low,
            "problem_bootstrap_95_high": high,
        })
    return output


def summarize_wins(pairs):
    counts = {}
    for row in pairs:
        if row["comparison_class"] != "DIRECT" or row["eligible_for_claim"] != 1:
            continue
        category = row["category"]
        counts.setdefault(category, {"NUMERIX wins": 0, "GSL wins": 0, "Tie / uncertain": 0})
        low = safe_float(row["bootstrap_ratio_95_low"])
        high = safe_float(row["bootstrap_ratio_95_high"])
        if math.isfinite(low) and low > 1.0:
            label = "NUMERIX wins"
        elif math.isfinite(high) and high < 1.0:
            label = "GSL wins"
        else:
            label = "Tie / uncertain"
        counts[category][label] += 1
    return [{"category": category, **counts[category],
             "win_rule": "95% bootstrap interval for paired workload median ratio excludes 1; otherwise tie/uncertain"}
            for category in CORE_CATEGORIES if category in counts]


def summarize_memory(memory_rows, measurements, workloads):
    problem_map = create_problem_map(workloads)
    main = {(r["configuration_id"], r["library"]): r for r in measurements}
    output = []
    for row in memory_rows:
        identity = (row["configuration_id"], row["library"])
        timed = main.get(identity, {})
        output.append({
            "problem_id": problem_map.get(row["configuration_id"], ""),
            "configuration_id": row["configuration_id"],
            "pair_id": row["pair_id"], "category": row["category"],
            "case": row["case"], "method": row["method"], "phase": row["phase"],
            "comparison_label": row["comparison_label"],
            "comparison_mode": row["comparison_mode"],
            "library": row["library"], "comparison_class": comparison_class(
                row["comparison_label"], row["comparison_mode"]),
            "status": row["status"],
            "accuracy_pass": row["accuracy_pass"],
            "main_run_median_ns": timed.get("median_ns", ""),
            "requested_heap_bytes": row["requested_heap_bytes"],
            "peak_live_requested_heap_bytes": row["peak_live_requested_heap_bytes"],
            "allocation_calls": row["allocation_calls"],
            "setup_live_requested_heap_bytes": row["setup_live_requested_heap_bytes"],
            "live_requested_heap_bytes_after_destroy": row["live_requested_heap_bytes_after_destroy"],
        })
    return output


def summarize_accuracy(measurements, workloads):
    problems = create_problem_map(workloads)
    by_pair = {}
    for row in measurements:
        if row["pair_id"]:
            by_pair.setdefault(row["pair_id"], {})[row["library"]] = row
    pair_rows = []
    grouped = {}
    for pair_id, members in by_pair.items():
        if set(members) != set(LIBRARIES):
            continue
        n, g = members["NUMERIX"], members["GSL"]
        if n["status"] != "0" or g["status"] != "0":
            continue
        n_abs, g_abs = safe_float(n["absolute_error"]), safe_float(g["absolute_error"])
        n_rel, g_rel = safe_float(n["relative_error"]), safe_float(g["relative_error"])
        if not (math.isfinite(n_abs) and math.isfinite(g_abs)):
            continue
        if math.isfinite(n_rel) and math.isfinite(g_rel):
            n_measure, g_measure, measure_name = n_rel, g_rel, "relative_error"
        else:
            n_measure, g_measure, measure_name = n_abs, g_abs, "absolute_error"
        tolerance = 1e-15 * max(1.0, abs(n_measure), abs(g_measure))
        if abs(n_measure - g_measure) <= tolerance:
            better = "tie"
        else:
            better = "NUMERIX" if n_measure < g_measure else "GSL"
        pid = problems.get(pair_id, "")
        record = {
            "problem_id": pid, "pair_id": pair_id, "category": n["category"],
            "case": n["case"], "method": n["method"], "phase": n["phase"],
            "comparison_class": comparison_class(n["comparison_label"], n["comparison_mode"]),
            "numerix_absolute_error": n_abs, "gsl_absolute_error": g_abs,
            "numerix_relative_error": n_rel, "gsl_relative_error": g_rel,
            "accuracy_metric_for_direction": measure_name,
            "numerix_accuracy_pass": n["accuracy_pass"],
            "gsl_accuracy_pass": g["accuracy_pass"], "lower_error": better,
        }
        pair_rows.append(record)
        grouped.setdefault((n["category"], pid), []).append(record)

    per_problem = {}
    for (category, pid), records in grouped.items():
        by_library = {"NUMERIX": [], "GSL": []}
        for record in records:
            for library in LIBRARIES:
                value = safe_float(record[library.lower() + "_relative_error"])
                if not math.isfinite(value):
                    value = safe_float(record[library.lower() + "_absolute_error"])
                by_library[library].append(value)
        n_error = statistics.median(by_library["NUMERIX"])
        g_error = statistics.median(by_library["GSL"])
        per_problem.setdefault(category, []).append((pid, n_error, g_error))

    summary = []
    for category in CORE_CATEGORIES:
        observations = per_problem.get(category, [])
        n_wins = sum(n < g for _, n, g in observations)
        g_wins = sum(g < n for _, n, g in observations)
        ties = len(observations) - n_wins - g_wins
        refs = sorted({r["reference_method"] for r in measurements
                       if r["category"] == category})
        n_values = [n for _, n, _ in observations]
        g_values = [g for _, _, g in observations]
        summary.append({
            "category": category,
            "paired_problem_configurations": len(observations),
            "numerix_median_error": statistics.median(n_values) if n_values else "",
            "gsl_median_error": statistics.median(g_values) if g_values else "",
            "lower_error_problem_configurations_numerix": n_wins,
            "lower_error_problem_configurations_gsl": g_wins,
            "ties": ties,
            "reference_methods": ";".join(refs),
            "metric_note": "median per-problem error across measured paired methods/phases; relative error when defined, otherwise absolute error",
        })
    return pair_rows, summary


def summarize_memory_comparisons(memory_rows):
    by_pair = {}
    for row in memory_rows:
        if row["pair_id"]:
            by_pair.setdefault(row["pair_id"], {})[row["library"]] = row
    pairs = []
    for pair_id, members in by_pair.items():
        if set(members) != set(LIBRARIES):
            continue
        n, g = members["NUMERIX"], members["GSL"]
        classification = comparison_class(n["comparison_label"], n["comparison_mode"])
        n_peak = safe_float(n["peak_live_requested_heap_bytes"])
        g_peak = safe_float(g["peak_live_requested_heap_bytes"])
        n_traffic = safe_float(n["requested_heap_bytes"])
        g_traffic = safe_float(g["requested_heap_bytes"])
        eligible = (classification == "DIRECT" and n["status"] == "0" and
                    g["status"] == "0" and n["accuracy_pass"] == "1" and
                    g["accuracy_pass"] == "1")
        pairs.append({
            "pair_id": pair_id, "category": n["category"], "case": n["case"],
            "method": n["method"], "phase": n["phase"],
            "comparison_class": classification,
            "numerix_peak_live_requested_bytes": n_peak,
            "gsl_peak_live_requested_bytes": g_peak,
            "gsl_minus_numerix_peak_bytes": g_peak - n_peak,
            "numerix_cumulative_requested_bytes": n_traffic,
            "gsl_cumulative_requested_bytes": g_traffic,
            "numerix_allocation_calls": n["allocation_calls"],
            "gsl_allocation_calls": g["allocation_calls"],
            "eligible_direct_memory_pair": int(eligible),
            "note": "wrapped allocation calls only; setup plus operation, not RSS",
        })
    summary = []
    for category in CORE_CATEGORIES:
        valid = [r for r in pairs if r["category"] == category and
                 r["eligible_direct_memory_pair"] == 1]
        if not valid:
            summary.append({"category": category, "eligible_direct_pairs": 0,
                           "numerix_median_peak_bytes": "", "gsl_median_peak_bytes": "",
                           "median_gsl_minus_numerix_peak_bytes": "",
                           "numerix_lower_peak_count": 0, "gsl_lower_peak_count": 0,
                           "ties": 0})
            continue
        differences = [r["gsl_minus_numerix_peak_bytes"] for r in valid]
        summary.append({
            "category": category, "eligible_direct_pairs": len(valid),
            "numerix_median_peak_bytes": statistics.median(
                r["numerix_peak_live_requested_bytes"] for r in valid),
            "gsl_median_peak_bytes": statistics.median(
                r["gsl_peak_live_requested_bytes"] for r in valid),
            "median_gsl_minus_numerix_peak_bytes": statistics.median(differences),
            "numerix_lower_peak_count": sum(d > 0 for d in differences),
            "gsl_lower_peak_count": sum(d < 0 for d in differences),
            "ties": sum(d == 0 for d in differences),
        })
    return pairs, summary


def save_figure(fig, directory, name):
    directory.mkdir(parents=True, exist_ok=True)
    fig.savefig(directory / (name + ".png"), dpi=220, bbox_inches="tight")
    svg_path = directory / (name + ".svg")
    fig.savefig(svg_path, bbox_inches="tight")
    svg_lines = svg_path.read_text(encoding="utf-8").splitlines()
    svg_path.write_text("\n".join(line.rstrip() for line in svg_lines) + "\n",
                        encoding="utf-8")
    plt.close(fig)


def make_figures(out, measurements, raw, pairs, category_perf, algorithm_perf,
                 accuracy, reliability, memory):
    fig_dir = out / "figures"
    plt.rcParams.update({"font.size": 9, "axes.titlesize": 10,
                         "figure.titlesize": 13, "axes.labelsize": 9})

    direct = [r for r in category_perf if r["comparison_class"] == "DIRECT" and
              math.isfinite(r["gmean_speedup_gsl_over_numerix"])]
    fig, ax = plt.subplots(figsize=(8.5, 4.2))
    if direct:
        y = np.arange(len(direct))
        values = [r["gmean_speedup_gsl_over_numerix"] for r in direct]
        lows = [r["problem_bootstrap_95_low"] for r in direct]
        highs = [r["problem_bootstrap_95_high"] for r in direct]
        ax.errorbar(values, y,
                    xerr=[[v - l if math.isfinite(l) else 0 for v, l in zip(values, lows)],
                          [h - v if math.isfinite(h) else 0 for v, h in zip(values, highs)]],
                    fmt="o", color="#1769aa", capsize=3)
        ax.set_yticks(y, [r["category"] for r in direct])
        ax.axvline(1.0, color="black", linewidth=1, linestyle="--")
        ax.set_xscale("log", base=2)
        ax.set_xlabel("Geometric mean of GSL / NUMERIX median runtime (95% problem bootstrap CI)")
        ax.set_title("Same-algorithm performance by category")
        ax.grid(True, axis="x", alpha=0.25)
    else:
        ax.text(0.5, 0.5, "No eligible direct comparisons", ha="center", va="center")
    save_figure(fig, fig_dir, "direct_speedup_by_category")

    best = [r for r in category_perf if r["comparison_class"] in {"BEST_AVAILABLE", "APPROXIMATE"}
            and math.isfinite(r["gmean_speedup_gsl_over_numerix"])]
    fig, ax = plt.subplots(figsize=(8.5, 4.2))
    if best:
        labels = [r["category"] + " · " + r["comparison_class"].replace("_", " ") for r in best]
        y = np.arange(len(best))
        vals = [r["gmean_speedup_gsl_over_numerix"] for r in best]
        lo = [r["problem_bootstrap_95_low"] for r in best]
        hi = [r["problem_bootstrap_95_high"] for r in best]
        ax.errorbar(vals, y,
                    xerr=[[v - l if math.isfinite(l) else 0 for v, l in zip(vals, lo)],
                          [h - v if math.isfinite(h) else 0 for v, h in zip(vals, hi)]],
                    fmt="o", color="#d97706", capsize=3)
        ax.set_yticks(y, labels)
        ax.axvline(1.0, color="black", linewidth=1, linestyle="--")
        ax.set_xscale("log", base=2)
        ax.set_xlabel("Descriptive GSL / NUMERIX median runtime ratio; not same-algorithm speedup")
        ax.set_title("Best-available and approximate comparisons (separate from direct results)")
        ax.grid(True, axis="x", alpha=0.25)
    else:
        ax.text(0.5, 0.5, "No eligible best-available comparisons", ha="center", va="center")
    save_figure(fig, fig_dir, "best_available_category_ratios")

    direct_pairs = [r for r in pairs if r["comparison_class"] == "DIRECT" and
                    r["eligible_for_claim"] == 1]
    fig, ax = plt.subplots(figsize=(6.8, 6.2))
    colors = plt.get_cmap("tab10")
    for index, category in enumerate(CORE_CATEGORIES):
        subset = [r for r in direct_pairs if r["category"] == category]
        if subset:
            x = [r["numerix_median_ns"] for r in subset]
            y = [r["gsl_median_ns"] for r in subset]
            ax.scatter(x, y, s=24, alpha=0.75, color=colors(index), label=category)
    values = [r["numerix_median_ns"] for r in direct_pairs] + [r["gsl_median_ns"] for r in direct_pairs]
    if values:
        low, high = min(values), max(values)
        ax.plot([low, high], [low, high], color="black", linestyle="--", linewidth=1)
        ax.set_xscale("log")
        ax.set_yscale("log")
    ax.set_xlabel("NUMERIX median wall time (ns / operation)")
    ax.set_ylabel("GSL median wall time (ns / operation)")
    ax.set_title("Direct paired runtime; each point is a method/phase workload")
    if direct_pairs:
        ax.legend(fontsize=8, ncol=2)
    ax.grid(True, which="both", alpha=0.2)
    save_figure(fig, fig_dir, "direct_runtime_scatter")

    perf = [r for r in algorithm_perf if r["comparison_class"] == "DIRECT"]
    methods = sorted({r["category"] + " / " + r["method"] + " / " + r["phase"] for r in perf})
    categories = list(CORE_CATEGORIES)
    matrix = np.full((len(methods), len(categories)), np.nan)
    labels = np.full((len(methods), len(categories)), "", dtype=object)
    for row in perf:
        method = row["category"] + " / " + row["method"] + " / " + row["phase"]
        i, j = methods.index(method), categories.index(row["category"])
        value = row["gmean_speedup_gsl_over_numerix"]
        if math.isfinite(value) and value > 0:
            matrix[i, j] = math.log2(value)
            labels[i, j] = f"{value:.2g}\n(n={row['problem_configurations']})"
    fig, ax = plt.subplots(figsize=(9.2, max(3.8, 0.36 * len(methods))))
    masked = np.ma.masked_invalid(matrix)
    image = ax.imshow(masked, aspect="auto", cmap="RdBu_r",
                      norm=TwoSlopeNorm(vmin=-2, vcenter=0, vmax=2))
    ax.set_xticks(np.arange(len(categories)), categories, rotation=30, ha="right")
    ax.set_yticks(np.arange(len(methods)), methods)
    ax.set_title("Algorithm/phase performance heatmap (log2 GSL / NUMERIX; >0 favors NUMERIX)")
    for i in range(len(methods)):
        for j in range(len(categories)):
            if labels[i, j]:
                ax.text(j, i, labels[i, j], ha="center", va="center", fontsize=7)
    fig.colorbar(image, ax=ax, label="log2(GSL median / NUMERIX median)")
    save_figure(fig, fig_dir, "algorithm_performance_heatmap")

    win_rows = summarize_wins(pairs)
    fig, ax = plt.subplots(figsize=(7.5, 4.3))
    if win_rows:
        columns = ("NUMERIX wins", "Tie / uncertain", "GSL wins")
        matrix = np.asarray([[row[column] for column in columns] for row in win_rows])
        image = ax.imshow(matrix, aspect="auto", cmap="RdYlGn_r", vmin=0,
                          vmax=max(1, int(matrix.max())))
        ax.set_xticks(np.arange(len(columns)), columns, rotation=15, ha="right")
        ax.set_yticks(np.arange(len(win_rows)), [r["category"] for r in win_rows])
        ax.set_title("Direct workload wins and ties (95% interval rule)")
        for i in range(matrix.shape[0]):
            for j in range(matrix.shape[1]):
                ax.text(j, i, str(matrix[i, j]), ha="center", va="center", fontsize=9)
        fig.colorbar(image, ax=ax, label="Method/phase workload pairings")
    save_figure(fig, fig_dir, "win_loss_tie_heatmap")

    relative = [r for r in accuracy if r["status"] == "0" and
                math.isfinite(safe_float(r["median_relative_error"]))]
    fig, axes = plt.subplots(2, 4, figsize=(13, 6.5), squeeze=False)
    for idx, category in enumerate(CORE_CATEGORIES):
        ax = axes.flat[idx]
        for library, color in (("NUMERIX", "#1769aa"), ("GSL", "#d97706")):
            subset = [r for r in relative if r["category"] == category and r["library"] == library]
            for accuracy_flag, marker in (("1", "o"), ("0", "x")):
                flagged = [r for r in subset if r["accuracy_pass"] == accuracy_flag]
                values = [max(safe_float(r["median_relative_error"]), 1e-18) for r in flagged]
                if values:
                    ax.scatter(np.arange(len(values)), values, s=16, alpha=0.6,
                               color=color, marker=marker,
                               label=library if marker == "o" else None)
        ax.set_yscale("log")
        ax.set_title(category)
        ax.set_ylabel("Relative error")
        ax.set_xlabel("Solver-success workload results (× failed accuracy gate)")
        ax.grid(True, axis="y", alpha=0.2)
    axes.flat[-1].axis("off")
    axes.flat[0].legend(fontsize=7)
    fig.suptitle("Relative error across solver-success results; × marks accuracy failures")
    save_figure(fig, fig_dir, "relative_accuracy_by_category")

    absolute = [r for r in accuracy if r["status"] == "0" and
                math.isfinite(safe_float(r["median_absolute_error"]))]
    fig, axes = plt.subplots(2, 4, figsize=(13, 6.5), squeeze=False)
    for idx, category in enumerate(CORE_CATEGORIES):
        ax = axes.flat[idx]
        for library, color in (("NUMERIX", "#1769aa"), ("GSL", "#d97706")):
            subset = [r for r in absolute if r["category"] == category and r["library"] == library]
            for accuracy_flag, marker in (("1", "o"), ("0", "x")):
                flagged = [r for r in subset if r["accuracy_pass"] == accuracy_flag]
                values = [max(safe_float(r["median_absolute_error"]), 1e-18) for r in flagged]
                if values:
                    ax.scatter(np.arange(len(values)), values, s=16, alpha=0.6,
                               color=color, marker=marker,
                               label=library if marker == "o" else None)
        ax.set_yscale("log")
        ax.set_title(category)
        ax.set_ylabel("Absolute error (category-specific units)")
        ax.set_xlabel("Workload results")
        ax.grid(True, axis="y", alpha=0.2)
    axes.flat[-1].axis("off")
    axes.flat[0].legend(fontsize=7)
    fig.suptitle("Absolute error across solver-success outputs; × marks accuracy failures")
    save_figure(fig, fig_dir, "absolute_accuracy_by_category")

    colors = {"NUMERIX": "#1769aa", "GSL": "#d97706"}
    fig, axes = plt.subplots(2, 4, figsize=(13, 7), squeeze=False)
    for idx, category in enumerate(CORE_CATEGORIES):
        ax = axes.flat[idx]
        for library in LIBRARIES:
            subset = [r for r in measurements if r["category"] == category and
                      r["library"] == library and r["status"] == "0" and
                      math.isfinite(safe_float(r["relative_error"]))]
            for accuracy_flag, marker in (("1", "o"), ("0", "x")):
                flagged = [r for r in subset if r["accuracy_pass"] == accuracy_flag]
                x = [max(safe_float(r["median_ns"]), 1e-12) for r in flagged]
                y = [max(safe_float(r["relative_error"]), 1e-18) for r in flagged]
                if x:
                    ax.scatter(x, y, s=17, alpha=0.65, color=colors[library], marker=marker,
                               label=library if marker == "o" else None)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_title(category)
        ax.set_xlabel("Median ns / operation")
        ax.set_ylabel("Relative error")
        ax.grid(True, which="both", alpha=0.18)
    axes.flat[-1].axis("off")
    axes.flat[0].legend(fontsize=7)
    fig.suptitle("Accuracy versus runtime; × marks accuracy failures")
    save_figure(fig, fig_dir, "accuracy_vs_runtime")

    fig, axes = plt.subplots(2, 4, figsize=(13, 6.5), squeeze=False)
    for idx, category in enumerate(CORE_CATEGORIES):
        ax = axes.flat[idx]
        for library, color in colors.items():
            vals = [safe_float(r["elapsed_ns_per_operation"]) for r in raw
                    if r["category"] == category and r["library"] == library and
                    math.isfinite(safe_float(r["elapsed_ns_per_operation"]))]
            if vals:
                ax.boxplot(vals, positions=[1 if library == "NUMERIX" else 2],
                           widths=0.55, showfliers=False,
                           boxprops={"color": color}, medianprops={"color": color},
                           whiskerprops={"color": color}, capprops={"color": color})
        ax.set_yscale("log")
        ax.set_xticks([1, 2], ["NUMERIX", "GSL"])
        ax.set_title(category)
        ax.set_ylabel("Batch mean ns / operation")
        ax.grid(True, axis="y", alpha=0.2)
    axes.flat[-1].axis("off")
    fig.suptitle("Timing-batch distributions; samples are nested in workloads")
    save_figure(fig, fig_dir, "timing_variability_by_category")

    # Problem-size scalability: dense LU total cost and conditioning diagnostics.
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2))
    for case, label, color in (("dense_well", "scale 1", "#1769aa"),
                               ("dense_small", "scale 1e-12", "#d97706"),
                               ("dense_large", "scale 1e12", "#2e8b57")):
        for library, marker in (("NUMERIX", "o"), ("GSL", "s")):
            subset = [r for r in measurements if r["category"] == "linear" and
                      r["case"] == case and r["phase"] == "total" and
                      r["library"] == library and r["status"] == "0" and
                      r["accuracy_pass"] == "1"]
            subset.sort(key=lambda r: int(r["input_size"]))
            if subset:
                axes[0].plot([int(r["input_size"]) for r in subset],
                             [safe_float(r["median_ns"]) for r in subset],
                             marker=marker, color=color, linestyle="-" if library == "NUMERIX" else "--",
                             label=f"{label}, {library}")
                axes[1].plot([int(r["input_size"]) for r in subset],
                             [max(safe_float(r["backward_error"]), 1e-18) for r in subset],
                             marker=marker, color=color, linestyle="-" if library == "NUMERIX" else "--",
                             label=f"{label}, {library}")
    axes[0].set_yscale("log")
    axes[0].set_xlabel("Dense matrix dimension n")
    axes[0].set_ylabel("Median ns / total factor and solve")
    axes[0].set_title("Runtime versus matrix size")
    axes[1].set_yscale("log")
    axes[1].set_xlabel("Dense matrix dimension n")
    axes[1].set_ylabel("Normwise backward error")
    axes[1].set_title("Backward error versus matrix size")
    for ax in axes:
        ax.grid(True, which="both", alpha=0.2)
        ax.legend(fontsize=7)
    fig.suptitle("Dense LU scalability; only successful, accuracy-passing systems shown")
    save_figure(fig, fig_dir, "linear_scalability_and_backward_error")

    fig, ax = plt.subplots(figsize=(7.5, 4.5))
    for library in LIBRARIES:
        subset = [r for r in measurements if r["category"] == "interpolation" and
                  r["method"] == "newton" and r["phase"] == "evaluate" and
                  r["case"] in {"uniform_cubic", "uniform_cubic_n4", "uniform_cubic_n8",
                                "uniform_cubic_n16", "uniform_quartic_n8",
                                "uniform_quartic_n16"} and
                  r["library"] == library and r["status"] == "0" and
                  r["accuracy_pass"] == "1"]
        subset.sort(key=lambda r: int(r["input_size"]))
        if subset:
            ax.scatter([int(r["input_size"]) for r in subset],
                       [safe_float(r["median_ns"]) for r in subset],
                       label=library, color=colors[library], alpha=0.8)
    ax.set_yscale("log")
    ax.set_xlabel("Interpolation nodes")
    ax.set_ylabel("Median evaluation time (ns)")
    ax.set_title("Polynomial interpolation evaluation versus data size")
    ax.legend()
    ax.grid(True, which="both", alpha=0.2)
    save_figure(fig, fig_dir, "interpolation_scalability")

    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2))
    selected_ode = {"exponential_growth", "exponential_decay", "harmonic",
                    "exponential_growth_steps_10", "exponential_decay_steps_10",
                    "harmonic_steps_10", "exponential_growth_steps_500",
                    "exponential_decay_steps_500", "harmonic_steps_500"}
    for library in LIBRARIES:
        subset = [r for r in measurements if r["category"] == "ode" and
                  r["method"] == "fixed_step_rk4" and r["case"] in selected_ode and
                  r["library"] == library and r["status"] == "0"]
        subset.sort(key=lambda r: (r["case"].split("_steps")[0], int(r["input_size"])))
        if subset:
            for case in sorted({r["case"].split("_steps")[0] for r in subset}):
                series = [r for r in subset if r["case"].split("_steps")[0] == case]
                axes[0].plot([int(r["input_size"]) for r in series],
                             [safe_float(r["median_ns"]) for r in series],
                             marker="o", color=colors[library],
                             linestyle="-" if library == "NUMERIX" else "--",
                             label=f"{case}, {library}")
                axes[1].plot([int(r["input_size"]) for r in series],
                             [max(safe_float(r["trajectory_error"]), 1e-18) for r in series],
                             marker="o", color=colors[library],
                             linestyle="-" if library == "NUMERIX" else "--",
                             label=f"{case}, {library}")
    axes[0].set_yscale("log")
    axes[0].set_xlabel("Fixed RK4 steps on [0,1]")
    axes[0].set_ylabel("Median ns / integration")
    axes[0].set_title("Runtime scalability")
    axes[1].set_yscale("log")
    axes[1].set_xlabel("Fixed RK4 steps on [0,1]")
    axes[1].set_ylabel("Maximum trajectory error")
    axes[1].set_title("Accuracy versus step count")
    for ax in axes:
        ax.grid(True, which="both", alpha=0.2)
        ax.legend(fontsize=7)
    fig.suptitle("Fixed-step classical RK4 scalability")
    save_figure(fig, fig_dir, "ode_scalability_and_accuracy")

    accuracy_cells = {}
    for row in accuracy:
        if row["status"] != "0":
            continue
        value = safe_float(row["median_relative_error"])
        if math.isfinite(value):
            accuracy_cells.setdefault((row["category"], row["library"]), []).append(value)
    matrix = np.full((len(CORE_CATEGORIES), 2), np.nan)
    annotations = np.full(matrix.shape, "N/A", dtype=object)
    for i, category in enumerate(CORE_CATEGORIES):
        for j, library in enumerate(LIBRARIES):
            values = accuracy_cells.get((category, library), [])
            if values:
                matrix[i, j] = math.log10(max(statistics.median(values), 1e-18))
                failed_count = sum(r["category"] == category and r["library"] == library and
                                   r["status"] == "0" and r["accuracy_pass"] == "0"
                                   for r in accuracy)
                annotations[i, j] = f"{statistics.median(values):.2g}\n(n={len(values)}, fail={failed_count})"
    fig, ax = plt.subplots(figsize=(5.8, 5.2))
    image = ax.imshow(np.ma.masked_invalid(matrix), aspect="auto", cmap="viridis_r")
    ax.set_xticks([0, 1], LIBRARIES)
    ax.set_yticks(np.arange(len(CORE_CATEGORIES)), CORE_CATEGORIES)
    ax.set_title("Median relative error by category (all solver-success outputs)")
    for i in range(len(CORE_CATEGORIES)):
        for j in range(2):
            ax.text(j, i, annotations[i, j], ha="center", va="center", color="white", fontsize=8)
    fig.colorbar(image, ax=ax, label="log10 median relative error")
    save_figure(fig, fig_dir, "category_accuracy_heatmap")

    reliability_cells = {(r["category"], r["library"]): r for r in reliability}
    matrix = np.full((len(CORE_CATEGORIES), 2), np.nan)
    labels = np.full(matrix.shape, "N/A", dtype=object)
    for i, category in enumerate(CORE_CATEGORIES):
        for j, library in enumerate(LIBRARIES):
            row = reliability_cells.get((category, library))
            if row and row["accuracy_pass_rate"] != "":
                value = float(row["accuracy_pass_rate"])
                matrix[i, j] = value
                labels[i, j] = (
                    f"accuracy {value:.0%}\n"
                    f"{row['accuracy_pass_records']}/{row['accuracy_applicable_records']} pass\n"
                    f"status {row['solver_success_records']}/{row['workload_method_phase_records']} ok"
                )
    fig, ax = plt.subplots(figsize=(5.8, 5.2))
    image = ax.imshow(np.ma.masked_invalid(matrix), aspect="auto", cmap="RdYlGn", vmin=0, vmax=1)
    ax.set_xticks([0, 1], LIBRARIES)
    ax.set_yticks(np.arange(len(CORE_CATEGORIES)), CORE_CATEGORIES)
    ax.set_title("Accuracy pass rate; cells also show solver-status success")
    for i in range(len(CORE_CATEGORIES)):
        for j in range(2):
            ax.text(j, i, labels[i, j], ha="center", va="center", fontsize=8)
    fig.colorbar(image, ax=ax, label="Accuracy-pass rate")
    save_figure(fig, fig_dir, "reliability_heatmap")

    memory_valid = [r for r in memory if r["category"] in CORE_CATEGORIES]
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.4))
    for ax, field, title in ((axes[0], "requested_heap_bytes", "Cumulative requested bytes"),
                             (axes[1], "peak_live_requested_heap_bytes", "Peak live requested bytes")):
        for i, category in enumerate(CORE_CATEGORIES):
            for offset, library in enumerate(LIBRARIES):
                values = [safe_float(r[field]) for r in memory_valid
                          if r["category"] == category and r["library"] == library and
                          safe_float(r[field]) >= 0]
                if values:
                    position = i * 3 + offset + 1
                    ax.boxplot(values, positions=[position], widths=0.65,
                               showfliers=False,
                               boxprops={"color": colors[library]},
                               medianprops={"color": colors[library]},
                               whiskerprops={"color": colors[library]},
                               capprops={"color": colors[library]})
        ax.set_xticks([i * 3 + 1.5 for i in range(len(CORE_CATEGORIES))], CORE_CATEGORIES,
                      rotation=25, ha="right")
        ax.set_yscale("symlog", linthresh=1)
        ax.set_ylabel("Bytes; wrapped heap calls (symlog, includes zero)")
        ax.set_title(title)
        ax.grid(True, axis="y", which="both", alpha=0.2)
        ax.plot([], [], color=colors["NUMERIX"], label="NUMERIX")
        ax.plot([], [], color=colors["GSL"], label="GSL")
        ax.legend(fontsize=8)
    ax = axes[2]
    for i, category in enumerate(CORE_CATEGORIES):
        for offset, library in enumerate(LIBRARIES):
            values = [safe_float(r["allocation_calls"]) for r in memory_valid
                      if r["category"] == category and r["library"] == library]
            if values:
                position = i * 3 + offset + 1
                ax.boxplot(values, positions=[position], widths=0.65,
                           showfliers=False,
                           boxprops={"color": colors[library]},
                           medianprops={"color": colors[library]},
                           whiskerprops={"color": colors[library]},
                           capprops={"color": colors[library]})
    ax.set_xticks([i * 3 + 1.5 for i in range(len(CORE_CATEGORIES))], CORE_CATEGORIES,
                  rotation=25, ha="right")
    ax.set_ylabel("Wrapped allocation calls")
    ax.set_title("Allocation call count")
    ax.grid(True, axis="y", alpha=0.2)
    ax.plot([], [], color=colors["NUMERIX"], label="NUMERIX")
    ax.plot([], [], color=colors["GSL"], label="GSL")
    ax.legend(fontsize=8)
    fig.suptitle("All attempted requested-heap metrics (static GSL linker wrapper)")
    save_figure(fig, fig_dir, "requested_heap_by_category")

    pareto_rows = [r for r in measurements if r["category"] == "root" and
                   r["case"] == "polynomial" and r["status"] == "0" and
                   r["accuracy_pass"] == "1"]
    fig, ax = plt.subplots(figsize=(7.2, 5.2))
    points = []
    for row in pareto_rows:
        point = (safe_float(row["median_ns"]), max(safe_float(row["relative_error"]), 1e-18))
        points.append((point[0], point[1], row["library"], row["method"]))
        ax.scatter(point[0], point[1], color=colors[row["library"]],
                   marker={"bisection": "o", "brent": "s", "newton": "^"}.get(row["method"], "x"),
                   s=55, label=f"{row['library']} {row['method']}")
    frontier = []
    for point in sorted(points):
        if not any(other[0] <= point[0] and other[1] <= point[1] and other != point
                   for other in points):
            frontier.append(point)
    frontier = sorted(set(frontier))
    if len(frontier) > 1:
        ax.plot([p[0] for p in frontier], [p[1] for p in frontier], color="black",
                linestyle="--", linewidth=1, label="Pareto frontier")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Median wall time (ns / operation)")
    ax.set_ylabel("Relative root error")
    ax.set_title("Case-specific runtime–accuracy Pareto view: x² − 2")
    handles, labels = ax.get_legend_handles_labels()
    unique = dict(zip(labels, handles))
    ax.legend(unique.values(), unique.keys(), fontsize=8)
    ax.grid(True, which="both", alpha=0.2)
    save_figure(fig, fig_dir, "pareto_root_polynomial")

    memory_by_pair = {(r["pair_id"], r["library"]): r for r in memory}
    pareto_memory_rows = [r for r in pareto_rows if r["method"] in {"bisection", "newton"}]
    fig, ax = plt.subplots(figsize=(7.2, 5.2))
    mem_points = []
    for row in pareto_memory_rows:
        mem = memory_by_pair.get((row["pair_id"], row["library"]))
        if not mem:
            continue
        t = safe_float(row["median_ns"])
        b = safe_float(mem["peak_live_requested_heap_bytes"])
        if t > 0 and b > 0:
            mem_points.append((t, b, row["library"], row["method"]))
            ax.scatter(t, b, color=colors[row["library"]],
                       marker={"bisection": "o", "newton": "^"}[row["method"]],
                       s=55, label=f"{row['library']} {row['method']}")
    mem_frontier = [p for p in mem_points if not any(
        q[0] <= p[0] and q[1] <= p[1] and q != p for q in mem_points)]
    if len(mem_frontier) > 1:
        mem_frontier.sort()
        ax.plot([p[0] for p in mem_frontier], [p[1] for p in mem_frontier],
                color="black", linestyle="--", linewidth=1, label="Pareto frontier")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Median wall time (ns / operation)")
    ax.set_ylabel("Peak live requested heap bytes")
    ax.set_title("Case-specific runtime–memory Pareto view: root x² − 2")
    handles, labels = ax.get_legend_handles_labels()
    unique = dict(zip(labels, handles))
    ax.legend(unique.values(), unique.keys(), fontsize=8)
    ax.grid(True, which="both", alpha=0.2)
    save_figure(fig, fig_dir, "pareto_root_runtime_memory")


def analyze(run_dir, plan_path, memory_path, out_dir):
    run_dir, out_dir = Path(run_dir), Path(out_dir)
    measurements = read_csv(run_dir / "measurements.csv")
    workloads = read_csv(run_dir / "workloads.csv")
    raw = read_csv(run_dir / "raw_timing.csv")
    accuracy_rows = read_csv(run_dir / "accuracy.csv")
    plan = read_csv(plan_path)
    memory_raw = read_csv(memory_path) if memory_path else []
    pairs, samples, _ = make_pairs(measurements, workloads, raw)
    performance = []
    for classification in ("DIRECT", "BEST_AVAILABLE", "APPROXIMATE"):
        performance.extend(summarize_category_performance(pairs, classification))
    algorithm_performance = summarize_algorithm_performance(pairs)
    accuracy = aggregate_accuracy(measurements, workloads, accuracy_rows)
    coverage = summarize_coverage(plan, workloads, pairs)
    reliability = summarize_reliability(measurements, workloads)
    memory = summarize_memory(memory_raw, measurements, workloads) if memory_raw else []
    accuracy_pairs, accuracy_summary = summarize_accuracy(measurements, workloads)
    memory_pairs, memory_summary = summarize_memory_comparisons(memory)

    tables = out_dir / "tables"
    write_csv(tables / "runtime_pairings.csv", pairs)
    write_csv(tables / "category_performance.csv", performance)
    write_csv(tables / "algorithm_performance.csv", algorithm_performance)
    write_csv(tables / "accuracy_by_workload.csv", accuracy)
    write_csv(tables / "accuracy_pairings.csv", accuracy_pairs)
    write_csv(tables / "category_accuracy.csv", accuracy_summary)
    write_csv(tables / "coverage.csv", coverage)
    write_csv(tables / "reliability.csv", reliability)
    write_csv(tables / "wins_losses_ties.csv", summarize_wins(pairs))
    write_csv(tables / "memory_by_workload.csv", memory)
    write_csv(tables / "memory_pairings.csv", memory_pairs)
    write_csv(tables / "category_memory.csv", memory_summary)
    make_figures(out_dir, measurements, raw, pairs, performance,
                 algorithm_performance, accuracy, reliability, memory)
    print(f"Analyzed {len(measurements)} workload summaries, {len(raw)} raw timing batches, "
          f"{len(accuracy_rows)} accuracy probes, {len(pairs)} paired workloads.")
    print(f"Wrote tables and figures under {out_dir}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-dir", type=Path, required=True)
    parser.add_argument("--plan", type=Path, required=True)
    parser.add_argument("--memory", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    analyze(args.run_dir, args.plan, args.memory, args.output_dir)


if __name__ == "__main__":
    main()
