#include "numerix.h"

#include <math.h>
#include <stdio.h>

static double sin_func(double x, void *user_data)
{
    (void)user_data;
    return sin(x);
}

int main(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;
    const double pi = 3.14159265358979323846;
    const double analytical = 2.0;

    numerix_options_init(&options);

    /* Adaptive Simpson integration of sin(x) on [0, pi]. */
    status = numerix_integrate_adaptive_simpson(
        sin_func, NULL, 0.0, pi, &options, 24, &result, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Integration failed: %s\n", error.message);
        return 1;
    }

    printf("Method: adaptive Simpson\n");
    printf("Integral of sin(x) on [0, pi]: %.10f\n", result.value);
    printf("Analytical:                  %.10f\n", analytical);
    printf("Absolute error:              %.3e\n", fabs(result.value - analytical));
    if (isfinite(result.estimated_error))
        printf("Estimated error:             %.3e\n", result.estimated_error);
    printf("Function evaluations:        %zu\n", result.function_evaluations);

    return fabs(result.value - analytical) < 1e-6 ? 0 : 1;
}
