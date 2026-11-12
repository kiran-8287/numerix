# PHASE 14 — EXPANDED MULTI-LIBRARY, SEVEN-MODULE BENCHMARKING AND COMPARATIVE EVALUATION

## 1. Your role

Act as a senior numerical-computing engineer, C library benchmarking specialist, numerical analyst, performance engineer, statistician, and scientific-data visualization specialist.

You are working on the existing NUMERIX C numerical-computation library.

Your task is to significantly expand and improve the previous benchmarking work. The objective is to conduct a rigorous, reproducible, statistically meaningful comparison of NUMERIX against established numerical libraries, including specialist libraries that are particularly strong in individual numerical areas.

This is not merely a task to add a few benchmark functions or graphs.

You must:

1. Audit the current benchmarking infrastructure and previous benchmark results.
2. Identify appropriate competitor libraries for every NUMERIX module.
3. Verify the actual capabilities of each competitor and its APIs.
4. Design a large and mathematically diverse benchmark dataset.
5. Create at least 100 distinct test problems for EACH of NUMERIX's seven modules.
6. Execute the same eligible test problems using NUMERIX and the appropriate competitor libraries.
7. Collect performance, accuracy, reliability, memory, convergence, and scalability measurements.
8. Perform statistically sound comparisons.
9. Generate clean, professional, publication-quality graphs.
10. Produce a complete comparative evaluation report with conclusions, limitations, and reproducibility instructions.

The final work should answer:

- Where is NUMERIX faster?
- Where is NUMERIX slower?
- Where is NUMERIX more accurate?
- Where does NUMERIX produce less accurate results?
- Which library is more reliable for particular problem classes?
- How do the libraries scale as problem sizes increase?
- How much memory does each library use?
- How many function evaluations, iterations, and other computational operations are required?
- How does performance change when comparable accuracy is required?
- Which NUMERIX modules are competitive with established numerical libraries?
- Which modules need further engineering work?

Do not manufacture a universal winner. Conclusions must follow the measured results.

---

# 2. ABSOLUTE PROJECT CONSTRAINTS

These constraints are mandatory.

### 2.1 Preserve NUMERIX

Do not redesign or rewrite NUMERIX.

Do not change production numerical algorithms merely to improve benchmark results.

Do not change the public API, mathematical behavior, numerical tolerances, or solver implementation as part of this benchmark phase.

You may add or modify benchmark-only source files, test-problem generators, adapters, scripts, visualization code, documentation, and report-generation tools.

If you discover a numerical bug in NUMERIX:

- Record the exact problem and reproducible evidence.
- Explain its effect on the benchmark.
- Classify its severity.
- Do not silently patch the numerical algorithm during this phase.
- Keep the affected results in the raw dataset.
- Explain whether the issue prevents a particular comparison from being interpreted fairly.

Numerical corrections should be handled separately before a fresh benchmark run.

### 2.2 Preserve previous benchmark work

The previous Phase 13B, 13C, and 13D outputs are historical records.

Do not overwrite, delete, rename, or regenerate them in place.

Preserve:

- Phase 13 raw timing data.
- Phase 13 accuracy data.
- Previous workload manifests.
- Existing comparison tables.
- Existing graphs.
- Existing reports.
- Existing verification scripts.
- Previous environment information and source fingerprints.

Create a completely separate Phase 14 output hierarchy.

### 2.3 Preserve repository state

Before doing anything, inspect:

- Current branch.
- Current HEAD commit.
- Git working-tree status.
- Tracked and untracked modifications.
- Existing build configuration.
- Current NUMERIX version.
- Current benchmark dependencies.

Previous work reported that benchmark changes were uncommitted and that the working tree was not necessarily clean. Treat the actual current repository state as authoritative.

Do not reset, clean, stash, discard, or overwrite existing user modifications.

Do not create a commit, push to GitHub, create a tag, or bump the project version unless explicitly requested.

NUMERIX should remain at its existing version during this phase.

### 2.4 Formatting and code quality

All newly written C code must use professional, conventional, readable, multi-line formatting.

Use explicit error handling, checked allocation sizes, clear naming, and appropriate comments.

Do not introduce unnecessary dependencies into the NUMERIX production library.

Competitor libraries must remain optional benchmark dependencies, not mandatory NUMERIX runtime dependencies.

---

# 3. THE SEVEN MODULES — ALL MUST BE BENCHMARKED

NUMERIX has seven primary numerical modules.

Every module must have its own workload plan, benchmark results, analysis, and clearly presented visualizations.

Do not omit any module.

| No. | NUMERIX module | Required coverage |
|---|---|---|
| 1 | Root finding | Scalar root-finding methods and applicable multiple-root tests |
| 2 | Linear algebra | Vector/matrix operations, linear-system solving, LU and related supported operations |
| 3 | Numerical differentiation | First derivatives, second derivatives, finite-difference methods, Richardson methods |
| 4 | Interpolation | Linear, polynomial, Lagrange, Newton and other implemented interpolation methods |
| 5 | Numerical integration | Trapezoidal, Simpson's, adaptive Simpson and other implemented quadrature methods |
| 6 | Optimization | Golden-section minimization, gradient-based optimization and other implemented methods |
| 7 | ODE solving | Euler, Heun, RK4, scalar ODEs and supported vector systems |

First inspect the actual NUMERIX public API and source code to identify precisely which methods and problem types are supported.

Do not assume that a feature exists simply because it was in the original project plan.

The comparison must reflect the current implementation.

---

# 4. MINIMUM TEST-PROBLEM REQUIREMENT

This is one of the most important requirements.

## 4.1 Minimum 100 distinct problems per module

Create at least:

**100 distinct mathematical problem instances for EACH of the seven modules.**

That means:

- Root finding: at least 100 problems.
- Linear algebra: at least 100 problems.
- Numerical differentiation: at least 100 problems.
- Interpolation: at least 100 problems.
- Numerical integration: at least 100 problems.
- Optimization: at least 100 problems.
- ODE solving: at least 100 problems.

Minimum total: **700 distinct core mathematical problem instances.**

Preferably target 120–150 distinct instances per module if the quality of the test suite can be maintained.

For every library being compared in a module, run the same eligible problem IDs and input data.

For example:

- NUMERIX versus GSL: at least 100 paired problems in each of the seven modules.
- NUMERIX versus LAPACKE/LAPACK with OpenBLAS: at least 100 paired linear-algebra problems.
- NUMERIX versus NLopt: at least 100 paired optimization problems.
- NUMERIX versus SUNDIALS: at least 100 paired ODE problems.
- NUMERIX versus CMINPACK, if accepted as a suitable root comparator: at least 100 paired scalar root problems.

If a particular competitor genuinely cannot support a problem type, record it as unsupported rather than inventing a result.

Do not claim a category-wide comparison has sufficient coverage when fewer than 100 common, eligible problems were actually completed.

