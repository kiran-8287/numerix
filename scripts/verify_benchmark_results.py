#!/usr/bin/env python3
"""Verify historical Phase 9/10 summaries or Phase 13B raw measurement records."""
import argparse
import csv
import math
from pathlib import Path
import statistics


def load(path):
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        rows = list(reader)
        fields = reader.fieldnames or []
    assert rows, f"No observations in {path}"
    assert all(None not in row for row in rows), f"Malformed CSV in {path}"
    return rows, fields


def legacy_key(row):
    return tuple(row[name] for name in
                 ("category", "case", "size", "method", "phase", "library"))


def verify_legacy(directory):
    rows, _ = load(directory / "measurements.csv")
    indexed = {legacy_key(row): row for row in rows}
    assert len(indexed) == len(rows), "Duplicate historical benchmark observation"
    for row in rows:
        assert int(row["trials"]) >= 3
        assert int(row["successes"]) <= int(row["trials"])
        timings = [float(row[name]) for name in
                   ("min_ns", "median_ns", "mean_ns", "stddev_ns", "p95_ns")]
        assert all(math.isfinite(value) and value >= 0 for value in timings), legacy_key(row)
        assert timings[0] <= timings[1] <= timings[4], legacy_key(row)
        if row["status"] != "0":
            expected = row["case"] in {"no_bracket", "hilbert", "endpoint_sqrt"}
            assert expected, ("Unexpected historical failure", legacy_key(row), row["status"])
        if row["phase"] not in {"construct", "factor"} and row["status"] == "0":
            if row["category"] not in {"linear", "ode"}:
                actual = abs(float(row["value"]) - float(row["reference"]))
                assert math.isclose(actual, float(row["absolute_error"]),
                                    rel_tol=1e-12, abs_tol=1e-30), legacy_key(row)
        if row["category"] in {"interpolation", "differentiation",
                               "optimization", "capability"}:
            assert row["status"] == "0", legacy_key(row)
            assert row["accuracy_pass"] in {"1", "-1"}, legacy_key(row)
    memory_path = directory / "allocations.csv"
    if memory_path.exists():
        memory, _ = load(memory_path)
        assert {legacy_key(row) for row in memory} == set(indexed)
        for row in memory:
            assert row["memory_measured"] == "1"
            assert row["live_bytes_after_destroy"] == "0", ("Leak", legacy_key(row))
            assert int(row["peak_live_bytes"]) <= int(row["allocated_bytes"])
            assert row["status"] == indexed[legacy_key(row)]["status"], legacy_key(row)
    print(f"Verified {len(rows)} historical observations; legacy schema retained.")


def close(actual, expected, label):
    assert math.isfinite(actual) and math.isfinite(expected), label
    assert math.isclose(actual, expected, rel_tol=2e-10, abs_tol=1e-12), (
        label, actual, expected)


def precision(row):
    storage = int(row["storage_bits"])
    significand = int(row["significand_bits"])
    minimum = int(row["min_exponent"])
    maximum = int(row["max_exponent"])
    assert storage > 0 and 0 < significand <= storage, row
    assert minimum < 0 < maximum, row
    return storage, significand, minimum, maximum


def expected_order(seed, pair_index):
    block = pair_index // 2
    orientation = (seed & 1) ^ (block & 1)
    return "NUMERIX_FIRST" if (orientation ^ (pair_index & 1)) == 0 else "GSL_FIRST"


