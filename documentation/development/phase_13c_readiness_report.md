# NUMERIX Phase 13C Pre-Benchmark Readiness Report

Date: 2026-11-12<br>
Branch and baseline commit: `master`, `6c2ef314bbe0b563dbf2353dd70a3b1ee2748574`<br>
Project version: `0.2.0-rc1`

## Repository and historical integrity

The Phase 13B baseline commit is present. The worktree was already dirty at the
start of Phase 13C, with Phase 13B source, documentation, tests, and result
artifacts modified or untracked. Those files were preserved; Phase 13C changes
are limited to the comparison harness, benchmark protocol documentation,
analysis artifacts, and new Phase 13C results. Phase 9 and Phase 10 checksums
match the values captured in the Phase 13B completion report. A SHA-256
inventory of the 35 existing Phase 13B result files was captured outside the
repository before Phase 13C runs.

The Phase 13B paired smoke run independently verifies as schema v2 with six
workload summaries, 18 timing batches, and 18 accuracy probes. Its files were
read only.

## Environment

| Item | Observed value |
| --- | --- |
| OS | Oracle Linux Server 8.10, Linux 5.15, x86_64 |
| CPU | Intel Core i5-7400, 4 online cores, 1 socket, no SMT |
| Compiler | GCC 8.5.0; Clang 21.1.8 available |
| CMake / CTest | Bundled CMake 4.4.4; CTest is available with it |
| Python | 3.9.25 |
| Analysis packages | Matplotlib 3.9.4, NumPy 1.26.4, SciPy 1.13.1 |
| Floating point | `double`: 64 storage bits, 53 significand bits, exponent bounds -1021 to 1024 |
| Timer | `CLOCK_MONOTONIC`; reported elapsed time is wall time, not CPU time |
| GSL | 2.8, clean local build with GCC `-O3 -DNDEBUG`, bundled CBLAS |
| Benchmark schema | Version 2 |

The optimized GSL prefix is `/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean`.
The earlier default-optimization GSL installation is separate. A clean Release
CMake build against the optimized prefix reports `GSL=2.8` and links both GSL
and GSL CBLAS. Numerix and GSL use GCC 8.5 with `-O3 -DNDEBUG`; no fast-math
flags are used. Host scheduling and CPU frequency are not controlled.

No system LAPACK/OpenBLAS, NLopt, SUNDIALS, Eigen, or Boost.Math installation
was detected. They are deferred because GSL covers the required APIs and adding
another library would require new adapters and validation without improving the
priority-one study enough to justify that effort.

## Study plan and pilot

The expanded plan contains 114 distinct mathematical problem configurations
across the seven requested categories:

| Category | Planned problem configurations | Coverage |
| --- | ---: | --- |
| Root finding | 16 | Bracketed, flat/multiple/tangent, scaled, tiny, large-coordinate, oscillatory, and expected no-bracket cases |
| Linear algebra | 30 | Dense LU at six sizes and three uniform scales; Hilbert stress systems; near-rank-one systems at three sizes and two diagonal gaps |
| Differentiation | 15 | Four analytic functions at multiple points, two output scales, and a near-zero square-root derivative |
| Interpolation | 12 | Linear segment; cubic and quartic polynomials; uniform/nonuniform nodes; 4–16 points; scaled values |
| Integration | 16 | Polynomial, trigonometric, exponential, endpoint singularity, oscillatory, Gaussian/narrow-peak, rational peak, tail, and scaled integrands |
| Optimization | 10 | Quadratic and exponential objectives; shallow/steep scales; wide and large-coordinate intervals; quartic and cusp minima |
| ODE | 15 | Exponential growth/decay at multiple rates and step counts; harmonic oscillator |

The requested target was approximately 90–125 configurations. Sixteen
additional NUMERIX-only capability probes are present in the harness but are
not counted in the seven-category study.

A three-sample pilot was run against the expanded matrix before the main study.
The schema verifier accepted 520 workload summaries, 1,560 raw timing batches,
and 1,560 linked accuracy probes. It exercised all seven categories with both
libraries where an API pairing exists. The pilot retained the planned stress
outcomes: no-bracket and tangent bracketing failures, an oscillatory Newton
nonconvergence, ill-conditioned Hilbert factorization/accuracy failures,
low-order quadrature accuracy misses, and ODE accuracy misses at deliberately
coarse steps or demanding rates. These are represented as failures or accuracy
misses, not successful results, and excluded from valid speed ratios.

The study uses direct same-algorithm comparisons for bisection, Newton, LU,
golden-section minimization, and fixed-step classical RK4. Brent safeguards,
adaptive differentiation/integration, and polynomial interpolation use
separate best-available or approximate classifications. Trapezoid/Simpson,
Euler/Heun, and NUMERIX orchestration/capability workloads remain NUMERIX-only
because GSL does not provide the same API or algorithm in this harness.

The C harness's ODE adapter calls GSL's low-level fixed-step `step_apply` once
per output interval. It does not use an adaptive driver or step doubling. The
protocol documentation was corrected before the main benchmark.

## Readiness decision

**READY FOR THE PRESPECIFIED MAIN RUN.** The optimized build links against GSL
2.8; the expanded all-category pilot verifies; all configurations have
analytic or known references; the execution order is counterbalanced; and
historical results remain unchanged. The main run uses 21 measured batches per
library/workload, seed `20261112`, and a new `benchmarks/results/phase13c/`
directory. Requested-heap comparison will use a separate static-GSL build with
linker allocation wrapping; process RSS is not treated as interchangeable with
that metric.
