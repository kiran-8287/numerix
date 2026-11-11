# NUMERIX

NUMERIX is a C11 numerical computation library being built as a modular foundation for numerical algorithms, problem analysis, and execution diagnostics.

## Status

Phase 0 provides version/status/error APIs, scalar callback problems, shared options, a result foundation, allocation helpers, and an execution context. Phase 1 adds bracketed and open root solvers, dense vector/matrix operations and direct solvers, finite-difference derivatives, polynomial interpolation, fixed/adaptive quadrature, one-dimensional and gradient-based minimization, and fixed-step vector ODE solvers. Expression parsing, method recommendation, fallback, and advanced diagnostics remain planned.

Phase 2 adds a compiled mathematical expression engine for `x`, `pi`, `e`, arithmetic, powers, and common standard math functions. Compiled expressions can be adapted to existing scalar callback algorithms with a caller-owned callback context; see [the expression engine guide](documentation/api/expression_engine.md) and run `numerix_expression_demo` for examples.

The intended long-term vision is to support multiple methods per problem, assess numerical challenges, recommend methods, report diagnostics, and let callers control and inspect execution. These are roadmap goals, not current features.

## Platforms

The code targets Windows and POSIX-compatible Linux with C11 compilers. Linux can use `clock_gettime(CLOCK_MONOTONIC)` and Windows uses the performance counter API. Platform verification status is recorded in `documentation/development/phase0_report.md`.

## Build and test

With CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

With Make:

```sh
make
make test
make clean
```

The CMake static library target and Make archive are named `numerix` and `libnumerix.a` respectively. The CLI executable is `numerix_cli`; `numerix_expression_demo` demonstrates expression evaluation and callback integration. `make test` runs the Phase 0 checks, Phase 1 algorithm suite, and expression engine tests.

## Initialization example

```c
#include "numerix.h"
#include <stdio.h>

int main(void)
{
    numerix_error error;
    numerix_status status = numerix_initialize(&error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "%s\n", error.message);
        return 1;
    }
    printf("NUMERIX %s ready\n", NUMERIX_VERSION_STRING);
    return 0;
}
```

## Project layout

- `include/`: the single public API header.
- `src/core/`: shared implementation and platform timing.
- `src/internal/`: private implementation declarations.
- `src/algorithms/`: reserved subdirectories for future algorithm families.
- `src/expression/`, `piecewise/`, `intelligence/`, `execution/`, `diagnostics/`: future module boundaries.
- `cli/`: small startup application.
- `tests/`: automated unit and integration checks.
- `documentation/`: architecture and development notes.

## Roadmap

1. Phase 0: project foundation (complete).
2. Phase 1: core numerical algorithms (current).
3. Phase 2: expression parsing and evaluation (current).
4. Later phases: piecewise representations, execution controls and diagnostics, then method analysis and recommendation.

No later-phase feature should be inferred from the existence of a reserved directory or foundation type.


## Phase 3: piecewise functions

NUMERIX can compile ordered piecewise expression segments with open or closed bounds, evaluate the matching segment, estimate one-sided limits, and report numerical boundary classifications. Piecewise callback, boundary-split trapezoid integration, and a central-difference guard connect the feature to the existing numerical algorithms. Discontinuity detection is numerical evidence, not symbolic proof; see [the piecewise API guide](documentation/api/piecewise.md).


## Phase 4: algorithm recommendations

NUMERIX can explain deterministic recommendations across the implemented numerical categories, execute a recommended method, retain explicit algorithm overrides, and orchestrate root-solver fallback or sequential portfolios. See [the intelligence API guide](documentation/api/intelligence.md) and [architecture notes](documentation/architecture/intelligence.md).

## Phase 5: managed execution

The opt-in managed execution API supports root problems, progress summaries,
cooperative cancellation, monitored root fallback, and genuine stepwise
pause/resume for explicit Bisection. Other problem categories and portfolio
management are not yet supported; see [the execution API guide](documentation/api/execution_management.md)
for details.
