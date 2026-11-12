# Phase 14C optimization and ODE API / methodology audit

Audit date: 2026-11-12. This is an API, dependency, and method-mapping audit.
The attached ODE and optimizer runs are correctness-only pilots. **No Phase
14C timing or memory measurements were collected.** The optimizer pilot covers
one case per public optimization method; it does not establish method-wide
accuracy.

## Public numerical API verified

The inventory in [`optimization_ode_inventory.csv`](optimization_ode_inventory.csv)
was checked against `include/numerix.h` and the current implementation files:

- `src/algorithms/optimization/optimization.c`
- `src/algorithms/ode/ode.c`

The numerical scope is two optimization methods and three ODE methods. The
`numerix_ode_solution_destroy` function is lifecycle management, not a fourth
ODE method. The optimizer solution/vector destroy functions are likewise
lifecycle operations. They must be included in end-to-end and allocation
protocols where their corresponding object creation is included, but should not
be presented as extra numerical algorithms.

### Optimization behavior and work accounting

- `numerix_optimize_golden_section` is bounded, scalar golden-section search.
  The public call allocates no explicit heap workspace in the current source;
  its result is scalar and its working state is automatic local storage. This
  source inspection is not a memory measurement. NLopt solver-object setup and
  teardown must be kept separate from `nlopt_optimize` and also captured in an
  end-to-end lifecycle scope.
- `numerix_optimize_gradient_descent` accepts an unconstrained vector initial
  point, positive initial step, objective callback and gradient callback. The
  current implementation uses steepest descent with Armijo backtracking. It
  allocates three `n`-element arrays (`x`, `g`, `trial`) and the returned
  solution struct; `x` becomes the returned solution data while `g` and
  `trial` are freed before return. The public `function_evaluations` counter
  counts objective calls, not gradient calls. Pilot and final records must
  count objective and gradient callbacks externally and retain both.
- NLopt has no matching golden-section algorithm. Use bounded `NLOPT_GN_DIRECT_L`
  for scalar minimization as **best available**, not direct. For vector
  gradient descent use a named gradient-based NLopt local method (for example
  `NLOPT_LD_LBFGS`) as **best available**; it is quasi-Newton and has a
  different search/termination rule. Set the same input point and objective
  evaluation budget, track gradient evaluations separately, and compare
  objective gap and minimizer distance against the frozen reference. A common
  target must be evaluated from achieved output, not inferred from solver
  return status.
- `numerix_optimize_golden_section` reports a bracket-derived position error;
  its returned `residual` field contains the objective value. Treat these as
  distinct metrics. Do not use the scalar minimizer coordinate as the
  optimization objective value.

### ODE behavior and method equivalence

- The only public numerical ODE methods are fixed-step Euler, Heun, and
  classical RK4. Inputs include the initial state, dimension, one signed step,
  and a step count. NUMERIX returns the **entire** `steps + 1` trajectory and
  reports the RHS callback count. There is no adaptive NUMERIX ODE method in
  the current header.
- SUNDIALS 7.9.0's serial ARKODE ERKStep tables support direct fixed-step
  comparisons for all three methods when the tableau is verified: forward
  Euler (`ARKODE_FORWARD_EULER_1_1`), Heun's explicit trapezoid
  (`ARKODE_HEUN_EULER_2_1_2`), and classical RK4
  (`ARKODE_KUTTA_RK4a_4_4`). The checked local source table has the expected
  coefficients. The companion pilot asserts the coefficients at runtime and
  compares all output points on one analytic two-state exponential system.
  It is not a timing run.
- Adaptive CVODE Adams can be an optional **best-available** ODE comparison;
  it is not equivalent to the fixed-step NUMERIX methods. Keep it separate,
  compare at the same requested output points and common achieved-error
  target, and preserve accepted/rejected steps and RHS-evaluation counts.
- Accuracy must compare all trajectory states at identical time points.
  Normalize trajectory error by a documented state/reference scale and retain
  final-state error separately. Analytic exponential systems are suitable
  pilot references; the final manifest should additionally use coupled,
  oscillatory, nonlinear, stiff, forward and backward systems with frozen
  parameters.

## Dependency discovery

Default system `pkg-config` did not find NLopt or SUNDIALS. The private Phase 14
prefixes were then checked directly:

