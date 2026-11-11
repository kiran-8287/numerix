# Tolerances and numerical evidence

`numerix_options_init` sets absolute/relative tolerances to 1e-8, an iteration
budget of 1000 and an evaluation budget of 10000. A zero time limit disables
managed time limits. Raw algorithms do not uniformly implement wall-time limits.
All tolerances must be finite and nonnegative; at least one must be positive.
Negative values, NaN, infinity, both-zero tolerances and zero resource budgets
are rejected. Tiny positive values are valid requests, not promises that binary64
can attain them. Very large finite tolerances permit correspondingly coarse answers.

| Operation | Absolute / relative tolerance meaning |
| --- | --- |
| Bracketed roots | Position units: `abs + rel*abs(x)`. Bisection checks the maximum distance from the returned midpoint to either endpoint. Chord, Brent-style and safeguarded Newton return a sampled endpoint only after the entire bracket width meets its position tolerance. Continuity/sign-bracket assumptions remain the caller's responsibility. |
| Open Newton / secant | Position units. A small nonzero residual cannot terminate a solve. The estimate is the larger of the last step and `step*q/(1-q)` for observed step contraction q<1; at least two nonzero steps and a decreasing residual are required. This is heuristic local convergence evidence, not a bound or a proof that a root exists. |
| All roots | An exactly zero callback result is accepted, including endpoints. Floating-point underflow/cancellation inside a user callback can create false exact zeros; the library cannot recover information the callback lost. Residual is reported in function units. Iteration/evaluation exhaustion returns LIMIT_REACHED, unresolved stagnation CONVERGENCE_FAILURE, invalid arithmetic/zero denominator NUMERICAL_FAILURE. |
| Adaptive Simpson | Integral units: estimated global absolute error <= `max(abs, rel*abs(integral))`. It is an estimate, not a certified bound. |
| Fixed trapezoid / Simpson | Caller-selected panel count controls discretization; tolerances do not adapt the grid. Error estimate is unavailable (NAN). |
| Differentiation | Caller-selected positive finite h controls differences. Shared tolerance fields do not tune h. Richardson/five-point corrections estimate approximation error; they are not measured forward errors. |
| LU / direct linear solves | Pivot rejection uses matrix scale, dimension and machine precision, not the scalar absolute tolerance. A tiny backward residual can coexist with large forward error in an ill-conditioned system. |
| Golden-section optimization | Position/interval units, `abs + rel*max(abs(lower),abs(upper))`, not objective-value units. |
| Gradient descent | Gradient norm relative to iterate scale; consult the optimization guide. |
| Fixed-step ODE | Caller-selected step and number of steps; no tolerance-driven local error control. |
| Root discovery | Shared root options are position tolerances. Discovery's separate `residual_tolerance` is in function units. Merge tolerances control heuristic consolidation, not certification. |

Root diagnostics independently report residuals without inventing a function-unit
threshold from position options: `has_residual_tolerance` is false and its value
NAN unless an explicit discovery residual threshold exists. Root position-error
estimates and their thresholds are separate. Availability flags govern all
metrics; unavailable metrics must not be interpreted as measured zero. Forward
error requires a known reference; conditioning/sensitivity/error estimates and
multiplicity/discontinuity assessments are evidence with limitations.
