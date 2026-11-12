# NUMERIX — PHASE 14B: COMPLETE METHOD-LEVEL BENCHMARKING, MULTI-LIBRARY COMPARISON, EDGE-CASE ANALYSIS AND INTERACTIVE HTML REPORT

## 1. Primary Objective

Continue the NUMERIX Phase 14 expanded multi-library benchmarking project.

Phase 14 has already established a substantial benchmarking foundation:
- 700 distinct mathematical problems across seven numerical modules.
- 2,000 library/problem benchmark pairs.
- 62,000 timing batches.
- 2,000 accuracy records.
- 8,000 work metrics.
- 2,000 isolated-process RSS observations.
- 22 existing figures in PNG and SVG formats.
- Detailed Markdown and HTML reports.

However, Phase 14 is currently classified as PARTIAL because it does not benchmark every public NUMERIX method individually.

Your task is to complete the missing method-level benchmarking, improve the quality and fairness of measurements, thoroughly evaluate edge cases, and produce a professional, interactive HTML report that makes it easy to inspect every library, every implemented method, and every comparison.

This is an extension and completion of Phase 14, NOT a replacement or restart.

The final objective is to establish, with reproducible evidence:

1. Which methods are implemented by each library.
2. Which NUMERIX methods have equivalent implementations in competing libraries.
3. How every comparable method performs on identical mathematical problems.
4. Which methods are more accurate, faster, more reliable, or more memory-efficient.
5. How methods behave on normal, difficult, extreme, and edge-case inputs.
6. Where NUMERIX performs well and where it needs improvement.
7. Where comparisons are not mathematically equivalent or cannot be fairly made.

Do not manufacture a universal winner. Conclusions must be based on the measured evidence.

---

# 2. CRITICAL FIRST STEP: CREATE AND MAINTAIN THE MASTER REQUIREMENTS FILE

Before modifying anything, create a file named:

**`a.md`**

It MUST be placed in the NUMERIX repository root.

This file must contain the complete Phase 14B requirements, objectives, method coverage matrix, benchmark requirements, comparison rules, edge-case requirements, report requirements, validation requirements, and final acceptance criteria.

Do not create it in a subdirectory.

### Mandatory instructions for a.md

1. Write the complete Phase 14B specification into `a.md`.
2. Treat `a.md` as the authoritative checklist for this phase.
3. Read `a.md` before beginning implementation.
4. Reopen and consult `a.md` before starting each major module.
5. Cross-check every completed module against `a.md`.
6. Update its progress checklist as tasks are completed.
7. Before declaring Phase 14B complete, go through the entire file line by line and verify that every requirement has been addressed.
8. Do not silently remove requirements or mark incomplete work as complete.
9. If a requirement cannot be implemented, document the exact reason and mark it PARTIAL or BLOCKED.
10. Keep `a.md` in the repository root as a permanent project artifact.

The purpose is to ensure that the entire phase is completed systematically rather than focusing on only the easiest benchmark tasks.

The requirements file must remain useful for future auditing and verification.

---

# 3. FIRST: INSPECT THE EXISTING REPOSITORY AND PHASE 14 RESULTS

Repository:

`/home/142401034/numerix`

Before making changes, inspect the actual repository state.

Read these files carefully:

- `a.md` if it already exists.
- `PHASE14_REQUIREMENTS.md`
- `documentation/development/phase_14_audit_and_execution_log.md`
- `documentation/development/phase_14_completion_report.md`
- `documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.md`
- `documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.html`
- Relevant benchmark source code.
- Existing workload manifests.
- Existing benchmark CSV schemas.
- Existing analysis and verification scripts.
- Existing public API declarations in `include/numerix.h`.
- Actual implementations under `src/`.
- Existing tests and edge-case tests.
- Existing Phase 13B and Phase 14 benchmark artifacts.

Inspect the actual public API and implementation before designing additional benchmarks.

Do not assume that every method listed in this specification exists exactly as described. Confirm the actual function names, signatures, supported inputs, algorithms, and documented behavior.

Produce an initial method coverage audit showing:

- Public NUMERIX method.
- Source implementation.
- Existing benchmark coverage.
- Existing competing-library equivalent.
- Missing benchmark coverage.
- Required additional tests.
- Whether a fair direct comparison is possible.

## Repository preservation rules

The Phase 14 study contains substantial existing untracked output.

Preserve all existing Phase 14 artifacts.

Do not:
- Delete existing benchmark results.
- Overwrite historical CSV files.
- Replace historical reports without retaining the originals.
- Remove existing figures.
- Reset the Git repository.
- Run destructive cleanup commands.
- Change the project version.
- Create a Git commit.
- Push to GitHub.

Preserve the existing Phase 13 and Phase 14 checksums and reproducibility records.

Use a separate Phase 14B results directory wherever possible.

Suggested structure:

`benchmarks/results/phase14b/`

Do not mix new Phase 14B results with historical Phase 14 raw measurements.

The existing Phase 14 study must remain independently reproducible.

---

# 4. COMPLETE METHOD COVERAGE — ALL SEVEN NUMERICAL MODULES

The most important requirement is to benchmark the actual public methods individually.

Do not benchmark only one representative algorithm from each module.

Build a coverage matrix using the actual NUMERIX public API.

For every method:
- Record whether it is implemented.
- Record the algorithm used.
- Record its competing-library equivalent.
- Benchmark it on appropriate problems.
- Evaluate its numerical accuracy.
- Record execution time.
- Record work metrics.
- Test edge cases.
- Generate relevant graphs.
- Record observations.
- Include the results in the interactive HTML report.

## MODULE 1 — ROOT FINDING

Benchmark all implemented NUMERIX root-finding methods.

Expected methods to investigate:

1. Bisection.
2. Regula Falsi.
3. Secant.
4. Newton-Raphson.
5. Safeguarded Newton.
6. Brent-style root finding.
7. Multiple-root discovery.

Confirm the exact methods available in the public API.

### Required workload categories

Include different mathematical characteristics:

