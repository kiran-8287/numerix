# Phase 14 workload and measurement preregistration

## Workload freeze

`benchmarks/problems/phase14/workloads.csv` and the SHA-256-linked `problem_inputs.jsonl` are the frozen problem inventory. The generator seed is `20261112`. Each module contains 20 named mathematical families and five parameterized instances per family, for 100 distinct problems per module and 700 total. Repeated timing batches, methods, and libraries do not increase the distinct problem count.

The manifest includes the formula/structure, parameters, expected behavior, reference-generation method, absolute and relative tolerances, difficulty, comparison mode, and eligible libraries. Dense matrices, interpolation nodes, and independent validation grids are stored in the JSONL sidecar; their exact contents are bound to the CSV rows with SHA-256 digests.

The first all-library pilot found that the harmonic-oscillator closed form had omitted the frequency factors for the initial velocity. The reference was corrected in the generator before the primary run; problem IDs and input payloads remain unchanged. The corrected reference is (y(t)=y_0\cos(\omega t)+(y'_0/\omega)\sin(\omega t)), with derivative (-\omega y_0\sin(\omega t)+y'_0\cos(\omega t)). The pilot artifacts preserve the earlier reference through their recorded input-manifest digest and remain separate from the primary results.

## Reference policy

- Root values are closed-form roots; no solver output is used as reference.
- Linear systems are constructed from known `x_reference` values and saved `A`/`b`; residuals are recomputed independently. Singular matrices are retained as reliability cases and excluded from successful-solve timing summaries.
- Differentiation uses 70-digit arbitrary-precision differentiation of the analytic family function, checked at 100 digits.
- Interpolation references are direct function values on a saved, independent 101-point validation grid, never the interpolation nodes.
- Integration references use 70-decimal mpmath quadrature, split at a known kink, peak, or singular endpoint as appropriate.
- Optimization references use constructed global minimizers; the minimum is checked against the objective's bounds and stationary-point structure. Multimodal nonnegative ripples retain a known global minimizer at the center.
- ODE references are analytic where available. Nonlinear systems use independent SciPy DOP853 references with tight tolerances and a halved maximum step; the endpoint change is saved in the reference method field.

## Measurement policy

- Primary comparisons use double precision, C callbacks, one thread, and no `-ffast-math`.
- Native C APIs are called from standalone C adapters. Python prepares reference/input files and analyzes saved measurements; it does not wrap the timed library calls.
- Timed function calls are separate from untimed accuracy and work-count probes. Timing uses `CLOCK_MONOTONIC` and `CLOCK_PROCESS_CPUTIME_ID`; each run preserves individual batches and batch size.
- Mode A and Mode B comparisons remain separate in the method map and analysis tables. In particular, fixed-step RK4 and adaptive ODE comparisons are never pooled.
- OpenBLAS uses its Haswell AVX2 target and is limited to one runtime thread. Reference LAPACK is built with Netlib reference BLAS.
- Requested heap and process RSS are distinct fields. Missing measurements are blank/`unavailable`, never encoded as zero.

## Important eligibility limits

- CMINPACK is a local nonlinear-system solver for scalar roots represented as a one-variable system; it does not provide bracket guarantees.
- NUMERIX exposes one-dimensional bounded optimization and vector gradient descent, so the NLopt benchmark is restricted to dimension one.
- NUMERIX quadrature is finite-interval and one-dimensional; no infinite-domain or multidimensional claim is made.
- The current NUMERIX interpolation API provides linear, Lagrange, and Newton polynomial methods. Higher-order GSL spline methods are not counted as equivalent functionality.
- Nonlinear ODE reference families and singular/ill-conditioned matrix cases remain clearly marked so their reliability observations do not inflate ordinary runtime conclusions.

## Reproduction entry points

1. Run `python3 benchmarks/problems/phase14/prepare_workloads.py` to regenerate the frozen problem data.
2. Run `python3 benchmarks/analysis/phase14/verify_workloads.py` to verify counts, uniqueness, references, and input digests.
3. Build the standalone adapters with the documented optional dependency roots in `benchmarks/adapters/phase14/README.md`.
4. Run pilot and main commands recorded in the Phase 14 execution log and run metadata.
5. Recompute tables and graphs from the saved raw CSV records using the Phase 14 analysis scripts.