## 4.2 What counts as a distinct problem?

A problem must represent a meaningfully different mathematical test case.

Examples:

- Different mathematical functions.
- Different root structures.
- Different matrix structures and conditioning.
- Different interpolation functions and node distributions.
- Different integration integrands and singularity behavior.
- Different optimization landscapes.
- Different ODE systems, parameters, initial conditions, and time intervals.

Parameter variations can count as distinct cases when they materially change the numerical challenge.

However, do NOT count the following as new mathematical problems:

- Repeating the same problem 21 times for timing.
- Running the same problem with another library.
- Running the same problem using another algorithm.
- Changing only a benchmark label.
- Duplicating identical inputs with different IDs.

Create at least 20 meaningful mathematical problem families per module, with multiple carefully selected instances per family.

Each problem must have:

- Stable unique `problem_id`.
- Module.
- Problem family.
- Mathematical description.
- Input parameters.
- Input size or relevant complexity measure.
- Expected behavior.
- Reference solution or reference value.
- Reference-generation method.
- Numerical tolerance.
- Problem difficulty classification.
- Expected status.
- Eligible competitor libraries.
- Applicable comparison modes.

Keep a master problem manifest.

## 4.3 Problem difficulty and diversity

Do not fill the benchmark with only easy, well-conditioned mathematical examples.

Every module should include:

- Simple baseline cases.
- Moderate cases.
- Difficult cases.
- Small-scale and large-scale cases.
- Numerically well-conditioned cases.
- Ill-conditioned or sensitive cases where applicable.
- Extreme floating-point scale cases.
- Boundary cases.
- Difficult convergence cases.
- Cases that reveal numerical weaknesses.

Include both ordinary practical problems and carefully designed numerical stress problems.

Do not make every problem pathological. The benchmark must represent realistic use as well as numerical edge cases.

---

# 5. COMPETITOR LIBRARY SELECTION

Use multiple established libraries, not just GSL.

Before implementation, investigate each candidate's official documentation, supported algorithms, C APIs, licenses, versions, build requirements, and platform limitations.

Create a library capability matrix.

## 5.1 Broad numerical library — GNU Scientific Library

**GSL is mandatory.**

Use GSL as the primary broad numerical-library comparator.

Its benchmark coverage must include all seven NUMERIX modules where equivalent functionality exists:

1. Root finding.
2. Linear algebra.
3. Numerical differentiation.
4. Interpolation.
5. Numerical integration.
6. Optimization/minimization.
7. ODE solving.

Verify the exact methods available in the installed GSL version.

Do not assume that two methods are algorithmically identical merely because they solve the same mathematical problem.

Record:

- GSL version.
- Build configuration.
- Compiler.
- Optimization flags.
- Linked BLAS implementation.
- Whether the build is static or shared.
- Relevant runtime settings.

## 5.2 Linear algebra specialist — LAPACK / LAPACKE / OpenBLAS

This comparison is mandatory.

NUMERIX must be compared against a specialist linear-algebra implementation.

Investigate and benchmark:

**A. Reference LAPACK with LAPACKE and reference BLAS**

Use the reference LAPACK implementation with its C interface.

**B. LAPACK/LAPACKE with OpenBLAS**

Use the LAPACKE interface backed by an optimized OpenBLAS implementation.

These are two different configurations and must be labelled accurately.

Do not describe OpenBLAS alone as a complete replacement for every LAPACK solver.

Use supported, equivalent operations:

- Dense linear-system solving.
- LU factorization.
- LU-based solving.
- Matrix/vector operations where NUMERIX has equivalent functionality.
- Other operations only where NUMERIX actually provides comparable support.

For example:

- NUMERIX LU versus LAPACK `dgetrf`.
- NUMERIX solve versus the appropriate LAPACK/LAPACKE solve path.
- NUMERIX vector/matrix operations versus equivalent BLAS operations.

For Gauss-Jordan and methods without a direct equivalent, use a clearly labelled best-available comparison.

Separate:

1. Factorization time.
2. Solve time.
3. Factorization plus solve time.
4. Input/output conversion overhead.
5. Total end-to-end time.

Test different matrix dimensions, structures, and condition numbers.

Record memory layout, row/column-major conversion, workspace, and allocation policies.

Do not hide conversion overhead. Show solver-only and end-to-end results separately.

## 5.3 Optimization specialist — NLopt

NLopt is mandatory for the optimization comparison if its C API can be built and validated in the environment.

Investigate suitable algorithms for the actual optimization functionality supported by NUMERIX.

Potential comparison categories include:

- Derivative-free optimization.
- Gradient-based optimization.
- Bounded minimization.
- One-dimensional optimization where comparable.
- Multidimensional optimization only if the relevant NUMERIX API supports the same problem dimension.

Compare:

- NUMERIX golden-section method with an equivalent method where available.
- NUMERIX gradient-based method with appropriate gradient-based alternatives where comparable.
- NLopt's best-available method for the same objective and constraints.

Do not force a multidimensional comparison if NUMERIX only supports one-dimensional optimization.

Clearly distinguish algorithm-equivalent and best-available results.

## 5.4 ODE specialist — SUNDIALS

SUNDIALS is mandatory for the ODE comparison if its required C components can be built and tested.

Investigate:

- CVODE.
- ARKODE.
- Relevant explicit and implicit integration methods.
- Available serial-vector configuration.

Compare the actual NUMERIX supported methods against:

- Equivalent methods where one truly exists.
- Appropriate best-available methods at equivalent accuracy targets.

NUMERIX currently has fixed-step Euler, Heun, RK4, and vector-system functionality.

Do not compare an adaptive solver against a fixed-step solver and call the result an identical-algorithm comparison.

Separate fixed-step comparison from adaptive/best-available comparison.

Use the same ODE definitions, initial conditions, interval, and reference solutions.

## 5.5 Additional root solver candidate — CMINPACK

Evaluate CMINPACK as an additional root-solving comparator.

It implements MINPACK nonlinear equation solvers through a C interface.

A one-dimensional nonlinear equation can be represented as a one-variable system.

However, CMINPACK's hybrid-system method is not equivalent to all NUMERIX scalar bracketing methods.

Therefore:

- Include it only after verifying the actual C API and behavior.
- Use one-dimensional problems with the same mathematical function.
- Classify it as a best-available/local nonlinear-system comparison.
- Do not claim it is equivalent to Bisection, Brent, or another bracketed algorithm.
- Document its method and limitations.

If it cannot be integrated reliably, explain why and keep GSL as the primary root competitor.

## 5.6 Additional-library research for remaining modules

Investigate whether there are other mature, actively maintained, C-callable libraries that are genuinely appropriate for:

- Numerical differentiation.
- Interpolation.
- One-dimensional integration.
- Root finding.

Do not add a library merely to increase the number of competitors.

Do not silently treat C++ libraries as native C libraries.

