#!/usr/bin/env python3
"""Build the Phase 14 Markdown and HTML reports from saved analysis results."""
from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from pathlib import Path

import markdown

ROOT = Path(__file__).resolve().parents[3]
MODULES = ("root", "linear_algebra", "differentiation", "interpolation",
           "integration", "optimization", "ode")
LABELS = {
    "root": "Root finding", "linear_algebra": "Linear algebra",
    "differentiation": "Numerical differentiation", "interpolation": "Interpolation",
    "integration": "Numerical integration", "optimization": "Optimization",
    "ode": "ODE solving",
}
LIBS = {
    "NUMERIX": "NUMERIX", "GSL": "GSL 2.8", "CMINPACK": "CMINPACK 1.3.15",
    "LAPACK_REFERENCE": "Reference LAPACK/LAPACKE",
    "LAPACK_OPENBLAS": "LAPACK/LAPACKE + OpenBLAS",
    "NLOPT": "NLopt 2.11.0", "SUNDIALS_ARKODE": "SUNDIALS ARKODE 7.9.0",
    "SUNDIALS_CVODE": "SUNDIALS CVODE 7.9.0",
}
METHODS = {
    "root": "NUMERIX Brent, or Newton with analytic derivative for designated multiple-root families; GSL Brent/Newton; CMINPACK hybrd1 as a one-variable nonlinear system.",
    "linear_algebra": "Dense Gaussian-elimination solve through NUMERIX, GSL LU decomposition/solve, and LAPACKE dgesv with reference BLAS or OpenBLAS.",
    "differentiation": "NUMERIX two-point central difference and GSL gsl_deriv_central. First derivatives only in this measured path.",
    "interpolation": "NUMERIX global Newton polynomial evaluation and GSL gsl_interp_polynomial evaluation.",
    "integration": "NUMERIX adaptive Simpson and GSL QAG with a 61-point Gauss-Kronrod rule.",
    "optimization": "NUMERIX bounded 1D golden section; GSL grid bracket scan plus golden section; NLopt bounded 1D DIRECT-L.",
    "ode": "Fixed-step classical RK4 through NUMERIX, GSL odeiv2, and SUNDIALS ARKODE Kutta-RK4a; adaptive CVODE Adams is reported separately.",
}

def read_csv(path):
    csv.field_size_limit(sys.maxsize)
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))

def number(value):
    try:
        x = float(value) if value not in (None, "") else math.nan
        return x if math.isfinite(x) else None
    except (TypeError, ValueError):
        return None

def fmt(value, digits=3):
    x = number(value)
    return "—" if x is None else f"{x:.{digits}g}"

def table(headers, rows):
    out = ["| " + " | ".join(headers) + " |",
           "| " + " | ".join("---" for _ in headers) + " |"]
    out.extend("| " + " | ".join(str(v).replace("|", "\\|") for v in row) + " |"
               for row in rows)
    return "\n".join(out)

