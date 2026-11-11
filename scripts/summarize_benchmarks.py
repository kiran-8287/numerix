#!/usr/bin/env python3
"""Generate per-case summary tables from the recorded observations."""
import argparse
import csv
import math
from pathlib import Path


def number(value, microseconds=False):
    value = float(value)
    if not math.isfinite(value):
        return "—"
    return f"{value / 1000:.3f}" if microseconds else f"{value:.3g}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    with (args.directory / "measurements.csv").open() as source:
        rows = list(csv.DictReader(source))
    with (args.directory / "comparisons.csv").open() as source:
        comparisons = list(csv.DictReader(source))
    def key(row):
        return tuple(row[name] for name in ("category", "case", "size", "method", "phase"))
    ratios = {key(row): row["speedup_gsl_over_numerix"] for row in comparisons}
    lines = ["# Machine-generated Phase 9 summary", "",
             "Medians are microseconds per API operation; full min/mean/stddev/p95, statuses,",
             "tolerances, reference values and reliability probes are in measurements.csv.",
             "Blank speedups exclude different algorithms/work, failures or failed accuracy checks.",
             "A ratio above1 means NUMERIX was faster in that particular API workload.", ""]
    for category in dict.fromkeys(row["category"] for row in rows):
        lines.extend([f"## {category}", "",
                      "| Case | n | Method / phase | Library | Median µs | abs error | rel error | f / df | iterations | status / accuracy | speedup |",
                      "| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | --- | ---: |"])
        for row in rows:
            if row["category"] != category:
                continue
            ratio = ratios.get(key(row), "") if row["library"] == "NUMERIX" else ""
            lines.append(f"| {row['case']} | {row['size']} | {row['method']} / {row['phase']} | "
                         f"{row['library']} | {number(row['median_ns'], True)} | "
                         f"{number(row['absolute_error'])} | {number(row['relative_error'])} | "
                         f"{row['function_evaluations']} / {row['derivative_evaluations']} | "
                         f"{row['iterations']} | {row['status']} / {row['accuracy_pass']} | "
                         f"{number(ratio) if ratio else '—'} |")
        lines.append("")
    memory_file = args.directory / "allocations.csv"
    if memory_file.exists():
        with memory_file.open() as source:
            memory = list(csv.DictReader(source))
        lines.extend(["## Allocation scope: setup + one operation + destruction", "",
                      "Requested heap bytes, including nested linked-object allocations; inputs, stack,",
                      "allocator metadata and process RSS are excluded. All figures are measured,",
                      "not formulas. Setup bytes were live just before the operation.", "",
                      "| Case | n | Method / phase | Library | allocations | cumulative B | peak live B | setup B | after destroy B |",
                      "| --- | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: |"])
        for row in memory:
            lines.append(f"| {row['case']} | {row['size']} | {row['method']} / {row['phase']} | "
                         f"{row['library']} | {row['allocations']} | {row['allocated_bytes']} | "
                         f"{row['peak_live_bytes']} | {row['setup_live_bytes']} | "
                         f"{row['live_bytes_after_destroy']} |")
    (args.directory / "summary.md").write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
