# Final release checklist

Date: 2026-11-12 (Asia/Kolkata). Candidate version: **0.1.0**.
Assessment: **release candidate with documented limitations**.

This checklist classifies actual executed checks. “Passed” covers the stated
scope; it does not imply every platform/input is validated. Command outputs and
exit records are under [validation](../../benchmarks/results/phase9/validation/).

| Area | Status | Evidence / remaining action |
| --- | --- | --- |
| Initial Git state | Passed | Clean master at 29e2f7ac536e0d71f38a0e59ad148556b1b0c84f; origin/master matched; Phase 8 report inspected |
| Existing functionality | Passed | Original baseline17 CTests and Make suite; final regression suites preserve all numerical families |
| Clean Make Debug | Passed | GCC 8.5 `-O0 -g3 -Werror`, all targets and complete tests |
| Clean Make Release | Passed | GCC 8.5 `-O3 -DNDEBUG -Werror`, all targets and complete tests |
| CMake GCC Debug / Release | Passed | Separate configurations; all 31 CTests in each |
| CMake Clang Debug / Release | Passed | Clang 21.1.8; all 31 CTests in each, warning errors enabled |
| CTest registration | Passed | Unit, integration, CLI and all five examples registered; optional allocation faults make32 tests |
| Benchmark targets | Passed | Make/CMake GSL comparison; both existing internal benchmark targets |
| GSL optional dependency | Passed | Explicitly disabled-GSL CMake build and all NUMERIX-only benchmark cases; no GSL dependency in archive/public header |
| Linux portability | Passed | Oracle Linux8.10 x86_64 with GCC 8.5 and Clang 21.1; other Linux environments untested |
| Windows MinGW Phase 9 | Unavailable | No native Windows environment/toolchain; Phase 8's Windows GCC 6.3 results remain historical |
| MSVC Phase 9 | Unavailable | No MSVC; CMake MSVC branch is not treated as build evidence |
| ASan / UBSan / leak detection | Passed | Combined Clang sanitizer configuration;32 CTests including allocation failures, detect_leaks=1, halt_on_error=1 |
| Valgrind unit/integration | Passed | All11 test executables, zero errors and no live heap at exit |
| Valgrind full memory benchmark / fault injection | Passed | Complete linked-static-GSL allocation probe and all injection cases, zero errors/leaks |
| Allocation failure handling | Passed | Every observed allocation index in expression compile and six discovery workloads; selected paths only |
| GCC static analysis | Passed | GCC 15.2.1 `-fanalyzer` on 25 library translation units, no final diagnostics |
| Clang static analysis | Passed |25 library translation units plus benchmark, tracker and CLI, no final diagnostics |
| Cppcheck production analysis | Passed | Locally built2.18.3, exhaustive warning/performance/portability scan of library/benchmarks/CLI/examples |
| Supplemental Cppcheck test-harness scan | Partially passed | Three possible dereferences following failed CHECKs; two intentional integer-to-pointer sentinels. Documented in static_analysis_review.md, no suppressions |
| Numerical accuracy | Passed | Verifier confirms recorded calculations/expected failure classes.222 observations include11 status failures and21 successful-call misses; accuracy is not claimed for those cases |
| Runtime statistics | Passed |21 batches/case, calibrated >=2 ms target, min/median/mean/sample SD/p95 retained; different-work pairs excluded from direct ratios |
| Memory measurements | Passed |222 independent probes; allocations/cumulative bytes/peak live/setup bytes; zero tracked bytes after cleanup. Direct search metrics and process RSS reported separately |
| NUMERIX capabilities | Passed | Expression, piecewise, recommendation, execution, diagnostics, discovery and cross-phase functional suites; selected independent performance workloads |
| CLI | Passed | Startup/help, expression input, managed root/diagnostics, discovery, fixed piecewise demo, invalid numeric and domain-error cases |
| Examples | Passed | All five compile and execute through CTest; Valgrind checks library tests separately |
| Public API review | Passed | Ownership, initialization, output/error behavior and ABI limits documented; no public signatures/fields added in Phase 9 |
| Stable ABI guarantee | Not attempted | Deliberately not promised for 0.1.0; Phase 8 public struct growth requires rebuilding old clients |
| Documentation | Passed | Current README, benchmark definitions, raw CSVs, generated tables, final report and ownership/release guide |
| Install packaging | Passed | CMake install to temporary prefix; independent C11 and C++11 consumers compile/link/run |
| Binary package generation | Passed | CPack TGZ generated outside repo; archive contains NUMERIX static library/header/API docs/license |
| Source packaging | Passed | Git-tracked source review and reproducible archive recipe; no external dependency sources or build artifacts committed |
| Final diff / test / working tree | Passed | Final verification and Git status recorded in completion response; raw measurement metadata deliberately captures pre-commit dirty state |
| Commit / push | Not attempted at report snapshot | Performed after final verification; exact final hash/push/tree status is in the completion response. Existing remote only, no force push |
| Release tag / public binary publication | Not attempted | Neither requested nor implied; no externally uploaded release assets |

## Remaining limitations before a stronger stable-release claim

1. Rebuild/test natively on Windows MinGW and MSVC. Linux success is not Windows
   validation; no compatibility guarantee can be inferred from static review.
2. Broaden hardware, scaling, ill-conditioning and noisy-function testing.
   Flat/small-scaled roots need correctly scaled residual/position evidence;
   adaptive Simpson has a demonstrated sqrt endpoint depth limit.
3. Preserve direct versus different-work comparisons. GSL step doubling and
   adaptive error control cannot be presented as the same work as fixed steps.
4. Retain the root-discovery incompleteness, estimated multiplicity/conditioning
   and memory-measurement scope caveats. Extend allocation-failure coverage to
   other library paths in future work.
5. Rebuild clients against the matching public header/archive. There is no
   stable shared-library ABI or exported installed CMake package config.
6. Existing test CHECK macros can continue into a dereference after an earlier
   failure, and some sentinel tests assume an integer-derived pointer. These
   harness portability warnings remain visible; production scans are clean.

## Reproduce the candidate

```sh
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j2
ctest --test-dir cmake-build-release --output-on-failure
cmake --install cmake-build-release --prefix /tmp/numerix-install
cpack --config cmake-build-release/CPackConfig.cmake -B /tmp/numerix-packages
# After committing, produce a source archive outside the working tree:
git archive --format=tar.gz --prefix=numerix-0.1.0/ \
  --output=/tmp/numerix-0.1.0-source.tar.gz HEAD
```

See [benchmarks/README.md](../../benchmarks/README.md) for optional GSL setup,
allocation probes and complete validation commands. Packaging success is not
an assertion of ABI stability or a replacement for unavailable platform checks.
