# Phase 13B Baseline

Inspection captured before Phase 13B implementation edits.

## Repository

- Branch: `master`
- HEAD: `6c2ef314bbe0b563dbf2353dd70a3b1ee2748574` (`Prepare relocatable 0.2.0 release candidate distribution`)
- Initial working tree: one pre-existing untracked file, `documentation/development/phase_13a_benchmark_readiness_audit.md`; no tracked modifications. This Phase 13A audit is preserved.
- Release version: `0.2.0-rc1`.

## Build and test configuration

- C11, extensions disabled; CMake static library target `numerix` and Makefile static archive `libnumerix.a` use the modular `src/` implementation. The CMake and Make source lists match at inspection.
- CMake registers the library/unit, numerical category, integration, release regression, CLI, example, allocation-failure, accuracy, and portability-layout tests. The default Make `test` target runs the library test programs and release regressions; it does not replace the CMake CLI/example registrations.
- Available compilers: GCC 8.5.0 and Clang 21.1.8; GNU Make 4.2.1; Python 3.9.25; Valgrind is installed. CMake/CTest and GSL are not on `PATH` in this shell. Existing CMake build trees cache a bundled CMake executable at `/tmp/numerix-phase9-tools/python/cmake/data/bin/cmake`; its availability will be checked when running validation.

## Benchmark sources and historical outputs

- Main comparison harness: `benchmarks/comparison.c` / `benchmarks/comparison.h`.
- Other benchmark programs: `benchmarks/diagnostics_benchmark.c` and `benchmarks/root_discovery_benchmark.c`.
- Driver and analysis tools: `scripts/run_benchmarks.py`, `scripts/verify_benchmark_results.py`, and `scripts/summarize_benchmarks.py`.
- Historical results are under `benchmarks/results/phase9/` and `benchmarks/results/phase10/`; later validation artifacts are under `benchmarks/results/phase11/` and `benchmarks/results/phase12/`.
- The comparison executable writes CSV to standard output; `run_benchmarks.py` selects the destination file. New output can therefore be isolated under `benchmarks/results/phase13b/` without modifying historical files.
- Initial SHA-256 checksums for Phase 9 `allocations.csv`, `comparisons.csv`, `measurements.csv`, and `numerix_only.csv`, and Phase 10 `allocations.csv`, `comparisons.csv`, and `measurements.csv` were captured during inspection for final integrity comparison.

## Audit-to-source observations

The source matches the reported candidates: bisection computes `interval_converged` but still calls the callback before returning; LU and Gauss-Jordan use a global-magnitude threshold with a `DBL_MIN` floor; `deriv` lacks stencil-coordinate distinctness checks; linear interpolation is implemented in a compact direct-difference expression; ODE implementation is compact and checks finiteness without a documented stage-progress contract; differentiation's generic result has a residual field without an availability bit; and the comparison CSV exposes `precision_bits`, aggregates timing samples, and emits library rows in fixed NUMERIX-then-GSL order.

The historical result files were read only. Their initial checksums are retained in the Phase 13B completion report.
