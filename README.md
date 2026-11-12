# NUMERIX

NUMERIX is a C11 numerical computation library. Its public API is declared in
[`include/numerix.h`](include/numerix.h). The project currently installs a static
library and a CMake package; it does not promise a stable binary ABI or provide
a shared-library build.

**Version:** 0.2.0-rc1. The release candidate documents known numerical and
platform limitations. Root tolerances describe position evidence separately
from residual units. Adaptive integration, open-root convergence and multiple
root discovery still have method-specific limitations; a successful status is
not a general accuracy certificate. See the [tolerance guide](documentation/api/tolerances.md)
and [release contract](documentation/api/release_contract.md).

## Contents

1. [Project overview and features](#project-overview-and-features)
2. [Platforms and compilers](#platforms-and-compilers)
3. [Repository structure](#repository-structure)
4. [Requirements](#requirements)
5. [Windows prerequisites](#windows-prerequisites)
6. [MSYS2 UCRT64 installation](#msys2-ucrt64-installation)
7. [Clone and open the project](#clone-and-open-the-project)
8. [Windows MinGW Release build](#windows-mingw-release-build)
9. [Windows MinGW Debug build](#windows-mingw-debug-build)
10. [Windows MSVC setup and builds](#windows-msvc-setup-and-builds)
11. [Windows testing](#windows-testing)
12. [Windows demos](#windows-demos)
13. [Windows CLI](#windows-cli)
14. [Linux prerequisites](#linux-prerequisites)
15. [Linux GCC Release build](#linux-gcc-release-build)
16. [Linux GCC Debug build](#linux-gcc-debug-build)
17. [Linux Clang builds](#linux-clang-builds)
18. [Linux testing](#linux-testing)
19. [Linux demos](#linux-demos)
20. [Linux CLI](#linux-cli)
21. [Numerical modules](#numerical-modules)
22. [CTest suite](#ctest-suite)
23. [Benchmarks](#benchmarks)
24. [External C application integration](#external-c-application-integration)
25. [Installation and packaging](#installation-and-packaging)
26. [Troubleshooting](#troubleshooting)
27. [Contributing](#contributing)
28. [License](#license)
29. [Historical reports and technical documentation](#historical-reports-and-technical-documentation)

## Project overview and features

The library has no required third-party numerical dependency. It includes:

- Scalar root finding and dense vector, matrix and LU operations.
- Finite-difference derivatives, interpolation and finite-interval integration.
- Bounded and gradient-based optimization and fixed-step vector ODE methods.
- Compiled expressions, piecewise functions and boundary analysis.
- Algorithm recommendation, fallback portfolios, managed execution and cancellation.
- Diagnostics, sensitivity analysis and heuristic multiple-root discovery.

The supported API is in one public header. Examples, tests, benchmark tools,
package metadata and guides live beside the implementation. GSL is optional and
is only used by the comparison benchmark. Python is optional; some benchmark,
release-validation and source-packaging utilities use it.

## Platforms and compilers

| Platform/toolchain | Status in this work | Notes |
| --- | --- | --- |
| Linux x86_64, GCC 8.5.0 | **Tested** | Debug and Release, CMake 4.4.4, `-Werror`; install and C/C++ consumers tested. |
| Linux x86_64, Clang 21.1.8 | **Tested** | Debug and Release, CMake 4.4.4, `-Werror`; Debug also passed AddressSanitizer and UndefinedBehaviorSanitizer. |
| Windows 10/11, MSYS2 UCRT64 MinGW GCC | **Expected to work; not tested in this work** | Windows CI jobs and setup instructions are present. Native Windows results are still required. |
| Windows 10/11, MSVC | **Expected to work; not tested in this work** | Requires VS 2022 17.5+ C11 atomics support. Native Windows results are still required. |
| macOS, Clang | **Expected to work; not tested in this work** | A macOS Clang workflow matrix remains configured; no run was made for these edits. |
| Shared-library builds | **Not currently supported** | CMake intentionally rejects `BUILD_SHARED_LIBS=ON`. |

The matrix above describes this work's verification, not every historical run.
See the [portability report](documentation/development/phase11_portability_report.md)
for earlier evidence and the [cross-platform build report](documentation/development/cross_platform_build_report.md)
for the current changes and remaining checks.

## Repository structure

| Path | Contents |
| --- | --- |
| `include/numerix.h` | Public C API and version macros |
| `src/algorithms/` | Root finding, linear algebra, differentiation, interpolation, integration, optimization and ODE algorithms |
| `src/core/`, `src/internal/` | Shared services and private implementation contracts |
| `src/expression/`, `src/piecewise/` | Expression parser/evaluator and piecewise functions |
| `src/intelligence/`, `src/execution/` | Recommendation, method portfolios and managed execution |
| `src/diagnostics/`, `src/root_discovery/` | Numerical diagnostics and heuristic root discovery |
| `cli/` | `numerix_cli` command-line application |
| `examples/` | Five standalone demo programs |
| `tests/` | C tests, installed C/C++ consumer and benchmark verification |
| `benchmarks/` | Internal and optional comparative benchmark programs, data and protocol |
| `cmake/` | Installed CMake package and pkg-config templates |
| `scripts/` | Build, benchmark, validation and packaging helpers |
| `documentation/api/` | Module and API guides |
| `documentation/architecture/` | Internal design and ownership notes |
| `documentation/development/`, `documentation/reports/` | Phase history, benchmark results and engineering reports |

## Requirements

Required for a normal build:

- A C11 compiler: GCC, Clang or supported MSVC.
- CMake 3.16 or newer, or GNU Make with the platform's required shell utilities.
- A platform C runtime and math library. Linux and macOS CMake targets link `m`;
  Windows targets do not.

Optional tools:

- G++/Clang++ for building the independent installed C++ consumer.
- Python 3 for benchmark runners, optional benchmark verification tests and
  source packaging.
- GSL for comparisons in the optional comparative benchmark only.
- Valgrind, Clang static analysis or Cppcheck for additional checks when installed.

The `ctest --test-dir` convenience option requires CTest 3.20 or newer. With
CMake/CTest 3.16–3.19, run CTest from the build directory with
`cmake -E chdir <build-dir> ctest ...`; the supplied build scripts use this
compatible form.

## Windows prerequisites

Use 64-bit Windows 10 or 11. Install Git for Windows, Visual Studio Code and
MSYS2 from their official downloads. The commands below are for **PowerShell**
unless explicitly labeled as MSYS2 shell commands.

- [Git for Windows](https://git-scm.com/download/win)
- [Visual Studio Code for Windows](https://code.visualstudio.com/docs/setup/windows)
- [MSYS2 installer and documentation](https://www.msys2.org/)
- [CMake downloads](https://cmake.org/download/)
- [Visual Studio Build Tools 2022](https://visualstudio.microsoft.com/downloads/)

Install CMake separately or install its MSYS2 UCRT64 package. Reopen PowerShell
after changing PATH. Confirm that `cmake`, `gcc`, `g++` and `mingw32-make` resolve
to the intended tools before configuring a build. Do not combine toolchains in
one build directory.

## MSYS2 UCRT64 installation

Launch **MSYS2 UCRT64** from the Start menu. Do not install or build this project
from the plain MSYS shell: that uses a different runtime and toolchain. UCRT64
provides the x86_64 GCC toolchain against the Windows Universal C Runtime.

These package commands are for the **MSYS2 UCRT64 shell**:

```bash
pacman -Syu
# If MSYS2 asks to close the terminal, close it, reopen UCRT64, then run:
pacman -Su
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-make \
  mingw-w64-ucrt-x86_64-cmake
```

The project does not require Python for its normal build. Install Python only if
you intend to use the Python benchmark, release-validation or packaging scripts.

If using **PowerShell**, add the MSYS2 UCRT64 binary directory for the current
session. Change `C:\msys64` if MSYS2 was installed elsewhere:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
gcc --version
g++ --version
cmake --version
mingw32-make --version
git --version
```

The MinGW workflow uses the `MinGW Makefiles` CMake generator, which expects
`mingw32-make`. The CI workflow separately uses Ninja in MSYS2 UCRT64; Ninja is
not required by the PowerShell script or the commands below.

## Clone and open the project

These commands are for **PowerShell**:

```powershell
git clone https://github.com/kiran-8287/numerix.git
cd numerix
code .
```

`code .` opens the repository folder in VS Code. If PowerShell cannot find
`code`, restart it after installing VS Code or use **File → Open Folder**.

## Windows MinGW Release build

These commands are for **PowerShell** from the repository root. Release and
Debug use different build folders.

```powershell
cmake -S . -B build/windows-release `
  -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_C_COMPILER=gcc

cmake --build build/windows-release --parallel
ctest --test-dir build/windows-release --output-on-failure
```

`cmake -S . -B ...` configures the source into the selected build folder.
`-G` selects MinGW Makefiles, and `-DCMAKE_C_COMPILER` pins GCC. The build
command compiles the library, CLI, demos and tests. CTest runs registered tests
and returns a failing exit code if any test fails.

The project is C-only. CMake does not need a C++ compiler for the main build;
the installed-consumer check separately configures C and C++ programs.

The provided **PowerShell** helper performs configuration, build, CTest,
installation and the installed C/C++ consumer check:

```powershell
.\scripts\build-windows.ps1 -Compiler MinGW -Configuration Release
```

Add `-RunExamples` to execute the CLI and all five demos after the checks.

## Windows MinGW Debug build

These commands are for **PowerShell**. Keep the Debug and Release configurations
separate so compiler flags and generated files do not mix.

```powershell
cmake -S . -B build/windows-debug `
  -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=gcc

cmake --build build/windows-debug --parallel
ctest --test-dir build/windows-debug --output-on-failure
```

Or use the **PowerShell** helper:

```powershell
.\scripts\build-windows.ps1 -Compiler MinGW -Configuration Debug -RunExamples
```

## Windows MSVC setup and builds

Install Visual Studio 2022 Build Tools and select the **Desktop development
with C++** workload. Include the MSVC C++ build tools and a Windows SDK. Open an
x64 **Developer PowerShell for VS 2022** so `cl.exe` and the matching build
environment are on PATH. Microsoft documents the [C++ workload](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022)
and [Developer PowerShell](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022).

NUMERIX checks for MSVC C11 atomics and currently requires Visual Studio 2022
17.5 or newer with `/experimental:c11atomics`. This is a project requirement;
the workflow cannot be considered verified until a native MSVC build passes.
The Visual Studio 2022 CMake generator requires CMake 3.21 or newer.

These commands are for **Developer PowerShell**. Visual Studio is a
multi-configuration generator, so the same build directory holds Debug and
Release; specify the configuration at build and test time:

```powershell
cmake -S . -B build/windows-msvc `
  -G "Visual Studio 17 2022" -A x64

cmake --build build/windows-msvc --config Release --parallel
ctest --test-dir build/windows-msvc -C Release --output-on-failure

cmake --build build/windows-msvc --config Debug --parallel
ctest --test-dir build/windows-msvc -C Debug --output-on-failure
```

The **PowerShell** helper accepts either configuration and uses the Visual
Studio generator without MinGW compiler paths:

```powershell
.\scripts\build-windows.ps1 -Compiler MSVC -Configuration Release -RunExamples
```

The helper verifies that `cl.exe` is available before configuring, installs the
library into a build-local staging prefix, then builds and tests the installed
C and C++ consumers.

## Windows testing

Run these from **PowerShell** after building the matching configuration:

```powershell
ctest --test-dir build/windows-release -N
ctest --test-dir build/windows-release --output-on-failure
ctest --test-dir build/windows-release -R numerix_root_finding --output-on-failure
```

Use `build/windows-debug` for MinGW Debug, or add `-C Debug`/`-C Release` for
the MSVC build directory. CTest's current default Linux configuration registers
35 tests. Optional benchmark settings can add tests; the number is determined
by the configured build, not hard-coded in the workflow.

## Windows demos

All demo source files are in `examples/`; CMake creates these five executables.
Commands are for **PowerShell** and use the MinGW Release directory; replace the
folder with `build/windows-debug` or `build/windows-msvc\Release` as needed.

```powershell
.\build\windows-release\numerix_expression_demo.exe
.\build\windows-release\numerix_piecewise_demo.exe
.\build\windows-release\numerix_intelligence_demo.exe
.\build\windows-release\numerix_managed_execution_demo.exe
.\build\windows-release\numerix_root_discovery_demo.exe
```

The expression demo evaluates expressions and demonstrates root solving,
integration and differentiation. Key results are a root near `2`, the integral
of `sin(x)` near `2` and the derivative of `x^3` at `2` near `12`. The piecewise
demo evaluates `f(-2)=4`, `f(0)=0`, an integral near `6.6668` and derivative at
`1` near `2`. The intelligence demo solves `x^2-2` near `sqrt(2)` and shows a
fallback root near `0.206299474198`. The managed-execution demo pauses and
resumes bisection and ends near `sqrt(2)`. Root discovery reports roots near
`1`, `2` and `4`, a repeated root near `1`, then piecewise and cancellation
cases; unresolved intervals are reported and completeness is not guaranteed.

## Windows CLI

The supported options in `cli/main.c` are `--help`, `--demo`, `--eval EXPR X`,
`--root EXPR LOWER UPPER`, `--roots EXPR LOWER UPPER` and `--piecewise-demo`.
PowerShell single quotes preserve expression text, including operators and
parentheses:

```powershell
.\build\windows-release\numerix_cli.exe --help
.\build\windows-release\numerix_cli.exe --demo
.\build\windows-release\numerix_cli.exe --eval 'sin(pi/2)+x^2' 2
.\build\windows-release\numerix_cli.exe --root 'x^2-2' 0 2
.\build\windows-release\numerix_cli.exe --roots '(x-1)*(x-2)' 0 3
.\build\windows-release\numerix_cli.exe --piecewise-demo
```

The expression command prints `f(2) = 5`. `--root` reports recommendation,
managed execution and diagnostics. `--roots` is heuristic: it can report
unresolved intervals and cannot prove that it found every root. The piecewise
option is a fixed demonstration, not a general interactive editor.

## Linux prerequisites

Commands in this section are for **Bash**. The library has no required external
numerical package. Install the compiler and CMake with your distribution's
package manager.

Ubuntu or Debian:

```bash
sudo apt update
sudo apt install build-essential clang cmake ninja-build git pkg-config
```

Fedora:

```bash
sudo dnf install gcc gcc-c++ clang cmake make ninja-build git pkgconf-pkg-config
```

Python 3 is optional for normal builds. Install it if you plan to run the
benchmark runner or source packaging script. GSL is optional and only needed to
add GSL comparisons to the comparative benchmark.

Optional Python install commands (**Bash**):

```bash
sudo apt install python3   # Ubuntu/Debian
sudo dnf install python3   # Fedora
```

## Linux GCC Release build

These commands are for **Bash** from the repository root:

```bash
cmake -S . -B build/linux-gcc-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc

cmake --build build/linux-gcc-release --parallel
ctest --test-dir build/linux-gcc-release --output-on-failure
```

Or run the repository script, which also installs the library into the build
folder and validates independent C and C++ consumers:

```bash
scripts/build-linux.sh --config Release --compiler gcc
```

## Linux GCC Debug build

These commands are for **Bash**:

```bash
cmake -S . -B build/linux-gcc-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=gcc

cmake --build build/linux-gcc-debug --parallel
ctest --test-dir build/linux-gcc-debug --output-on-failure
```

Or use the **Bash** helper:

```bash
scripts/build-linux.sh --config Debug --compiler gcc --run-examples
```

The script resolves the repository root from its own location, uses separate
build folders and stops on the first failed command.

## Linux Clang builds

Use separate folders for Clang Debug and Release. These commands are for
**Bash**:

```bash
cmake -S . -B build/linux-clang-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang
cmake --build build/linux-clang-release --parallel
ctest --test-dir build/linux-clang-release --output-on-failure

cmake -S . -B build/linux-clang-debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang
cmake --build build/linux-clang-debug --parallel
ctest --test-dir build/linux-clang-debug --output-on-failure
```

The **Bash** helper provides the same configurations and adds `-Werror`:

```bash
scripts/build-linux.sh --config Release --compiler clang
scripts/build-linux.sh --config Debug --compiler clang
```

## Linux testing

These **Bash** commands list tests, run the complete suite, or select one exact
test:

```bash
ctest --test-dir build/linux-gcc-release -N
ctest --test-dir build/linux-gcc-release --output-on-failure
ctest --test-dir build/linux-gcc-release -R numerix_root_finding --output-on-failure
```

To run the full suite under AddressSanitizer and UndefinedBehaviorSanitizer with
Clang, configure a separate folder:

```bash
cmake -S . -B build/linux-clang-sanitizers \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
cmake --build build/linux-clang-sanitizers --parallel
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/linux-clang-sanitizers --output-on-failure
```

Valgrind and static analysis are optional. The current environment passed a
Valgrind run of the core unit test. Clang's analyzer was clean for the two
production files changed in this work. See the [engineering report](documentation/development/cross_platform_build_report.md)
for exact scope and tool availability.

## Linux demos

The **Bash** script can execute every CLI mode and all five demos after tests
and installed-consumer checks:

```bash
scripts/build-linux.sh --config Release --compiler gcc --run-examples
```

Or run the compiled demos directly from the repository root:

```bash
./build/linux-gcc-release/numerix_expression_demo
./build/linux-gcc-release/numerix_piecewise_demo
./build/linux-gcc-release/numerix_intelligence_demo
./build/linux-gcc-release/numerix_managed_execution_demo
./build/linux-gcc-release/numerix_root_discovery_demo
```

See [Windows demos](#windows-demos) for what each program demonstrates and the
expected kinds of numerical results.

## Linux CLI

The supported command-line options are the same on Linux. Commands below are
for **Bash**; quote expressions so the shell passes them as a single argument:

```bash
./build/linux-gcc-release/numerix_cli --help
./build/linux-gcc-release/numerix_cli --demo
./build/linux-gcc-release/numerix_cli --eval 'sin(pi/2)+x^2' 2
./build/linux-gcc-release/numerix_cli --root 'x^2-2' 0 2
./build/linux-gcc-release/numerix_cli --roots '(x-1)*(x-2)' 0 3
./build/linux-gcc-release/numerix_cli --piecewise-demo
```

The expression example evaluates to `f(2) = 5`. See the [CLI option descriptions](#windows-cli)
for each operation's behavior and limitations.

## Numerical modules

All module APIs are declared in [`include/numerix.h`](include/numerix.h). The
table links to their detailed API contracts and identifies implementation,
test and example sources. The most complete short integration example is in
[External C application integration](#external-c-application-integration).

| Module | Public header | What it does and representative API | Main implementation | Tests and examples | Guide |
| --- | --- | --- | --- | --- |
| Root finding | `include/numerix.h` | Bisection, regula falsi, secant, Newton, safeguarded Newton and Brent methods. Representative call: `numerix_root_brent(...)`. | `src/algorithms/root_finding/root_finding.c` | `tests/test_phase1.c` (`numerix_root_finding`); expression and root-discovery demos also use roots. | [Root finding](documentation/api/root_finding.md) |
| Linear algebra | `include/numerix.h` | Dense vectors/matrices, multiplication, row-major storage, LU decomposition and solves. Representative calls: `numerix_lu_decompose(...)`, `numerix_lu_solve(...)`. | `src/algorithms/linear_algebra/linear_algebra.c` | `tests/test_phase1.c` (`numerix_linear_algebra`); installed consumer uses a 2×2 LU solve. | [Linear algebra](documentation/api/linear_algebra.md) |
| Differentiation | `include/numerix.h` | Backward, central, five-point, Richardson and second derivatives. Representative call: `numerix_diff_central(...)`. | `src/algorithms/differentiation/differentiation.c` | `tests/test_phase1.c` (`numerix_differentiation`); expression demo differentiates a cubic. | [Differentiation](documentation/api/differentiation.md) |
| Interpolation | `include/numerix.h` | Linear interpolation, Lagrange evaluation and reusable Newton interpolants. Representative calls: `numerix_interp_newton_create(...)`, `numerix_interp_newton_evaluate(...)`. | `src/algorithms/interpolation/interpolation.c` | `tests/test_phase1.c` (`numerix_interpolation`); there is no dedicated interpolation demo. | [Interpolation](documentation/api/interpolation.md) |
| Integration | `include/numerix.h` | Composite trapezoid/Simpson and adaptive Simpson integration over finite intervals. Representative call: `numerix_integrate_adaptive_simpson(...)`. | `src/algorithms/integration/integration.c` | `tests/test_phase1.c` (`numerix_phase1_integration`); expression demo integrates `sin(x)`. | [Integration](documentation/api/integration.md) |
| Optimization | `include/numerix.h` | Bounded golden-section minimization and vector gradient descent. Representative call: `numerix_optimize_golden_section(...)`. | `src/algorithms/optimization/optimization.c` | `tests/test_phase1.c` (`numerix_optimization`); no dedicated optimization demo. | [Optimization](documentation/api/optimization.md) |
| ODE | `include/numerix.h` | Fixed-step Euler, Heun and classical RK4 for vector-valued initial-value problems. Representative call: `numerix_ode_rk4(...)`. | `src/algorithms/ode/ode.c` | `tests/test_phase1.c` (`numerix_ode`); no dedicated ODE demo. | [ODE](documentation/api/ode.md) |

Additional public modules include:

- [Expression compilation and evaluation](documentation/api/expression_engine.md):
  `src/expression/`, tested by `tests/test_expression.c` and shown in
  `examples/expression_demo.c`.
- [Piecewise functions and boundary analysis](documentation/api/piecewise.md):
  `src/piecewise/`, tested by `tests/test_piecewise.c` and shown in
  `examples/piecewise_demo.c`.
- [Recommendations and execution portfolios](documentation/api/intelligence.md):
  `src/intelligence/`, tested by `tests/test_intelligence.c` and shown in
  `examples/intelligence_demo.c`.
- [Managed execution and cancellation](documentation/api/execution_management.md):
  `src/execution/`, tested by `tests/test_execution.c` and shown in
  `examples/managed_execution_demo.c`.
- [Diagnostics and sensitivity analysis](documentation/api/diagnostics.md):
  `src/diagnostics/`, tested by `tests/test_diagnostics.c`.
- [Heuristic multiple-root discovery](documentation/api/root_discovery.md):
  `src/root_discovery/`, tested by `tests/test_root_discovery.c` and shown in
  `examples/root_discovery_demo.c`.

## CTest suite

Test names are registered by [`CMakeLists.txt`](CMakeLists.txt). With the
default CMake configuration in this checkout, CTest registers 35 tests. Optional
benchmark and allocation configurations can change the count. The exact test
name, source and purpose are listed below.

| CTest name | Main source/command | Coverage |
| --- | --- | --- |
| `numerix_unit` | `tests/test_numerix.c unit` | Core units and common numerical behavior |
| `numerix_integration` | `tests/test_numerix.c integration` | Cross-module integration checks |
| `numerix_root_finding` | `tests/test_phase1.c root_finding` | Scalar root methods |
| `numerix_linear_algebra` | `tests/test_phase1.c linear_algebra` | Vectors, matrices and LU |
| `numerix_differentiation` | `tests/test_phase1.c differentiation` | Finite differences |
| `numerix_interpolation` | `tests/test_phase1.c interpolation` | Interpolation methods |
| `numerix_phase1_integration` | `tests/test_phase1.c integration` | Finite integration methods |
| `numerix_optimization` | `tests/test_phase1.c optimization` | Bounded and gradient methods |
| `numerix_ode` | `tests/test_phase1.c ode` | Fixed-step ODE methods |
| `numerix_expression_engine` | `tests/test_expression.c` | Parser, compiler and evaluator |
| `numerix_audit_regressions` | `tests/test_audit_regressions.c` | Numerical audit regression cases |
| `numerix_piecewise` | `tests/test_piecewise.c` | Piecewise functions and boundaries |
| `numerix_intelligence` | `tests/test_intelligence.c` | Recommendation and fallback |
| `numerix_execution` | `tests/test_execution.c` | Managed execution behavior |
| `numerix_diagnostics` | `tests/test_diagnostics.c` | Diagnostics, convergence and sensitivity |
| `numerix_root_discovery` | `tests/test_root_discovery.c` | Multi-root searches and edge cases |
| `numerix_phase8_integration` | `tests/test_phase8_integration.c` | Cross-module Phase 8 integration |
| `numerix_release_regressions` | `tests/test_release_regressions.c` | Release-blocking numerical regressions |
| `numerix_phase13b_regressions` | `tests/test_phase13b_regressions.c` | Correctness and measurement-gate regressions |
| `numerix_benchmark_measurement` | `tests/test_benchmark_measurement.c` | Paired timing statistics and metadata helpers |
| `numerix_cli_startup` | `cli/main.c` | CLI no-argument startup behavior |
| `numerix_cli_help` | `cli/main.c` | CLI help output |
| `numerix_cli_expression` | `cli/main.c` | Expression command result |
| `numerix_cli_root` | `cli/main.c` | Single-root command |
| `numerix_cli_discovery` | `cli/main.c` | Heuristic multi-root command |
| `numerix_cli_piecewise` | `cli/main.c` | Piecewise demonstration command |
| `numerix_cli_invalid` | `cli/main.c` | Invalid CLI input handling |
| `numerix_cli_domain_error` | `cli/main.c` | Expression domain-error handling |
| `numerix_example_expression` | `examples/expression_demo.c` | Expression demo starts and completes |
| `numerix_example_piecewise` | `examples/piecewise_demo.c` | Piecewise demo starts and completes |
| `numerix_example_intelligence` | `examples/intelligence_demo.c` | Recommendation demo starts and completes |
| `numerix_example_managed_execution` | `examples/managed_execution_demo.c` | Managed execution demo starts and completes |
| `numerix_example_root_discovery` | `examples/root_discovery_demo.c` | Root discovery demo starts and completes |
| `numerix_accuracy` | `tests/test_accuracy.c` | Accuracy checks against analytical references |
| `numerix_portability_layout` | `tests/portability_probe.c` | Public layout and portability probe |

Run one test on **Bash** or **PowerShell** with the same CTest syntax:

```sh
ctest --test-dir build/linux-gcc-release -R numerix_root_finding --output-on-failure
```

This form needs CTest 3.20 or newer. For CTest 3.16–3.19, use this
**Bash/PowerShell**-compatible invocation instead:

```sh
cmake -E chdir build/linux-gcc-release ctest -R numerix_root_finding --output-on-failure
```

The source can also be run directly, but CTest is the canonical interface
because it supplies selectors and executable paths. Optional comparative
benchmark builds register `numerix_benchmark_raw_verification` when Python is
found. Optional allocation fault-injection builds can register another test.

## Benchmarks

Benchmarking is separate from correctness and accuracy testing. The internal
diagnostics and root-discovery executables are optional CMake targets excluded
from the default build:

```bash
cmake --build build/linux-gcc-release --target numerix_diagnostics_benchmark
./build/linux-gcc-release/numerix_diagnostics_benchmark

cmake --build build/linux-gcc-release --target numerix_root_discovery_benchmark
./build/linux-gcc-release/numerix_root_discovery_benchmark
```

The diagnostics benchmark times root finding, dense linear algebra,
differentiation, integration, optimization and RK4 with diagnostics disabled,
basic and full. The root-discovery benchmark runs its fixed root, multiple-root,
repeated-root, close-root, no-root and pole cases at several panel counts and
reports time, evaluations, allocations and unresolved intervals. These are
machine-specific observations, not portable performance guarantees.

The optional comparative benchmark can run NUMERIX workloads alone or compare
supported paths with GSL when GSL is installed. GSL is not a library dependency.
Build and run from **Bash**:

```bash
cmake -S . -B build/linux-gcc-comparison \
  -DCMAKE_BUILD_TYPE=Release \
  -DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON
cmake --build build/linux-gcc-comparison --parallel
./build/linux-gcc-comparison/numerix_comparison_benchmark --versions
python3 scripts/run_benchmarks.py \
  --build-dir build/linux-gcc-comparison \
  --output /tmp/numerix-results
```

The runner records run metadata and raw measurements in the selected output
directory. Read [`benchmarks/README.md`](benchmarks/README.md) before comparing
results: workload definitions, accuracy gates, paired timing, statistics,
allocation scope, supported libraries and interpretation limits are described
there. `NUMERIX_BENCHMARK_ALLOCATIONS=ON` is a separate Linux GNU/ELF probe and
requires static GSL/CBLAS archives; it is not supported on Windows or MSVC.

This work built and ran both internal benchmark executables. It also built the
comparison executable in NUMERIX-only mode (GSL was unavailable), ran its
version check and passed the raw-output verification test. No comparative
performance run was recorded here. Phase 14B's method-level report currently
contains root measurements while other module runs remain pending; see the
[Phase 14B completion report](documentation/development/phase_14b_completion_report.md).
Historical benchmark outputs are retained as their own datasets and should not
be overwritten or combined without checking their schemas and run metadata.

## External C application integration

The public API is in `<numerix.h>`. Check each returned `numerix_status`, read
`numerix_error.message` on failure, initialize options with
`numerix_options_init`, and destroy objects that the API returns. Ownership,
status and numerical limitations are explained in the [release contract](documentation/api/release_contract.md).

This complete **C** example solves `x*x-2=0` on `[0,2]`:

```c
#include <numerix.h>
#include <stdio.h>

static double equation(double x, void *data)
{
    (void)data;
    return x * x - 2.0;
}

int main(void)
{
    numerix_result result;
    numerix_error error;
    numerix_status status = numerix_root_brent(
        equation, NULL, 0.0, 2.0, NULL, &result, &error);

    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "NUMERIX: %s\n", error.message);
        return 1;
    }
    printf("root = %.12g; residual = %.3e\n", result.value, result.residual);
    return 0;
}
```

Install first, for example with the **Bash** command below:

```bash
cmake --install build/linux-gcc-release --prefix "$PWD/install"
```

For a project using CMake, add this to its `CMakeLists.txt`:

```cmake
find_package(NUMERIX 0.2 CONFIG REQUIRED)
add_executable(my_program main.c)
target_link_libraries(my_program PRIVATE NUMERIX::numerix)
```

Then configure with **Bash**:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$PWD/install"
cmake --build build --parallel
```

The imported target supplies the include directory and platform math linkage.
The repository's [`tests/installed_consumer/`](tests/installed_consumer/)
project compiles the same real API consumer as C and C++.

For manual static linking, set `PREFIX` to the install prefix and use the
library directory selected by `CMAKE_INSTALL_LIBDIR`. These commands are for
**Bash**; the default directory may be `lib` or `lib64` depending on the system:

```bash
cc -std=c11 -I"$PREFIX/include" main.c \
  "$PREFIX/lib/libnumerix.a" -lm -o my_program
clang -std=c11 -I"$PREFIX/include" main.c \
  "$PREFIX/lib/libnumerix.a" -lm -o my_program
```

Change `lib` to `lib64` if that is the configured install directory. For
MinGW, use **PowerShell** and the MinGW archive name; Windows does not link `m`:

```powershell
gcc -std=c11 -I"$env:NUMERIX_PREFIX\include" .\main.c `
  "$env:NUMERIX_PREFIX\lib\libnumerix.a" -o .\my_program.exe
```

For MSVC, use **Developer PowerShell** and set `$env:NUMERIX_PREFIX` to the
installation prefix. This links the MSVC static-library name:

```powershell
& cl /nologo /std:c11 "/I${env:NUMERIX_PREFIX}\include" .\main.c `
  "${env:NUMERIX_PREFIX}\lib\numerix.lib" /Femy_program.exe
```

The CMake package is preferred when the install directory is customized.

On POSIX systems, pkg-config metadata is installed under
`$PREFIX/$CMAKE_INSTALL_LIBDIR/pkgconfig`. For this static library, request
private link flags (including `-lm`) with `--static`:

```bash
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"  # use lib64/pkgconfig if configured
cc -std=c11 $(pkg-config --cflags --libs --static numerix) main.c -o my_program
```

Native Windows pkg-config consumption was not tested. CMake package discovery
is the supported cross-platform integration path. `pkg-config` command-line
flags do not reliably quote install paths containing spaces; the CMake package
was tested successfully with a prefix containing spaces.

## Installation and packaging

CMake installs the static archive, public header, exported `NUMERIX::numerix`
target, package config/version files and `numerix.pc`. The default install
prefix is platform-dependent; set it explicitly when needed:

```bash
cmake --install build/linux-gcc-release --prefix "$PWD/install"
```

The corresponding **PowerShell** command is:

```powershell
cmake --install build\windows-release --prefix "$PWD\install"
```

To create a source archive, install Python 3 and run this **Bash** command:

```bash
python3 scripts/package_source.py --output dist
```

CPack can create a binary package for the current build platform. Neither the
source archive nor CPack establishes that an untested platform binary is
validated. See [installation details](documentation/user_guide/installation.md)
and [compatibility/versioning](documentation/user_guide/compatibility.md).

## Troubleshooting

| Symptom | Check and fix |
| --- | --- |
| `cmake` not found | Install CMake or the MSYS2 UCRT64 CMake package, reopen the terminal, then run `cmake --version`. |
| `gcc` or `g++` not found on Windows | Open an MSYS2 UCRT64 terminal, install `mingw-w64-ucrt-x86_64-gcc`, or add the matching `ucrt64\bin` directory to the current PowerShell PATH. |
| `cl.exe` not found | Use the x64 Developer PowerShell/Command Prompt installed with Visual Studio 2022 C++ tools. Do not point an MSVC CMake build at MinGW GCC. |
| `mingw32-make` not found | Install `mingw-w64-ucrt-x86_64-make` from UCRT64 and check that `C:\msys64\ucrt64\bin` is on PATH. |
| MSYS2 GCC is from the wrong environment | Use the UCRT64 shell/toolchain. Plain MSYS, MINGW64 and UCRT64 have different runtimes; use a fresh build directory after changing toolchains. |
| CMake says the generator does not match | A build directory caches its generator. Configure into the documented generator-specific directory; do not mix generators in one cache. |
| CMake configuration fails | Save the full configure output; check compiler version, generator, CMake version and path. Reconfigure in a new build directory instead of deleting unrelated files. |
| Windows pkg-config path error from old `file(RELATIVE_PATH)` code | The old code supplied Unix-root paths. The current code derives the relative path from the configured native install prefix and library directory. Pull the corrected CMake source and configure again. |
| MinGW compiler warnings | Keep the warning output and compiler version. The sign checks in diagnostics and root discovery were changed to avoid MinGW's `signbit` narrowing warnings; do not silence all warnings. |
| Undefined references while linking | Link the installed static archive and platform math runtime (`-lm` on Linux/macOS). With pkg-config, use `pkg-config --static --libs numerix`. Prefer `NUMERIX::numerix` in CMake. |
| Windows reports a missing runtime DLL | Run the executable from the matching UCRT64 environment or ensure its matching UCRT64 runtime directory is on PATH. Do not mix MSYS2 runtimes. |
| A test fails | Run the exact name with `ctest --test-dir <build-dir> -R <test-name> -VV`; retain stdout/stderr and the build configuration. Do not skip or weaken the test. |
| CLI expression is split or interpreted by a shell | Quote the whole expression. In PowerShell and Bash, single quotes are useful for examples such as `'x^2-2'`. |
| Install or CMake package discovery fails | Verify the install prefix contains `include/numerix.h` and the CMake package files under its library directory. Set `CMAKE_PREFIX_PATH` to the prefix itself. |
| pkg-config cannot locate NUMERIX | Set `PKG_CONFIG_PATH` to the installed `lib/pkgconfig` or `lib64/pkgconfig` folder and run `pkg-config --validate numerix`. Use `--static` when linking the static archive. |

For a useful report, include the OS, compiler and CMake versions, exact configure
and build commands, complete failing output, and the matching CTest name. Verbose
build output is available with `cmake --build <build-dir> --verbose`; use
`ctest -VV` to include test command output.

## Contributing

Use a separate build directory for each compiler and configuration. Before
submitting a change, build and run CTest with the compiler you changed, keep
numerical assertions and tolerances intact, and document any platform result
that could not be checked locally. Keep new benchmark data in a new run
directory with its metadata; do not replace historical measurements. No
separate contribution guide is currently present.

## License

NUMERIX is distributed under the [MIT License](LICENSE).

## Historical reports and technical documentation

- [API contracts and guides](documentation/api/)
- [User installation and compatibility guides](documentation/user_guide/)
- [Architecture and ownership notes](documentation/architecture/)
- [Final project report](documentation/development/final_project_report.md)
- [Final release checklist](documentation/development/final_release_checklist.md)
- [Final benchmark report](documentation/development/final_benchmark_report.md)
- [Phase 10 accuracy report](documentation/development/phase10_accuracy_report.md)
- [Phase 11 portability report](documentation/development/phase11_portability_report.md)
- [Phase 13A benchmark-readiness audit](documentation/development/phase_13a_benchmark_readiness_audit.md)
- [Phase 13B baseline](documentation/development/phase_13b_baseline.md) and [completion report](documentation/development/phase_13b_completion_report.md)
- [Phase 13C/13D readiness](documentation/development/phase_13c_readiness_report.md) and [completion report](documentation/development/phase_13c_13d_completion_report.md)
- [Phase 14 audit and execution log](documentation/development/phase_14_audit_and_execution_log.md), [completion report](documentation/development/phase_14_completion_report.md) and [library capability matrix](documentation/development/phase_14_library_capability_matrix.md)
- [Phase 14B completion report](documentation/development/phase_14b_completion_report.md), [execution log](documentation/development/phase_14b_execution_log.md), [method coverage](documentation/development/phase_14b_method_coverage_matrix.md), [library capability matrix](documentation/development/phase_14b_library_capability_matrix.md) and [verification report](documentation/development/phase_14b_verification_report.md)
- [Current cross-platform build report](documentation/development/cross_platform_build_report.md)
- [Comparative evaluation](documentation/reports/NUMERIX_comparative_evaluation.md), [expanded multi-library evaluation](documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.md) and [Phase 14B method-level evaluation](documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md)
- [Benchmark protocol and data guide](benchmarks/README.md)

The reports preserve results and limitations from their respective phases. They
are not interchangeable runs: consult each report's environment, method
coverage and schema before comparing measurements.