- Simple linear roots.
- Quadratic roots.
- Cubic roots.
- Higher-degree polynomial roots.
- Roots near interval boundaries.
- Roots near zero.
- Large-magnitude roots.
- Small-magnitude roots.
- Flat functions near roots.
- Roots with small derivatives.
- Multiple roots.
- Repeated roots.
- Tangential roots without sign changes.
- Closely spaced roots.
- Functions with multiple roots.
- Highly nonlinear functions.
- Oscillatory functions.
- Poor initial guesses.
- Difficult convergence cases.
- Functions with domain restrictions.

### Important edge cases

Test:

- Root exactly at the initial endpoint.
- Root exactly at the opposite endpoint.
- No root in a supplied bracket.
- Same-sign endpoint values.
- Nearly identical endpoints.
- Very small brackets.
- Very large brackets.
- Floating-point midpoint stagnation.
- Derivative close to zero.
- Derivative equal to zero.
- NaN and infinity from callbacks.
- Discontinuous functions.
- Invalid expressions.
- Maximum iteration exhaustion.
- Evaluation budget exhaustion.
- Cancellation where supported.
- Multiple roots with and without sign changes.

### Competing libraries

Investigate and use:

- GSL.
- CMINPACK where appropriate.
- Any other already integrated root-finding library if genuinely useful.

Separate the comparisons into:

**A. Direct algorithm comparison**

Compare the same or mathematically equivalent methods, such as:
- Bisection versus bisection.
- Newton versus Newton.
- Secant versus secant.
- Brent-style method versus Brent-style method.

**B. Best-available algorithm comparison**

Compare the best appropriate method each library provides for the same problem.

Do not present these as identical algorithms.

CMINPACK is not a direct equivalent for every bracketing method. Explain its suitability and use it only for appropriate problems.

### Root metrics

Record:

- Wall-clock runtime.
- CPU runtime.
- Function evaluations.
- Derivative evaluations.
- Iteration count.
- Final root estimate.
- Absolute root error.
- Relative root error.
- Final residual.
- Convergence status.
- Success rate.
- Accuracy pass rate.
- Time to reach specified accuracy.
- Failure reason.
- Work required to achieve the same accuracy.

For multiple-root discovery, also record:

- Number of reference roots.
- Number discovered.
- Missed roots.
- False positives.
- Root-location errors.
- Multiplicity estimates.
- Unresolved search regions.
- Runtime versus search resolution.

Do not claim completeness when the algorithm is heuristic.

---

## MODULE 2 — LINEAR ALGEBRA

Benchmark the actual NUMERIX linear algebra API at the primitive, factorization, and solve levels.

Investigate:

### Vector operations

- Vector creation and destruction.
- Addition.
- Subtraction.
- Scalar multiplication.
- Dot product.
- Norm calculations.
- Any other implemented vector operations.

### Matrix operations

- Matrix creation and destruction.
- Addition.
- Subtraction.
- Multiplication.
- Transpose.
- Matrix-vector multiplication.
- Norm calculations.
- Other implemented matrix operations.

### Linear-system algorithms

- Gaussian elimination with pivoting.
- Gauss-Jordan elimination.
- LU factorization with pivoting.
- LU-based solve.
- Factorization-only operations.
- Solve-only operations using existing factors.
- Combined factorization and solve.

Benchmark the stages separately whenever the API permits.

### Problem sizes

Use a range of matrix dimensions, including small, medium, and large sizes.

Include sizes around:

- 2–8.
- 16–32.
- 64.
- 128.
- 256.
- Larger sizes where practical and where resource limits allow.

Use identical matrices for all competing libraries.

### Matrix characteristics

Include:

- Random well-conditioned matrices.
- Diagonally dominant matrices.
- Symmetric matrices where applicable.
- Positive-definite matrices where applicable.
- Sparse-looking matrices where dense APIs are being evaluated.
- Ill-conditioned matrices.
- Nearly singular matrices.
- Singular matrices.
- Matrices requiring pivoting.
- Matrices with very different row scales.
- Very small finite values.
- Very large finite values.
- Mixed-magnitude matrices.

### Edge cases

- 1x1 matrices.
- Zero matrices.
- Singular matrices.
- Nearly singular matrices.
- Tiny but well-conditioned matrices.
- Large-magnitude matrices.
- Overflow during elimination.
- Underflow.
- Zero pivots.
- Invalid dimensions.
- Allocation failure.
- Non-finite input values.
- Residual verification.
- Factorization failure.
- Partial-output cleanup.

### Competing libraries

Use:

- GSL.
- Reference LAPACK.
- LAPACK linked with OpenBLAS.

Do not combine reference LAPACK and OpenBLAS into a single result. Report them as distinct configurations.

Where relevant, compare:
- Factorization only.
- Solve only.
- Factorization plus solve.
- Matrix primitives.
- End-to-end operation.

Clearly identify the cost of data conversion and workspace initialization.

### Linear algebra metrics

- Runtime.
- CPU time.
- Matrix dimension.
- Memory consumption where measurable.
- Number of operations or FLOP estimates where meaningful.
- Residual norm.
- Relative solution error.
- Backward error.
- Condition number or condition estimate.
- Numerical status.
- Accuracy pass rate.
- Scaling with matrix size.

Do not use theoretical FLOP counts as if they were measured hardware instruction counts.

---

## MODULE 3 — NUMERICAL DIFFERENTIATION

Benchmark every implemented differentiation method.

Investigate:

1. Forward difference.
2. Backward difference.
3. Central difference.
4. Five-point difference.
5. Richardson extrapolation.
6. Second derivative methods.
7. Any other public differentiation method.

### Test functions

Include:

- Polynomial functions.
- Linear functions.
- Quadratic functions.
- Cubic functions.
- Exponential functions.
- Logarithmic functions.
- Trigonometric functions.
- Smooth nonlinear functions.
- Functions with rapidly changing derivatives.
- Functions near domain boundaries.
- Functions with large and small scales.
- Functions with known analytical derivatives.

### Edge cases

- Very small step size.
- Very large step size.
- Step size causing coordinate collapse.
- Large evaluation coordinates.
- Near-zero coordinates.
- Derivative near zero.
- Domain boundaries.
- Non-finite callback results.
- Invalid step sizes.
- Floating-point cancellation.
- Functions with discontinuities.
- Functions without sufficient smoothness.

### Competing libraries

