# Phase 14C linear algebra audit and benchmark plan

## Current status

This is an API/dependency audit and representative pilot, not a completed
benchmark. No 100-problem full workload has been measured and no general
performance conclusions are supported. The fixed competitor is LAPACK with
OpenBLAS as its BLAS backend. Pilot builds use the designated prefixes:
OpenBLAS 0.3.34 at
`/home/142401034/.local/numerix-deps/prefix/phase14/openblas` and reference
LAPACK/LAPACKE 3.12.0 at
`/home/142401034/.local/numerix-deps/prefix/phase14/lapack-reference`.
OpenBLAS reports single-thread settings through the runner. The system package
probe initially found OpenBLAS 0.3.15 and LAPACK 3.8.0; those system versions
are not used for the pinned pilot. A coordinator note initially said LAPACK
3.12.1, but the installed prefix's pkg-config and CMake metadata identify it as
3.12.0; the pilot records the installed metadata version. Oracle Linux Server
8.10, x86_64, GCC 8.5.0, and CMake 3.26.5 were present at this audit.

The current audit has 21 grouped rows: 18 numerical operations and three
lifecycle entries, covering 23 declared public symbols. Each symbol was checked
against the current header, implementation, and existing test references; see
[`api_audit.md`](api_audit.md). Numerical operations are kept distinct from
vector, matrix, and LU lifecycle functions. Names and proposed mappings are in
[`linear_algebra_method_inventory.csv`](linear_algebra_method_inventory.csv).

## Pinned dependency and numerical pilot

[`openblas_lapack_probe.c`](openblas_lapack_probe.c) was compiled with:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -I/usr/include/openblas \
  benchmarks/phase14c/linear_algebra/openblas_lapack_probe.c \
  -lopenblas -o /tmp/phase14c-openblas-lapack-probe
