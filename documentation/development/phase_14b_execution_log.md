# NUMERIX Phase 14B execution log

This log follows the full authoritative requirements in the repository-root file a.md. Phase 14B is a separate extension; original Phase 14 and Phase 13 data remain immutable.

## Initial repository and result audit

- Worktree: /home/142401034/numerix.
- Branch: master.
- HEAD at Phase 14B start: cbc3d27bb89451f66f938cfe245274f0dccb37b4.
- Project version: 0.2.0-rc1; no version change is allowed.
- Initial status: only the saved Phase 14B request copy and a.md were untracked. Existing Phase 14 outputs are in HEAD and the Phase 13 checksum inventory verified all 125 paths.
- The original Phase 14 primary output remains benchmarks/results/phase14/main: 700 problems, 2,000 pairs, 62,000 timing rows, 2,000 accuracy records, 8,000 work rows, 2,000 RSS probes, and 22 PNG/SVG figure pairs.
- Phase 14’s report remains classified PARTIAL because it measured only one selected primary path per module. Phase 14B creates separate manifests, raw data, tables, figures, and reports under phase14b paths.
- The existing Phase 14 report, execution log, completion report, capability matrix, runner, method map, manifests, analysis scripts, and result CSV schemas were inspected. Public API and source implementations were inspected before writing the initial matrix.
- The complete user brief is preserved at PHASE14B_REQUIREMENTS.md. Its complete text is copied into a.md; only the appended progress tracker is additional.

## Initial method coverage audit

- Initial inventory: 49 core numerical method/operation entries across seven modules: root 9, linear algebra 20, differentiation 7, interpolation 4, integration 4, optimization 2, ODE 3.
- The inventory includes the six scalar root algorithms, auto and portfolio orchestration, root discovery, vector/matrix lifecycle and primitive operations, both system solvers and LU stages, all six scalar differentiation methods, piecewise differentiation, all interpolation construction/evaluation methods, all fixed/adaptive and piecewise integration methods, both optimization APIs, and all three fixed-step ODE methods.
- General diagnostics, expression, managed-execution, and generic context utilities are supporting APIs, not separate core algorithms. Their relationship to the method benchmark is explicitly documented rather than silently counted as algorithm coverage.
- Human-readable audit: documentation/development/phase_14b_method_coverage_matrix.md.
- Machine-readable audit: benchmarks/analysis/phase14b/method_coverage.csv.
- Generator: benchmarks/analysis/phase14b/generate_method_audit.py.
- The existing Phase 14 measured paths are Brent/Newton root, Gaussian dense solve, central first derivative, global Newton interpolation, adaptive Simpson, bounded golden-section, and fixed-step RK4, plus GSL/specialist matches recorded in the old method map. The remaining 42 rows have no method-level Phase 14 measurement.

## Ordered implementation checklist

| Stage | Status | Evidence / next work |
| --- | --- | --- |
| Requirements copied, fully read, and progress tracker added to a.md | Complete | a.md, PHASE14B_REQUIREMENTS.md |
| Repository and historical artifact inspection | Complete | This log; 125 Phase 13 checksums pass |
| Initial method/API coverage audit | Complete | 49-row CSV and Markdown matrix |
| Root method and edge-case expansion | Complete | Final v3 root run, paired analysis, 19-case edge suite, per-root discovery details, method graphs, and invalid-expression probe are under `benchmarks/results/phase14b/`; see the root module record below |
| Linear algebra primitives/factorization/solve | Pending | Consult a.md Module 2 before implementation |
| Differentiation methods and step sweeps | Pending | Consult a.md Module 3 before implementation |
| Interpolation construction/evaluation methods | Pending | Consult a.md Module 4 before implementation |
| Fixed/adaptive and piecewise integration | Pending | Consult a.md Module 5 before implementation |
| Golden-section and gradient descent | Pending | Consult a.md Module 6 before implementation |
| Euler, Heun, RK4 and trajectory-wide evaluation | Pending | Consult a.md Module 7 before implementation |
| Edge-case suite, time-to-accuracy and memory investigation | Pending | Use separate Phase 14B schemas and retain all outcomes |
| Static, interactive HTML and Markdown reports | In progress | Offline report generator now emits all 392 library/method cards and filters; measured values currently populate only root finding. Local paths, anchors, assets, and JavaScript syntax were checked |
| Validation, checksums, final line-by-line audit of a.md | Pending | Must disclose any unfinished acceptance criteria |

## Constraints

- Do not overwrite, delete, or rename Phase 13 or Phase 14 artifacts.
- Do not change production numerical algorithms as part of benchmarking.
- Do not create a commit or push; do not change the project version.
- Distinguish direct algorithm comparisons from best-available methods.
- Preserve failed, inaccurate, rejected, unsupported, and budget-exhausted cases as separate outcome categories.
- Any future library limitation must be marked as unsupported, not assigned a fabricated result.

