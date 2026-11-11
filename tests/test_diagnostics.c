#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void check(int condition, const char *name)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

static double root_function(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
}

static double square_function(double x, void *user_data)
{
    (void)user_data;
    return x * x;
}

static void test_root_residual(void)
{
    numerix_options options;
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_execution_output output;
    numerix_diagnostics_report report;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    memset(&request, 0, sizeof(request));
    memset(&execution_options, 0, sizeof(execution_options));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = root_function;
    request.options = &options;
    request.problem.root.lower = 1.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;

    status = numerix_execute_recommended(&request, &execution_options,
                                         &output, &error);
    check(status == NUMERIX_STATUS_SUCCESS, "root execution succeeds");
    status = numerix_diagnostics_analyze(&request, &output, NULL, &report,
                                         &error);
    check(status == NUMERIX_STATUS_SUCCESS, "root diagnostics succeeds");
    check(report.has_residual, "root residual is available");
    check(report.absolute_residual < 1.0e-6,
          "root residual is independently evaluated");
    check(report.reliability == NUMERIX_RELIABILITY_ACCEPTABLE,
          "successful root with small residual is acceptable");

    numerix_diagnostics_report_destroy(&report);
    numerix_execution_output_destroy(&output);
}

static void test_linear_residual_and_condition(void)
{
    numerix_options options;
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_execution_output output;
    numerix_diagnostics_options diagnostics_options;
    numerix_diagnostics_report report;
    numerix_matrix *matrix = NULL;
    numerix_vector *rhs = NULL;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    (void)numerix_matrix_create(2, 2, &matrix, &error);
    (void)numerix_vector_create(2, &rhs, &error);
    matrix->data[0] = 4.0;
    matrix->data[1] = 1.0;
    matrix->data[2] = 2.0;
    matrix->data[3] = 3.0;
    rhs->data[0] = 1.0;
    rhs->data[1] = 2.0;

    memset(&request, 0, sizeof(request));
    memset(&execution_options, 0, sizeof(execution_options));
    request.category = NUMERIX_PROBLEM_LINEAR_ALGEBRA;
    request.options = &options;
    request.problem.linear.matrix = matrix;
    request.problem.linear.right_hand_side = rhs;
    status = numerix_execute_recommended(&request, &execution_options,
                                         &output, &error);
    check(status == NUMERIX_STATUS_SUCCESS, "linear solve succeeds");

    diagnostics_options.estimate_linear_condition = 1;
    status = numerix_diagnostics_analyze(&request, &output,
                                         &diagnostics_options, &report,
                                         &error);
    check(status == NUMERIX_STATUS_SUCCESS, "linear diagnostics succeeds");
    check(report.has_residual_vector, "linear residual vector is returned");
    check(report.has_backward_error, "linear backward error is available");
    check(report.normwise_backward_error < 1.0e-14,
          "linear backward error is small");
    check(report.has_condition_estimate, "condition estimate is available");
    check(fabs(report.condition_estimate - 3.0) < 1.0e-12,
          "infinity norm condition estimate matches known value");

    numerix_diagnostics_report_destroy(&report);
    numerix_execution_output_destroy(&output);
    numerix_vector_destroy(rhs);
    numerix_matrix_destroy(matrix);
}

static void test_integration_error_and_sensitivity(void)
{
    numerix_options options;
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_execution_output output;
    numerix_diagnostics_report diagnostics;
    numerix_sensitivity_report sensitivity;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    memset(&request, 0, sizeof(request));
    memset(&execution_options, 0, sizeof(execution_options));
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = square_function;
    request.options = &options;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 1.0;
    request.problem.integration.panels = 100;
    execution_options.explicit_algorithm =
        NUMERIX_ALGORITHM_INTEGRATE_ADAPTIVE_SIMPSON;
    execution_options.panels = 100;
    execution_options.adaptive_depth = 20;

    status = numerix_execute_recommended(&request, &execution_options,
                                         &output, &error);
    check(status == NUMERIX_STATUS_SUCCESS, "integration execution succeeds");
    status = numerix_diagnostics_analyze(&request, &output, NULL,
                                         &diagnostics, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "integration diagnostics succeeds");
    check(diagnostics.has_estimated_error,
          "integration estimate is reported as estimated error");
    numerix_diagnostics_report_destroy(&diagnostics);
    numerix_execution_output_destroy(&output);

    status = numerix_diagnostics_scalar_sensitivity(
        square_function, NULL, 3.0, 0.01, &options, &sensitivity, &error);
    check(status == NUMERIX_STATUS_SUCCESS, "scalar sensitivity succeeds");
    check(fabs(sensitivity.derivative - 6.0) < 1.0e-7,
          "Richardson sensitivity matches derivative");
    check(sensitivity.has_relative_sensitivity,
          "relative sensitivity is available for nonzero output");
    check(fabs(sensitivity.relative_sensitivity - 2.0) < 1.0e-7,
          "relative sensitivity matches known value");
}

