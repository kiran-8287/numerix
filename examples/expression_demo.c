#include "numerix.h"

#include <stdio.h>

static int compile_expression(
    const char *source,
    numerix_expression **expression,
    numerix_error *error)
{
    numerix_status status = numerix_expression_compile(source, expression, error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Could not compile '%s': %s\n", source, error->message);
        return 0;
    }

    return 1;
}

static int demonstrate_basic_evaluation(void)
{
    const char *source = "x^2 + 3*x - 5";
    const double inputs[] = {
        -1.0,
        0.0,
        2.0
    };
    numerix_expression *expression = NULL;
    numerix_error error;
    size_t index;

    if (!compile_expression(source, &expression, &error))
    {
        return 0;
    }

    puts("Basic expression: x^2 + 3*x - 5");

    for (index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index)
    {
        double value;
        numerix_status status = numerix_expression_evaluate(
            expression,
            inputs[index],
            &value,
            &error);

        if (status != NUMERIX_STATUS_SUCCESS)
        {
            fprintf(stderr, "Evaluation failed: %s\n", error.message);
            numerix_expression_destroy(expression);
            return 0;
        }

        printf("  x = %.3f -> %.6f\n", inputs[index], value);
    }

    numerix_expression_destroy(expression);
    return 1;
}

static int demonstrate_functions(void)
{
    numerix_expression *expression = NULL;
    numerix_error error;
    const double inputs[] = {
        0.0,
        0.7853981633974483
    };
    size_t index;

    if (!compile_expression("sin(x) + cos(x)", &expression, &error))
    {
        return 0;
    }

    puts("Function expression: sin(x) + cos(x)");

    for (index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index)
    {
        double value;
        numerix_status status = numerix_expression_evaluate(
            expression,
            inputs[index],
            &value,
            &error);

        if (status != NUMERIX_STATUS_SUCCESS)
        {
            fprintf(stderr, "Evaluation failed: %s\n", error.message);
            numerix_expression_destroy(expression);
            return 0;
        }

        printf("  x = %.6f -> %.6f\n", inputs[index], value);
    }

    numerix_expression_destroy(expression);
    return 1;
}

static int demonstrate_root_finding(void)
{
    numerix_expression *expression = NULL;
    numerix_expression_callback_context callback_context;
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    if (!compile_expression("x^2 - 4", &expression, &error))
    {
        return 0;
    }

    status = numerix_expression_callback_context_init(
        &callback_context,
        expression,
        &error);

    if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_options_init(&options);
        status = numerix_root_brent(
            numerix_expression_callback,
            &callback_context,
            0.0,
            3.0,
            &options,
            &result,
            &error);
    }

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Root solve failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 0;
    }

    printf("Root of x^2 - 4 in [0, 3]: %.10f\n", result.value);
    numerix_expression_destroy(expression);
    return 1;
}

static int demonstrate_integration(void)
{
    numerix_expression *expression = NULL;
    numerix_expression_callback_context callback_context;
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    if (!compile_expression("sin(x)", &expression, &error))
    {
        return 0;
    }

    status = numerix_expression_callback_context_init(
        &callback_context,
        expression,
        &error);

    if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_options_init(&options);
        status = numerix_integrate_adaptive_simpson(
            numerix_expression_callback,
            &callback_context,
            0.0,
            3.14159265358979323846,
            &options,
            24,
            &result,
            &error);
    }

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Integration failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 0;
    }

    printf("Integral of sin(x) from 0 to pi: %.10f (expected 2)\n", result.value);
    numerix_expression_destroy(expression);
    return 1;
}

static int demonstrate_differentiation(void)
{
    numerix_expression *expression = NULL;
    numerix_expression_callback_context callback_context;
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;
    const double x = 2.0;

    if (!compile_expression("x^3", &expression, &error))
    {
        return 0;
    }

    status = numerix_expression_callback_context_init(
        &callback_context,
        expression,
        &error);

    if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_options_init(&options);
        status = numerix_diff_central(
            numerix_expression_callback,
            &callback_context,
            x,
            1e-4,
            &options,
            &result,
            &error);
    }

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Differentiation failed: %s\n", error.message);
        numerix_expression_destroy(expression);
        return 0;
    }

    printf("Derivative of x^3 at x=2: %.8f (expected %.8f)\n",
           result.value,
           3.0 * x * x);
    numerix_expression_destroy(expression);
    return 1;
}

int main(void)
{
    int success = 1;

    puts("NUMERIX expression engine examples");
    success = demonstrate_basic_evaluation() && success;
    success = demonstrate_functions() && success;
    success = demonstrate_root_finding() && success;
    success = demonstrate_integration() && success;
    success = demonstrate_differentiation() && success;

    return success ? 0 : 1;
}
