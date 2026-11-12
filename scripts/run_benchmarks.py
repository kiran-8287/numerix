#!/usr/bin/env python3
"""Run the versioned Phase 13B benchmark and preserve raw observations."""
import argparse
import csv
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import uuid


def capture(command):
    try:
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, check=False)
        return {"command": command, "exit_code": result.returncode,
                "output": result.stdout.strip()}
    except OSError as error:
        return {"command": command, "unavailable": str(error)}


def file_contents(path):
    try:
        return Path(path).read_text()
    except OSError:
        return "unavailable"


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def safe_output_directory(path, repository_root):
    output = path.resolve()
    for historical in (repository_root / "benchmarks/results/phase9",
                       repository_root / "benchmarks/results/phase10"):
        try:
            output.relative_to(historical.resolve())
        except ValueError:
            continue
        raise ValueError(f"historical results are read-only: {historical}")
    if output.exists():
        if not output.is_dir() or any(output.iterdir()):
            raise ValueError(f"output directory must be empty: {output}")
    else:
        output.mkdir(parents=True)
    return output


def write_comparisons(directory):
    with (directory / "measurements.csv").open(newline="") as source:
        rows = list(csv.DictReader(source))
    pairs = {}
    for row in rows:
        if row["pair_id"]:
            pairs.setdefault((row["run_id"], row["pair_id"]), {})[
                row["library"]] = row
    destination = directory / "comparisons.csv"
    with destination.open("x", newline="") as output:
        writer = csv.writer(output, lineterminator="\n")
        writer.writerow([
            "schema_version", "run_id", "pair_id", "configuration_id",
            "category", "case", "input_size", "method", "phase",
            "comparison_mode", "numerix_median_ns", "gsl_median_ns",
            "speedup_gsl_over_numerix", "eligible_for_direct_ratio",
            "execution_order", "randomization_seed", "reason"
        ])
        for (run_id, pair_id), libraries in pairs.items():
            if set(libraries) != {"NUMERIX", "GSL"}:
                continue
            numerix = libraries["NUMERIX"]
            gsl = libraries["GSL"]
            same_algorithm = (
                numerix["comparison_mode"] == "same_algorithm" and
                gsl["comparison_mode"] == "same_algorithm" and
                numerix["method"] == gsl["method"] and
                numerix["phase"] == gsl["phase"]
            )
            successful = all(
                row["status"] == "0" and row["accuracy_pass"] in {"1", "-1"}
                for row in (numerix, gsl)
            )
            eligible = same_algorithm and successful
            ratio = ""
            reason = "different comparison mode or method"
            if eligible:
                numerator = float(gsl["median_ns"])
                denominator = float(numerix["median_ns"])
                if denominator > 0.0:
                    ratio = numerator / denominator
                    reason = "same-algorithm candidate; see protocol and accuracy records"
                else:
                    eligible = False
                    reason = "zero NUMERIX timing median"
            writer.writerow([
                2, run_id, pair_id, numerix["configuration_id"],
                numerix["category"], numerix["case"], numerix["input_size"],
                numerix["method"], numerix["phase"], numerix["comparison_mode"],
                numerix["median_ns"], gsl["median_ns"], ratio, int(eligible),
                numerix["execution_order"], numerix["randomization_seed"], reason
            ])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--samples", type=int, default=21)
    parser.add_argument("--seed", type=int, default=20261112)
    parser.add_argument("--compiler", default="gcc")
    parser.add_argument("--case")
    parser.add_argument("--library", choices=("NUMERIX", "GSL"))
    parser.add_argument("--gsl-source", type=Path)
    parser.add_argument("--gsl-flags", default="unknown")
    args = parser.parse_args()
    if args.samples < 3 or args.samples > 101 or args.samples % 2 == 0:
        parser.error("--samples must be an odd number from 3 through 101")
    if args.seed < 0 or args.seed >= 2**64:
        parser.error("--seed must fit an unsigned 64-bit integer")

    repository_root = Path(__file__).resolve().parents[1]
    run_id = uuid.uuid4().hex
    output_request = args.output or (
        repository_root / "benchmarks/results/phase13b" / run_id)
    try:
        output = safe_output_directory(output_request, repository_root)
    except ValueError as error:
        parser.error(str(error))
    build = args.build_dir.resolve()
    executable_name = "numerix_comparison_benchmark.exe" if os.name == "nt" else \
        "numerix_comparison_benchmark"
    binary = build / executable_name
    if not binary.is_file():
        parser.error(f"benchmark executable not found: {binary}")

    version_record = capture([str(binary), "--versions"])
    environment = {
        "schema_version": 2,
        "run_id": run_id,
        "recorded_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "platform": platform.platform(),
        "os_release": file_contents("/etc/os-release"),
        "cpu": capture(["lscpu"]),
        "ram": capture(["free", "-b"]),
        "cpu_affinity": sorted(os.sched_getaffinity(0))
        if hasattr(os, "sched_getaffinity") else None,
        "load_average": os.getloadavg() if hasattr(os, "getloadavg") else None,
        "compiler": capture([args.compiler, "--version"]),
        "versions": version_record,
        "git_head": capture(["git", "rev-parse", "HEAD"]),
        "git_status": capture(["git", "status", "--short"]),
        "randomization_seed": args.seed,
        "requested_timing_samples_per_workload": args.samples,
        "case_filter": args.case,
        "library_filter": args.library,
        "timer": "CLOCK_MONOTONIC on POSIX; QueryPerformanceCounter on Windows",
        "timer_resolution": "host-dependent; not queried; batches are calibrated to at least 2 ms",
        "timing_policy": {
            "warmup": "one untimed warm-up operation per library/workload",
            "calibration": "powers-of-two batches until at least 2 ms, capped at 1,048,576 operations",
            "measured_batches": args.samples,
            "accuracy_probes": "one separate untimed deterministic probe after each timing batch",
            "pair_order": "counterbalanced in adjacent pairs; seed parity selects each block's starting library",
            "wall_clock": "monotonic wall-clock time; CPU time is not captured",
            "summary": "sample standard deviation; median by middle value or average of middle pair; p95 nearest rank ceil(0.95*n)",
            "outliers": "retained; no outlier removal",
        },
        "gsl_compile_flags": args.gsl_flags,
        "cmake_cache": file_contents(build / "CMakeCache.txt"),
        "numerix_compile_flags": file_contents(build / "CMakeFiles/numerix.dir/flags.make"),
        "benchmark_compile_flags": file_contents(
            build / "CMakeFiles/numerix_comparison_benchmark.dir/flags.make"),
        "benchmark_link": file_contents(
            build / "CMakeFiles/numerix_comparison_benchmark.dir/link.txt"),
        "binary_sha256": sha256(binary),
        "library_source_sha256": {
            str(path.relative_to(repository_root)): sha256(path)
            for path in sorted((repository_root / "src").rglob("*.c"))
        },
        "benchmark_source_sha256": {
            str(path.relative_to(repository_root)): sha256(path)
            for path in (
                repository_root / "benchmarks/comparison.c",
                repository_root / "benchmarks/measurement.c",
                repository_root / "benchmarks/measurement.h",
            )
        },
    }
    if args.gsl_source:
        environment["gsl_source_sha256"] = sha256(args.gsl_source)
    (output / "environment.json").write_text(
        json.dumps(environment, indent=2) + "\n")

    command = [
        str(binary), "--samples", str(args.samples),
        "--seed", str(args.seed), "--output-dir", str(output),
        "--run-id", run_id
    ]
    if args.case:
        command.extend(["--case", args.case])
    if args.library:
        command.extend(["--library", args.library])
    with (output / "measurements.csv").open("x") as result_file:
        subprocess.run(command, stdout=result_file, check=True)

    subprocess.run(
        [sys.executable, str(repository_root / "scripts/verify_benchmark_results.py"),
         str(output)], check=True)
    write_comparisons(output)
    subprocess.run(
        [sys.executable, str(repository_root / "scripts/summarize_benchmarks.py"),
         str(output)], check=True)
    print(f"Recorded schema v2 run {run_id} in {output}")


if __name__ == "__main__":
    main()