static void test_interpolation_data_residual(void)
{
    const double x_values[] = {0.0, 1.0, 2.0};
    const double y_values[] = {1.0, 2.0, 5.0};
    numerix_options options;
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_execution_output output;
    numerix_diagnostics_report diagnostics;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    memset(&request, 0, sizeof(request));
    memset(&execution_options, 0, sizeof(execution_options));
    request.category = NUMERIX_PROBLEM_INTERPOLATION;
    request.options = &options;
    request.problem.interpolation.x = x_values;
    request.problem.interpolation.y = y_values;
    request.problem.interpolation.count = 3;
    request.problem.interpolation.query = 1.5;
    execution_options.explicit_algorithm = NUMERIX_ALGORITHM_INTERP_LAGRANGE;

    status = numerix_execute_recommended(&request, &execution_options,
                                         &output, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "interpolation execution succeeds");
    status = numerix_diagnostics_analyze(&request, &output, NULL,
                                         &diagnostics, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "interpolation diagnostics succeeds");
    check(diagnostics.has_data_residual,
          "interpolation data residual is available");
    check(diagnostics.maximum_data_residual < 1.0e-12,
          "interpolant reproduces supplied data points");

    numerix_diagnostics_report_destroy(&diagnostics);
    numerix_execution_output_destroy(&output);
}

static void test_managed_execution_diagnostics(void)
{
    numerix_options options;
    numerix_recommendation_request request;
    numerix_managed_execution_options managed_options;
    numerix_managed_execution *execution = NULL;
    const numerix_execution_output *output = NULL;
    numerix_diagnostics_report diagnostics;
    numerix_error error;
    numerix_status status;

    numerix_options_init(&options);
    memset(&request, 0, sizeof(request));
    memset(&managed_options, 0, sizeof(managed_options));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = root_function;
    request.options = &options;
    request.problem.root.lower = 1.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;

    status = numerix_managed_execution_create(
        &request, &managed_options, &execution, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "managed execution creation succeeds");
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = numerix_managed_execution_run(execution, &error);
        check(status == NUMERIX_STATUS_SUCCESS,
              "managed root execution succeeds");
        status = numerix_managed_execution_get_output(
            execution, &output, &error);
        check(status == NUMERIX_STATUS_SUCCESS,
              "managed execution output is available");
        if (status == NUMERIX_STATUS_SUCCESS)
        {
            status = numerix_diagnostics_analyze(
                &request, output, NULL, &diagnostics, &error);
            check(status == NUMERIX_STATUS_SUCCESS,
                  "managed output can be diagnosed");
            if (status == NUMERIX_STATUS_SUCCESS)
            {
                check(diagnostics.has_residual,
                      "managed output has root residual evidence");
                numerix_diagnostics_report_destroy(&diagnostics);
            }
        }
    }

    numerix_managed_execution_destroy(execution);
}

int main(void)
{
    test_root_residual();
    test_linear_residual_and_condition();
    test_integration_error_and_sensitivity();
    test_interpolation_data_residual();
    test_managed_execution_diagnostics();

    if (failures != 0)
    {
        fprintf(stderr, "%d diagnostics test(s) failed.\n", failures);
        return 1;
    }

    printf("All diagnostics tests passed.\n");
    return 0;
}
