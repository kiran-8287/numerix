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
