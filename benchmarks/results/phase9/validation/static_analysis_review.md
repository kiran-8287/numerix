# Static analysis review

- GCC 15.2.1 `-fanalyzer`, all 25 library translation units: completed, no final
  diagnostics. Compile commands, warnings and exits are retained in results.json
  and gcc_analyzer_*.log. GCC 8.5 lacks this analyzer, so a locally available GCC
  toolset was used for analysis only.
- Clang 21.1.8 analyzer, all 25 library translation units: completed, no final
  diagnostics. Extra scans of the benchmark, allocation tracker and CLI are
  retained separately. Per-file analysis does not prove the absence of defects.
- Cppcheck 2.18.3 built locally from its tagged upstream source. Final exhaustive
  warning/performance/portability scan covers library, benchmarks, CLI and all
  five examples, with exit0 and no diagnostics (cppcheck_production.log).
  The initial normal-level scan emitted branch-limiting information and returned
  a nonzero exit; it was rerun with exhaustive analysis, not suppressed.
- A supplemental scan including existing tests reported three test-harness
  null-check/dereference warnings and two intentional integer-to-pointer sentinel
  portability warnings. CHECK macros record a failure and continue; subsequent
  dereferences can indeed crash if the preceding library operation fails.
  The sentinels deliberately test clearing non-NULL output handles. These are
  existing test-harness limitations, not production-source findings. No warning
  suppression was added. The supplemental scan is **partially passed** and its
  original diagnostics remain in cppcheck_tests_review.log.

Concrete changes prompted by review include the NULL LU input crash, consistent
status propagation when error=NULL, invalid LU permutation/output checks,
explicit known-status error helper returns, initialized root-attempt/boundary
failure evidence, an explicit floating-point nonzero test and removal of an
unused optimizer assignment. These preserve numerical recurrences. ASan/UBSan,
Valgrind and selected allocation-fault tests supply separate dynamic evidence.
No static-analysis success is claimed for unavailable MSVC tooling.
