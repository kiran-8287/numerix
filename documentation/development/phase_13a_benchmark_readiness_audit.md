# Phase 13A — Benchmark readiness and numerical correctness audit

Date: 2026-11-12. Audited release: **NUMERIX 0.2.0-rc1**. Decision: **NOT READY for the proposed comprehensive multi-library benchmark yet**. The library has a substantial, useful test and validation history. Source inspection nevertheless found confirmed edge cases where finite, currently accepted inputs can produce an incorrect result or status, and the existing benchmark output has measurement gaps that matter for the proposed wider comparison. Fix or explicitly exclude those cases, strengthen the measurement record, and then benchmark the defined supported domain.

This is an inspection report. It does not implement the recommendations or start Phase 13B. The repository’s previously recorded builds, tests, static analysis, and benchmark outputs were inspected; this audit did not rerun the suite or overwrite historical data. Source-level cases below are labeled confirmed when the implementation path establishes the outcome; they were not separately compiled as new regression tests during this audit.

## 1. Executive summary

Phases 0–12 left NUMERIX with seven numerical categories, callback and compiled-expression interfaces, a deterministic recommendation layer, managed execution, qualified diagnostics, heuristic root discovery, optional GSL 2.8 comparison infrastructure, compiler validation, and a reproducible static distribution. The current release candidate is a clean, pushed `master` at commit `6c2ef314bbe0b563dbf2353dd70a3b1ee2748574`. Local HEAD and `origin/master` match. Its version is `0.2.0-rc1` in the public header and release metadata; CMake uses numeric version `0.2.0` for package matching.

The existing Phase 9 benchmark is a useful, reproducible API-level study, but its 222 CSV rows are **not 222 distinct mathematical problems**. They include library-specific observations, methods, factor/solve or construct/evaluate phases, and 21 repeated timing summaries per row. The raw data has 41 case names, 61 category/case/size keys (45 core numerical case/size entries and 16 NUMERIX capability entries), 97 method keys, and 128 method/phase keys before library rows are counted. Repeated rows reuse the same deterministic input; they are not new problem samples.

Confirmed benchmark-readiness blockers are: a mislabeled precision field; fixed NUMERIX-before-GSL order; no retained individual timing samples or maximum; and code paths in pivoting, finite differences, linear interpolation, ODE coordinate stepping, and narrow-bracket bisection that need correction or an explicit exclusion policy before those cases are benchmarked. The current benchmark does not exercise most of these edge cases. The 10 remaining nonzero statuses and 13 successful-call accuracy misses in Phase 10 are retained in the data and documented; they are distinct from the newly identified source-level cases.

**Verdict: NOT READY.** Correct the confirmed numerical and measurement issues listed in section 14, or define and enforce a narrower benchmark domain that excludes affected inputs with transparent records. Then run the expanded audit suite before timing additional libraries.

## 2. Repository and release-candidate status

| Check | Observed state | Evidence |
|---|---|---|
| Branch and worktree | `master`, clean before this report | `git status -sb` reported `## master...origin/master` with no entries |
| HEAD / remote | Both `6c2ef314bbe0b563dbf2353dd70a3b1ee2748574` | `git rev-parse HEAD` and `git ls-remote origin refs/heads/master` |
| Version | Header/release string `0.2.0-rc1`; CMake package version `0.2.0` | `include/numerix.h`, `CMakeLists.txt` |
| Public interface | One public header; 126 declared public functions have archive definitions | `include/numerix.h`, `benchmarks/results/phase12/public_symbols.txt` |
| Production sources | 25 C translation units; Make/CMake lists previously mechanically matched | `src/`, Phase 12 report and validation logs |
| Tests and examples | 15 test C files, five example C files; current CTest registration reports 33 tests, 34 with optional allocation-failure test | `tests/`, `examples/`, `CMakeLists.txt`, Phase 12 validation logs |
| Benchmark infrastructure | Phase 9 GSL/NUMERIX harness plus discovery and diagnostics benchmarks; Phase 10 accuracy rerun | `benchmarks/`, `scripts/`, `benchmarks/results/phase9/` and `phase10/` |
| Build/install | C11, Make and CMake; static library; relocatable CMake and pkg-config packages | `Makefile`, `CMakeLists.txt`, `cmake/`, Phase 12 consumer logs |
| Local release archives | Source TGZ 168,207 bytes; Linux x86_64 TGZ 164,068 bytes. These are local, ignored `dist/` outputs, not GitHub release assets. | `dist/`, `benchmarks/results/phase12/artifacts.json` |

Source archive SHA256: `eec819dec4ac7c569522fa06fae0c00089c1d2563a36279daa22a2df2e3fc067`. Linux binary archive SHA256: `36ffc610d13683335e14ed7794ac322fad51b6a02d6365e2d9254d358ff531aa`. Both sidecars and the artifact manifest were checked against the files during the preceding Phase 12 verification. There is no stable ABI or shared-library promise.

This new report is the sole intended project artifact for Phase 13A. The pre-report worktree was clean. No production source, header, build file, benchmark, test, prior report, version, or release artifact was changed in this audit.

## 3. Existing architecture

NUMERIX is a C11 static library with a single public header. Its 25 production translation units are grouped into core/platform support; seven numerical categories; an expression lexer/parser/evaluator; piecewise functions; algorithm recommendations and execution; managed execution; diagnostics; and root discovery. Make and CMake build the same sources. GSL is optional and used only by the comparison executable.

Callers supply borrowed callbacks, data, vectors, matrices, expressions, or piecewise objects. Algorithms return inline scalar results or owned specialized outputs. Matching destroy routines release owned vectors, matrices, LU data, interpolation objects, ODE histories, diagnostic reports, execution outputs, and discovery reports. The benchmark and API review must retain these distinctions: input/setup allocation is sometimes outside a timer, per-call allocation often is inside, and some workflows measure an entire orchestration layer rather than a numerical kernel.

## 4. Numerical correctness audit across the seven categories

### Root finding

The six direct methods are bisection, regula falsi, secant, Newton, safeguarded Newton, and Brent-style safeguarded interpolation. Bracket methods require finite ordered endpoints and a computed sign change, or an exact endpoint zero. Their sign-change guarantee assumes continuity; sign-changing discontinuities and poles can still mislead a solver. Open Newton/secant methods now avoid stopping on a small nonzero residual and use observed step contraction plus decreasing residual as **heuristic** position evidence. They cannot certify a root or a forward-error bound. A zero derivative, degenerate secant denominator, stagnation, non-finite callback, and exhausted budgets have separate outcomes.

