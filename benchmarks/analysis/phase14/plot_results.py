#!/usr/bin/env python3
"""Generate reproducible Phase 14 overview and module figures."""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "benchmarks/problems/phase14/workloads.csv"
MODULES = ("root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode")
COLORS = {
    "NUMERIX": "#0072B2",
    "GSL": "#E69F00",
    "CMINPACK": "#CC79A7",
    "LAPACK_REFERENCE": "#009E73",
    "LAPACK_OPENBLAS": "#D55E00",
    "NLOPT": "#56B4E9",
    "SUNDIALS_ARKODE": "#8C6D31",
    "SUNDIALS_CVODE": "#7F3C8D",
}
LABELS = {
    "NUMERIX": "NUMERIX",
    "GSL": "GSL 2.8",
    "CMINPACK": "CMINPACK",
    "LAPACK_REFERENCE": "Reference LAPACK",
    "LAPACK_OPENBLAS": "LAPACK + OpenBLAS",
    "NLOPT": "NLopt DIRECT-L",
    "SUNDIALS_ARKODE": "SUNDIALS ARKODE RK4",
    "SUNDIALS_CVODE": "SUNDIALS CVODE",
}


def read_csv(path: Path) -> list[dict[str, str]]:
    csv.field_size_limit(sys.maxsize)
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def number(value: str | None) -> float | None:
    try:
        result = float(value) if value not in (None, "") else math.nan
    except (TypeError, ValueError):
        return None
    return result if math.isfinite(result) else None


def save(fig: plt.Figure, directory: Path, name: str) -> None:
    fig.tight_layout()
    fig.savefig(directory / f"{name}.png", dpi=200, bbox_inches="tight")
    svg_path = directory / f"{name}.svg"
    fig.savefig(svg_path, bbox_inches="tight")
    svg_text = svg_path.read_text(encoding="utf-8")
    svg_path.write_text("".join(line.rstrip() + "\n" for line in svg_text.splitlines()), encoding="utf-8")
    plt.close(fig)


