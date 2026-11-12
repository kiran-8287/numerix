# Phase 14 native adapters

The adapters are a standalone benchmark-only C build. They do not modify or add dependencies to NUMERIX's public library target. All library calls are made from `phase14_runner.c` through native C APIs.

## Configure and build

Pass local dependency prefixes as CMake cache variables. No machine-specific path is compiled into source:

```sh
cmake -S benchmarks/adapters/phase14 -B build/phase14-adapters \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DGSL_ROOT_DIR="$GSL_ROOT" \
  -DCMINPACK_ROOT="$CMINPACK_ROOT" \
  -DNLOPT_ROOT="$NLOPT_ROOT" \
  -DSUNDIALS_ROOT="$SUNDIALS_ROOT" \
  -DREFERENCE_LAPACK_ROOT="$REFERENCE_LAPACK_ROOT" \
  -DOPENBLAS_ROOT="$OPENBLAS_ROOT"
cmake --build build/phase14-adapters --parallel
```

Every `PHASE14_WITH_*` option is independently optional. Missing optional libraries do not prevent the NUMERIX-only adapter build. Reference LAPACK and OpenBLAS are built as separate runner binaries because both export the same LAPACKE symbols.

## Run one paired workload

```sh
build/phase14-adapters/phase14_runner \
  benchmarks/problems/phase14/problem_inputs.jsonl \
  P14-ROOT-01-01 NUMERIX 21 32 \
  benchmarks/results/phase14/main phase14-main
```

Arguments are: input JSONL, stable problem ID, library ID, number of independent timing batches, operations per batch, output directory, and run ID. Use `--memory` after the run ID to execute a single workload in an isolated process and append its peak-RSS observation. Run the specialist executables with their matching library IDs.

The runner makes one untimed accuracy/work probe, two warm-ups, then preserves each timing batch. Work counters are enabled only during the untimed probe. Both wall time (`CLOCK_MONOTONIC`) and process CPU time (`CLOCK_PROCESS_CPUTIME_ID`) are recorded. Timed calls include adapter workspace creation/allocation and solver execution; data-file parsing is outside the timed region. Requested heap interception is currently unavailable and is left blank in the memory schema.