Phase 10 addressed the confirmed flat/scaled root error. The current tolerance guide separates position units from function-residual units. The portfolio and fallback code reuse those root results. Root discovery uses finite sampling and explicit residual/merge controls; `search_inconclusive` is intentionally always true. Tangential candidates use minimization of `|f|`; unresolved intervals, piecewise boundaries, poles, merging, and optional multiplicity estimates are tracked. Discovery and multiplicity are not completeness proofs.

Remaining audit point: a bisection interval that already meets position tolerance still evaluates its midpoint before returning success. If the two endpoint checks consumed the configured evaluation budget, the solver reports `LIMIT_REACHED` despite already having a position-qualified midpoint. See finding ROOT-01. No Phase 10 test is identified in the source/report inventory for this exact narrow-bracket/two-evaluation case.

### Linear algebra

The implementation is **dense-only**. It provides dense vector/matrix arithmetic, partial-pivoted LU, Gaussian solve through LU, and a separate partial-pivoted Gauss–Jordan solve. There are no sparse or specialized symmetric solvers. Matrix operations reject non-finite inputs and many non-finite results. The tests and benchmark cover several dense sizes and scales, pivoting, and Hilbert stress cases; they do not comprehensively cover subnormal scaling or factorization intermediates near the binary64 maximum.

Two confirmed extremes need attention. The pivot threshold has an absolute `DBL_MIN` floor, so a well-conditioned 1×1 matrix `[1e-310]` is classified as numerically singular although solving `[1e-310] x = [1e-310]` has solution `x=1`. Conversely, LU elimination can create an infinite factor from finite input and still return success because factor updates are not checked for finiteness. See LA-01 and LA-02.

The linear diagnostics compute residuals, relative residuals, normwise backward error, and an optional infinity-norm condition estimate by factoring and solving against coordinate vectors. The condition estimate is capped at 256 rows. It is an estimate, not a certified condition bound. A small residual/backward error does not guarantee small forward error for an ill-conditioned system. Hilbert stress rows demonstrate this distinction. Condition estimates can also be unavailable when the library’s pivot threshold rejects a matrix.

### Differentiation

Forward, backward, central, five-point central, Richardson, and central second differences are implemented. The caller supplies a positive finite step; the library does not select it from `x`, scale, curvature, or callback noise. Richardson/five-point corrections are estimates. Other methods mark their error estimate unavailable in the API documentation.

The stencil validates that sample coordinates are finite, but not that `x+h`, `x-h`, or higher stencil points are distinct from `x` and from one another in floating-point arithmetic. For example, at `x=1e16` with `h=0.25`, binary64 rounds both `x+h` and `x-h` to `x`; a central difference can report a zero derivative with success for `f(x)=x`. See DIFF-01. Existing tests exercise analytic functions near `x=0.7`, not this representational-spacing limit.

### Interpolation

The API offers two-point linear interpolation, direct Lagrange evaluation, and reusable Newton divided differences evaluated by nested multiplication. Duplicate abscissas and non-finite inputs are rejected. Extrapolation must be requested explicitly for polynomial objects. High-degree and uneven-node instability is documented; no interpolation error estimate is claimed.

The linear formula uses unscaled differences. For finite valid inputs `x0=-1e308`, `x1=1e308`, `y0=-1e308`, `y1=1e308`, and `x=0`, intermediate differences overflow and the expression encounters `infinity * 0`; the exact interpolated value is zero, but the implementation reports numerical failure. See INTERP-01. The benchmark’s eight-node cubic cases do not exercise this range.

### Integration

Fixed trapezoid and composite Simpson use caller-selected positive panel counts (even for Simpson), compensated accumulation, and extended precision where available. Their tolerance arguments do not refine the grid; their error estimate is unavailable. Adaptive Simpson uses the same Simpson/Richardson rule with a max-error heap, global estimated-error budget, roundoff floor, and explicit depth/evaluation/split failure statuses. The Phase 10 square-root endpoint regression passes at depth 32 and requested `1e-10` tolerance, with recorded actual error `4.18e-13` and 829 evaluations.

The adaptive estimate is conservative on tested problems but **heuristic**, not a rigorous bound. Unsampled/aliased oscillations, narrow peaks, and arbitrary callbacks can fool finite sampling. Infinite endpoint values such as `1/sqrt(x)` at zero fail; improper-integral transformations are not implemented. Piecewise integration splits at known boundaries but does not discover arbitrary singularities. No new quadrature algorithm is indicated as a Phase 13A defect; the benchmark must keep actual error and estimated error as separate columns and retain limit failures.

### Optimization

Golden section is a bounded one-dimensional method and relies on a unimodal objective over its interval for the standard guarantee. Gradient descent is a vector method using a caller-supplied gradient and Armijo backtracking; it is local and can stop when no decrease is found. Neither method promises a global optimum for a general non-convex or multimodal objective. The comparison benchmark covers four one-dimensional golden-section cases. It does not benchmark gradient descent, dimensional scaling, gradient accuracy, or non-convex multimodal cases.

Golden section reports half the final interval width while returning one of its interior samples. That quantity is an estimate, not a demonstrated error bound for the returned sample; diagnostics and future comparison reports should not present it as actual position error. The separate analytical benchmark check remains the appropriate measured error for known minima.

### ODE solvers

Euler, Heun, and classical RK4 integrate vector systems on a caller-selected fixed signed step and return every requested point. They do not adapt the step or estimate local error. No stiffness detection or stiff solver exists. Euler/Heun/RK4 can be unstable or inaccurate for stiff systems, large steps, and long intervals.

The implementation checks that each computed `x+h` is finite but not that it differs from `x`. For a nonzero step below the representable spacing at a large `x`, it can return success with repeated time coordinates while advancing state using the nominal step. RK4 can also use midpoint abscissas that round back to the current time. See ODE-01. The current tests and benchmark use moderate coordinates and `h=0.01`, so this does not explain a recorded Phase 9 failure.

## 5. Previously identified accuracy issues

The raw CSVs were inspected rather than relying only on report totals.

