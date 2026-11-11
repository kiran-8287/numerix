# Final benchmarking and release evaluation

Date: 2026-11-12 (Asia/Kolkata). Assessment: **release candidate with documented limitations**.

This is a bounded empirical evaluation of NUMERIX 0.1.0, not a universal ranking.
The initial tree was clean on master at
`29e2f7ac536e0d71f38a0e59ad148556b1b0c84f`; `git ls-remote` confirmed the same
origin/master commit and the reported Phase 8 commit message. The earlier
Windows results were verified as repository history/report claims, not rerun
on an unavailable Windows system. All seven numerical implementations, the
public header, core/internal infrastructure, supporting modules, tests, build
files, examples, CLI and Phase 8 report were inspected before changes.

The final suite records222 timed observations:46 root,88 linear algebra,
8 differentiation,24 integration,20 interpolation,8 minimization,12 ODE and
16 independent capability workloads. It retains11 nonzero statuses (including
expected invalid brackets and ill-conditioned rejections) and21 successful-call
accuracy misses. These are test observations, not production failure rates.
No major numerical method was added, removed or replaced.

## A. Benchmark environment

- Intel Core i5-7400 at nominal3.00GHz,4 logical CPUs,1 thread/core,4 cores.
- Oracle Linux Server8.10, x86_64; kernel5.15.0-326.222.4.el8uek.x86_64.
- Reported physical RAM16,353,718,272 bytes. Available RAM/load/frequency varied;
  raw environment data records the observed state. No exclusive CPU isolation.
