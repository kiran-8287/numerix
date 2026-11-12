#!/usr/bin/env python3
"""Write the honest Phase 14C report, HTML render, completion note and hashes."""
from __future__ import annotations

import csv
import hashlib
import json
from collections import Counter
from pathlib import Path

import markdown

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "benchmarks/results/phase14c/module_comparisons"
MANIFEST = BASE / "manifests"
REPORTS = BASE / "reports"
MODULES = ["root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode"]
COMPETITOR = {
    "root": "GSL 2.8",
    "linear_algebra": "LAPACK 3.12.0 with OpenBLAS 0.3.34",
    "differentiation": "GSL 2.8",
    "interpolation": "GSL 2.8",
    "integration": "GSL 2.8",
    "optimization": "NLopt 2.10.1",
    "ode": "SUNDIALS 7.9.0 (ARKODE ERKStep)",
}


def rows(path: Path):
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def module_data():
    inventory = rows(MANIFEST / "method_coverage.csv")
    module_rows = {m: [r for r in inventory if r["module"] == m] for m in MODULES}
    analyses = {m: rows(BASE / "analysis" / m / "method_summary.csv") for m in MODULES}
    workload = rows(MANIFEST / "workload_manifest.csv")
    workload_counts = Counter(r["module"] for r in workload)
    return inventory, module_rows, analyses, workload, workload_counts


def root_tables() -> tuple[str, str]:
    summary = rows(BASE / "analysis/root/method_summary.csv")
    grouped = {(r["library"], r["method_key"]): r for r in summary}
    mapping = [
        ("Bisection", "bisection"), ("Regula Falsi", "regula_falsi"),
        ("Secant", "secant"), ("Newton-Raphson", "newton"),
        ("Safeguarded Newton", "safeguarded_newton"), ("Brent", "brent"),
        ("Multiple-root discovery", "multiple_root_discovery"),
    ]
    paired = rows(BASE / "analysis/root/paired_runtime_summary.csv")
    paired_map = {r["method_key"]: r for r in paired}
    timing_md = ["| Method | GSL / NUMERIX median time ratio | Problem bootstrap 95% interval | GSL accuracy passes | NUMERIX accuracy passes |",
                 "|---|---:|---:|---:|---:|"]
    accuracy_md = ["| Method | NUMERIX | GSL |", "|---|---:|---:|"]
    for label, key in mapping:
        p = paired_map.get(key, {})
        n = grouped.get(("NUMERIX", key), {})
        g = grouped.get(("COMPETITOR", key), {})
        ratio = float(p["median_paired_ratio"]) if p.get("median_paired_ratio") else None
        lo = float(p["bootstrap_95ci_low"]) if p.get("bootstrap_95ci_low") else None
        hi = float(p["bootstrap_95ci_high"]) if p.get("bootstrap_95ci_high") else None
        interval = f"[{lo:.3f}, {hi:.3f}]" if lo is not None and hi is not None else "—"
        n_pass = f"{n.get('accuracy_pass_count','—')}/{n.get('accuracy_attempt_count','—')}"
        g_pass = f"{g.get('accuracy_pass_count','—')}/{g.get('accuracy_attempt_count','—')}"
        timing_md.append(f"| {label} | {ratio:.3f} | {interval} | {g_pass} | {n_pass} |" if ratio is not None else f"| {label} | — | — | {g_pass} | {n_pass} |")
        accuracy_md.append(f"| {label} | {n_pass} | {g_pass} |")
    return "\n".join(timing_md), "\n".join(accuracy_md)


def source_checksums() -> int:
    candidates = set()
    for directory in (ROOT / "benchmarks/phase14c", ROOT / "benchmarks/results/phase14c"):
        for path in directory.rglob("*"):
            if not path.is_file():
                continue
            if "__pycache__" in path.parts or "build" in path.parts:
                continue
            if path.name == "source_checksums.txt":
                continue
            if path.suffix.lower() in {".c", ".h", ".py", ".md", ".csv", ".json", ".txt", ".html", ".svg", ".png", ".cmake"}:
                candidates.add(path)
    historical = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
    for name in ("raw_timing.csv", "accuracy.csv", "method_workloads.csv", "run_metadata.json"):
        candidates.add(historical / name)
    candidates.update({
        ROOT / "benchmarks/problems/phase14/workloads.csv",
        ROOT / "benchmarks/problems/phase14/problem_inputs.jsonl",
        ROOT / "benchmarks/analysis/phase14b/method_coverage.csv",
        ROOT / "include/numerix.h",
    })
    files = sorted(p for p in candidates if p.is_file())
    out = MANIFEST / "source_checksums.txt"
    out.write_text("".join(f"{sha(path)}  {path.relative_to(ROOT)}\n" for path in files), encoding="utf-8")
    return len(files)


