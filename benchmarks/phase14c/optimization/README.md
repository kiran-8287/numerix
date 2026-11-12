# Phase 14C optimizer correctness pilot

This is a **two-case correctness pilot** for the two public NUMERIX optimizer
methods. It is not a performance benchmark. Both routes receive a maximum of
200 objective evaluations, and the adapter records gradient callbacks
separately. Native status, normalized status, convergence, and post-run accuracy
are independent fields; reaching the evaluation cap is never labeled success.

| NUMERIX method | NLopt route | Matching properties | Difference |
|---|---|---|---|
| Golden section | `NLOPT_GN_DIRECT_L` | Same bounded scalar objective and interval `[-2, 2]` | Best-available comparison. NUMERIX has no initial-point argument; NLopt takes an input vector, but DIRECT-L does not provide an equivalent bracketed start. |
| Gradient descent | `NLOPT_LD_LBFGS` | Same unbounded 2D objective, initial point `[-3, 4]`, and objective-evaluation budget | Best-available comparison. L-BFGS is quasi-Newton rather than steepest descent with Armijo backtracking. |

Analytical minima are frozen in the manifest. The one case per route checks
adapter dispatch, callback accounting, native-status preservation, and the
independent location/objective accuracy calculation. It does not establish
method-wide correctness or a stable performance distribution.

## Build and run

```sh
cmake -S benchmarks/phase14c/optimization \
  -B build/phase14c-optimization-pilot \
  -DCMAKE_BUILD_TYPE=Release \
  -DNLOPT_ROOT=/home/142401034/.local/numerix-deps/prefix/phase14c/nlopt-2.10.1
cmake --build build/phase14c-optimization-pilot --parallel
python3 benchmarks/phase14c/optimization/run_correctness_pilot.py
```

The executable rejects any loaded NLopt version other than 2.10.1. The runner
checks manifest parameter hashes, per-method record coverage, the common
objective budget, and that NLopt status `NLOPT_MAXEVAL_REACHED` remains a
non-converged `limit_reached` status. It writes `raw_accuracy_pilot.csv` and
`run_metadata.json` under `benchmarks/phase14c/pilots/optimization/` with
source and output checksums.

## Observed pilot output

All four library/method records passed the `1e-5` independent location and
objective checks. NUMERIX golden section converged in 29 objective calls;
NLopt DIRECT-L used all 200 and returned native status 5 (`MAXEVAL_REACHED`),
which remains `limit_reached` and `converged=0` despite passing accuracy.
NUMERIX gradient descent converged in 94 objective and 94 gradient calls;
NLopt L-BFGS converged in 5 objective and 5 gradient calls. These counts are
pilot observations for these two inputs only; they are not timing results or
claims of comparative speed.

No timing, RSS, or heap measurements were taken. The memory methodology and
allocator-visibility limitations are reviewed in
[`../optimization_ode_audit.md`](../optimization_ode_audit.md).
