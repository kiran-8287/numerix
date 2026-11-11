#include <numerix.h>

#include <math.h>
#include <stdio.h>

static double scaled_equation(double position, void *data)
{
    (void)data;
    return 1e-12 * (position - 2.0);
}

int main(void)
{
    numerix_result result;
    numerix_error error;
    numerix_options options;
    numerix_lu factorization = {0};
    numerix_vector *solution = NULL;
    numerix_expression *expression = NULL;
    numerix_expression *invalid_expression = NULL;
    double entries[] = {3.0, 1.0, 1.0, 2.0};
    double right_entries[] = {5.0, 5.0};
    numerix_matrix matrix = {2U, 2U, entries};
    numerix_vector right = {2U, right_entries};
    double value = 0.0;
    int success = 0;

    numerix_options_init(&options);
    options.absolute_tolerance = 1e-10;
    options.relative_tolerance = 1e-10;
    if (numerix_root_brent(scaled_equation, NULL, 0.0, 5.0, &options, &result, &error) !=
        NUMERIX_STATUS_SUCCESS || fabs(result.value - 2.0) > 3e-10)
    {
        goto cleanup;
    }
    if (numerix_lu_decompose(&matrix, NULL, &factorization, &error) != NUMERIX_STATUS_SUCCESS ||
        numerix_lu_solve(&factorization, &right, &solution, &error) != NUMERIX_STATUS_SUCCESS)
    {
        goto cleanup;
    }
    if (fabs(solution->data[0] - 1.0) > 1e-12 || fabs(solution->data[1] - 2.0) > 1e-12)
    {
        goto cleanup;
    }
    if (numerix_expression_compile("sin(pi/2)+x^2", &expression, &error) != NUMERIX_STATUS_SUCCESS ||
        numerix_expression_evaluate(expression, 2.0, &value, &error) != NUMERIX_STATUS_SUCCESS ||
        fabs(value - 5.0) > 1e-12)
    {
        goto cleanup;
    }
    if (numerix_expression_compile("sin(", &invalid_expression, NULL) != NUMERIX_STATUS_INVALID_INPUT ||
        invalid_expression != NULL)
    {
        goto cleanup;
    }
    success = 1;

cleanup:
    numerix_expression_destroy(invalid_expression);
    numerix_expression_destroy(expression);
    numerix_vector_destroy(solution);
    numerix_lu_destroy(&factorization);
    if (!success)
    {
        fputs("Installed consumer verification failed\n", stderr);
        return 1;
    }
    puts("Installed consumer: roots, LU, expressions, errors and cleanup passed.");
    return 0;
}