Use GSL's relevant numerical differentiation implementations.

Only classify methods as direct comparisons when the underlying formulas and assumptions are sufficiently equivalent.

Also provide best-available comparisons where useful.

### Metrics

- Absolute derivative error.
- Relative derivative error.
- Runtime.
- CPU time.
- Function evaluations.
- Step size.
- Stability across step sizes.
- Accuracy versus step size.
- Time to reach a target error.
- Success and failure rates.

Include plots showing how derivative accuracy changes with the selected step size.

---

## MODULE 4 — INTERPOLATION

Benchmark all implemented interpolation methods.

Investigate:

1. Linear interpolation.
2. Lagrange interpolation.
3. Newton divided differences.
4. Horner evaluation where applicable.
5. Interpolation construction and evaluation as separate stages where the API allows.

### Test cases

Include:

- Linear functions.
- Quadratic functions.
- Cubic functions.
- Higher-degree polynomials.
- Smooth nonlinear functions.
- Exponential functions.
- Trigonometric functions.
- Equally spaced points.
- Unequally spaced points.
- Small datasets.
- Large datasets.
- Clustered interpolation nodes.
- Widely separated nodes.
- Extrapolation.
- Ill-conditioned interpolation configurations.

### Edge cases

- One data point.
- Two data points.
- Duplicate x-coordinates.
- Nearly duplicate x-coordinates.
- Very large coordinates.
- Very small coordinates.
- Large y-values.
- Non-finite input values.
- Extrapolation far outside the data range.
- Overflow-prone polynomial evaluation.
- Poorly conditioned node distributions.

### Competing libraries

Use GSL's relevant interpolation and polynomial evaluation methods.

Explain when the algorithms differ.

### Metrics

- Construction time.
- Evaluation time.
- Total end-to-end time.
- Absolute interpolation error.
- Relative error.
- Maximum error over an independent validation grid.
- Mean error.
- Memory use where measurable.
- Scaling with number of interpolation points.
- Extrapolation error.
- Stability.

Use independent validation points. Do not evaluate accuracy only at the interpolation nodes, where interpolation may reproduce the input values by construction.

---

## MODULE 5 — NUMERICAL INTEGRATION

Benchmark every implemented integration method.

Investigate:

1. Composite trapezoidal rule.
2. Composite Simpson's 1/3 rule.
3. Adaptive Simpson's rule.
4. Any additional public integration methods.

### Test functions

Include:

- Constant functions.
- Linear functions.
- Polynomial functions.
- Exponential functions.
- Trigonometric functions.
- Smooth nonlinear functions.
- Highly oscillatory functions.
- Functions with narrow peaks.
- Functions with sharp changes.
- Functions with integrable singularities where supported.
- Functions with discontinuities.
- Piecewise functions.

### Edge cases

- Zero-width interval.
- Reversed interval.
- Very large integration intervals.
- Very small intervals.
- Odd/even subdivision requirements.
- Discontinuity at an endpoint.
- Discontinuity inside the interval.
- Narrow features missed by coarse sampling.
- Non-finite function values.
- Maximum subdivision exhaustion.
- Roundoff-limited convergence.
- Very strict tolerances.
- Invalid tolerance values.

### Competing libraries

Use relevant GSL integration algorithms, including QAG where appropriate.

Separate:

**Fixed-rule comparisons**
- Trapezoidal versus equivalent composite trapezoidal implementations.
- Simpson versus equivalent composite Simpson implementations.

**Adaptive comparisons**
- NUMERIX adaptive Simpson versus GSL adaptive quadrature.

Do not imply that adaptive Simpson and Gauss-Kronrod QAG are the same algorithm.

### Metrics

- Absolute integration error.
- Relative error.
- Estimated error.
- Runtime.
- CPU time.
- Function evaluations.
- Subinterval or panel count.
- Requested tolerance.
- Actual achieved error.
- Success status.
- Accuracy pass rate.
- Time to reach target accuracy.
- Memory usage where measurable.

### Tolerance sweeps

Run multiple tolerance levels.

Plot:
- Runtime versus achieved error.
- Function evaluations versus achieved error.
- Requested tolerance versus actual error.
- Accuracy versus computational cost.

Investigate whether the method actually achieves the requested tolerance.

Clearly distinguish estimated error from independently measured reference error.

---

## MODULE 6 — OPTIMIZATION

Benchmark all implemented optimization methods.

Investigate:

1. Golden-section search.
2. Bounded minimization.
3. Gradient descent.
4. Any other public optimization method.

Confirm whether the public API supports scalar or multidimensional objectives for each method.

Do not silently treat a one-dimensional method as a general multidimensional optimizer.

### Test functions

Include:

- Convex quadratic functions.
- Nonlinear unimodal functions.
- Flat minima.
- Narrow minima.
- Poorly scaled functions.
- Multimodal functions.
- Functions with local minima.
- Functions with boundary minima.
- Smooth functions.
- Difficult gradient landscapes.
- Multidimensional problems where supported.

### Edge cases

- Minimum at interval endpoint.
- Flat objective near minimum.
- Very narrow minimum.
- Multiple local minima.
- Poor initial guess.
- Zero or near-zero gradient.
- Oversized gradient step.
- Non-finite objective values.
- Invalid bounds.
- Unbounded or unsupported domain.
- Evaluation budget exhaustion.
- Iteration limit.
- Stagnation.
- Ill-scaled variables.

### Competing libraries

Use:

- GSL.
- NLopt.

The previous Phase 14 configuration resulted in NLopt returning accurate points but zero successful convergence statuses because the configured evaluation budget was exhausted.

Investigate this properly.

Do not simply loosen tolerances until NLopt appears successful.

Instead:

1. Inspect each library's termination semantics.
2. Select comparable stopping criteria.
3. Use a justified and documented evaluation budget.
4. Record all objective evaluations.
5. Record termination reasons.
6. Compare accuracy and objective gap.
7. Separate a valid but budget-exhausted point from a successful convergence result.
8. Report any remaining incompatibility.

### Metrics

