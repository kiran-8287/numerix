# Phase 14C API and comparison audit: differentiation, interpolation, integration

Audit date: 2026-11-12. Repository: NUMERIX working tree at audit time. This
document covers current public APIs in the three assigned modules and their
representative adapter pilots. It is **not a final benchmark report**.

## Scope verified from the public header and source

The current public header declares six numerical differentiation functions,
four interpolation functions (three numerical operations plus Newton setup),
and three integration functions. The generated Phase 14B capability inventory
contains these 13 rows plus two supporting piecewise numerical APIs, for 15
audited rows in this inventory. The current implementation files are:

- `src/algorithms/differentiation/differentiation.c`
- `src/algorithms/interpolation/interpolation.c`
- `src/algorithms/integration/integration.c`
- `src/piecewise/adapters.c` (the two supporting numerical APIs)

`numerix_interpolant_destroy` is a lifecycle operation, not an interpolation
algorithm. It must be exercised in setup/teardown memory and end-to-end
measurements, but should not be counted as another numerical method.

The row-by-row method/API mapping and direct-versus-best-available labels are
in [`gsl_scalar_modules_inventory.csv`](gsl_scalar_modules_inventory.csv).
The two `supporting_piecewise` rows are marked `not_comparable` and have no
competitor result; they are outside the seven core module studies.

## Competitor version and pilot provenance

The required competitor is GSL 2.8. The system `pkg-config` still resolves
GSL 2.5 (`-lgsl -lgslcblas -lm`; `libgsl.so.23`), but an isolated GSL 2.8.0
build is available at
`/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean`. The pilot was compiled
and linked against that prefix and run with its library directory in
`LD_LIBRARY_PATH`; it did not use the system GSL.

The three modules each have three predeclared problems in this pilot. The run
exercises API dispatch, identical inputs, independent references, counter
wrappers, 31 timing batches per route, and raw record formats. These are
**pilot-only observations, not module benchmark findings**. No comparative
performance conclusion is supported.

Host/compiler observed during the audit: Linux x86_64, GCC 8.5.0. This is
environment discovery only; it does not establish the benchmark CPU model,
thread policy, or final run conditions.

## Workload and accuracy plan

Freeze one manifest before measurement. For each of the three modules, target
at least 100 distinct mathematical problems from at least 20 documented
families, with multiple scales or sizes. A problem row should include an
immutable ID, formula/data, parameters, family, size/difficulty, applicable
methods, required quality, independent reference and reference-generation
method. NUMERIX and GSL receive the same inputs and callbacks. Retain every
attempt and status.

### Differentiation

Use analytic derivatives for polynomial, trigonometric, exponential,
logarithmic, rational and smooth composite families, with high-precision
independent evaluation for the reference. Include scaled coordinates, small
and large derivatives, cancellation-prone points, smooth versus nonsmooth
cases, and points near finite-domain boundaries. Hold `x` and initial `h`
identical; record function evaluations, actual stencil behavior where
observable, absolute/relative derivative error, status, and requested target.
Use a predeclared h/quality protocol: do not tune each library after seeing its
results. GSL helper methods adjust their work/step estimates, so their routes
are best-available comparisons, not same-stencil comparisons.

### Interpolation

Use frozen analytic functions sampled at identical nodes. Cover uniform and
well-spaced nonuniform nodes, increasing node counts, smooth polynomial and
non-polynomial functions, extrapolation enabled/disabled behavior, and
conditioning-sensitive node layouts. Evaluate error on a separate fixed holdout
grid, never only at interpolation nodes. For Lagrange and Newton, record setup
time and evaluation time separately, plus create/evaluate/destroy end-to-end
time. Compare memory with the identical input arrays excluded consistently;
include interpolant storage and GSL object/accelerator/workspace allocations.
GSL polynomial interpolation is a best-available representation comparison,
not an identical Newton/Lagrange implementation.

### Integration

