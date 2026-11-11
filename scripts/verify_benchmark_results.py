#!/usr/bin/env python3
"""Validate CSV integrity and analytical checks without hiding numerical failures."""
import argparse
import csv
import math
from pathlib import Path


def load(path):
    with path.open() as source:
        rows = list(csv.DictReader(source))
    assert rows, f"No observations in {path}"
    assert all(None not in row for row in rows), f"Malformed CSV in {path}"
    return rows


def key(row):
    return tuple(row[name] for name in ("category", "case", "size", "method", "phase", "library"))


def verify(directory):
    rows = load(directory / "measurements.csv")
    indexed = {key(row): row for row in rows}
    assert len(indexed) == len(rows), "Duplicate benchmark observation"
    for row in rows:
        assert int(row["trials"]) >= 3
        assert int(row["successes"]) <= int(row["trials"])
        timings = [float(row[name]) for name in ("min_ns", "median_ns", "mean_ns", "stddev_ns", "p95_ns")]
        assert all(math.isfinite(value) and value >= 0 for value in timings), key(row)
        assert timings[0] <= timings[1] <= timings[4], key(row)
        if row["status"] != "0":
            expected = row["case"] in {"no_bracket", "hilbert", "endpoint_sqrt"}
            assert expected, ("Unexpected failure", key(row), row["status"])
        if row["phase"] not in {"construct", "factor"} and row["status"] == "0":
            if row["category"] not in {"linear", "ode"}:
                actual = abs(float(row["value"]) - float(row["reference"]))
                assert math.isclose(actual, float(row["absolute_error"]), rel_tol=1e-12, abs_tol=1e-30), key(row)
        if row["category"] in {"interpolation", "differentiation", "optimization", "capability"}:
            assert row["status"] == "0", key(row)
            assert row["accuracy_pass"] in {"1", "-1"}, key(row)
    memory_path = directory / "allocations.csv"
    if memory_path.exists():
        memory = load(memory_path)
        assert {key(row) for row in memory} == set(indexed)
        for row in memory:
            assert row["memory_measured"] == "1"
            assert row["live_bytes_after_destroy"] == "0", ("Leak", key(row))
            assert int(row["peak_live_bytes"]) <= int(row["allocated_bytes"])
            assert row["status"] == indexed[key(row)]["status"], key(row)
    print(f"Verified {len(rows)} timed observations, numerical failures retained and classified.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    verify(parser.parse_args().directory)