| Dataset | Rows | Nonzero status rows | Successful calls failing the defined accuracy check | Interpretation |
|---|---:|---:|---:|---|
| Phase 9 | 222 | 11 | 21 | Four no-sign-change bracket rejections (two libraries × two bracket methods); six NUMERIX Hilbert factor/total rejections; one NUMERIX adaptive-Simpson square-root endpoint failure; plus 21 successful-call accuracy misses |
| Phase 10 | 222 | 10 | 13 | No-sign-change and Hilbert rejections remain; the square-root endpoint status failure is gone; 13 successful-call misses remain |

There are **32** rows with `accuracy_pass=0` in Phase 9 and **23** in Phase 10. Subtracting the nonzero-status rows gives the documented 21 and 13 successful-call misses. Rows marked `-1` are not applicable (for example factor-only and construction-only phases), not passes. This distinction agrees with the reports; the CSV counts make it explicit.

The no-bracket case is `x^2+1` on `[-1,1]`: both endpoints have the same sign, so bracket rejection is expected. It should remain as an expected-failure case, separated from numerical algorithm failures and excluded from speed ratios.

Hilbert matrices are nonsingular mathematically but severely ill-conditioned. NUMERIX rejects six large-dimension factor/total cases under its numerical pivot threshold. That is a conservative solver outcome, not evidence that the mathematical matrices are singular. GSL returns success for several such rows with very large forward errors. Additionally, benchmark setup forms `b` by summing matrix entries in double precision while the stated reference is the all-ones vector. The rounded `b` is not the exact row sum; ill-conditioning amplifies this perturbation. Keep these stress cases, but label input perturbation and distinguish solver forward error from the change in the reference problem. Use a high-precision RHS construction/reference for a cleaner condition study.

Phase 9’s square-root adaptive Simpson failure was a genuine limitation that Phase 10 corrected: the raw Phase 9 row reports NUMERIX convergence status 6 and no valid result; the Phase 10 rerun records success with the measured error above. Fixed 1024-panel trapezoid/Simpson misses are expected discretization errors against the benchmark’s tight check, not false status successes; they should remain as fixed-grid accuracy results and not be ranked as tolerance-controlled adaptive methods.

The flat-cubic and small-scale root misses in Phase 9 were genuine stopping defects and the Phase 10 position-error checks pass the successful valid root rows. Phase 10’s GSL driver also changed its stopping evidence, so Phase 9-to-10 timing changes are not a clean performance-only before/after. References use double-precision analytical constants. They are adequate for the current broad tolerances; Hilbert stress requires higher-precision construction/reference because of conditioning.

No Phase 9 or Phase 10 failure row was found to have been deleted from its raw CSV. `verify_benchmark_results.py` preserves and classifies the expected status cases. The Phase 9 report and Phase 10 accuracy report explain the changes. The additional edge cases in section 4 were not part of these historical rows and cannot be described as reproduced Phase 9/10 failures.

## 6. Diagnostics and reliability assessment

| Metric | What the code computes | Meaning / limitation |
|---|---|---|
| Root residual | Fresh absolute callback residual at returned root | Function units; does not establish position error without conditioning/structure |
| Linear residual | `b-Ax`, infinity norm, optional relative residual | Genuine computed residual, subject to finite/double range |
| Normwise backward error | Residual normalized by matrix, solution, and RHS norms | Backward-error evidence, not forward error |
| Linear condition | LU-based estimate of `||A||∞ ||A⁻¹||∞`, at most 256 rows | Estimated; can be unavailable when LU rejects the matrix |
| Forward error | Difference from caller-supplied scalar/vector/matrix reference | Actual relative to supplied reference; reference quality is the caller’s responsibility |
| Scalar/multivariable sensitivity | Finite-difference derivative or gradient estimates | Step- and smoothness-dependent; not a certified sensitivity bound |
| Error propagation | Independent-input, local-linearized uncertainty propagation | Omits correlations and large nonlinear perturbations |
| Convergence order/trend | Residual history analysis | Heuristic; residuals act as error proxies |
| Integration/differentiation error | Reuses method estimate when supplied | Estimated, not actual error; unavailable values should remain unavailable |
| Optimization / ODE | Objective/gradient evidence or stored-state summary | No local-error estimate for fixed-step ODE; no universal optimizer error guarantee |
| Discovery/multiplicity/boundaries | Finite samples, candidate checks, ratio and one-sided estimates | Heuristic evidence; no symbolic proof or completeness guarantee |

Reports use `has_*` flags and `NAN` for unavailable metrics in current diagnostic APIs. Structured warnings are bounded and identify estimated/heuristic evidence. One API-level concern remains: successful finite-difference results set the generic `numerix_result.residual` field to `0`, although a derivative has no residual. See DIAG-01. This can be misread by generic consumers as measured zero error; the differentiation guide documents derivative value and estimated error, not a zero residual.

Do not compare residual, estimated error, function evaluations, and solver status as interchangeable success measures. External APIs differ in whether initialization and final checks call the function again. Compare actual error against a common reference where possible, and retain estimates in separate fields.

## 7. Existing benchmark infrastructure

The central implementation is [benchmarks/comparison.c](../../benchmarks/comparison.c), with workload definitions/instructions in [benchmarks/README.md](../../benchmarks/README.md), environment capture in [scripts/run_benchmarks.py](../../scripts/run_benchmarks.py), summary generation in [scripts/summarize_benchmarks.py](../../scripts/summarize_benchmarks.py), and integrity checks in [scripts/verify_benchmark_results.py](../../scripts/verify_benchmark_results.py). The optional allocation wrapper is [benchmarks/allocation_tracker.c](../../benchmarks/allocation_tracker.c); root discovery has a separate focused benchmark.

The Phase 9 `measurements.csv` and `allocations.csv` each have 222 keyed rows. The measurement rows comprise eight categories: roots 46; linear algebra 88; differentiation 8; integration 24; interpolation 20; optimization 8; ODEs 12; and NUMERIX-only capabilities 16. There are 41 distinct case labels. Using category + case + size as the definition of one input configuration gives 61 keys, including the panel-size variants in discovery; excluding the 16 capability keys leaves 45 core numerical category/case/size configurations: roots 8, linear systems 16, differentiation 4, integration 6, interpolation 4, optimization 4, and ODEs 3. Methods and phases expand that to 97 category/case/size/method keys and 128 keys after factor/solve and construct/evaluate phases. Library rows and some NUMERIX-only rows produce 222 observations.

