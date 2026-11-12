#include "numerix.h"

#include <math.h>
#include <stdio.h>

/* f(x) = x^2 - 4 */
static double f(double x, void *user_data)
{
    (void)user_data;
    return x * x - 4.0;
}

int main(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);

    status = numerix_root_brent(f, NULL, 0.0, 3.0, &options, &result, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Root solve failed: %s\n", error.message);
        return 1;
    }

    printf("Method: %s\n", result.algorithm ? result.algorithm : "brent");
    printf("Root:   %.10f\n", result.value);
    printf("Expected: 2.0000000000\n");
    printf("Residual:  %.3e\n", result.residual);
    printf("Iterations: %zu\n", result.iterations);
    printf("Function evaluations: %zu\n", result.function_evaluations);

    return fabs(result.value - 2.0) < 1e-6 ? 0 : 1;
}