def legend_handles(libraries: list[str]):
    return [plt.Line2D([0], [0], color=COLORS.get(lib, "#333333"), marker="o",
                       linestyle="-", linewidth=2, label=LABELS.get(lib, lib))
            for lib in libraries]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", help="Phase 14 run directory")
    args = parser.parse_args()
    run_dir = (ROOT / args.run_dir).resolve()
    figure_dir = run_dir / "figures"
    figure_dir.mkdir(parents=True, exist_ok=True)
    coverage = read_csv(run_dir / "coverage.csv")
    comparisons = read_csv(run_dir / "comparisons.csv")
    accuracy = read_csv(run_dir / "accuracy.csv")
    timing = read_csv(run_dir / "raw_timing.csv")
    work = read_csv(run_dir / "work_metrics.csv")
    memory = read_csv(run_dir / "memory.csv")
    problems = {row["problem_id"]: row for row in read_csv(MANIFEST)}
    inputs = {}
    with (ROOT / "benchmarks/problems/phase14/problem_inputs.jsonl").open(encoding="utf-8") as stream:
        for line in stream:
            item = json.loads(line)
            inputs[item["problem_id"]] = item["input_parameters"]

    by_module_lib = {(row["module"], row["library"]): row for row in coverage}
    libraries_by_module: dict[str, list[str]] = defaultdict(list)
    for row in coverage:
        libraries_by_module[row["module"]].append(row["library"])
    acc_index = {(row["module"], row["library"], row["problem_id"]): row for row in accuracy}
    work_index: dict[tuple[str, str, str], dict[str, float | None]] = defaultdict(dict)
    for row in work:
        if row.get("value_status") == "measured":
            work_index[(row["module"], row["library"], row["problem_id"])][row["metric"]] = number(row.get("value"))
    timing_by_problem: dict[tuple[str, str, str], list[float]] = defaultdict(list)
    for row in timing:
        elapsed = number(row.get("wall_elapsed_seconds"))
        batch_size = number(row.get("batch_size"))
        if elapsed is not None and batch_size:
            timing_by_problem[(row["module"], row["library"], row["problem_id"])].append(elapsed / batch_size)
    medians = {key: float(np.median(values)) for key, values in timing_by_problem.items()}
    memory_by_pair = {
        (row["module"], row["library"], row["problem_id"]): number(row.get("peak_rss_kb"))
        for row in memory
    }
    plt.rcParams.update({
        "font.family": "DejaVu Sans", "font.size": 9,
        "axes.titlesize": 11, "axes.labelsize": 9,
        "legend.fontsize": 8, "figure.titlesize": 15,
        "svg.fonttype": "none", "axes.spines.top": False,
        "axes.spines.right": False, "axes.grid": True,
        "grid.alpha": 0.23, "grid.linestyle": ":",
    })

    # 1. Coverage: counts are annotations; missing cells are visibly empty.
    libs = sorted({row["library"] for row in coverage})
    cover = np.full((len(MODULES), len(libs)), np.nan)
    annotations = [["" for _ in libs] for _ in MODULES]
    for i, module in enumerate(MODULES):
        for j, lib in enumerate(libs):
            row = by_module_lib.get((module, lib))
            if row:
                cover[i, j] = float(row["accuracy_observations"])
                annotations[i][j] = f"{row['accuracy_observations']} attempted\n{row['accuracy_pass_count']} accurate"
    fig, ax = plt.subplots(figsize=(12, 5.3))
    image = ax.imshow(np.ma.masked_invalid(cover), cmap="Blues", vmin=0, vmax=100, aspect="auto")
    ax.set_xticks(range(len(libs)), [LABELS.get(x, x) for x in libs], rotation=28, ha="right")
    ax.set_yticks(range(len(MODULES)), [m.replace("_", " ").title() for m in MODULES])
    ax.set_title("Phase 14 paired benchmark coverage")
    ax.set_xlabel("Library configuration")
    ax.set_ylabel("NUMERIX module")
    for i in range(len(MODULES)):
        for j in range(len(libs)):
            if annotations[i][j]: ax.text(j, i, annotations[i][j], ha="center", va="center", fontsize=7)
    fig.colorbar(image, ax=ax, label="Accuracy-probe records (out of 100)")
    save(fig, figure_dir, "overview_01_coverage")

    # 2. Status, convergence, and accuracy pass rates remain separate.
    fig, axes = plt.subplots(1, 3, figsize=(17, 5.5), sharey=True)
    measures = [("status_success_rate_percent", "Status success (%)"),
                ("convergence_rate_percent", "Converged (%)"),
                ("accuracy_pass_rate_percent", "Accuracy pass (%)")]
    for ax, (field, title) in zip(axes, measures):
        x = np.arange(len(MODULES))
        width = 0.82 / max(1, max(len(libraries_by_module[m]) for m in MODULES))
        for module_i, module in enumerate(MODULES):
            choices = sorted(libraries_by_module[module])
            for k, lib in enumerate(choices):
                row = by_module_lib[(module, lib)]
                value = float(row[field]) if row[field] != "NA" else np.nan
                offset = (k - (len(choices)-1)/2) * width
                ax.bar(module_i+offset, value, width=width*0.92,
                       color=COLORS.get(lib, "#444444"), label=LABELS.get(lib, lib) if module_i == 0 else None)
        ax.set_title(title)
        ax.set_ylim(0, 105)
        ax.set_xticks(x, [m.replace("_", "\n") for m in MODULES], rotation=26, ha="right", fontsize=7)
        ax.set_ylabel("Problems (%)")
    handles = [plt.Rectangle((0,0),1,1,color=COLORS[lib],label=LABELS[lib]) for lib in libs]
    if handles: fig.legend(handles, [h.get_label() for h in handles], loc="upper center", ncol=4, bbox_to_anchor=(0.5, 1.04))
    fig.suptitle("Reliability and numerical accuracy are distinct outcomes", y=1.10)
    save(fig, figure_dir, "overview_02_status_convergence_accuracy")

    # 3. Reliability count plot with status failure and non-convergence side by side.
    fig, axes = plt.subplots(1, 2, figsize=(14, 5.5))
    for ax, field, title in [(axes[0], "status_success_count", "Successful status count"),
                             (axes[1], "converged_count", "Converged count")]:
        ypos = 0
        ticks, labels = [], []
        for module in MODULES:
            for lib in sorted(libraries_by_module[module]):
                row = by_module_lib[(module, lib)]
                value = int(row[field])
                ax.barh(ypos, value, color=COLORS.get(lib, "#444444"))
                ticks.append(ypos); labels.append(f"{module.replace('_',' ')} · {LABELS.get(lib, lib)}")
                ypos += 1
            ypos += 0.4
        ax.set_yticks(ticks, labels, fontsize=7)
        ax.set_xlim(0, 105); ax.set_xlabel("Workloads (out of 100)"); ax.set_title(title)
        ax.invert_yaxis()
    fig.suptitle("Solver status and convergence by module and library")
    save(fig, figure_dir, "overview_03_reliability")

    # 4. Paired speedup forest plot. Ratios above one favor NUMERIX.
    ordered = sorted(comparisons, key=lambda r: (MODULES.index(r["module"]), r["competitor"]))
    fig, ax = plt.subplots(figsize=(11.5, max(5.5, 0.44*len(ordered))))
    y = np.arange(len(ordered))
    for i, row in enumerate(ordered):
        med = number(row["median_paired_speedup"])
        low = number(row["median_speedup_ci95_low"])
        high = number(row["median_speedup_ci95_high"])
        if med is not None and low is not None and high is not None:
            ax.errorbar(med, i, xerr=[[med-low], [high-med]], fmt="o",
                        color=COLORS.get(row["competitor"], "#333333"), capsize=3)
            ax.text(high*1.06, i,
                    f"n={row['performance_qualified_pairs']}; accuracy-pass={row['accuracy_qualified_pairs']} · {row['comparison_mode'].replace('_',' ')}",
                    va="center", fontsize=7)
        else:
            ax.text(1.04, i,
                    f"no valid speed estimate (accuracy-pass={row['accuracy_qualified_pairs']}; status+accuracy={row['performance_qualified_pairs']})",
                    va="center", fontsize=7, color="#444444")
    ax.axvline(1.0, color="#222222", linestyle="--", linewidth=1.2, label="Equal median time")
    ax.set_xscale("log")
    ax.set_yticks(y, [f"{r['module'].replace('_',' ').title()} — {LABELS.get(r['competitor'],r['competitor'])}" for r in ordered])
    ax.invert_yaxis()
    ax.set_xlabel("Paired speedup = competitor time / NUMERIX time (greater than 1 favors NUMERIX)")
    ax.set_title("Status and accuracy-qualified paired speedups with 95% problem-bootstrap intervals")
    save(fig, figure_dir, "overview_04_paired_speedup")

    # 5. Runtime scalability by module; family categories are used when scalar size is fixed.
    fig, axes = plt.subplots(2, 4, figsize=(17, 8.5))
    axes = axes.flatten()
    for idx, module in enumerate(MODULES):
        ax = axes[idx]
        selected = [p for p in problems.values() if p["module"] == module]
        has_size = len({int(p["input_size"]) for p in selected}) > 1
        if has_size:
            for lib in sorted(libraries_by_module[module]):
                points = defaultdict(list)
                for problem in selected:
                    key = (module, lib, problem["problem_id"])
                    if key in medians: points[int(problem["input_size"])].append(medians[key])
                xs = sorted(points)
                ys = [statistics_median(points[x]) for x in xs]
                ax.plot(xs, ys, marker="o", markersize=3, color=COLORS.get(lib), label=LABELS.get(lib,lib))
            ax.set_xlabel("Problem size (dimension, nodes, or steps)")
        else:
            groups = defaultdict(list)
            for problem in selected:
                for lib in libraries_by_module[module]:
                    key=(module,lib,problem["problem_id"])
                    if key in medians: groups[lib].append(medians[key])
            for lib, vals in groups.items():
                ax.scatter(np.arange(len(vals)), vals, s=9, alpha=.55, color=COLORS.get(lib), label=LABELS.get(lib,lib))
            ax.set_xlabel("Independent workload index (scalar size fixed)")
        ax.set_yscale("log"); ax.set_ylabel("Median wall time per solve (s)")
        ax.set_title(module.replace("_", " ").title())
    axes[-1].axis("off")
    handles = [plt.Line2D([0],[0],color=COLORS[lib],marker="o",label=LABELS[lib]) for lib in libs]
    fig.legend(handles, [h.get_label() for h in handles], loc="lower right", bbox_to_anchor=(0.96, 0.02), ncol=2)
    fig.suptitle("Runtime scalability by module; scalar workloads have no varying size axis")
    save(fig, figure_dir, "overview_05_runtime_scalability")

    # 6. Peak resident memory; baseline RSS is retained and visibly dominates.
    fig, axes = plt.subplots(2, 4, figsize=(17, 8.5))
    axes = axes.flatten()
    for idx, module in enumerate(MODULES):
        ax=axes[idx]
        selected=[p for p in problems.values() if p["module"]==module]
        for lib in sorted(libraries_by_module[module]):
            by_size=defaultdict(list)
            for problem in selected:
                rss=memory_by_pair.get((module,lib,problem["problem_id"]))
                if rss is not None: by_size[int(problem["input_size"])].append(rss)
            xs=sorted(by_size)
            if xs: ax.plot(xs,[np.median(by_size[x]) for x in xs],marker="o",markersize=3,color=COLORS.get(lib),label=LABELS.get(lib,lib))
        ax.set_title(module.replace("_"," ").title())
        ax.set_xlabel("Problem size (module-specific)");ax.set_ylabel("Peak process RSS (KiB)")
    axes[-1].axis("off")
    fig.legend([plt.Line2D([0],[0],color=COLORS[lib],marker="o",label=LABELS[lib]) for lib in libs],
               [LABELS[lib] for lib in libs],loc="lower right",bbox_to_anchor=(0.96,0.02),ncol=2)
    fig.suptitle("Isolated-process peak RSS; shared-library and harness baseline is included")
    save(fig,figure_dir,"overview_06_memory_scalability")

    # 7. Error distributions; log axis is clamped at the smallest displayed positive value.
    fig, axes = plt.subplots(2, 4, figsize=(17, 9))
    axes=axes.flatten()
    for idx,module in enumerate(MODULES):
        ax=axes[idx]
        libs_for=sorted(libraries_by_module[module])
        vals_by_lib=[]
        for lib in libs_for:
            vals=[number(r["absolute_error"]) for r in accuracy if r["module"]==module and r["library"]==lib]
            vals_by_lib.append([max(v,1e-30) for v in vals if v is not None])
        if any(vals_by_lib):
            bp=ax.boxplot(vals_by_lib,showfliers=False,patch_artist=True)
            for box,lib in zip(bp["boxes"],libs_for):box.set_facecolor(COLORS.get(lib,"#777777"));box.set_alpha(.75)
        ax.set_yscale("log");ax.set_xticks(range(1,len(libs_for)+1),[LABELS.get(l,l) for l in libs_for],rotation=30,ha="right",fontsize=7)
        ax.set_title(module.replace("_"," ").title());ax.set_ylabel("Primary absolute error (module-specific units)")
    axes[-1].axis("off")
    fig.suptitle("Absolute error distributions; each module has its own error definition")
    save(fig,figure_dir,"overview_07_accuracy_distribution")

    # 8. Time-to-accuracy trade-off on the saved problem-level values.
    fig, axes=plt.subplots(2,4,figsize=(17,9));axes=axes.flatten()
    for idx,module in enumerate(MODULES):
        ax=axes[idx]
        for lib in sorted(libraries_by_module[module]):
            xs=[];ys=[]
            for r in accuracy:
                if r["module"]!=module or r["library"]!=lib or r["accuracy_pass"]!="1":continue
                key=(module,lib,r["problem_id"])
                t=medians.get(key);e=number(r["absolute_error"])
                if t and e is not None:xs.append(t);ys.append(max(e,1e-30))
            ax.scatter(xs,ys,s=12,alpha=.55,color=COLORS.get(lib),label=LABELS.get(lib,lib))
        ax.set_xscale("log");ax.set_yscale("log");ax.set_title(module.replace("_"," ").title())
        ax.set_xlabel("Median wall time per solve (s)");ax.set_ylabel("Actual absolute error")
    axes[-1].axis("off")
    fig.suptitle("Runtime versus actual error for accuracy-passing workloads")
    handles=[plt.Line2D([0],[0],color=COLORS[lib],marker="o",label=LABELS[lib]) for lib in libs]
    fig.legend(handles,[h.get_label() for h in handles],loc="lower right",bbox_to_anchor=(.96,.02),ncol=2)
    save(fig,figure_dir,"overview_08_time_to_accuracy")

    # Module-specific work/correctness views.
    module_specs = {
        "root": ("function_evaluations", "Root location error", "Absolute root-location error"),
        "differentiation": ("function_evaluations", "Derivative error", "Absolute first-derivative error"),
        "integration": ("function_evaluations", "Integral error", "Absolute integration error"),
        "optimization": ("function_evaluations", "Optimality gap", "Absolute objective-value gap"),
        "ode": ("rhs_evaluations", "Final-state error", "Final-state infinity error"),
    }
    for module,(work_metric,chart_title,error_label) in module_specs.items():
        fig,ax=plt.subplots(figsize=(8.5,5.5))
        for lib in sorted(libraries_by_module[module]):
            xs=[];ys=[]
            for row in accuracy:
                if row["module"]!=module or row["library"]!=lib:continue
                w=work_index.get((module,lib,row["problem_id"]),{}).get(work_metric)
                e=number(row["absolute_error"])
                if w and e is not None: xs.append(w);ys.append(max(e,1e-30))
            ax.scatter(xs,ys,s=18,alpha=.62,color=COLORS.get(lib),label=LABELS.get(lib,lib))
        ax.set_xscale("log");ax.set_yscale("log");ax.set_xlabel(work_metric.replace("_"," ").title())
        ax.set_ylabel(error_label);ax.set_title(f"{module.title()}: {chart_title} versus measured work")
        ax.legend(fontsize=7,ncol=2)
        save(fig,figure_dir,f"{module}_error_vs_work")

    # Linear solve forward error against conditioning and residual error.
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    for lib in sorted(libraries_by_module["linear_algebra"]):
        cond_x=[]; forward_y=[]; residual_y=[]
        for row in accuracy:
            if row["module"]!="linear_algebra" or row["library"]!=lib: continue
            cond=number(str(inputs[row["problem_id"]].get("condition_number_reference", "")))
            forward=number(row["absolute_error"])
            residual=number(row["secondary_error"])
            if cond is not None and cond>0 and forward is not None:
                cond_x.append(cond);forward_y.append(max(forward,1e-30))
            if residual is not None and residual>=0: residual_y.append(max(residual,1e-30))
        axes[0].scatter(cond_x,forward_y,s=17,alpha=.62,color=COLORS.get(lib),label=LABELS.get(lib,lib))
        if residual_y:
            axes[1].boxplot(residual_y,positions=[len(axes[1].containers)+1],showfliers=False,
                            patch_artist=True,boxprops={"facecolor":COLORS.get(lib,"#888888"),"alpha":.65})
    axes[0].set_xscale("log");axes[0].set_yscale("log")
    axes[0].set_xlabel("Finite reference condition number");axes[0].set_ylabel("Forward solution infinity error")
    axes[0].set_title("Forward error by conditioning")
    axes[1].clear()
    for i,lib in enumerate(sorted(libraries_by_module["linear_algebra"])):
        vals=[max(number(r["secondary_error"]),1e-30) for r in accuracy if r["module"]=="linear_algebra" and r["library"]==lib and number(r["secondary_error"]) is not None]
        if vals: axes[1].boxplot(vals,positions=[i+1],showfliers=False,patch_artist=True,
            boxprops={"facecolor":COLORS.get(lib,"#888888"),"alpha":.7})
    la_libs=sorted(libraries_by_module["linear_algebra"])
    axes[1].set_yscale("log");axes[1].set_xticks(range(1,len(la_libs)+1),[LABELS.get(l,l) for l in la_libs],rotation=25,ha="right",fontsize=7)
    axes[1].set_ylabel("Residual infinity norm");axes[1].set_title("Residual distribution")
    axes[0].legend(fontsize=7,ncol=2)
    fig.suptitle("Linear-solve accuracy and conditioning; singular cases remain in reliability counts")
    save(fig,figure_dir,"linear_algebra_conditioning_and_residual")

    # Interpolation maximum and RMS error versus node count.
    fig,axes=plt.subplots(1,2,figsize=(12,4.8))
    for lib in sorted(libraries_by_module["interpolation"]):
        max_by_n=defaultdict(list);rms_by_n=defaultdict(list)
        for row in accuracy:
            if row["module"]!="interpolation" or row["library"]!=lib:continue
            n=int(problems[row["problem_id"]]["input_size"])
            e=number(row["absolute_error"]);rms=number(row["secondary_error"])
            if e is not None:max_by_n[n].append(max(e,1e-30))
            if rms is not None:rms_by_n[n].append(max(rms,1e-30))
        xs=sorted(max_by_n);axes[0].plot(xs,[np.median(max_by_n[x]) for x in xs],marker="o",color=COLORS.get(lib),label=LABELS.get(lib,lib))
        xs=sorted(rms_by_n);axes[1].plot(xs,[np.median(rms_by_n[x]) for x in xs],marker="o",color=COLORS.get(lib),label=LABELS.get(lib,lib))
    for ax,title in zip(axes,["Maximum validation-grid error","RMS validation-grid error"]):
        ax.set_yscale("log");ax.set_xlabel("Interpolation node count");ax.set_ylabel("Absolute interpolation error");ax.set_title(title);ax.legend(fontsize=8)
    fig.suptitle("Global-polynomial interpolation accuracy versus node count")
    save(fig,figure_dir,"interpolation_error_vs_node_count")

    # Differentiation step-size sensitivity from the frozen input sidecar.
    fig,ax=plt.subplots(figsize=(8.5,5.5))
    for lib in sorted(libraries_by_module["differentiation"]):
        xs=[];ys=[]
        for row in accuracy:
            if row["module"]!="differentiation" or row["library"]!=lib:continue
            h=number(str(inputs[row["problem_id"]].get("step", "")));err=number(row["absolute_error"])
            if h is not None and err is not None:xs.append(h);ys.append(max(err,1e-30))
        ax.scatter(xs,ys,s=22,alpha=.65,color=COLORS.get(lib),label=LABELS.get(lib,lib))
    ax.set_xscale("log");ax.set_yscale("log");ax.set_xlabel("Finite-difference step size");ax.set_ylabel("Actual absolute derivative error")
    ax.set_title("Differentiation error versus input step size");ax.legend(fontsize=8)
    save(fig,figure_dir,"differentiation_error_vs_step")

    # Integration evaluation effort against the requested tolerance.
    fig,ax=plt.subplots(figsize=(8.5,5.5))
    for lib in sorted(libraries_by_module["integration"]):
        xs=[];ys=[]
        for row in accuracy:
            if row["module"]!="integration" or row["library"]!=lib:continue
            tol=number(str(inputs[row["problem_id"]].get("requested_abs_tolerance", "")))
            ev=work_index.get(("integration",lib,row["problem_id"]),{}).get("function_evaluations")
            if tol is not None and ev is not None and ev>0:xs.append(tol);ys.append(ev)
        ax.scatter(xs,ys,s=22,alpha=.65,color=COLORS.get(lib),label=LABELS.get(lib,lib))
    ax.set_xscale("log");ax.set_yscale("log");ax.set_xlabel("Requested absolute tolerance");ax.set_ylabel("Measured function evaluations")
    ax.set_title("Integration function evaluations versus requested tolerance");ax.legend(fontsize=8)
    save(fig,figure_dir,"integration_evaluations_vs_tolerance")

    # Optimization objective gap against evaluation count and runtime.
    fig,axes=plt.subplots(1,2,figsize=(13,5.2))
    for lib in sorted(libraries_by_module["optimization"]):
        eval_x=[];eval_y=[];time_x=[];time_y=[]
        for row in accuracy:
            if row["module"]!="optimization" or row["library"]!=lib:continue
            gap=number(row["secondary_error"]);key=("optimization",lib,row["problem_id"])
            ev=work_index.get(key,{}).get("function_evaluations");elapsed=medians.get(key)
            if gap is not None and ev is not None and ev>0:eval_x.append(ev);eval_y.append(max(gap,1e-30))
            if gap is not None and elapsed is not None:time_x.append(elapsed);time_y.append(max(gap,1e-30))
        axes[0].scatter(eval_x,eval_y,s=18,alpha=.6,color=COLORS.get(lib),label=LABELS.get(lib,lib))
        axes[1].scatter(time_x,time_y,s=18,alpha=.6,color=COLORS.get(lib),label=LABELS.get(lib,lib))
    for ax,xlab in zip(axes,["Objective evaluations","Median wall time per solve (s)"]):
        ax.set_xscale("log");ax.set_yscale("log");ax.set_xlabel(xlab);ax.set_ylabel("Absolute objective gap");ax.legend(fontsize=7,ncol=2)
    axes[0].set_title("Optimality gap versus objective work");axes[1].set_title("Optimality gap versus runtime")
    fig.suptitle("One-dimensional bounded optimization; budget exhaustion is a separate reliability result")
    save(fig,figure_dir,"optimization_gap_work_runtime")

    # ODE accuracy by family, with fixed-step and adaptive libraries shown distinctly.
    ode_families=sorted({p["family"] for p in problems.values() if p["module"]=="ode"})
    ode_libs=sorted(libraries_by_module["ode"])
    fig,ax=plt.subplots(figsize=(14,5.5));width=.8/len(ode_libs)
    for k,lib in enumerate(ode_libs):
        vals=[]
        for family in ode_families:
            ids=[pid for pid,p in problems.items() if p["module"]=="ode" and p["family"]==family]
            rec=[acc_index[("ode",lib,pid)] for pid in ids if ("ode",lib,pid) in acc_index]
            vals.append(100*sum(r["accuracy_pass"]=="1" for r in rec)/len(rec) if rec else 0)
        pos=np.arange(len(ode_families))+(k-(len(ode_libs)-1)/2)*width
        ax.bar(pos,vals,width=width*.9,color=COLORS.get(lib),label=LABELS.get(lib,lib))
    ax.set_xticks(np.arange(len(ode_families)),[x.replace("_","\n") for x in ode_families],rotation=30,ha="right",fontsize=7)
    ax.set_ylim(0,105);ax.set_ylabel("Accuracy-passing workloads (%)");ax.set_title("ODE final-state accuracy by family")
    ax.legend(fontsize=8,ncol=2);fig.suptitle("Fixed-step and adaptive ODE methods remain separate")
    save(fig,figure_dir,"ode_accuracy_by_family")

    # Root accuracy by difficulty; scalar root-location problems do not measure root-discovery recall.
    root_difficulties=sorted({p["difficulty"] for p in problems.values() if p["module"]=="root"})
    fig,ax=plt.subplots(figsize=(8.5,4.8))
    for lib in sorted(libraries_by_module["root"]):
        vals=[]
        for difficulty in root_difficulties:
            ids=[pid for pid,p in problems.items() if p["module"]=="root" and p["difficulty"]==difficulty]
            rec=[acc_index[("root",lib,pid)] for pid in ids if ("root",lib,pid) in acc_index]
            vals.append(100*sum(r["accuracy_pass"]=="1" for r in rec)/len(rec) if rec else 0)
        ax.plot(root_difficulties,vals,marker="o",color=COLORS.get(lib),label=LABELS.get(lib,lib))
    ax.set_ylim(0,105);ax.set_ylabel("Accuracy-passing workloads (%)");ax.set_xlabel("Preregistered difficulty")
    ax.set_title("Root-finding accuracy by difficulty");ax.legend(fontsize=8)
    save(fig,figure_dir,"root_accuracy_by_difficulty")

    # Integration estimated versus actual error calibration.
    fig,ax=plt.subplots(figsize=(7,6))
    for lib in sorted(libraries_by_module["integration"]):
        xs=[];ys=[]
        for row in accuracy:
            if row["module"]!="integration" or row["library"]!=lib:continue
            actual=number(row["absolute_error"]);estimated=number(row["reported_error_estimate"])
            if actual is not None and estimated is not None and actual>0 and estimated>0:xs.append(estimated);ys.append(actual)
        if xs:ax.scatter(xs,ys,s=22,alpha=.7,color=COLORS.get(lib),label=LABELS.get(lib,lib))
    lims=ax.get_xlim();lims2=ax.get_ylim();lo=max(min(lims[0],lims2[0]),1e-30);hi=max(lims[1],lims2[1]);ax.plot([lo,hi],[lo,hi],"k--",label="estimate = actual")
    ax.set_xscale("log");ax.set_yscale("log");ax.set_xlabel("Reported error estimate");ax.set_ylabel("Actual absolute integration error")
    ax.set_title("Integration error-estimate calibration");ax.legend(fontsize=8)
    save(fig,figure_dir,"integration_error_estimate_calibration")

    # Differentiation estimate calibration.
    fig,ax=plt.subplots(figsize=(7,6))
    for lib in sorted(libraries_by_module["differentiation"]):
        xs=[];ys=[]
        for row in accuracy:
            if row["module"]!="differentiation" or row["library"]!=lib:continue
            actual=number(row["absolute_error"]);estimated=number(row["reported_error_estimate"])
            if actual is not None and estimated is not None and actual>0 and estimated>0:xs.append(estimated);ys.append(actual)
        if xs:ax.scatter(xs,ys,s=22,alpha=.7,color=COLORS.get(lib),label=LABELS.get(lib,lib))
    if ax.collections:
        lo=max(min(ax.get_xlim()[0],ax.get_ylim()[0]),1e-30);hi=max(ax.get_xlim()[1],ax.get_ylim()[1]);ax.plot([lo,hi],[lo,hi],"k--",label="estimate = actual")
    ax.set_xscale("log");ax.set_yscale("log");ax.set_xlabel("Reported derivative error estimate");ax.set_ylabel("Actual derivative error")
    ax.set_title("Differentiation error-estimate calibration");ax.legend(fontsize=8)
    save(fig,figure_dir,"differentiation_error_estimate_calibration")

    print(f"generated {len(list(figure_dir.glob('*.png')))} PNG and SVG figure pairs in {figure_dir}")
    return 0


def statistics_median(values):
    return float(np.median(values))


if __name__ == "__main__":
    raise SystemExit(main())
