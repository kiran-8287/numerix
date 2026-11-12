#!/usr/bin/env python3
"""Draw three broad overview figures from the executed-method evidence only."""
from __future__ import annotations

import csv
import json
from collections import Counter
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Patch

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = ROOT / "benchmarks/results/phase14c/executed_methods"
OUT = PACKAGE / "combined"
ROUTES = PACKAGE / "executed_method_routes.csv"
MODULES = ["root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode"]
MODULE_LABELS = {
    "root": "Root", "linear_algebra": "Linear algebra", "differentiation": "Differentiation",
    "interpolation": "Interpolation", "integration": "Integration", "optimization": "Optimization", "ode": "ODE",
}
LIBRARY_COLORS = {
    "NUMERIX": "#3167a8", "GSL": "#df7848", "LAPACK/OpenBLAS": "#348b6d",
    "NLopt": "#9671b6", "SUNDIALS": "#bf5261",
}


def read_routes() -> list[dict[str, str]]:
    with ROUTES.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def write_csv(path: Path, rows: list[dict], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def save_figure(fig, name: str) -> list[Path]:
    OUT.mkdir(parents=True, exist_ok=True)
    png = OUT / f"{name}.png"
    svg = OUT / f"{name}.svg"
    fig.savefig(png, dpi=190, bbox_inches="tight", facecolor="white")
    fig.savefig(svg, bbox_inches="tight", facecolor="white")
    svg_text = svg.read_text(encoding="utf-8")
    svg.write_text("\n".join(line.rstrip() for line in svg_text.splitlines()) + "\n", encoding="utf-8")
    plt.close(fig)
    return [png, svg]


def runtime_figure(routes: list[dict[str, str]]) -> tuple[list[dict], list[Path]]:
    rows = [r for r in routes if int(r["timed_problem_count"] or 0) > 0 and r["median_seconds_per_call"]]
    rows.sort(key=lambda r: (MODULES.index(r["module"]), r["operation"], r["timing_scope"], r["library"]))
    write_csv(OUT / "combined_runtime_routes.csv", rows, list(routes[0].keys()))
    height = max(13, 0.285 * len(rows) + 2.5)
    fig, ax = plt.subplots(figsize=(17, height))
    y = np.arange(len(rows))
    values = [float(r["median_seconds_per_call"]) * 1e6 for r in rows]
    colors = [LIBRARY_COLORS.get(r["library"], "#737d89") for r in rows]
    ax.barh(y, values, color=colors, height=0.72)
    labels = []
    for r in rows:
        scope = r["timing_scope"]
        label = f"{MODULE_LABELS[r['module']]} | {r['operation']} | {r['library']}"
        if scope in {"steady/setup", "end-to-end"}:
            label += f" | {scope}"
        labels.append(label)
    ax.set_yticks(y, labels, fontsize=7.2)
    ax.invert_yaxis()
    ax.set_xscale("log")
    ax.set_xlabel("Median per-problem median runtime (µs/call; log scale)")
    ax.set_title("Runtime — all modules and all executed timing routes")
    ax.grid(axis="x", which="both", alpha=0.24)
    module_boundaries = []
    seen = None
    for i, row in enumerate(rows):
        if seen is not None and row["module"] != seen:
            module_boundaries.append(i - 0.5)
        seen = row["module"]
    for boundary in module_boundaries:
        ax.axhline(boundary, color="#7b8794", linewidth=0.8, alpha=0.8)
    libraries = list(dict.fromkeys(r["library"] for r in rows))
    handles = [Patch(color=LIBRARY_COLORS.get(lib, "#737d89"), label=lib) for lib in libraries]
    ax.legend(handles=handles, frameon=False, loc="lower right", ncol=1, fontsize=8)
    fig.text(
        0.01, 0.006,
        "Root data are historical Phase 14B; the other timed routes are Phase 14C pilots. Optimization and ODE have no timing runs. "
        "Different modules use different operations and workloads; this is an overview, not a cross-module speed ranking. Counts are in combined_runtime_routes.csv.",
        fontsize=8, color="#4d5968",
    )
    fig.subplots_adjust(left=0.39, bottom=0.045, top=0.96, right=0.98)
    return rows, save_figure(fig, "runtime_all_modules")


def memory_figure(routes: list[dict[str, str]]) -> tuple[list[dict], list[Path]]:
    ops = {module: sorted({r["operation"] for r in routes if r["module"] == module}) for module in MODULES}
    rows = [{
        "module": module,
        "executed_method_count": len(ops[module]),
        "executed_methods": "; ".join(ops[module]),
        "method_specific_memory_status": "not measured",
        "measured_bytes": "",
    } for module in MODULES]
    write_csv(OUT / "combined_memory_status.csv", rows, list(rows[0].keys()))

    fig, ax = plt.subplots(figsize=(13, 7.6))
    ax.axis("off")
    ax.set_title("Method-specific memory — all modules", fontsize=17, pad=24)
    table_rows = [[MODULE_LABELS[r["module"]], r["executed_method_count"], "NOT MEASURED"] for r in rows]
    table = ax.table(
        cellText=table_rows,
        colLabels=["Module", "Methods with run evidence", "Method-level memory"],
        colWidths=[0.28, 0.28, 0.40],
        cellLoc="center", colLoc="center", loc="center",
    )
    table.auto_set_font_size(False)
    table.set_fontsize(11)
    table.scale(1, 2.0)
    for (r, c), cell in table.get_celld().items():
        cell.set_edgecolor("#d5dde6")
        if r == 0:
            cell.set_facecolor("#eaf0f6")
            cell.set_text_props(weight="bold", color="#20354b")
        elif c == 2:
            cell.set_facecolor("#fff2ed")
            cell.set_text_props(weight="bold", color="#9b4338")
        else:
            cell.set_facecolor("#ffffff" if r % 2 else "#f6f8fa")
    fig.text(
        0.04, 0.08,
        "No method-specific memory measurements passed route-level verification for any module. Positive-control allocation sizes and process RSS are not algorithm memory results, so no byte values are plotted.",
        fontsize=9, color="#4d5968", wrap=True,
    )
    fig.subplots_adjust(top=0.90, bottom=0.20, left=0.05, right=0.95)
    return rows, save_figure(fig, "memory_all_modules")


def accuracy_figure(routes: list[dict[str, str]]) -> tuple[list[dict], list[dict], list[Path]]:
    # Integration has two timing scopes; deduplicate its identical accuracy records.
    unique = {}
    for r in routes:
        if int(r["accuracy_attempt_count"] or 0) > 0:
            unique.setdefault((r["module"], r["method_key"], r["library"]), r)
    pass_rows = list(unique.values())
    pass_rows.sort(key=lambda r: (MODULES.index(r["module"]), r["operation"], r["library"]))
    write_csv(OUT / "combined_accuracy_routes.csv", pass_rows, list(routes[0].keys()))

    ode_unique = {}
    for r in routes:
        if r["module"] == "ode" and int(r["finite_error_count"] or 0) > 0 and r["median_normalized_error"]:
            ode_unique.setdefault((r["method_key"], r["library"]), r)
    ode_rows = list(ode_unique.values())
    ode_rows.sort(key=lambda r: (r["operation"], r["library"]))
    fig_height = max(15, 0.29 * len(pass_rows) + 5)
    fig = plt.figure(figsize=(18, fig_height))
    grid = fig.add_gridspec(2, 1, height_ratios=[max(1, len(pass_rows)), 4], hspace=0.28)
    ax = fig.add_subplot(grid[0])
    y = np.arange(len(pass_rows))
    rates = [100 * int(r["accuracy_pass_count"] or 0) / int(r["accuracy_attempt_count"]) for r in pass_rows]
    colors = [LIBRARY_COLORS.get(r["library"], "#737d89") for r in pass_rows]
    ax.barh(y, rates, color=colors, height=0.72)
    labels = [f"{MODULE_LABELS[r['module']]} | {r['operation']} | {r['library']}" for r in pass_rows]
    ax.set_yticks(y, labels, fontsize=6.8)
    ax.invert_yaxis()
    ax.set_xlim(0, 124)
    ax.set_xlabel("Accuracy-target pass rate (%)")
    ax.set_title("Evaluated accuracy gates — pass rate by executed method/library route")
    ax.axvline(100, color="#596575", linestyle="--", linewidth=0.9)
    ax.grid(axis="x", alpha=0.22)
    for i, r in enumerate(pass_rows):
        passed = int(r["accuracy_pass_count"] or 0)
        attempted = int(r["accuracy_attempt_count"] or 0)
        rate = rates[i]
        ax.text(min(101 + max(0, rate - 100), 108), i, f"{passed}/{attempted}", va="center", fontsize=6.2, color="#293746")

    libs = list(dict.fromkeys(r["library"] for r in pass_rows))
    ax.legend(
        handles=[Patch(color=LIBRARY_COLORS.get(lib, "#737d89"), label=lib) for lib in libs],
        frameon=False, loc="lower right", ncol=3, fontsize=7,
    )
    boundaries = []
    previous = None
    for i, r in enumerate(pass_rows):
        if previous is not None and r["module"] != previous:
            boundaries.append(i - 0.5)
        previous = r["module"]
    for boundary in boundaries:
        ax.axhline(boundary, color="#7b8794", linewidth=0.7, alpha=0.75)

    ax_ode = fig.add_subplot(grid[1])
    methods = list(dict.fromkeys(r["method_key"] for r in ode_rows))
    method_labels = [next(r["operation"] for r in ode_rows if r["method_key"] == key) for key in methods]
    libraries = list(dict.fromkeys(r["library"] for r in ode_rows))
    yy = np.arange(len(methods)); width = min(0.72 / max(1, len(libraries)), 0.34)
    for li, library in enumerate(libraries):
        values = []
        for key in methods:
            row = next((r for r in ode_rows if r["method_key"] == key and r["library"] == library), None)
            values.append(float(row["median_normalized_error"]) if row else float("nan"))
        offset = (li - (len(libraries) - 1) / 2) * width
        ax_ode.barh(yy + offset, values, height=width, color=LIBRARY_COLORS.get(library, "#737d89"), label=library)
    ax_ode.set_xscale("log")
    ax_ode.set_yticks(yy, method_labels, fontsize=8)
    ax_ode.set_xlabel("Normalized maximum trajectory error (log scale)")
    ax_ode.set_title("ODE analytic-reference error — threshold not set, so no pass rate")
    ax_ode.grid(axis="x", which="both", alpha=0.22)
    ax_ode.legend(frameon=False, loc="lower right", fontsize=8)

    fig.suptitle("Accuracy overview — all modules with executed evidence", fontsize=17, y=0.995)
    fig.text(
        0.01, 0.005,
        "Pass denominators include failed statuses; optimization accuracy pass does not imply convergence. ODE errors use one two-state, 21-point pilot with no threshold. "
        "Newton construction has no numerical-output accuracy result. Per-module targets differ, so rates are diagnostic rather than a cross-module ranking. Root records are historical Phase 14B.",
        fontsize=8, color="#4d5968",
    )
    fig.subplots_adjust(left=0.37, right=0.98, top=0.965, bottom=0.035)
    return pass_rows, ode_rows, save_figure(fig, "accuracy_all_modules")


def main() -> int:
    routes = read_routes()
    runtime_rows, runtime_images = runtime_figure(routes)
    memory_rows, memory_images = memory_figure(routes)
    accuracy_rows, ode_rows, accuracy_images = accuracy_figure(routes)
    files = runtime_images + memory_images + accuracy_images
    verification = {
        "schema_version": "phase14c-combined-module-figures-v1",
        "figures": {
            "runtime": [str(p.relative_to(PACKAGE)) for p in runtime_images],
            "memory": [str(p.relative_to(PACKAGE)) for p in memory_images],
            "accuracy": [str(p.relative_to(PACKAGE)) for p in accuracy_images],
        },
        "runtime_route_count": len(runtime_rows),
        "accuracy_gate_route_count": len(accuracy_rows),
        "ode_error_route_count": len(ode_rows),
        "memory_module_count": len(memory_rows),
        "all_memory_status_not_measured": all(r["method_specific_memory_status"] == "not measured" for r in memory_rows),
        "root_is_historical": all(r["evidence_period"] == "historical Phase 14B" for r in runtime_rows + accuracy_rows if r["module"] == "root"),
        "optimization_ode_have_no_timing": not any(r["module"] in {"optimization", "ode"} for r in runtime_rows),
        "combined_png_count": sum(p.suffix == ".png" for p in files),
        "combined_svg_count": sum(p.suffix == ".svg" for p in files),
        "full_study_complete": False,
    }
    (OUT / "verification.json").write_text(json.dumps(verification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    write_readme(verification)
    print(json.dumps(verification, indent=2))
    valid = (
        len(runtime_rows) > 0 and len(accuracy_rows) > 0 and len(ode_rows) == 6
        and verification["memory_module_count"] == 7
        and verification["all_memory_status_not_measured"]
        and verification["root_is_historical"]
        and verification["optimization_ode_have_no_timing"]
        and verification["combined_png_count"] == 3 and verification["combined_svg_count"] == 3
    )
    return 0 if valid else 1


def write_readme(verification: dict) -> None:
    content = f"""# All-module combined figures

These three figures bring together only the methods with recorded evidence.

- [Combined runtime — PNG](runtime_all_modules.png) · [SVG](runtime_all_modules.svg): {verification['runtime_route_count']} timed routes. Root data are historical Phase 14B; other timings are Phase 14C pilots. Optimization and ODE have no timing runs. Different operations and workloads prevent a cross-module speed ranking.
- [Combined memory status — PNG](memory_all_modules.png) · [SVG](memory_all_modules.svg): all seven modules are explicitly marked **not measured** for method-specific memory. No byte values are fabricated from smoke controls or RSS.
- [Combined accuracy overview — PNG](accuracy_all_modules.png) · [SVG](accuracy_all_modules.svg): evaluated target pass rates are shown by route; the ODE panel uses normalized analytic trajectory error because no pass threshold was set. Setup-only interpolation construction has no numerical-output accuracy value.

Supporting route data:

- [`combined_runtime_routes.csv`](combined_runtime_routes.csv)
- [`combined_accuracy_routes.csv`](combined_accuracy_routes.csv)
- [`combined_memory_status.csv`](combined_memory_status.csv)
- [`verification.json`](verification.json)

Accuracy targets differ by module, so the combined plot is an evidence overview, not a ranking across modules. See the [executed-method inventory](../README.md) for the exact methods, run IDs, sample counts, and copied raw data.
"""
    (OUT / "README.md").write_text(content, encoding="utf-8")


if __name__ == "__main__":
    raise SystemExit(main())