## Root module implementation entry

- Consulted the authoritative Module 1 requirements in `a.md` before starting this module.
- Plan: use the existing frozen Phase 14 root problem/input IDs, add explicit method applicability and comparison labels, and run each root method through a new Phase 14B-only native adapter. Existing Phase 14 runner and results are read-only.
- The original workloads store one analytical root, but some functions have multiple roots (notably the quadratic and sinusoidal families). The method-level runner will derive the complete in-interval reference root set for discovery scoring instead of treating the single Phase 14 reference value as a complete root inventory.

## Root module results

- Final study: `benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/`; 17 library/method routes, 100 frozen root problems per route, 1,700 accuracy records, and 52,700 timing batches. The route set is NUMERIX 9, GSL 7, and CMINPACK 1; GSL and CMINPACK method mappings are marked direct or best-available in the capability matrix rather than conflated.
- Direct paired analysis: 94 bisection, 95 regula-falsi, 85 secant, 90 Newton, and 95 Brent problem pairs. The GSL/NUMERIX median ratios are 1.0818, 1.0241, 1.0754, 1.0694, and 1.0372, respectively. The 5,000-resample problem-bootstrap intervals are descriptive; no significance claim is made.
- Multiple-root discovery: NUMERIX found 100/100 reference roots with no misses or false positives. The GSL fixed 4,096-panel sign-change scan found 94/100 and missed six. Both are heuristic discovery routes; GSL does not expose an equivalent discovery API, and its unresolved-region and multiplicity fields are not available.
- Edge suite: 19 cases; 322 method/library accuracy outcomes and 6,762 timing batches, plus a separate invalid-expression probe. One CMINPACK discontinuity route is explicitly marked unsupported/not run. Reversed intervals are rejected for both NUMERIX and GSL discovery. Budget exhaustion, discovery cancellation, inaccurate success at a discontinuity, non-finite values, close roots, and floating-point boundaries remain separately recorded.
- Root-only analysis tables are in `benchmarks/results/phase14b/analysis/root/` and `analysis/root_edges/`. Root PNG/SVG graphs are in `benchmarks/results/phase14b/figures/root/`.
- Pilot runs and the pre-reversed-interval edge run were retained in separately named directories and excluded from final-v3 analysis. The discovery-options pilot is not mixed with the final data.
- The root runner compiled with GCC using `-std=c11 -O3 -DNDEBUG -Wall -Wextra -Wpedantic`; GSL 2.8 and CMINPACK 1.3.15 were linked from local dependency prefixes. CMake was unavailable (`command -v cmake` returned no path), so this run used the explicit manual compiler command; `benchmarks/adapters/phase14b/CMakeLists.txt` remains available for reproduction.
- The report generator is `benchmarks/analysis/phase14b/generate_phase14b_reports.py`. It currently links all library/method routes and shows pending status for unmeasured modules. The generated HTML currently has eight library accordions, 49 methods per library, 392 nested method cards, and local filtering/navigation controls. The final report will remain marked PARTIAL until other modules and acceptance requirements are completed.

## Verification checkpoint

- Rebuilt and ran `make -B test`; all Make targets passed with the Makefile's GCC `-O2` and strict warning flags. This includes the seven Phase 1 numerical module categories and the existing Phase 13B regression and benchmark measurement tests.
- Regenerated root method analysis, root edge analysis, and root PNG/SVG charts from the preserved Phase 14B raw data. Counts were 1,700 accuracy rows / 52,700 timing batches / 17 main routes and 322 edge accuracy outcomes / 6,762 edge timing batches / 323 planned mappings.
- Added `benchmarks/analysis/phase14b/verify_phase14b_artifacts.py`; it independently reconstructed the five paired speed ratios and passed its checks for schemas/counts, local HTML references, charts, and historical data.
- Rendered the HTML report in headless Chromium and exercised the filters: all libraries (392 cards), GSL (49), root module (9), Newton (1), reset (392), expand, and collapse. All reported the expected state. Embedded JavaScript syntax passed `node --check`.
- Verified all 125 Phase 13 preservation checksums. The original Phase 14 analysis and result directories remain unchanged.
- Valgrind completed a focused 21-batch NUMERIX Brent smoke with no definite or indirect leak. Clang static analysis reported four dead-store warnings in the included pre-existing Phase 14 parser; no Phase 14B runner warning remains.
- CMake and CTest executables were not available in the current shell. The adapter was manually compiled and linked using GCC 8.5.0, GSL 2.8, and CMINPACK 1.3.15. ASan/UBSan and installed consumer checks were not run.
- Written reports: `documentation/development/phase_14b_completion_report.md` and `documentation/development/phase_14b_verification_report.md`. Both retain PARTIAL overall status because six modules, time-to-accuracy, and method-specific memory work remain pending.
