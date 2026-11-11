# Numerical integration

Finite bounds and finite callback samples are required. Reversing endpoints
negates the result; equal bounds return zero. Fixed trapezoid and Simpson 1/3
use a positive panel count (even for Simpson). Their compensated sums use
extended precision where available, and overflow returns a failure. Fixed rules
have no adaptive tolerance guarantee or error estimate (`NAN`).

Adaptive Simpson uses the same Simpson panels and Richardson `/15` correction,
with a heap prioritizing the largest estimated-error leaf. It stops when the sum
of leaf estimates meets `max(abs_tol, rel_tol*abs(integral))`; it does not divide
one tiny local tolerance equally down an endpoint branch. The estimate uses the
full successive-rule difference (rather than the smooth-only difference/15) and
a roundoff floor. This is deliberately conservative for the tested endpoint
behavior, but is still heuristic. Finite sampling can miss localized/aliased
oscillations; this is not a certified quadrature method.

`maximum_depth` remains 1 through 64. Depth/evaluation/split exhaustion returns
LIMIT_REACHED, representational stagnation or an unattainable roundoff target
CONVERGENCE_FAILURE, and non-finite samples/arithmetic NUMERICAL_FAILURE. Failure
returns NAN integral and an available partial error estimate where meaningful.
No recursion is used; heap storage grows with assessed leaves and is released
on every path, including allocation failure. Iterations count successful splits.
A failed integral never reports a fabricated zero residual/error as success.

`sqrt(x)` on [0,1] is handled at depth32 and 1e-10 tolerance. `1/sqrt(x)` at x=0
still fails because this finite-endpoint API does not implement improper limits.
An explicit substitution such as x=t² can reformulate that problem; it must be
supplied by the caller. No new improper-integral algorithm was introduced.

See [tolerance semantics](tolerances.md) and the Phase10 accuracy report.
