# NUMERIX

NUMERIX is a C11 numerical computation library with one public header,
`include/numerix.h`, and no external numerical-library dependency.

**0.2.0-rc1 — release candidate with documented limitations.** Phases10–12
harden convergence, validate available toolchains and prepare installation/source
packages. Native Windows/macOS jobs have been requested but remain unvalidated
until execution results exist. See the [final project report](https://github.com/kiran-8287/numerix/blob/master/documentation/development/final_project_report.md)
and [release checklist](https://github.com/kiran-8287/numerix/blob/master/documentation/development/final_release_checklist.md).

## Capabilities

Scalar root solvers; dense LU/Gaussian/Gauss-Jordan linear algebra; finite
differences; linear/polynomial interpolation; fixed/adaptive integration; bounded
and gradient minimization; fixed-step vector ODEs; compiled expressions; piecewise
functions/boundaries; recommendation/fallback; managed execution/cancellation;
diagnostics/sensitivity; and heuristic multiple-root discovery.

Root tolerances now describe position accuracy, independently of residual units.
Global adaptive Simpson resolves the tested sqrt endpoint problem without raising
its depth limit. Open-root convergence, quadrature estimates and root discovery
remain heuristic. Read [tolerance semantics](documentation/api/tolerances.md)
before treating a successful return as an accuracy guarantee.

## Requirements and validated platforms

Use a C11 compiler and CMake 3.16+ or GNU Make. CMake installation is recommended;
GNU Make installation needs POSIX/MSYS2 install utilities. The static library
links the platform math runtime. GSL is optional for comparisons; Python 3.9+ is
optional for benchmark, validation and source-packaging scripts.

Recorded Linux x86_64 validation covers GCC 8.5/11.5/15.2 and Clang 21.1 in Debug
and Release, ASan/UBSan/leak detection, Valgrind and static analysis. New native
Windows MinGW/MSVC and macOS results are pending; Phase 8's older MinGW6.3 report
is historical. MSVC needs its checked C11 atomics capability. This is not a
promise of portability to every C11 environment. Only static libraries are
currently supported; no stable ABI is promised.

## Build and test

```sh
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --parallel 2
cmake -E chdir cmake-build-release ctest --output-on-failure
cmake --install cmake-build-release --prefix /path/to/numerix
```

For Debug, use a separate build directory and `-DCMAKE_BUILD_TYPE=Debug`.
Multi-configuration generators use `--config Debug/Release` and CTest `-C`.
Alternatively run `make`, `make test`, and `make install PREFIX=/path`.
Run `make clean` before changing Make compiler or flags.

## Quick start

```c
#include <numerix.h>
#include <stdio.h>

static double equation(double position, void *data)
{
    (void)data;
    return position * position - 2.0;
}

int main(void)
{
    numerix_result result;
    numerix_error error;

    if (numerix_root_brent(equation, NULL, 0.0, 2.0, NULL, &result, &error) !=
        NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "%s\n", error.message);
        return 1;
    }
    printf("root = %.12g; residual = %.3e\n", result.value, result.residual);
    return 0;
}
```

Independent CMake integration:

```cmake
find_package(NUMERIX 0.2 CONFIG REQUIRED)
add_executable(my_program main.c)
target_link_libraries(my_program PRIVATE NUMERIX::numerix)
```

Set `CMAKE_PREFIX_PATH` to the installation prefix. The target propagates headers
and POSIX math linkage. Manual POSIX linking uses `cc -I/path/include main.c
/path/lib/libnumerix.a -lm`; CMake installs relocatable pkg-config metadata too.
See [installation](documentation/user_guide/installation.md),
[ownership](documentation/api/release_contract.md) and
[compatibility/versioning](documentation/user_guide/compatibility.md).

## CLI and examples

```sh
./numerix_cli --help
./numerix_cli --eval 'sin(pi/2)+x^2' 2
./numerix_cli --root 'x^2-2' 0 2
./numerix_cli --roots '(x-1)*(x-2)' 0 3
./numerix_cli --piecewise-demo
```

The five `numerix_*_demo` executables demonstrate expressions, piecewise functions,
intelligence, managed execution and discovery. CTest executes all of them. The
root CLI shows recommendation, managed execution summaries and diagnostics;
actual stepwise pause/resume is supported only for explicit Bisection.
Piecewise CLI input is a fixed demonstration rather than a general editor.

## Benchmarks and distribution

```sh
cmake -S . -B cmake-build-bench -DCMAKE_BUILD_TYPE=Release \
  -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON
cmake --build cmake-build-bench --parallel 2
python3 scripts/run_benchmarks.py --build-dir cmake-build-bench --output /tmp/numerix-results
```

[Benchmark definitions and instructions](benchmarks/README.md) explain setup,
statistics, analytical references, GSL 2.8 comparisons and allocation scope.
Different algorithms/work and failed accuracy cases are excluded from direct
rankings. The latest mathematical baseline is Phase 10; Phase 9 raw results remain
historical and unchanged. Existing internal benchmark targets are retained.

Create source archives with `python3 scripts/package_source.py --output dist`.
CPack creates a separate binary package for the build platform. Source archives
exclude local builds, IDE state, raw environment logs and development-only reports;
those reports/results remain available in the repository. No binaries for
unvalidated platforms are distributed.

## Layout and documentation

- [API guides](documentation/api/): algorithms, representations, execution and diagnostics.
- [User guides](documentation/user_guide/): installation, compatibility and versioning.
- [Architecture](documentation/architecture/): module and ownership design.
- [Development history/results](https://github.com/kiran-8287/numerix/tree/master/documentation/development): empirical reports and known limits.
- `src/algorithms/`: seven numerical categories.
- `src/core/`, `src/internal/`: common services and private contracts.
- Other `src/` modules: expressions, piecewise functions, intelligence, execution,
  diagnostics and root discovery.
- `tests/`, `examples/`, `benchmarks/`, `cmake/`, `scripts/`: validation and distribution support.

Important limitations include incomplete root discovery, heuristic open-method
estimates, finite-endpoint quadrature, ill-conditioned linear-system forward
error, fixed-step ODEs, and unavailable native platform evidence. Old precompiled
clients must be rebuilt with the matching header/archive. See [CHANGELOG](CHANGELOG.md).

NUMERIX is distributed under the [MIT license](LICENSE).