| Library | Exact local build | Evidence |
|---|---|---|
| NLopt | 2.10.1 for the correctness pilot; 2.11.0 also remains installed | The pilot loads only `/home/142401034/.local/numerix-deps/prefix/phase14c/nlopt-2.10.1`, checks `nlopt_version()` at startup, and recorded runtime version 2.10.1. The separate Phase 14 prefix remains NLopt 2.11.0. |
| SUNDIALS | 7.9.0 | `SUNDIALSConfigVersion.cmake` reports 7.9.0; headers and imported CMake targets `SUNDIALS::arkode_shared`, `SUNDIALS::nvecserial_shared`, and `SUNDIALS::core_shared` are present under the private prefix. |

The current machine also exposes unrelated MATLAB-bundled SUNDIALS runtime
libraries (CVODE 5.7 / ARKODE 4.7). They are **not** the validated Phase 14
prefix and must not be linked for a 7.9.0 run. The phase-specific CMake pilot
requires an exact SUNDIALS 7.9.0 package version.

## Phase 14 results: historical context, not Phase 14C coverage

The preserved `benchmarks/results/phase14/main` run contains 100 optimization
problems and 100 ODE problems, each with 20 families × 5 cases and 31 timing
batches per route. Its manifest/source checksum is
`9a93895682caf257a8aa61e9c47b796f8e22f2fe5510a8aaefded51877b3b3f1` for
`benchmarks/problems/phase14/problem_inputs.jsonl`. These artifacts may guide
the next frozen manifest, but their `run_id` and old dependencies must remain
historical; they are not new Phase 14C observations.

Specific gaps relative to this task:

1. The Phase 14 optimization run measured NUMERIX golden section, GSL golden
   section, and NLopt DIRECT-L, but did not measure the public NUMERIX gradient
   descent method. Its NLopt build was 2.11.0 rather than the Phase 14C
   contract's 2.10.1.
2. The Phase 14 ODE run measured NUMERIX/GSL/ARKODE fixed RK4 and adaptive
   CVODE; it did not measure NUMERIX Euler or Heun.
3. The existing Phase 14 adapter extracts only the **final state** for the
   SUNDIALS ODE paths. It does not establish the requested full-trajectory
   accuracy comparisons.
4. Phase 14 `raw_memory.csv` stores isolated-process peak RSS. Its run metadata
   explicitly says requested heap visibility is unavailable and process RSS
   includes the harness and shared-library baseline. Allocation counts and
   requested heap bytes are blank. It cannot support method-specific memory
   graphs.

The old NLopt DIRECT-L records show a status/accuracy split that must remain
visible: all 100 native statuses were `NLOPT_MAXEVAL_REACHED` (native status
5), mapped to NUMERIX `LIMIT_REACHED` (status 6), while all 100 recorded
minimizer results passed the independent location/objective accuracy checks.
This is not 100 converged solver statuses. The Phase 14C workload and budget
must make the status policy explicit rather than mapping max-evaluation exit
to success after seeing an accurate point.

Thus historical runtime/accuracy routes are useful for selecting pilot
workloads but do not satisfy Phase 14C's method completeness, full-trajectory,
version, or method-memory gates.

## Memory instrumentation audit and proposed scope

Current NUMERIX heap behavior is visible in the implementation source:

- Golden-section search has no explicit heap allocation in the current source.
- Gradient descent allocates three dimension-sized `double` work arrays and a
  returned vector descriptor; the state vector allocated as `x` is transferred
  to the result, while the gradient and trial buffers are released on success.
- ODE methods allocate returned `x`/`y` trajectories plus stage arrays. Their
  workspace arrays scale with dimension: Euler allocates `k1` and `temporary`,
  Heun adds `k2`, and RK4 adds `k2`, `k3`, and `k4`. These are source-derived
  observations, not recorded memory values.

An `LD_PRELOAD` allocator interposer can observe ordinary dynamic `malloc`,
`calloc`, `realloc`, and `free` calls in the runner and dynamically linked
libraries. It still must be validated for each actual dependency route. It may
miss custom allocators, direct `mmap`, static/runtime-internal allocation,
aligned allocation (`aligned_alloc`, `posix_memalign`), C++ allocation paths,
and stack storage. Interposer
metadata must not allocate recursively; use a recursion-safe fixed record
table or equivalent. Report requested live bytes, not allocator metadata or
RSS, and leave a route unavailable if a controlled test allocation inside its
dependency is not observed.

