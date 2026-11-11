# Optimization

`numerix_optimize_golden_section` minimizes a scalar callback over finite ordered bounds. It repeatedly retains the subinterval containing the lower sampled objective value, using the golden ratio. `result.value` is the best interior sample, `result.residual` is its objective value, and `estimated_error` is half the remaining interval width. It is a bounded one-dimensional search and requires a unimodal objective on the interval for the usual minimum guarantee; it does not claim global optimization for arbitrary functions.

`numerix_optimize_gradient_descent` is also available for vector inputs. The objective callback is `double f(const double *x, size_t dimension, void *data)` and the gradient callback writes a finite gradient and returns a status. It uses steepest descent with Armijo backtracking from a positive initial step. The returned vector is caller-owned and must be released with `numerix_vector_destroy`. Result `value` is the objective, `residual` is the final gradient norm, and function evaluation count counts objective calls (gradient calls are tracked by the gradient provider and excluded).

Both methods use shared absolute/relative tolerances and iteration/evaluation limits. Non-finite objective values, callback failures, invalid intervals/steps, and limits return statuses. Gradient descent may stop when its backtracking search cannot find a decrease; this is reported as convergence failure.

```c
numerix_result minimum;
numerix_status s = numerix_optimize_golden_section(
    objective, data, -2.0, 4.0, NULL, &minimum, &error);
```