- Objective function evaluations.
- Gradient evaluations.
- Runtime.
- CPU time.
- Final objective value.
- Reference minimum or best-known objective.
- Objective gap.
- Distance from known minimizer where available.
- Convergence status.
- Iterations.
- Success rate.
- Accuracy pass rate.
- Time to target objective accuracy.

Keep local-minimum performance separate from global-minimum claims.

---

## MODULE 7 — ORDINARY DIFFERENTIAL EQUATIONS

Benchmark all implemented ODE methods.

Investigate:

1. Euler.
2. Heun.
3. Classical RK4.
4. Vector-system implementations.
5. Any additional public ODE method.

### Test problems

Include:

- Simple scalar ODEs with analytical solutions.
- Exponential growth and decay.
- Polynomial solutions.
- Harmonic oscillator.
- Coupled linear systems.
- Nonlinear systems.
- Stiff problems where appropriate.
- Long integration intervals.
- Rapidly changing solutions.
- Problems with different step sizes.
- Systems with multiple state variables.

### Edge cases

- Zero integration interval.
- Backward integration.
- Very small step size.
- Very large step size.
- Time advancement failure.
- Floating-point time stagnation.
- Non-finite intermediate stages.
- Invalid initial conditions.
- Dimension mismatch.
- Overflow or underflow.
- Numerical instability.
- Long trajectory accumulation error.

### Competing libraries

Use:

- GSL where appropriate.
- SUNDIALS ARKODE.
- SUNDIALS CVODE.

Separate:

**Direct algorithm comparison**
- Fixed-step RK4 versus an equivalent fixed-step RK4 implementation.
- Other equivalent methods where available.

**Best-available comparison**
- NUMERIX fixed-step methods versus adaptive solvers.
- SUNDIALS adaptive methods versus other adaptive methods.

Do not claim that a fixed-step method and an adaptive solver are identical algorithms.

### Trajectory-wide accuracy

Do not evaluate only the final state.

Compare the entire trajectory using an independent validation grid.

Record:
- Maximum trajectory error.
- Mean trajectory error.
- Final-state error.
- Error growth over time.
- Runtime.
- CPU time.
- RHS evaluations.
- Accepted and rejected steps for adaptive solvers.
- Step sizes.
- Memory consumption where measurable.
- Termination status.

Generate error-versus-time graphs for representative cases.

---

# 5. PROBLEM COUNT AND WORKLOAD DESIGN

The existing Phase 14 already contains 100 distinct problems per module.

Do not discard or unnecessarily recreate those 700 problems.

Reuse existing problem IDs wherever the method and mathematical assumptions make the problem suitable.

However, method-level coverage must be expanded.

### Required workload approach

1. Preserve the original 700 problems.
2. Create an explicit mapping between each method and the applicable original problems.
3. Add new method-specific problems when existing cases do not adequately test a method.
4. Give new problems unique IDs.
5. Do not duplicate trivial mathematical problems under different IDs merely to increase counts.
6. Include normal, difficult, and edge-case problems.
7. Ensure competing libraries receive identical mathematical inputs.
8. Preserve the reference solution and accuracy criteria for every problem.

Aim for broad and meaningful coverage of every public method.

For primary methods, target approximately 100 distinct applicable problem cases where feasible.

If a method has a narrower mathematical domain, explain the appropriate smaller test set rather than manufacturing irrelevant cases.

### Workload manifest

Every workload must record:

- Problem ID.
- Module.
- Method.
- Function or mathematical definition.
- Input parameters.
- Initial conditions.
- Domain.
- Tolerances.
- Reference solution.
- Reference accuracy.
- Difficulty category.
- Edge-case classification.
- Applicable competing libraries.
- Direct or best-available comparison classification.

---

# 6. FAIR BENCHMARKING PROTOCOL

This is critical.

A benchmark is only useful when the comparison is fair and reproducible.

## A. Two comparison categories

Every comparison must be clearly labeled:

**1. Same-algorithm comparison**

The implementations use the same algorithm or a sufficiently equivalent numerical method.

**2. Best-available comparison**

Each library uses its most appropriate available method for the same mathematical problem.

Never mix these categories without labeling them.

## B. Identical workloads

Use identical:
- Mathematical functions.
- Input data.
- Initial values.
- Matrix dimensions.
- Integration intervals.
- Tolerances where comparable.
- Initial guesses.
- ODE initial conditions.
- Validation points.

Where algorithm-specific configuration prevents identical settings, document the difference.

## C. Timing

Retain the existing 31-batch approach where statistically appropriate.

Record:
- Raw wall-clock timing samples.
- Raw CPU timing samples.
- Minimum.
- Median.
- Mean.
- Standard deviation.
- P95.
- Maximum.
- Sample count.
- Batch size.
- Warm-up information.

Retain zero-resolution CPU observations in raw data, but exclude them from positive CPU-time summaries.

Never fabricate timing values.

Avoid adding benchmark harness overhead to one library but not another.

Measure setup, conversion, initialization, and solver execution separately where possible.

Report both:
- Library/API execution time.
- End-to-end time including relevant setup and conversion.

## D. Avoid survivorship bias

Do not report only successful and accurate cases.

Show separate statistics for:

1. All attempted cases.
2. Successful-status cases.
3. Accuracy-passing cases.
4. Cases that are both successful and accurate.

A library that is fast on a small subset must not appear universally faster.

For speed ratios, clearly state the eligible sample count.

Keep failures and inaccurate outputs in the raw datasets.

## E. Accuracy

Use independent high-quality reference solutions.

Use analytical solutions where available.

For other problems, use a more accurate reference method, increased precision, or independent validation.

Document reference methodology.

Use appropriate absolute and relative tolerances.

Do not use one arbitrary tolerance for every module and scale.

## F. Statistical analysis

Use paired problem-level comparisons.

Report:
- Median paired speed ratio.
- Distribution.
- Confidence intervals.
- Number of eligible pairs.
- Number of excluded pairs.
- Reason for exclusion.

Use a problem-level paired bootstrap for confidence intervals.

When comparing many methods, consider multiple-comparison correction, such as Holm correction, and explain its use.

Do not treat 31 timing batches of the same problem as 31 independent mathematical problems.

Do not claim statistical significance when evidence is insufficient.

## G. Work metrics

Collect appropriate work counters:

