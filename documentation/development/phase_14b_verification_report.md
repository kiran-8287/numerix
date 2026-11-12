# NUMERIX Phase 14B verification report

**Verification scope:** completed root-finding module and its generated artifacts. This does not verify the six pending numerical modules as benchmarks. Phase status remains **PARTIAL**.

## Results

| Check | Result |
|---|---|
| Requirements source | `a.md` exists at the repository root and contains the complete Phase 14B specification plus the execution tracker. |
| Method audit | 49 unique method/operation rows across seven modules; 392 unique library/module/method capability rows. |
| Main root study | 1,700 accuracy rows, 52,700 timing batches, 17 observed library-method routes, 100 frozen problems per route, and 5,100 work-metric rows. |
| Main root analysis | Regenerated from saved raw records. Five paired method ratios were independently reconstructed from per-problem medians and matched the report table. |
| Edge study | 19 numerical edge cases plus one supporting expression probe; 322 accuracy outcomes, 6,762 timing batches, 323 planned method/case routes, and one unsupported route retained as not run. |
| Figures | Regenerated 28 root PNG/SVG assets: five study-level charts and nine method-level charts, each in PNG and SVG. |
| Offline report | HTML parsed successfully; 8 library accordions and 392 method cards; 84 local links/assets checked with no missing paths or anchors. Node accepted the embedded JavaScript syntax. |
| Browser interaction | Chromium headless loaded the `file://` report. Initial count was 392; choosing GSL showed 49; adding root showed 9; choosing Newton showed 1; reset restored 392; expand and collapse controls worked. |
| Repository test suite | `make -B test` rebuilt the library and test executables with GCC `-O2 -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow`; all Make test targets passed, including all seven Phase 1 module categories, expression, piecewise, intelligence, execution, diagnostics, root discovery, Phase 8 integration, Phase 13B regression, accuracy, release regression, and benchmark measurement checks. |
| Phase 13 preservation | All 125 entries in `benchmarks/results/phase14/logs/phase13_preservation.sha256` matched. |
| Phase 14 preservation | No changes under `benchmarks/results/phase14/` or `benchmarks/analysis/phase14/` were present in the worktree. |
| Valgrind | A 21-batch NUMERIX Brent smoke run completed with `--leak-check=full`; Valgrind reported no definite or indirect leaks and exited successfully. This was a focused adapter smoke check, not a full leak run of the entire benchmark. |
| Clang static analysis | Completed on the root adapter translation unit. Four `deadcode.DeadStores` warnings occur in the included existing Phase 14 parser's `parse_stage` assignments. The Phase 14B adapter introduced no remaining analyzer warning after removing one unused initialization. |
| CMake/CTest | Not run: `cmake` and `ctest` executables were unavailable in the active shell. The Phase 14B CMake configuration exists; the adapter was manually compiled and linked. |
| Sanitizers / installed consumers | ASan/UBSan and installed C/C++ consumer checks were not run in this Phase 14B verification. |
| Version / Git | Version remains `0.2.0-rc1`; no commit or push was made. |

## Reproduction commands

The analysis and report can be regenerated with:

```sh
python3 benchmarks/analysis/phase14b/analyze_root_methods.py
python3 benchmarks/analysis/phase14b/analyze_root_edges.py
python3 benchmarks/analysis/phase14b/plot_root_methods.py
python3 benchmarks/analysis/phase14b/generate_phase14b_reports.py
python3 benchmarks/analysis/phase14b/verify_phase14b_artifacts.py
```

The automated artifact verifier writes `benchmarks/results/phase14b/verification/phase14b_artifact_verification.json`. Its final status is `PASS`; it checks method/capability counts, CSV uniqueness and row counts, timing batches, paired timing reconstruction, edge coverage, expression probe status, figure files, dashboard links/controls, Phase 13 checksums, and Phase 14 worktree preservation.

The root adapter's manual optimized compile used GCC 8.5.0, C11, `-O3 -DNDEBUG -Wall -Wextra -Wpedantic`, the Phase 14 adapter static NUMERIX library, GSL 2.8, and CMINPACK 1.3.15 from local prefixes. No CMake-based build result is claimed.

## Outstanding validation

The available Make suite and completed root artifact checks pass. The remaining module-specific benchmark manifests, schemas, raw data, accuracy/work metrics, paired comparisons, edge suites, and graphs do not exist yet. Full Phase 14B acceptance, time-to-accuracy, method-scoped memory validation, CMake/CTest, and installed C/C++ consumer checks remain pending.