def main() -> int:
    inventory, module_rows, analyses, workload, workload_counts = module_data()
    classes = Counter(r["comparison_type"] for r in inventory)
    module_counts = Counter(r["module"] for r in inventory)
    verification = json.loads((BASE / "verification/benchmark_verification.json").read_text(encoding="utf-8"))
    graph = json.loads((BASE / "verification/graph_verification.json").read_text(encoding="utf-8"))
    input_verification = json.loads((MANIFEST / "workload_intake_verification.json").read_text(encoding="utf-8"))
    la_verification = json.loads((ROOT / "benchmarks/phase14c/linear_algebra/pilot_runs/phase14c-la-pilot-20261112T055827Z/verification.json").read_text(encoding="utf-8"))
    opt_metadata = json.loads((ROOT / "benchmarks/phase14c/pilots/optimization/run_metadata.json").read_text(encoding="utf-8"))
    ode_metadata = json.loads((ROOT / "benchmarks/phase14c/pilots/ode/run_metadata.json").read_text(encoding="utf-8"))
    root_timing, root_accuracy = root_tables()

    module_summaries = {
        "root": "Historical Phase 14B data; seven paired routes, two NUMERIX-only routes; not new Phase 14C measurements.",
        "linear_algebra": "Four of 18 numerical operations piloted; four inputs and eight routes.",
        "differentiation": "Six scalar derivative methods piloted on three problems each.",
        "interpolation": "Four methods piloted on three cases each; construction accuracy is N/A.",
        "integration": "Three methods piloted on three problems each; setup and end-to-end routes retained.",
        "optimization": "Correctness-only pilot: one analytic case per method; no runtime data.",
        "ode": "Correctness-only pilot: one two-state system, 21 output points; no accuracy gate or runtime data.",
    }
    module_table = ["| Module | Primary competitor | Inventory rows | Candidate problems | Pilot or historical evidence | Accuracy | Memory |", "|---|---|---:|---:|---|---|---|"]
    for module in MODULES:
        rows_for = module_rows[module]
        kinds = Counter(r["kind"] for r in rows_for)
        timing_summary = next((r for r in analyses[module] if r.get("library") == "NUMERIX"), {})
        accuracy = f"{timing_summary.get('accuracy_pass_count', '0')}/{timing_summary.get('accuracy_attempt_count', '0')} NUMERIX passes"
        if module == "root":
            accuracy = "Historical"
        elif module == "ode":
            accuracy = "Not evaluated"
        module_table.append(
            f"| {module.replace('_',' ').title()} | {COMPETITOR[module]} | {len(rows_for)} ({kinds.get('numerical',0)} numerical, {kinds.get('lifecycle',0)} lifecycle) | {workload_counts.get(module,0)} | {module_summaries[module]} | {accuracy} | Not measured |"
        )

    figure_table = ["| Module | Runtime | Memory | Accuracy |", "|---|---|---|---|"]
    for module in MODULES:
        title = module.replace("_", " ").title()
        figures = [f"[PNG](../figures/{module}/{module}_{kind}.png) · [SVG](../figures/{module}/{module}_{kind}.svg)" for kind in ("runtime", "memory", "accuracy")]
        figure_table.append(f"| {title} | {figures[0]} | {figures[1]} | {figures[2]} |")

    root_accuracy_md = root_accuracy
    report = f"""# NUMERIX Phase 14C — Benchmark Evidence and Completion Report

> **Status: incomplete.** The dependencies, public-method audit, module pilots, analysis tables, and required figure files are assembled. The requested 100-problem-per-module full study and method-specific memory comparison were not completed. The figures distinguish historical results and pilots from missing data; they are not a final seven-module ranking.

## Executive summary

Phase 14C was requested to compare all applicable public numerical operations across seven modules, using 100 distinct problems and 20 families per module, and to report execution time, method-specific memory, and numerical accuracy. The current evidence package meets the audit and artifact requirements, but does **not** meet the full experimental coverage requirement.

- The current API audit contains **50 grouped rows**: root 9, linear algebra 21 (18 numerical operations and 3 lifecycle groups), differentiation 7, interpolation 4, integration 4, optimization 2, and ODE 3. It also lists 6 associated lifecycle/setup APIs separately. The previously omitted `numerix_lu_destroy` is included. Definitions were checked against `include/numerix.h` and `src/`.
- A candidate set of **700 unique historical Phase 14 problems** is checksum-verified: 100/module and 20 source families/module. The Phase 14C manifest has 5,000 method/problem planning rows. This is a candidate workload base, not 700 newly executed Phase 14C cases.
- Normalized figures use 52,824 raw timing observations and 1,696 accuracy records across sources. These totals include **49,600 historical Phase 14B root timing rows**; the remaining timing rows are small Phase 14C pilots. Optimization and ODE have no timing records.
- Exactly **21 required figure types** were generated, each in PNG and SVG (42 image files). All PNGs opened and verified, all SVGs parsed, and no extra graph type was generated. Every memory figure states “not measured.”
- Method-specific memory was not measured for any module. Historical process RSS was not reused as algorithm memory. A Linux/glibc allocator wrapper passed known-allocation, GSL 2.8 vector, and NUMERIX vector positive controls, but method-route visibility for OpenBLAS/LAPACK, NLopt, and SUNDIALS remains unverified.
- The runtime environment is Oracle Linux 8.10, glibc 2.28, GCC 8.5, and an Intel i7-13700. Pilot runs used isolated dependency prefixes. The selected OpenBLAS build reports bundled LAPACK 3.12.0; an older Phase 14 manifest says 3.12.1, so the Phase 14C pilot records the actual selected 3.12.0.

### Module status

{chr(10).join(module_table)}

### Figure index

Each cell links to both image formats. The memory figures state “not measured.”

{chr(10).join(figure_table)}

## Competitors and dependency setup

The fixed competitor mapping was preserved: GSL 2.8 for root finding, differentiation, interpolation, and integration; LAPACK with one-thread OpenBLAS 0.3.34 for linear algebra; NLopt 2.10.1 for optimization; and SUNDIALS 7.9.0 for ODE solving. GSL, OpenBLAS/LAPACK, and SUNDIALS were available in isolated user prefixes. NLopt 2.10.1 was built and installed alongside the existing 2.11.0 prefix at `~/.local/numerix-deps/prefix/phase14c/nlopt-2.10.1`; no system-wide package install was made. Markdown 3.7 was installed in the user Python environment to render the HTML report; Matplotlib 3.9.1 and Pillow 10.4.0 were already available.

Exact prefixes, source hashes/commits, build versions, and the historical LAPACK metadata discrepancy are recorded in [`library_versions.json`](../manifests/library_versions.json) and [`benchmark_environment.json`](../manifests/benchmark_environment.json).

## Method inventory and workload audit

The definitive grouped method list is [`method_coverage.csv`](../manifests/method_coverage.csv). Its comparison classes are: {classes.get('direct',0)} direct, {classes.get('best_available',0)} best available, {classes.get('numerix_only',0)} NUMERIX-only, {classes.get('not_comparable',0)} not comparable, and {classes.get('lifecycle_only',0)} lifecycle-only. No row remains unclassified. Automatic root selection and solver portfolio have no invented GSL measurements. Piecewise differentiation/integration operations are preserved as not comparable to GSL boundary-aware APIs. A separate [`auxiliary_lifecycle_coverage.csv`](../manifests/auxiliary_lifecycle_coverage.csv) lists report, interpolant, piecewise-object, and ODE-solution cleanup/setup APIs.

The 700 problem definitions and input sidecar were copied byte-for-byte into the Phase 14C manifest area after checking each input hash and unique ID. The manifest script checks current public symbols and implementation definitions, identifies the stale Phase 14B portfolio source field, and records candidate applicability as planned where no adapter has run. Full workload coverage is **not claimed**.

## Module findings

### Root finding — historical Phase 14B evidence

GSL 2.8 is the selected competitor. Phase 14B v3 has 100 problems per NUMERIX root method, 31 batches per timed route, and the full nine-entry NUMERIX root inventory. Seven public methods have GSL routes; automatic method selection and solver portfolio are NUMERIX-only. Multiple-root discovery uses a fixed GSL scan and remains heuristic, not an equivalent completeness guarantee. CMINPACK rows in the historical root source were excluded from the fixed GSL comparison.

The paired ratio is `GSL per-call median / NUMERIX per-call median`; values above one favor NUMERIX. These are descriptive historical summaries, not hypothesis tests or Phase 14C results:

{root_timing}

Historical accuracy-pass counts (denominator includes the attempted method records) are:

{root_accuracy_md}

Root memory is unavailable at method scope. Historical source and run hashes are retained in the manifests; the original Phase 14B files were not relabeled or rewritten.

### Linear algebra

The audited inventory has 18 numerical operations and three lifecycle groups. The accepted pilot run, `{la_verification['run_id']}`, covers matrix addition, matrix multiplication, LU factorization, and LU solve with four problem definitions, eight library routes, 31 batches per route, 8/8 accuracy passes, and 2/2 setup checks. The route-specific competitor/NUMERIX per-call ratios are 1.172 for matrix addition, 0.348 for matrix multiplication, 0.363 for LU factorization, and 1.768 for LU solve, each on **one** workload. These ratios only describe those four pilot inputs and cannot establish which library is generally faster. DGETRF end-to-end includes row-major/column-major conversion; LU solve uses each library's saved factors.

The remaining 14 numerical operations were not piloted. The 8 pilot memory rows are explicitly unavailable. OpenBLAS ran with one thread. The runtime build actually used LAPACK 3.12.0 despite the old phase-level manifest's 3.12.1 value.

### Numerical differentiation

The three-problem GSL 2.8 pilot recorded 1,116 timing rows: 36 routes/problems/accuracy records with 31 timing batches per method/problem/library route. All 36 calls returned successful finite results; 30/36 met the strict configured accuracy target. Forward and backward NUMERIX difference records missed that pilot target (0/3 each), while GSL passed those six records. Central, five-point, Richardson, and second-derivative records passed in the pilot. The second-derivative competitor is a wrapper finite-difference baseline, not a native GSL second-derivative API. The piecewise central-difference method was audited but not measured and has no verified GSL equivalent.

This result is useful for identifying step-size and accuracy issues in the adapter. Three problems do not support a module-wide speed or accuracy conclusion.

### Interpolation

The pilot covered linear interpolation, Lagrange evaluation, Newton construction, and Newton evaluation on three cases each. It retained 744 timing rows with 31 batches per route. All 18 numerical evaluation calls returned status success, but only 4/18 met the accuracy target: linear 0/6, Lagrange 2/6, Newton evaluation 2/6 across both libraries. The six setup records for Newton construction have no numerical-output accuracy result and are marked N/A. Construction and evaluation costs are separate routes. These three cases do not support a general ranking.

### Numerical integration

The pilot covered composite trapezoid, composite Simpson, and adaptive Simpson on three inputs each and retained steady/setup and end-to-end measurements separately. All 18 accuracy records had successful statuses; 14/18 met the configured gate. The fixed GSL route uses 65-point Gauss-Legendre, not a composite trapezoid/Simpson algorithm; adaptive Simpson versus QAG also uses different work policies. Those are best-available comparisons and cannot be read as identical-method speed contests. Piecewise trapezoidal integration was inventoried but not measured.

### Optimization

NLopt 2.10.1 passed runtime version enforcement. The one-case correctness pilot generated four records, all passing the independent minimizer/objective gate. Golden-section versus DIRECT-L used a common 200-objective-call budget, but the NUMERIX API has no matching initial-point control. NLopt DIRECT-L returned native status 5 (`MAXEVAL_REACHED`), recorded as `limit_reached` and `converged=0` even though its candidate passed the accuracy gate. Gradient descent and L-BFGS both converged on the pilot case; objective and gradient call counts are kept separate. No timing or memory values were collected.

### ODE solving

SUNDIALS 7.9.0 ARKODE ERKStep tableaus were checked against NUMERIX Euler, Heun, and classical RK4. One two-state analytic system was evaluated at all 21 output points. Relative max trajectory errors (scaled by the maximum analytic state on the grid) were about 8.95e-2 for Euler, 3.09e-3 for Heun, and 1.53e-6 for RK4; NUMERIX and SUNDIALS trajectories differed by at most 2.66e-15, 3.55e-15, and 7.11e-15 respectively. SUNDIALS reports one extra startup RHS evaluation (21/41/81 versus NUMERIX 20/40/80). The pilot did not define an accuracy threshold, so accuracy pass/fail is `not_evaluated`. No timing or memory values were collected.

## Memory measurement and status

The interposer tracks requested bytes through `malloc`, `calloc`, `realloc`, `aligned_alloc`, `memalign`, `posix_memalign`, and `free`. It uses a static pointer table and compiles under `-Wall -Wextra -Wpedantic -Werror`. The synthetic positive control reports 970 B peak / 1,090 B total requested with success/failure realloc and aligned-allocation controls; GSL 2.8 `gsl_vector_alloc(100)` reports 856 B; NUMERIX `vector_add` on 100 values reports 816 B. These are **visibility controls**, not algorithm memory results.

Direct `mmap`, custom allocators, some libc allocation paths, and separately loaded `LD_PRELOAD` behavior are not validated. Positive controls through OpenBLAS/LAPACK, NLopt, and SUNDIALS are outstanding. Therefore all seven method-memory graphs state “not measured,” and every memory conclusion remains unavailable.

## Graphs, raw data, and reproducibility

Each module has exactly the three requested figures: runtime, method-specific memory, and numerical accuracy. Each figure is saved as PNG and SVG. The artifact generator is [`generate_comparison_artifacts.py`](../../../../phase14c/generate_comparison_artifacts.py); the audit/workload manifest generator is [`prepare_phase14c_manifest.py`](../../../../phase14c/prepare_phase14c_manifest.py). Raw pilot CSVs and run metadata are preserved by run ID under `../raw/`; historical root data remain in their original Phase 14B directory and are referenced by checksum.

The separately filtered [executed-method inventory and figures](../../executed_methods/README.md) include only routes with run records. They include historical root findings, bounded timing pilots, and correctness-only pilots with their evidence type labeled.

Three combined all-module figures for [runtime, memory, and accuracy](../../executed_methods/combined/README.md) provide the broad overview. Their captions identify unavailable data and the limits on cross-module comparisons.

Processed per-method summaries and paired problem-level timing ratios are under `../analysis/<module>/`. The statistical unit is the problem; timing batches are first reduced to a within-problem median. Bootstrap intervals resample problem ratios, but three-problem and single-problem pilots are too small to support broad inference. All accuracy failures, not-applicable setup records, convergence/native statuses, and missing routes remain visible in the CSV summaries or figure annotations.

Verification files:

- [`benchmark_verification.json`](../verification/benchmark_verification.json) — raw-source counts and study limits.
- [`graph_verification.json`](../verification/graph_verification.json) — exactly 21 figures, 21 verified PNGs, 21 valid SVGs.
- [`workload_intake_verification.json`](../manifests/workload_intake_verification.json) — 700 candidate inputs and 5,000 method/problem planning rows.
- [`source_checksums.txt`](../manifests/source_checksums.txt) — checksums for the source, raw, processed, and figure artifacts.
- [`methodology_audit.md`](../../../../phase14c/methodology_audit.md) — independent fairness, statistics, provenance, and memory review.

The existing presentation directory was not modified during Phase 14C. No dashboard was created, and no repository changes were committed or pushed.

## What remains before this can be called a complete benchmark

1. Run every applicable method on the frozen 100-problem/20-family set in every module, including all remaining linear-algebra routes and the NUMERIX-only piecewise inputs.
2. Establish and record independent high-precision/analytic references, common accuracy targets, applicability, native status, and work counts for every attempted route. Define an ODE accuracy gate before classifying pass/fail.
3. Validate allocator visibility through each actual GSL, OpenBLAS/LAPACK, NLopt, and SUNDIALS route; then collect isolated per-operation memory samples with symmetric input/output/workspace scopes. Keep an unavailable status whenever visibility fails.
4. Reconcile full raw timing, accuracy, memory, and process status against the frozen method/workload manifest; record timer resolution and CPU affinity; regenerate figures and report from the final data.

The current package is reproducible pilot evidence and a verified inventory. It does not answer the requested final all-module “which is faster, smaller, or more accurate” question.
"""

    REPORTS.mkdir(parents=True, exist_ok=True)
    md_path = REPORTS / "NUMERIX_module_comparison_report.md"
    html_path = REPORTS / "NUMERIX_module_comparison_report.html"
    md_path.write_text(report, encoding="utf-8")
    body = markdown.markdown(report, extensions=["tables", "fenced_code", "toc"])
    html = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NUMERIX Phase 14C Benchmark Evidence Report</title>
