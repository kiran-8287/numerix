# Root finding

All six functions use `numerix_function`, validated shared `numerix_options`, and write `numerix_result` (root in `value`, function residual in `residual`, interval/step estimate in `estimated_error`, plus iterations and callback counts). Options may be `NULL` for defaults. Errors are returned as `numerix_status` and optionally described by `numerix_error`.

`numerix_root_bisection`, `numerix_root_regula_falsi`, and `numerix_root_brent` accept a finite ordered bracket. They return an endpoint root immediately and otherwise require opposite endpoint signs. Bisection halves the interval. False position uses the chord intercept and bisects when a step would stagnate near an edge. The Brent-Dekker solver maintains a sign-changing bracket, attempts inverse quadratic interpolation when three distinct function values are available, uses a secant step otherwise, and falls back to bisection when interpolation is unsafe. It is an independent safeguarded implementation, not a reproduction of a specific reference implementation.

`numerix_root_secant` takes two distinct finite initial estimates and uses the secant recurrence `x[n+1] = x[n] - f[n](x[n]-x[n-1])/(f[n]-f[n-1])`. A near-zero denominator is a numerical failure.

`numerix_root_newton` takes the function callback, derivative callback, shared `user_data`, and initial point. Newton updates are `x[n+1]=x[n]-f(x[n])/f'(x[n])`; a near-zero/non-finite derivative is rejected. `numerix_root_safeguarded_newton` accepts a finite bracket and starts at its midpoint. It keeps the sign bracket, taking an in-bracket Newton step or a bisection step when Newton is unsafe.

Bracketed methods reject missing sign changes with `NUMERIX_STATUS_INVALID_INPUT`. Non-finite callback values, evaluation/iteration limits, and failed convergence have distinct status codes. Derivative callback evaluations count against `function_evaluations`. A residual tolerance or the shared absolute/relative position tolerance stops iteration. A bracket is required to be continuous in practice; callback continuity cannot be verified by the library.

```c
numerix_result result;
numerix_error error;
numerix_options options;
numerix_options_init(&options);
numerix_status s = numerix_root_brent(f, user_data, 0.0, 2.0,
                                      &options, &result, &error);
```
