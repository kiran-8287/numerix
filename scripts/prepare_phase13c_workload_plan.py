#!/usr/bin/env python3
"""Create the Phase 13C mathematical-problem manifest from a verified pilot."""
import argparse
import csv
import hashlib
import json
from pathlib import Path


PROBLEM_FIELDS = (
    "category", "case", "variant", "difficulty", "input_size", "lower",
    "upper", "initial", "step", "scale", "absolute_tolerance",
    "relative_tolerance", "expected_value", "reference_method",
)


def stable_problem_id(row):
    payload = json.dumps([row.get(field, "") for field in PROBLEM_FIELDS],
                         ensure_ascii=True, separators=(",", ":"))
    return "problem-" + hashlib.sha256(payload.encode("utf-8")).hexdigest()[:16]


def inclusion_reason(row):
    difficulty = row.get("difficulty", "")
    size = row.get("input_size", "0")
    scale = row.get("scale", "1")
    return (f"Named {row.get('case')} problem; difficulty level {difficulty}; "
            f"input size {size}; scale {scale}. Reference is recorded as "
            f"{row.get('reference_method')}.")


def create_plan(source, destination):
    with Path(source).open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    grouped = {}
    for row in rows:
        grouped.setdefault(stable_problem_id(row), []).append(row)

    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    fields = ["problem_id", *PROBLEM_FIELDS, "planned_libraries",
              "planned_methods_phases", "comparison_labels",
              "planned_workload_records", "inclusion_reason"]
    with destination.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        for problem_id in sorted(grouped):
            members = grouped[problem_id]
            first = members[0]
            output = {field: first.get(field, "") for field in PROBLEM_FIELDS}
            output.update({
                "problem_id": problem_id,
                "planned_libraries": ";".join(sorted({r["library"] for r in members})),
                "planned_methods_phases": ";".join(sorted({
                    r["method"] + ":" + r["phase"] for r in members})),
                "comparison_labels": ";".join(sorted({r["comparison_label"] for r in members})),
                "planned_workload_records": len({
                    (r["configuration_id"], r["library"]) for r in members}),
                "inclusion_reason": inclusion_reason(first),
            })
            writer.writerow(output)
    return len(grouped)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pilot_workloads", type=Path,
                        help="workloads.csv from the verified Phase 13C pilot")
    parser.add_argument("output", type=Path, help="planned problem manifest CSV")
    args = parser.parse_args()
    count = create_plan(args.pilot_workloads, args.output)
    print(f"Wrote {count} distinct problem configurations to {args.output}")


if __name__ == "__main__":
    main()
