# Phase 4: recommendations, execution, fallback, and portfolios

NUMERIX intelligence is a deterministic rule layer in `src/intelligence/`. It inspects only information present in a `numerix_recommendation_request`, validates supported structural facts, and invokes existing algorithms through their public APIs. It does not duplicate numerical solvers or infer unavailable matrix conditioning, function smoothness, ODE stiffness, or global optimization properties.

## Problem inputs

A request selects one implemented category and identifies scalar input representation as a callback, immutable compiled expression, or immutable piecewise function. Type-specific data reuses existing `numerix_matrix`, `numerix_vector`, `numerix_options`, interpolation arrays, objective/gradient callbacks, and ODE callback types. Requests borrow all pointed-to data. For root and scalar execution, the engine creates a local callback context and propagates expression/piecewise evaluation errors. The callback context is per synchronous operation and is never shared across worker threads.

`numerix_recommend_algorithm` only recommends. Its fixed-size result reports the chosen algorithm, suitability, alternatives and explanations, limitations, and root bracket probe count. A root bracket is checked by evaluating both endpoints; `bracket_was_checked` distinguishes an unchecked bracket from a checked interval without a sign change. Recommendations are based on observed data and do not constitute mathematical guarantees.

## Rules by category

- **Root finding:** A verified sign change with a derivative and in-bracket initial point recommends safeguarded Newton. Other verified brackets recommend Brent-Dekker, with bisection and false position as alternatives. When an initial estimate and derivative are both supplied, the derivative is sampled once; an observed zero or non-finite derivative is reported and derivative-based methods are skipped. Without a bracket, a finite initial and usable sampled derivative recommend conditional Newton; two distinct initial estimates recommend conditional secant. Invalid/missing information is reported.
- **Linear algebra:** A square matrix and dimension-matched right-hand side are required. Existing LU decomposition is used to check whether the matrix is accepted before recommending LU; Gaussian elimination and Gauss-Jordan remain alternatives. This check does not estimate conditioning.
- **Differentiation:** Central differences are recommended for a callback and positive step. Piecewise input selects the boundary-guarded central method. Five-point and forward differences are alternatives. No request-only check can prove smoothness.
- **Interpolation:** Duplicate or non-finite data are rejected. Two points recommend linear interpolation; more points recommend Newton divided differences, with Lagrange as an alternative. Out-of-range queries require extrapolation to be enabled.
- **Integration:** Finite nonzero intervals recommend adaptive Simpson for ordinary scalar input, or boundary-splitting piecewise trapezoid for piecewise input. Error estimates remain numerical estimates; singularity handling is not automatic.
- **Optimization:** A bounded scalar objective recommends golden-section search. A supplied objective, gradient and initial vector recommend gradient descent conditionally. The latter is local and neither method guarantees a global optimum.
- **ODE:** A finite initial state and nonzero fixed step plan recommend RK4, with Heun and Euler as alternatives. Stiffness is not inferred.

## Automatic execution and explicit choice

`numerix_execute_recommended` recommends and executes a category method. Set `explicit_algorithm` to a non-`NONE` enum to request an override; the selected algorithm is reported. Output scalar values live in `scalar_result`. The execution output owns any `vector_result` and ODE buffers and must be released with `numerix_execution_output_destroy`.

For root solving, `numerix_root_solve_auto` provides an explicit/automatic mode and configurable fallback. Fallback is attempted after numerical or convergence failure, and after a missing derivative only when another method is supported by the supplied bracket or second initial estimate. Invalid intervals, evaluation limits, allocation failure and other non-recoverable input errors stop the sequence. The first failure remains in `original_failure`; each attempt stores its status, result, explanation, and error. A fallback that succeeds does not erase the failed attempt. Fallback choices depend on available bracket/initial data: Newton or safeguarded Newton can fall back to Brent when bracketed; a derivative-free initial solve can use secant when a second initial estimate exists; a failed Brent attempt can fall back to bisection. Explicit algorithms are never replaced unless fallback is enabled.

Recommendation function and derivative probes are reported in the recommendation record and included in aggregate orchestration counts; endpoint function probes have a separate field. Each solver attempt receives the caller's options and therefore applies its own evaluation/iteration limit; aggregate totals are included in the orchestration report.

## Root portfolio

`numerix_root_solve_portfolio` runs applicable root methods sequentially. A bit mask can restrict methods; zero asks for all methods applicable to the supplied information. It accepts a result only if the solver reports success, the root and residual are finite, and either the residual or root error estimate meets tolerance. Among valid results it prefers smaller residual, then fewer iterations. It reports all statuses, counts, selected result index, and whether successful estimates agree within tolerance.

Portfolio attempts are deliberately sequential. Callback thread safety is not part of the callback contract, so no concurrent calls are made. Different methods may converge to different roots when multiple roots lie in the bracket; the portfolio is a comparison of single-root solvers, not a root-discovery algorithm.

## Limitations

Recommendations are rule-based and deterministic. They cannot certify smoothness, derivative quality, conditioning, unimodality, stiffness, or integrability from finite observations. Root recommendations probe bracket endpoints, which invokes user callbacks. Expression and piecewise wrappers preserve evaluation statuses, but ordinary C callbacks can only communicate failure by returning a non-finite value. Existing direct algorithm APIs are unchanged.