- Function evaluations.
- Derivative evaluations.
- Iterations.
- Matrix dimensions.
- RHS evaluations.
- Accepted/rejected ODE steps.
- Subintervals.
- Objective evaluations.
- Operation counts where meaningful.

Do not compare unlike counters as if they represent the same work.

---

# 7. MEMORY MEASUREMENT ENHANCEMENT

The existing Phase 14 RSS measurements are useful but include the benchmark harness and shared-library baseline.

Do not claim that RSS alone proves library-specific memory usage.

Investigate whether reliable allocation instrumentation can be added.

Possible approaches include:
- Linker wrapping.
- Allocation interception.
- LD_PRELOAD where suitable.
- Library-specific allocation hooks if available.

Before relying on allocation instrumentation, verify which allocations it actually intercepts.

Measure separately where possible:

1. Harness baseline.
2. Library initialization.
3. Input/setup allocations.
4. Method execution allocations.
5. Peak live requested heap.
6. Total requested allocation.
7. Allocations after cleanup.
8. Isolated process RSS.

Document exclusions such as:
- Stack memory.
- Allocator metadata.
- Shared-library internal allocations not intercepted.
- Memory allocated by uninstrumented dependencies.
- Runtime and harness overhead.

If comparable memory measurement is impossible for a library, mark it unavailable.

Do not invent a memory winner.

---

# 8. TIME-TO-ACCURACY ANALYSIS

Add time-to-accuracy wherever possible.

Runtime alone does not fully describe numerical performance.

For each suitable method, measure how long it takes to achieve predefined accuracy targets.

Examples:
- Root error below specified thresholds.
- Derivative error below specified thresholds.
- Integration error below specified thresholds.
- Interpolation error below specified thresholds.
- Optimization objective gap below specified thresholds.
- ODE trajectory error below specified thresholds.

Generate plots showing computational cost versus achieved accuracy.

Where a method cannot reach the required accuracy, show that explicitly.

---

# 9. EDGE-CASE TESTING AND REPORTING

Every public method must have a dedicated edge-case section.

Do not hide edge-case results inside a general benchmark summary.

For every method, document:

- Normal cases.
- Difficult cases.
- Extreme-scale cases.
- Boundary cases.
- Invalid-input cases.
- Numerical stability cases.
- Failure cases.
- Accuracy limitations.
- Known algorithmic limitations.

For each edge case, record:

| Field | Description |
|---|---|
| Edge-case ID | Unique identifier |
| Method | Exact method name |
| Input | Actual mathematical input |
| Expected behavior | Reference expectation |
| NUMERIX result | Actual output |
| Competitor results | Available outputs |
| Status | Success, failure, rejected, inconclusive |
| Accuracy | Measured error |
| Runtime | Measured time |
| Observation | Explanation |
| Conclusion | What the result demonstrates |

Important: an invalid-input rejection can be correct behavior. Do not automatically classify every rejected input as a numerical failure.

Distinguish:
- Correct rejection.
- Numerical failure.
- Non-convergence.
- Inaccurate successful result.
- Resource exhaustion.
- Unsupported configuration.

---

# 10. INTERACTIVE HTML REPORT — MAJOR DELIVERABLE

The final HTML report is not merely a static summary.

It must be a professional, interactive, easy-to-navigate benchmarking dashboard.

The report should allow a reader to explore:

- Each numerical library.
- Each module.
- Each method implemented by each library.
- Each method's graphs.
- Its edge cases.
- Its numerical accuracy.
- Its runtime.
- Its work metrics.
- Its comparison with other libraries.
- Its observations and limitations.

The report must work locally without requiring a web server, external API, or internet connection.

Use embedded or locally referenced assets with working relative paths.

Preserve a clear, professional academic and engineering-report design.

## 10.1 Overall report layout

Recommended structure:

### A. Header

Include:
- NUMERIX logo/title treatment.
- Phase 14B — Expanded Method-Level Benchmarking.
- Study date.
- Benchmark environment.
- Study version.
- Dataset identifier.
- Report navigation.

### B. Executive summary

Show:
- Number of libraries.
- Number of modules.
- Number of methods.
- Number of distinct problems.
- Number of benchmark pairs.
- Number of timing samples.
- Number of accuracy records.
- Overall success rate.
- Accuracy pass rate.
- Main findings.
- Important limitations.

Do not display a misleading overall winner.

### C. Navigation

Provide a sidebar or top navigation with:

1. Overview.
2. Library comparison.
3. Root finding.
4. Linear algebra.
5. Differentiation.
6. Interpolation.
7. Integration.
8. Optimization.
9. ODE.
10. Edge-case analysis.
11. Memory analysis.
12. Method coverage.
13. Methodology.
14. Limitations.
15. Reproducibility and raw data.

Use working navigation links.

---

## 10.2 LIBRARY DROPDOWNS — REQUIRED

Create expandable dropdowns/accordion sections for each library.

Include every library actually benchmarked:

- NUMERIX.
- GNU Scientific Library (GSL).
- CMINPACK.
- Reference LAPACK.
- LAPACK with OpenBLAS.
- NLopt.
- SUNDIALS ARKODE.
- SUNDIALS CVODE.

If a library does not implement a method, state that clearly.

Do not create fictional implementations or results.

Each library must have its own expandable section.

Inside each library section, show:

1. Library name.
2. Version.
3. Build configuration.
4. Supported modules.
5. Implemented methods.
6. Methods not supported.
7. Relevant implementation details.
8. Benchmark workload coverage.
9. Overall success rate.
10. Accuracy pass rate.
11. Runtime overview.
12. Memory information where available.
13. Important limitations.

### Method dropdowns inside each library

Inside each library section, create a second level of dropdowns for its methods.

For example:

- NUMERIX
  - Root Finding
    - Bisection
    - Regula Falsi
    - Secant
    - Newton-Raphson
    - Safeguarded Newton
    - Brent-style
    - Multiple-root discovery
  - Linear Algebra
    - Gaussian elimination
    - Gauss-Jordan
    - LU factorization
    - Vector operations
    - Matrix operations
  - Differentiation
    - Forward difference
    - Backward difference
    - Central difference
    - Five-point difference
    - Richardson extrapolation
    - Second derivative
  - Interpolation
    - Linear
    - Lagrange
    - Newton divided differences
  - Integration
    - Trapezoidal
    - Simpson
    - Adaptive Simpson
  - Optimization
    - Golden-section
    - Gradient descent
  - ODE
    - Euler
    - Heun
    - RK4

