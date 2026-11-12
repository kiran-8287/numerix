#!/usr/bin/env python3
"""Run and archive the isolated one-case optimization correctness pilot."""

from __future__ import annotations

import csv
import hashlib
import json
import os
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
PILOT_DIR = ROOT / "benchmarks/phase14c/pilots/optimization"
RUN_ID = "phase14c-opt-correctness-pilot-20261112"
EXECUTABLE = ROOT / "build/phase14c-optimization-pilot/optimizer_correctness_pilot"
NLOPT_PREFIX = Path(
    "/home/142401034/.local/numerix-deps/prefix/phase14c/nlopt-2.10.1"
)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def digest_text(value: str) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()


def output(argv: list[str]) -> str:
    result = subprocess.run(argv, cwd=ROOT, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"command exited {result.returncode}: {' '.join(argv)}\n{result.stderr}"
        )
    return result.stdout.strip()


def main() -> None:
    manifest = PILOT_DIR / "workload_manifest.csv"
    accuracy_path = PILOT_DIR / "raw_accuracy_pilot.csv"
    metadata_path = PILOT_DIR / "run_metadata.json"
    if not manifest.is_file() or not EXECUTABLE.is_file():
        raise FileNotFoundError("Build the optimizer pilot and freeze its manifest first")

    with manifest.open(newline="", encoding="utf-8") as manifest_file:
        manifest_rows = list(csv.DictReader(manifest_file))
    if len(manifest_rows) != 2:
        raise ValueError(f"expected two frozen pilot workloads, got {len(manifest_rows)}")
    for row in manifest_rows:
        if digest_text(row["parameters_json"]) != row["input_data_sha256"]:
            raise ValueError(f"workload parameter checksum mismatch: {row['problem_id']}")
    manifest_ids = {row["problem_id"] for row in manifest_rows}

    started = datetime.now(timezone.utc)
    stdout = output([str(EXECUTABLE)])
    finished = datetime.now(timezone.utc)
    accuracy_path.write_text(stdout + "\n", encoding="utf-8")
    rows = list(csv.DictReader(accuracy_path.open(newline="", encoding="utf-8")))
    expected = {
        ("P14C-OPT-PILOT-GOLDEN-001", "NUMERIX"),
        ("P14C-OPT-PILOT-GOLDEN-001", "NLopt"),
        ("P14C-OPT-PILOT-GRADIENT-001", "NUMERIX"),
        ("P14C-OPT-PILOT-GRADIENT-001", "NLopt"),
    }
    observed = {(row["problem_id"], row["library"]) for row in rows}
    if len(rows) != 4 or observed != expected:
        raise ValueError(f"expected four per-library records, got {observed}")
    if {row["problem_id"] for row in rows} != manifest_ids:
        raise ValueError("pilot output problem IDs do not match the frozen manifest")
    for row in rows:
        metrics = json.loads(row["work_metrics_json"])
        budget = metrics["objective_budget"]
        if metrics["objective_evaluations"] > budget:
            raise ValueError(f"objective-call budget exceeded: {row}")
        if not row["native_status_name"]:
            raise ValueError(f"native termination status was lost: {row}")
        if row["library"] == "NLopt" and row["native_status"] == "5" and row["converged"] != "0":
            raise ValueError("NLOPT_MAXEVAL_REACHED must remain a non-converged limit status")

    git_head = output(["git", "rev-parse", "HEAD"])
    git_status = subprocess.run(
        ["git", "status", "--porcelain"], cwd=ROOT, check=True,
        capture_output=True, text=True
    ).stdout
    metadata = {
        "run_id": RUN_ID,
        "run_kind": "single-case correctness-only pilot",
        "started_utc": started.isoformat(),
        "finished_utc": finished.isoformat(),
        "git_head": git_head,
        "working_tree_dirty": bool(git_status),
        "system": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "cpu_count_logical": os.cpu_count(),
        "compiler": output(["gcc", "--version"]).splitlines()[0],
        "compiler_flags": "CMake Release; -Wall -Wextra -Wpedantic -Werror",
        "dependencies": {"NUMERIX": "0.2.0-rc1", "NLopt": "2.10.1"},
        "nlopt_prefix": str(NLOPT_PREFIX),
        "thread_policy": "single-process, deterministic methods; no worker threads",
        "random_seed": None,
        "timing_measured": False,
        "memory_measured": False,
        "objective_evaluation_budget": 200,
        "accuracy_targets": {"absolute": 1e-5, "relative": 1e-5},
        "problem_count": 2,
        "library_method_records": len(rows),
        "native_statuses_preserved": True,
        "method_comparison_class": "best_available for both routes",
        "limitations": [
            "One analytic test problem per optimization method; this is not a workload study.",
            "NUMERIX golden-section has no initial-point parameter; the NLopt DIRECT-L x argument is recorded but is not a shared controllable start.",
            "NUMERIX gradient descent is unbounded; the gradient pilot uses no box constraints in either implementation.",
            "NLopt maximum-evaluation termination is reported separately from accuracy_pass.",
            "No timing or method-specific memory observations were collected.",
        ],
        "invocation": [
            "cmake -S benchmarks/phase14c/optimization -B build/phase14c-optimization-pilot -DCMAKE_BUILD_TYPE=Release -DNLOPT_ROOT=<NLopt-2.10.1-prefix>",
            "cmake --build build/phase14c-optimization-pilot --parallel",
            "python3 benchmarks/phase14c/optimization/run_correctness_pilot.py",
        ],
        "sha256": {
            "workload_manifest": digest(manifest),
            "adapter_source": digest(ROOT / "benchmarks/phase14c/optimization/optimizer_correctness_pilot.c"),
            "adapter_cmake": digest(ROOT / "benchmarks/phase14c/optimization/CMakeLists.txt"),
            "runner_script": digest(Path(__file__).resolve()),
            "raw_accuracy": digest(accuracy_path),
        },
        "record_statuses": [
            {
                "problem_id": row["problem_id"],
                "library": row["library"],
                "native_status": row["native_status"],
                "native_status_name": row["native_status_name"],
                "normalized_status": row["normalized_status"],
                "converged": row["converged"] == "1",
                "accuracy_pass": row["accuracy_pass"] == "1",
                "objective_evaluations": json.loads(row["work_metrics_json"])["objective_evaluations"],
            }
            for row in rows
        ],
    }
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {accuracy_path.relative_to(ROOT)} ({len(rows)} rows)")
    print(f"wrote {metadata_path.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
