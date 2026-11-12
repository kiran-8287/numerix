#include "numerix.h"

#include <math.h>
#include <stdio.h>

int main(void)
{
    numerix_piecewise_function *function = NULL;
    numerix_result result;
    numerix_error error;
    numerix_status status;
    double value;

    /* Create the piecewise function object. */
    status = numerix_piecewise_create(&function, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "piecewise create failed: %s\n", error.message);
        return 1;
    }

    /* Segment 1: x^2 on x < 0 (lower=-inf, upper=0, exclude both). */
    status = numerix_piecewise_add_expression(
        function, -INFINITY, 0.0, 0, 0, "x^2", &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "add segment 1 failed: %s\n", error.message);
        numerix_piecewise_destroy(function);
        return 1;
    }

    /* Segment 2: 2*x on x >= 0 (lower=0, upper=inf, include lower, exclude upper). */
    status = numerix_piecewise_add_expression(
        function, 0.0, INFINITY, 1, 0, "2*x", &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "add segment 2 failed: %s\n", error.message);
        numerix_piecewise_destroy(function);
        return 1;
    }

    /* Evaluate at representative points on both sides of zero. */
    status = numerix_piecewise_evaluate(function, -2.0, &value, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
        printf("f(-2) = %.6f  (expected 4.000000)\n", value);
    else
        fprintf(stderr, "evaluate at -2 failed: %s\n", error.message);

    status = numerix_piecewise_evaluate(function, 0.0, &value, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
        printf("f( 0) = %.6f  (expected 0.000000)\n", value);
    else
        fprintf(stderr, "evaluate at 0 failed: %s\n", error.message);

    status = numerix_piecewise_evaluate(function, 2.0, &value, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
        printf("f( 2) = %.6f  (expected 4.000000)\n", value);
    else
        fprintf(stderr, "evaluate at 2 failed: %s\n", error.message);

    /* Integrate over [-2, 2]: integral of x^2 on [-2,0) is 8/3,
       integral of 2*x on [0,2] is 4, total = 8/3 + 4 = 20/3 ≈ 6.6667. */
    status = numerix_piecewise_integrate_trapezoid(
        function, -2.0, 2.0, 100, NULL, &result, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        printf("Integral on [-2, 2] = %.8f  (expected ≈ 6.66666667)\n", result.value);
    }
    else
    {
        fprintf(stderr, "integration failed: %s\n", error.message);
    }

    /* Destroy every owned object. */
    numerix_piecewise_destroy(function);

    return 0;
}