There are 19 distinct strings in the CSV `method` column and 20 category/method assignments: 15 numerical category/method assignments plus five NUMERIX capability workflows. The `newton` label is reused for root solving and Newton interpolation. Method assignments include only the variants listed below, not every public algorithm. The 16 capability entries are four discovery equations at three panel counts each, plus single expression, automatic execution, managed execution, and diagnostics workloads. These counts identify input configurations and method coverage, not 61 or 45 unrelated formulae. Scale variants reuse formulae; roots, derivatives, and integrals also reuse functions across different task categories. The CSV and README do not define a formula-equivalence count, so it would be misleading to claim a count of unique analytic expressions as though it were a measured field.

Each row has `trials=21`: 21 **independent untimed result/accuracy probes**, one after each timed batch, for the same deterministic input. Timing uses a separate warm-up and powers-of-two batch calibration to at least 2 ms, capped at 1,048,576 operations. It then measures 21 batches. CSV min/median/mean/sample standard deviation/p95 are batch means per operation. P95 is a percentile of 21 batch means, not single-call latency. The individual 21 timing samples are not retained. There is no maximum statistic in the CSV.

Input creation and persistent library workspaces are outside timing. Per-call checks, allocation, copying, reset, and destruction are generally inside. LU factor/solve/total and interpolation construct/evaluate are split into phases. This measures public API cost, not an isolated arithmetic kernel. Callback and derivative counts are collected by wrappers; they are not available uniformly for all work counts. Failed statuses remain in the rows, and direct ratios are emitted only for selected comparable, successful, accurate pairs. `comparisons.csv` contains 94 paired rows: 59 eligible for the direct ratio, 35 excluded. The exclusions include non-equivalent work, failure, or failed accuracy. No aggregate winner is reported.

The comparison set includes 8 root labels with bisection/Brent/Newton where applicable; LU for four matrix families at sizes 8, 32, 64, and 128 with factor/solve/total phases; four differentiation functions; six integrals with adaptive NUMERIX/GSL and NUMERIX-only 1024-panel rules; cubic interpolation data at eight nodes plus a two-node linear case; four scalar minimum cases; and three ODE systems at 100 steps. The 16 capability rows cover expression evaluation, automatic/managed/diagnostic execution, and four discovery problems at three panel counts. Difficulty labels are 1–4. These are compact regression/baseline workloads, not broad scaling sweeps.

Environment JSON records OS/kernel, CPU report, RAM, affinity/load, compiler/version, binary/version strings, git head/status, flags, build/link files, binary/source hashes, sample count, and timer description. The historical environment capture records a dirty pre-commit source state and local absolute paths; it is valuable evidence, not a portable replay bundle. The problem definitions live in C and README prose rather than a machine-readable benchmark configuration file.

## 8. Measurement coverage matrix

| Measurement | Coverage | Audit result |
|---|---|---|
| Wall-clock time | Available and verified | Monotonic clock on POSIX; QueryPerformanceCounter path exists on Windows but current Windows benchmark execution is unvalidated |
| CPU time | Missing in central comparison | Central rows use elapsed wall time; separate diagnostics/root-discovery timings use different timers/workloads and are not equivalent CPU-time rows |
| Median, mean, min, sample SD, p95 | Available and verified | Computed over 21 batch means |
| Maximum and individual timing samples | Missing | Only summaries are stored; exact historical spread/max cannot be reconstructed from CSV |
| Speedup ratios | Partial | 59/94 pairs receive ratios under family/success/accuracy checks; method equivalence remains case-specific; no aggregate ranking |
| Function evaluations | Available, not uniform | Callback wrappers capture many cases; definitions differ across APIs and initialization/final checks |
| Derivative evaluations | Partial | Captured separately in root wrappers; gradient-provider and other derivative-like work is not a common count |
| Iterations/subdivisions | Partial | Library-specific iteration fields; GSL quadrature workspace size and NUMERIX leaf splits are not identical work units |
| Matrix-operation counts | Missing | No FLOP/operation count is recorded |
| ODE accepted/rejected steps | Missing/not applicable to NUMERIX fixed-step methods | NUMERIX executes a fixed number of steps; GSL RK step-doubling performs different work |
| Absolute/relative error and residual | Available for defined cases | Computed from a double reference; metric is category-specific and not uniformly applicable to factor/construct rows |
| Forward/backward error | Partial | Forward error exists only where an analytic reference is supplied; linear backward error is reported for solve rows |
| Estimated versus actual error | Partial | Both are retained where returned, but estimates remain heuristic and their meanings differ by method |
| Reliability/success/failure | Partial | Status and 21 repeated fixed-input probes are stored; this is reproducibility on one deterministic case, not a population success probability |
| Timeout/instability rates | Missing | No independent timeout distributions or randomized stress populations |
| Requested heap bytes/allocation count | Available for selected Linux static runs | Separate allocation executable and ELF linker wrapping; requested bytes, not allocator overhead/process RSS |
| Peak process RSS | Partial/manual | Historical report has separate aggregate `/usr/bin/time -v` dense-run RSS. It is not a per-row field in the benchmark harness |
| Memory per input size / temporary workspace | Partial | Some LU/discovery cases expose probes; no uniform cross-library, all-category memory series |

## 9. Memory measurement readiness

The allocation probe measures successful malloc/calloc/realloc/free activity for an isolated operation, reporting count, cumulative requested bytes, peak live requested heap bytes, setup bytes, and live requested bytes after destruction. The 222 Phase 9 allocation rows end with zero tracked bytes. Discovery separately tracks its direct result/workspace buffers; those metrics have narrower scope than the general wrapper. Valgrind and selected allocation-failure injection provide separate evidence.

These numbers exclude input arrays, stack, allocator metadata, libc internals, shared-library internal calls, and process RSS. The wrapper requires a compatible Linux linker and static linked libraries; it is not a Windows measurement implementation. The Phase 9 report separately records `/usr/bin/time -v` peak process RSS of 2,412 KiB for one filtered NUMERIX dense run and 2,628 KiB for the corresponding GSL run. Those are whole-process run maxima, not per-algorithm memory fields in the harness. Future reports must label requested heap and OS process memory separately, and report peak RSS only when measured with a documented per-run method.

For cross-library memory comparison, isolate each workload in a fresh child process, measure allocator requests with library-aware wrappers where available, and separately record peak process memory using platform APIs. Define whether inputs, workspaces, outputs, language-runtime overhead, BLAS worker pools, and setup are included. Windows working-set/private-commit values and Linux RSS are not automatically interchangeable; record both the metric definition and OS.

