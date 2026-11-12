#!/usr/bin/env python3
"""Build and run one reproducible Phase 14C linear-algebra methodology pilot."""

from __future__ import annotations

import csv
import datetime as dt
import hashlib
import json
import os
import platform
import re
import shlex
import subprocess
import sys
from pathlib import Path


SCRIPT = Path(__file__).resolve()
MODULE_DIR = SCRIPT.parent
REPO = SCRIPT.parents[3]
PILOT_ROOT = MODULE_DIR / "pilot_runs"
OPENBLAS = Path("/home/142401034/.local/numerix-deps/prefix/phase14/openblas")
LAPACK = Path("/home/142401034/.local/numerix-deps/prefix/phase14/lapack-reference")
OPENBLAS_VERSION = "0.3.34"
LAPACK_VERSION = "3.12.0"  # Installed package metadata; see version note below.
SEED = 2026111201


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], *, env: dict[str, str], cwd: Path = REPO) -> str:
    print("$", shlex.join(command), flush=True)
    return subprocess.check_output(command, cwd=cwd, env=env, text=True, stderr=subprocess.STDOUT)


def command_record(command: list[str], log: list[dict[str, str]], env: dict[str, str]) -> None:
    output = run(command, env=env)
    log.append({"command": shlex.join(command), "output": output.strip()})


def current_git() -> tuple[str, bool]:
    head = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=REPO, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain"], cwd=REPO, text=True).strip())
    return head, dirty


def os_info() -> dict[str, object]:
    model = "unknown"
    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.exists():
        for line in cpuinfo.read_text(errors="replace").splitlines():
            if line.lower().startswith("model name"):
                model = line.split(":", 1)[-1].strip()
                break
    release = {}
    os_release = Path("/etc/os-release")
    if os_release.exists():
        for line in os_release.read_text(errors="replace").splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                release[key] = value.strip('"')
    return {
        "platform": platform.platform(),
        "uname": list(platform.uname()),
        "os_release": release,
        "libc": list(platform.libc_ver()),
        "cpu_model": model,
        "logical_cpu_count": os.cpu_count(),
    }


