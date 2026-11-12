#!/usr/bin/env python3
"""Prepare frozen pilot inputs and finalize metadata/analysis for pilot.c."""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import time

REPO = Path(__file__).resolve().parents[4]
GSL_PREFIX = Path("/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean")
RESULTS = REPO / "benchmarks/results/phase14c/module_comparisons"
RAW_MODULES = ("differentiation", "interpolation", "integration")
METHODS = {
    "differentiation": ["forward", "backward", "central", "five_point", "richardson", "second"],
    "interpolation": ["linear", "lagrange", "newton_create", "newton_evaluate"],
    "integration": ["composite_trapezoid", "composite_simpson", "adaptive_simpson"],
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(args: list[str]) -> str:
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT).strip()


def write_manifest(run_id: str) -> Path:
    target = RESULTS / "manifests" / f"pilot_workload_{run_id}.csv"
    target.parent.mkdir(parents=True, exist_ok=True)
    rows = [
        [run_id, "differentiation", "D-PILOT-SIN-01", "trigonometric", "sin(x)", json.dumps({"x": 0.7, "h": 0.01}), 1, ";".join(METHODS["differentiation"]), "cos(0.7); second derivative -sin(0.7)", "analytic derivative", "double-precision libm inputs; analytic formula", 1e-5, 1e-5, "best_available"],
        [run_id, "differentiation", "D-PILOT-EXP-01", "exponential", "exp(0.2*x)", json.dumps({"x": 1.2, "h": 0.005}), 1, ";".join(METHODS["differentiation"]), "0.2*exp(0.24); second derivative 0.04*exp(0.24)", "analytic derivative", "analytic formula evaluated with system libm", 1e-5, 1e-5, "best_available"],
        [run_id, "differentiation", "D-PILOT-CUBIC-01", "polynomial", "x^3-2*x", json.dumps({"x": 0.3, "h": 0.002}), 1, ";".join(METHODS["differentiation"]), "3*x^2-2; second derivative 6*x", "analytic derivative", "exact polynomial formula", 1e-5, 1e-5, "best_available"],
        [run_id, "interpolation", "I-PILOT-CUBIC-01", "polynomial", "x^3-2*x+1", json.dumps({"x": [-1, 0, 1, 2], "y": [0, 1, 0, 5], "query": 0.35, "linear_nodes": [0, 3]}), 4, ";".join(METHODS["interpolation"]), "q^3-2*q+1", "analytic function at query; no library used as reference", "exact polynomial formula", 1e-7, 1e-7, "linear direct functional; polynomial routes best available"],
        [run_id, "interpolation", "I-PILOT-EXP-01", "smooth_nonpolynomial", "exp(x)", json.dumps({"x": [-1, -0.5, 0.5, 1, 1.5], "query": 0.2, "linear_nodes": [0, 4]}), 5, ";".join(METHODS["interpolation"]), "exp(0.2)", "analytic function at query; no library used as reference", "exp evaluated independently at query", 1e-7, 1e-7, "linear direct functional; polynomial routes best available"],
        [run_id, "interpolation", "I-PILOT-RATIONAL-01", "rational", "1/(1+x^2)", json.dumps({"x": [-1, -0.5, 0, 0.5, 1], "query": 0.25, "linear_nodes": [0, 4]}), 5, ";".join(METHODS["interpolation"]), "1/(1+0.25^2)", "analytic function at query; no library used as reference", "exact rational formula", 1e-7, 1e-7, "linear direct functional; polynomial routes best available"],
        [run_id, "integration", "Q-PILOT-SIN-01", "trigonometric", "integral sin(x) dx", json.dumps({"a": 0, "b": "pi", "panels": 64, "gsl_fixed_points": 65}), 64, ";".join(METHODS["integration"]), "2", "analytic integral", "closed form", 1e-7, 1e-7, "best_available; fixed Gauss-Legendre differs; QAG budget unmatched"],
        [run_id, "integration", "Q-PILOT-EXP-01", "exponential", "integral exp(x) dx", json.dumps({"a": 0, "b": 1, "panels": 64, "gsl_fixed_points": 65}), 64, ";".join(METHODS["integration"]), "e-1", "analytic integral", "closed form", 1e-7, 1e-7, "best_available; fixed Gauss-Legendre differs; QAG budget unmatched"],
        [run_id, "integration", "Q-PILOT-X4-01", "polynomial", "integral x^4 dx", json.dumps({"a": -1, "b": 1, "panels": 64, "gsl_fixed_points": 65}), 64, ";".join(METHODS["integration"]), "2/5", "analytic integral", "closed form", 1e-7, 1e-7, "best_available; fixed Gauss-Legendre differs; QAG budget unmatched"],
    ]
    columns = ["run_id", "module", "problem_id", "family", "mathematical_definition", "serialized_parameters", "problem_size", "applicable_methods", "reference_value", "reference_method", "reference_precision", "target_absolute", "target_relative", "comparison_equivalence"]
    with target.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(columns)
        w.writerows(rows)
    return target