For example:

- ALGLIB has broad numerical functionality, but verify its actual language interface before treating it as a C library.
- Boost.Math is C++, not a native C API.
- Cuba primarily addresses multidimensional integration; do not use it as a direct equivalent to NUMERIX's one-dimensional quadrature without a separate, clearly defined multidimensional benchmark.

You may list such libraries as possible future comparisons, but the main study must accurately identify which libraries are native C or have a documented C API.

If no appropriate additional C competitor exists for a module, compare NUMERIX with GSL and explicitly document the lack of a suitable second specialist C library.

## 5.7 Library capability matrix

Create a table containing:

| Library | Language/API | Module | Algorithms | Version | License | Build status | Comparison type |
|---|---|---|---|---|---|---|---|

Also include:

- Official project URL.
- Documentation URL.
- Version used.
- Build flags.
- External dependencies.
- Whether a native C API is used.
- Whether a wrapper was needed.
- Unsupported functionality.
- Exact reason for excluding any candidate.

Do not claim a library was tested if it was only researched.

---

# 6. TWO SEPARATE COMPARISON MODES

Every comparison must be assigned one of these modes.

## Mode A — Algorithm-equivalent comparison

Compare the same or genuinely equivalent algorithm using the same problem, input, numerical precision, and comparable stopping criteria.

Examples:

- Bisection versus Bisection.
- LU factorization versus LU factorization.
- Golden-section minimization versus Golden-section minimization.
- Fixed-step RK4 versus the same fixed-step RK4 implementation.

A method must only be labelled algorithm-equivalent after verifying the algorithm, not merely its name.

## Mode B — Best-available comparison

Compare the best appropriate algorithm available in each library for the same mathematical problem.

Use:

- The same input.
- The same reference answer.
- A common numerical accuracy target.
- Comparable evaluation or resource budgets.
- Clearly documented algorithm choices.

For example:

- NUMERIX adaptive Simpson versus GSL QAG/CQUAD.
- NUMERIX adaptive fixed-step strategy versus a suitable adaptive ODE solver.
- NUMERIX root method versus the most appropriate GSL or CMINPACK method.

This comparison measures the result of choosing a library and its suitable algorithm, not pure implementation speed.

Do not combine Mode A and Mode B in the same aggregate performance result.

Do not report an algorithm-mismatched comparison as a same-algorithm speed test.

---

# 7. FAIRNESS AND BENCHMARKING METHODOLOGY

The methodology must be consistent across libraries.

## 7.1 Common inputs

Every library must receive exactly the same mathematical problem and input data for a paired comparison.

Use identical:

- Function definitions.
- Matrix data.
- Initial guesses.
- Integration limits.
- Interpolation data.
- Optimization objective.
- Constraints.
- ODE initial conditions.
- ODE time intervals.
- Numerical precision.
- Accuracy targets.

For callback-based algorithms, use equivalent callback functions and the same mathematical implementation.

Use the NUMERIX callback interface as the common comparison path.

Do not mix expression-engine parsing overhead into an ordinary callback comparison.

Benchmark NUMERIX expression-based inputs separately as a NUMERIX-specific capability and overhead study.

## 7.2 Numerical tolerance

Use both absolute and relative error.

Define the correctness test as:

`absolute_error <= absolute_tolerance + relative_tolerance * abs(reference_value)`

Use appropriate tolerances for different problem classes and output scales.

Do not use one arbitrary tolerance for all numerical problems.

Pre-register the tolerance values in the workload manifest before running the final benchmark.

Do not change tolerances after seeing which library performs better.

For vector and matrix results, use appropriate norms.

For linear systems, report residuals and backward error, not just element-wise error.

## 7.3 Reference answers

The benchmark must use independent references.

Preferred reference sources:

1. Exact analytical solution.
2. High-precision computation.
3. Independently validated numerical reference.
4. A carefully verified convergence study where an exact solution is unavailable.

Do not use the output of one competing library as the unquestioned reference for another.

Record the reference-generation method and its reliability.

Use stable formulas when generating reference data. Avoid introducing avoidable rounding errors into test inputs.

## 7.4 Compiler and build settings

Use a consistent compiler and optimization level for the primary comparison.

Recommended baseline:

- C11-compatible compiler.
- Release build.
- `-O3 -DNDEBUG`.
- No `-ffast-math`.
- Double-precision inputs and outputs where supported.
- One thread per library.
- Same machine.
- Same operating-system environment.

Record the complete compiler flags and dependency versions.

Use single-thread execution for the primary benchmark.

For OpenBLAS, control its thread count explicitly.

Do not allow one library to use several cores while another uses one core.

A separate multithreaded comparison may be added later, but it must never be mixed with the primary single-thread results.

## 7.5 Timing

Measure both:

- Wall-clock time.
- CPU time.

Use appropriate monotonic high-resolution timers.

Timing procedure:

1. Initialize the required library and problem.
2. Perform warm-up runs.
3. Calibrate batch sizes.
4. Run at least 21 independent timing batches per workload configuration. Prefer 31 when practical.
5. Preserve every raw timing batch.
6. Randomize or counterbalance paired library execution order using a recorded deterministic seed.
7. Avoid systematic NUMERIX-first or competitor-first bias.
8. Record batch size and elapsed time.

Record:

- Minimum.
- Median.
- Mean.
- Standard deviation.
- P95.
- Maximum.
- Number of samples.
- Batch size.

Do not discard unusual timing measurements without a documented technical reason.

Do not use one aggregate runtime for problems with very different sizes.

## 7.6 Separate initialization and operation costs

Measure separately:

- Library initialization.
- Problem construction.
- Workspace allocation.
- Input conversion.
- Solver execution.
- Output conversion.
- Total end-to-end execution.

Where persistent setup is required, distinguish setup-inclusive and solver-only measurements.

For LAPACK, explicitly separate layout conversion from the underlying operation.

## 7.7 Function evaluations and computational work

Collect relevant operation counts.

Depending on the module, record:

- Function evaluations.
- Derivative evaluations.
- Jacobian evaluations.
- Iterations.
- RHS evaluations for ODEs.
- Linear solves.
- Factorization operations where available.
- Subinterval or panel counts.
- Optimization objective evaluations.
- Number of interpolation evaluations.

Use callback instrumentation in separate untimed runs when counting operations would distort timing.

If an exact counter is unavailable, record the metric as unavailable. Never use zero to represent missing data.

Do not claim estimated FLOP counts as measured operation counts.

## 7.8 Memory

Memory comparison is mandatory.

Measure two different memory concepts and keep them separate.

**A. Peak process resident memory**

Where supported, execute each library/problem workload in an isolated child process and measure peak resident set size.

Use consistent process-launch and measurement procedures.

Record platform and measurement method.

**B. Requested heap allocation**

Use allocator instrumentation where possible to measure:

