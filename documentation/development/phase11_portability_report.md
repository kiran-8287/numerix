# Phase 11 — platform and compiler validation

Date: 2026-11-12. Numerical baseline: Phase10 commit
5900aee77480821aeb2559a864b76eea079a14c2. No numerical algorithms were changed
in this phase. The Phase10 completion gate was satisfied before this work began.

## Actual validation matrix

| Platform | Compiler | Debug | Release | Tests and consumers | Status |
| --- | --- | --- | --- | --- | --- |
| Oracle Linux8.10 x86_64 | GCC8.5.0 | Passed | Passed |33 CTests/config; Make full suite; installed C/C++ clients | Passed |
| Same Linux | GCC11.5.0 + matching binutils2.36.1 | Passed | Passed |33 CTests/config | Passed |
| Same Linux | GCC15.2.1 + matching toolset binutils | Passed | Passed |33 CTests/config | Passed |
| Same Linux | Clang21.1.8 | Passed | Passed |33 CTests/config; installed C/C++ clients | Passed |
| Same Linux | Clang ASan+UBSan+leak detection | Passed | n/a |34 CTests, including allocation faults | Passed |
| Windows native | MSYS2 UCRT64 MinGW | Not executed | Not executed | Hosted run requested; no jobs/results returned | Unvalidated |
| Windows native | MSVC | Not executed | Not executed | Hosted run requested; no jobs/results returned | Unvalidated |
| macOS native | Apple Clang | Not executed | Not executed | Hosted run requested; no jobs/results returned | Unvalidated |

Windows and macOS execution environments are unavailable locally. Hosted CI was
configured and pushed, but runs remained queued with empty job lists while the
available validation was completed. This is **not** a compiler success/failure
result, nor proof of portability. See the actual
[run snapshot](../../benchmarks/results/phase11/hosted/run.json) and
[hosted workflow](https://github.com/kiran-8287/numerix/actions/runs/43096194532).
Queued jobs must execute before those platform rows can be upgraded. Phase8's
native Windows MinGW6.3 results remain historical, not modern revalidation.

## Build systems and portability work

Make and CMake still build the same25 production sources. GSL remains optional
and benchmark-only. CMake registers the accuracy and public-layout probes along
with the earlier suites, all five examples, CLI and integration tests. Make now
supports PREFIX/DESTDIR installation on POSIX/MSYS2; a real installed C client
was built and run against its archive/header. Native Windows can use CMake install.

MSVC C11 atomics require `/experimental:c11atomics` on supported Visual Studio
versions. CMake now checks the actual header/atomic operation and gives an explicit
configuration error if unavailable, then applies that compiler flag privately.
This preparation follows Microsoft's documented C11 atomics requirement; it is
not a substitute for the pending native job. No compiler-specific numerical method
or public atomic type was introduced.

The hosted matrix defines Debug/Release GCC, Clang, Apple Clang, MSVC and MinGW
jobs, optional benchmark compilation without GSL, full CTest execution, installation,
independent C/C++ consumers, public-layout/version artifacts, and native MinGW
Make/CLI/example execution. MSYS2 command argument conversion is explicitly handled.
Only actual successful jobs may be used as validation evidence.

GCC11 initially invoked the older host assembler, which rejected `--gdwarf-4`.
The failure is retained in gcc11_debug.log. Using `-B` to select the compiler's
matching binutils resolved it; both configurations passed. No warning was suppressed
and no source change was made to mask this toolchain mismatch.

## Memory safety and static analysis

- Passed: ASan+UBSan, detect_leaks=1 and halt_on_error=1, all34 tests.
- Passed: Valgrind on12 complete unit/integration executables, including the new
  accuracy suite; zero invalid accesses/leaks at exit.
- Passed: separate Valgrind allocation-fault executable and all222 linked-static
  allocation probes; no errors/leaks. Fault tests retain expression/discovery
  traces and now include adaptive Simpson heap growth/failure cleanup.
- Passed: GCC15 `-fanalyzer` and Clang analyzer on all25 library translation units,
  with no diagnostics in final logs.
- Passed: exhaustive Cppcheck2.18.3 warning/performance/portability analysis of
  library, tests, benchmarks, CLI and examples. Earlier five test-harness findings
  were addressed by real retained-owner sentinels and guards after failed checks,
  preserving output-reset assertions without integer-derived pointers/crashes.
  No finding was indiscriminately suppressed.

[Local command/results](../../benchmarks/results/phase11/local/results.json) and
individual logs retain actual commands/exits. Local runner records accurately
mark local Windows tools unavailable; the separate hosted snapshot records queued
attempts. These scopes must not be conflated.

## Public API, layouts and consumers

The public header remains C11/C++ compatible with no Phase11 public signature or
layout changes. Four installed consumers (GCC/G++, Clang/Clang++) compiled/linked
and ran. A separate CMake consumer tests scaled roots, LU, expressions, error
returns and resource cleanup as both C and C++, linking only the installed archive.
It never compiles NUMERIX implementation sources. Phase12 will replace its manual
installed-library discovery with an exported package target.

On this Linux x86_64 ABI: pointer/size_t8 B, double53 mantissa bits, long double64,
status4 B, result64 B, options40 B, diagnostics3400 B, discovered root7536 B and
search report248 B. Raw output is in public_layout.txt. These observations do not
establish cross-platform/cross-compiler ABI compatibility. Static archives are the
supported form; shared-library ABI/export validation is not claimed.

## Completion gate and limits

All locally available compilers, configurations, sanitizers, memory/static tools,
Make/CMake builds and installed clients executed successfully. Critical findings
were investigated, reports retained and platform-dependent preparation committed
separately. Native Windows/macOS remain explicitly unvalidated because no execution
results were available from their queued hosted requests. This allowed completion
of the **available-environment validation** gate, not a claim that every platform
passed. Phase12 must retain release-candidate status unless missing evidence is
obtained. Exact phase commits/push status are in the final completion response.

Microsoft reference: https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2/