## 10. Fair-comparison readiness

Existing GSL 2.8 is built locally as a static library with reference CBLAS, not a tuned OpenBLAS/vendor BLAS. Both libraries use GCC 8.5 Release flags for the Phase 9 run. The planned external libraries have not been integrated or version-checked by this audit.

| Category | Existing GSL coverage | Planned library coverage and comparison boundary |
|---|---|---|
| Roots | Bisection, Brent-family, Newton; excludes NUMERIX regula falsi, secant, and safeguarded Newton | Boost.Math is a candidate API to map; the other planned libraries are not established as root comparators in this repository |
| Linear algebra | Dense pivoted LU, sizes 8–128; matrix family includes Hilbert and scaled dense matrices | LAPACK is the direct factor/solve comparison; OpenBLAS changes the BLAS backend, so distinguish algorithm from backend. Eigen is an optional dense C++ comparator. SUNDIALS internal linear solvers are a separate integration context |
| Differentiation | NUMERIX Richardson versus GSL adaptive central difference; different methods | External API support and step/tolerance semantics need version-specific verification before pairing |
| Interpolation | Newton/divided differences and linear-family comparisons; Lagrange representation is different | Planned-library API and representation must be checked per method; do not infer equivalence from a shared result type |
| Integration | Simpson versus GSL QAG21 is explicitly a different adaptive method; fixed composite rules are NUMERIX-only | Boost.Math may be evaluated after API/version mapping; SUNDIALS ODE integration is not a scalar quadrature substitute |
| Optimization | One-dimensional golden section versus GSL golden-section family, with different initialization | NLopt should be a best-available optimization mode unless a truly equivalent algorithm/configuration is identified; gradient descent is not currently benchmarked |
| ODEs | NUMERIX fixed Euler/Heun/RK4 versus GSL RK4 step-doubling; differing work | SUNDIALS adaptive solvers are best-available comparisons, not same-algorithm fixed-step pairs. Record error control, stiffness handling, steps, and tolerances |

Use two separately labeled modes. **Same-algorithm mode** requires equivalent numerical methods, identical problem data, comparable stopping tests and budgets, precision, output requirements, and clearly described setup cost. If these cannot be matched, mark the pair non-equivalent and do not publish a direct speed ratio. **Best-available mode** allows each library to use its appropriate supported solver for the same problem and accuracy target; it measures practical library capability, not algorithm speed. Report the two modes separately.

For each library and workload, report both **library-only** costs (prebuilt workspaces/data where the algorithm permits) and **end-to-end API** costs (allocation, validation, setup, and destruction). Fix single-thread execution for the primary comparison; run a separate, explicitly configured multithreaded study for BLAS or other threaded backends. Record compiler, wrapper language, ABI crossings, flags, runtime, dependency versions, BLAS version, thread environment, and setup policy.

## 11. Portability and reproducibility

Phase 11 records Linux x86_64 builds/tests under GCC 8.5, GCC 11.5, GCC 15.2, and Clang 21.1, including Debug and Release for the listed compiler configurations. CMake 3.16.8 and 4.4.4 and Make were exercised in Phase 12. The combined Clang ASan/UBSan/leak run passed 34 tests; Valgrind covered 12 executables and the allocation suites; GCC/Clang analyzers covered 25 production translation units; exhaustive Cppcheck passed. These are actual Linux results.

Native modern Windows MinGW, MSVC, and macOS jobs remain unvalidated. The current hosted run `43099042637` for HEAD is queued; earlier hosted runs also had empty job lists. Historical Windows MinGW 6.3 results apply to earlier project states. Do not use them as native validation of 0.2.0-rc1. Cross-platform benchmark comparisons are blocked until native execution, timer resolution, math libraries, compiler flags, ABI, and memory collection are recorded for each host.

For a reproducible future run, store one machine-readable configuration with problem formula/parameters, method/API identifiers, tolerance semantics, expected outcome, reference precision and provenance, input generation seed, size, repetitions, warmups, batch policy, timeout, and thread count. Capture OS/kernel, CPU model/architecture, RAM, compiler and complete flags, runtime, dependency versions/build flags, BLAS implementation, affinity/power policy, benchmark date, git SHA/dirty status, and binary/config hashes. Retain raw per-sample measurements and a script/version that regenerates derived tables and plots. Current raw rows are reproducible as a design, but timing values cannot be reconstructed exactly from aggregated summaries.

## 12. Recommended additional mathematical test problems

Add analytical, high-precision, or independently checked tests before using the affected categories in broad benchmark conclusions.

| Category | Families to add | Coverage purpose |
|---|---|---|
| Roots | Simple and repeated roots; flat odd/even multiplicity; near-coincident roots; roots at/near either endpoint; extreme function scales; poorly scaled nonlinear roots; near-zero derivatives; bad initial points; invalid brackets; discontinuous jumps/poles; representationally narrow brackets; evaluation/iteration/stagnation limits | Position error and residual reported separately; verified continuity assumptions; include no-root and expected-rejection controls |
| Linear algebra | Well-conditioned diagonally dominant and random seeded dense matrices; scaled identity; pivot-requiring matrices; near singular and singular matrices; Hilbert/Vandermonde stress; row/column scaling; tiny/subnormal and near-maximum finite scales; residual/backward-error and forward-error references | Separate mathematical singularity, threshold rejection, factorization overflow, and conditioning amplification; sparse matrices remain unsupported |
| Differentiation | Polynomials of known degree, exp/trig/rational functions, high curvature, large and small x, step sweeps through local ULP scales, large output offsets, noisy callback model, first and second derivatives | Plot actual error against h to show truncation/roundoff and identify points where stencil coordinates collapse |
| Interpolation | Uniform/Chebyshev-like/clustered/nonuniform nodes; node counts 2, 4, 8, 16, 32 and a controlled larger level; scaled/offset polynomials; smooth non-polynomial functions; near-node queries; interior and exterior queries; duplicate and near-duplicate nodes | Separate construction from evaluation, compare interpolation error between nodes, retain node residual as a different metric |
| Integration | Constants and polynomials, smooth transcendental functions, oscillations with varied frequency, narrow smooth peaks, endpoint square-root behavior, integrable and non-integrable singularities, interior jumps/poles, reversed bounds, large offsets and cancellation, tolerance/depth/evaluation sweeps | Compare actual error, reported estimate, callback count, and correct resource-limit status; do not imply improper-integral support |
| Optimization | Convex quadratics, shifted/scaled minima, boundary minima, flat valleys, ill-scaled coordinates, non-convex/multimodal functions, saddle/poor gradient, noisy objective, multiple dimensions | State when an objective violates golden-section unimodality; distinguish local convergence from a global minimum |
| ODEs | Scalar and vector analytic systems; harmonic/oscillatory, stiff and non-stiff systems; varied dimensions; rapidly growing/decaying solutions; long intervals; step/coordinate-resolution sweeps; deliberate overflow and instability | Compare trajectory and final error, work, stability region, and fixed-step cost; adaptive SUNDIALS results belong in best-available mode |

