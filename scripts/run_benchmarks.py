#!/usr/bin/env python3
"""Record the benchmark environment, CSV observations, and conservative ratios."""
import argparse
import csv
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import subprocess


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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--samples", type=int, default=21)
    parser.add_argument("--compiler", default="gcc")
    parser.add_argument("--gsl-source", type=Path)
    parser.add_argument("--gsl-flags", default="unknown")
    args = parser.parse_args()
    build = args.build_dir.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    binary = build / "numerix_comparison_benchmark"
    environment = {
        "recorded_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "platform": platform.platform(),
        "os_release": file_contents("/etc/os-release"),
        "cpu": capture(["lscpu"]),
        "ram": capture(["free", "-b"]),
        "cpu_affinity": sorted(os.sched_getaffinity(0)) if hasattr(os, "sched_getaffinity") else None,
        "load_average": os.getloadavg() if hasattr(os, "getloadavg") else None,
        "compiler": capture([args.compiler, "--version"]),
        "versions": capture([str(binary), "--versions"]),
        "git_head": capture(["git", "rev-parse", "HEAD"]),
        "git_status": capture(["git", "status", "--short"]),
        "samples": args.samples,
        "timer": "CLOCK_MONOTONIC on POSIX; QueryPerformanceCounter on Windows",
        "method": "warm-up; powers-of-two batch >=2ms; independent untimed accuracy probes; sample means",
        "gsl_compile_flags": args.gsl_flags,
        "cmake_cache": file_contents(build / "CMakeCache.txt"),
        "numerix_compile_flags": file_contents(build / "CMakeFiles/numerix.dir/flags.make"),
        "benchmark_compile_flags": file_contents(build / "CMakeFiles/numerix_comparison_benchmark.dir/flags.make"),
        "benchmark_link": file_contents(build / "CMakeFiles/numerix_comparison_benchmark.dir/link.txt"),
        "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "library_source_sha256": {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                                  for path in sorted(Path("src").rglob("*.c"))},
        "source_sha256": {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                          for path in [Path("benchmarks/comparison.c"),
                                       Path("benchmarks/allocation_tracker.c")]},
    }
    if args.gsl_source:
        environment["gsl_source_sha256"] = hashlib.sha256(args.gsl_source.read_bytes()).hexdigest()
    (args.output / "environment.json").write_text(json.dumps(environment, indent=2) + "\n")
    with (args.output / "measurements.csv").open("w") as output:
        subprocess.run([str(binary), "--samples", str(args.samples)], stdout=output, check=True)
    memory = build / "numerix_comparison_memory"
    if memory.exists():
        with (args.output / "allocations.csv").open("w") as output:
            subprocess.run([str(memory), "--memory"], stdout=output, check=True)
    with (args.output / "measurements.csv").open() as source:
        rows = list(csv.DictReader(source))
    paired = {}
    for row in rows:
        key = tuple(row[name] for name in ("category", "case", "size", "method", "phase"))
        paired.setdefault(key, {})[row["library"]] = row
    with (args.output / "comparisons.csv").open("w", newline="") as output:
        writer = csv.writer(output, lineterminator="\n")
        writer.writerow(["category", "case", "size", "method", "phase", "comparison",
                         "numerix_median_ns", "gsl_median_ns", "speedup_gsl_over_numerix",
                         "eligible_for_direct_ratio", "reason"])
        for key, pair in paired.items():
            if "NUMERIX" not in pair or "GSL" not in pair:
                continue
            numerix, gsl = pair["NUMERIX"], pair["GSL"]
            equivalent = numerix["comparison"] in {"api_equivalent", "equivalent_representation",
                                                    "equivalent_family"}
            success = all(row["status"] == "0" and row["accuracy_pass"] in {"1", "-1"}
                          for row in pair.values())
            eligible = equivalent and success
            ratio = float(gsl["median_ns"]) / float(numerix["median_ns"])
            writer.writerow([*key, numerix["comparison"], numerix["median_ns"], gsl["median_ns"],
                             ratio if eligible and math.isfinite(ratio) else "",
                             int(eligible), "family/API comparison; see method caveats" if eligible else
                             "different work, reported failure, or failed accuracy criterion"])
    print(f"Recorded {len(rows)} observations in {args.output}")


if __name__ == "__main__":
    main()
