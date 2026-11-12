# Initial-value ODE solvers

The Phase 1 ODE API integrates vector systems `dy/dx=f(x,y)`. The callback receives the current x and y array, dimension, output derivative array, and user data; it returns a `numerix_status`. Euler uses one callback per step, Heun uses predictor-corrector with two, and classical RK4 uses the four stages `k1`, `k2`, `k3`, `k4` and `(k1+2k2+2k3+k4)/6`.

Each solver accepts finite `x0`, finite `y0[]`, positive dimension, finite nonzero signed step, and positive step count. Negative steps integrate backward. The allocated `numerix_ode_solution` data is stored as `y[point * dimension + component]`; x values have one entry per point. It owns both arrays and reports point count, dimension, status, and callback count. Release with `numerix_ode_solution_destroy`.

The solvers return all requested fixed-step points. They do not adapt the step or estimate error. Every step must advance the stored independent variable: if `x + step == x` after floating-point rounding, the solver returns `NUMERIX_STATUS_NUMERICAL_FAILURE`. RK4 also requires its midpoint coordinate to be finite and distinct from both endpoints; a step can advance `x` while still collapsing this midpoint. Choose a step large enough for the local spacing, or rescale/rebase the independent variable. Negative steps are checked the same way and continue to integrate backward. The API requires a nonzero step and positive step count; it does not represent a zero-length integration request.

On a coordinate-stagnation failure, the current API clears the allocated history and returns `points == 0`, `x == NULL`, and `y == NULL`, with the failure status retained. Explicit Euler and Heun can be unstable for stiff systems; RK4 is still an explicit fixed-step method. Invalid callbacks, non-finite derivatives/solutions, bad dimensions, and allocation failures are reported.

```c
numerix_ode_solution solution;
numerix_status s = numerix_ode_rk4(rhs, user_data, 0.0, y0,
                                   dimension, 0.01, 100, &solution, &error);
if (s == NUMERIX_STATUS_SUCCESS) numerix_ode_solution_destroy(&solution);
```
