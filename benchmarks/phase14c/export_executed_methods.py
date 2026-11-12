#!/usr/bin/env python3
"""Export only methods with recorded Phase 14C or historical run evidence."""
from __future__ import annotations

import csv
import hashlib
import json
import shutil
from collections import Counter, defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "benchmarks/results/phase14c/module_comparisons"
OUT = ROOT / "benchmarks/results/phase14c/executed_methods"
MODULES = ["root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode"]
COLORS = ["#3267a8", "#dc7748", "#3a8f72", "#9874b9"]
SCALAR_RUN = "phase14c-pilot-scalar-20261112T055820Z"
LA_RUN = "phase14c-la-pilot-20261112T055827Z"
OPT_RUN = "phase14c-opt-correctness-pilot-20261112"
ODE_RUN = "phase14c-ode-correctness-pilot-20261112T060326Z"
ROOT_RUN = "phase14b-root-20261112-v3"


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, rows: list[dict], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def raw_source(module: str, run_id: str) -> Path:
    if module == "root":
        return ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
    return BASE / "raw" / module / run_id


def copy_raw_evidence(summaries: dict[str, list[dict[str, str]]]) -> list[dict]:
    file_sets = {
        "root": [("raw_timing.csv", "raw_timing.csv"), ("accuracy.csv", "raw_accuracy.csv"),
                 ("method_workloads.csv", "method_workloads.csv"), ("run_metadata.json", "run_metadata.json")],
        "linear_algebra": [(n, n) for n in ("raw_timing.csv", "raw_accuracy.csv", "raw_memory.csv", "raw_setup_timing.csv",
                                             "per_problem_runtime_summary.csv", "environment.json", "verification.json", "workload_manifest.csv")],
        "differentiation": [(n, n) for n in ("raw_timing.csv", "raw_accuracy.csv", "raw_memory.csv", "run_metadata.json")],
        "interpolation": [(n, n) for n in ("raw_timing.csv", "raw_accuracy.csv", "raw_memory.csv", "run_metadata.json")],
        "integration": [(n, n) for n in ("raw_timing.csv", "raw_accuracy.csv", "raw_memory.csv", "run_metadata.json")],
        "optimization": [(n, n) for n in ("raw_accuracy.csv", "run_metadata.json", "workload_manifest.csv")],
        "ode": [(n, n) for n in ("raw_accuracy.csv", "run_metadata.json")],
    }
    index = []
    for module in MODULES:
        run_ids = sorted({row["source_run_id"] for row in summaries[module] if row.get("source_run_id")})
        expected_run = {
            "root": ROOT_RUN, "linear_algebra": LA_RUN,
            "differentiation": SCALAR_RUN, "interpolation": SCALAR_RUN,
            "integration": SCALAR_RUN, "optimization": OPT_RUN, "ode": ODE_RUN,
        }[module]
        if expected_run not in run_ids:
            raise SystemExit(f"Expected canonical run {expected_run} absent from {module} method summaries: {run_ids}")
        source = raw_source(module, expected_run)
        dest = OUT / "data/raw" / module / expected_run
        for src_name, dest_name in file_sets[module]:
            src = source / src_name
            if not src.is_file():
                continue
            dest.mkdir(parents=True, exist_ok=True)
            copied = dest / dest_name
            shutil.copy2(src, copied)
            source_hash = sha256(src)
            copied_hash = sha256(copied)
            if source_hash != copied_hash:
                raise SystemExit(f"Copied evidence hash mismatch: {src} -> {copied}")
            index.append({
                "module": module,
                "run_id": expected_run,
                "source_path": str(src.relative_to(ROOT)),
                "stored_path": str(copied.relative_to(OUT)),
                "sha256": copied_hash,
                "bytes": copied.stat().st_size,
            })
    return index