The above is an illustrative structure. Generate the actual list from the verified public APIs and library capabilities.

Each method dropdown must display its own method-specific results.

---

## 10.3 METHOD DETAILS — CONTENT REQUIRED INSIDE EACH METHOD

This is one of the most important requirements.

For every method in every library, include the following sections directly below its method heading or inside its expanded dropdown.

### 1. Method overview

- Method name.
- Numerical algorithm.
- Short explanation.
- Intended problem type.
- Supported inputs.
- Important assumptions.
- Implementation details.
- Whether the method is direct-comparable or best-available only.

### 2. Benchmark workload

Show:
- Number of problems tested.
- Problem IDs or expandable workload list.
- Problem categories.
- Difficulty distribution.
- Input ranges.
- Tolerances.
- Number of successful executions.
- Number of failed executions.
- Accuracy pass count.

### 3. Performance graphs

Place the relevant graphs immediately below the method's overview and benchmark summary.

Where data exists, show:

- Runtime comparison against other libraries.
- CPU-time comparison.
- Accuracy comparison.
- Error distribution.
- Runtime distribution.
- Time-to-accuracy.
- Work-count comparison.
- Scaling with input size.
- Memory comparison where valid.

Use consistent axes and units.

Display sample counts.

Do not use graphs without supporting data.

If there are insufficient samples, state that clearly.

### 4. Comparison with other libraries

Directly below the graphs, include a comparison table.

Example structure:

| Library | Method | Problems | Success | Accuracy pass | Median time | Median error | Work |
|---|---|---:|---:|---:|---:|---:|---:|
| NUMERIX | Actual method | ... | ... | ... | ... | ... | ... |
| GSL | Equivalent method | ... | ... | ... | ... | ... | ... |
| Other library | Equivalent method | ... | ... | ... | ... | ... | ... |

Use actual measured values.

Clearly distinguish same-algorithm comparisons from best-available comparisons.

### 5. Observations

Include specific observations for the method.

Examples of the type of analysis expected:

- NUMERIX was faster on a specific workload range.
- GSL achieved lower error on difficult cases.
- One implementation scaled better with input size.
- A method required more function evaluations.
- One library rejected an ill-conditioned problem while another returned a result with poor accuracy.
- A particular algorithm was sensitive to initial guesses.
- A method was effective on smooth functions but struggled with discontinuities.

These are examples of analysis style, NOT predetermined conclusions.

Only include conclusions supported by measured data.

### 6. Edge cases

Show an expandable edge-case table immediately below the method's main comparison.

Include:
- Edge-case description.
- Input.
- Expected result.
- NUMERIX behavior.
- Competitor behavior.
- Error.
- Status.
- Runtime.
- Observation.

### 7. Method conclusion

End each method section with:

- Accuracy assessment.
- Performance assessment.
- Reliability assessment.
- Edge-case assessment.
- Strengths.
- Weaknesses.
- Known limitations.
- Whether the method is competitive.
- Recommended future improvements.

Avoid vague statements such as "performs well."

Use evidence and sample counts.

---

## 10.4 MODULE-LEVEL COMPARISON PAGES

In addition to library-first dropdowns, create module-level views.

For example, selecting Root Finding should display all root methods across all libraries.

This allows a reader to compare one mathematical method across libraries without navigating through every library individually.

Each module page should include:

1. Overview.
2. Implemented methods.
3. Library coverage matrix.
4. Method-by-method benchmark graphs.
5. Accuracy comparison.
6. Runtime comparison.
7. Edge-case results.
8. Important observations.
9. Overall module conclusion.

### Coverage matrix

Use a matrix with methods as rows and libraries as columns.

Example:

| Method | NUMERIX | GSL | CMINPACK | LAPACK | NLopt | SUNDIALS |
|---|---|---|---|---|---|---|
| Bisection | Yes | Yes | No | N/A | N/A | N/A |
| Newton | Yes | Yes | Appropriate equivalent only | N/A | N/A | N/A |

Use accurate capability labels:
- Direct equivalent.
- Approximate equivalent.
- Best available.
- Not supported.
- Not applicable.
- Not benchmarked.

Do not treat N/A as a failed benchmark.

---

## 10.5 INTERACTIVE FILTERING

The HTML report should provide useful controls.

Implement:
- Library filter.
- Module filter.
- Method filter.
- Comparison-type filter.
- Problem difficulty filter.
- Edge-case filter.
- Accuracy status filter.
- Success/failure filter.

Allow the reader to expand and collapse library and method dropdowns.

Where possible, provide:
- Expand all.
- Collapse all.
- Search methods.
- Search problem IDs.
- Search edge-case descriptions.

Filters must update the displayed tables and graphs where appropriate.

Do not build decorative controls that do nothing.

---

## 10.6 GRAPH REQUIREMENTS

Produce high-quality, readable, publication-quality graphs.

Generate PNG and SVG versions wherever practical.

Include at least the following graph types where supported by the data:

1. Method-level runtime comparison.
2. Method-level CPU-time comparison.
3. Accuracy/error comparison.
4. Runtime versus achieved accuracy.
5. Time-to-accuracy comparison.
6. Success-rate comparison.
7. Accuracy-pass-rate comparison.
8. Work-count comparison.
9. Scaling with problem size.
10. Edge-case failure distribution.
11. Memory comparison where measurement is valid.
12. Method coverage heatmap.
13. Library capability heatmap.
14. Problem difficulty comparison.
15. Method-level speed-ratio forest plots with confidence intervals.

Use:
- Clear titles.
- Axis labels.
- Units.
- Legends.
- Sample counts.
- Consistent library identity and visual encoding.
- Readable font sizes.
- Appropriate scales.
- Confidence intervals when available.

Avoid:
- Overcrowded graphs.
- Misleading truncated axes.
- Comparing unrelated quantities.
- Missing units.
- Inconsistent colors for the same library.
- Pie charts for large numbers of categories.
- Graphs that hide failures.
- Graphs that imply statistical significance without evidence.

