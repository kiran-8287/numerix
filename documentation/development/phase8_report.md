# Phase 8 validation report

## A. Implementation summary

Root discovery no longer allocates both result arrays at the configured
maximum candidate count before searching. Root and unresolved-interval arrays
grow geometrically as entries are recorded, starting at up to four entries
and never exceeding `maximum_candidates`. Allocation-size checks guard each
growth, and a failed growth preserves already stored results while ending the
search with an allocation failure.

The public root-search report now exposes actual array capacities, retained
result-array bytes, successful allocations for each result array, peak bytes
across directly owned search buffers, and the count of successful direct
buffer allocations. These metrics exclude nested diagnostic allocations,
execution-context and solver internals, allocator metadata, and process
resident memory. The public report struct has grown; precompiled consumers
must be rebuilt. Function signatures and the numerical search strategy are
unchanged; callers must still use the documented result counts to check
whether result pointers are available.

The all-zero sampled-function path now destroys temporary root diagnostics and
releases the root result array before returning its unresolved-region report.
The root-discovery benchmark reports these allocation metrics and averages
process CPU time over repeated searches. A new integration test covers
expression discovery, managed recommendation and execution, diagnostics, and
piecewise boundary analysis with root discovery.

## B. Reliability improvements

- Result storage is bounded by observed roots and unresolved intervals rather
  than eagerly reserving the full configured capacity.
- Capacity growth and byte-size calculations are checked before allocation.
- Allocation failure retains earlier results and reports a failure state.
- Discarded all-zero candidate roots have their diagnostic resources released.
- Regression checks cover empty results, geometric result storage, bounded
  capacities, and piecewise boundary roots.

## C. Portability results

Validation was performed on Windows using MinGW.org GCC 6.3.0 and GNU Make
3.82.90. Both optimized and Debug Make builds compiled the library, CLI,
examples, and tests with the project's warning flags plus `-Werror`.

CMake, Linux GCC/Clang, Windows Clang, and MSVC were unavailable in this
environment. The CMake source and test registrations were reviewed and updated
for the new integration test, but no CMake configure, build, or CTest run is
claimed for this phase.

## D. Testing results

Passed on the available Windows MinGW toolchain:

- Clean optimized build and complete test suite using `CFLAGS="-O2 -Werror"`.
- Clean Debug build and complete test suite using `CFLAGS="-O0 -g3 -Werror"`.
- Existing Phase 0 and Phase 1 checks, expression engine (3,138 checks), audit
  regressions, piecewise, intelligence, managed execution, diagnostics, and
  root discovery suites.
- New Phase 8 cross-phase integration executable.
- CLI startup, expression example, and root-discovery example.
- Root-discovery and diagnostics benchmarks.
- `git diff --check`.

AddressSanitizer and UndefinedBehaviorSanitizer could not be linked: the
MinGW GCC installation reports missing `-lasan` and `-lubsan`. GCC 6.3 rejects
`-fanalyzer`; Clang, Cppcheck, and Valgrind were not installed. No sanitizer or
static-analysis success is claimed.

## E. Performance and memory results

The Phase 7 baseline eagerly reserved 1,910,784 bytes for the two default
result arrays on every search. In the Phase 8 benchmark, a no-root smooth
search stores 0 result-array bytes. At 16 panels, its measured peak among
directly owned search buffers is 544 bytes; the corresponding peaks are 1,056
bytes at 32 panels and 2,080 bytes at 64 panels.

A one-root result retains 29,760 bytes of root-array capacity and peaks at
30,304 direct workspace bytes at 16 panels. This is a 98.4% reduction in
retained result-array storage compared with the previous 1,910,784-byte
reservation. Cases storing unresolved intervals also allocate only the
geometrically grown interval array; for example, the 16-panel pole case
retains 96 result bytes and peaks at 640 direct workspace bytes.

Root benchmark cases were repeated 1,000 times. Function-evaluation counts at
16, 32, and 64 panels match the Phase 7 baseline, so the storage change did not
alter the measured search work. Representative average process CPU times at
16 panels were 6 microseconds for one root, 68 microseconds for three roots,
and 6 microseconds for no roots. Values are local timing observations from
`clock()` and are not portable performance guarantees.

The existing diagnostics benchmark was run with 10,000 repetitions per mode.
Representative disabled/basic/full timings in milliseconds per call were:

| Workload | Disabled | Basic diagnostics | Full configured diagnostics |
| --- | ---: | ---: | ---: |
| Root finding | 0.00710 | 0.00740 | 0.00760 |
| Linear solve, 8×8 | 0.00570 | 0.00650 | 0.01070 |
| Linear solve, 32×32 | 0.03410 | 0.04190 | 0.14280 |
| Integration | 0.00850 | 0.03370 | 0.04840 |
| Differentiation | 0.00770 | 0.00770 | 0.00860 |
| Optimization | 0.00630 | 0.00660 | 0.00700 |
| ODE RK4 | 0.01710 | 0.05960 | 0.06160 |

These timing results use process CPU time on one Windows machine. No GSL or
other external-library comparison was performed.

## F. Remaining limitations

- Linux, CMake, MSVC, and Clang builds still need validation in environments
  where those tools are available.
- Sanitizer execution and dedicated static analysis remain unverified because
  the available toolchain lacks the required runtimes and analyzers.
- Allocation-failure branches were reviewed and size checks were compiled,
  but the repository has no allocator fault-injection facility for exercising
  each failure point deterministically.
- Workspace metrics deliberately exclude nested diagnostic and solver/context
  allocations, allocator overhead, and process RSS.
- Root discovery remains heuristic and cannot prove that all roots were
  located.

## G. Git status

The implementation was based on the Phase 7 commit on `master`. The phase
commit and push result are recorded in the completion message after repository
verification.

## H. Phase 8 assessment

**Substantially complete with documented limitations.** The implemented
reliability changes, Windows MinGW Release and Debug builds, full test suites,
cross-phase workflows, CLI/examples, and repeated internal benchmarks all
passed. Cross-platform builds, sanitizer runs, and external static analyzers
could not be performed in the available environment.