def executed_rows(summaries: dict[str, list[dict[str, str]]]) -> list[dict]:
    result = []
    for module in MODULES:
        for row in summaries[module]:
            timed = int(row.get("timed_problem_count") or 0)
            accuracy_attempts = int(row.get("accuracy_attempt_count") or 0)
            finite_errors = int(row.get("finite_error_count") or 0)
            if not (timed or accuracy_attempts or finite_errors):
                continue
            scope = row.get("timing_scope", "")
            historical = "historical" in row.get("evidence_scope", "").lower() or row.get("source_run_id", "").startswith("phase14b")
            if timed and accuracy_attempts:
                evidence_type = "historical_timing_and_accuracy" if historical else "timing_and_accuracy_pilot"
            elif timed:
                evidence_type = "historical_timing_only" if historical else "timing_pilot_accuracy_not_applicable"
            elif accuracy_attempts:
                evidence_type = "correctness_only_pilot"
            else:
                evidence_type = "trajectory_error_pilot_threshold_not_evaluated"
            route_dir = Path("data/raw") / module / row["source_run_id"]
            error_status = "not_evaluated" if finite_errors and not accuracy_attempts else ""
            result.append({
                "module": module,
                "operation": row["operation"],
                "method_key": row["method_key"],
                "library": row["library"],
                "comparison_type": row["comparison_type"],
                "run_id": row["source_run_id"],
                "evidence_period": "historical Phase 14B" if historical else "Phase 14C",
                "evidence_type": evidence_type,
                "timing_scope": scope,
                "timed_problem_count": timed,
                "timing_batch_count": int(row.get("timing_batch_count") or 0),
                "median_seconds_per_call": row.get("median_problem_median_seconds_per_call", ""),
                "accuracy_attempt_count": accuracy_attempts,
                "accuracy_status": error_status or ("evaluated" if accuracy_attempts else "not_applicable_or_no_reference"),
                "status_success_count": row.get("status_success_count", ""),
                "accuracy_pass_count": row.get("accuracy_pass_count", ""),
                "accuracy_pass_rate": row.get("accuracy_pass_rate", ""),
                "finite_error_count": finite_errors,
                "median_absolute_error": row.get("median_absolute_error", ""),
                "median_normalized_error": row.get("median_normalized_error", ""),
                "accuracy_metric": row.get("accuracy_metric", ""),
                "memory_status": "not measured at method scope",
                "evidence_scope": row.get("evidence_scope", ""),
                "stored_raw_directory": str(route_dir),
            })
    result.sort(key=lambda r: (MODULES.index(r["module"]), r["operation"], r["library"], r["timing_scope"]))
    return result


def dedup_method_library(rows: list[dict], module: str) -> list[dict]:
    unique = {}
    for row in rows:
        if row["module"] != module:
            continue
        unique.setdefault((row["method_key"], row["library"]), row)
    return list(unique.values())


def save_figure(fig, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path.with_suffix(".png"), dpi=190, bbox_inches="tight", facecolor="white")
    svg = path.with_suffix(".svg")
    fig.savefig(svg, bbox_inches="tight", facecolor="white")
    text = svg.read_text(encoding="utf-8")
    svg.write_text("\n".join(line.rstrip() for line in text.splitlines()) + "\n", encoding="utf-8")
    plt.close(fig)


