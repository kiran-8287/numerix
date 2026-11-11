#include "numerix.h"

#include <float.h>
#include <math.h>
#include <stdio.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static double shifted_cubic(double x, void *data)
{
    (void)data;
    return (x - 1.0) * (x - 1.0) * (x - 1.0) + 0.5;
}

static double shifted_cubic_derivative(double x, void *data)
{
    (void)data;
    return 3.0 * (x - 1.0) * (x - 1.0);
}

static double quadratic_root(double x, void *data)
{
    (void)data;
    return x * x - 2.0;
}

static double constant_one(double x, void *data)
{
    (void)x;
    (void)data;
    return 1.0;
}

static double constant_maximum(double x, void *data)
{
    (void)x;
    (void)data;
    return DBL_MAX;
}

static void test_safeguarded_newton(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    status = numerix_root_safeguarded_newton(
        shifted_cubic, shifted_cubic_derivative, NULL, 0.0, 2.0,
        &options, &result, &error);

    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(shifted_cubic(result.value, NULL)) <= options.absolute_tolerance);
    CHECK(fabs(result.value - 1.0) > 0.1);
}

static void test_bisection_evaluation_limit(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    options.maximum_function_evaluations = 1;
    status = numerix_root_bisection(
        quadratic_root, NULL, 0.0, 2.0, &options, &result, &error);

    CHECK(status == NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(isnan(result.residual));
}

static void test_brent_solver(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    status = numerix_root_brent(
        quadratic_root, NULL, 0.0, 2.0, &options, &result, &error);

    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - sqrt(2.0)) < 1.0e-8);
    CHECK(fabs(result.residual) <= options.absolute_tolerance);
}

static void test_integration_finite_results(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    status = numerix_integrate_simpson(
        constant_one, NULL, -DBL_MAX, DBL_MAX, 2,
        &options, &result, &error);
    CHECK(status != NUMERIX_STATUS_SUCCESS);
    CHECK(!isfinite(result.value));

    status = numerix_integrate_simpson(
        constant_maximum, NULL, 0.0, 2.0, 2,
        &options, &result, &error);
    CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(isnan(result.value));

    status = numerix_integrate_trapezoid(
        constant_maximum, NULL, 0.0, 2.0, 2,
        &options, &result, &error);
    CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(isnan(result.value));

    status = numerix_integrate_adaptive_simpson(
        constant_maximum, NULL, 0.0, 2.0,
        &options, 16, &result, &error);
    CHECK(status != NUMERIX_STATUS_SUCCESS || isfinite(result.value));
}

int main(void)
{
    test_safeguarded_newton();
    test_bisection_evaluation_limit();
    test_brent_solver();
    test_integration_finite_results();

    if (failures != 0) {
        fprintf(stderr, "%d audit regression check(s) failed.\n", failures);
        return 1;
    }

    puts("All audit regression checks passed.");
    return 0;
}
