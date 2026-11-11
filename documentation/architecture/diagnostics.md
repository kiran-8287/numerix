# Numerical diagnostics architecture

Phase 6 diagnostics are a post-execution layer in `src/diagnostics/diagnostics.c`.
They accept the original `numerix_recommendation_request` and the existing
`numerix_execution_output`, so the algorithms and Phase 5 manager remain the
owners of computation and execution state. Diagnostics do not mutate either
input and do not run the solver again.

The report is deliberately evidence based. A metric is accompanied by a
`has_*` field where zero and unavailable have different meanings. The linear
residual vector is owned by the report and released by
`numerix_diagnostics_report_destroy`.

## Available evidence

| Category | Current report evidence |
| --- | --- |
| Root finding | Fresh absolute residual evaluation at the returned root, including callback, expression, and piecewise inputs |
| Linear algebra | `b - A*x`, infinity norm, relative residual against `||b||∞` when nonzero, normwise backward error, and optional infinity-norm condition estimate |
| Differentiation | Reuses the algorithm's finite estimated error when present |
| Interpolation | Rebuilds the Newton interpolant and measures the maximum discrepancy at input data points; duplicate or invalid data is reported by the existing interpolant validation |
| Integration | Reuses the algorithm's estimated absolute error and reports a relative estimate when the integral is not near zero |
| Optimization and ODE | Reports that current execution outputs do not expose enough gradient or local-error history for a reliability estimate |

Linear backward error uses infinity norms:

`||b - A*x||∞ / (||A||∞ * ||x||∞ + ||b||∞)`.

The optional condition calculation factors `A` once using the existing pivoted
LU routine, solves against each coordinate vector, and accumulates the row
sums of the computed inverse columns. It does not form or retain a full inverse
matrix. It is still an additional O(n³) operation and is skipped above 256
rows. It is an estimate based on floating-point LU solves, not a certified
bound.

Integration estimates are estimates produced by the selected method. They are
not measurements of the unknown true integration error. Interpolation data
residual is not an estimate of error between the supplied nodes. A small
residual alone does not establish forward accuracy.

## Reliability classification

Classification is rule based and is not a probability. A non-successful solve
is `unreliable`; non-finite evidence is `low confidence`; a failed available
tolerance check or a very large computed linear condition estimate produces
`caution`; successful, finite available evidence that passes checks is
`acceptable`. Where a category exposes no usable evidence the result is
`inconclusive`. These classes do not prove correctness or stability.

Warnings are bounded text explanations in the report. They currently do not
carry stable diagnostic identifiers, severity enums, numerical evidence fields,
or suggested actions. Arbitrary callback internals are not inspected, so
floating-point events such as cancellation or intermediate overflow cannot be
detected generally.

## Phase integration and scope

After Phase 4 `numerix_execute_recommended`, pass the same request and output
to `numerix_diagnostics_analyze`. After Phase 5 managed execution completes,
obtain the output through `numerix_managed_execution_get_output` and analyze it
the same way while the managed execution remains alive. The caller must keep
the request inputs alive through both calls.

This is a final, opt-in assessment. It does not run periodically during managed
execution, inspect step history, alter algorithm recommendations, trigger
fallback, or change a user-selected algorithm. Phase 4 recommendation reports
and Phase 5 summaries remain available through their existing APIs.

The scalar sensitivity helper uses the existing Richardson central-difference
algorithm and performs one extra callback evaluation to scale the derivative.
Its uncertainty is the existing Richardson estimate, which is empirical and
can be affected by nonsmoothness, cancellation, callback noise, and the chosen
step. Relative sensitivity is omitted near a zero function value. The local
linear interpretation assumes sufficiently small perturbations.

## Phase 6.1 extensions

Forward-error APIs compare against caller-supplied scalar or vector references.
Vector absolute error is the Euclidean norm. A zero reference has no relative
error. The reference is only as trustworthy as the caller's source.

Multivariable sensitivity reuses the existing scalar
`numerix_objective_function` representation and estimates its gradient with
central differences and Richardson refinement. It uses four objective
evaluations per input coordinate plus one nominal evaluation. Relative
sensitivities are available only when the nominal output is nonzero.
Independent-input error propagation uses
`sqrt(sum((df/dx_i * sigma_i)^2))`; correlations are not represented. Both
operations assume a locally smooth function and small perturbations.

Root conditioning reports `1/|f'(x*)|` from a supplied derivative or
Richardson estimate. This describes sensitivity to additive function-value
perturbations under that local model; it is scale dependent, not a universal
root condition number. The estimate is unavailable when the derivative cannot
be distinguished from its estimated uncertainty.

Convergence-history analysis accepts signed residual observations. Trend
categories use heuristics and need at least three samples. The optional
empirical order uses residual magnitudes as proxies for errors and is not a
convergence guarantee. The bounded periodic observer retains at most the
configured number of residuals. It can be called from an existing Phase 5 step
callback (detailed steps are currently provided for Bisection). The caller
decides which steps to pass and must serialize access; the observer does not
install callbacks, synchronize, or alter solver state.

Phase 4's history-aware recommendation accepts prior request metadata and a
diagnostics report. It only considers the same callback, user-data pointer,
representation, and exact bracket. Weak Newton or safeguarded Newton evidence
can cause Brent to be suggested when the current bracket is verified. It does
not alter explicit execution overrides or fallback rules. Expressions,
piecewise functions, linear systems, and cross-bracket history are not used
for diagnostic-informed selection.

Optimization reports now expose the objective value and, for gradient
descent, the final gradient norm already recorded by the optimizer. Golden
section reports its existing interval-width estimate. No objective-change or
parameter history is inferred. ODE diagnostics summarize stored solution
points and finite state magnitude. Euler, Heun, and RK4 do not expose local
error estimates; diagnostics do not invent one.

Structured records accompany the backward-compatible warning strings. Records
include stable identifiers, severity, category, problem and algorithm,
description, cause, consequence, suggested action, and estimate/heuristic
flags. Metric and threshold fields are populated when a comparison provides
them. Reports remain bounded to four records.

`numerix_diagnostics_check_subtraction` explicitly checks a caller-visible
subtraction. It compares the result scale to the operand scale and estimates
decimal digits at risk. This is a heuristic and does not instrument arbitrary
callbacks or every expression-evaluator operation.