def verify_phase13b(directory):
    summary, _ = load(directory / "measurements.csv")
    raw, _ = load(directory / "raw_timing.csv")
    accuracy, _ = load(directory / "accuracy.csv")
    workloads, _ = load(directory / "workloads.csv")

    for rows, name in ((summary, "summary"), (raw, "raw timing"),
                       (accuracy, "accuracy"), (workloads, "workload manifest")):
        assert all(row.get("schema_version") == "2" for row in rows), (
            f"Unsupported or mixed {name} schema")

    summary_by_key = {}
    for row in summary:
        key = (row["run_id"], row["configuration_id"], row["library"])
        assert key not in summary_by_key, ("Duplicate summary", key)
        summary_by_key[key] = row

    workload_by_key = {}
    for row in workloads:
        key = (row["run_id"], row["configuration_id"], row["library"])
        assert key not in workload_by_key, ("Duplicate workload identity", key)
        workload_by_key[key] = row
        assert row["configuration_id"], row
        assert row["reference_method"], row
        assert row["comparison_mode"] in {
            "same_algorithm", "best_available", "capability_only",
            "excluded_unranked_case"
        }, row
        precision(row)

    accuracy_by_id = {}
    accuracy_by_key = {}
    for row in accuracy:
        key = (row["run_id"], row["configuration_id"], row["library"],
               int(row["probe_index"]))
        assert key not in accuracy_by_key, ("Duplicate accuracy probe", key)
        assert row["accuracy_record_id"] not in accuracy_by_id, (
            "Duplicate accuracy record ID", row["accuracy_record_id"])
        assert row["probe_kind"] == "untimed_result_probe", row
        assert row["accuracy_pass"] in {"-1", "0", "1"}, row
        accuracy_by_key[key] = row
        accuracy_by_id[row["accuracy_record_id"]] = row

    raw_by_key = {}
    batches = {}
    orders = {}
    precision_by_key = {}
    seed_by_pair = {}
    for row in raw:
        key = (row["run_id"], row["configuration_id"], row["library"])
        assert key in summary_by_key, ("Raw sample has no summary", key)
        assert key in workload_by_key, ("Raw sample has no manifest row", key)
        sample_index = int(row["sample_index"])
        sample_key = key + (sample_index,)
        assert sample_key not in raw_by_key, ("Duplicate raw sample", sample_key)
        assert row["timing_unit"] == "nanoseconds_per_operation", row
        elapsed = float(row["elapsed_ns_per_operation"])
        batch_elapsed = float(row["batch_elapsed_ns"])
        batch = int(row["batch_size"])
        assert math.isfinite(elapsed) and elapsed > 0.0, row
        assert math.isfinite(batch_elapsed) and batch_elapsed > 0.0, row
        assert batch > 0, row
        close(elapsed, batch_elapsed / batch, ("batch timing", sample_key))
        assert row["execution_order"] not in {"", "nan", "None"}, row
        assert row["accuracy_pass"] in {"-1", "0", "1"}, row
        assert row["accuracy_record_id"] in accuracy_by_id, (
            "Missing associated accuracy record", sample_key)
        accuracy_row = accuracy_by_id[row["accuracy_record_id"]]
        assert (accuracy_row["run_id"], accuracy_row["configuration_id"],
                accuracy_row["library"], int(accuracy_row["probe_index"])) == (
                    row["run_id"], row["configuration_id"], row["library"],
                    sample_index), ("Accuracy linkage mismatch", sample_key)
        assert accuracy_row["status"] == row["status"], (
            "Status differs from accuracy probe", sample_key)
        assert accuracy_row["accuracy_pass"] == row["accuracy_pass"], (
            "Accuracy result differs from accuracy probe", sample_key)
        manifest = workload_by_key[key]
        for field in ("pair_id", "category", "case", "method", "phase",
                      "randomization_seed"):
            assert row[field] == manifest[field], (
                "Inconsistent workload identity", field, sample_key)
        assert int(row["pair_index"]) == int(manifest["pair_index"]), sample_key
        precision_key = precision(row)
        assert precision_key == precision(manifest), (
            "Precision metadata differs from manifest", sample_key)
        assert precision_key == precision(summary_by_key[key]), (
            "Precision metadata differs from summary", sample_key)
        raw_by_key[sample_key] = row
        batches.setdefault(key, set()).add(batch)
        orders.setdefault(key, set()).add(row["execution_order"])
        precision_by_key[key] = precision_key
        if row["pair_id"]:
            pair_key = (row["run_id"], row["pair_id"])
            seed_pair = (int(row["randomization_seed"]), int(row["pair_index"]))
            assert pair_key not in seed_by_pair or seed_by_pair[pair_key] == seed_pair, (
                "Paired rows have inconsistent seed or index", pair_key)
            seed_by_pair[pair_key] = seed_pair

    assert set(summary_by_key) == set(workload_by_key), (
        "Summary and workload manifest identifiers differ")
    assert set(summary_by_key) == set(batches), (
        "A summary has no raw samples or raw samples lack a summary")
    assert set(accuracy_by_key) == set(raw_by_key), (
        "Missing or unassociated accuracy probes")

    for key, summary_row in summary_by_key.items():
        manifest = workload_by_key[key]
        for field in ("pair_id", "category", "case", "method", "phase",
                      "comparison_mode", "randomization_seed"):
            assert summary_row[field] == manifest[field], (
                "Summary differs from workload manifest", field, key)
        samples = [float(raw_by_key[key + (index,)]["elapsed_ns_per_operation"])
                   for index in sorted(sample_key[3] for sample_key in raw_by_key
                                       if sample_key[:3] == key)]
        indices = sorted(sample_key[3] for sample_key in raw_by_key
                         if sample_key[:3] == key)
        expected_count = int(summary_row["raw_sample_count"])
        assert indices == list(range(expected_count)), (
            "Missing timing sample", key, indices, expected_count)
        assert len(batches[key]) == 1, ("Inconsistent batch size", key, batches[key])
        batch = next(iter(batches[key]))
        assert int(summary_row["batch_size"]) == batch, (
            "Summary batch size differs from raw records", key)
        assert len(orders[key]) == 1, ("Inconsistent order metadata", key, orders[key])
        order = next(iter(orders[key]))
        if order in {"NUMERIX_FIRST", "GSL_FIRST"}:
            assert summary_row["pair_id"], ("Paired order without pair ID", key)
        else:
            assert order in {"UNPAIRED", "GSL_UNAVAILABLE",
                             "UNPAIRED_LIBRARY_FILTER"}, (
                "Malformed execution order", key, order)
        accuracy_rows = [accuracy_by_key[key + (index,)]
                         for index in range(expected_count)]
        assert int(summary_row["accuracy_probe_count"]) == expected_count, key
        assert int(summary_row["successful_probes"]) == sum(
            row["status"] == "0" for row in accuracy_rows), key
        assert int(summary_row["accurate_probes"]) == sum(
            row["accuracy_pass"] == "1" for row in accuracy_rows), key
        close(float(summary_row["min_ns"]), min(samples), ("minimum", key))
        close(float(summary_row["median_ns"]), statistics.median(samples),
              ("median", key))
        close(float(summary_row["mean_ns"]), statistics.mean(samples), ("mean", key))
        close(float(summary_row["sample_stddev_ns"]), statistics.stdev(samples),
              ("sample standard deviation", key))
        percentile_index = math.ceil(0.95 * len(samples)) - 1
        close(float(summary_row["p95_ns"]), sorted(samples)[percentile_index],
              ("95th percentile, nearest rank", key))
        close(float(summary_row["max_ns"]), max(samples), ("maximum", key))
        assert int(summary_row["status"]) == int(accuracy_rows[-1]["status"]), key
        assert int(summary_row["accuracy_pass"]) == int(
            accuracy_rows[-1]["accuracy_pass"]), key

    paired = {}
    for key, summary_row in summary_by_key.items():
        pair_id = summary_row["pair_id"]
        if pair_id:
            paired.setdefault((summary_row["run_id"], pair_id), {})[
                summary_row["library"]] = summary_row
    for pair_key, libraries in paired.items():
        if set(libraries) == {"NUMERIX", "GSL"}:
            numerix = libraries["NUMERIX"]
            gsl = libraries["GSL"]
            expected = expected_order(int(numerix["randomization_seed"]),
                                      int(numerix["pair_index"]))
            assert numerix["execution_order"] == expected, (
                "Order does not match recorded seed", pair_key)
            assert gsl["execution_order"] == expected, (
                "Paired order differs between libraries", pair_key)

    print(f"Verified schema v2: {len(summary)} workload summaries, "
          f"{len(raw)} raw timing batches, {len(accuracy)} separate accuracy probes.")


def verify(directory):
    summary, fields = load(directory / "measurements.csv")
    if "schema_version" in fields and all(row["schema_version"] == "2"
                                          for row in summary):
        verify_phase13b(directory)
    else:
        verify_legacy(directory)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    verify(parser.parse_args().directory)
