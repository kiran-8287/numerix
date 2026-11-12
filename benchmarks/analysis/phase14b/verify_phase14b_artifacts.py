#!/usr/bin/env python3
"""Validate the saved Phase 14B root study, reports, and preserved history."""
from __future__ import annotations

import csv
import hashlib
import json
import statistics
import subprocess
from collections import Counter, defaultdict
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[3]
ROOT_RUN = ROOT / "benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112"
ROOT_ANALYSIS = ROOT / "benchmarks/results/phase14b/analysis/root"
EDGE_RUN = ROOT / "benchmarks/results/phase14b/edge_cases/root"
EDGE_ANALYSIS = ROOT / "benchmarks/results/phase14b/analysis/root_edges"
FIGURES = ROOT / "benchmarks/results/phase14b/figures/root"
HTML_REPORT = ROOT / "documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.html"
MD_REPORT = ROOT / "documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md"


def rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


class LocalHTML(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.ids: set[str] = set()
        self.refs: list[str] = []
        self.method_cards = 0
        self.library_cards = 0

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attrs_d = dict(attrs)
        if attrs_d.get("id"):
            self.ids.add(str(attrs_d["id"]))
        for key in ("href", "src"):
            if attrs_d.get(key):
                self.refs.append(str(attrs_d[key]))
        if tag == "details" and "method-card" in attrs_d.get("class", ""):
            self.method_cards += 1
        if tag == "details" and "library-card" in attrs_d.get("class", ""):
            self.library_cards += 1


def main() -> int:
    checks: dict[str, object] = {}
    problems: list[str] = []

    audit = rows(ROOT / "benchmarks/analysis/phase14b/method_coverage.csv")
    caps = rows(ROOT / "benchmarks/results/phase14b/manifests/library_method_capabilities.csv")
    if len(audit) != 49:
        problems.append(f"expected 49 audited method rows, found {len(audit)}")
    if len(caps) != 392 or len({(r['library'], r['module'], r['method']) for r in caps}) != 392:
        problems.append("library capability matrix must contain 392 unique library/module/method rows")
    checks["method_inventory_rows"] = len(audit)
    checks["capability_rows"] = len(caps)

    acc = rows(ROOT_RUN / "accuracy.csv")
    timing = rows(ROOT_RUN / "raw_timing.csv")
    work = rows(ROOT_RUN / "work_metrics.csv")
    workload = rows(ROOT_RUN / "method_workloads.csv")
    acc_by_key = {(r["library"], r["requested_method"], r["problem_id"]): r for r in acc}
    if len(acc) != 1700 or len(acc_by_key) != len(acc):
        problems.append(f"root accuracy should contain 1,700 unique rows, found {len(acc)}")
    expected_per_route = Counter((r["library"], r["requested_method"]) for r in acc)
    if len(expected_per_route) != 17 or set(expected_per_route.values()) != {100}:
        problems.append("root study must have 17 observed routes with 100 cases each")
    timing_by_key: dict[tuple[str, str, str], list[dict[str, str]]] = defaultdict(list)
    timing_keys = set()
    for r in timing:
        key = (r["library"], r["requested_method"], r["problem_id"])
        unique = (*key, r["batch_index"])
        if unique in timing_keys:
            problems.append(f"duplicate root timing key {unique}")
        timing_keys.add(unique)
        timing_by_key[key].append(r)
        if float(r["wall_seconds"]) <= 0 or float(r["cpu_seconds"]) < 0:
            problems.append(f"invalid root time sample {unique}")
    if len(timing) != 52700 or set(map(len, timing_by_key.values())) != {31}:
        problems.append(f"root timing should contain 52,700 rows with 31 batches per case, found {len(timing)}")
    if len(workload) != 1700:
        problems.append(f"root method workload mapping should contain 1,700 rows, found {len(workload)}")
    if not work:
        problems.append("root work metrics are empty")
    checks.update(root_accuracy_rows=len(acc), root_timing_batches=len(timing),
                  root_work_metric_rows=len(work), root_routes=len(expected_per_route),
                  root_frozen_problems=len({r['problem_id'] for r in acc}))

    comparisons = rows(ROOT_ANALYSIS / "paired_comparisons.csv")
    direct_methods = {"bisection", "regula_falsi", "secant", "newton", "brent"}
    if {r["method"] for r in comparisons} != direct_methods:
        problems.append("direct comparison table must contain the five direct root methods")
    for c in comparisons:
        ratios = []
        method = c["method"]
        for (lib, m, problem_id), nr in acc_by_key.items():
            if lib != "NUMERIX" or m != method:
                continue
            gr = acc_by_key.get(("GSL", m, problem_id))
            if not gr or nr["applicable"] != "1" or gr["applicable"] != "1":
                continue
            if any(x["status"] != "0" or x["converged"] != "1" or x["accuracy_pass"] != "1"
                   for x in (nr, gr)):
                continue
            nt = timing_by_key.get(("NUMERIX", m, problem_id), [])
            gt = timing_by_key.get(("GSL", m, problem_id), [])
            if not nt or not gt:
                continue
            nm = statistics.median(float(x["wall_seconds"]) / float(x["batch_size"]) for x in nt)
            gm = statistics.median(float(x["wall_seconds"]) / float(x["batch_size"]) for x in gt)
            if nm > 0 and gm > 0:
                ratios.append(gm / nm)
        actual = statistics.median(ratios)
        reported = float(c["median_paired_speed_ratio"])
        if len(ratios) != int(c["eligible_paired_problems"]) or abs(actual - reported) > 1e-12 * max(1.0, abs(actual)):
            problems.append(f"paired ratio reconstruction mismatch for {method}")
    checks["paired_comparison_rows"] = len(comparisons)
    checks["paired_ratios_reconstructed"] = True

    edge_manifest = rows(ROOT / "benchmarks/results/phase14b/manifests/root_edge_cases.csv")
    edge_acc = rows(EDGE_RUN / "accuracy.csv")
    edge_timing = rows(EDGE_RUN / "raw_timing.csv")
    outcomes = rows(EDGE_ANALYSIS / "edge_outcomes.csv")
    if len(edge_manifest) != 20 or sum(r.get("edge_case") != "invalid_expression" for r in edge_manifest) != 19:
        problems.append(f"expected 19 numerical root edge cases plus one supporting expression probe, found {len(edge_manifest)} manifest rows")
    if len(edge_acc) != 322 or len(outcomes) != 322 or len(edge_timing) != 6762:
        problems.append(f"edge data row counts mismatch: accuracy={len(edge_acc)}, outcomes={len(outcomes)}, timing={len(edge_timing)}")
    planned = rows(EDGE_RUN / "method_workloads.csv")
    if len(planned) != 323:
        problems.append(f"expected 323 planned edge method routes, found {len(planned)}")
    expression = rows(EDGE_RUN / "expression_invalid_probe.csv")
    if not expression or not any(r.get("passed") == "1" for r in expression):
        problems.append("invalid-expression API probe did not pass")
    checks.update(root_edge_cases=sum(r.get("edge_case") != "invalid_expression" for r in edge_manifest),
                  supporting_api_edge_probes=sum(r.get("edge_case") == "invalid_expression" for r in edge_manifest), edge_accuracy_rows=len(edge_acc),
                  edge_timing_batches=len(edge_timing), edge_planned_routes=len(planned),
                  edge_measured_routes=len(outcomes), expression_probe_passed=True)

    expected_figures = [f"root_{name}.{ext}" for name in
        ("direct_runtime_paired", "direct_speed_ratio_forest", "accuracy_pass_rates",
         "discovery_counts", "edge_accuracy_counts") for ext in ("png", "svg")]
    expected_figures += [f"root_method_{m}_runtime.{ext}" for m in
        ("bisection", "regula_falsi", "secant", "newton", "safeguarded_newton", "brent",
         "auto_selection", "portfolio", "multiple_root_discovery") for ext in ("png", "svg")]
    missing_figures = [f for f in expected_figures if not (FIGURES / f).is_file()]
    if missing_figures:
        problems.append(f"missing graph assets: {missing_figures}")
    checks["graph_assets"] = len(expected_figures) - len(missing_figures)

    html = LocalHTML()
    html.feed(HTML_REPORT.read_text(encoding="utf-8"))
    missing_paths = []
    missing_anchors = []
    for ref in html.refs:
        u = urlparse(ref)
        if u.scheme or ref.startswith("//"):
            continue
        if u.fragment and not u.path and unquote(u.fragment) not in html.ids:
            missing_anchors.append(ref)
        elif u.path and not (HTML_REPORT.parent / unquote(u.path)).resolve().exists():
            missing_paths.append(ref)
    report_text = HTML_REPORT.read_text(encoding="utf-8")
    controls = ("f-library", "f-module", "f-method", "f-comparison", "f-difficulty", "f-edge",
                "f-accuracy", "f-success", "f-search", "reset-filters", "expand-all", "collapse-all")
    if html.library_cards != 8 or html.method_cards != 392:
        problems.append(f"HTML accordion count mismatch: libraries={html.library_cards}, methods={html.method_cards}")
    if missing_paths or missing_anchors:
        problems.append(f"HTML broken refs: paths={missing_paths}; anchors={missing_anchors}")
    if any(c not in report_text for c in controls):
        problems.append("one or more required dashboard controls are missing")
    if not MD_REPORT.is_file():
        problems.append("Markdown companion is missing")
    checks.update(html_library_accordions=html.library_cards, html_method_cards=html.method_cards,
                  html_local_references=len(html.refs), html_missing_paths=len(missing_paths),
                  html_missing_anchors=len(missing_anchors), html_controls_present=all(c in report_text for c in controls))

    inventory = ROOT / "benchmarks/results/phase14/logs/phase13_preservation.sha256"
    inventory_entries = 0
    checksum_failures = []
    for line in inventory.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        digest, name = line.split(maxsplit=1)
        target = ROOT / name.strip().lstrip("*")
        inventory_entries += 1
        if not target.is_file() or sha256(target) != digest:
            checksum_failures.append(name)
    if inventory_entries != 125 or checksum_failures:
        problems.append(f"Phase 13 preservation mismatch: {inventory_entries} entries; failures={checksum_failures}")
    phase14_changes = subprocess.run(["git", "status", "--short", "--",
       "benchmarks/results/phase14", "benchmarks/analysis/phase14", "documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.html"],
       cwd=ROOT, text=True, capture_output=True, check=True).stdout.strip()
    if phase14_changes:
        problems.append(f"historical Phase 14 artifacts have working-tree changes: {phase14_changes}")
    checks.update(phase13_checksum_entries=inventory_entries, phase13_checksum_failures=len(checksum_failures),
                  historical_phase14_worktree_changes=bool(phase14_changes),
                  html_sha256=sha256(HTML_REPORT), markdown_sha256=sha256(MD_REPORT))

    result = {"status": "PASS" if not problems else "FAIL", "checks": checks, "problems": problems}
    target = ROOT / "benchmarks/results/phase14b/verification/phase14b_artifact_verification.json"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if not problems else 1


if __name__ == "__main__":
    raise SystemExit(main())
