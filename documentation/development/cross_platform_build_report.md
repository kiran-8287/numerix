# Cross-platform build and documentation report

**Work date:** 2026-11-12  
**Repository baseline:** `cbc3d27bb89451f66f938cfe245274f0dccb37b4`  
**Project version:** 0.2.0-rc1

## Scope and status

This work corrected the install metadata path calculation that failed on
Windows, adjusted the two reported `signbit` warning sites, retained and made
the Windows CI compiler/generator selections explicit, added developer build
scripts and rewrote the repository entry-point documentation. Linux GCC and
Clang validation is complete for the configurations listed below. A Windows
host and a GitHub Actions run were unavailable in this work, so MinGW/MSVC and
remote CI results remain pending.

No numerical algorithm, public API signature, test assertion or benchmark
measurement protocol was intentionally changed. Historical phase files and
measurement data were left intact. One previously untracked Phase 14B adapter
was updated to avoid the same conversion warning in that saved tooling.

## 1. Original CMake failure and correction

The old `file(RELATIVE_PATH ...)` call passed `/${CMAKE_INSTALL_LIBDIR}/pkgconfig`
and `/` as absolute paths. Those Unix-root paths are not valid absolute paths
for Windows, where a drive-qualified path is needed. CMake therefore failed
while configuring the project, before compilation or tests.

In [`CMakeLists.txt`](../../CMakeLists.txt), when `CMAKE_INSTALL_LIBDIR` is
relative, the new code forms the pkg-config directory below the configured
install prefix and computes a path from that directory back to the same prefix.
The generated `.pc` file uses `${pcfiledir}` for that relative relationship and
`${prefix}` for the library and include directories. No synthetic root path or
arbitrary `.` substitution is used. If the user explicitly sets an absolute
library directory, the generated metadata keeps that configured absolute
directory and prefix; that case is deliberately non-relocatable. Absolute
include directories are also respected.

The Linux configure/install checks exercised a custom prefix, a `lib64` library
directory and a prefix containing spaces. The CMake package's independent C and
C++ consumers passed. The generated pkg-config metadata validated and a static
consumer compiled and ran when invoked with `pkg-config --static` (needed to
include `Libs.private`, including `-lm`). A shell compile using a prefix with
spaces does not work reliably with pkg-config's unescaped `-I`/`-L` output;
the CMake package worked at that prefix. Native Windows CMake configuration and
Windows pkg-config consumption were not run, so the Windows-specific behavior
still needs the native workflow evidence.

## 2. Compiler portability changes

`src/diagnostics/diagnostic_analysis.c` now checks sign using
`copysign(1.0, value) < 0.0`. This retains negative-zero detection and avoids
the MinGW warning reported at its previous `signbit` call. The diagnostics
regression suite now includes a `-0.0` history value and checks its oscillatory
classification.

`src/root_discovery/root_discovery.c` already treats either zero endpoint as
not an opposite-sign pair. After that check it now compares the signs with
`(left < 0.0) != (right < 0.0)`, which is equivalent for finite, nonzero
endpoints and does not call the warning-producing macro. Clang analysis and
warnings-as-errors builds passed for the modified production sources. Native
MinGW warnings and MSVC compilation remain unverified.

The untracked Phase 14B runner
[`phase14b_root_runner.c`](../../benchmarks/adapters/phase14b/phase14b_root_runner.c)
had equivalent `signbit` calls and received the same `copysign` treatment. It
is a benchmark adapter created by the earlier Phase 14B work, not a new
production target.

## 3. Build, workflow and documentation changes

- [`scripts/build-linux.sh`](../../scripts/build-linux.sh) supports GCC/Clang
  Debug and Release builds, CTest, staged installation, independent C/C++
  installed consumers and optional CLI/demo execution. It resolves the
  repository root from its own location, uses separate ignored build folders,
  enables `-Werror`, stops on errors and does not delete files.
- [`scripts/build-windows.ps1`](../../scripts/build-windows.ps1) supports MinGW
  and MSVC Debug/Release configurations, CTest, staged installation and
  independent C/C++ consumers. `-RunExamples` also invokes every CLI mode and
  demo. It checks that the selected compiler tools are on PATH and stops on a
  failing native command. It was not executable on this Linux host.
