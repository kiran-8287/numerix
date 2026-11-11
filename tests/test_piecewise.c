#include "numerix.h"

#include <math.h>
#include <stdio.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static numerix_piecewise_function *make_function(void)
{
    numerix_piecewise_function *function = NULL;
    numerix_error error;

    CHECK(numerix_piecewise_create(&function, &error) == NUMERIX_STATUS_SUCCESS);
    return function;
}

static void test_evaluation_and_validation(void)
{
    numerix_piecewise_function *function = make_function();
    numerix_error error;
    double value = NAN;

    CHECK(numerix_piecewise_add_expression(
        function, -INFINITY, 0.0, 0, 0, "x^2", &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
        function, 0.0, INFINITY, 1, 0, "2*x+1", &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_evaluate(function, -2.0, &value, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(value - 4.0) < 1.0e-12);
    CHECK(numerix_piecewise_evaluate(function, 0.0, &value, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(value - 1.0) < 1.0e-12);
    CHECK(numerix_piecewise_evaluate(function, 2.0, &value, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(value - 5.0) < 1.0e-12);
    CHECK(numerix_piecewise_evaluate(function, -1.0e-12, &value, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(value > 0.0 && value < 1.0e-20);

    CHECK(numerix_piecewise_add_expression(
        function, -1.0, 1.0, 1, 1, "x", &error) ==
        NUMERIX_STATUS_INVALID_INPUT);
    CHECK(numerix_piecewise_add_expression(
        function, 2.0, 2.0, 1, 1, "x", &error) ==
        NUMERIX_STATUS_INVALID_INPUT);
    CHECK(numerix_piecewise_add_expression(
        function, NAN, 3.0, 0, 0, "x", &error) ==
        NUMERIX_STATUS_INVALID_INPUT);
    CHECK(numerix_piecewise_evaluate(function, -INFINITY, &value, &error) ==
          NUMERIX_STATUS_INVALID_ARGUMENT);
    numerix_piecewise_destroy(function);
}

static void test_boundary_diagnostics(void)
{
    numerix_piecewise_function *continuous = make_function();
    numerix_piecewise_function *jump = make_function();
    numerix_piecewise_function *removable = make_function();
    numerix_piecewise_function *infinite = make_function();
    numerix_piecewise_boundary_report report;
    numerix_error error;

    numerix_piecewise_add_expression(continuous, -INFINITY, 1.0, 0, 0,
                                    "x^2", &error);
    numerix_piecewise_add_expression(continuous, 1.0, INFINITY, 1, 0,
                                    "x^2", &error);
    CHECK(numerix_piecewise_analyze_boundary(
        continuous, 1.0, NULL, &report, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(report.type == NUMERIX_DISCONTINUITY_CONTINUOUS);

    numerix_piecewise_add_expression(jump, -INFINITY, 0.0, 0, 0, "x", &error);
    numerix_piecewise_add_expression(jump, 0.0, INFINITY, 1, 0, "x+2", &error);
    numerix_piecewise_analyze_boundary(jump, 0.0, NULL, &report, &error);
    CHECK(report.type == NUMERIX_DISCONTINUITY_JUMP);
    CHECK(report.confidence == NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED);

    numerix_piecewise_add_expression(removable, -INFINITY, 1.0, 0, 0,
                                    "(x^2-1)/(x-1)", &error);
    numerix_piecewise_add_expression(removable, 1.0, INFINITY, 0, 0,
                                    "(x^2-1)/(x-1)", &error);
    numerix_piecewise_analyze_boundary(removable, 1.0, NULL, &report, &error);
    CHECK(report.type == NUMERIX_DISCONTINUITY_REMOVABLE);
    CHECK(report.function_status != NUMERIX_STATUS_SUCCESS);

    numerix_piecewise_add_expression(infinite, 2.0, 3.0, 0, 1,
                                    "1/(x-2)", &error);
    numerix_piecewise_analyze_boundary(infinite, 2.0, NULL, &report, &error);
    CHECK(report.type == NUMERIX_DISCONTINUITY_INFINITE);
    CHECK(report.confidence == NUMERIX_DIAGNOSTIC_SUSPECTED);

    numerix_piecewise_destroy(continuous);
    numerix_piecewise_destroy(jump);
    numerix_piecewise_destroy(removable);
    numerix_piecewise_destroy(infinite);
}

static void test_boundaries_scan_and_callbacks(void)
{
    numerix_piecewise_function *function = make_function();
    numerix_piecewise_boundary_report reports[2];
    numerix_piecewise_candidate candidates[8];
    numerix_piecewise_callback_context callback_context;
    numerix_result result;
    numerix_error error;
    size_t report_count = 0;
    size_t candidate_count = 0;

    numerix_piecewise_add_expression(function, -INFINITY, 0.0, 0, 0,
                                    "x+2", &error);
    numerix_piecewise_add_expression(function, 0.0, INFINITY, 1, 0,
                                    "x-3", &error);
    CHECK(numerix_piecewise_analyze_boundaries(
        function, NULL, reports, 2, &report_count, &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(report_count == 1);
    CHECK(numerix_piecewise_callback_context_init(
        &callback_context, function, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_root_bisection(
        numerix_piecewise_callback, &callback_context, -3.0, -1.0,
        NULL, &result, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value + 2.0) < 1.0e-7);
    CHECK(numerix_piecewise_callback_status(&callback_context) ==
          NUMERIX_STATUS_SUCCESS);

    numerix_piecewise_destroy(function);

    function = make_function();
    numerix_piecewise_add_expression(function, 0.0, 4.0, 1, 1,
                                    "1/(x-2)", &error);
    CHECK(numerix_piecewise_scan_interval(
        function, 0.0, 4.0, 8, candidates, 8,
        &candidate_count, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(candidate_count > 0);
    numerix_piecewise_destroy(function);
}

static void test_integration_and_differentiation(void)
{
    numerix_piecewise_function *function = make_function();
    numerix_piecewise_function *gapped = make_function();
    numerix_result result;
    numerix_error error;

    numerix_piecewise_add_expression(function, -INFINITY, 0.0, 0, 0,
                                    "x^2", &error);
    numerix_piecewise_add_expression(function, 0.0, INFINITY, 1, 0,
                                    "2*x", &error);
    CHECK(numerix_piecewise_integrate_trapezoid(
        function, -2.0, 2.0, 100, NULL, &result, &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - (20.0 / 3.0)) < 0.01);
    CHECK(numerix_piecewise_diff_central(
        function, 1.0, 1.0e-4, NULL, &result, &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - 2.0) < 1.0e-6);
    CHECK(numerix_piecewise_diff_central(
        function, 0.0, 1.0e-4, NULL, &result, &error) ==
        NUMERIX_STATUS_INVALID_INPUT);

    numerix_piecewise_add_expression(gapped, 0.0, 1.0, 1, 0, "x", &error);
    numerix_piecewise_add_expression(gapped, 2.0, 3.0, 0, 1, "x", &error);
    CHECK(numerix_piecewise_integrate_trapezoid(
        gapped, 0.0, 3.0, 10, NULL, &result, &error) ==
        NUMERIX_STATUS_INVALID_INPUT);

    numerix_piecewise_destroy(function);
    numerix_piecewise_destroy(gapped);
}

int main(void)
{
    test_evaluation_and_validation();
    test_boundary_diagnostics();
    test_boundaries_scan_and_callbacks();
    test_integration_and_differentiation();

    if (failures > 0) {
        fprintf(stderr, "%d piecewise check(s) failed.\n", failures);
        return 1;
    }
    puts("All piecewise checks passed.");
    return 0;
}