def memory_records(module_dir: Path, run_id: str) -> list[dict[str, str]]:
    timing_path = module_dir / "raw_timing.csv"
    with timing_path.open(newline="") as f:
        timing = list(csv.DictReader(f))
    unique = {}
    for row in timing:
        key = tuple(row[k] for k in ("problem_id", "library", "library_version", "numerix_method", "competitor_method", "comparison_type"))
        unique[key] = row
    out = []
    for key, row in sorted(unique.items()):
        setup = "end_to_end_setup_included" if "end_to_end" in row["competitor_method"] else "steady_state_or_public_call"
        out.append({
            "run_id": run_id, "module": row["module"], "problem_id": row["problem_id"],
            "library": row["library"], "library_version": row["library_version"],
            "numerix_method": row["numerix_method"], "competitor_method": row["competitor_method"],
            "comparison_type": row["comparison_type"], "scope": setup,
            "peak_additional_live_requested_bytes": "", "total_requested_bytes": "",
            "allocation_count": "", "reallocation_count": "", "free_count": "",
            "instrumentation": "not instrumented",
            "measurement_status": "unavailable",
            "unavailability_reason": "pilot did not implement or validate allocator interception; value intentionally omitted, not zero",
        })
    target = module_dir / "raw_memory.csv"
    with target.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(out[0].keys()))
        w.writeheader()
        w.writerows(out)
    return out