Use independently known analytic integrals or high-precision arbitrary-
precision quadrature references. Include smooth, oscillatory, narrow-peak,
scaled, cusp/piecewise, and integrable endpoint-singularity families, with
reversed and zero-length intervals as explicit status/edge workloads. Fixed
NUMERIX rules receive frozen panel counts; GSL fixed Gauss-Legendre uses a
predeclared evaluation budget, and must be labeled best-available. Adaptive
Simpson versus GSL QAG is also best-available: compare achieved reference error,
callback count and runtime under a common accuracy target and explicit work
budget, not by passing equal relative/absolute tolerance values and assuming
equivalent stopping quality. Record absolute and scale-normalized error,
reported status, function evaluations, iterations/subdivisions if observable,
and whether the requested quality was attained.

## Measurement controls and limitations to settle before a full run

- Use monotonic wall and CPU clocks, warmups, at least 31 measured batches per
  problem/route where practical, and counterbalanced library order. Retain raw
  batch data; the independent unit for paired inference is the mathematical
  problem, not its repeated timer batches.
- Separate steady-state cost from initialization/setup. Save end-to-end cost
  too where normal public use includes object construction or destruction.
- Pin the single-thread policy and record the exact GSL build/compiler and host.
- Use a common C callback wrapper with an explicit evaluation counter, because
  the NUMERIX result counts evaluations but the GSL helper does not return that
  count itself. Keep the callback and inputs identical.
- Measure additional method-owned peak live requested bytes, total requested
  bytes and allocation count, excluding harness/input storage consistently.
  Validate allocation interception by forcing known allocations in NUMERIX
  and GSL, including library-internal allocations and workspace creation. If
  interposition cannot see an allocation path, mark that route unavailable;
  do not write zero bytes. GSL interpolation contexts and integration
  workspaces are setup state and should be reported separately from per-call
  scratch, then included in an end-to-end lifecycle measurement.
- Keep all failures and non-finite outputs in raw/accuracy records. Paired
  runtime ratios must state their eligibility rules and counts.

## Status

| Module | Current API methods | Competitor required | GSL status | Phase 14C measurements |
|---|---:|---|---|---|
| Differentiation | 6 | GSL 2.8 | Isolated 2.8.0 used | 3 pilot problems; 31 timing batches/route; 36 accuracy rows |
| Interpolation | 4 method/setup rows | GSL 2.8 | Isolated 2.8.0 used | 3 pilot problems; 31 timing batches/route; setup has no numerical output |
| Integration | 3 | GSL 2.8 | Isolated 2.8.0 used | 3 pilot problems; 31 timing batches/route; fixed rules and QAG differ |
| Supporting piecewise APIs | 2 | None assigned | Not applicable | Not benchmarked; not comparable |

The canonical corrected pilot run is
`phase14c-pilot-scalar-20261112T055820Z`; its raw files are under
`benchmarks/results/phase14c/module_comparisons/raw/{differentiation,interpolation,integration}/`.
It records 1,116 differentiation timing rows, 744 interpolation timing rows,
and 1,116 integration timing rows, with 31 batches in every route group and no
timing-status failures. Accuracy records include 36 differentiation attempts
with 36/36 successful native statuses and 30/36 meeting the pilot accuracy
target; 18 interpolation evaluations with 18/18 successful statuses and 4/18
meeting the pilot target, plus six successful setup-only rows where numerical
accuracy is not applicable; and 18 integration attempts with 18/18 successful
statuses and 14/18 meeting the pilot target. These are distinct status and
accuracy-pass counts. The pass counts reflect only illustrative pilot targets
and are not comparative findings.
Memory rows explicitly say `unavailable` because allocation interception has
not been implemented or validated. The pilot did not meet the 100-problem/20-
family coverage target or represent varied difficulty and size levels.
Adaptive Simpson and QAG do not have matched work limits in this pilot, so that
timing route is an adapter smoke check only. Composite trapezoid/Simpson versus
Gauss-Legendre are best-available comparisons, not direct algorithm matches.
Earlier debug iterations remain preserved and are indexed as superseded in
`manifests/superseded_pilot_runs.csv`; the pilot README identifies the canonical
run.

No final runtime, memory, or accuracy conclusion is supported by this audit.
The existing Phase 14B root data are outside this module audit and remain
historical.