def plot_module(module: str, rows: list[dict]) -> None:
    module_rows = [r for r in rows if r["module"] == module]
    timed = [r for r in module_rows if int(r["timed_problem_count"] or 0) > 0]
    categories = []
    category_map = {}
    for row in timed:
        key = (row["method_key"], row["timing_scope"])
        if key not in category_map:
            category_map[key] = len(categories)
            label = row["operation"]
            if row["timing_scope"] not in ("solver call including library workspace", "not_measured", "steady/setup"):
                label += f"\n{row['timing_scope']}"
            elif row["timing_scope"] == "steady/setup":
                label += "\nsteady/setup"
            categories.append((key, label))
    if timed:
        fig, ax = plt.subplots(figsize=(max(11, min(22, len(categories) * 1.25)), 7.0))
        x = np.arange(len(categories))
        libraries = sorted({r["library"] for r in timed}, key=lambda name: (name != "NUMERIX", name))
        width = min(0.72 / max(1, len(libraries)), 0.34)
        for li, library in enumerate(libraries):
            values = []
            for (method_key, scope), _ in categories:
                route = next((r for r in timed if r["method_key"] == method_key and r["timing_scope"] == scope and r["library"] == library), None)
                values.append(float(route["median_seconds_per_call"]) * 1e6 if route and route["median_seconds_per_call"] else float("nan"))
            offset = (li - (len(libraries) - 1) / 2) * width
            ax.bar(x + offset, values, width=width, color=COLORS[li % len(COLORS)], label=library)
        if any(v > 0 for container in ax.containers for v in container.datavalues if np.isfinite(v)):
            ax.set_yscale("log")
        ax.set_xticks(x, [label for _, label in categories], rotation=42, ha="right", fontsize=8)
        ax.set_ylabel("Median per-problem median time (µs/call)")
        ax.set_xlabel("Executed method and timing scope")
        ax.set_title(f"{module.replace('_', ' ').title()} — executed routes only")
        ax.grid(axis="y", which="both", alpha=0.22)
        ax.legend(frameon=False)
        fig.text(0.01, 0.01, "Only routes with recorded timing are shown. Exact problem and batch counts are in executed_method_routes.csv.", fontsize=8, color="#4d5968")
        fig.subplots_adjust(bottom=0.38, top=0.90)
        save_figure(fig, OUT / "figures" / module / f"{module}_runtime")
    else:
        not_measured_figure(module, "runtime", "No execution-time routes were recorded for this module.")

    accuracy_rows = dedup_method_library(module_rows, module)
    if module == "ode":
        plotted = [r for r in accuracy_rows if int(r["finite_error_count"] or 0) > 0 and r["median_normalized_error"]]
        if plotted:
            methods = list(dict.fromkeys(r["method_key"] for r in plotted))
            labels = [next(r["operation"] for r in plotted if r["method_key"] == key) for key in methods]
            libraries = sorted({r["library"] for r in plotted})
            fig, ax = plt.subplots(figsize=(max(11, len(methods) * 1.7), 7.0))
            x = np.arange(len(methods)); width = min(0.72 / len(libraries), 0.34)
            for li, library in enumerate(libraries):
                values = []
                for key in methods:
                    row = next((r for r in plotted if r["method_key"] == key and r["library"] == library), None)
                    values.append(max(1e-18, float(row["median_normalized_error"])) if row else float("nan"))
                offset = (li - (len(libraries) - 1) / 2) * width
                ax.bar(x + offset, values, width=width, color=COLORS[li % len(COLORS)], label=library)
            ax.set_yscale("log")
            ax.set_xticks(x, labels, rotation=32, ha="right")
            ax.set_ylabel("Normalized max trajectory error vs analytic solution")
            ax.set_xlabel("Executed fixed-step method")
            ax.set_title("ODE — executed correctness pilot; accuracy threshold not evaluated")
            fig.text(0.01, 0.01, "One two-state system, 21 output points; bars show analytic-reference error, not pass rate. NUMERIX and SUNDIALS tableaus matched; no time or memory measurements were taken.", fontsize=8, color="#4d5968")
            ax.grid(axis="y", which="both", alpha=0.22); ax.legend(frameon=False)
            fig.subplots_adjust(bottom=0.34, top=0.90)
            save_figure(fig, OUT / "figures" / module / f"{module}_accuracy")
        else:
            not_measured_figure(module, "accuracy", "No evaluated accuracy or analytic-error records were found.")
    else:
        evaluated = [r for r in accuracy_rows if int(r["accuracy_attempt_count"] or 0) > 0]
        if evaluated:
            methods = list(dict.fromkeys(r["method_key"] for r in evaluated))
            labels = [next(r["operation"] for r in evaluated if r["method_key"] == key) for key in methods]
            libraries = sorted({r["library"] for r in evaluated}, key=lambda name: (name != "NUMERIX", name))
            fig, ax = plt.subplots(figsize=(max(11, min(22, len(methods) * 1.45)), 7.0))
            x = np.arange(len(methods)); width = min(0.72 / len(libraries), 0.34)
            for li, library in enumerate(libraries):
                values = []; annotations = []
                for key in methods:
                    row = next((r for r in evaluated if r["method_key"] == key and r["library"] == library), None)
                    if row and int(row["accuracy_attempt_count"] or 0) > 0:
                        total = int(row["accuracy_attempt_count"]); passed = int(row["accuracy_pass_count"] or 0)
                        values.append(100.0 * passed / total); annotations.append(f"{passed}/{total}")
                    else:
                        values.append(float("nan")); annotations.append("")
                offset = (li - (len(libraries) - 1) / 2) * width
                bars = ax.bar(x + offset, values, width=width, color=COLORS[li % len(COLORS)], label=library)
                for bar, label in zip(bars, annotations):
                    if label:
                        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 2, label, ha="center", va="bottom", fontsize=7, rotation=90)
            ax.set_ylim(0, 125)
            ax.set_xticks(x, labels, rotation=42, ha="right", fontsize=8)
            ax.set_ylabel("Accuracy-target pass rate (%)")
            ax.set_xlabel("Executed method")
            ax.set_title(f"{module.replace('_', ' ').title()} — executed accuracy routes only")
            ax.grid(axis="y", alpha=0.22); ax.legend(frameon=False)
            ax.axhline(100, color="#677383", linewidth=0.8, linestyle="--", alpha=0.7)
            fig.text(0.01, 0.01, "Labels show pass/attempt count; failed statuses remain in the denominator. N/A setup records are not plotted.", fontsize=8, color="#4d5968")
            fig.subplots_adjust(bottom=0.38, top=0.90)
            save_figure(fig, OUT / "figures" / module / f"{module}_accuracy")
        else:
            not_measured_figure(module, "accuracy", "No evaluated accuracy records were found.")

    methods = list(dict.fromkeys(r["operation"] for r in module_rows))
    note = ", ".join(methods)
    not_measured_figure(module, "memory", "Method-specific memory was not measured for these runs.\n" + note)