- [`.github/workflows/validation.yml`](../../.github/workflows/validation.yml)
  retains Ubuntu GCC, Ubuntu Clang, macOS Clang and Windows MSVC Debug/Release
  matrix entries, plus the separate Windows UCRT64 MinGW Debug/Release matrix.
  It now explicitly selects Visual Studio 2022 x64 for MSVC and explicitly
  chooses GCC/G++ for MinGW consumer builds. The UCRT64 Make package is named
  alongside the existing GCC, CMake and Ninja packages. Tests, installed
  consumers, versions and portability artifacts remain part of the jobs.
- [`README.md`](../../README.md) now contains new-developer setup instructions,
  platform status, exact targets/options/test names, module links, external
  consumer examples, packaging, troubleshooting and history links.

The scripts invoke CTest through `cmake -E chdir` so they also work with the
declared CMake 3.16 minimum. The convenience option `ctest --test-dir` was
introduced in CTest 3.20; README calls that out and retains the direct command
examples for current CTest.

## 4. Linux validation

Tool versions on this host were GCC 8.5.0, Clang 21.1.8 and CMake/CTest 4.4.4.
The CMake compiler flags included `-Werror` in the four standard configurations.

| Compiler | Configuration | Result | CTest |
| --- | --- | --- | ---: |
| GCC 8.5.0 | Release | Passed | 35/35 |
| GCC 8.5.0 | Debug | Passed | 35/35 |
| Clang 21.1.8 | Release | Passed | 35/35 |
| Clang 21.1.8 | Debug | Passed | 35/35 |

All four configurations also installed NUMERIX into a build-local prefix,
configured and built the independent C and C++ consumers with the same compiler
family, and passed 2/2 consumer tests. The GCC Release `--run-examples` script
run succeeded after building and testing: CLI help, demo, expression, root,
discovery and piecewise modes all returned success, followed by all five demo
programs.

An additional GCC Release build enabled
`NUMERIX_BUILD_COMPARISON_BENCHMARK=ON` with GSL explicitly disabled. It built
the comparison executable, passed its `--versions` output check and passed the
expanded **36/36** CTest suite, including raw benchmark output verification.
This was NUMERIX-only; it is not a GSL comparison result.

Clang Debug with AddressSanitizer, UndefinedBehaviorSanitizer and leak detection
passed **35/35** CTest tests. The host's GCC sanitizer runtime was unavailable:
the GCC sanitizer link check could not find `/usr/lib64/libasan.so.5.0.0` and
`/usr/lib64/libubsan.so.1.0.0`. Clang supplied working sanitizer runtimes, so
the sanitizer suite was run successfully with Clang.

Valgrind 3.22.0 passed `numerix_tests unit` with full definite/indirect leak
checking and an error exit code. This is one core unit test executable run, not
a claim that the entire suite was run under Valgrind. Clang's static analyzer
reported no diagnostics on `diagnostic_analysis.c` and `root_discovery.c`.
GCC 8.5.0 does not implement `-fanalyzer`; its analyzer command was unavailable.

## 5. Windows validation and CI

No native Windows execution occurred in this work. MinGW Debug, MinGW Release,
MSVC Debug and MSVC Release therefore remain **not tested here**. The Windows
workflow changes are source-reviewed but have no new hosted-run result. The
repository was not pushed, and no remote GitHub Actions check was started or
claimed. The matrix still has all requested compiler and configuration entries.

The `MSVC` CMake branch requires Visual Studio 2022 17.5+ with experimental C11
atomics, as checked by the existing project configuration. The Windows setup
script's PowerShell syntax, installed package path behavior, generated `.pc`
metadata and runtime DLL behavior also need confirmation on a Windows host.

## 6. Demos, CLI and benchmark targets

On Linux GCC Release, the following CLI options all returned success:
`--help`, `--demo`, `--eval 'sin(pi/2)+x^2' 2`, `--root 'x^2-2' 0 2`,
`--roots '(x-1)*(x-2)' 0 3` and `--piecewise-demo`. The expression returned
`f(2) = 5`. All five demo executables returned success:

- `numerix_expression_demo`
- `numerix_piecewise_demo`
- `numerix_intelligence_demo`
- `numerix_managed_execution_demo`
- `numerix_root_discovery_demo`