Every graph must have a short written interpretation immediately below it.

---

# 11. HTML IMPLEMENTATION AND USABILITY

The final HTML report must be polished and fully functional.

Requirements:

- Responsive layout.
- Readable typography.
- Consistent spacing.
- Clear visual hierarchy.
- Expandable library sections.
- Nested method sections.
- Functional filters.
- Functional navigation.
- Search.
- Graph rendering.
- Tables.
- Edge-case accordions.
- Clear status indicators.
- No broken images.
- No missing graph references.
- No dead controls.
- No unnecessary animation.

The report must work when opened locally.

Avoid requiring a running backend.

Use local JavaScript and CSS, or embed the required data and graph assets.

If a charting library is used, ensure it is available locally or otherwise ensure the report still works offline.

Provide a useful fallback when a graph cannot be rendered.

Keep the report understandable to both:
- A numerical computing researcher.
- A university project evaluator who may not know every algorithm.

Do not overload the executive summary with technical details. Put the technical details in the relevant method sections.

---

# 12. REQUIRED REPORT AND DATA ARTIFACTS

Create a dedicated Phase 14B output structure.

Suggested structure:

benchmarks/results/phase14b/
- workloads/
- raw/
- summaries/
- edge_cases/
- memory/
- analysis/
- figures/
- verification/
- manifests/

Suggested documentation:

documentation/reports/
- NUMERIX_phase14b_method_level_comparative_evaluation.md
- NUMERIX_phase14b_method_level_comparative_evaluation.html

documentation/development/
- phase_14b_execution_log.md
- phase_14b_completion_report.md
- phase_14b_method_coverage_matrix.md
- phase_14b_verification_report.md

Keep the original Phase 14 report intact.

The new HTML report should be the primary interactive report for the extended study.

The Markdown report should provide a complete textual equivalent of the main findings.

The execution log should record:
- Every major action.
- Files changed or added.
- Commands executed.
- Test results.
- Benchmark results.
- Failures.
- Deviations.
- Decisions.
- Remaining limitations.

---

# 13. REPRODUCIBILITY

Every benchmark run must be reproducible.

Record:

- Unique run ID.
- Date and time.
- Git commit.
- Source fingerprints.
- Library versions.
- Compiler.
- Compiler flags.
- Build configuration.
- Operating system.
- CPU model.
- Number of cores.
- Timing source.
- Random seed.
- Workload manifest hash.
- Benchmark schema version.
- Measurement configuration.
- Library configuration.
- Environment variables relevant to execution.

Preserve raw timing samples.

Preserve raw accuracy records.

Preserve failed attempts.

Do not overwrite previous run directories.

Generate checksum inventories for new artifacts.

Ensure analysis scripts can regenerate the tables and figures from raw data.

Ensure the HTML report is generated from structured result data rather than manually copied values.

---

# 14. VALIDATION AND QUALITY ASSURANCE

After implementation, run the available validation suite.

At minimum:

### NUMERIX validation
- Existing Make tests.
- Existing CMake tests.
- Release build.
- Debug build where practical.
- Public API consistency.
- Installed C consumer.
- Installed C++ consumer.
- Existing Phase 13 tests and verification.
- Existing Phase 14 verification.

### Benchmark validation
- Workload manifest validation.
- Schema validation.
- Raw timing validation.
- Accuracy record validation.
- Method coverage validation.
- Pairing validation.
- Statistics reconstruction.
- Speed-ratio validation.
- Failed-case retention.
- Memory measurement validation.

### Analysis validation
- Analysis unit tests.
- Figure regeneration.
- Table regeneration.
- Report regeneration.
- Checksum verification.
- HTML link validation.
- Local asset validation.
- Graph file validation.

### Memory and static checks
- Valgrind where available.
- ASan and UBSan where the required compiler runtimes are available.
- Static analysis where available.
- `git diff --check`.

If a tool is unavailable, record that explicitly.

Do not report a test as passed if it was not executed.

Do not claim that a platform is validated when it was not tested.

---

# 15. NUMERICAL ALGORITHM FIXES MUST BE SEPARATE

The primary objective of Phase 14B is benchmarking and evaluation.

Do not modify production numerical algorithms merely to improve benchmark results.

If benchmarking reveals a genuine correctness defect:

1. Reproduce the issue.
2. Document it.
3. Create a focused test.
4. Explain the mathematical problem.
5. Propose a separate correction.
6. Keep the change isolated.
7. Re-run the affected tests and benchmarks.
8. Record before-and-after results.

Do not tune NUMERIX specifically to win a benchmark.

Do not weaken accuracy requirements.

Do not hide poor results.

Do not change competitor configurations unfairly.

If a method is genuinely weak, report that honestly.

---

# 16. FINAL ANALYSIS — WHAT THE RESULTS MUST EXPLAIN

The final report must answer these questions.

### Method coverage
- Have all public NUMERIX methods been identified?
- Which methods have been benchmarked?
- Which methods have direct competitors?
- Which methods have only best-available comparisons?
- Which methods remain untested?

### Numerical accuracy
- Which methods are most accurate?
- Which methods lose accuracy on difficult cases?
- Which methods are sensitive to input scale?
- Which methods are affected by conditioning?
- Which methods fail near machine precision?

### Performance
- Which methods are faster under equivalent algorithms?
- Which libraries perform better for particular workload sizes?
- Which methods scale better?
- What happens as accuracy requirements become stricter?
- How much runtime is spent in setup versus actual computation?

### Reliability
- Which methods have the highest successful completion rates?
- Which methods return inaccurate results despite successful statuses?
- Which methods correctly reject invalid or unsupported inputs?
- Which methods are vulnerable to stagnation, overflow, underflow, or non-convergence?

### Edge cases
- Which methods handle edge cases reliably?
- Which cases expose numerical weaknesses?
- Are the weaknesses specific to NUMERIX or shared by competitors?

### Memory
- Which memory observations are genuinely comparable?
- Which measurements include harness or runtime overhead?
- Which memory conclusions are inconclusive?

