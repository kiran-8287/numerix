# Numerical differentiation

`numerix_diff_forward`, `numerix_diff_backward`, `numerix_diff_central`, `numerix_diff_central_five_point`, `numerix_diff_richardson`, and `numerix_diff_second` accept a scalar callback, its user data, finite `x`, positive finite step `h`, shared options (or `NULL` defaults), result, and error output. The derivative estimate is `result.value`; callback count and method label are also reported.

Formulas are the first-order forward `(f(x+h)-f(x))/h`, first-order backward `(f(x)-f(x-h))/h`, second-order central `(f(x+h)-f(x-h))/(2h)`, and fourth-order central `(f(x-2h)-8f(x-h)+8f(x+h)-f(x+2h))/(12h)`. Richardson applies `(4 D(h/2)-D(h))/3` to central differences. The second derivative uses `(f(x+h)-2f(x)+f(x-h))/h²`.

`estimated_error` is populated for the five-point estimate (difference from the central estimate) and Richardson (extrapolation correction). These are indicators, not guaranteed bounds. Other formulas report `NAN` for an unavailable error estimate. Too-small steps can amplify roundoff; too-large steps increase truncation error. Non-finite points or callback results are reported as failures.

```c
numerix_result d;
numerix_status s = numerix_diff_richardson(f, data, 1.0, 1e-3,
                                           NULL, &d, &error);
```
