#!/usr/bin/env python3
"""Run randomized, paired Phase 14 native-library measurements."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import platform
import random
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "benchmarks/problems/phase14/workloads.csv"
INPUTS = ROOT / "benchmarks/problems/phase14/problem_inputs.jsonl"
BUILD = ROOT / "build/phase14-adapters"
SEED = 20261112
LIBRARIES = {
    "root": ["NUMERIX", "GSL", "CMINPACK"],
    "linear_algebra": ["NUMERIX", "GSL", "LAPACK_REFERENCE", "LAPACK_OPENBLAS"],
    "differentiation": ["NUMERIX", "GSL"],
    "interpolation": ["NUMERIX", "GSL"],
    "integration": ["NUMERIX", "GSL"],
    "optimization": ["NUMERIX", "GSL", "NLOPT"],
    "ode": ["NUMERIX", "GSL", "SUNDIALS_ARKODE", "SUNDIALS_CVODE"],
}
EXECUTABLE = {
    "LAPACK_REFERENCE": "phase14_runner_reference_lapack",
    "LAPACK_OPENBLAS": "phase14_runner_openblas",
}
BATCH_SIZE = {
    "root": 32,
    "linear_algebra": 8,
    "differentiation": 64,
    "interpolation": 8,
    "integration": 32,
    "optimization": 32,
    "ode": 4,
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def command_output(args: list[str]) -> str:
    try:
        return subprocess.run(args, check=False, text=True, capture_output=True).stdout.strip()
    except OSError as exc:
        return f"unavailable: {exc}"


def git_state() -> dict[str, str]:
    return {
        "head": command_output(["git", "rev-parse", "HEAD"]),
        "branch": command_output(["git", "branch", "--show-current"]),
        "status_porcelain": command_output(["git", "status", "--porcelain"]),
    }


def read_rows() -> list[dict[str, str]]:
    csv.field_size_limit(sys.maxsize)
    with MANIFEST.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, help="Phase 14 result directory")
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--batches", type=int, default=21)
    parser.add_argument("--batch-size", type=int, help="override the preregistered per-module repeat count")
    parser.add_argument("--seed", type=int, default=SEED)
    parser.add_argument("--modules", nargs="+", choices=sorted(LIBRARIES), help="limit a pilot to selected modules")
    parser.add_argument("--memory", action="store_true", help="collect isolated-process peak RSS for every pair")
    parser.add_argument("--pilot", action="store_true", help="mark this run as a pilot in metadata")
    args = parser.parse_args()
    if args.batches < 21:
        parser.error("at least 21 timing batches are required")
    if args.batch_size is not None and args.batch_size < 1:
        parser.error("batch size must be positive")

    started_at = datetime.now(timezone.utc).isoformat()
    output = (ROOT / args.output_dir).resolve()
    output.mkdir(parents=True, exist_ok=True)
    if any((output / name).exists() for name in ("raw_timing.csv", "accuracy.csv", "memory.csv")):
        parser.error("output directory already contains benchmark measurements; select a fresh Phase 14 run directory")
    rows = read_rows()
    selected_rows = [row for row in rows if not args.modules or row["module"] in args.modules]
    rng = random.Random(args.seed)
    plans: list[tuple[dict[str, str], str, int]] = []
    for problem in selected_rows:
        libraries = list(LIBRARIES[problem["module"]])
        declared = set(problem["eligible_libraries"].lower().split("|"))
        aliases = {
            "NUMERIX": "numerix", "GSL": "gsl", "CMINPACK": "cminpack",
            "LAPACK_REFERENCE": "lapack_reference", "LAPACK_OPENBLAS": "lapack_openblas",
            "NLOPT": "nlopt", "SUNDIALS_ARKODE": "sundials", "SUNDIALS_CVODE": "sundials",
        }
        libraries = [lib for lib in libraries if aliases[lib] in declared]
        rng.shuffle(libraries)
        plans.extend((problem, library, order) for order, library in enumerate(libraries, 1))

    order_path = output / "execution_order.csv"
    with order_path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(["run_id", "random_seed", "problem_id", "module", "order_within_problem", "library"])
        writer.writerows([args.run_id, args.seed, p["problem_id"], p["module"], order, lib]
                         for p, lib, order in plans)

    environment = os.environ.copy()
    environment.update({
        "OPENBLAS_NUM_THREADS": "1",
        "OMP_NUM_THREADS": "1",
        "MKL_NUM_THREADS": "1",
        "BLIS_NUM_THREADS": "1",
        "VECLIB_MAXIMUM_THREADS": "1",
    })
    failures: list[dict[str, str | int]] = []
    total = len(plans)
    for index, (problem, library, order) in enumerate(plans, 1):
        executable = EXECUTABLE.get(library, "phase14_runner")
        command = [
            str(BUILD / executable), str(INPUTS), problem["problem_id"], library,
            str(args.batches), str(args.batch_size or BATCH_SIZE[problem["module"]]),
            str(output), args.run_id,
        ]
        result = subprocess.run(command, cwd=ROOT, env=environment,
                                text=True, capture_output=True)
        if result.returncode:
            failures.append({
                "problem_id": problem["problem_id"], "module": problem["module"],
                "library": library, "exit_code": result.returncode,
                "stderr": result.stderr.strip() or "NA",
            })
        if args.memory:
            memory_command = command[:-1] + [args.run_id, "--memory"]
            memory_result = subprocess.run(memory_command, cwd=ROOT, env=environment,
                                           text=True, capture_output=True)
            if memory_result.returncode:
                failures.append({
                    "problem_id": problem["problem_id"], "module": problem["module"],
                    "library": library, "exit_code": memory_result.returncode,
                    "stderr": memory_result.stderr.strip() or "memory probe failed",
                })
        if index % 100 == 0 or index == total:
            print(f"{index}/{total} problem-library pairs complete", flush=True)

    metadata = {
        "schema_version": "phase14-run-metadata-v1",
        "run_id": args.run_id,
        "run_kind": "pilot" if args.pilot else "main",
        "started_utc": started_at,
        "finished_utc": datetime.now(timezone.utc).isoformat(),
        "seed": args.seed,
        "timing_batches": args.batches,
        "batch_size_by_module": {m: args.batch_size or n for m, n in BATCH_SIZE.items()},
        "randomized_library_order_within_problem": True,
        "pair_count_planned": total,
        "pair_count_failed_to_launch_or_write": len(failures),
        "problem_count": len(rows),
        "problems_in_run": len(selected_rows),
        "machine": platform.node(),
        "system": platform.platform(),
        "processor": platform.processor() or command_output(["lscpu"]),
        "cpu_count_logical": os.cpu_count(),
        "python": sys.version,
        "compiler": command_output(["gcc", "--version"]).splitlines()[0],
        "compiler_flags": "-O3 -DNDEBUG -Wall -Wextra -Wpedantic; dependency flags recorded in library manifest",
        "thread_environment": {key: environment[key] for key in (
            "OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS",
            "BLIS_NUM_THREADS", "VECLIB_MAXIMUM_THREADS")},
        "git": git_state(),
        "sha256": {
            "adapter_source": sha256(ROOT / "benchmarks/adapters/phase14/phase14_runner.c"),
            "json_reader": sha256(ROOT / "benchmarks/adapters/phase14/phase14_json.h"),
            "adapter_cmake": sha256(ROOT / "benchmarks/adapters/phase14/CMakeLists.txt"),
            "manifest": sha256(MANIFEST),
            "input_sidecar": sha256(INPUTS),
            "runner_script": sha256(Path(__file__)),
            "library_manifest": sha256(ROOT / "benchmarks/analysis/phase14/libraries.csv"),
            "method_mapping": sha256(ROOT / "benchmarks/analysis/phase14/method_mapping.csv"),
        },
        "memory_requested": args.memory,
        "measurements": {
            "wall_clock": "CLOCK_MONOTONIC, elapsed seconds per batch",
            "cpu_clock": "CLOCK_PROCESS_CPUTIME_ID, elapsed seconds per batch",
            "rss": "getrusage(RUSAGE_SELF).ru_maxrss in isolated process; includes harness and shared-library baseline",
            "requested_heap": "unavailable; not encoded as zero",
        },
    }
    (output / "run_metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    with (output / "execution_failures.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=["problem_id", "module", "library", "exit_code", "stderr"], lineterminator="\n")
        writer.writeheader()
        writer.writerows(failures)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