For every generated case, keep a high-precision or analytically justified reference, explicitly define which rounded input is actually solved, and state why the reference is reliable. For ill-conditioned problems, report input perturbation and conditioning alongside forward error.

## 13. Input sizes, variants, and repetitions

Use the prompt’s 300–700 distinct problem configurations as a planning range, not a goal by itself. A balanced first suite can target approximately **400 configurations** after deduplication. Define a configuration as a unique mathematical input plus method, precision, and tolerance/resource variant; do not count its repeated timing samples as new configurations.

One initial allocation is: 60 root configurations across root structure, scale, bracket/initial quality, and direct solver; 60 dense-linear configurations across four or five sizes, conditioning/scale families, and LU/solve phases; 60 differentiation configurations across six functions, x scales, and step regimes; 55 interpolation configurations across node families/sizes/query positions; 65 integration configurations across smoothness, oscillation, endpoints, rules, and tolerances; 45 optimization configurations across objective shape, dimension, and interval/initialization; and 55 ODE configurations across system family, dimension, step, and interval. These are design counts to refine against API support. Keep unsupported combinations out and publish a generated manifest so actual totals are auditable.

For short-running algorithms, use a pilot to choose a batch duration long enough to exceed timer noise, then record the batch size and overhead. Collect **20–30 independent measured batches per configuration** as a starting point. Retain each batch value, not only mean/median/p95. Use stable warmups and a randomized or counterbalanced library order within paired blocks to reduce thermal/order bias. Report confidence intervals or robust spread; add repetitions when uncertainty is large relative to the observed difference. Repeating the same deterministic input estimates timing variation, not algorithm reliability over a problem distribution. Reliability requires varied seeded inputs and explicitly counted outcomes.

Choose input sizes according to asymptotic work and memory. For dense LU, include small API-dominated sizes and larger feasible cubic-work sizes; record allocation/workspace separately. For ODEs, vary dimension, interval, and step count independently. For root/integration/differentiation, vary conditioning/frequency/step/tolerance, not an arbitrary array length. Include stress cases but label expected failure, numerical singularity, and unsupported domains separately from valid accuracy trials.

## 14. Required corrections before expanded benchmarking

The corrections below are the pre-benchmark gate. A correction can be an implementation fix plus regression, or a validated explicit domain exclusion with a recorded expected status. Do not silently drop a case.

1. **Fix or exclude the confirmed numerical edge cases ROOT-01, LA-01/02, DIFF-01, INTERP-01, and ODE-01.** Add independent regressions for the exact mechanisms. If excluded, encode ranges/spacing constraints in the case manifest and report them as unsupported/expected outcomes. This is required before claiming broad scale and floating-point robustness.
2. **Correct BMARK-01 and BMARK-02.** Record significand precision (`DBL_MANT_DIG`) separately from storage width, and randomize/counterbalance paired library order. Verify the output and include tests of metadata and order behavior.
3. **Strengthen the result record under BMARK-03.** Preserve all timing batches, include max or let it be derived from retained samples, add configuration identity/hash, and keep status/accuracy outcomes for each probe. The planned statistical report cannot be regenerated fully from current aggregate-only CSVs.
4. **Resolve DIAG-01’s unavailable residual representation.** Keep an unavailable derivative residual distinguishable from numerical zero in any generic output/export path. Review benchmark serializers for every field that is not meaningful to a method.
5. **Define and validate the common comparison contract.** For each planned library/API, choose same-algorithm versus best-available mode, map tolerance/stopping semantics, match reference/input generation, declare setup/allocation inclusion, pin precision and thread count, and predeclare accuracy/failure eligibility. Keep failures in raw output.

## 15. Improvements that can wait until after the first credible comparison

These do not invalidate correctly scoped measurements of current supported routines, though they may limit future conclusions:

- Add sparse or structured linear algebra, optimized BLAS integration, and large-scale solvers.
- Add adaptive/stiff ODE methods and general accepted/rejected-step reporting.
- Add multidimensional and broader non-convex optimization methods.
- Add improper-integral transformations, specialized quadrature, and symbolic methods only if comparative evidence supports them.
- Add stronger root-discovery completeness heuristics or symbolic analysis; never promise completeness for finite black-box sampling.
- Add broader mixed-precision APIs, stable ABI, shared libraries, and custom allocators under a separately designed compatibility plan.
- Extend user-facing examples, fuzzing, noisy-function testing, and allocation-failure coverage after the comparison framework can show where they matter most.

## 16. Unsupported features and limitations

- Sparse matrix storage/solvers and specialized symmetric/eigenvalue APIs.
- Adaptive ODE stepping, local error control, stiffness detection, and stiff solvers.
- General global optimization and guaranteed convergence for non-convex objectives.
- Automatic finite-difference step selection from scale/noise/curvature.
- Improper infinite-endpoint quadrature and certified quadrature error bounds.
- Symbolic discontinuity/multiplicity analysis and complete black-box root discovery.
- General multithreaded execution; NUMERIX managed cancellation is cooperative and callback-boundary based.
- Cross-platform stable ABI, shared-library distribution, and universal memory-limit enforcement.
- Modern native Windows MinGW/MSVC and macOS validation for the current release candidate.

These are scope limits, not all defects. Future benchmarks must not imply these capabilities through comparison labels.

## 17. Risks to comparative conclusions

