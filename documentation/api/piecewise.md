# Piecewise functions and discontinuity diagnostics

Phase 3 represents a piecewise function as an ordered list of compiled expression segments. Each segment has lower and upper bounds plus independent inclusion flags. Bounds may be infinite only at the outward end of an interval; infinite endpoints are always open. Segments must be appended in increasing order. Overlap, empty intervals, invalid flags, and NaN bounds are rejected. Gaps are allowed at construction and are reported as undefined during evaluation or integration.

`numerix_piecewise_add_expression` compiles the expression and transfers ownership to the piecewise object. The object destroys all compiled expressions when destroyed. A failed addition leaves the existing object unchanged. A zero width interval is invalid, so an isolated point value cannot be added as its own segment. A punctured interval can still be represented with open boundaries and analyzed using its one-sided limits.

```c
numerix_piecewise_function *function = NULL;
numerix_error error;
double value;

numerix_piecewise_create(&function, &error);
numerix_piecewise_add_expression(
    function, -INFINITY, 0.0, 0, 0, "x^2", &error);
numerix_piecewise_add_expression(
    function, 0.0, INFINITY, 1, 0, "2*x+1", &error);
numerix_piecewise_evaluate(function, 0.0, &value, &error);
numerix_piecewise_destroy(function);
```

Membership uses exact comparisons and the supplied open/closed flags. When an input belongs to no segment, evaluation returns `NUMERIX_STATUS_INVALID_INPUT`. Expressions use the Phase 2 compiler and evaluator; domain errors, division by zero, and non-finite results are propagated.

## One-sided limits and boundary reports

`numerix_piecewise_estimate_one_sided_limit` samples geometrically closer points from the requested side while evaluating only the segment on that side. It needs repeated stable samples to report numerical convergence. Repeated growth can produce a suspected signed infinite limit. A missing segment, expression evaluation failure, or exhausted iteration budget is reported in the status and confidence output.

`numerix_piecewise_analyze_boundary` compares both one-sided estimates with the exact piecewise value where defined. It can label a point continuous, jump, removable, infinite, or unknown. The report retains separate status codes for both sides and the point value. `numerically confirmed` means the configured numerical samples met the test; it is not a symbolic proof. `numerix_piecewise_analyze_boundaries` gathers each finite segment endpoint into caller-provided report storage.

`numerix_piecewise_scan_interval` probes a user-selected finite interval and reports suspected regions where evaluation status changes, values become non-finite, or adjacent sample behavior changes sharply. It is a finite uniform scan: narrow singularities, oscillations between samples, and subtle discontinuities can be missed. Candidates are not proofs.

## Existing numerical algorithms

The piecewise callback adapter follows NUMERIX's `numerix_function` signature. Its context borrows the piecewise object and records the most recent status and error. Finish adding segments before evaluation; mutation during active calls is unsupported. Use a separate context for each active call; do not share a mutable callback context between concurrent evaluations.

`numerix_piecewise_integrate_trapezoid` splits the interval at known segment boundaries, calls the existing composite trapezoid rule on each covered segment, and combines the values. It rejects uncovered gaps. Open endpoints are moved one representable value into the segment before evaluating the endpoint sample, so this is an approximation to the one-sided endpoint behavior. It does not resolve arbitrary interior singularities and does not assert integrability.

`numerix_piecewise_diff_central` refuses a finite-difference stencil that crosses a known boundary. At the boundary itself, it allows the estimate only when both the boundary report and nearby one-sided slope checks are consistent. At a jump, corner, or inconclusive boundary it returns an error instead of an ordinary derivative. Differentiation inside a smooth segment can use the existing difference routines through the callback adapter.

All diagnostic classifications are numerical evidence only. The expression engine does not perform symbolic limit analysis, and no finite sampling scheme can guarantee detection or classification of every discontinuity.