- GCC 8.5.0 for NUMERIX and GSL measurements; Release `-O3 -DNDEBUG`.
  NUMERIX also uses C11 and `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.
- GSL 2.8 built from the official GNU tarball into a temporary local prefix,
  static archives, `--disable-shared`, reference GSL CBLAS; no vendor BLAS.
  GSL was initially absent. Passwordless system installation was unavailable,
  so the dependency was built locally. Package queries also encountered an
  unrelated repository signature issue; no system security settings were changed.
- CMake 4.4.4 installed locally; GNU Make4.2.1. Other validation used
  Clang 21.1.8, GCC 15.2.1 for `-fanalyzer`, Valgrind 3.22.0 and locally built
  Cppcheck 2.18.3. No MSVC or native Windows runtime was available.
- Binary64 arithmetic,53 mantissa bits; scalar storage64 bits. Timings are
  CLOCK_MONOTONIC wall time. CPU frequency, scheduling/cache effects and machine
  load are not controlled. Results are local observations.

[Environment JSON](../../benchmarks/results/phase9/environment.json) records
compiler/link flags, exact versions, affinity, OS, RAM, source/tarball hashes,
measurement UTC timestamp and the dirty source state based on the Phase 8 commit.
It intentionally describes the pre-commit measurement tree; source hashes
identify the actual benchmark implementation.

## B. Methodology

[Problem definitions and reproduction commands](../../benchmarks/README.md)
specify mathematical functions, analytical references, all inputs, difficulty
levels, stopping rules, accuracy checks, setup and measurement scope.
[Raw measurements](../../benchmarks/results/phase9/measurements.csv),
[comparison eligibility/ratios](../../benchmarks/results/phase9/comparisons.csv),
[allocation probes](../../benchmarks/results/phase9/allocations.csv) and
[complete generated tables](../../benchmarks/results/phase9/summary.md) are retained.

Every timed workload warms up and calibrates a powers-of-two batch to at least
2 ms, capped at 1,048,576 operations, then measures21 batches. Min, median, mean,
sample standard deviation and nearest-rank p95 describe **batch means per
operation**, not single-call tail latency. Accuracy is measured outside timed
batches, once per sample; callbacks are counted in both libraries. Root Newton
function and derivative counters are separate. Setup/input generation is outside
execution timing except inherent per-call API validation, copies, initialization
and allocation. LU factorization/solve/total and polynomial construction/evaluation
are separate. Memory probes are untimed, in a separate executable.

Same-family comparisons use matched mathematical inputs and tolerance formulae,
but retain implementation differences. GSL drivers use its public iterative
interfaces while NUMERIX incorporates stopping inside a single API call, so driver
and library overhead are part of these API measurements. Direct kernel-only
rankings are not inferred. Brent safeguards, adaptive differentiation/quadrature,
Lagrange representation and RK4 step-doubling pairs are explicitly unranked.
No multiple-root search is compared with a single-root solver.

## C. Root finding

Representative roots; all 46 rows and every bracket are in the raw/summary files.
Times below are medians in µs. Separate derivative counts and relative errors are in CSV.

| Case | Method | NUMERIX µs | GSL µs | GSL/N ratio | N abs error | G abs error | N/G f calls |
| --- | --- | --- | --- | --- | --- | --- | --- |
| polynomial | bisection | 0.300 | 0.636 | 2.12 | 2.22e-10 | 2.22e-10 | 35/34 |
| polynomial | brent | 0.380 | 0.200 | excluded | 7.68e-14 | 4.13e-14 | 18/8 |
| polynomial | newton | 0.097 | 0.110 | 1.13 | 1.59e-12 | 1.59e-12 | 5/5 |
| cosine | bisection | 0.922 | 0.998 | 1.08 | 8.87e-11 | 8.87e-11 | 35/34 |
| cosine | brent | 0.899 | 0.254 | excluded | 1.11e-16 | 4.77e-15 | 22/7 |
| cosine | newton | 0.185 | 0.200 | 1.08 | 0 | 0 | 5/5 |
| flat_cubic | bisection | 0.299 | 0.555 | 1.86 | 6.98e-11 | 6.98e-11 | 35/34 |
| flat_cubic | brent | 0.333 | 0.744 | excluded | 0.000413 | 0.000265 | 13/27 |
| flat_cubic | newton | 0.308 | 0.325 | excluded | 0.000338 | 0.000338 | 19/19 |
| small_scale | bisection | 0.298 | 0.657 | 2.21 | 2.22e-10 | 2.22e-10 | 35/34 |
| small_scale | brent | 0.090 | 0.083 | excluded | 0.414 | 0.414 | 3/3 |
| small_scale | newton | 0.050 | 0.058 | excluded | 0.414 | 0.414 | 1/1 |
| large_scale | bisection | 0.310 | 0.648 | 2.09 | 2.22e-10 | 2.22e-10 | 35/34 |
| large_scale | brent | 0.691 | 0.253 | excluded | 7.68e-14 | 2.22e-16 | 36/9 |
| large_scale | newton | 0.112 | 0.128 | 1.14 | 0 | 0 | 6/6 |
| multiple_bracket | bisection | 0.079 | 0.077 | 0.98 | 0 | 0 | 3/3 |
| multiple_bracket | brent | 0.425 | 0.216 | excluded | 2.22e-16 | 4.42e-12 | 20/8 |
| multiple_bracket | newton | 0.111 | 0.125 | 1.12 | 2.09e-14 | 2.09e-14 | 5/5 |
| near_endpoint | bisection | 0.326 | 0.692 | 2.12 | 4.65e-13 | 4.65e-13 | 42/41 |
| near_endpoint | brent | 0.547 | 0.089 | excluded | 2.07e-25 | 2.07e-25 | 30/3 |
| near_endpoint | newton | 0.061 | 0.071 | 1.18 | 2.72e-17 | 2.72e-17 | 2/2 |

Polynomial bisection returns position error 2.22e-10 and residual6.27e-10 in
both libraries under a2.41e-10 position tolerance. The derivative-based polynomial
case has error 1.59e-12 and residual4.51e-12. All valid bisection cases pass their
position check, including the endpoint case (error 4.65e-13).

Flat cubic residual stopping accepts root errors approximately3.38e-4 for Newton
and4.13e-4 for NUMERIX Brent. Scaling `x^2-2` by 1e-14 causes residual-based methods
to accept x=1, error 0.414, while bisection locates sqrt(2). These are accuracy
misses despite success status. GSL here is deliberately driven with matched
residual shortcuts; this is not a claim that GSL's usual interval-only stopping
has the same weakness. The public NUMERIX tolerance interface currently mixes
function and position units; callers must scale tolerances and inspect evidence.

Both libraries reject `x^2+1` on[-1,1] in all bracketing rows. GSL Brent uses fewer
callbacks on several smooth/endpoint problems; NUMERIX's10% interior guards can
force many bisections. Its algorithm was preserved rather than tuned for this
suite. Different Brent safeguards exclude direct speedup ranking.

## D. Linear algebra

Well-conditioned unit-scale dense partial-pivoted LU. Times are µs; total includes
necessary copies/allocations and freeing results, not input generation or residual calculation.

| n | N factor | N solve | N total | G factor | G solve | G total | G/N total | N relative x error | N residual inf | N backward error |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 8 | 0.485 | 0.270 | 0.666 | 0.810 | 0.262 | 1.013 | 1.52 | 4.44e-16 | 6.13e-16 | 1.26e-16 |
| 32 | 8.355 | 1.328 | 9.667 | 10.162 | 1.311 | 11.484 | 1.19 | 8.88e-16 | 1.61e-15 | 2.91e-16 |
| 64 | 47.515 | 4.528 | 51.729 | 55.828 | 4.628 | 60.361 | 1.17 | 1.55e-15 | 3.4e-15 | 5.79e-16 |
| 128 | 317.241 | 17.199 | 329.554 | 357.800 | 17.977 | 374.394 | 1.14 | 2e-15 | 3.4e-15 | 5.47e-16 |

Scales1e-12 and1e12 use the same matrix shape and known solution; their complete
factor/solve/total results are retained. Well-conditioned solution errors stay
near floating-point rounding. Solve-only times are close enough that small ratios
need interpretation against spread; there is no universal dense-solve advantage.
The observed cubic factorization growth is consistent with dense LU work, not a
claim about optimized BLAS/LAPACK performance.

Hilbert stress results separate tiny backward error from potentially huge forward
error. Rounded b perturbs an already severely ill-conditioned problem:

| n | N status | N solution error | G status | G solution error | G residual inf | G backward error |
| --- | --- | --- | --- | --- | --- | --- |
| 8 | 0 | 4.19e-07 | 0 | 4.39e-07 | 2.51e-16 | 4.61e-17 |
| 32 | 7 | — | 0 | 347 | 3.05e-15 | 2.16e-18 |
| 64 | 7 | — | 0 | 563 | 3.34e-15 | 1.25e-18 |
| 128 | 7 | — | 0 | 1.27e+03 | 1.02e-14 | 1.48e-18 |

NUMERIX rejects negligible pivots for n>=32 (SINGULAR_MATRIX, status 7). GSL returns
success with large forward error. Neither outcome is ranked as faster or more accurate.
At n=8 both return inaccurate solutions relative to the strict1e-8 forward check.

## E. Differentiation

NUMERIX fixed-step Richardson versus GSL adaptive central differences at x=0.7,
initial h=1e-3. These are different algorithms and are unranked.

| Case | N µs | G µs | N abs error | G abs error | N/G f calls |
| --- | --- | --- | --- | --- | --- |
| quartic_derivative | 0.048 | 0.114 | 2.01e-13 | 1.97e-11 | 5/8 |
| sine_derivative | 0.090 | 0.184 | 1.64e-14 | 2.45e-13 | 5/8 |
| exp_derivative | 0.091 | 0.188 | 3.03e-13 | 5.98e-12 | 5/8 |
| rational_derivative | 0.054 | 0.125 | 1.59e-13 | 1.1e-12 | 5/8 |

All eight observations pass the derivative accuracy criterion. NUMERIX uses 5
evaluations and GSL 8 here; this does not establish a fixed-step advantage for
unseen scales or noisy functions. Estimated errors differ from actual errors.

## F. Integration

Adaptive Simpson versus adaptive Gauss-Kronrod QAG21: same integrals/references,
different rules and error control, so no direct speedup ranking.

| Case | N µs | G µs | N/G status | N actual error | N estimate | G actual error | G estimate | N/G f calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| quartic_integral | 2.711 | 0.189 | 0/0 | 2.78e-17 | 3.1e-11 | 2.78e-17 | 2.22e-15 | 257/21 |
| sine_integral | 7.923 | 0.309 | 0/0 | 5.55e-15 | 1.02e-10 | 0 | 2.22e-14 | 369/21 |
| exp_integral | 2.721 | 0.361 | 0/0 | 6.66e-16 | 3.56e-11 | 0 | 1.91e-14 | 129/21 |
| curved_integral | 6.710 | 1.365 | 0/0 | 1.38e-14 | 3.97e-11 | 2.78e-17 | 1.63e-15 | 573/147 |
| endpoint_sqrt | 0.833 | 6.123 | 6/0 | — | 0 | 3.56e-14 | 3.69e-11 | 67/777 |
| oscillatory | 93.189 | 2.797 | 0/0 | 6.59e-16 | 4.36e-11 | 4.86e-17 | 4.62e-13 | 3817/147 |

QAG21 needs substantially fewer evaluations for several smooth/oscillatory
problems. NUMERIX reaches its depth 32 limit on sqrt(x), returning convergence
failure and NAN rather than claiming a result; GSL succeeds. Faster failure is
not success. This is a clear limitation of the available adaptive Simpson method.

The fixed1024-panel trapezoid rows miss the1e-10-scale comparison check on all six
functions; they have no adaptive tolerance guarantee. Fixed Simpson also misses
it on sqrt(x) and sin(40x), despite success completing the requested grid.
Other fixed Simpson errors range from rounding scale to about1e-12. No directly
equivalent fixed composite API was used in GSL, so these remain NUMERIX-only.
Error estimates are estimates, not certified bounds; fixed rules expose NAN when
an estimate is unavailable. Full relative errors are retained in CSV.

## G. Interpolation

Eight-node cubic datasets, query0.37; setup and evaluation are separate.

| Case | Phase | N µs | G µs | G/N | N abs error | G abs error |
| --- | --- | --- | --- | --- | --- | --- |
| uniform_cubic | construct | 0.171 | 0.174 | 1.02 | — | — |
| uniform_cubic | evaluate | 0.086 | 0.051 | 0.59 | 1.11e-16 | 1.11e-16 |
| nonuniform_cubic | construct | 0.166 | 0.167 | 1.01 | — | — |
| nonuniform_cubic | evaluate | 0.083 | 0.047 | 0.57 | 1.11e-16 | 1.11e-16 |
| scaled_cubic | construct | 0.162 | 0.171 | 1.05 | — | — |
| scaled_cubic | evaluate | 0.083 | 0.048 | 0.57 | 4.77e-07 | 4.77e-07 |

NUMERIX Newton and GSL polynomial interpolation both use divided differences,
with distinct validation/storage APIs. GSL evaluates these small interpolants
faster in this run. Linear two-node evaluation is also faster in GSL. The
1e10-scaled polynomial has larger absolute error but essentially the same relative
accuracy. All interpolation accuracy checks pass. NUMERIX's direct Lagrange
implementation performs repeated O(n²) validation/products per query, while GSL
uses a preconstructed polynomial; those different representations are unranked.
Construction/destruction of the Newton object is measured independently, not
charged to every evaluation.

## H. Optimization

Golden-section minimization, matched1e-6 absolute interval tolerance, relative0.
GSL starts from an additional interior point; initialization paths differ.

| Case | N µs | G µs | G/N | N position error | G position error | N objective | G objective | N/G f calls |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| convex_minimum | 0.335 | 0.746 | 2.23 | 3.44e-08 | 1.76e-08 | 1.18e-15 | 3.08e-16 | 33/43 |
| nonlinear_minimum | 0.568 | 1.075 | 1.89 | 3e-08 | 4.4e-08 | 1 | 1 | 33/45 |
| shifted_minimum | 0.334 | 0.721 | 2.16 | 3.44e-08 | 1.76e-08 | 1.18e-15 | 3.08e-16 | 33/43 |
| scaled_minimum | 0.333 | 0.730 | 2.19 | 3.44e-08 | 1.76e-08 | 0.00118 | 0.000308 | 33/43 |

All eight minimization accuracy probes pass21/21. NUMERIX takes 33 objective
calls, versus 43 or45 here for GSL. These same-family API observations do not
extend to gradient-based optimization; no equivalent GSL line-search pair was forced.

## I. ODE solving

Same initial conditions, outer h=0.01,100 steps and complete trajectory storage.
GSL classical RK4 uses step doubling; NUMERIX advances one classical step. Different
work/error estimation means no direct ranking.

| Case | N µs | G µs | N final error | G final error | N trajectory error | G trajectory error | N/G RHS calls |
| --- | --- | --- | --- | --- | --- | --- | --- |
| exponential_growth | 3.591 | 9.291 | 2.25e-10 | 1.41e-11 | 2.25e-10 | 1.41e-11 | 400/1100 |
| exponential_decay | 3.472 | 8.946 | 3.67e-10 | 2.27e-11 | 4.99e-10 | 3.09e-11 | 400/1100 |
| harmonic | 3.829 | 10.509 | 6.97e-11 | 4.37e-12 | 6.97e-11 | 4.37e-12 | 400/1100 |

NUMERIX uses 400 RHS evaluations, GSL 1100. GSL is more accurate on these inputs
because of its additional work. Euler and Heun remain separate NUMERIX-only rows,
with 100 and200 evaluations respectively. Errors increase appropriately at lower
order; every ODE row passes its explicitly method-specific accuracy check.

## J. Accuracy and reliability

Absolute error is `abs(computed-reference)`. Relative error divides by the
nonzero reference magnitude; it is unavailable (NAN) at a zero reference, not
artificially divided by epsilon. Matrix errors are infinity norms, with a known
all-ones solution, long-double residual accumulation and normwise backward error.
ODE rows compare every returned time/state with the analytical trajectory.
Independent checks in `scripts/verify_benchmark_results.py` validate CSV integrity,
accuracy calculations, duplicate keys, expected failure classes and leak cleanup.

Deterministic valid cases repeat21 accuracy/status probes each. Root flatness and
scaling misses, Hilbert forward-error misses, fixed-grid tolerance misses and the
sqrt depth failure are explicitly retained. A successful return or a tiny residual
does not by itself prove requested forward accuracy. No random-input confidence
interval, completeness guarantee, or universal reliability rate is claimed.

## K. Runtime interpretation

The comparison table defines speedup as `T_GSL/T_NUMERIX`, greater than 1 meaning
NUMERIX was faster in that particular equivalent API workload. Differences near1
must be read alongside the raw sample spread. No speedup is emitted for failed
accuracy cases, Hilbert systems or different-work/algorithm pairs.

Measured strengths are low raw scalar-solver overhead, relatively small dense LU
setup costs and economical golden-section execution on these cases. Weaknesses
include small-polynomial interpolation evaluation, conservative Brent steps,
expensive adaptive Simpson work on oscillation/curvature, and sqrt endpoint
convergence. GSL's richer adaptive error control and its iterative API can have
very different work and overhead. No algorithm was changed for benchmark scores.

## L. Memory

The ELF-wrapped **separate untimed** executable measures successful allocation
counts, cumulative requested bytes, peak live requested heap bytes, persistent
setup workspace and zero live bytes after destruction. It includes linked static
GSL objects, nested NUMERIX solver/diagnostic/context allocations and the GSL ODE
trajectory adapter. Inputs, stack, allocator metadata, libc-internal allocations
and RSS are excluded. Timing runs do not use the allocation wrappers.

| Case | n | Phase | Library | Allocations | Cumulative B | Peak live B | Setup B |
| --- | --- | --- | --- | --- | --- | --- | --- |
| polynomial | 0 | execute | NUMERIX | 0 | 0 | 0 | 0 |
| polynomial | 0 | execute | GSL | 2 | 64 | 64 | 64 |
| polynomial | 0 | execute | NUMERIX | 0 | 0 | 0 | 0 |
| polynomial | 0 | execute | GSL | 2 | 48 | 48 | 48 |
| dense_well | 8 | total | NUMERIX | 4 | 656 | 656 | 0 |
| dense_well | 8 | total | GSL | 11 | 864 | 776 | 0 |
| dense_well | 32 | total | NUMERIX | 4 | 8720 | 8720 | 0 |
| dense_well | 32 | total | GSL | 11 | 9024 | 8840 | 0 |
| dense_well | 64 | total | NUMERIX | 4 | 33808 | 33808 | 0 |
| dense_well | 64 | total | GSL | 11 | 34240 | 33928 | 0 |
| dense_well | 128 | total | NUMERIX | 4 | 133136 | 133136 | 0 |
| dense_well | 128 | total | GSL | 11 | 133824 | 133256 | 0 |
| uniform_cubic | 8 | evaluate | NUMERIX | 2 | 128 | 128 | 128 |
| uniform_cubic | 8 | evaluate | GSL | 5 | 256 | 256 | 256 |
| exponential_growth | 100 | execute | NUMERIX | 7 | 1656 | 1656 | 0 |
| exponential_growth | 100 | execute | GSL | 9 | 1720 | 1720 | 104 |

All222 memory observations end with 0 tracked bytes live. Expression evaluation
has no per-evaluation allocation, but its probe includes compilation/setup:
8 successful allocations,648 cumulative bytes,640 peak bytes and296 retained
setup bytes. Managed execution uses 2 allocations and10,616 peak bytes in this
workload; automatic execution and basic root diagnostics allocate no heap here.

Phase 8's direct workspace metrics remain distinct and unchanged in scope:

| Search | Panels | Direct retained arrays B | Direct workspace peak B | All tracked allocations | All tracked peak B |
| --- | --- | --- | --- | --- | --- |
| discovery_empty | 16 | 0 | 544 | 3 | 904 |
| discovery_one | 16 | 30144 | 30688 | 4 | 31048 |
| discovery_empty | 32 | 0 | 1056 | 3 | 1416 |
| discovery_one | 32 | 30144 | 31200 | 4 | 31560 |
| discovery_empty | 64 | 0 | 2080 | 3 | 2440 |
| discovery_one | 64 | 30144 | 32224 | 4 | 32584 |

Empty discovery retains0 result-array bytes. Its direct16/32/64-panel peaks
544/1056/2080 B reproduce Phase 8's reported direct-buffer values; broader linked
allocation peaks are904/1416/2440 B. The difference is nested workspace, not a
regression in result reservations. One-root arrays retain30,144 B on this Linux
ABI; Phase 8's Windows report recorded29,760 B and the old eager reservation
1,910,784 B. Different platform layouts/builds prevent exact cross-platform byte
parity claims; no Phase 9 public struct fields were changed.

Expression compilation plus six discovery workloads inject a one-time failure
at every observed allocation index (12;3,4,5,9,5,6 respectively), including
geometric root-array growth and all-zero/pole cleanup. All probes clean up; these
do not exhaust every allocation path in the library. Valgrind also checks the
complete memory suite and these fault-injection cases with 0 errors/leaks.

Separate `/usr/bin/time -v` filtered dense runs measured process peak RSS2412 KiB
for the NUMERIX run and2628 KiB for the GSL run. Those figures include the harness,
code, allocator retention, all four sizes and all three LU phases; they are not
per-operation library allocations or interchangeable with the heap table.

## M. NUMERIX-specific capabilities

These workflows were not forced into a single-routine GSL comparison.

| Feature | Evidence | Scope/limitation |
| --- | --- | --- |
| Expression parsing/evaluation | 3138 expression checks; timed compiled evaluation; compile fault injection | Repeated immutable AST evaluation; no GSL parser ranking |
| Piecewise/boundary handling | Piecewise tests, CLI demo, Phase 8 integration | Numerical one-sided evidence, gaps and boundaries; not symbolic proof |
| Recommendation/fallback | Intelligence suite and example; timed automatic root execution | Deterministic heuristics, explicit overrides and retained fallback history |
| Execution management | Execution suite; timed managed call; pause/resume example | Cooperative cancellation; stepwise execution only for explicit Bisection |
| Diagnostics/sensitivity | Diagnostics suite and internal benchmark | Residuals, conditioning/error evidence and sensitivity; estimates not guarantees |
| Root discovery/multiple roots | Discovery suite, twelve timed grid/problem cases, memory/fault probes | Heuristic counts1/0/3/1 on selected inputs; no completeness or exact multiplicity proof |
| Error handling/API usability | Release regression suite, CLI invalid/domain inputs, five examples, C/C++ consumer builds | Focused code/usage review, not a user study or exhaustive malformed-struct fuzzer |

## N. Limitations

- Only one Linux CPU/OS configuration was measured; frequency/scheduler/cache
  effects remain. Small runtime differences require repetition on other machines.
- GSL uses reference CBLAS, not optimized BLAS. No other external library was
  ranked, because a separate equivalent, consistently built comparison was not
  established. This does not predict LAPACK/vendor-library results.
- No equivalent fixed trapezoid/Simpson or explicit Euler/Heun GSL pair was used;
  adaptive differentiation/quadrature and RK4 work differ. Initialization/API
  overhead cannot be completely separated from arithmetic in public calls.
- Root residual and position tolerances have different units; small scales,
  flat roots, conservative Brent guards and numerical singularity thresholds
  remain. Discovery/multiplicity/discontinuity evidence remains heuristic.
- Memory tracking requires static GSL and an ELF-compatible wrapper linker. It
  does not capture every process allocation or prove absence of all leaks.
- Windows/MinGW and MSVC were unavailable for native Phase 9 validation. The
  earlier Phase 8 Windows results remain historical evidence only.
- The broad test-harness Cppcheck scan retains three possible dereferences after
  failed CHECK macros and two intentional pointer-sentinel portability warnings;
  production-source scans are clean. There is no exhaustive fuzzing or proof.
- The0.1.0 ABI is not guaranteed stable. Phase 8 expanded public structs; clients
  must rebuild. Installed CMake package discovery/export, a custom allocator,
  generalized stepwise execution and uniform metric initialization remain absent.

## O. Overall evaluation and release assessment

**Release candidate with documented limitations.** The library preserves its
seven numerical families and supporting workflows, has repeatable comparison
and memory evidence, and passes the available native build, regression and
memory-safety checks. GSL remains the stronger reference for adaptive quadrature
and richer error-controlled computation; NUMERIX offers integrated representations,
recommendation, execution and diagnostics with modest overhead on these examples.
Neither library wins every workload, and this finite suite cannot establish that
NUMERIX is universally faster, more accurate or more reliable.

A limited0.1.0 source/static-library candidate is reviewable and packageable.
A stronger stable release claim needs native Windows/MSVC reruns, broader
hardware/problem testing, and continued API/accuracy-contract work. No release
tag or public binary upload is implied by this assessment.

## P. Portability and verification evidence

See [the release checklist](final_release_checklist.md),
[command/status records](../../benchmarks/results/phase9/validation/results.json)
and [static-analysis triage](../../benchmarks/results/phase9/validation/static_analysis_review.md).

Passed: clean Make Debug and Release builds/full tests; clean GCC and Clang
CMake Debug/Release builds with 31 CTests each; combined ASan/UBSan/leak checks
with 32 CTests including allocation faults; opt-in GSL build with 32 CTests;
Valgrind on all 11 unit/integration executables plus allocation faults and the
complete allocation suite; GCC 15 and Clang source analysis; exhaustive Cppcheck
production scan; no-GSL build/full NUMERIX benchmark; GSL benchmark; both existing
internal benchmark suites; CLI/examples; install and independent C/C++ consumers;
CPack TGZ generation. Windows/MSVC remain unavailable. Supplemental test-harness
Cppcheck analysis is partially passed, not silently treated as clean.

## Q. Correctness fixes and reproducibility

The baseline NULL LU matrix call crashed with SIGSEGV. It now validates before
reading dimensions. LU solve clears output handles and rejects out-of-range
permutations. Optional error outputs now preserve the actual returned status;
known-status helpers make this explicit to strict compilers/analyzers. Failure
root/boundary evidence is initialized, the vector norm uses an explicit nonzero
comparison and an overwritten optimizer assignment was removed. The numerical
recurrences and public layouts were preserved. No performance optimization or
cache was introduced; therefore no fabricated before/after speedup is claimed.

The sources, analytical case definitions, CSV/verifier/summary scripts, local-tool
instructions, exact environment metadata and measured results are all in the
repository. A fresh run writes its own environment; it must not reuse these numbers.
For GSL method details see the official
[differentiation](https://www.gnu.org/software/gsl/doc/html/diff.html) and
[ODE](https://www.gnu.org/software/gsl/doc/html/ode-initval.html) documentation,
and the locally built GSL 2.8 source (`roots/`, `interpolation/`, `integration/`).