def build(run_dir):
    coverage = read_csv(run_dir / "coverage.csv")
    comparisons = read_csv(run_dir / "comparisons.csv")
    measurements = read_csv(run_dir / "measurements.csv")
    failures = read_csv(run_dir / "execution_failures.csv")
    raw = read_csv(run_dir / "raw_timing.csv")
    accuracy = read_csv(run_dir / "accuracy.csv")
    metadata = json.loads((run_dir / "run_metadata.json").read_text(encoding="utf-8"))
    verify = json.loads((run_dir / "analysis_verification.json").read_text(encoding="utf-8"))
    manifest = read_csv(ROOT / "benchmarks/problems/phase14/workloads.csv")
    if verify.get("result") != "PASS" or len(manifest) != 700 or len(raw) != 62000 or len(accuracy) != 2000:
        raise ValueError("Saved Phase 14 data has not passed expected count and integrity checks")

    cindex = {(r["module"], r["library"]): r for r in coverage}
    lines = [
        "# NUMERIX Phase 14 — Expanded Multi-Library Comparative Evaluation", "",
        "**Study date:** 12 November 2026<br>",
        f"**Primary run:** {metadata['run_id']}<br>",
        "**Project version:** 0.2.0-rc1<br>",
        "**Study status:** **Partial, with the problem-count floor and paired-run coverage met.**", "",
        "## 1. Executive summary", "",
        "The primary run exercised 700 distinct mathematical problems: 100 in each of NUMERIX's seven modules, drawn from 20 families per module. Every configured library/module combination ran the same 100 stable problem IDs. The run contains 2,000 library/problem accuracy records, 62,000 raw wall/CPU timing batches (31 per pair), 8,000 work-metric records, and 2,000 isolated-process RSS probes. There were no launch or output-write failures.", "",
        "The evidence does not meet every broad method-coverage requirement in the supplied Phase 14 brief. This run measures one selected method family per module and omits several required NUMERIX methods and operation-level phases. CPU and wall timing are retained, but paired inference is based on wall time. Process RSS includes the harness/library baseline and requested heap allocation could not be instrumented. The study is therefore partial and does not claim comprehensive evaluation of every method or a universal winner.", "",
        "NUMERIX passed accuracy on all 100 root and ODE cases; 82 differentiation, 76 linear-system, 38 interpolation, 95 integration, and 100 optimization cases. NLopt returned accurate minimizers for all 100 optimization cases but exhausted its 4,000-evaluation limit in all 100 and had zero successful convergence statuses. Those NLopt trials do not support a valid speed claim. The main numerical limitations exposed here are differentiation accuracy, global-polynomial interpolation, difficult linear systems, and inverse-square-root endpoint integration.", "",
        "## 2. NUMERIX overview and repository state", "",
        "The benchmark used the existing 0.2.0-rc1 public C API and did not modify production algorithms, public headers, mathematical behavior, or project version. NUMERIX currently exposes scalar root finding; vector/matrix operations, dense solves and LU; first/second finite differences; linear/Lagrange/Newton interpolation; finite trapezoid/Simpson/adaptive Simpson quadrature; bounded one-dimensional golden-section and vector gradient descent; and vector-valued fixed-step Euler, Heun and RK4 ODE methods. This run measured only the selected paths below.", "",
        "The repository was on branch master at commit 2292ee804b02bd82be56803a6eae9d36327ef117 at run time. Phase 13 files are preserved and have a SHA-256 inventory. The full pasted request is copied to [PHASE14_REQUIREMENTS.md](../../PHASE14_REQUIREMENTS.md). The brief forbids a Phase 14 commit, push, tag, or version change.", "",
        "## 3. Competitor libraries", "",
        table(["Library/configuration", "Modules", "Measured role"], [
            ["GSL 2.8", "all seven", "Broad native-C comparator; actual methods are listed below."],
            ["CMINPACK 1.3.15", "root", "One-variable MINPACK hybrid solver; best-available local method, no bracket guarantee."],
            ["Reference LAPACK/LAPACKE 3.12.1 + reference BLAS", "linear algebra", "Row-major LAPACKE dgesv combined dense solve."],
            ["LAPACK/LAPACKE 3.12.1 + OpenBLAS 0.3.34", "linear algebra", "OpenBLAS-backed LAPACKE dgesv; one runtime thread."],
            ["NLopt 2.11.0", "optimization", "Bounded one-dimensional DIRECT-L best available."],
            ["SUNDIALS 7.9.0 ARKODE", "ODE", "Fixed-step Kutta-RK4a tableau verified against classical RK4."],
            ["SUNDIALS 7.9.0 CVODE", "ODE", "Adaptive Adams with serial fixed-point solver; best available."],
        ]), "",
        "All competitor builds are optional benchmark dependencies. Versions, licenses, official URLs, build flags, linked BLAS, API type, exclusions and limitations are in the [library capability matrix](../development/phase_14_library_capability_matrix.md) and [libraries.csv](../../benchmarks/analysis/phase14/libraries.csv). ALGLIB and Boost.Math were excluded from the native-C run due to language/API fit; Cuba targets multidimensional integration; Cephes is not a comparable seven-module package.", "",
        "## 4. Experimental methodology", "",
        "The workload seed is 20261112. Each module has 20 named families and five distinct parameterized inputs per family. Stable IDs, SHA-256-linked input data, references, tolerances, difficulty, expected behavior, eligible libraries and method metadata are saved. References use closed forms, known constructed solutions, 70-digit mpmath, or independent SciPy DOP853 with a halved-step check for nonlinear ODEs. A harmonic-oscillator reference formula was corrected during pilot validation before the primary run; IDs and input hashes were retained.", "",
        f"The primary build used {metadata['compiler']}, -O3 -DNDEBUG, no -ffast-math, double precision, and single-threaded libraries. The machine was {metadata['machine']} ({metadata['system']}; {metadata['processor']}, {metadata['cpu_count_logical']} logical CPUs). Each pair had 31 timing batches after warm-up/calibration; library execution order was deterministically shuffled with seed 20261112. Raw rows contain CLOCK_MONOTONIC wall time and CLOCK_PROCESS_CPUTIME_ID CPU time; per-call summaries are in measurements.csv. Paired inference uses wall time.", "",
        "Mode A means algorithm-equivalent; Mode B means best-available but algorithmically different. Categories are not pooled. The speed ratio is competitor median wall time / NUMERIX median wall time, so above 1 favors NUMERIX. Only pairs where both libraries returned successful status and both passed the frozen accuracy gate contribute to speed estimates. Confidence intervals are 95% paired-problem bootstrap intervals (10,000 resamples); two-sided Wilcoxon tests on log ratios are exploratory and not adjusted for multiple comparisons.", "",
        "## 5. Benchmark coverage", "",
        table(["Module", "Problems", "Families", "Library/problem pairs", "Accuracy records", "Timing batches"], [
            [LABELS[m], "100", "20",
             str(sum(r["module"] == m for r in coverage) * 100),
             str(sum(int(r["accuracy_observations"]) for r in coverage if r["module"] == m)),
             str(sum(int(r["timing_samples"]) for r in coverage if r["module"] == m))]
            for m in MODULES
        ]), "",
        "Every configured competitor has 100 common IDs in its applicable module. Status, convergence and accuracy are separate measurements.", "",
        table(["Module", "Library", "Observed", "Status", "Converged", "Accurate", "Timing batches"], [
            [LABELS[r["module"]], LIBS.get(r["library"], r["library"]), r["accuracy_observations"],
             f"{r['status_success_count']}/100 ({fmt(r['status_success_rate_percent'], 4)}%)",
             f"{r['converged_count']}/100 ({fmt(r['convergence_rate_percent'], 4)}%)",
             f"{r['accuracy_pass_count']}/100 ({fmt(r['accuracy_pass_rate_percent'], 4)}%)",
             r["timing_samples"]]
            for r in coverage
        ]), "",
        "## 6. Results by module", "",
    ]
    for module in MODULES:
        nux = cindex[(module, "NUMERIX")]
        peers = [r for r in comparisons if r["module"] == module]
        lines += [f"### {LABELS[module]}", "",
                  f"**Methods measured:** {METHODS[module]}", "",
                  f"**NUMERIX:** status {nux['status_success_count']}/100; converged {nux['converged_count']}/100; accurate {nux['accuracy_pass_count']}/100. Each configured competitor completed all 100 input IDs.", ""]
        lines.append(table(["Competitor", "Mode", "Common IDs", "Accuracy-qualified", "Status + accuracy", "Median speedup (95% CI)"], [
            [LIBS.get(r["competitor"], r["competitor"]),
             "A: equivalent" if r["comparison_mode"] == "algorithm_equivalent" else "B: best available",
             r["paired_common_problems"], r["accuracy_qualified_pairs"],
             r["status_and_accuracy_qualified_pairs"],
             (f"{fmt(r['median_paired_speedup'])} ({fmt(r['median_speedup_ci95_low'])}, {fmt(r['median_speedup_ci95_high'])})"
              if number(r["median_paired_speedup"]) is not None else "not estimable: no qualifying pairs")]
            for r in peers
        ]) if peers else "No external competitor was configured.")
        module_notes = {
            "root": "GSL's pooled median interval spans parity. CMINPACK is a local nonlinear-system comparison, not a bracketed-root equivalent.",
            "linear_algebra": "All paths are combined dense solves. This run does not isolate factorization, solve-only, BLAS primitives, or a full backward-error comparison. Row-major LAPACKE conversion is included end to end.",
            "differentiation": "NUMERIX accuracy is 82/100 under the preregistered gate. Speed comparison uses only successful and accurate pairs and does not imply greater overall accuracy.",
            "interpolation": "Both global-polynomial implementations pass only 38/100 under the strict frozen gate. Conclusions apply only to that passing subset.",
            "integration": "NUMERIX has 95 successful/accurate cases; inverse-square-root endpoint stress cases account for its main misses. GSL QAG and adaptive Simpson are different algorithms.",
            "optimization": "NLopt passes the location accuracy gate for 100 returned points but reports zero successful/converged statuses because all runs hit the 4,000-evaluation cap. It has no status+accuracy-qualified speed estimate. GSL has five status failures despite 100 accuracy passes.",
            "ode": "Fixed-mesh RK4 comparisons use the matched classical tableau. Adaptive CVODE is Mode B and remains separate. All implementations pass all 100 status, convergence and accuracy checks.",
        }
        lines += [module_notes[module], ""]

    lines += [
        "## 7. Cross-module paired performance results", "",
        "Only successful-status and accuracy-passing pairs enter these ratios. Above 1 favors NUMERIX. n counts distinct problem pairs; a missing estimate means no qualifying pairs, not zero speed.", "",
        table(["Module", "Competitor", "Mode", "n", "Median ratio", "95% problem CI", "Wilcoxon p"], [
            [LABELS[r["module"]], LIBS.get(r["competitor"], r["competitor"]),
             "A" if r["comparison_mode"] == "algorithm_equivalent" else "B",
             r["performance_qualified_pairs"], fmt(r["median_paired_speedup"]),
             (f"[{fmt(r['median_speedup_ci95_low'])}, {fmt(r['median_speedup_ci95_high'])}]"
              if number(r["median_speedup_ci95_low"]) is not None else "not estimable"),
             fmt(r["wilcoxon_p_two_sided"], 3)]
            for r in comparisons
        ]), "",
        "These results are algorithm- and workload-specific. GSL was faster on the accuracy/status-qualified adaptive-integration subset; NUMERIX was faster on the qualified differentiation subset. Neither result generalizes to a whole library.", "",
        "## 8. Accuracy analysis", "",
        "Accuracy is tested against independent references using absolute and relative tolerances recorded in the manifest. Solver success and reported error estimates are not substitutes for actual error.", "",
        "See the per-library table in Section 5, [accuracy.csv](../../benchmarks/results/phase14/main/accuracy.csv), and module figures below. Linear failures include singular/ill-conditioned systems; both global polynomial implementations have low pass rates at this strict tolerance; NUMERIX misses some differentiation and endpoint integration cases. ODE references were corrected before the main run and all 20 harmonic checks passed in validation.", "",
        "## 9. Performance and computational effort", "",
        f"The run retained {len(raw):,} raw batches with both clocks. {sum(int(r['cpu_batch_samples']) for r in measurements):,} positive CPU batch measurements are included in per-call summaries in measurements.csv; {verify['cpu_zero_elapsed_rows']} zero-resolution CPU timer deltas are retained in raw_timing.csv but excluded from those summaries. There are {len(measurements):,} grouped method/size summaries. Each of 2,000 library/problem pairs has 31 batches.", "",
        "Function evaluations, iterations and ODE right-hand-side evaluations are recorded where exposed by the API. Missing and inapplicable work metrics are explicitly marked; they are not written as zero. Work summaries provide medians, means, p95 and maxima. There is no complete linear algebra operation decomposition or interpolation construction/evaluation timing split.", "",
        "## 10. Memory analysis", "",
        "There are 2,000 isolated-process getrusage(RUSAGE_SELF).ru_maxrss observations in KiB. Typical measurements are about 9.7 MiB and nearly flat because the process and shared-library baseline dominates. RSS includes the harness and setup; it is not solver-only memory. Requested heap bytes, allocation counts and live heap are unavailable and blank, not zero. These data do not establish a memory winner.", "",
        "## 11. Reliability and convergence", "",
        f"The main run has {len(failures)} launch/write failures, and all planned adapter/problem outputs are represented. Numerical status failures remain in counts and raw records. NLopt's 0/100 successful statuses and 0/100 convergence are distinct from its 100/100 accurate returned locations.", "",
        "## 12. Scalability", "",
        "Input dimensions vary for selected workloads such as dense matrix order, interpolation nodes and ODE step counts. The overview plots size curves where the input has a meaningful size variable. Fixed-size scalar workloads use independent workload index and are not presented as size-scaling curves. This method-level scope cannot establish broad asymptotic behavior.", "",
        "## 13. Main observations", "",
        "- NUMERIX has strong status/accuracy outcomes in measured root, bounded optimization and ODE paths, while differentiation, interpolation, difficult linear systems and endpoint integration expose correctness work.",
        "- GSL is the broad baseline. Specialist libraries add scoped comparisons; LAPACK/LAPACKE + OpenBLAS is accurately described as the LAPACKE path backed by OpenBLAS.",
        "- NLopt's accurate output is not a converged result under the configured budget.",
        "- RSS mostly measures process baseline. Heap instrumentation is still needed for useful library-memory attribution.",
        "- Fixed-step RK4 and adaptive/best-available comparisons remain separate.", "",
        "## 14. Limitations and unmet requirements", "",
        "The primary run meets the 700 distinct-problem floor, 20 families/module, applicable competitors, and 100 paired inputs/current adapter. It does not comprehensively measure all public methods requested in the brief:", "",
        "- Root: selected Brent/Newton paths only; no method-by-method bisection, regula falsi, secant, safeguarded-method, or multiple-root discovery recall study.",
        "- Linear algebra: combined dense solve only; no vector/matrix primitives, NUMERIX LU factorization, factor-only/solve-only phases, or full backward-error comparison.",
        "- Differentiation: first derivative central difference only; no second derivative, NUMERIX five-point/Richardson paths, or method-by-method coverage.",
        "- Interpolation: global Newton polynomial only; no linear/Lagrange comparisons or construction/evaluation timing split.",
        "- Integration: adaptive Simpson only; no fixed trapezoid/Simpson methods or separately preregistered common tolerance sweep.",
        "- Optimization: one-dimensional golden section only; no NUMERIX gradient descent. NLopt's budget exhaustion leaves no successful speed estimate.",
        "- ODE: fixed RK4 plus adaptive CVODE; no NUMERIX Euler/Heun comparison, and main accuracy gate uses endpoint rather than full trajectory norm.",
        "- CPU time is retained and summarized but inferential comparisons use wall time. Memory is RSS-only; heap allocation instrumentation is unavailable.",
        "- One Linux host, compiler and thread configuration do not establish cross-platform performance.", "",
        "These gaps are why the study remains partial. A follow-on should add method-level workload/run matrices, operation-specific linear algebra timing/error, more differentiation and interpolation alternatives, fixed quadrature and optimization/ODE coverage, trajectory-wide ODE error, and allocation instrumentation.", "",
        "## 15. Recommendations", "",
        "1. Improve NUMERIX central-difference accuracy under curvature and floating-point-scale stress.",
        "2. Investigate interpolation conditioning and node strategy while retaining the observed low pass rate as a correctness signal.",
        "3. Investigate singular endpoint handling in adaptive Simpson.",
        "4. Improve status/error reporting for singular and ill-conditioned systems.",
        "5. Use a separately preregistered realistic NLopt budget; do not reinterpret the current exhausted runs as converged.",
        "6. Extend the benchmark to every public method and add heap instrumentation before category-wide conclusions.", "",
        "## 16. Reproducibility and artifacts", "",
        "From repository root, validate the saved workload; preparation can regenerate the Phase 14 manifest and input sidecar:", "",
        "    python3 benchmarks/analysis/phase14/verify_workloads.py", "",
        "Regenerate analysis, figures and reports without rerunning timed library measurements:", "",
        "    python3 benchmarks/analysis/phase14/analyze_results.py benchmarks/results/phase14/main",
        "    python3 benchmarks/analysis/phase14/plot_results.py benchmarks/results/phase14/main",
        "    python3 benchmarks/analysis/phase14/generate_report.py benchmarks/results/phase14/main", "",
        "Analysis requires NumPy/SciPy, plotting requires NumPy/Matplotlib, and HTML conversion uses Python-Markdown. Adapter build details are in [README](../../benchmarks/adapters/phase14/README.md), [runner](../../benchmarks/analysis/phase14/run_benchmark.py), and the [execution log](../development/phase_14_audit_and_execution_log.md).", "",
        "Evidence: [manifest](../../benchmarks/problems/phase14/workloads.csv), [input sidecar](../../benchmarks/problems/phase14/problem_inputs.jsonl), [raw timing](../../benchmarks/results/phase14/main/raw_timing.csv), [accuracy](../../benchmarks/results/phase14/main/accuracy.csv), [work metrics](../../benchmarks/results/phase14/main/work_metrics.csv), [RSS](../../benchmarks/results/phase14/main/memory.csv), [run metadata](../../benchmarks/results/phase14/main/run_metadata.json), [verification](../../benchmarks/results/phase14/main/analysis_verification.json), [coverage](../../benchmarks/results/phase14/main/coverage.csv), and [comparisons](../../benchmarks/results/phase14/main/comparisons.csv).", "",
        "## Figures", "",
    ]

    titles = {
        "overview_01_coverage": "Coverage by module and library",
        "overview_02_status_convergence_accuracy": "Status, convergence and accuracy",
        "overview_03_reliability": "Status and convergence counts",
        "overview_04_paired_speedup": "Status and accuracy-qualified speedups",
        "overview_05_runtime_scalability": "Runtime by module and available size axis",
        "overview_06_memory_scalability": "Isolated-process peak RSS",
        "overview_07_accuracy_distribution": "Module-specific error distributions",
        "overview_08_time_to_accuracy": "Wall time versus observed error",
        "root_accuracy_by_difficulty": "Root accuracy by difficulty",
        "root_error_vs_work": "Root error versus function evaluations",
        "linear_algebra_conditioning_and_residual": "Linear conditioning and residual",
        "differentiation_error_estimate_calibration": "Differentiation error-estimate calibration",
        "differentiation_error_vs_step": "Differentiation error versus step",
        "differentiation_error_vs_work": "Differentiation error versus work",
        "interpolation_error_vs_node_count": "Interpolation error versus node count",
        "integration_error_estimate_calibration": "Integration error-estimate calibration",
        "integration_error_vs_work": "Integration error versus work",
        "integration_evaluations_vs_tolerance": "Integration evaluations versus tolerance",
        "optimization_error_vs_work": "Optimization error versus work",
        "optimization_gap_work_runtime": "Optimization gap, work and runtime",
        "ode_accuracy_by_family": "ODE accuracy by family",
        "ode_error_vs_work": "ODE error versus RHS evaluations",
    }
    figures = sorted(p.stem for p in (run_dir / "figures").glob("*.png"))
    if len(figures) != 22 or any(not (run_dir / "figures" / f"{n}.svg").exists() for n in figures):
        raise ValueError("Expected 22 PNG/SVG figure pairs before report generation")
    for name in figures:
        title = titles.get(name, name.replace("_", " ").title())
        lines += [f"### {title}", "",
                  f"![{title}](../../benchmarks/results/phase14/main/figures/{name}.png)", "",
                  f"[SVG](../../benchmarks/results/phase14/main/figures/{name}.svg)", ""]
    return "\n".join(lines).rstrip() + "\n"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", nargs="?", default="benchmarks/results/phase14/main")
    args = parser.parse_args()
    run_dir = (ROOT / args.run_dir).resolve()
    report = build(run_dir)
    out_dir = ROOT / "documentation/reports"
    out_dir.mkdir(parents=True, exist_ok=True)
    md_path = out_dir / "NUMERIX_expanded_multi_library_comparative_evaluation.md"
    html_path = out_dir / "NUMERIX_expanded_multi_library_comparative_evaluation.html"
    md_path.write_text(report, encoding="utf-8")
    body = markdown.markdown(report, extensions=["tables", "fenced_code"])
    document = """<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NUMERIX Phase 14 Comparative Evaluation</title>
<style>
body{font:16px/1.55 system-ui,-apple-system,Segoe UI,sans-serif;max-width:1200px;margin:2rem auto;padding:0 1.25rem;color:#18212b}
h1,h2,h3{line-height:1.2;color:#102a43}h1{border-bottom:2px solid #bcccdc;padding-bottom:.5rem}
table{border-collapse:collapse;width:100%;font-size:.9rem;display:block;overflow-x:auto;margin:1rem 0 1.5rem}
th,td{border:1px solid #bcccdc;padding:.45rem .6rem;text-align:left;vertical-align:top}th{background:#f0f4f8}
img{max-width:100%;height:auto;border:1px solid #d9e2ec}pre{background:#f0f4f8;padding:1rem;overflow:auto}
code{background:#f0f4f8;padding:.1rem .25rem;border-radius:3px}a{color:#1261a0}
</style></head><body>
""" + body + "\n</body></html>\n"
    html_path.write_text(document, encoding="utf-8")
    print(f"generated {md_path.relative_to(ROOT)} and {html_path.relative_to(ROOT)}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