OPENBLAS_NUM_THREADS=1 /tmp/phase14c-openblas-lapack-probe
```

Observed output:

```text
OpenBLAS 0.3.15 DYNAMIC_ARCH NO_AFFINITY Prescott SINGLE_THREADED
OpenBLAS threads=1
CBLAS DGEMM row-major=PASS
LAPACK DGETRF/DGETRS column-major=PASS x=(1.0,2.0)
```

This first probe used system libraries and is only an environment discovery
record. The pinned methodology run is reproducible with:

```sh
python3 benchmarks/phase14c/linear_algebra/run_pilot.py
```

The runner uses OpenBLAS 0.3.34 and reference LAPACK 3.12.0, builds NUMERIX
in an isolated module-local build directory, freezes a four-case manifest,
and writes 31 timing batches per route, accuracy/status rows, separate saved
factorization setup times, and memory rows marked unavailable. The current
pilot routes cover matrix addition, matrix multiplication, LU factorization,
and LU solve. The LU factorization path explicitly converts row-major inputs
to LAPACK's column-major representation within the end-to-end measured call;
the adapter accuracy record verifies the resulting `P*A=L*U` identity. LU
solve is measured with each library's own saved factors; setup is timed and
reported separately. The pilot remains far short of the 100 distinct workload
target and does not establish memory visibility or general speed claims.

The canonical accepted run is
[`phase14c-la-pilot-20261112T055827Z`](pilot_runs/phase14c-la-pilot-20261112T055827Z/).
Its `verification.json` passes: four manifest problems, eight routes, 31
batches per route (248 timing records), eight of eight accuracy records, two of
two setup records, and a 21-row/23-symbol public API inventory check. The
`environment.json` confirms OpenBLAS 0.3.34 with one runtime thread and the
selected LAPACK metadata version 3.12.0. Its four workloads are deterministic
and checksummed in the manifest. Earlier pilot directories are retained as
iteration history and superseded by this run, which includes the audited
`numerix_lu_destroy` lifecycle entry. Do not treat the pilot as a full benchmark
or infer a winner.

## Fair operation mapping

- Contiguous vector and matrix arithmetic maps to CBLAS copy/AXPY/scale,
  dot/norm, GEMV and GEMM. CBLAS `DOMATCOPY` is an OpenBLAS extension for
  transpose and must be labeled best available because it is not standard BLAS.
- NUMERIX accepts/returns row-major `double` matrices. CBLAS row-major entry
  points avoid a layout conversion for GEMV/GEMM. LAPACK DGETRF/DGETRS use
  column-major factors, so adapters must use an explicit row-major-to-column-
  major copy. Report both the factor/solve kernel time and end-to-end public
  operation time including conversion and required output allocation. Never
  pass a row-major matrix directly to LAPACK as if it were column-major.
- `numerix_vector_dot` uses compensated summation while CBLAS DDOT may use a
  different accumulation order. Compare achieved accuracy and classify this
  as best available unless pilot evidence establishes an equivalent procedure.
- NUMERIX LU uses scaled partial pivoting and allocates factors, a permutation,
  and a temporary row-scale array. LAPACK DGETRF uses partial pivoting and
  returns 1-based integer pivots. Treat factorization and Gaussian solve as
  best-available algorithm comparisons, verify the permutation convention,
  and compare backward/forward error as well as timing.
- `numerix_lu_solve` accepts NUMERIX's saved factors. LAPACK DGETRS must use
  factors produced by DGETRF, with factorization outside the solve-only timed
  region. A separate factorize-plus-solve run is needed for end-to-end solve.
- LAPACK has no Gauss-Jordan routine. Compare `numerix_linear_solve_gauss_jordan`
  with DGETRF+DGETRS/DGESV as best available, and state that the algorithms
  differ. Do not call it a direct Gauss-Jordan comparison.

## Workload freeze proposal

Before the full run, freeze deterministic problem IDs and inputs. Use separate
families so each operation is meaningfully exercised:

- Vector lengths across cache regimes (for example 8 through 1,048,576), with
  deterministic signed, cancellation-heavy, mixed-magnitude, all-zero, and
  near-range-limit finite data. Pairwise vector operands share identical
  stored doubles.
- Rectangular matrix and vector shapes for copy, add/subtract, scale,
  transpose, and GEMV; square and rectangular compatible dimensions for GEMM.
- Square solve matrices with diagonal-dominant, pivot-required, banded,
  mixed-scale, controlled-condition, and near-singular families. Generate a
  known solution `x_ref`, then form `b = A*x_ref` using an independent higher
  precision path before rounding the inputs to binary64. Record cases where a
  reference status is not applicable rather than dropping them.
- At least 20 distinct generator families and five independently seeded,
  frozen instances per family give the 100-problem target. Sizes are explicit
  workload dimensions, not counted as independent timing replicates. Reuse
  the same frozen data for each applicable implementation.

Use the published shared contract before generating these cases. Each row needs
a stable problem ID, operation applicability,
family, shape, seed, exact binary64 input checksum, reference method, and
accuracy target. Matrix-size caps must be piloted first: NUMERIX's naive GEMM
and Gauss-Jordan implementations become expensive as dimensions grow.

## Proposed result protocol

- Time each route/problem in calibrated batches with a monotonic wall clock and
  thread CPU clock. Warm both implementations, exclude warmups, counterbalance
  execution order with a fixed saved seed, and retain all 31 batch values per
  route/problem. Summarize each problem first; use problem as the statistical
  unit. Report kernel-only and end-to-end timings separately where layout
  conversion or output allocation differs.
- The direct NUMERIX APIs allocate result objects internally. For a symmetric
  end-to-end memory record, competitor wrappers should allocate their result
  buffers within the measured call boundary too. Preallocated input arrays and
  harness buffers stay outside that boundary. Preserve peak live requested
  bytes, total requested bytes, and allocation count separately. A process-wide
  interposer must be validated against OpenBLAS/LAPACK and warmed before each
  tracked call; if library-internal allocation is not intercepted, mark that
  route unavailable/incomplete rather than zero.
- Preserve factors/setup outputs for factorization-only or repeated-RHS
  workloads as the public operation's retained result. For solve-only, measure
  solution output allocation symmetrically. The memory report must distinguish
  retained result bytes from temporary peak bytes and allocator overhead.
- Accuracy references must be independent of both tested libraries. Verify
  elementwise/vector outputs with scale-normalized errors, and solve results
  with relative forward error, residual norm and normwise backward error.
  Include failures in the raw table; record `not_applicable`, `failed`, and
  `reference_unavailable` distinctly from measured zero error or zero
  allocations.

## Remaining work

1. Expand adapters from the four pilot routes to the full audited numerical
   method inventory, preserving ownership symmetry, Fortran integer-width and
   layout checks, and an explicit policy for lifecycle-only APIs.
2. Pilot representative sizes and all method families; verify independent
   references, dispatch, output checks, and allocator interception.
3. Freeze at least 100 distinct problem definitions, run 31 counterbalanced
   timing batches per applicable problem, collect raw allocation events, and
   independently verify summaries before any figures are made.

This pilot contains timing measurements for four method pairs. It reports no
overall winner or general performance conclusion; all memory measurements are
unavailable, and the full Phase 14C workload is outstanding.