For apples-to-apples ODE memory, preallocate identical input state and a
trajectory buffer for SUNDIALS because its normal evolution interface returns
states incrementally, while NUMERIX returns the complete trajectory. Include
the NUMERIX output arrays and the equivalent SUNDIALS trajectory buffer in
both operation scopes. Report solver/context setup, solve peak, and
create-solve-destroy lifecycle separately. For NLopt, report optimizer-object
creation separately and also include it in end-to-end optimization memory.
Record the scope and the allocation-visibility test result in every memory
row; do not turn source-derived zero allocation or unavailable interception
into an unverified memory measurement.

## Status

| Module | Methods inventoried | New Phase 14C pilot | New measurements |
|---|---:|---|---|
| Optimization | 2 | Passed correctness pilot with NLopt 2.10.1: one analytic input each for golden section vs DIRECT-L and gradient descent vs L-BFGS; all four records passed location and objective checks. Golden NLopt returned native `MAXEVAL_REACHED` (5), preserved as `limit_reached`, `converged=0`, despite accuracy passing. Calls: NUMERIX/NLopt golden 29/200 objective; gradient descent/L-BFGS 94/5 objective and 94/5 gradient. | No timing or memory measurements |
| ODE | 3 | Passed: SUNDIALS 7.9.0 exact table checks and all 21 trajectory points on one analytic two-state case; max NUMERIX/SUNDIALS trajectory deltas were `2.66e-15` (Euler), `3.55e-15` (Heun), and `7.11e-15` (RK4). SUNDIALS RHS counts include one initial startup call (21/41/81 vs NUMERIX 20/40/80). | No timing or memory measurements |

## Review of the shared Phase 14C measurement contract

The contract's core safeguards are appropriate for these modules: freeze the
workload before final measurements, include failed attempts, retain native and
normalized statuses independently, classify unlike methods as best available,
and calculate accuracy against an independent reference. Two extra rules are
needed in the optimization workload/method metadata to make the common budget
meaningful: state whether the budget counts objective calls, gradient calls,
or both, and record every callback count separately. For NLopt, `maxeval` caps
objective callbacks even when the callback also computes a gradient; the pilot
uses a shared cap of 200 objective evaluations and reports gradient calls
separately. A shared evaluation cap does not make differently structured
algorithms equivalent, so compare quality/status first and use call-count
curves only as work accounting, not as a proxy for elapsed time.

The pilot also illustrates where literal input parity cannot be achieved from
the public APIs. Gradient descent receives the same initial point, no bounds,
and the same objective in both implementations. Golden-section receives the
same objective and bracket: NUMERIX exposes no initial-point argument, while
NLopt DIRECT-L is global and its `x` input is not an equivalent bracketed
start. This remains `best_available` and is disclosed in the manifest and
metadata. It must not be presented as an identical-start comparison.

The memory contract correctly disallows calling RSS method-specific memory
and labels unobserved heap instrumentation unavailable. Before trusting a
requested-live-byte series, add positive-control allocations through every
loaded route (NUMERIX, NLopt, ARKODE/NVector), test `malloc`/`calloc`/`realloc`/
`free` and aligned allocation paths actually used, and verify the interposer
does not allocate recursively or lose `realloc` ownership transitions. Check
that shared-library and static-library routes are both visible. A missed
control allocation invalidates that route's heap series; it is not a zero.
Keep the contract's baseline, setup, operation, and end-to-end scopes distinct,
and treat output/trajectory buffers symmetrically. The current pilots have not
run allocator controls, so the memory gate remains open and no memory values
are reported.

For fixed-step ODE comparisons, exact tableau checks and identical output
times are strong direct-method controls. SUNDIALS startup callback counts
differ from NUMERIX by one on the pilot, so RHS counts must stay visible when
interpreting work; they are not evidence of a numerical mismatch. Adaptive
CVODE remains a separate best-available route and should compare interpolation
at the same requested times with achieved error and adaptive-step counters
retained.
