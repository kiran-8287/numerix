# Phase 14C linear algebra work

## Canonical pilot

The accepted representative pilot is
[`phase14c-la-pilot-20261112T055827Z`](pilot_runs/phase14c-la-pilot-20261112T055827Z/).
Its [`verification.json`](pilot_runs/phase14c-la-pilot-20261112T055827Z/verification.json)
has `passed: true`, 4 frozen problems, 8 method routes, 248 timing rows (31
batches per route), 8/8 accuracy passes, 2/2 setup passes, and 8 memory rows
marked `unavailable`. Its API inventory check covers 21 grouped rows (18
numerical, 3 lifecycle) and 23 public symbols.

The run uses OpenBLAS 0.3.34 with one runtime thread and LAPACK/LAPACKE 3.12.0
from the pinned Phase 14 prefixes. The LAPACK prefix metadata reports 3.12.0;
the historical manifest's 3.12.1 value is recorded as a discrepancy in
[`benchmark_plan.md`](benchmark_plan.md) and the run metadata.

Reproduce the build and pilot with:

```sh
python3 benchmarks/phase14c/linear_algebra/run_pilot.py
```

The canonical run supersedes earlier iterations after the lifecycle inventory
was corrected to include `numerix_lu_destroy`. Earlier output directories are
preserved for provenance.

## Scope

This is a four-problem method pilot, not the full study. Its routes are matrix
addition, matrix multiplication, LU factorization, and saved-factor LU solve.
No overall performance winner is concluded. Memory visibility is unmeasured.
See [`api_audit.md`](api_audit.md) for the public API implementation/test
cross-reference and [`benchmark_plan.md`](benchmark_plan.md) for the expanded
workload design and remaining work.
