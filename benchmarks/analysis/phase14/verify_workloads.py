#!/usr/bin/env python3
"""Validate Phase 14 workload counts, uniqueness, and reference/input linkage."""

from __future__ import annotations

import csv
import hashlib
import json
import math
import sys
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
MANIFEST = ROOT / "benchmarks/problems/phase14/workloads.csv"
INPUTS = ROOT / "benchmarks/problems/phase14/problem_inputs.jsonl"
OUTPUT = ROOT / "benchmarks/analysis/phase14/workload_validation.json"
MODULES = ("root", "linear_algebra", "differentiation", "interpolation", "integration", "optimization", "ode")


def finite_tree(value):
    if isinstance(value, (int, float)):
        return math.isfinite(value)
    if isinstance(value, list):
        return all(finite_tree(v) for v in value)
    if isinstance(value, dict):
        return all(finite_tree(v) for v in value.values())
    return True


def main():
    csv.field_size_limit(sys.maxsize)
    with MANIFEST.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    inputs = {}
    with INPUTS.open(encoding="utf-8") as stream:
        for line in stream:
            item = json.loads(line)
            if item["problem_id"] in inputs:
                raise ValueError(f"duplicate input ID: {item['problem_id']}")
            inputs[item["problem_id"]] = item

    if len(rows) != 700 or len({r["problem_id"] for r in rows}) != len(rows):
        raise ValueError("manifest must contain exactly 700 unique problem IDs")
    if set(inputs) != {r["problem_id"] for r in rows}:
        raise ValueError("manifest and input sidecar IDs differ")

    counts = Counter(r["module"] for r in rows)
    family_counts = {m: len({r["family"] for r in rows if r["module"] == m}) for m in MODULES}
    variants = defaultdict(Counter)
    errors = []
    for r in rows:
        m = r["module"]
        item = inputs[r["problem_id"]]
        payload = item["input_parameters"]
        actual_hash = hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
        if actual_hash != r["input_sha256"] or item["input_sha256"] != r["input_sha256"]:
            errors.append(f"input digest mismatch: {r['problem_id']}")
        if not finite_tree(payload):
            errors.append(f"non-finite input: {r['problem_id']}")
        try:
            reference = json.loads(r["reference_value"])
            if not finite_tree(reference):
                errors.append(f"non-finite reference: {r['problem_id']}")
        except json.JSONDecodeError:
            # High-precision scalar decimal strings are intentionally stored as JSON strings.
            reference = r["reference_value"].strip('"')
            if not reference or not math.isfinite(float(reference)):
                errors.append(f"invalid reference: {r['problem_id']}")
        variants[m][r["family"]] += 1

        if m == "root":
            if not payload["lower"] <= payload["root"] <= payload["upper"]:
                errors.append(f"root outside interval: {r['problem_id']}")
            if int(r["model"]) == 6 and payload["lower"] + payload["shift"] <= 0:
                errors.append(f"logarithm undefined on root interval: {r['problem_id']}")
        elif m == "linear_algebra":
            a, b, x = payload["matrix"], payload["rhs"], payload["x_reference"]
            n = payload["n"]
            if n != len(a) or len(b) != n or len(x) != n or any(len(line) != n for line in a):
                errors.append(f"linear-system dimensions mismatch: {r['problem_id']}")
            elif r["expected_status"] != "singular_expected":
                residual = max(abs(sum(a[i][j] * x[j] for j in range(n)) - b[i]) for i in range(n))
                scale = max(1.0, max(abs(v) for v in b))
                if residual > 2e-12 * scale:
                    errors.append(f"linear reference residual too large: {r['problem_id']}: {residual}")
        elif m == "interpolation":
            if len(payload["nodes"]) != payload["n"] or len(payload["values"]) != payload["n"]:
                errors.append(f"interpolation node count mismatch: {r['problem_id']}")
            if any(a >= b for a, b in zip(payload["nodes"], payload["nodes"][1:])):
                errors.append(f"interpolation nodes not strictly increasing: {r['problem_id']}")
            if len(payload["validation_x"]) != len(payload["validation_y"]):
                errors.append(f"interpolation validation vectors differ: {r['problem_id']}")
        elif m == "optimization":
            x = payload["reference_minimizer"]
            if not payload["lower"] <= x <= payload["upper"]:
                errors.append(f"optimizer reference outside bounds: {r['problem_id']}")
        elif m == "ode":
            if len(payload["initial_state"]) != payload["dimension"] or len(json.loads(r["reference_value"])) != payload["dimension"]:
                errors.append(f"ODE state dimension mismatch: {r['problem_id']}")
            if abs(payload["fixed_step"] * payload["fixed_steps"] - (payload["tf"] - payload["t0"])) > 1e-11:
                errors.append(f"ODE fixed mesh does not end at tf: {r['problem_id']}")

    if any(counts[m] != 100 for m in MODULES):
        errors.append(f"module counts do not equal 100: {dict(counts)}")
    if any(family_counts[m] != 20 for m in MODULES):
        errors.append(f"module family counts do not equal 20: {family_counts}")
    if any(set(counter.values()) != {5} for counter in variants.values()):
        errors.append("each problem family must have five variants")
    if len({r["input_sha256"] for r in rows}) != 700:
        errors.append("input payloads are not all distinct")
    if errors:
        raise SystemExit("\n".join(errors))

    result = {
        "schema_version": "phase14-workload-validation-v1",
        "manifest": str(MANIFEST.relative_to(ROOT)),
        "input_sidecar": str(INPUTS.relative_to(ROOT)),
        "problem_count": len(rows),
        "module_counts": dict(counts),
        "family_counts": family_counts,
        "families_per_module": 20,
        "variants_per_family": 5,
        "distinct_input_payloads": len({r["input_sha256"] for r in rows}),
        "result": "PASS",
        "checks": ["unique IDs", "module and family coverage", "five distinct inputs per family", "input SHA-256 linkage", "finite references", "matrix/reference residuals", "interpolation data dimensions", "root bounds and logarithm domain", "optimization bounds", "ODE state dimensions and mesh endpoint"],
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