1. **Incorrect numerical output/status on untested scale edges** can make a library appear inaccurate, fast because it failed early, or less reliable. Preserve status, accuracy, and time together.
2. **Precision metadata and oracle perturbation** can misstate the arithmetic or blame the solver for an input/reference mismatch. Record mantissa/exponent, high-precision input construction, and the exact rounded problem.
3. **Fixed library order and uncontrolled host state** can bias short-run ratios. Randomize paired order, repeat across fresh processes, record affinity/load/power policy, and compare spreads.
4. **Different work and stopping rules** make raw time ratios misleading. Most current GSL pairs are not identical algorithms; GSL RK4 step doubling and NUMERIX fixed RK4 explicitly do different work.
5. **Aggregated timing only** hides outliers and prevents recomputing uncertainty. Future reports require per-sample data and derivation code.
6. **Memory metric mixing** confuses requested allocations with RSS and excludes library/runtime/stack/backend memory inconsistently.
7. **Single-host and missing native platforms** limit external validity. The present Linux timing does not establish Windows/macOS performance or support.
8. **Accuracy-gated exclusions** can hide the hardest cases if only ratios are published. Retain and report excluded failures and reasons beside every eligible ratio.

## 18. Findings register and prioritized recommendations

Severity is assigned for the proposed benchmark scope. “Confirmed, source-level” means the code path and IEEE-754 arithmetic establish the case; a standalone reproduction was not run in Phase 13A. “Inherent limitation” is documented behavior rather than a confirmed implementation defect.

| ID | Category / finding and evidence | Severity / status | Potential benchmark effect | Recommended action |
|---|---|---|---|---|
| BMARK-01 | Precision column: `benchmarks/comparison.c`, `emit_workload`, writes `sizeof(double)*8` to `precision_bits` (64), while `--versions` reports `DBL_MANT_DIG` (53). No metadata regression is registered in `tests/`. | High; confirmed metadata defect | Reports 64 bits of precision where binary64 has a 53-bit significand; undermines precision-stratified comparisons | Store significand bits, storage bits, and exponent range as separate fields; verify CSV against runtime metadata |
| BMARK-02 | Paired order: `benchmarks/comparison.c`, `run_case`, iterates NUMERIX then GSL for every paired workload; no randomized/counterbalanced order. No order-bias test is registered. | Medium; confirmed design bias risk | Thermal, frequency, and host drift can systematically favor one side, especially short operations | Randomize/counterbalance paired order; retain order/seed and repeat in fresh processes |
| BMARK-03 | Statistics: `emit_workload` stores only aggregate min/median/mean/sample-SD/p95 of 21 batch means; no raw batch list or maximum. `measurements.csv` cannot rebuild full distribution. Existing integrity check is `scripts/verify_benchmark_results.py`; it validates summaries, not the lost samples. | Medium; confirmed coverage gap | Outliers and future confidence/statistic calculations are unrecoverable from historical CSV alone | Persist per-batch values, derive summaries from them, include maximum and configuration identity |
| ROOT-01 | Narrow bracket: `src/algorithms/root_finding/root_finding.c`, `solve_bracket`; it computes `interval_converged` but still calls `evaluate_root` before returning success. Existing nearest tests are `tests/test_accuracy.c` root position/resource-limit cases; no narrow-bracket/two-evaluation regression was located. | Low; confirmed source-level status defect | Reports resource failure for a position-qualified midpoint in low-budget edge cases | Return the qualified midpoint using cached endpoints before spending an unnecessary evaluation; add narrow-bracket/evaluation-budget regression |
| LA-01 | Pivot floor: `src/algorithms/linear_algebra/linear_algebra.c`, `lu_raw` and `numerix_linear_solve_gauss_jordan`; `fmax(DBL_MIN, eps*n*max)` rejects a well-conditioned matrix scaled below DBL_MIN. Existing nearest tests are the `linear_algebra` category in `tests/test_phase1.c` and Phase 9 `dense_small` cases; no subnormal scaled-identity regression was located. | High; confirmed source-level scale defect | False singular status on small-scaled, well-conditioned systems; distorts scale sweeps and success rates | Use scale-invariant pivot treatment with explicit subnormal policy; test scaled identity and known solution over exponent range |
| LA-02 | Factor finiteness: `src/algorithms/linear_algebra/linear_algebra.c`, `lu_raw`; finite entries near DBL_MAX can overflow the Schur update and factorization has no post-update finiteness check before success. Nearest tests are `tests/test_phase1.c` linear algebra and Phase 9 scale/Hilbert rows; no near-maximum finite factor regression was located. | High; confirmed source-level false-success risk | A successful decomposition can contain infinite factors; invalidates solve/performance data | Check every generated factor/multiplier and fail explicitly; add finite-input overflow and cleanup regression |
| DIFF-01 | Stencil collapse: `src/algorithms/differentiation/differentiation.c`, `deriv`/`d_eval`; finite sample coordinates are checked but distinctness from x/stencil neighbors is not. Nearest tests are the `differentiation` category in `tests/test_phase1.c` and Phase 9 four-function fixed-step run; no local-ULP step regression was located. | High; confirmed source-level accuracy defect | Returns zero or aliased derivative samples with success at large x/small h; false accuracy and quick timings | Detect indistinguishable points and report a clear convergence/input failure; add local-ULP step sweeps |
| INTERP-01 | Linear interpolation overflow: `src/algorithms/interpolation/interpolation.c`, `numerix_interp_linear`; unscaled finite differences overflow for finite symmetric extreme endpoints even when the interpolated result is representable. Nearest tests are the `interpolation` category in `tests/test_phase1.c` and Phase 9 eight-node cubic/two-node ordinary-scale rows; no extreme-range regression was located. | Medium; confirmed source-level range defect | Valid large-scale cases become failures and skew reliability/latency | Use a scaled form or explicit documented range bound; test representable results across extreme finite ranges |
| ODE-01 | Coordinate stagnation: `src/algorithms/ode/ode.c`, `solve`; finite nonzero h is accepted without checking `x+h != x` or distinct RK stages. Nearest tests are the `ode` category in `tests/test_phase1.c` and Phase 9 three systems at x=0, h=0.01; no sub-ULP step regression was located. | High; confirmed source-level success defect | Reports success for a grid that does not advance in x and whose stages may be evaluated at the same abscissa | Detect coordinate stagnation and fail explicitly; add large-x/sub-ULP step regression |
| DIAG-01 | Unavailable residual: `src/algorithms/differentiation/differentiation.c`, `deriv`, sets generic `result.residual=0` on success despite no derivative residual. Nearest tests are `tests/test_diagnostics.c` and Phase 9 derivative accuracy rows; no assertion for derivative residual availability was located. | Medium; confirmed misleading metric | Generic serializers/consumers can interpret unavailable evidence as a measured zero | Represent unavailable as NAN/availability flag and audit output serializers |
| ROOT-02 | Open-method position estimate: `root_finding.c`, `step_estimate` and Newton/secant paths use observed contraction, not an enclosure. [Tolerance guide](../api/tolerances.md) labels this heuristic. Nearest tests are `tests/test_accuracy.c` scaled/flat root regressions. | Medium; inherent limitation | Status/estimated error cannot certify location for callback discontinuities, noise, or later loss of contraction | Benchmark only as heuristic evidence; independently compute reference position error and classify discontinuity assumptions |
| INTEG-01 | Adaptive error: `integration.c`, `assess_panel` and heap loop use finite successive-rule differences plus a roundoff floor. [Integration guide](../api/integration.md) explicitly says the estimate is heuristic. Nearest tests are `tests/test_accuracy.c` endpoint/oscillation/resource cases. | Medium; inherent limitation | Aliased oscillations/narrow peaks can appear converged with an optimistic estimate | Retain adversarial/frequency sweeps; report actual reference error separately; preserve limit statuses |
| OPT-01 | Coverage/estimate: Phase 9 compares only one-dimensional golden section; Phase 1 gradient descent is not benchmarked. Golden section’s returned sample is paired with half interval width as an estimate. Nearest tests are `tests/test_phase1.c` optimization and Phase 9 four minima. | Low; confirmed coverage gap / estimate limitation | No claims about vector optimization; half-width should not be presented as actual error | Add gradient cases or scope it out; record known-minimum error separately and label interval estimate |
| DISC-01 | Completeness: `src/root_discovery/root_discovery.c`, finite adaptive sampling, bounded candidates, explicit unresolved regions; [root discovery guide](../api/root_discovery.md) says `search_inconclusive` is always true. Nearest regression suite is `tests/test_root_discovery.c`. | Medium; inherent limitation | Root counts are search outcomes, not proof that all roots were found | Keep discovery as NUMERIX-only capability measurement; include missed-root adversarial references and unresolved counts |
| PORT-01 | Native benchmark environment: [Phase 11 report](phase11_portability_report.md) and `benchmarks/results/phase11/hosted/run.json`; current Windows/Mac jobs have no execution evidence. The native matrix is defined in `.github/workflows/validation.yml`. | High; confirmed validation gap | Cannot generalize current timings/support to Windows/macOS or verify timers/memory there | Obtain actual native MinGW/MSVC/Apple Clang results before platform claims; compare within each host |

