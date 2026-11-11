# Scalar root finding

All six methods accept a scalar callback and shared options (NULL selects
defaults), return a status and write a `numerix_result`. Position and callback
values must be finite. Residuals are in function units; position tolerances are
`absolute_tolerance + relative_tolerance*abs(x)`, in x units.

Bisection, safeguarded chord/false-position, Brent-style IQI/secant/bisection,
and safeguarded Newton maintain a sign-changing interval. An exactly zero sample
is accepted. Otherwise bisection returns a midpoint with a maximum endpoint
separation within tolerance; the other methods return a sampled endpoint whose
entire bracket width meets tolerance. The Brent interior safeguards remain
independent of GSL's implementation. Continuity is an input assumption; a sign
change across a pole or jump is not proof of a root.

Newton and secant use local step/contraction evidence, not small residuals.
Tiny nonzero function/derivative magnitudes alone are not a failure: Newton
rejects a zero derivative, and secant normalizes its function difference by the
sample magnitude before checking degeneracy. Newton derivative calls count in
`function_evaluations`. Open-method error estimates are heuristic. Incorrect
derivatives, flat roots, machine stagnation and callback cancellation can still
prevent meaningful convergence. Supply a valid bracket for stronger position
evidence; see [tolerances](tolerances.md).

Unsignalled brackets return INVALID_INPUT; invalid arguments have distinct
statuses. Budget exhaustion returns LIMIT_REACHED, zero derivative/degenerate
secant arithmetic NUMERICAL_FAILURE, and representational stagnation
CONVERGENCE_FAILURE. On failure inspect status before using approximate values.
An exactly zero callback caused by underflow/cancellation remains a limitation.

```c
numerix_result result;
numerix_error error;
numerix_options options;
numerix_options_init(&options);
/* f and user_data are caller-provided, borrowed for the call. */
numerix_status status = numerix_root_brent(
    f, user_data, 0.0, 2.0, &options, &result, &error);
```
