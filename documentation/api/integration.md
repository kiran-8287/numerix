# Numerical integration

Each operation integrates a finite-interval scalar callback and accepts shared options, which limit callback evaluations. Reversed endpoints are supported and produce a signed integral; equal endpoints return zero.

`numerix_integrate_trapezoid` applies the composite trapezoid sum with caller-selected positive panel count: `h[(f(a)+f(b))/2 + Σ f(a+ih)]`. `numerix_integrate_simpson` applies composite Simpson 1/3 with a positive even panel count: `h/3 [f0+fn+4Σodd+2Σeven]`. Neither fixed rule estimates error, so `estimated_error` is `NAN`.

`numerix_integrate_adaptive_simpson` recursively compares one Simpson panel with its two half-panels. It accepts a maximum depth from 1 through 64. The reported estimate accumulates `|S_left+S_right-S|/15` and the accepted result uses the usual `/15` correction. The estimate is heuristic, not a guaranteed bound. Insufficient depth returns convergence failure; evaluation limits and non-finite integrands are also reported.

All methods return the integral in `result.value`, callback count, iterations/subdivisions, elapsed time, and where implemented estimated error. Fixed rules can be expensive for very large panel counts and do not adapt to singularities.

```c
numerix_result integral;
numerix_status s = numerix_integrate_adaptive_simpson(
    f, data, 0.0, 1.0, NULL, 24, &integral, &error);
```
