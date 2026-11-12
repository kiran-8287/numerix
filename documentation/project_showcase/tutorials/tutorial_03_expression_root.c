#include "numerix.h"

#include <math.h>
#include <stdio.h>

int main(void)
{
    numerix_expression *expression = NULL;
    numerix_expression_callback_context callback_context;
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    /* 1. Compile the expression string. */
    status = numerix_expression_compile("x^2 - 4", &expression, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Expression compile failed: %s\n", error.message);
        return 1;
    }

    /* 2. Initialize the expression callback context (borrows the expression). */
    status = numerix_expression_callback_context_init(
        &callback_context, expression, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Callback context init failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 1;
    }

    /* 3. Solve through the expression callback adapter using Brent's method. */
    numerix_options_init(&options);
    status = numerix_root_brent(
        numerix_expression_callback,
        &callback_context,
        0.0,
        3.0,
        &options,
        &result,
        &error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Root solve failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 1;
    }

    printf("Method: %s\n", result.algorithm ? result.algorithm : "brent");
    printf("Root of x^2 - 4 on [0, 3]: %.10f\n", result.value);
    printf("Expected: 2.0000000000\n");
    printf("Residual:  %.3e\n", result.residual);
    printf("Iterations: %zu\n", result.iterations);

    /* 5. Destroy owned resources. */
    numerix_expression_destroy(expression);

    return fabs(result.value - 2.0) < 1e-6 ? 0 : 1;
}