- Total requested allocation.
- Peak live requested heap bytes.
- Allocation count.
- Reallocation count.
- Free count.
- Remaining live requested bytes after cleanup.

This is not the same as process RSS.

Clearly document that requested heap measurements exclude stack use, allocator metadata, unobserved external allocations, and other process overhead.

Do not compare incomplete allocator-interception figures as though they represented total memory use.

If an external library cannot be instrumented reliably, use process RSS and report the limitation.

Measure memory for representative small, medium, and large problems.

---

# 8. DESIGN THE TEST SUITES FOR EACH MODULE

Create a dedicated workload design for every module.

## 8.1 Root finding — at least 100 problems

Include families such as:

- Linear roots.
- Quadratic roots.
- Higher-degree polynomial roots.
- Trigonometric functions.
- Exponential functions.
- Logarithmic functions.
- Rational functions.
- Mixed transcendental functions.
- Multiple roots.
- Repeated roots.
- Tangential roots.
- Flat functions near roots.
- Closely spaced roots.
- Scaled functions.
- Roots near interval boundaries.
- Difficult initial guesses.
- Poorly conditioned roots.
- Functions with poles or discontinuities.
- Problems with no root in the specified interval.

Vary:

- Function scale.
- Root location.
- Root multiplicity.
- Initial guess.
- Bracketing interval.
- Tolerance.
- Function shape.

Measure:

- Absolute root-location error.
- Relative root-location error.
- Final residual.
- Function evaluations.
- Iterations.
- Convergence status.
- Success rate.
- Accuracy pass rate.
- Runtime.
- Failure classification.

For NUMERIX multiple-root discovery, report the following separately:

- Root detection recall.
- False positives.
- Root-location error.
- Multiplicity-estimation behavior.
- Unresolved-region count.

Do not pretend that every other library supports NUMERIX's full multiple-root-discovery feature.

## 8.2 Linear algebra — at least 100 problems

Include different matrix sizes and structures.

Examples:

- Small dense matrices.
- Medium dense matrices.
- Large dense matrices.
- Diagonally dominant matrices.
- Well-conditioned matrices.
- Ill-conditioned matrices.
- Near-singular matrices.
- Singular matrices.
- Scaled matrices.
- Mixed-row-scale matrices.
- Random deterministic matrices.
- Symmetric matrices where the supported methods permit.
- Matrices with different pivoting requirements.

Vary dimensions over a meaningful range, including sufficiently large sizes to show scaling.

Choose sizes that can be completed within reasonable benchmark limits.

Generate systems with known solutions:

`A * x_reference = b`

Use carefully controlled matrix generation and calculate reference data with sufficient precision.

Measure:

- Factorization time.
- Solve time.
- Total factor-and-solve time.
- Relative forward solution error.
- Residual vector.
- Residual infinity norm.
- Normwise backward error.
- Condition number or a reference condition estimate.
- Pivot behavior.
- Iterations or work metrics where applicable.
- Memory usage.
- Matrix-size scalability.

Report singular and near-singular cases as reliability results, not as successful timing results.

Do not automatically treat every rejected ill-conditioned matrix as a defect. Assess whether the numerical rejection is appropriate.

## 8.3 Numerical differentiation — at least 100 problems

Include:

- Polynomial derivatives.
- Trigonometric derivatives.
- Exponential derivatives.
- Logarithmic derivatives.
- Mixed functions.
- High-curvature functions.
- Slowly varying functions.
- Large-scale functions.
- Small-scale functions.
- Oscillatory functions.
- Functions with cancellation sensitivity.
- Functions near boundaries.
- One-sided differentiation cases.
- Second-derivative cases.
- Difficult finite-difference step sizes.

Use exact analytical derivatives where possible.

Measure:

- Absolute derivative error.
- Relative derivative error.
- Error versus function evaluations.
- Error versus runtime.
- Error estimate versus actual error.
- Function-evaluation count.
- Sensitivity to step size.
- Success and failure rates.

Clearly state whether a library's error estimate is heuristic or otherwise documented.

Do not compare reported error estimates as if they were actual errors.

## 8.4 Interpolation — at least 100 problems

Include:

- Linear interpolation.
- Polynomial interpolation.
- Lagrange interpolation.
- Newton interpolation.
- Smooth functions.
- Oscillatory functions.
- Exponential functions.
- Trigonometric functions.
- High-degree polynomial cases.
- Nonuniform node spacing.
- Uniform node spacing.
- Clustered nodes.
- Sparse nodes.
- Dense nodes.
- Poorly distributed nodes.
- Extrapolation cases where supported.

Use different node counts and distributions.

Measure the interpolation error on an independent validation grid, not merely at the original interpolation nodes.

Record:

- Construction time.
- Evaluation time.
- Total time.
- Maximum absolute interpolation error.
- Maximum relative interpolation error.
- RMS error over the validation grid.
- Node count.
- Memory.
- Stability with increasing polynomial degree.

Keep construction and evaluation costs separate.

## 8.5 Numerical integration — at least 100 problems

Include:

- Smooth polynomial integrands.
- Trigonometric integrands.
- Exponential integrands.
- Logarithmic integrands.
- Rational integrands.
- Oscillatory integrands.
- Narrow peaks.
- Broad peaks.
- Difficult endpoint behavior.
- Integrable endpoint singularities.
- Known interior discontinuities.
- Piecewise functions.
- Large-magnitude integrands.
- Small-magnitude integrands.
- Different finite intervals.

Include infinite or semi-infinite domains only where the NUMERIX implementation genuinely supports them.

Use exact analytical integrals where available.

Measure:

- Absolute integration error.
- Relative integration error.
- Function evaluations.
- Number of intervals/panels.
- Runtime.
- Memory.
- Convergence status.
- Requested tolerance.
- Returned estimated error.
- Actual error.
- Accuracy pass rate.

Perform an error-estimate reliability analysis.

Important: compare actual error against estimated error.

Determine whether the reported numerical error estimate is generally conservative, optimistic, or unreliable.

Keep failed integrations and accuracy misses in the report.

## 8.6 Optimization — at least 100 problems

First inspect the precise NUMERIX optimization API and supported dimensions.

Include problems that it can genuinely solve.

For one-dimensional optimization, include:

- Quadratic objectives.
- Polynomial objectives.
- Smooth convex functions.
- Nonconvex functions.
- Multimodal functions.
- Narrow minima.
- Flat minima.
- Poorly scaled minima.
- Asymmetric functions.
- Bounded objectives.
- Difficult initial intervals.
- Boundary minima.
- Different curvature scales.

If the NUMERIX implementation supports multidimensional objectives, create a separate multidimensional workload.

Measure:

- Absolute distance from reference minimizer.
- Objective-value error or optimality gap.
- Gradient norm where meaningful.
- Bound/constraint violation.
- Function evaluations.
- Iterations.
- Runtime.
- Convergence status.
- Success rate.
- Memory.

