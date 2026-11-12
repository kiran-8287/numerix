# Phase 14C scalar-module adapter pilot

This is a **pilot-only** harness for differentiation, interpolation, and
integration. It is not a full Phase 14C comparison and does not support a
performance ranking or module-level conclusion.

## Reproduce the corrected pilot

The harness uses the NUMERIX release archive already built at
`build/linux-gcc-release/libnumerix.a` and the isolated GSL 2.8.0 build at
`/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean`. The system GSL 2.5 is
not used. Run from the repository root:

```bash
benchmarks/phase14c/pilots/gsl_scalar_modules/run_pilot.sh
```

The script freezes a three-problem-per-module manifest first, compiles with
GCC and warnings as errors, confirms the resolved GSL shared-library path with
`ldd`, collects 31 alternating-order timing batches per route, writes raw
accuracy records, marks memory unavailable, calculates per-route pilot
summaries, writes provenance and runs schema checks.

The canonical corrected run is
`phase14c-pilot-scalar-20261112T055820Z`. Its problem manifest, pre-run
manifest checksum, and linkage record are in
`benchmarks/results/phase14c/module_comparisons/manifests/`.
Raw files live in `raw/{module}/{run_id}/`; per-route summaries live in
`analysis/{module}/{run_id}/pilot_route_summary.csv`; the automated pilot
check is in `verification/`.

## What this pilot checks

- NUMERIX/GSL API dispatch for all six scalar derivative methods, all
  interpolation methods including setup and evaluation, and all three
  integration methods.
- Identical callback definitions, arguments, nodes, intervals, references,
  and target thresholds for corresponding routes.
- Independent analytic reference calculations and visible accuracy failures.
- 31 batch records per route, with alternating execution order and wall/CPU
  clock fields.
- Explicitly unavailable memory values. Allocation interception was not
  implemented or validated, so no memory quantity is represented as zero.

The three pilot problems per module are far short of the full plan's 100
problems and 20 families. GSL derivative helpers adjust their stencil/work;
the finite-difference routes are best available. GSL fixed Gauss-Legendre is
not the same method as composite trapezoid or Simpson. The adaptive Simpson
and GSL QAG work limits are not matched here. Treat the adapter smoke timings
as method-call checks only; do not publish them as comparative findings.

No graphs are created from this pilot. Graphs belong after a full, frozen,
fairly budgeted dataset and memory instrumentation have passed verification.

`superseded_pilot_runs.csv` records earlier harness iterations retained in the
raw folders for provenance. Use only the corrected canonical run above for
the pilot schema review.
