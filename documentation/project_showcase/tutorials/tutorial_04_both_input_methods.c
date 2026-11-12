#include "numerix.h"

#include <math.h>
#include <stdio.h>

/* Version A callback: f(x) = x^2 - 4 */
static double f_callback(double x, void *user_data)
{
    (void)user_data;
    return x * x - 4.0;
}

int main(void)
{
    numerix_options options;
    numerix_result result_a;
    numerix_result result_b;
    numerix_error error;
    numerix_status status;

    numerix_expression *expression = NULL;
    numerix_expression_callback_context callback_context;

    numerix_options_init(&options);

    /* ---------------- Version A: C callback ---------------- */
    status = numerix_root_brent(
        f_callback, NULL, 0.0, 3.0, &options, &result_a, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Version A (callback) failed: %s\n", error.message);
        return 1;
    }

    printf("Version A (C callback):\n");
    printf("  Method: %s\n", result_a.algorithm ? result_a.algorithm : "brent");
    printf("  Root:   %.10f\n", result_a.value);
    printf("  Residual: %.3e\n", result_a.residual);

    /* ---------------- Version B: expression string ---------------- */
    status = numerix_expression_compile("x^2 - 4", &expression, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Expression compile failed: %s\n", error.message);
        return 1;
    }

    status = numerix_expression_callback_context_init(
        &callback_context, expression, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Callback context init failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 1;
    }

    status = numerix_root_brent(
        numerix_expression_callback,
        &callback_context,
        0.0,
        3.0,
        &options,
        &result_b,
        &error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Version B (expression) failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 1;
    }

    printf("Version B (expression string):\n");
    printf("  Method: %s\n", result_b.algorithm ? result_b.algorithm : "brent");
    printf("  Root:   %.10f\n", result_b.value);
    printf("  Residual: %.3e\n", result_b.residual);

    printf("Difference: %.3e\n", fabs(result_a.value - result_b.value));

    numerix_expression_destroy(expression);

    return (fabs(result_a.value - 2.0) < 1e-6 &&
            fabs(result_b.value - 2.0) < 1e-6) ? 0 : 1;
}