For multimodal functions, distinguish local minimization success from verified global minimization.

Do not claim a global optimum unless the reference solution is actually known or independently established.

## 8.7 ODE solving — at least 100 problems

Include scalar and supported vector systems.

Include problem families such as:

- Constant derivatives.
- Linear ODEs.
- Exponential growth and decay.
- Harmonic oscillators.
- Damped oscillators.
- Coupled linear systems.
- Nonlinear systems with analytical solutions.
- Nonlinear systems with validated numerical references.
- Different initial conditions.
- Different time intervals.
- Different rates of change.
- Different time-step sizes.
- Difficult but valid integration cases.
- Stiff problems as a separate stress group where applicable.

Do not treat a stiff-problem failure of a fixed-step explicit solver as equivalent to an adaptive solver failure without considering the method's intended capabilities.

Measure:

- Global trajectory error.
- Final-state error.
- Maximum trajectory error.
- RMS trajectory error.
- RHS evaluations.
- Number of time steps.
- Runtime.
- Requested and achieved accuracy.
- Stability behavior.
- Memory.
- Success and failure rates.

For adaptive competitors, record their actual error-control tolerances and achieved accuracy.

Keep fixed-step and adaptive comparisons separate.

---

# 9. METRICS THAT MUST BE COLLECTED

The report must not be based on runtime alone.

Create a metric inventory and ensure every available metric is collected consistently.

## 9.1 Correctness and numerical accuracy

Required:

- Absolute error.
- Relative error.
- Forward error.
- Residual error.
- Backward error where applicable.
- Maximum error.
- RMS error where applicable.
- Accuracy pass/fail.
- Error-estimate reliability where available.

## 9.2 Runtime and performance

Required:

- Wall-clock time.
- CPU time.
- Minimum.
- Median.
- Mean.
- Standard deviation.
- P95.
- Maximum.
- Runtime scaling.
- Solver-only time.
- End-to-end time.
- Initialization time where meaningful.

## 9.3 Reliability

Required:

- Solver success rate.
- Accuracy pass rate.
- Numerical failure rate.
- Invalid-input behavior.
- Non-finite output rate.
- Timeout rate.
- Stagnation rate.
- Allocation failure behavior where testable.
- Unresolved/convergence failure categories.

**Status success and accuracy success are separate measurements.**

A library returning a success status with an inaccurate answer must not be counted as an accurate success.

## 9.4 Computational effort

Required where supported:

- Function evaluations.
- Derivative evaluations.
- Jacobian evaluations.
- Iteration count.
- RHS evaluations.
- Subinterval/panel counts.
- Linear factorization and solve work.
- Objective evaluations.

## 9.5 Memory

Required:

- Peak process RSS.
- Requested heap allocation metrics where reliable.
- Peak live requested bytes.
- Allocation count.
- Memory scaling with problem size.

## 9.6 Scalability

Required:

- Runtime versus problem size.
- Memory versus problem size.
- Accuracy versus problem size.
- Function evaluations versus problem difficulty.
- Runtime growth rate where meaningful.
- Time-to-accuracy.

Do not manufacture unavailable metrics.

Use explicit missing values and explanations.

---

# 10. STATISTICAL ANALYSIS

A reliable statistical analysis is mandatory.

Do not simply average every row across every problem and announce a winner.

## 10.1 Paired comparisons

Use the same `problem_id` for every library in a comparison.

Calculate paired performance ratios.

Define the speedup ratio explicitly.

For example:

`speedup = competitor_median_time / NUMERIX_median_time`

Interpretation:

- Greater than 1: NUMERIX is faster.
- Less than 1: competitor is faster.
- Equal to 1: equal median runtime.

Use this definition consistently in all tables and graphs.

## 10.2 Confidence intervals

Calculate 95% confidence intervals for major paired comparisons.

Prefer bootstrap resampling across independent problem IDs rather than pretending repeated timing batches are independent mathematical problems.

Preserve the paired nature of observations.

Report:

- Number of paired problems.
- Median paired ratio.
- Geometric mean paired ratio where appropriate.
- 95% confidence interval.
- Number of NUMERIX wins, losses, and ties.

Do not combine unrelated algorithms, input sizes, or problem families into a misleading overall average.

## 10.3 Separate problem classes

Analyze different categories independently.

For example:

- Easy versus difficult root problems.
- Well-conditioned versus ill-conditioned matrices.
- Small versus large matrices.
- Smooth versus singular integration functions.
- Convex versus multimodal optimization.
- Nonstiff versus stiff ODE systems.

A library may excel on one problem family and perform poorly on another.

Expose that difference.

## 10.4 Avoid selection bias

Do not choose the winning algorithm for every library after inspecting the final benchmark results and then claim an unbiased best-available comparison.

Predefine method-selection rules based on documented capabilities and a separate tuning subset, or use a clearly documented fixed method mapping.

Where hyperparameters need tuning, separate tuning problems from final evaluation problems.

Keep final test problems independent of method-selection tuning.

---

# 11. GRAPH AND VISUALIZATION REQUIREMENTS

This section is critical.

The previous graphs were not sufficiently clear or consistently designed.

Do not simply regenerate the same charts with different colors.

Design a new visualization system around the questions each graph should answer.

Every graph must be mathematically meaningful, visually clean, professionally formatted, and directly supported by verified data.

Use Python with Matplotlib for the graphs.

Do not use Seaborn.

Generate both PNG and SVG versions.

Use consistent fonts, typography, sizing, spacing, and a professional color palette.

Use readable figure dimensions appropriate for reports and presentation slides.

## 11.1 Global overview graphs

Create the following where sufficient data exists.

### Graph 1 — Benchmark coverage matrix

Show:

- NUMERIX modules as rows.
- Libraries as columns.
- Number of distinct eligible problems.
- Number of paired completed problems.
- Number of accuracy-passing problems.

This is a coverage visualization, not a performance ranking.

It must make missing module coverage obvious.

### Graph 2 — Accuracy pass rate by module and library

Show the percentage of problems satisfying the predefined correctness tolerance.

Display:

- Library.
- Module.
- Percentage.
- Number attempted.
- Number accurate.

Do not merge status success and accuracy success.

### Graph 3 — Solver reliability comparison

Show:

- Successful statuses.
- Numerical failures.
- Nonconvergence.
- Timeouts.
- Other relevant failure classes.

Use consistent definitions across the libraries.

### Graph 4 — NUMERIX speedup by competitor and module

Use a forest/dot plot with 95% confidence intervals.

Show:

- Module.
- Competitor.
- Algorithm-equivalent or best-available mode.
- Paired problem count.
- Paired speedup ratio.
- Confidence interval.

Draw a clearly labelled equality reference line at 1.

The graph title and axis must explicitly explain which direction favors NUMERIX.

Do not include comparisons with insufficient paired coverage without clearly marking them.

