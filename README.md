# NUMERIX

NUMERIX is a C11 numerical computation library with a single public header,
`include/numerix.h`, and no external numerical-library dependency.

It provides scalar root solvers, dense linear algebra, finite-difference
derivatives, polynomial interpolation, fixed/adaptive integration, bounded and
gradient minimization, and fixed-step vector ODE solvers. Supporting modules
compile mathematical expressions, handle piecewise functions and boundaries,
recommend algorithms, orchestrate root fallback, manage execution, and report
numerical diagnostics, sensitivity and heuristic multiple-root discovery.

Phases 0–8 are implemented. Phase 9 adds reproducible GSL comparisons, Linux
validation, release regression checks and release documentation. The current
version is **0.1.0, a release candidate with documented limitations**; see the
[final evaluation](documentation/development/final_benchmark_report.md) and
[release checklist](documentation/development/final_release_checklist.md).
Discovery is heuristic and does not prove that every root has been found.

## Build and test

CMake 3.16+ and a C11 compiler:

```sh
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug -j2
ctest --test-dir cmake-build-debug --output-on-failure
```

Or GNU Make:

```sh
make
make test
make clean
```

Make builds a static `libnumerix.a`, CLI and five examples. CMake uses the
`numerix` library target and registers the unit, integration, CLI and example
checks with CTest. Use separate build directories for Debug, Release and
sanitizers. Run `make clean` before changing Make compiler/optimization flags.

The code targets POSIX Linux and Windows. Phase 9 validates Linux GCC 8.5,
Clang 21.1, CMake, ASan/UBSan and Valgrind on the recorded machine. The Phase 8
report records earlier Windows MinGW6.3 checks; Windows/MSVC have not been
revalidated here. These are tested configurations, not a promise that every
C11 compiler/platform works.

## Use

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

Link with the archive and the platform math library (for example
`cc -Iinclude program.c libnumerix.a -lm`). CMake can install the archive,
header and documentation with `cmake --install cmake-build-debug --prefix /path`.
Read [ownership and compatibility](documentation/api/release_contract.md)
before reusing caller-owned result structures. Clients built against earlier
public-struct layouts must be rebuilt; ABI compatibility is not established.

## CLI and examples

```sh
./numerix_cli --help
./numerix_cli --eval 'sin(pi/2)+x^2' 2
./numerix_cli --root 'x^2-2' 0 2
./numerix_cli --roots '(x-1)*(x-2)' 0 3
./numerix_cli --piecewise-demo
./numerix_expression_demo
./numerix_piecewise_demo
./numerix_intelligence_demo
./numerix_managed_execution_demo
./numerix_root_discovery_demo
```

The root CLI demonstrates recommendation, managed execution and residual
diagnostics. The managed example demonstrates actual Bisection pause/resume;
other methods provide cooperative cancellation and summaries, without general
stepwise execution. CLI inputs accept finite numeric values and quoted
expressions; piecewise input is demonstrated by a fixed two-segment example.

## Benchmarks and validation

GSL is optional and used only in the opt-in comparison executable:

```sh
cmake -S . -B cmake-build-bench -DCMAKE_BUILD_TYPE=Release \
  -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON
cmake --build cmake-build-bench -j2
python3 scripts/run_benchmarks.py --build-dir cmake-build-bench \
  --output /tmp/numerix-results
```

See [benchmark instructions and mathematical definitions](benchmarks/README.md)
for GSL setup, fairness caveats, allocation probes, raw CSV output and validation
commands. Existing internal suites remain available through
`make benchmark-diagnostics` and `make benchmark-root-discovery`.

## Documentation and layout

- [Numerical APIs](documentation/api/): algorithms, expressions, piecewise,
  recommendations, execution, diagnostics and discovery.
- [Architecture](documentation/architecture/): data structures and module design.
- [Development reports](documentation/development/): validation evidence and limitations.
- `src/algorithms/`: seven implemented numerical categories.
- `src/core/`, `src/internal/`: shared context, timing and internal contracts.
- `src/expression/`, `src/piecewise/`, `src/intelligence/`, `src/execution/`,
  `src/diagnostics/`, `src/root_discovery/`: supporting functionality.
- `tests/`, `examples/`, `benchmarks/`: regression checks, demonstrations and evaluation.

NUMERIX is distributed under the [MIT license](LICENSE).
