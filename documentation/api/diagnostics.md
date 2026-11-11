# Diagnostics API

Include `numerix.h`. Diagnostics are optional and run after an execution.

```c
numerix_diagnostics_options diagnostic_options = {1};
numerix_diagnostics_report report;
numerix_status status;

status = numerix_diagnostics_analyze(
    &request,
    &execution_output,
    &diagnostic_options,
    &report,
    &error);
if (status == NUMERIX_STATUS_SUCCESS)
{
    if (report.has_residual)
    {
        printf("absolute residual: %.6g\n", report.absolute_residual);
    }
    printf("reliability: %s\n",
           numerix_reliability_class_name(report.reliability));
    numerix_diagnostics_report_destroy(&report);
}
```

Set `estimate_linear_condition` to zero to skip the additional linear
condition calculation. A null options pointer enables it. Condition estimation
is limited to matrices with at most 256 rows. All fields with a `has_` prefix
must be checked before reading the corresponding value. A returned linear
residual vector belongs to the diagnostics report and is released by
`numerix_diagnostics_report_destroy`. Destroy a prior report before reusing
its storage.

Root residual evaluation adds one callback or expression evaluation. Linear
diagnostics reuse the returned solution and do not reevaluate the solver.

`numerix_diagnostics_scalar_sensitivity` estimates a scalar derivative using
Richardson central differences. The caller chooses a positive finite step and
retains ownership of callback data. The callback is evaluated multiple times;
the report's uncertainty is an algorithm estimate, not a guaranteed bound.
Relative sensitivity is only available when the function value is safely
away from zero.

## Example: scalar sensitivity

```c
numerix_sensitivity_report sensitivity;
numerix_status status = numerix_diagnostics_scalar_sensitivity(
    function,
    user_data,
    evaluation_point,
    1.0e-4,
    &options,
    &sensitivity,
    &error);
if (status == NUMERIX_STATUS_SUCCESS)
{
    printf("derivative estimate: %.8g\n", sensitivity.derivative);
    printf("estimated uncertainty: %.3g\n",
           sensitivity.estimated_uncertainty);
}
```

## Limitations

Root reports contain an absolute function residual; a relative residual is not
provided because a scale for arbitrary callback values is not defined.
Linear backward error uses infinity norms. Integration estimated error is not
actual error. Interpolation node residual is not between-node interpolation
error. Forward error is only calculated against explicit caller-supplied
references.

## Additional Phase 6.1 APIs

- `numerix_diagnostics_forward_error_scalar`, `_vector`, and `_matrix` compare
  computed values against caller-supplied references. Vector and flattened
  matrix absolute errors use the Euclidean norm.
- `numerix_diagnostics_root_condition` estimates reciprocal derivative
  sensitivity for a simple root. Interpret it using the function's units and
  the additive function-perturbation model.
- `numerix_diagnostics_multivariable_sensitivity` reuses the existing scalar
  objective callback and returns a caller-owned gradient vector. Release it
  with `numerix_multivariable_sensitivity_destroy`.
- `numerix_diagnostics_propagate_independent_uncertainty` assumes independent
  standard uncertainties and local linearization. Correlations and large
  perturbations are unsupported.
- `numerix_diagnostics_analyze_convergence` uses signed residual history. Any
  reported empirical order uses residuals as error proxies and is heuristic.
- `numerix_periodic_diagnostics_*` provides bounded residual observation for
  composition inside an existing Phase 5 step callback. It is not synchronized
  and does not replace Phase 5 monitoring.
- `numerix_diagnostics_check_subtraction` checks caller-visible subtraction
  and reports a heuristic cancellation indicator.
- `numerix_recommend_algorithm_with_diagnostics` uses prior evidence only for
  matching callback roots with the same user-data pointer and exact bracket.
  It returns a recommendation; explicit execution overrides remain controlled
  by the caller.

Optimization diagnostics expose objective value, final gradient norm when
available, and Golden-section interval estimates. ODE diagnostics summarize
stored states but do not produce local-error estimates for fixed-step methods.

The standalone `make benchmark-diagnostics` target compares disabled, basic,
and optional diagnostics for root finding, 8x8 and 32x32 linear systems,
integration, differentiation, optimization, and RK4. It uses 10,000 runs per
mode and C `clock()` process time. Results vary by machine and run; allocations
and periodic observer memory are not instrumented.

One measured Windows MinGW run (10,000 repetitions per mode, milliseconds per
execution):

| Case | Diagnostics disabled | Basic diagnostics | Optional diagnostics |
| --- | ---: | ---: | ---: |
| Root finding | 0.00760 | 0.00800 | 0.00800 |
| Linear 8x8 | 0.00620 | 0.00700 | 0.01150 |
| Linear 32x32 | 0.03710 | 0.04320 | 0.15250 |
| Integration | 0.00920 | 0.03460 | 0.03250 |
| Differentiation | 0.00450 | 0.00430 | 0.00460 |
| Optimization | 0.00600 | 0.00590 | 0.00510 |
| ODE RK4 | 0.01160 | 0.03570 | 0.03550 |

The benchmark reported six extra function evaluations for root diagnostics in
full mode (one residual evaluation and five Richardson evaluations); the
other listed diagnostic paths reported no additional function or derivative
callback evaluations. The linear full mode instead spends time on LU-based
condition estimation.

These are single-run averages from `clock()` and are sensitive to host load and
timer resolution. In particular, the basic post-run reports scan ODE state
history and can add cost proportional to the number of stored points. The
condition estimate accounts for the larger 32x32 full-mode cost.
