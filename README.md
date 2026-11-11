# NUMERIX

NUMERIX is a C11 numerical computation library being built as a modular foundation for numerical algorithms, problem analysis, and execution diagnostics.

## Status

Phase 0 provides version/status/error APIs, scalar callback problems, shared options, a result foundation, allocation helpers, an execution context with counters and monotonic elapsed timing, a static library, a startup CLI, and automated tests. No numerical solving algorithms or expression engine are implemented yet.

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

The CMake static library target and Make archive are named `numerix` and `libnumerix.a` respectively. The CLI executable is `numerix_cli`.

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

1. Phase 0: clean project foundation (current).
2. Later phases: expression and problem representations, numerical algorithm families, execution controls and diagnostics, then method analysis and recommendation.

No later-phase feature should be inferred from the existence of a reserved directory or foundation type.