## 19. Final decision and next phase

**NOT READY** for the proposed comprehensive comparison. The existing GSL study is usable as a historical, scoped Linux API benchmark, with its failures and method differences retained. It does not establish broad scale correctness, a full performance distribution, uniform memory measurement, cross-platform comparability, or equivalent coverage for the additional libraries.

**A. Must fix before benchmarking**

- Correct LA-01, LA-02, DIFF-01, and ODE-01, or enforce a clearly documented valid-input range that excludes their trigger conditions. Add independent regression cases and analytical references.
- Correct BMARK-01 and BMARK-02 before collecting new comparisons.
- Complete BMARK-03 so the expanded statistics can be regenerated from raw batches.
- Resolve the narrow-bracket status issue ROOT-01 and interpolation range issue INTERP-01 before including those ranges in headline accuracy/reliability conclusions.
- Keep reference construction trustworthy for ill-conditioned problems and retain every failed or accuracy-missed observation.

**B. Should improve before benchmarking**

- Resolve DIAG-01 and explicitly export availability for every non-applicable metric.
- Generate a machine-readable configuration/manifest and predeclared same-algorithm and best-available comparison eligibility rules.
- Add category-appropriate scaling/adversarial cases, especially step/ULP sweeps, linear scaling/overflow, integration oscillation/tolerance sweeps, and ODE stiffness/step-size tests.
- Add a uniform documented memory protocol and native Windows/macOS runs before publishing cross-platform conclusions.
- Record CPU time separately if it is a stated project metric; preserve wall time as a different measurement.

**C. Can improve after benchmarking**

- Adaptive/stiff ODE solvers, sparse/structured linear algebra, expanded optimization families, improper integrals, stable ABI/shared libraries, custom allocator, and more complete discovery methods. Benchmark results can help prioritize these expansions.

**Recommended next phase: Phase 13B — Correctness and measurement-gate fixes.** Work in this order: (1) add regressions for the confirmed arithmetic/status cases; (2) fix or explicitly constrain those input domains; (3) correct precision metadata and paired order; (4) persist per-batch timing and a machine-readable workload manifest; (5) re-run the available compiler/sanitizer suite and raw CSV verifier without overwriting Phase 9/10 history; (6) recheck every previously failed row and the new edge cases; (7) publish a signed-off benchmark protocol and external-library API map; then begin the separate comparative campaign. Keep Phase 9/10 data immutable as historical evidence.

## Evidence index

- Current source and API: [public header](../../include/numerix.h), [root finding](../../src/algorithms/root_finding/root_finding.c), [linear algebra](../../src/algorithms/linear_algebra/linear_algebra.c), [differentiation](../../src/algorithms/differentiation/differentiation.c), [interpolation](../../src/algorithms/interpolation/interpolation.c), [integration](../../src/algorithms/integration/integration.c), [optimization](../../src/algorithms/optimization/optimization.c), [ODEs](../../src/algorithms/ode/ode.c), [diagnostics](../../src/diagnostics/diagnostics.c), [root discovery](../../src/root_discovery/root_discovery.c).
- Existing benchmarks: [problem definitions and protocol](../../benchmarks/README.md), [measurement source](../../benchmarks/comparison.c), [Phase 9 raw observations](../../benchmarks/results/phase9/measurements.csv), [Phase 9 comparison eligibility](../../benchmarks/results/phase9/comparisons.csv), [Phase 9 environment](../../benchmarks/results/phase9/environment.json), [Phase 10 measurements](../../benchmarks/results/phase10/measurements.csv).
- Validation/platform/package: [Phase 11 report](phase11_portability_report.md), [Phase 12 validation records](../../benchmarks/results/phase12/validation/results.json), [artifact manifest](../../benchmarks/results/phase12/artifacts.json).
- Previous mathematical findings: [Phase 10 accuracy report](phase10_accuracy_report.md), [tolerance contract](../api/tolerances.md), [final project report](final_project_report.md).