<style>
body{{margin:0;background:#f4f6f8;color:#192433;font:16px/1.65 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}}
main{{max-width:1120px;margin:36px auto;padding:48px 58px;background:#fff;border-radius:14px;box-shadow:0 8px 32px #152b4215}}
h1,h2,h3{{line-height:1.22;color:#142b44}}h1{{font-size:2.25rem;border-bottom:2px solid #dbe4ed;padding-bottom:18px}}h2{{margin-top:2.2em;border-bottom:1px solid #e4eaf0;padding-bottom:8px}}h3{{margin-top:1.8em}}
blockquote{{margin:20px 0;padding:14px 18px;border-left:5px solid #c2473d;background:#fff3f1;font-weight:650}}
table{{width:100%;border-collapse:collapse;font-size:.9rem;display:block;overflow-x:auto}}th,td{{border:1px solid #dce3ea;padding:9px 11px;text-align:left;vertical-align:top}}th{{background:#edf2f7;color:#19334e}}
code{{background:#f0f3f6;padding:2px 5px;border-radius:4px}}a{{color:#0f5ea8}}img{{max-width:100%;height:auto}}li{{margin:.28em 0}}hr{{border:0;border-top:1px solid #dce3ea;margin:28px 0}}
@media(max-width:760px){{main{{margin:0;padding:24px 18px;border-radius:0}}h1{{font-size:1.75rem}}}}
</style></head><body><main>{body}</main></body></html>"""
    html_path.write_text(html, encoding="utf-8")

    completion = """# Phase 14C completion report

**Status: incomplete.** This package contains a verified 50-row API/method inventory, 700 checksum-verified candidate problems, 5,000 method/problem planning rows, limited method pilots, an independent methodology audit, and exactly 21 PNG + 21 SVG figure files. No full 700-problem Phase 14C study was executed. Method-specific memory is unavailable for all modules; the memory figures say “not measured.”

Completed in this phase:

- Installed NLopt 2.10.1 in an isolated user prefix; verified GSL 2.8, OpenBLAS 0.3.34 with LAPACK 3.12.0, and SUNDIALS 7.9.0 prefixes.
- Audited current public declarations and implementations, including LU lifecycle cleanup and auxiliary output lifecycle APIs.
- Ran bounded pilots for GSL differentiation/interpolation/integration, four linear-algebra methods, two optimizer methods, and three fixed-step ODE methods. Root data remain historical Phase 14B evidence.
- Validated allocator known controls for the synthetic tracer, GSL vector allocation, and NUMERIX vector addition; these are not benchmark memory observations.
- Generated processed summaries, paired problem-level timing summaries, the 21 required figure slots in both formats, Markdown and HTML reports, and SHA-256 checksums.

Not complete: 100 problems and 20 families per module for every applicable method; full method-level memory measurements; OpenBLAS/LAPACK, NLopt, and SUNDIALS allocator-route validation; and a final module-wide ranking.
"""
    (BASE / "verification/completion_report.md").write_text(completion, encoding="utf-8")

    count = source_checksums()
    report_verification = {
        "schema_version": "phase14c-report-verification-v1",
        "markdown_report": str(md_path.relative_to(ROOT)),
        "html_report": str(html_path.relative_to(ROOT)),
        "markdown_nonempty": md_path.stat().st_size > 1000,
        "html_nonempty": html_path.stat().st_size > 1000,
        "source_checksum_entries": count + (0 if (BASE / "verification/report_verification.json").exists() else 1),
        "method_inventory_count": len(inventory),
        "candidate_problem_count": len(workload),
        "candidate_problems_per_module": dict(workload_counts),
        "full_phase14c_study_complete": False,
        "exact_required_figure_count": graph["required_graph_count"] == 21 and graph["png_count"] == 21 and graph["svg_count"] == 21,
        "unclassified_method_rows": classes.get("comparison_class_needs_review", 0),
    }
    (BASE / "verification/report_verification.json").write_text(json.dumps(report_verification, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    source_checksums()
    print(json.dumps(report_verification, indent=2))
    valid_workload = len(workload) == 700 and all(workload_counts.get(module) == 100 for module in MODULES)
    valid = (
        report_verification["markdown_nonempty"]
        and report_verification["html_nonempty"]
        and report_verification["exact_required_figure_count"]
        and report_verification["unclassified_method_rows"] == 0
        and len(inventory) == 50
        and valid_workload
    )
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