### Graph 5 — Runtime scalability overview

Create separate small panels for the seven modules.

Show median runtime versus a meaningful problem-size measure.

Use log scales where justified.

Do not combine incomparable dimensions into one axis.

### Graph 6 — Memory scalability overview

Show peak process RSS or other clearly identified memory metric versus problem size.

Do not mix requested heap bytes and process RSS in the same measurement series.

### Graph 7 — Accuracy distribution

Show error distributions for the libraries.

Use an appropriate logarithmic error axis where necessary.

Make units and error definitions explicit.

### Graph 8 — Time-to-accuracy / accuracy-performance trade-off

Show runtime against actual numerical error.

Indicate the predefined accuracy target.

This graph should reveal whether one library is faster because it is less accurate.

## 11.2 Root-finding graphs

Create:

1. Root-location error versus function-evaluation count.
2. Runtime distribution by method and root-problem family.
3. Convergence and accuracy success rates by problem difficulty.
4. Root-location error distribution.
5. Scaling and behavior for multiple-root or difficult root problems where meaningful.

## 11.3 Linear-algebra graphs

Create:

1. Runtime versus matrix dimension.
2. Factorization time versus matrix dimension.
3. Solve-only time versus matrix dimension.
4. Factorization-plus-solve time versus matrix dimension.
5. Relative forward error versus condition number.
6. Backward error distribution.
7. Peak memory versus matrix dimension.

Clearly distinguish reference LAPACK, OpenBLAS-backed LAPACK, GSL, and NUMERIX.

## 11.4 Differentiation graphs

Create:

1. Derivative error versus function-evaluation count.
2. Derivative error distribution by library/method.
3. Runtime versus derivative accuracy.
4. Actual error versus reported estimated error where available.

## 11.5 Interpolation graphs

Create:

1. Maximum interpolation error versus node count.
2. RMS validation-grid error versus node count.
3. Construction time versus node count.
4. Evaluation time versus node count.
5. Error distribution by interpolation method.

Use independent validation points.

## 11.6 Integration graphs

Create:

1. Actual integration error versus function evaluations.
2. Actual error versus runtime.
3. Function evaluations versus requested tolerance.
4. Runtime versus requested tolerance.
5. Reported estimated error versus actual error.
6. Accuracy pass rate by integrand family.

For error-estimate calibration, include a clearly labelled ideal-reference line.

Do not present requested tolerance as though it were achieved accuracy.

## 11.7 Optimization graphs

Create:

1. Objective optimality gap versus function evaluations.
2. Objective optimality gap versus runtime.
3. Convergence/accuracy success rate by objective family.
4. Runtime comparison by optimization algorithm.
5. Iterations and function evaluations versus problem difficulty.

## 11.8 ODE graphs

Create:

1. Global trajectory error versus RHS evaluations.
2. Final-state error versus runtime.
3. Runtime versus number of steps.
4. Error versus step size.
5. Accuracy pass rate by ODE family.
6. Runtime/accuracy comparison for fixed-step versus adaptive methods, clearly separated.

## 11.9 Design rules for EVERY graph

Every graph must have:

- A descriptive title.
- A subtitle when needed to explain methodology.
- Correct X-axis name.
- Correct Y-axis name.
- Units.
- Readable tick labels.
- Clearly identified libraries.
- A meaningful legend.
- Appropriate error bars or confidence intervals where applicable.
- Sample size or paired-problem count.
- Appropriate scale.
- Correct numerical formatting.
- No overlapping text.
- No clipped titles or labels.
- No unexplained abbreviations.
- No decorative elements that obscure data.

Use consistent library colors across the entire report.

Use colorblind-friendly and print-friendly choices.

Prefer direct labels where that improves clarity.

Use subtle gridlines and whitespace.

Use `constrained_layout` or carefully controlled layout adjustments.

Test every figure at its final display size.

Do not use:

- 3D graphs without a real analytical reason.
- Pie charts for performance comparisons.
- Unlabelled axes.
- Arbitrary normalized scores without explaining their formula.
- Mixed-unit axes.
- Overcrowded legends.
- Unreadable annotations.
- Graphs that combine algorithms that cannot be fairly compared.
- Trend lines that imply unsupported interpolation.
- Bar charts that conceal wide variations across problem instances.

Do not force every available metric into one chart.

If a graph cannot answer a specific question, do not create it.

## 11.10 Combined comparison graphs

Create combined graphs where they genuinely help.

Examples:

- All libraries in one module's runtime comparison.
- All modules in a normalized speedup overview.
- All libraries in an accuracy heatmap.
- Separate small-multiple runtime scaling panels for the seven modules.

Do not put raw seconds, bytes, error magnitudes, and iteration counts on a single shared axis.

---

# 12. DATA COLLECTION AND STORAGE DESIGN

The data architecture must preserve the evidence behind every graph.

Use a versioned benchmark schema.

Extend the previous schema without breaking the ability to verify Phase 9, Phase 10, or Phase 13 historical results.

Do not reuse the old schema if it cannot accurately represent the additional measurements.

Create a new schema version for Phase 14 where necessary.

Every measurement must be traceable to:

- Run ID.
- Library.
- Library version.
- NUMERIX version.
- Module.
- Problem ID.
- Problem family.
- Method.
- Comparison mode.
- Input size.
- Tolerance.
- Compiler.
- Build configuration.
- Machine.
- Timing batch.
- Measurement type.

Suggested Phase 14 artifacts:

### Raw timing data

`raw_timing.csv`

Store individual timing-batch observations, not just summary statistics.

### Accuracy results

`accuracy.csv`

Store independent, untimed accuracy-probe results.

### Work-count data

`work_metrics.csv`

Store callback counts, iterations, RHS evaluations, and applicable computational work.

### Memory data

`memory.csv`

Store peak RSS and requested-heap measurements in separately identified columns.

### Problem manifest

`workloads.csv`

Store all unique mathematical test problems and their reference data.

### Library manifest

`libraries.csv`

Store library versions, APIs, licenses, compiler/build configuration, and benchmark eligibility.

### Method mapping

`method_mapping.csv`

Explain which algorithms are considered equivalent and which use best-available comparisons.

### Summary data

`measurements.csv`

Store verified derived metrics.

### Comparison data

`comparisons.csv`

Store paired ratios, confidence intervals, accuracy eligibility, and exclusions.

### Run metadata

Record:

- Machine.
- CPU.
- Number of cores.
- Operating system.
- Compiler.
- Compiler flags.
- Build configuration.
- Timing source.
- Thread count.
- Dependency versions.
- Source fingerprints.
- Git state.
- Seeds.
- Date and time.
- Benchmark configuration.

Missing values must be represented explicitly.

Do not encode missing measurements as zero.

Maintain deterministic IDs and seeds.

Preserve all raw failures and ineligible observations.

---

# 13. BENCHMARK EXECUTION INFRASTRUCTURE