def write_manifest(output: Path) -> list[dict[str, str]]:
    rows = [
        {
            "problem_id": "LA-PILOT-ADD-256-INT-v1",
            "module": "linear_algebra",
            "family": "bounded signed integer matrix elementwise",
            "mathematical_definition": "C=A+B elementwise",
            "serialized_parameters": json.dumps({"rows": 256, "columns": 256, "A": "((7*i)%31)-15", "B": "((13*i)%23)-11"}, separators=(",", ":")),
            "problem_size": "256x256",
            "applicable_public_methods": "numerix_matrix_add",
            "reference_value": "exact int64 elementwise sum",
            "reference_method": "independent integer formula",
            "reference_precision": "exact int64 for bounded integral values",
            "target_absolute_accuracy": "1e-12",
            "target_relative_accuracy": "1e-12",
            "comparison_equivalence": "direct operation family",
            "input_sha256": input_hash_add(),
        },
        {
            "problem_id": "LA-PILOT-GEMM-64-INT-v1",
            "module": "linear_algebra",
            "family": "bounded signed integer square matrix product",
            "mathematical_definition": "C=A*B",
            "serialized_parameters": json.dumps({"m": 64, "n": 64, "k": 64, "A": "((i+3*j)%17)-8", "B": "((5*i+j)%13)-6"}, separators=(",", ":")),
            "problem_size": "64x64x64",
            "applicable_public_methods": "numerix_matrix_multiply",
            "reference_value": "exact int64 matrix product",
            "reference_method": "independent integer triple loop",
            "reference_precision": "exact int64 sums for bounded inputs",
            "target_absolute_accuracy": "1e-12",
            "target_relative_accuracy": "1e-12",
            "comparison_equivalence": "direct operation family",
            "input_sha256": input_hash_gemm(),
        },
        {
            "problem_id": "LA-PILOT-LUFACTOR-64-BLOCKPIVOT-v1",
            "module": "linear_algebra",
            "family": "pivot-requiring block-diagonal nonsingular system",
            "mathematical_definition": "P*A=L*U",
            "serialized_parameters": json.dumps({"n": 64, "A_block": [[0, 2], [1, 3]], "remaining": "identity"}, separators=(",", ":")),
            "problem_size": "64x64",
            "applicable_public_methods": "numerix_lu_decompose",
            "reference_value": "factorization identity; exact integer input A",
            "reference_method": "independent P*A-L*U reconstruction",
            "reference_precision": "long double product of binary64 factors",
            "target_absolute_accuracy": "1e-12",
            "target_relative_accuracy": "1e-12",
            "comparison_equivalence": "best available: scaled vs partial pivoting",
            "input_sha256": input_hash_lu_matrix(),
        },
        {
            "problem_id": "LA-PILOT-LUSOLVE-64-BLOCKPIVOT-v1",
            "module": "linear_algebra",
            "family": "known-solution pivot-requiring linear system",
            "mathematical_definition": "A*x=b",
            "serialized_parameters": json.dumps({"n": 64, "A_block": [[0, 2], [1, 3]], "remaining": "identity", "x": [1, 2] + [1] * 62}, separators=(",", ":")),
            "problem_size": "64x64; 1 RHS",
            "applicable_public_methods": "numerix_lu_solve",
            "reference_value": "known exact x=(1,2,1,...,1)",
            "reference_method": "analytical solution for integer block matrix",
            "reference_precision": "exact integer A,b and known solution",
            "target_absolute_accuracy": "1e-12",
            "target_relative_accuracy": "1e-12",
            "comparison_equivalence": "direct LU solve family with saved factors",
            "input_sha256": input_hash_lu(),
        },
    ]
    with (output / "workload_manifest.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    return rows


def f64_bytes(values: list[float]) -> bytes:
    import struct
    return b"".join(struct.pack("<d", value) for value in values)


def input_hash_add() -> str:
    a = [float((7 * i) % 31 - 15) for i in range(256 * 256)]
    b = [float((13 * i) % 23 - 11) for i in range(256 * 256)]
    return hashlib.sha256(f64_bytes(a) + f64_bytes(b)).hexdigest()


def input_hash_gemm() -> str:
    n = 64
    a = [float((i + 3 * j) % 17 - 8) for i in range(n) for j in range(n)]
    b = [float((5 * i + j) % 13 - 6) for i in range(n) for j in range(n)]
    return hashlib.sha256(f64_bytes(a) + f64_bytes(b)).hexdigest()


def input_hash_lu() -> str:
    return hashlib.sha256(f64_bytes(lu_matrix_values()) + f64_bytes([4.0, 7.0] + [1.0] * 62)).hexdigest()


def input_hash_lu_matrix() -> str:
    return hashlib.sha256(f64_bytes(lu_matrix_values())).hexdigest()


def lu_matrix_values() -> list[float]:
    n = 64
    a = [0.0] * (n * n)
    a[1] = 2.0
    a[n] = 1.0
    a[n + 1] = 3.0
    for i in range(2, n):
        a[i * n + i] = 1.0
    return a


def write_memory_unavailable(output: Path, run_id: str) -> None:
    routes = [
        ("LA-PILOT-ADD-256-INT-v1", "NUMERIX", "0.2.0-rc1", "numerix_matrix_add", "cblas_dcopy+cblas_daxpy", "direct"),
        ("LA-PILOT-ADD-256-INT-v1", "OpenBLAS", OPENBLAS_VERSION, "numerix_matrix_add", "cblas_dcopy+cblas_daxpy", "direct"),
        ("LA-PILOT-GEMM-64-INT-v1", "NUMERIX", "0.2.0-rc1", "numerix_matrix_multiply", "cblas_dgemm_row_major", "direct"),
        ("LA-PILOT-GEMM-64-INT-v1", "OpenBLAS", OPENBLAS_VERSION, "numerix_matrix_multiply", "cblas_dgemm_row_major", "direct"),
        ("LA-PILOT-LUFACTOR-64-BLOCKPIVOT-v1", "NUMERIX", "0.2.0-rc1", "numerix_lu_decompose", "LAPACKE_dgetrf_col_major", "best_available_scaled_vs_partial_pivot"),
        ("LA-PILOT-LUFACTOR-64-BLOCKPIVOT-v1", "LAPACK+OpenBLAS", f"LAPACK {LAPACK_VERSION} + OpenBLAS {OPENBLAS_VERSION}", "numerix_lu_decompose", "LAPACKE_dgetrf_col_major_with_row_major_conversion", "best_available_scaled_vs_partial_pivot"),
        ("LA-PILOT-LUSOLVE-64-BLOCKPIVOT-v1", "NUMERIX", "0.2.0-rc1", "numerix_lu_solve", "LAPACKE_dgetrs", "direct_lu_solve_family"),
        ("LA-PILOT-LUSOLVE-64-BLOCKPIVOT-v1", "LAPACK+OpenBLAS", f"LAPACK {LAPACK_VERSION} + OpenBLAS {OPENBLAS_VERSION}", "numerix_lu_solve", "LAPACKE_dgetrs_precomputed_factors", "direct_lu_solve_family"),
    ]
    fields = ["run_id", "module", "problem_id", "library", "library_version", "numerix_method", "competitor_method", "comparison_class", "measurement_status", "peak_additional_live_requested_bytes", "total_requested_bytes", "allocation_count", "reallocation_count", "free_count", "instrumentation_method", "setup_operation_scope", "sample_isolation", "reason"]
    with (output / "raw_memory.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for problem_id, library, version, method, competitor, klass in routes:
            writer.writerow({
                "run_id": run_id, "module": "linear_algebra", "problem_id": problem_id,
                "library": library, "library_version": version, "numerix_method": method,
                "competitor_method": competitor, "comparison_class": klass,
                "measurement_status": "unavailable", "peak_additional_live_requested_bytes": "",
                "total_requested_bytes": "", "allocation_count": "", "reallocation_count": "",
                "free_count": "", "instrumentation_method": "not run",
                "setup_operation_scope": "method call/output scope; not measured",
                "sample_isolation": "not run",
                "reason": "This pilot validates timing, dispatch, accuracy, and layout only; allocation interception was not run, so memory is unavailable, not zero.",
            })


def summarize_timing(timing_path: Path, output: Path) -> None:
    buckets: dict[tuple[str, str, str, str], list[float]] = {}
    with timing_path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            key = (row["problem_id"], row["library"], row["library_version"], row["competitor_method"])
            buckets.setdefault(key, []).append(float(row["monotonic_wall_elapsed_seconds"]) / int(row["batch_size"]))
    fields = ["problem_id", "library", "library_version", "competitor_method", "timing_batches", "median_seconds_per_call", "minimum_seconds_per_call", "maximum_seconds_per_call"]
    with (output / "per_problem_runtime_summary.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for (problem_id, library, version, method), values in sorted(buckets.items()):
            ordered = sorted(values)
            writer.writerow({
                "problem_id": problem_id, "library": library, "library_version": version,
                "competitor_method": method, "timing_batches": len(values),
                "median_seconds_per_call": ordered[len(ordered) // 2],
                "minimum_seconds_per_call": ordered[0], "maximum_seconds_per_call": ordered[-1],
            })


def validate_outputs(output: Path) -> dict[str, object]:
    with (output / "raw_timing.csv").open(newline="") as stream:
        timing = list(csv.DictReader(stream))
    with (output / "raw_accuracy.csv").open(newline="") as stream:
        accuracy = list(csv.DictReader(stream))
    with (output / "raw_setup_timing.csv").open(newline="") as stream:
        setup = list(csv.DictReader(stream))
    with (output / "workload_manifest.csv").open(newline="") as stream:
        manifest = list(csv.DictReader(stream))
    groups: dict[tuple[str, str, str], list[dict[str, str]]] = {}
    for row in timing:
        key = (row["problem_id"], row["library"], row["competitor_method"])
        groups.setdefault(key, []).append(row)
    by_problem_batch: dict[str, dict[str, set[str]]] = {}
    for row in timing:
        by_problem_batch.setdefault(row["problem_id"], {}).setdefault(row["batch_index"], set()).add(row["batch_order"])
    invalid = []
    for key, rows in groups.items():
        if len(rows) != 31:
            invalid.append(f"{key}: expected 31 rows, got {len(rows)}")
        for row in rows:
            if float(row["monotonic_wall_elapsed_seconds"]) <= 0 or float(row["cpu_elapsed_seconds"]) < 0:
                invalid.append(f"{key}: nonpositive/invalid timer")
            if row["status"] != "success" or row["calls_completed"] != row["batch_size"]:
                invalid.append(f"{key}: failed calls/status")
    if len(accuracy) != 8 or any(row["accuracy_pass"] != "true" for row in accuracy):
        invalid.append("accuracy record count or pass status mismatch")
    if len(set(row["problem_id"] for row in manifest)) != len(manifest) or len(manifest) != 4:
        invalid.append("workload manifest IDs are duplicate or incomplete")
    if len(set(row["problem_id"] for row in accuracy)) != 4:
        invalid.append("accuracy is missing a problem definition")
    for problem_id, batches in by_problem_batch.items():
        if any(len(orders) != 1 for orders in batches.values()):
            invalid.append(f"{problem_id}: route order disagrees within a batch")
        order_counts = {}
        for orders in batches.values():
            if len(orders) == 1:
                order = next(iter(orders))
                order_counts[order] = order_counts.get(order, 0) + 1
        if len(order_counts) != 2 or max(order_counts.values()) - min(order_counts.values()) > 1:
            invalid.append(f"{problem_id}: route order is not counterbalanced ({order_counts})")
    return {
        "pilot_only": True,
        "passed": not invalid,
        "errors": invalid,
        "manifest_problems": len(manifest),
        "timing_rows": len(timing),
        "timing_routes": len(groups),
        "timing_batches_per_route": sorted(set(len(rows) for rows in groups.values())),
        "accuracy_rows": len(accuracy),
        "accuracy_pass_rows": sum(row["accuracy_pass"] == "true" for row in accuracy),
        "setup_rows": len(setup),
        "setup_pass_rows": sum(row["status"] == "success" for row in setup),
        "memory_rows": 8,
        "memory_measurement_status": "unavailable",
    }


def verify_api_inventory() -> dict[str, object]:
    inventory_path = MODULE_DIR / "linear_algebra_method_inventory.csv"
    header = (REPO / "include/numerix.h").read_text()
    with inventory_path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    symbols = [symbol for row in rows for symbol in row["public_api"].split(" / ")]
    missing = [symbol for symbol in symbols if not re.search(r"\b" + re.escape(symbol) + r"\s*\(", header)]
    result = {
        "inventory_path": str(inventory_path.relative_to(REPO)),
        "public_header": "include/numerix.h",
        "inventory_rows": len(rows),
        "numerical_rows": sum(row["kind"] == "numerical" for row in rows),
        "lifecycle_rows": sum(row["kind"] == "lifecycle" for row in rows),
        "api_symbols_checked": len(symbols),
        "missing_public_declarations": missing,
        "passed": not missing and len(rows) == 21,
    }
    return result


def main() -> int:
    for dependency_path in (OPENBLAS, LAPACK):
        if not dependency_path.exists():
            raise SystemExit(f"required pinned dependency prefix missing: {dependency_path}")

    env = os.environ.copy()
    env["OPENBLAS_NUM_THREADS"] = "1"
    env["OMP_NUM_THREADS"] = "1"
    env["LD_LIBRARY_PATH"] = f"{OPENBLAS / 'lib'}:{env.get('LD_LIBRARY_PATH', '')}"
    env["PKG_CONFIG_PATH"] = f"{OPENBLAS / 'lib/pkgconfig'}:{LAPACK / 'lib64/pkgconfig'}:{env.get('PKG_CONFIG_PATH', '')}"

    log: list[dict[str, str]] = []
    build_dir = MODULE_DIR / "build" / "numerix-gcc-release"
    cmake_configure = ["cmake", "-S", str(REPO), "-B", str(build_dir),
                       "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_C_COMPILER=gcc",
                       "-DCMAKE_C_FLAGS=-Werror"]
    command_record(cmake_configure, log, env)
    cmake_build = ["cmake", "--build", str(build_dir), "--target", "numerix", "--parallel", "4"]
    command_record(cmake_build, log, env)

    binary = MODULE_DIR / "build" / "linear_algebra_pilot"
    source = MODULE_DIR / "linear_algebra_pilot.c"
    num_archive = build_dir / "libnumerix.a"
    compile_command = [
        "gcc", "-std=c11", "-O3", "-DNDEBUG", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-I", str(REPO / "include"), "-I", str(LAPACK / "include"), "-I", str(OPENBLAS / "include"),
        str(source), str(num_archive), "-L", str(LAPACK / "lib64"), "-llapacke", "-llapack",
        "-L", str(OPENBLAS / "lib"), f"-Wl,-rpath,{OPENBLAS / 'lib'}", "-lopenblas",
        "-lgfortran", "-lpthread", "-lm", "-ldl", "-o", str(binary),
    ]
    command_record(compile_command, log, env)

    versions = subprocess.check_output(
        ["pkg-config", "--modversion", "openblas", "lapack", "lapacke"],
        cwd=REPO, env=env, text=True,
    ).splitlines()
    if versions != [OPENBLAS_VERSION, LAPACK_VERSION, LAPACK_VERSION]:
        raise SystemExit(f"pinned package metadata mismatch; found {versions}")

    start = dt.datetime.now(dt.timezone.utc)
    run_id = "phase14c-la-pilot-" + start.strftime("%Y%m%dT%H%M%SZ")
    output = PILOT_ROOT / run_id
    output.mkdir(parents=True, exist_ok=False)
    manifest_rows = write_manifest(output)
    manifest_path = output / "workload_manifest.csv"
    manifest_hash = sha256(manifest_path)
    command = [str(binary), run_id, str(output / "raw_timing.csv"),
               str(output / "raw_accuracy.csv"), str(output / "raw_setup_timing.csv")]
    command_record(command, log, env)
    write_memory_unavailable(output, run_id)
    summarize_timing(output / "raw_timing.csv", output)
    verification = validate_outputs(output)
    api_verification = verify_api_inventory()
    verification["api_inventory"] = api_verification
    verification["passed"] = verification["passed"] and api_verification["passed"]
    if not api_verification["passed"]:
        verification["errors"].append("public API inventory/header verification failed")
    verification["run_id"] = run_id
    verification["workload_manifest_sha256"] = manifest_hash
    verification["input_problem_count"] = len(manifest_rows)
    (output / "verification.json").write_text(json.dumps(verification, indent=2) + "\n")

    git_head, git_dirty = current_git()
    openblas = __import__("ctypes").CDLL(str(OPENBLAS / "lib/libopenblas.so"))
    openblas.openblas_get_config.restype = __import__("ctypes").c_char_p
    openblas.openblas_get_num_threads.restype = __import__("ctypes").c_int
    openblas.openblas_set_num_threads.argtypes = [__import__("ctypes").c_int]
    openblas.openblas_set_num_threads(1)
    if openblas.openblas_get_num_threads() != 1:
        raise SystemExit("host process could not confirm OpenBLAS single-thread setting")
    end = dt.datetime.now(dt.timezone.utc)
    metadata = {
        "run_id": run_id,
        "pilot_only": True,
        "utc_start": start.isoformat(),
        "utc_end": end.isoformat(),
        "git_head": git_head,
        "git_dirty": git_dirty,
        "operating_environment": os_info(),
        "compiler": subprocess.check_output(["gcc", "--version"], text=True).splitlines()[0],
        "cmake_version": subprocess.check_output(["cmake", "--version"], text=True).splitlines()[0],
        "build_commands": [{"command": item["command"], "output": item["output"]} for item in log[:3]],
        "measurement_invocation": shlex.join(command),
        "dependencies": {
            "openblas": {"version": OPENBLAS_VERSION, "config": openblas.openblas_get_config().decode(), "runtime_threads": openblas.openblas_get_num_threads(), "prefix": str(OPENBLAS)},
            "lapack": {"version": LAPACK_VERSION, "metadata_source": str(LAPACK / "lib64/pkgconfig/lapack.pc"), "prefix": str(LAPACK)},
            "lapacke": {"version": LAPACK_VERSION, "metadata_source": str(LAPACK / "lib64/pkgconfig/lapacke.pc")},
            "numerix": "0.2.0-rc1",
        },
        "lapack_version_note": "Coordinator's initial note said LAPACK 3.12.1; pinned installed pkg-config and CMake metadata both report 3.12.0. Pilot records the installed metadata version.",
        "thread_environment": {"OPENBLAS_NUM_THREADS": env["OPENBLAS_NUM_THREADS"], "OMP_NUM_THREADS": env["OMP_NUM_THREADS"]},
        "timer": {"wall": "clock_gettime(CLOCK_MONOTONIC_RAW)", "cpu": "clock_gettime(CLOCK_THREAD_CPUTIME_ID)", "target_batch_seconds": TARGET_BATCH_SECONDS, "batches_per_route": 31, "warmup_calls_per_route_including_calibration": 6, "library_order": "deterministic seeded Fisher-Yates by problem/batch; order written to each row", "random_seed": SEED},
        "workload_manifest_sha256": manifest_hash,
        "adapter_sha256": sha256(source),
        "runner_sha256": sha256(SCRIPT),
        "command_log": log,
        "verification": verification,
        "limitations": [
            "Four deterministic pilot cases are not the required 100 distinct linear-algebra workloads.",
            "Only matrix addition, matrix multiplication, LU factorization and LU solve have pilot adapters.",
            "Memory instrumentation was not run; all raw memory statuses are unavailable, not zero.",
            "LU factorization compares NUMERIX scaled partial pivoting with LAPACK partial pivoting (best available).",
            "DGETRF end-to-end rows include explicit row-major to column-major conversion and allocations.",
            "LU solve is steady-state with each library's own saved factors; one-time factorization/setup times are saved separately.",
            "All measurements are for one machine and are pilot methodology evidence, not general performance conclusions.",
        ],
        "output_sha256": {
            path.name: sha256(path)
            for path in output.iterdir()
            if path.is_file() and path.name != "environment.json"
        },
    }
    (output / "environment.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({"output": str(output), "verification": verification}, indent=2))
    return 0 if verification["passed"] else 1


TARGET_BATCH_SECONDS = 0.002

if __name__ == "__main__":
    sys.exit(main())