### Practical conclusions
- Where is NUMERIX competitive?
- Where does NUMERIX need improvement?
- Which methods are suitable for different problem types?
- What should be improved before a stable release?

Do not produce a single overall ranking that hides differences between numerical modules.

---

# 17. ACCEPTANCE CRITERIA

Phase 14B can be marked COMPLETE only when all the following conditions are satisfied.

## Requirements and scope
- [ ] `a.md` exists in the repository root.
- [ ] `a.md` contains the complete Phase 14B specification.
- [ ] `a.md` was consulted throughout the work.
- [ ] Every requirement has been checked before completion.
- [ ] Existing Phase 14 and Phase 13 artifacts are preserved.

## Method coverage
- [ ] Every public NUMERIX method has been identified.
- [ ] Every method has a documented coverage status.
- [ ] Every applicable method has meaningful benchmark cases.
- [ ] Direct and best-available comparisons are separated.
- [ ] Unsupported methods are explicitly identified.
- [ ] All seven modules are represented.

## Measurement
- [ ] Raw timing data retained.
- [ ] CPU timing included where measurable.
- [ ] Accuracy records retained.
- [ ] Work metrics recorded.
- [ ] Failure cases retained.
- [ ] Time-to-accuracy evaluated where practical.
- [ ] Memory limitations documented honestly.
- [ ] Statistical comparisons use problem-level pairing.

## Edge cases
- [ ] Each method has documented edge cases.
- [ ] Expected and actual behavior are recorded.
- [ ] Correct rejection is distinguished from numerical failure.
- [ ] Important numerical limitations are explained.

## HTML report
- [ ] Every library has an expandable section.
- [ ] Every implemented method has a nested expandable section.
- [ ] Method graphs appear directly below relevant methods.
- [ ] Method comparisons with other libraries are included.
- [ ] Method observations are included.
- [ ] Method-specific edge cases are included.
- [ ] Module-level comparison views exist.
- [ ] Filters and navigation work.
- [ ] Graphs have written interpretations.
- [ ] Local HTML report works without a backend.
- [ ] No broken graph or asset references.
- [ ] No nonfunctional controls.

## Reproducibility
- [ ] Raw datasets and manifests are retained.
- [ ] Analysis can regenerate tables and graphs.
- [ ] Checksums are recorded.
- [ ] Verification scripts pass.
- [ ] Existing historical verifiers still pass.

## Final project state
- [ ] Completion report written.
- [ ] Execution log written.
- [ ] Verification report written.
- [ ] Remaining limitations listed.
- [ ] No unsupported claims of completion.
- [ ] No commit or push.
- [ ] No version change.

If one or more major requirements remain incomplete, mark the phase PARTIAL.

Do not mark the phase complete merely because a large number of benchmark rows were generated.

Method coverage and report completeness matter as much as the number of benchmark observations.

---

# 18. EXECUTION ORDER

Follow this order.

**Step 1 — Repository audit**
- Inspect Git state.
- Preserve all existing files.
- Read Phase 14 documentation.
- Inspect actual APIs and benchmark infrastructure.

**Step 2 — Create a.md**
- Write the full specification.
- Add the acceptance checklist.
- Read it again.
- Use it as the phase authority.

**Step 3 — Method coverage audit**
- Enumerate all public methods.
- Map methods to libraries.
- Identify gaps.
- Design workloads.

**Step 4 — Benchmark design**
- Define problem IDs.
- Define reference solutions.
- Define accuracy thresholds.
- Define fair comparison categories.
- Define edge-case datasets.
- Define measurement configuration.

**Step 5 — Benchmark implementation**
- Extend the existing infrastructure.
- Preserve original datasets.
- Add missing method-level cases.
- Capture raw measurements.
- Retain failures.

**Step 6 — Edge-case evaluation**
- Run the dedicated edge-case workloads.
- Record behavior for every method.
- Investigate unexpected outcomes.

**Step 7 — Analysis**
- Reconstruct statistics.
- Calculate paired comparisons.
- Generate method-level graphs.
- Generate module-level graphs.
- Analyze accuracy and performance.
- Analyze memory where possible.

**Step 8 — Interactive HTML report**
- Implement library dropdowns.
- Implement nested method dropdowns.
- Place graphs and comparisons directly below each method.
- Include observations and edge-case tables.
- Add module-level views and filters.
- Validate the complete report locally.

**Step 9 — Verification**
- Run all applicable tests.
- Verify historical results.
- Regenerate report and figures.
- Validate links and assets.
- Check checksums.

**Step 10 — Final cross-check**
- Reopen `a.md`.
- Check every acceptance criterion.
- Identify incomplete work.
- Update execution log.
- Write completion report.
- Assign COMPLETE or PARTIAL honestly.

---

# 19. FINAL RESPONSE REQUIRED FROM THE AI IDE

At the end, provide a concise but detailed completion summary containing:

1. Final Phase 14B status: COMPLETE or PARTIAL.
2. Location of `a.md`.
3. Confirmation that it was consulted throughout the work.
4. Number of NUMERIX public methods identified.
5. Number of methods benchmarked.
6. Number of libraries compared.
7. Number of distinct problems.
8. Number of benchmark pairs.
9. Number of timing observations.
10. Number of edge cases evaluated.
11. Main numerical accuracy findings.
12. Main performance findings.
13. Main reliability findings.
14. Memory measurement limitations.
15. HTML report location.
16. Markdown report location.
17. Figure and raw-data locations.
18. Validation results.
19. Remaining limitations.
20. Git status and confirmation that no commit, push, or version change occurred.

Provide the exact relative paths of all important deliverables.

Do not claim that everything is complete if any major acceptance criterion remains unmet.

---

# FINAL INSTRUCTION

Work carefully and systematically.

The goal is not to generate the largest possible number of benchmark rows. The goal is to produce a comprehensive, fair, scientifically defensible, method-by-method evaluation of NUMERIX against established numerical libraries.

The interactive HTML report is a major deliverable, not an afterthought.

Every implemented NUMERIX method must be accounted for. Every comparison must be understandable. Every important edge case must be documented. Every graph must be supported by real measurements.

Most importantly, **keep `a.md` in the repository root, consult it throughout the entire phase, and perform a final line-by-line requirements cross-check before declaring completion.**