Design adapters so each competitor library can be tested using a common interface.

The adapters must:

- Preserve each library's native method behavior.
- Convert inputs and outputs correctly.
- Track statuses accurately.
- Capture relevant error estimates.
- Count function evaluations in separate untimed runs where possible.
- Measure end-to-end and solver-only time.
- Avoid changing a competitor's default behavior without documentation.

Use separate executable targets or isolated adapter modules where necessary.

A missing optional dependency must not break the NUMERIX build.

Add optional configuration switches for competitor libraries.

For example, benchmarking should be able to detect whether GSL, LAPACKE, OpenBLAS, NLopt, SUNDIALS, and CMINPACK are available.

Do not hardcode local dependency paths into the source code.

Use configurable CMake options, environment variables, or documented dependency discovery.

Use the actual native C API for the primary benchmark.

Do not introduce Python or C++ wrappers into the primary runtime comparison.

Python may be used for reference-data generation, verification, statistics, analysis, and graph production.

If a library cannot be built or tested in the current environment:

- Record the exact reason.
- Save the relevant logs.
- Continue with other eligible libraries.
- Do not invent missing results.
- Do not claim complete competitor coverage.

---

# 14. SPECIAL NUMERICAL RELIABILITY CHECKS

A high-quality benchmark should not only test ordinary inputs.

Add dedicated stress and reliability suites.

Examples:

- Very small and very large magnitudes.
- Values near floating-point limits.
- Cancellation-sensitive expressions.
- Near-singular matrices.
- Ill-conditioned systems.
- Invalid brackets.
- Multiple roots.
- Flat derivatives.
- Tiny finite-difference steps.
- Interpolation nodes that are poorly distributed.
- Difficult integrand endpoints.
- Narrow peaks.
- Functions with discontinuities.
- ODE time-step stagnation.
- ODE stability problems.
- Optimization plateaus.
- Invalid and non-finite input cases.

For each stress family, record:

- Expected mathematical behavior.
- Whether the case is within the library's documented domain.
- Returned status.
- Whether the output is finite.
- Actual accuracy.
- Failure category.
- Whether the problem is an algorithm limitation or an implementation defect.

Keep stress-test results separate from ordinary performance conclusions.

Do not delete failed or inaccurate observations because they make the graphs less attractive.

---

# 15. REPORT STRUCTURE

Create a full report:

`documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.md`

Also generate:

`documentation/reports/NUMERIX_expanded_multi_library_comparative_evaluation.html`

The report must be professional, detailed, readable, and supported by the saved data.

Use the following structure.

## 1. Executive summary

Explain:

- What was compared.
- Which libraries were tested.
- Which seven modules were covered.
- How many unique problems were tested.
- Which metrics were measured.
- The major findings.
- The limitations.

## 2. NUMERIX overview

Describe the current seven modules and their implemented methods.

Do not claim unsupported features.

## 3. Competitor libraries

Explain each library, its relevant strengths, APIs, versions, and why it was chosen.

Include the capability matrix.

## 4. Experimental methodology

Explain:

- Problem selection.
- Test generation.
- Reference solutions.
- Numerical tolerances.
- Algorithm-equivalent comparisons.
- Best-available comparisons.
- Timing procedure.
- Memory measurement.
- Statistical analysis.

## 5. Benchmark coverage

Show the full problem-count matrix.

Demonstrate that the minimum problem count was actually satisfied.

## 6. Results by module

Provide a separate detailed section for each of the seven modules.

Each module must include:

- Libraries compared.
- Methods tested.
- Number of distinct problems.
- Number of successful runs.
- Number of accurate results.
- Runtime statistics.
- Accuracy statistics.
- Reliability statistics.
- Memory statistics.
- Scalability results.
- Important graphs.
- Key observations.
- Limitations.

## 7. Cross-module comparison

Provide clearly labelled combined comparisons.

Do not manufacture a single overall score unless there is a defensible, transparent methodology.

## 8. Accuracy analysis

Identify which libraries produce the most accurate results on particular problem families.

Distinguish forward error, residuals, backward error, and estimated errors.

## 9. Performance analysis

Explain where NUMERIX is faster and slower.

Discuss algorithm choice separately from implementation efficiency.

## 10. Memory analysis

Explain the actual memory measurements, what they include, and what they exclude.

## 11. Reliability analysis

Discuss failures, nonconvergence, invalid results, and accuracy misses.

## 12. Scalability analysis

Explain how execution time and memory grow with problem size.

## 13. Important observations

Identify strong areas and weak areas in NUMERIX.

Do not hide negative results.

## 14. Limitations

Include limitations in:

- Benchmark coverage.
- Competitor availability.
- Algorithm equivalence.
- Platform validation.
- Memory instrumentation.
- Timing methodology.
- Numerical references.
- Problem dimensions and supported functionality.

## 15. Recommendations

Identify which NUMERIX modules deserve future engineering attention based on the evidence.

Do not automatically recommend implementing algorithms just because another library has them.

## 16. Reproducibility

Include:

- Exact commands.
- Build instructions.
- Dependency versions.
- Run IDs.
- Raw-data locations.
- Analysis commands.
- Graph-generation commands.
- Report-generation commands.
- Verification results.

---

# 16. REQUIRED OUTPUT DIRECTORY STRUCTURE

Create a separate Phase 14 hierarchy.

Use a structure similar to:

`benchmarks/`
- `adapters/`
- `problems/`
- `results/`
  - `phase14/`
    - `main/`
    - `pilot/`
    - `memory/`
    - `logs/`
- `analysis/`
  - `phase14/`
    - `scripts/`
    - `tables/`
    - `figures/`
      - `png/`
      - `svg/`
    - `tests/`
- `documentation/` or the existing project documentation structure.

Create the final completion report:

`documentation/development/phase_14_completion_report.md`

Use sensible names if the existing repository has a different established benchmark organization.

Do not duplicate the entire dataset unnecessarily.

All figures must be reproducible from saved raw data.

All tables must be reproducible from saved raw data.

The HTML report must be reproducible from saved analysis outputs.

---

# 17. VERIFICATION AND TESTING

Do not consider the phase complete simply because a benchmark executable ran.

Verify all of the following:

### Benchmark correctness

- Every required module has at least 100 distinct mathematical problems.
- Library/problem mappings are correct.
- Identical problem IDs receive identical mathematical inputs.
- References are valid.
- Tolerances are recorded.
- Algorithm-equivalence classifications are justified.
- Best-available comparisons are separately classified.

### Data correctness

- Every timing result links to a valid problem.
- Every accuracy result links to a valid problem and library.
- Every summary can be reconstructed from raw samples.
- All missing values remain distinguishable from zero.
- Failed runs remain recorded.
- Paired ratios are calculated correctly.
- Confidence intervals are reproducible.

### Graph correctness