def not_measured_figure(module: str, kind: str, message: str) -> None:
    fig, ax = plt.subplots(figsize=(11, 6.5))
    ax.set_title(f"{module.replace('_', ' ').title()} — executed-method {kind} view")
    ax.axis("off")
    ax.text(0.5, 0.62, "NOT MEASURED", transform=ax.transAxes, ha="center", va="center", fontsize=22, color="#687586", weight="bold")
    ax.text(0.5, 0.42, message, transform=ax.transAxes, ha="center", va="center", fontsize=10, color="#4d5968", wrap=True)
    save_figure(fig, OUT / "figures" / module / f"{module}_{kind}")


def build_readme(rows: list[dict], source_index: list[dict]) -> None:
    unique_ops = {m: list(dict.fromkeys(r["operation"] for r in rows if r["module"] == m)) for m in MODULES}
    unique_routes = {m: {(r["method_key"], r["library"]) for r in rows if r["module"] == m} for m in MODULES}
    timing_ops = {m: list(dict.fromkeys(r["operation"] for r in rows if r["module"] == m and int(r["timed_problem_count"] or 0))) for m in MODULES}
    accuracy_ops = {m: list(dict.fromkeys(r["operation"] for r in rows if r["module"] == m and (int(r["accuracy_attempt_count"] or 0) or int(r["finite_error_count"] or 0)))) for m in MODULES}
    out = [
        "# NUMERIX — Methods Run and Their Separate Figures", "",
        "> This directory contains only methods with recorded timing, accuracy, or analytic trajectory-error evidence. It includes historical root data from Phase 14B, clearly labeled, and bounded Phase 14C pilots. It is not a full seven-module benchmark.", "",
        f"**Coverage:** {sum(len(unique_ops[m]) for m in MODULES)} distinct operations/methods, {sum(len(unique_routes[m]) for m in MODULES)} method-library routes, {len(rows)} route/scope records, and {len(source_index)} copied raw/metadata files.", "",
        "## Run inventory", "",
        "| Module | Methods with records | Methods with timing | Methods with accuracy/error evidence | Evidence period |", "|---|---:|---:|---:|---|",
    ]
    for module in MODULES:
        period = "Historical Phase 14B" if module == "root" else "Phase 14C pilot"
        out.append(f"| {module.replace('_',' ').title()} | {len(unique_ops[module])} | {len(timing_ops[module])} | {len(accuracy_ops[module])} | {period} |")
    out.extend(["", "## Methods recorded", ""])
    for module in MODULES:
        ops = unique_ops[module]
        out.append(f"### {module.replace('_',' ').title()}")
        out.append("")
        if not ops:
            out.append("No method records found.")
            out.append("")
            continue
        out.append("Methods: " + "; ".join(ops) + ".")
        out.append("")
        out.append("- Timing evidence: " + ("; ".join(timing_ops[module]) if timing_ops[module] else "none"))
        out.append("- Accuracy/error evidence: " + ("; ".join(accuracy_ops[module]) if accuracy_ops[module] else "none"))
        out.append("- Method-level memory: not measured.")
        if module == "root":
            out.append("- Evidence is historical Phase 14B v3: 100 problems/method and 31 batches/route. GSL has direct routes for six single-root methods and a heuristic scan for multiple-root discovery; auto-selection and portfolio are NUMERIX-only.")
        elif module == "linear_algebra":
            out.append("- Phase 14C pilot: matrix addition, matrix multiplication, LU factorization, and LU solve; one input per method/library route, 31 batches.")
        elif module == "differentiation":
            out.append("- Phase 14C GSL adapter pilot: three problems/method, 31 batches/route. The second-derivative competitor is a finite-difference wrapper, not a native GSL second-derivative API; piecewise differentiation was not run.")
        elif module == "interpolation":
            out.append("- Phase 14C GSL adapter pilot: three problems/method, 31 batches/route. Newton construction has timing records but no numerical-output accuracy result; construction and evaluation are separate routes.")
        elif module == "integration":
            out.append("- Phase 14C GSL adapter pilot: three problems/method, 31 batches/route. Fixed GSL Gauss-Legendre and adaptive QAG comparisons are best available methods with different work policies; piecewise integration was not run.")
        elif module == "optimization":
            out.append("- Phase 14C correctness-only pilot: one analytic case/method; no timing. NLopt DIRECT-L returned MAXEVAL_REACHED on the golden-section case although the candidate passed the accuracy gate.")
        elif module == "ode":
            out.append("- Phase 14C correctness-only pilot: Euler, Heun, and RK4 against matching SUNDIALS tableaus on one two-state system and 21 output points. Error was computed, but no pass threshold was set.")
        out.append("")
    out.extend([
        "## Separate figures", "",
        "These charts omit methods with no corresponding run evidence. Runtime and accuracy are shown only when measured. Memory figures say not measured.", "",
        "[Three broad all-module graphs](combined/README.md) combine the executed time routes, accuracy evidence, and per-module memory status.", "",
        "| Module | Runtime | Memory | Accuracy / error |", "|---|---|---|---|",
    ])
    for module in MODULES:
        links = [f"[PNG](figures/{module}/{module}_{kind}.png) · [SVG](figures/{module}/{module}_{kind}.svg)" for kind in ("runtime", "memory", "accuracy")]
        out.append(f"| {module.replace('_',' ').title()} | {links[0]} | {links[1]} | {links[2]} |")
    out.extend([
        "", "## Files", "",
        "- [`executed_method_routes.csv`](executed_method_routes.csv) — one row per actual method/library/timing-scope record, with sample counts and evidence type.",
        "- [`source_files.csv`](source_files.csv) — copied raw files, original paths, and SHA-256 hashes.",
        "- [`verification.json`](verification.json) — exported method, route, copied-file, and figure counts.",
        "- `data/raw/<module>/<run_id>/` — selected run data only. Root files are historical Phase 14B; other folders are the canonical Phase 14C pilots.",
        "", "Raw files were copied, not rewritten. Full-study coverage and method-level memory remain incomplete.", "",
    ])
    (OUT / "README.md").write_text("\n".join(out), encoding="utf-8")


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    summaries = {module: read_csv(BASE / "analysis" / module / "method_summary.csv") for module in MODULES}
    rows = executed_rows(summaries)
    route_fields = [
        "module", "operation", "method_key", "library", "comparison_type", "run_id", "evidence_period", "evidence_type",
        "timing_scope", "timed_problem_count", "timing_batch_count", "median_seconds_per_call", "accuracy_attempt_count",
        "accuracy_status", "status_success_count", "accuracy_pass_count", "accuracy_pass_rate", "finite_error_count",
        "median_absolute_error", "median_normalized_error", "accuracy_metric", "memory_status", "evidence_scope", "stored_raw_directory",
    ]
    write_csv(OUT / "executed_method_routes.csv", rows, route_fields)
    source_index = copy_raw_evidence(summaries)
    write_csv(OUT / "source_files.csv", source_index, ["module", "run_id", "source_path", "stored_path", "sha256", "bytes"])
    for module in MODULES:
        plot_module(module, rows)
    build_readme(rows, source_index)
    unique_methods = {m: sorted({r["method_key"] for r in rows if r["module"] == m}) for m in MODULES}
    unique_routes = {m: sorted({(r["method_key"], r["library"]) for r in rows if r["module"] == m}) for m in MODULES}
    figures = list((OUT / "figures").rglob("*.png")) + list((OUT / "figures").rglob("*.svg"))
    verification = {
        "schema_version": "phase14c-executed-methods-v1",
        "output_directory": str(OUT.relative_to(ROOT)),
        "executed_method_route_scope_rows": len(rows),
        "unique_methods_by_module": {m: len(unique_methods[m]) for m in MODULES},
        "unique_methods": unique_methods,
        "unique_method_library_routes_by_module": {m: len(unique_routes[m]) for m in MODULES},
        "historical_root_is_labeled": all(r["evidence_period"] == "historical Phase 14B" for r in rows if r["module"] == "root"),
        "phase14c_pilots_are_labeled": all(r["evidence_period"] == "Phase 14C" for r in rows if r["module"] != "root"),
        "method_memory_statuses": sorted({r["memory_status"] for r in rows}),
        "copied_raw_and_metadata_files": len(source_index),
        "copied_source_hashes_match": all(
            sha256(ROOT / item["source_path"]) == item["sha256"]
            for item in source_index
        ),
        "figure_png_count": len([p for p in figures if p.suffix == ".png"]),
        "figure_svg_count": len([p for p in figures if p.suffix == ".svg"]),
        "figures_per_module": {m: len(list((OUT / "figures" / m).glob("*.png"))) == 3 and len(list((OUT / "figures" / m).glob("*.svg"))) == 3 for m in MODULES},
        "full_phase14c_study_complete": False,
    }
    (OUT / "verification.json").write_text(json.dumps(verification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(verification, indent=2))
    valid = (len(rows) > 0 and len(source_index) > 0 and verification["figure_png_count"] == 21
             and verification["figure_svg_count"] == 21 and all(verification["figures_per_module"].values())
             and verification["historical_root_is_labeled"] and verification["phase14c_pilots_are_labeled"]
             and verification["copied_source_hashes_match"])
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
