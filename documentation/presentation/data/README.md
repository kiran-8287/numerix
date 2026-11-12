# Presentation benchmark data

This directory contains generated presentation inputs, not a new benchmark run.
Regenerate them from the preserved Phase 14B root analysis with:

```bash
python3 documentation/presentation/scripts/build_benchmark_data.py
```

The script reads the Phase 14B v3 `method_summary.csv`,
`paired_comparisons.csv`, `analysis_verification.json`, raw `accuracy.csv`,
raw `raw_timing.csv`, `method_workloads.csv`, run metadata, and the audited
method inventory. It checks the raw-file SHA-256 digests against the analysis
verification manifest and checks row counts before writing. It writes only:

- `phase14b_root_comparisons.json` — readable payload for review and data checks.
- `phase14b_root_comparisons.js` — the same payload assigned to
  `window.NUMERIX_BENCHMARK_DATA`, so the presentation can load it locally via
  `<script src>` under `file://` without fetch/CORS.

The payload's `coverage[]` records also carry an `operations[]` inventory for
all 49 audited method/operation rows. Each row includes the operation name,
public symbol, implementation path, Phase 14 coverage note, candidate
competitor, comparison class and Phase 14B method-level state. Root's nine
audited operations are marked measured; the other 40 are marked pending.

The script does not rewrite any source CSV, analysis output, Phase 14/14B
report, or historical figure. JSON numbers are derived from the verified
analysis rows rather than manually keyed presentation values.

## Data inventory

| Source/run | Verified contents |
| --- | --- |
| Phase 14B root v3 | Linux x86_64, GCC 8.5.0, Release-like `-O3 -DNDEBUG`, GSL 2.8 and CMINPACK 1.3.15; NUMERIX 0.2.0-rc1 |
| Accuracy CSV | 1,700 rows; 100 problem slots for each of 17 library/method routes |
| Timing CSV | 52,700 batches; 31 batches per problem/route, with two warmups in the run protocol |
| Method routes | 17 routes across NUMERIX (9), GSL (7) and CMINPACK (1) |
| Paired direct analysis | Five GSL/NUMERIX method-family comparisons; eligible sets require both methods to be applicable, successful and accuracy-passing |
| Method audit | 49 method/operation rows across root finding (9), linear algebra (20), differentiation (7), interpolation (4), integration (4), optimization (2) and ODEs (3) |

The statistical unit for direct ratios is the mathematical problem, not an
individual timing batch. Each eligible problem contributes the ratio of the
median of its per-call timing batches. The analysis uses 5,000 paired-problem
bootstrap resamples for descriptive 95% intervals.

## Direct paired timing results

The exact numbers below are read from `paired_comparisons.csv`. The ratio is
**GSL median wall time / NUMERIX median wall time**; values over one favor
NUMERIX on the tested eligible pairs. These are descriptive results, not a
statistical significance claim.

| Method | Eligible paired problems | Median ratio | Descriptive 95% bootstrap interval |
| --- | ---: | ---: | ---: |
| Bisection | 94 | 1.0818 | 1.0702–1.1020 |
| Regula Falsi | 95 | 1.0241 | 0.9871–1.0359 |
| Secant | 85 | 1.0754 | 1.0630–1.0921 |
| Newton | 90 | 1.0694 | 1.0590–1.0846 |
| Brent | 95 | 1.0372 | 1.0226–1.0600 |

Regula Falsi's descriptive interval includes one. Do not label that result as a
clear speed advantage. The individual all-attempt median runtime and reliability
counts are in each `methods[]` route record; they use a different denominator
from eligible paired ratios.

## How the presentation should interpret the records

- `methods[]` contains one record for every measured library/method route. The
  all-attempt median wall runtime, attempted/applicable counts, successful
  statuses, accuracy passes and successful-and-accurate counts are preserved.
- Each route's `problem_points[]` is a matched problem record. Its time is the
  median of that route's per-call batch times for that problem; its accuracy
  value and status come from the accuracy ledger. These records can support an
  **observed error versus runtime scatter at one configured tolerance**. They
  do not support a tolerance-sweep or time-to-accuracy curve.
- `paired_comparisons[]` contains only the five direct GSL/NUMERIX comparisons
  available in the analyzed file. It is appropriate for a paired speed-ratio
  chart with a reference at one and the recorded interval.
- The GSL safeguarded-Newton route actually uses GSL Brent as its best-available
  method. CMINPACK's Newton-labelled route is the MINPACK hybrid system solver,
  not scalar Newton. Multiple-root discovery compares NUMERIX against a
  fixed-grid GSL sign-change scan plus Brent refinement. The route records carry
  these distinctions from the source workload classification.
- Auto-selection and portfolio are NUMERIX-only orchestration routes. Do not
  invent competitor bars for them.
- The method inventory contains 49 audited operations. Phase 14B method-level
  measurements are available for root routes only; the other six modules are
  marked pending. A capability listing is not a measurement.
- Method-specific memory was not measured. Phase 14 process RSS includes
  harness/process overhead and is intentionally not used to compare methods.
- The run used one configured tolerance. No tolerance sweep or time-to-accuracy
  experiment was completed. No method-specific memory interception was
  completed.

These limits are also present in the generated payload's `limitations` field.
See the [Phase 14B method-level report](../../../documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md),
[analysis verification manifest](../../../benchmarks/results/phase14b/analysis/root/analysis_verification.json),
[paired comparison CSV](../../../benchmarks/results/phase14b/analysis/root/paired_comparisons.csv),
[method summary CSV](../../../benchmarks/results/phase14b/analysis/root/method_summary.csv),
[raw accuracy data](../../../benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/accuracy.csv),
[raw timing data](../../../benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/raw_timing.csv),
and [49-row method audit](../../../benchmarks/analysis/phase14b/method_coverage.csv).
