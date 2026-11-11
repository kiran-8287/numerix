# Interpolation

`numerix_interp_linear` evaluates the line through `(x0,y0)` and `(x1,y1)` at `x`. Its x coordinates must be distinct and all inputs finite.

`numerix_interp_lagrange` evaluates the Lagrange polynomial directly from finite `x[]` and `y[]` arrays and a point count of at least two. `numerix_interp_newton_create` computes divided-difference coefficients once into a caller-owned `numerix_interpolant`; `numerix_interp_newton_evaluate` evaluates that form using nested multiplication (Horner style). Repeated evaluation avoids rebuilding coefficients. Call `numerix_interpolant_destroy` once to release its owned arrays.

All abscissas must be pairwise distinct. Queries outside `[minimum, maximum]` are rejected by default; pass nonzero `allow_extrapolation` to explicitly request polynomial extrapolation. High-degree polynomial interpolation can be ill-conditioned, especially for uneven or numerous nodes. No error estimate is claimed.

Functions return the value in `numerix_result.value`, set an algorithm label, and report invalid data, range, allocation, or overflow errors. `user_data` does not apply to interpolation.

```c
numerix_interpolant p;
numerix_result value;
numerix_status s = numerix_interp_newton_create(xs, ys, count, &p, &error);
if (s == NUMERIX_STATUS_SUCCESS) {
    s = numerix_interp_newton_evaluate(&p, query, 0, &value, &error);
    numerix_interpolant_destroy(&p);
}
```
