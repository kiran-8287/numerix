#!/usr/bin/env python3
"""Small deterministic tests for Phase 13C derived analyses."""
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "analyze_phase13c", ROOT / "scripts/analyze_phase13c.py")
ANALYSIS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ANALYSIS)


class Phase13CAnalysisTests(unittest.TestCase):
    def test_problem_identity_ignores_algorithm_fields(self):
        left = {field: "x" for field in ANALYSIS.PROBLEM_FIELDS}
        right = dict(left)
        left["method"] = "bisection"
        right["method"] = "newton"
        self.assertEqual(ANALYSIS.problem_id(left), ANALYSIS.problem_id(right))
        right["input_size"] = "16"
        self.assertNotEqual(ANALYSIS.problem_id(left), ANALYSIS.problem_id(right))

    def test_comparison_classification(self):
        self.assertEqual(ANALYSIS.comparison_class("api_equivalent", "same_algorithm"),
                         "DIRECT")
        self.assertEqual(ANALYSIS.comparison_class("different_algorithm", "best_available"),
                         "BEST_AVAILABLE")
        self.assertEqual(ANALYSIS.comparison_class("different_safeguards", "best_available"),
                         "APPROXIMATE")
        self.assertEqual(ANALYSIS.comparison_class("ill_conditioned_unranked",
                                                   "excluded_unranked_case"), "INCOMPARABLE")
        self.assertEqual(ANALYSIS.comparison_class("numerix_only", "capability_only"),
                         "NOT_SUPPORTED")

    def test_geometric_mean_and_bootstrap_are_reproducible(self):
        self.assertAlmostEqual(ANALYSIS.geometric_mean([1.0, 4.0]), 2.0)
        first = ANALYSIS.bootstrap_gmean_ci([1.0, 2.0, 4.0], seed=17, repeats=500)
        second = ANALYSIS.bootstrap_gmean_ci([1.0, 2.0, 4.0], seed=17, repeats=500)
        self.assertEqual(first, second)
        self.assertLess(first[0], first[1])

    def test_paired_ratio_uses_accuracy_gate(self):
        config = "cfg-one"
        workload = {
            "configuration_id": config,
            "category": "root", "case": "polynomial", "variant": "0",
            "difficulty": "1", "input_size": "0", "lower": "0", "upper": "2",
            "initial": "1", "step": "0", "scale": "1",
            "absolute_tolerance": "1e-10", "relative_tolerance": "1e-10",
            "expected_value": "1.414", "reference_method": "analytic",
        }
        base = {
            "pair_id": config, "configuration_id": config,
            "category": "root", "case": "polynomial", "input_size": "0",
            "method": "bisection", "phase": "execute",
            "comparison_label": "api_equivalent", "comparison_mode": "same_algorithm",
            "status": "0", "accuracy_pass": "1", "median_ns": "10",
        }
        n = dict(base, library="NUMERIX")
        g = dict(base, library="GSL", median_ns="20")
        raw = []
        for library, values in (("NUMERIX", (9, 10, 11)), ("GSL", (18, 20, 22))):
            raw.extend({"configuration_id": config, "library": library,
                        "elapsed_ns_per_operation": str(value)} for value in values)
        pairs, _, _ = ANALYSIS.make_pairs([n, g], [dict(workload, library="NUMERIX"),
                                                     dict(workload, library="GSL")], raw)
        self.assertEqual(len(pairs), 1)
        self.assertEqual(pairs[0]["comparison_class"], "DIRECT")
        self.assertEqual(pairs[0]["eligible_for_claim"], 1)
        self.assertAlmostEqual(pairs[0]["speedup_gsl_over_numerix"], 2.0)

        g["accuracy_pass"] = "0"
        pairs, _, _ = ANALYSIS.make_pairs([n, g], [dict(workload, library="NUMERIX"),
                                                     dict(workload, library="GSL")], raw)
        self.assertEqual(pairs[0]["eligible_for_claim"], 0)
        self.assertTrue(pairs[0]["speedup_gsl_over_numerix"] != pairs[0]["speedup_gsl_over_numerix"])


if __name__ == "__main__":
    unittest.main()
