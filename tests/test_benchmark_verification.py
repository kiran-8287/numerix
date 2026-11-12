#!/usr/bin/env python3
"""Exercise raw timing preservation and independent schema-v2 verification."""
import csv
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def read_rows(path):
    with path.open(newline="") as source:
        reader = csv.DictReader(source)
        return reader.fieldnames, list(reader)


def write_rows(path, fields, rows):
    with path.open("w", newline="") as destination:
        writer = csv.DictWriter(destination, fieldnames=fields,
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def copy_run(source, root, name):
    destination = root / name
    shutil.copytree(source, destination)
    return destination


def expect_valid(verifier, directory):
    subprocess.run([sys.executable, str(verifier), str(directory)],
                   check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                   text=True)


def expect_invalid(verifier, directory):
    result = subprocess.run([sys.executable, str(verifier), str(directory)],
                            check=False, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True)
    assert result.returncode != 0, f"verifier accepted corrupted data in {directory}"


def main():
    binary = Path(sys.argv[1]).resolve()
    verifier = Path(sys.argv[2]).resolve()

    with tempfile.TemporaryDirectory(prefix="numerix-phase13b-verify-") as temporary:
        root = Path(temporary)
        run = root / "valid"
        run.mkdir()
        with (run / "measurements.csv").open("x") as summary:
            subprocess.run([
                str(binary), "--samples", "3", "--seed", "42",
                "--case", "polynomial", "--output-dir", str(run),
                "--run-id", "verification-test"
            ], stdout=summary, check=True)
        expect_valid(verifier, run)

        summary_fields, summary_rows = read_rows(run / "measurements.csv")
        raw_fields, raw_rows = read_rows(run / "raw_timing.csv")
        accuracy_fields, accuracy_rows = read_rows(run / "accuracy.csv")
        assert len(raw_rows) == sum(int(row["raw_sample_count"])
                                    for row in summary_rows)
        assert len(accuracy_rows) == len(raw_rows)

        directory = copy_run(run, root, "missing-sample")
        fields, rows = read_rows(directory / "raw_timing.csv")
        write_rows(directory / "raw_timing.csv", fields, rows[:-1])
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "duplicate-sample")
        fields, rows = read_rows(directory / "raw_timing.csv")
        rows.append(dict(rows[0]))
        write_rows(directory / "raw_timing.csv", fields, rows)
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "invalid-timing")
        fields, rows = read_rows(directory / "raw_timing.csv")
        rows[0]["elapsed_ns_per_operation"] = "0"
        write_rows(directory / "raw_timing.csv", fields, rows)
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "wrong-summary")
        fields, rows = read_rows(directory / "measurements.csv")
        rows[0]["max_ns"] = "999999"
        write_rows(directory / "measurements.csv", fields, rows)
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "bad-precision")
        fields, rows = read_rows(directory / "raw_timing.csv")
        rows[0]["storage_bits"] = "not-a-number"
        write_rows(directory / "raw_timing.csv", fields, rows)
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "missing-order")
        fields, rows = read_rows(directory / "raw_timing.csv")
        rows[0]["execution_order"] = ""
        write_rows(directory / "raw_timing.csv", fields, rows)
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "inconsistent-identity")
        fields, rows = read_rows(directory / "raw_timing.csv")
        rows[0]["case"] = "different-workload"
        write_rows(directory / "raw_timing.csv", fields, rows)
        expect_invalid(verifier, directory)

        directory = copy_run(run, root, "missing-accuracy")
        fields, rows = read_rows(directory / "accuracy.csv")
        write_rows(directory / "accuracy.csv", fields, rows[:-1])
        expect_invalid(verifier, directory)

    print("Raw timing and summary verifier corruption checks passed.")


if __name__ == "__main__":
    main()
