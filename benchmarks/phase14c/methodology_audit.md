# Phase 14C independent methodology and memory audit

Audit date: 2026-11-12. This review covers the draft benchmark contract, the
current heap interposer and its smoke checks, and the Phase 14C pilot artifacts
present in this workspace. **The workspace does not contain a complete Phase
14C benchmark.** Pilots below support adapter, dependency, correctness, and
recording checks only. They do not support a general performance or memory
ranking.

## Audit findings

### 1. Full-module evidence is not available

The intake manifest has 700 candidate inputs (100 per module), copied from
historical Phase 14 data. Its source run and hashes are retained, so those rows
are useful workload candidates, not new Phase 14C observations. The non-root
module pilots cover only three scalar problems, four linear-algebra problems,
two optimization problems, and one ODE system. **No new scalar root-finding
pilot output is present.** The 31 timing batches in scalar and linear-algebra
pilots are repeated measurements of the same small set of inputs; they are not
independent problems. Bootstrap intervals over these pilot problem counts
would be unstable and should not be used to make module-level claims.

### 2. Several adapter comparisons are correctly marked best available, but
the pilot tolerances expose expected method error

- Differentiation compares NUMERIX finite-difference routes with GSL derivative
  helpers. GSL may choose internal steps/stencils, so these are best-available
  comparisons and callback counts must remain separate. On the canonical
  scalar run, 30/36 accuracy records pass; six NUMERIX forward/backward results
  fail the `1e-5` target at the frozen steps. These methods have first-order
  truncation error at those steps, so the pilot demonstrates visible accuracy
  failures; it does not demonstrate an implementation/status failure.
- Interpolation has 24 accuracy rows: six setup rows are `not_applicable`, and
  only 4/18 evaluation rows pass the `1e-7` analytic-function target. With the
  frozen coarse nodes, large error for linear interpolation and the degree-four
  polynomial approximants is expected. For the final study, retain two
  different questions: agreement between implementations of the same
  interpolant, and approximation error against the analytic function. The
  latter target must be selected before results and must account for node
  density and interpolation order.
- Integration passes 14/18 accuracy records. Composite trapezoid/Simpson and
  fixed Gauss-Legendre are different quadrature rules; adaptive Simpson and
  GSL QAG also have unmatched stopping/work policies. Keep these routes
  best-available, record function evaluations and stopping limits, and do not
  interpret equal wall batches as equal numerical work. The pilot accurately
  retains the four accuracy failures.
- Linear algebra compares CBLAS routes directly for add/GEMM and labels the LU
  algorithm difference. Its LU factorization end-to-end route includes the
  row-major/column-major conversion and output allocations; solve-only uses
  each library's own saved factors and has separate setup records. This is a
  reasonable pilot scope. The 4 input cases are still far too few for method
  coverage or general performance conclusions.
- Optimization uses a 200-objective-call cap for both implementations and
  counts gradient calls separately. Gradient descent and L-BFGS use the same
  objective and initial point, but are best-available algorithms. The
  NUMERIX golden-section API has no start-point input; its NLopt DIRECT-L
  counterpart cannot be represented as an identical-start comparison. The
  pilot correctly preserves NLopt `MAXEVAL_REACHED` as a non-converged limit
  even when the independent accuracy check passes.
- ODE compares matching fixed-step tableaus and checks every point on one
  analytic two-state trajectory. This is the strongest direct-method
  correctness smoke, but one system does not cover coupled, oscillatory,
  nonlinear, stiff, or backward integration. The archived six-row pilot CSV
  reports errors and callback counts but correctly leaves `accuracy_pass`
  unevaluated because the original pilot did not define an error threshold.

The scalar integration and differentiation pilots use fixed steps/work limits
without common per-case budgets across all unlike methods. For final
optimization and adaptive integration records, freeze whether a budget counts
objective calls, gradients, RHS calls, intervals, iterations, or elapsed time.
Record each counter independently and preserve native stop codes. A common
budget improves repeatability; it does not make unlike algorithms equivalent.
In the scalar pilot files, the listed failures are accuracy-gate failures:
the tested numerical calls report success, while setup rows are separately
marked `not_applicable`.

### 3. Raw data need a single normalized schema before final analysis

