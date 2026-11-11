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
error. No actual forward error is claimed because a trusted reference solution
is not supplied. Optimization and ODE reliability evidence is inconclusive
when the existing output does not expose the necessary gradient or error
history.