The optional `numerix_diagnostics_benchmark` and
`numerix_root_discovery_benchmark` targets were built and run on Linux. The
optional `numerix_comparison_benchmark` built and ran `--versions` without GSL;
its raw verification CTest passed. No timings from these local benchmark
executions are presented as performance claims. No benchmark methodology or
historical measurement file was changed.

## 7. Installation and external consumers

The Linux install test used a prefix containing a space and `CMAKE_INSTALL_LIBDIR=lib64`.
The installed C and C++ consumer targets both configured, built and passed.
The `.pc` file generated relocatable relative-prefix paths for the relative
library directory. A separate no-space prefix was validated with pkg-config;
`pkg-config --static` supplied `-lm`, and a small compiled root-solving consumer
ran successfully.

There is no native Windows installation or consumer result. The CMake exported
target remains the documented cross-platform integration mechanism; pkg-config
is documented for POSIX and Windows use is explicitly unverified.

## 8. README and test coverage

The README covers the requested 29 entry-point sections: project and platform
status, repository map, requirements, UCRT64 setup, MinGW/MSVC build and test
commands, all demos and CLI modes, Linux GCC/Clang instructions, module guides,
the current exact CTest list, benchmarks, external C/C++ consumers,
installation/packaging, troubleshooting, contribution notes, license and
historical reports. It labels Linux configurations tested and Windows/macOS
instructions expected but not tested in this work.

The default CMake CTest set has 35 exact test names, sources and purpose
descriptions in the README. A comparison benchmark build with Python present
adds `numerix_benchmark_raw_verification`; optional allocation instrumentation
can add another test. The README does not treat 35 as a permanent count.

## 9. Remaining limits

- Native Windows MinGW and MSVC Debug/Release jobs have not been run locally.
- The updated GitHub Actions workflow has not run remotely; a push and hosted
  check are still needed, subject to the repository's no-push instruction for
  this task.
- macOS was not tested in this work.
- Windows pkg-config is unverified. POSIX pkg-config shell flags do not reliably
  quote prefixes containing spaces; the CMake package handles the tested
  space-containing prefix.
- GSL was unavailable for this work's comparison build, so only NUMERIX-only
  benchmark verification was performed. Phase 14B's pending method coverage
  remains pending.
- GCC sanitizer libraries and GCC `-fanalyzer` were unavailable. Clang
  ASan/UBSan ran the complete default test suite, and Clang static analysis
  covered the two modified production files.
- Valgrind coverage in this task was the core unit test executable, not every
  CTest process.
- Only static library builds are supported; accuracy and heuristic limitations
  remain described in the API and benchmark documentation.

## 10. Quick start

Windows MinGW, **PowerShell** after installing the prerequisites:

```powershell
git clone https://github.com/kiran-8287/numerix.git
cd numerix
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
.\scripts\build-windows.ps1 -Compiler MinGW -Configuration Release -RunExamples
```

Windows MSVC, **Developer PowerShell for VS 2022**:

```powershell
git clone https://github.com/kiran-8287/numerix.git
cd numerix
.\scripts\build-windows.ps1 -Compiler MSVC -Configuration Release -RunExamples
```

Linux, **Bash**:

```bash
git clone https://github.com/kiran-8287/numerix.git
cd numerix
scripts/build-linux.sh --config Release --compiler gcc --run-examples
```

## 11. Git status and preservation

No commit or push was made. The task's tracked changes are:

- Modified: `.github/workflows/validation.yml`, `CMakeLists.txt`,
  `cmake/numerix.pc.in`, `src/diagnostics/diagnostic_analysis.c`,
  `src/root_discovery/root_discovery.c`, `tests/test_diagnostics.c` and
  `README.md`.
- Created: `scripts/build-linux.sh`, `scripts/build-windows.ps1` and this report.
- Modified from the earlier untracked Phase 14B work:
  `benchmarks/adapters/phase14b/phase14b_root_runner.c`.

The pre-existing Phase 14B requirements, matrices, raw data, reports and other
untracked files were preserved. Build and install outputs are under ignored
build folders or `/tmp`; no historical benchmark results were replaced.