The current agents' outputs use related but different field names and scopes.
The scalar GSL timing file uses `batch_order`, while the contract specifies
`call_order` and `order_seed`; its accuracy file uses `method`, `pass`, and a
single scalar reference rather than the contract's `numerix_method`,
`accuracy_pass`, and reference/observed JSON. Linear-algebra files use
`comparison_class`, `monotonic_wall_elapsed_seconds`, and their own memory
status names. The optimizer CSV is closer to the proposed accuracy schema; the
ODE pilot had no saved CSV until this audit created one. These are acceptable
pilot-local schemas, but a final analysis must normalize them without losing
raw provenance or turning `not_applicable`, failure, or unavailable into zero.

The scalar pilot produces 31 rows per route and alternates order. Linear
algebra also produces 31 rows per route, but calibrates each route separately
to a target two-millisecond batch. In the accepted run
`phase14c-la-pilot-20261112T055827Z`, paired route batch sizes differ:
add 16/19 calls, GEMM 30/85, LU factor 18/55, and LU solve 358/651.
The summary divides elapsed time by calls, which is useful for a pilot, but
different repetition counts may affect cache, allocator, and frequency state.
For final timing, document this duration-calibration design and show per-call
stability across batch sizes, or select a common count after calibration.
Keep the per-problem batch median as the primary sample; do not treat 31
batches as 31 independent samples.

Timer provenance is recorded, but no saved pilot reports `clock_getres` or
enough calibration observations to verify timer stability; linear algebra
chooses each route's batch size from a single calibration call. Scalar GSL
uses `CLOCK_MONOTONIC` and process CPU time; linear algebra uses
`CLOCK_MONOTONIC_RAW` and thread CPU time. Keep those clock identities in every
run and do not compare elapsed values across modules as though their timer
paths were identical. The machine inventory says CPU affinity should be pinned,
but the pilot run metadata does not record an actual affinity mask. Pin and
record affinity for the final run, or explicitly report that it was not pinned.

The scalar pilot records adapter, script, verifier, and manifest hashes, but
not hashes of its raw CSVs. Linear algebra records output hashes; optimization
records its raw accuracy hash; the ODE pilot metadata records source, CMake,
binary, and CSV hashes. Hash every final raw output and summary. All inspected
run metadata reports a dirty Git tree; the same Git head does not identify all
uncommitted source changes. Preserve file hashes or run from a clean, tagged
snapshot. The accepted scalar-method run is
`phase14c-pilot-scalar-20261112T055820Z`, as recorded by the current README,
run ledger, audit, and verification. The generated Phase 14C figure set uses
these small pilots and the explicitly historical Phase 14B root source; it is
an evidence/status set, not the requested full study. Missing runtime and
memory values are labeled “not measured,” and the figures support no
module-wide ranking.

## Heap interposer review and smoke evidence

[`memory/phase14c_heaptrace.c`](memory/phase14c_heaptrace.c) is a Linux/glibc
allocator interposer. Its fixed, static pointer table avoids recursive heap
allocation; a lock protects the pointer map; and it tracks requested sizes,
not allocator capacity. It hooks `malloc`, `calloc`, `realloc`, `free`,
`aligned_alloc`, `memalign`, and `posix_memalign`. It does not observe arbitrary
custom allocators, direct `mmap`, stack storage, or all libc allocation paths;
`valloc`/`pvalloc` are not explicit hooks. A missing control allocation can
leave an unfreed pointer unrecognized, so `valid=1` alone is not proof that an
entire dependency's allocations are visible.

I compiled and ran the three available smoke programs against the current
source with `-Wall -Wextra -Wpedantic -Werror`. The synthetic allocation
sequence passed with `valid=1`, peak 970 requested bytes, total requested
1,090 bytes, 6 allocations, 1 successful reallocation, 6 frees, zero
current-live delta, and zero untracked pointers; it also covers aligned
allocation and an oversized failed `realloc`. The executable
linked against the isolated GSL 2.8 prefix resolved `libgsl.so.28` and
`libgslcblas.so.0` from that prefix; its vector-allocation smoke reported peak
856 bytes, 3 allocations/frees, and zero untracked pointers. The NUMERIX
`vector_add` smoke returned the expected value and reported peak 816 bytes, 2
allocations/frees, and zero untracked pointers. These are **instrumentation
visibility checks for these exact paths and calls**, not algorithm memory
results or Phase 14C `raw_memory.csv` values. The GSL/NUMERIX smoke programs
link the interposer into the executable; they do not validate a separately
loaded `LD_PRELOAD` shared object.