def add_setup_accuracy_rows(module_dir: Path, run_id: str) -> int:
    path = module_dir / "raw_accuracy.csv"
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        rows = list(reader)
    existing = {(r["problem_id"], r["library"], r["method"]) for r in rows}
    with (RESULTS / "manifests" / f"pilot_workload_{run_id}.csv").open(newline="") as f:
        manifest = {r["problem_id"]: r for r in csv.DictReader(f)}
    additions = []
    for problem_id, data in manifest.items():
        if data["module"] != "interpolation":
            continue
        n = len(json.loads(data["serialized_parameters"])["x"])
        for library, version, method, comp in [
            ("NUMERIX", "0.2.0-rc1", "newton_create", "gsl_interp_polynomial_init"),
            ("GSL", "2.8", "gsl_interp_polynomial_init", "gsl_interp_polynomial_init"),
        ]:
            if (problem_id, library, method) in existing:
                continue
            additions.append({
                "run_id": run_id, "module": "interpolation", "problem_id": problem_id,
                "library": library, "library_version": version, "method": method,
                "competitor_method": comp, "comparison_type": "best_available",
                "native_status": "0", "status": "not_applicable", "finite_result": "not_applicable",
                "reference": "not_applicable", "reference_method": "setup operation has no numerical output",
                "metric": "not_applicable", "absolute_error": "", "relative_error": "",
                "target_absolute": "", "target_relative": "", "pass": "not_applicable",
                "work_counter_name": "nodes_used", "work_counter": str(n),
            })
    if additions:
        with path.open("a", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writerows(additions)
    return len(additions)


def environment() -> dict:
    cpu = "unknown"
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.lower().startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    try:
        gcc = command(["gcc", "--version"]).splitlines()[0]
    except Exception:
        gcc = "unavailable"
    try:
        glibc = command(["ldd", "--version"]).splitlines()[0]
    except Exception:
        glibc = "unavailable"
    return {
        "os": platform.platform(), "kernel": platform.release(), "machine": platform.machine(),
        "cpu_model": cpu, "logical_cpu_count": os.cpu_count(), "libc": glibc,
        "compiler": gcc, "compiler_flags": "-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror",
        "gsl_version": command([str(GSL_PREFIX / "bin/gsl-config"), "--version"]),
        "gsl_prefix": str(GSL_PREFIX), "gsl_build_flags": command([str(GSL_PREFIX / "bin/gsl-config"), "--cflags"]),
        "numerix_version": "0.2.0-rc1", "numerix_archive": "build/linux-gcc-release/libnumerix.a",
        "thread_limits": {"OMP_NUM_THREADS": os.environ.get("OMP_NUM_THREADS", "unset"), "OPENBLAS_NUM_THREADS": os.environ.get("OPENBLAS_NUM_THREADS", "unset")},
        "random_seed": "none; deterministic frozen inputs",
    }


def finalize(run_id: str, start_utc: str, return_code: int) -> None:
    manifest = RESULTS / "manifests" / f"pilot_workload_{run_id}.csv"
    manifest_checksum_record = RESULTS / "manifests" / f"pilot_workload_checksum_{run_id}.txt"
    pilot_source = REPO / "benchmarks/phase14c/pilots/gsl_scalar_modules/pilot.c"
    run_script = REPO / "benchmarks/phase14c/pilots/gsl_scalar_modules/run_pilot.sh"
    support_script = Path(__file__).resolve()
    verify_script = REPO / "benchmarks/phase14c/pilots/gsl_scalar_modules/verify_pilot.py"
    head = command(["git", "rev-parse", "HEAD"])
    dirty = bool(command(["git", "status", "--porcelain"]).strip())
    end_utc = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    common = {
        "run_id": run_id, "run_scope": "pilot_only_not_final_benchmark",
        "start_utc": start_utc, "end_utc": end_utc, "git_head": head,
        "git_dirty": dirty, "environment": environment(),
        "invocation": f"run_pilot.sh {run_id}", "timer": "clock_gettime(CLOCK_MONOTONIC), CLOCK_PROCESS_CPUTIME_ID",
        "timing_design": "31 alternating-order batches per route; warmup excluded; pilot batch sizes vary by method scope",
        "memory_method": "not instrumented or validated; raw_memory values unavailable",
        "accuracy_targets": "as recorded in pilot workload manifest; accuracy rows retain all finite and failed outputs",
        "process_exit_status": return_code, "workload_manifest": str(manifest.relative_to(REPO)),
        "workload_manifest_sha256": sha256(manifest), "adapter": str(pilot_source.relative_to(REPO)),
        "workload_manifest_checksum_record": str(manifest_checksum_record.relative_to(REPO)),
        "adapter_sha256": sha256(pilot_source), "run_script_sha256": sha256(run_script),
        "support_script_sha256": sha256(support_script), "verify_script_sha256": sha256(verify_script),
    }
    checks = []
    for path in [manifest, manifest_checksum_record, pilot_source, run_script, support_script, verify_script, REPO / "build/linux-gcc-release/libnumerix.a"]:
        checks.append(f"{sha256(path)}  {path.relative_to(REPO)}")
    gsl_lib = GSL_PREFIX / "lib/libgsl.so"
    if gsl_lib.exists(): checks.append(f"{sha256(gsl_lib)}  {gsl_lib.relative_to(Path('/home/142401034/.local/numerix-deps'))}")
    checksum_file = RESULTS / "manifests" / f"pilot_source_checksums_{run_id}.txt"
    checksum_file.write_text("\n".join(checks) + "\n")
    for module in RAW_MODULES:
        module_dir = RESULTS / "raw" / module / run_id
        add_setup_rows = add_setup_accuracy_rows(module_dir, run_id) if module == "interpolation" else 0
        memory = memory_records(module_dir, run_id)
        with (module_dir / "raw_timing.csv").open(newline="") as f:
            timing = list(csv.DictReader(f))
        with (module_dir / "raw_accuracy.csv").open(newline="") as f:
            accuracy = list(csv.DictReader(f))
        with (module_dir / "raw_memory.csv").open(newline="") as f:
            memory_rows = list(csv.DictReader(f))
        group = {}
        for row in timing:
            key = (row["problem_id"], row["library"], row["library_version"], row["numerix_method"], row["competitor_method"], row["comparison_type"])
            group.setdefault(key, []).append(row)
        summary = []
        for key, values in sorted(group.items()):
            seconds_per_call = [float(r["wall_elapsed_seconds"]) / int(r["batch_size"]) for r in values]
            ratio = ""
            summary.append({
                "run_id": run_id, "module": module, "problem_id": key[0], "library": key[1],
                "library_version": key[2], "numerix_method": key[3], "competitor_method": key[4],
                "comparison_type": key[5], "timing_batches": len(values),
                "median_batch_wall_seconds_per_call": statistics.median(seconds_per_call),
                "mean_batch_wall_seconds_per_call": statistics.mean(seconds_per_call),
                "min_batch_wall_seconds_per_call": min(seconds_per_call),
                "max_batch_wall_seconds_per_call": max(seconds_per_call),
                "failed_batches": sum(r["status"] != "success" for r in values),
                "scope": "pilot descriptive summary; not inferential",
            })
        analysis_dir = RESULTS / "analysis" / module / run_id
        analysis_dir.mkdir(parents=True, exist_ok=True)
        if summary:
            with (analysis_dir / "pilot_route_summary.csv").open("w", newline="") as f:
                w = csv.DictWriter(f, fieldnames=list(summary[0].keys()))
                w.writeheader(); w.writerows(summary)
        metadata = dict(common)
        metadata.update({
            "module": module, "timing_rows": len(timing), "accuracy_rows": len(accuracy),
            "memory_rows": len(memory_rows), "setup_accuracy_rows_not_applicable_added": add_setup_rows,
            "timing_failed_rows": sum(r["status"] != "success" for r in timing),
            "accuracy_status_failures": sum(r["status"] == "failure" for r in accuracy),
            "accuracy_passes": sum(r["pass"] == "1" for r in accuracy),
            "memory_unavailable_rows": sum(r["measurement_status"] == "unavailable" for r in memory_rows),
            "analysis_summary": str((analysis_dir / "pilot_route_summary.csv").relative_to(REPO)),
            "limitations": ["three pilot problems only; not 100 problems or 20 families", "memory interception not implemented/validated", "no graphs generated", "results are not final benchmark findings"],
        })
        (module_dir / "run_metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_id")
    parser.add_argument("mode", choices=("prepare", "finalize"))
    parser.add_argument("--start-utc", default="")
    parser.add_argument("--return-code", type=int, default=0)
    args = parser.parse_args()
    if args.mode == "prepare":
        path = write_manifest(args.run_id)
        print(f"prepared {path} sha256={sha256(path)}")
    else:
        finalize(args.run_id, args.start_utc, args.return_code)
        print(f"finalized pilot metadata for {args.run_id}")


if __name__ == "__main__":
    main()
