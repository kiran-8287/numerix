# NUMERIX Phase 14B completion report

**Phase status: PARTIAL.** This report follows the complete specification saved in the repository root at `a.md`. The root-finding module has been benchmarked and analyzed in Phase 14B. The other six numerical modules, time-to-accuracy sweeps, and method-specific memory measurements remain unfinished, so the acceptance criteria for Phase 14B as a whole are not met.

## Work completed

- Preserved the full request in `a.md` and `PHASE14B_REQUIREMENTS.md`; added a Phase 14B tracker to `a.md` and consulted the module requirements during the root implementation.
- Audited 49 public numerical methods and operations across root finding, linear algebra, differentiation, interpolation, integration, optimization, and ODE. The machine-readable audit is `benchmarks/analysis/phase14b/method_coverage.csv`; the readable matrix is `documentation/development/phase_14b_method_coverage_matrix.md`.
- Created a 392-row capability matrix for eight candidate libraries/configurations. It distinguishes algorithm capability, direct versus best-available routes, and Phase 14B measurement status.
- Benchmarked all nine NUMERIX root methods and the available GSL/CMINPACK root routes on 100 frozen Phase 14 root problems. The study retained 1,700 accuracy outcomes and 52,700 timing batches.
- Evaluated 19 numerical root edge cases and one separate invalid-expression API probe. There are 322 measured edge outcomes, 6,762 edge timing batches, and one explicitly unsupported CMINPACK route among 323 planned mappings.
- Generated per-method and study-level root charts in PNG and SVG, an offline expandable HTML dashboard, and a Markdown report.
- Kept the original Phase 13 and Phase 14 datasets untouched. No production numerical algorithm was modified.

## Coverage and comparison counts

- NUMERIX public method/operation inventory: 49 rows; all seven modules represented.
- NUMERIX methods benchmarked at method level in this Phase 14B run: 9 root methods. The remaining 40 audited methods and operations are not yet method-level measured.
- Candidate library/configuration inventory: 8. Root performance was measured for three libraries: NUMERIX, GSL 2.8, and CMINPACK 1.3.15. LAPACK reference, LAPACK/OpenBLAS, NLopt, ARKODE, and CVODE appear in the capability matrix but do not have Phase 14B method-level measurements in this execution.
- Distinct numerical cases: 100 frozen root problems plus 19 root edge cases. The malformed-expression probe is a separate supporting-API check.
- Main-study observations: 1,700 library/method/problem accuracy records across 17 measured library-method routes; 52,700 raw timing batches; 5,100 untimed work-metric rows.
- Direct paired observations: 459 accurate, successful, applicable problem pairs across the five direct methods (94 bisection, 95 regula falsi, 85 secant, 90 Newton, and 95 Brent pairs). There were 500 possible direct method/problem pairs before exclusions.
- Edge-study observations: 322 measured library/method/case outcomes out of 323 planned routes, with 6,762 timing batches. The separate parser probe passed.

## Accuracy, speed, and reliability findings

Accuracy pass counts over each route’s 100 main-study attempts were:

| Method | NUMERIX | GSL | CMINPACK |
|---|---:|---:|---:|
| Bisection | 94/100 | 95/100 | Not supported |
| Regula falsi | 95/100 | 95/100 | Not supported |
| Secant | 100/100 | 85/100 | Not supported |
| Newton | 100/100 | 90/100 | 96/100, best available |
| Safeguarded Newton | 95/100 | 95/100 using a Brent route | Not supported |
| Brent | 95/100 | 95/100 | Not supported |
| Automatic selection | 100/100 | Not applicable | Not applicable |
| Portfolio | 100/100 | Not applicable | Not applicable |
| Multiple-root discovery | 100/100 | 94/100 using a fixed-grid scan | Not applicable |

The direct timing ratio is median GSL call time divided by median NUMERIX call time over successful, accurate, applicable paired problems; values above one favor NUMERIX. The descriptive ratios and problem-bootstrap intervals were: bisection 1.0818 [1.0702, 1.1020], regula falsi 1.0241 [0.9871, 1.0359], secant 1.0754 [1.0630, 1.0921], Newton 1.0694 [1.0590, 1.0846], and Brent 1.0372 [1.0226, 1.0600]. These are exploratory summaries, not significance claims or a library-wide ranking.

Successful status does not ensure an accurate answer: GSL reported successful status for all 100 Newton and secant attempts but passed the location/residual accuracy gate on 90 and 85, respectively. Root discovery is heuristic. NUMERIX detected all 100 reference roots in this frozen workload; the 4,096-panel GSL sign-change scan detected 94 and missed six. The scan has no equivalent completeness, tangency, multiplicity, or unresolved-region API.

Edge cases retain correct input rejection, numerical failures, cancellation, iteration/evaluation limits, unsupported routes, inaccurate successful results, non-finite evaluations, tangencies, close roots, and floating-point boundaries as distinct records. The discovery cancellation and evaluation-cap tests are not represented as directly equivalent GSL behavior where GSL lacks those controls.

## Limitations and uncompleted work

- Linear algebra, differentiation, interpolation, integration, optimization, and ODE require method-level workloads, measurements, edge suites, analysis, and figures.
- Time-to-accuracy was not evaluated because no tolerance sweeps were run.
- Method-specific heap/allocation interception was not completed. Historical process RSS includes harness and process baselines and is not suitable evidence of per-method library memory use.
- No CMake/CTest run was possible because `cmake` and `ctest` were unavailable in the active shell. The Phase 14B adapter has a CMake configuration and was compiled manually with GCC 8.5.0 and pinned local dependencies.
- This partial execution is not a stable-release readiness verdict. It does not support an overall library ranking.

## Important artifacts

- Requirements and tracker: `a.md`, `PHASE14B_REQUIREMENTS.md`.
- Execution log: `documentation/development/phase_14b_execution_log.md`.
- Verification report: `documentation/development/phase_14b_verification_report.md`.
- Method coverage: `benchmarks/analysis/phase14b/method_coverage.csv`, `documentation/development/phase_14b_method_coverage_matrix.md`.
- Library capability matrix: `benchmarks/results/phase14b/manifests/library_method_capabilities.csv`, `documentation/development/phase_14b_library_capability_matrix.md`.
- Interactive dashboard: `documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.html`.
- Markdown companion: `documentation/reports/NUMERIX_phase14b_method_level_comparative_evaluation.md`.
- Main root raw data: `benchmarks/results/phase14b/raw/root-method-study-final-v3-20261112/`.
- Main root analysis: `benchmarks/results/phase14b/analysis/root/`.
- Edge data and analysis: `benchmarks/results/phase14b/edge_cases/root/`, `benchmarks/results/phase14b/analysis/root_edges/`.
- Root PNG/SVG figures: `benchmarks/results/phase14b/figures/root/`.
- Repeatable artifact verifier and result: `benchmarks/analysis/phase14b/verify_phase14b_artifacts.py`, `benchmarks/results/phase14b/verification/phase14b_artifact_verification.json`.

## Project state

The project version remains `0.2.0-rc1`. No commit or push was made. Phase 14B remains **PARTIAL** until the remaining modules and outstanding measurement requirements are completed and rechecked against `a.md`.
