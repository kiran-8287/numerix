# Numerical differentiation

`numerix_diff_forward`, `numerix_diff_backward`, `numerix_diff_central`, `numerix_diff_central_five_point`, `numerix_diff_richardson`, and `numerix_diff_second` accept a scalar callback, its user data, finite `x`, positive finite step `h`, shared options (or `NULL` defaults), result, and error output. The derivative estimate is `result.value`; callback count and method label are also reported.

Formulas are the first-order forward `(f(x+h)-f(x))/h`, first-order backward `(f(x)-f(x-h))/h`, second-order central `(f(x+h)-f(x-h))/(2h)`, and fourth-order central `(f(x-2h)-8f(x-h)+8f(x+h)-f(x+2h))/(12h)`. Richardson applies `(4 D(h/2)-D(h))/3` to central differences. The second derivative uses `(f(x+h)-2f(x)+f(x-h))/h²`.

Before calling the function, each method checks that every required stencil coordinate is finite and pairwise distinct. If the requested `h` collapses one or more coordinates at the local floating-point spacing, the call returns `NUMERIX_STATUS_INVALID_INPUT` without evaluating the callback. The solver does not silently enlarge `h`; choose a step that produces distinct points, for example by checking neighboring values with `nextafter` when working at large coordinates. A distinct stencil can still be inaccurate because of roundoff or truncation error.

`estimated_error` is populated for the five-point estimate (difference from the central estimate) and Richardson (extrapolation correction). These are indicators, not guaranteed bounds. Other formulas report `NAN` for an unavailable error estimate. The generic `numerix_result.residual` field is also `NAN` for differentiation because a derivative estimate has no root residual. A zero derivative remains the ordinary numeric value `0.0`; `NAN` denotes unavailable or inapplicable metrics.

```c
numerix_result d;
numerix_status s = numerix_diff_richardson(f, data, 1.0, 1e-3,
                                           NULL, &d, &error);
```

Shared tolerances do not select a better step or certify derivative accuracy;
callers choose h. See [tolerance semantics](tolerances.md).