The initial strict-warning issue with `dlsym` function-pointer conversion was
fixed by copying the POSIX symbol representation into function-pointer
objects. All three smokes now pass with `-Wpedantic -Werror`. This does not
validate a separately loaded `LD_PRELOAD` interposer or full dependency-route
visibility.

Additional memory gates before reporting heap numbers:

- Run a controlled positive allocation through each actual final adapter and
  loaded dependency route: GSL, OpenBLAS/LAPACK, NLopt, and SUNDIALS/serial
  N_Vector, as well as NUMERIX. Check aligned allocation if those routes use
  it. The current evidence covers only synthetic calls, one GSL vector
  allocation, and one NUMERIX vector operation.
- The synthetic control now covers successful and failed `realloc`,
  `aligned_alloc`, `posix_memalign`, and `memalign`, with all allocated control
  pointers freed. Zero-size `realloc` behavior and positive controls through
  the OpenBLAS/LAPACK, NLopt, and SUNDIALS routes remain open. A failed/missed
  dependency control makes that route unavailable; do not emit zero.
- Use one isolated process/sample per operation, initialize harness/input and
  reusable state before `begin`, and keep memory capture separate from timed
  runs because the interposer's table/lock adds overhead. Report setup,
  operation, and end-to-end scopes separately and allocate output buffers
  symmetrically.
- The pointer table is process-global; concurrent background allocations
  contaminate a scope. Keep benchmark routes single-threaded and isolated.
  The table itself is static instrumentation memory and is not included in
  requested-heap values; RSS must remain process-level context only.
- `peak_additional_live_requested_bytes` is computed as current tracked live
  bytes minus the begin baseline, clamped at zero. Freeing a baseline object
  during a measured scope can mask later allocations in that delta. Keep
  baseline objects alive during operation-only scopes or separately report
  lifecycle/retained-result ownership.

Every inspected Phase 14C pilot currently marks memory unavailable or has no
memory rows. The smoke checks do not change that status.

## Pilot evidence that is safe to report

| Area | Safe pilot statement | Not supported |
|---|---|---|
| Scalar GSL modules, canonical run `055820Z` | GSL 2.8 loaded from its isolated prefix; 31 timed batches per route; derivative accuracy 30/36, interpolation 4/18 evaluated cases (6 setup rows N/A), integration 14/18; all attempts retained; all memory rows explicitly unavailable. | Module-wide speed ranking, full accuracy coverage, heap-memory comparison, or 100-problem statistics. |
| Linear algebra, run `phase14c-la-pilot-20261112T055827Z` | Exact OpenBLAS 0.3.34/LAPACK 3.12.0 metadata, one OpenBLAS thread, four inputs, eight 31-batch routes, eight accuracy rows passing, two setup rows passing; memory unavailable. | Full 18 numerical-operation coverage, 100-problem generalization, or memory values. |
| Optimization, `phase14c-opt-correctness-pilot-20261112` | NLopt 2.10.1 runtime check; four accuracy records pass; native DIRECT-L status 5 remains `MAXEVAL_REACHED`/`limit_reached`/not converged; objective and gradient calls are separate. | Timing or general optimizer ranking. |
| ODE, `phase14c-ode-correctness-pilot-20261112T060326Z` | Exact SUNDIALS 7.9.0 tableau checks; analytic errors and cross-library deltas for all 21 points of one two-state system, with RHS counts retained. | Timing, memory, full ODE family coverage, or a pass/fail tolerance not defined in the pilot. |
| Heap interposer smoke | Known allocation accounting and visibility through the tested GSL 2.8 and NUMERIX paths, subject to the build and path limitations above. | Per-method memory, all-library visibility, RSS equivalence, or validated `LD_PRELOAD` deployment. |

The saved ODE records are in
[`pilots/ode/raw_accuracy_pilot.csv`](pilots/ode/raw_accuracy_pilot.csv), with
run provenance in [`pilots/ode/run_metadata.json`](pilots/ode/run_metadata.json).