- Graph values agree with the source tables.
- Axis units are correct.
- Legends identify the correct libraries and algorithms.
- Sample counts are correct.
- Confidence intervals match the analysis.
- No unsupported observations are displayed.
- No charts are generated from incomplete or stale tables.
- All PNG and SVG files render properly.

### Reproducibility

- Re-running analysis reproduces the same tables.
- Re-running graph generation reproduces the same figures.
- The report is built from the saved results.
- Source and environment fingerprints are stored.
- All generated files are accounted for.

### NUMERIX regression protection

Run the existing NUMERIX test suite.

Run the available Make and CMake tests.

Verify that benchmark-only changes have not altered production library behavior.

Test all optional competitor configurations independently.

Run `git diff --check`.

Perform appropriate static, memory, or sanitizer checks on newly written benchmark adapters when practical.

Preserve the historical Phase 13 verification scripts.

---

# 18. IMPORTANT REPORTING RULES

These are non-negotiable.

1. Never claim a library is faster based on one or two selected examples.
2. Never claim a library is more accurate based only on its reported error estimate.
3. Never treat solver status success as proof of numerical correctness.
4. Never drop failed cases from reliability statistics.
5. Never compare different algorithms as though they were identical.
6. Never compare measurements collected at different accuracy targets without clearly explaining the difference.
7. Never combine unrelated modules into one misleading runtime average.
8. Never report missing measurements as zero.
9. Never invent test problems or reference answers after seeing the results to favor a particular library.
10. Never create a graph that cannot be traced to the saved source data.
11. Never claim all seven modules are comprehensively evaluated unless their minimum problem counts and comparison results are verified.
12. Never call requested heap allocation equal to total memory usage.
13. Never make a universal winner claim without sufficient evidence.
14. Never delete a benchmark observation because it looks inconvenient.
15. Never silently replace a failed comparison with a different algorithm or test problem.

---

# 19. REQUIRED EXECUTION ORDER

Work in this order.

### Stage 1 — Repository and benchmark audit

Inspect the entire existing benchmark structure and NUMERIX API.

Document what currently exists, what is reusable, and what is incomplete.

Verify previous Phase 13 outputs without modifying them.

### Stage 2 — Competitor-library research

Inspect official documentation and verify library capabilities, APIs, versions, and build feasibility.

Create the competitor capability matrix.

### Stage 3 — Benchmark workload design

Design the seven workload suites.

Create at least 100 distinct problem instances per module.

Create the master manifest and reference data.

Review the workload coverage before running final performance tests.

### Stage 4 — Adapter and infrastructure development

Implement adapters and new benchmark schema.

Keep all competitor dependencies optional.

Add timing, memory, work-count, accuracy, and status collection.

### Stage 5 — Pilot runs

Run small pilot tests to identify:

- Incorrect adapters.
- Incorrect references.
- Tolerance problems.
- Incorrect algorithm mappings.
- Runtime calibration problems.
- Memory measurement issues.
- Missing library functionality.

Do not use pilot observations as the final benchmark results.

### Stage 6 — Full benchmark execution

Run the complete benchmark on the same machine and configuration.

Record all raw observations.

Do not stop after a small representative sample.

Complete all seven modules.

### Stage 7 — Data verification and statistical analysis

Verify all records.

Calculate statistics and paired comparisons.

Generate all comparison tables.

### Stage 8 — Visualization

Generate the global overview and individual module figures.

Inspect the actual rendered PNG and SVG files.

Fix overlapping labels, bad axes, incorrect legends, poor spacing, misleading scales, and unreadable text.

### Stage 9 — Final report

Generate the Markdown and HTML reports.

Verify that every reported metric and graph agrees with the underlying data.

### Stage 10 — Completion report

Document exactly what was completed and what could not be completed.

Do not mark the entire study as fully complete if any required module, library, problem-count requirement, or verification step remains incomplete.

---

# 20. FINAL ACCEPTANCE CHECKLIST

The phase is considered complete only when all applicable requirements below have been verified.

- [ ] Existing Phase 13 data and reports preserved.
- [ ] Current repository state documented.
- [ ] All seven NUMERIX modules included.
- [ ] At least 100 distinct root-finding problems.
- [ ] At least 100 distinct linear-algebra problems.
- [ ] At least 100 distinct differentiation problems.
- [ ] At least 100 distinct interpolation problems.
- [ ] At least 100 distinct integration problems.
- [ ] At least 100 distinct optimization problems.
- [ ] At least 100 distinct ODE problems.
- [ ] GSL compared across all seven applicable modules.
- [ ] LAPACK/LAPACKE with reference BLAS evaluated.
- [ ] LAPACK/LAPACKE with OpenBLAS evaluated.
- [ ] NLopt evaluated for applicable optimization problems.
- [ ] SUNDIALS evaluated for applicable ODE problems.
- [ ] CMINPACK evaluated or formally excluded with justification.
- [ ] Other relevant C-callable candidates researched.
- [ ] Algorithm-equivalent and best-available comparisons separated.
- [ ] Numerical accuracy measured against independent references.
- [ ] Runtime measured and raw samples preserved.
- [ ] CPU time measured where supported.
- [ ] Memory measured using clearly distinguished methods.
- [ ] Reliability and convergence measured.
- [ ] Function evaluations and computational effort recorded where available.
- [ ] Scalability tested.
- [ ] Statistical confidence intervals calculated.
- [ ] All required global charts generated.
- [ ] All seven module result sections include relevant charts.
- [ ] Graph titles, axes, units, legends, and labels verified.
- [ ] PNG and SVG figures generated.
- [ ] Markdown and HTML reports generated.
- [ ] Raw data and manifests preserved.
- [ ] Reproducibility commands documented.
- [ ] Analysis and graph-generation tests pass.
- [ ] Existing NUMERIX regression tests pass.
- [ ] No unintended production-source changes.
- [ ] `git diff --check` passes.
- [ ] Final completion report created.

---

# 21. FINAL RESPONSE AFTER EXECUTION

When you finish, give me a detailed completion summary containing:

1. Libraries actually compared.
2. Modules covered.
3. Exact number of distinct problems per module.
4. Number of eligible and paired problems per library.
5. Metrics collected.
6. Number of raw timing samples.
7. Accuracy and reliability results.
8. Important performance findings.
9. Memory findings.
10. Number and locations of graphs.
11. Report locations.
12. Verification results.
13. Any libraries or modules not completed.
14. Any limitations or fairness concerns.
15. Git status and whether any files remain uncommitted.

Do not simply say that benchmarking is complete.

Show the actual coverage numbers and the actual output locations.

The objective is a genuine, reproducible, statistically defensible comparison of NUMERIX against established general-purpose and specialist numerical libraries across all seven modules.

Prioritize numerical correctness, fair comparisons, sufficient problem diversity, trustworthy data, and clear professional visualizations over producing impressive-looking but misleading results.