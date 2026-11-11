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

static double square_derivative(double x, void *user_data)
{
    (void)user_data;
    return 2.0 * x;
}

static double linear_objective(const double *x, size_t dimension,
                               void *user_data)
{
    (void)user_data;
    if (dimension != 2)
    {
        return NAN;
    }
    return x[0] * x[0] + 3.0 * x[1];
}

static numerix_status exponential_ode(double x, const double *y,
                                     size_t dimension, double *derivative,
                                     void *user_data)
{
    size_t index;

    (void)x;
    (void)user_data;
    for (index = 0; index < dimension; index++)
    {
        derivative[index] = y[index];
    }
    return NUMERIX_STATUS_SUCCESS;
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
    check(report.warning_count == 0,
          "acceptable root report has no unrelated warnings");

    {
        numerix_diagnostics_options diagnostic_options = {0, 1};

        status = numerix_diagnostics_analyze(
            &request, &output, &diagnostic_options, &report, &error);
        check(status == NUMERIX_STATUS_SUCCESS,
              "optional root condition analysis succeeds");
        check(report.has_condition_estimate,
              "root diagnostics include local reciprocal-derivative estimate");
        check(report.additional_function_evaluations >= 6,
              "root condition diagnostics report additional evaluations");
        numerix_diagnostics_report_destroy(&report);
    }

    output.scalar_result.status = NUMERIX_STATUS_NUMERICAL_FAILURE;
    status = numerix_diagnostics_analyze(&request, &output, NULL, &report,
                                         &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "failed root diagnostics still produce a report");
    check(report.warning_count > 0 &&
              strcmp(report.diagnostics[0].identifier,
                     "NUMERIX.SOLVER.NON_SUCCESS") == 0,
          "diagnostic includes a stable structured identifier");

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
    diagnostics_options.estimate_root_condition = 0;
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

static void test_forward_error_and_sensitivity(void)
{
    numerix_forward_error forward;
    numerix_multivariable_sensitivity sensitivity;
    numerix_error_propagation_report propagation;
    numerix_matrix *computed_matrix = NULL;
    numerix_matrix *reference_matrix = NULL;
    numerix_vector *point = NULL;
    numerix_vector *uncertainties = NULL;
    numerix_error error;
    numerix_status status;

    status = numerix_diagnostics_forward_error_scalar(
        2.1, 2.0, &forward, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "scalar forward-error calculation succeeds");
    check(fabs(forward.absolute_error - 0.1) < 1.0e-14,
          "scalar forward error is computed");
    check(forward.has_relative_error,
          "relative forward error is available for nonzero reference");

    (void)numerix_matrix_create(2, 1, &computed_matrix, &error);
    (void)numerix_matrix_create(2, 1, &reference_matrix, &error);
    computed_matrix->data[0] = 4.0;
    computed_matrix->data[1] = 6.0;
    reference_matrix->data[0] = 1.0;
    reference_matrix->data[1] = 2.0;
    status = numerix_diagnostics_forward_error_matrix(
        computed_matrix, reference_matrix, &forward, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "matrix forward-error calculation succeeds");
    check(fabs(forward.absolute_error - 5.0) < 1.0e-14,
          "matrix forward error uses Euclidean flattened norm");
    numerix_matrix_destroy(reference_matrix);
    numerix_matrix_destroy(computed_matrix);

    (void)numerix_vector_create(2, &point, &error);
    (void)numerix_vector_create(2, &uncertainties, &error);
    point->data[0] = 2.0;
    point->data[1] = 1.0;
    uncertainties->data[0] = 0.1;
    uncertainties->data[1] = 0.2;

    status = numerix_diagnostics_multivariable_sensitivity(
        linear_objective, NULL, point, 1.0e-4, &sensitivity, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "multivariable sensitivity succeeds");
    check(fabs(sensitivity.gradient->data[0] - 4.0) < 1.0e-6,
          "first objective partial derivative matches reference");
    check(fabs(sensitivity.gradient->data[1] - 3.0) < 1.0e-6,
          "second objective partial derivative matches reference");
    check(sensitivity.function_evaluations == 9,
          "multivariable sensitivity reports callback evaluations");
    check(sensitivity.has_relative_sensitivities &&
              fabs(sensitivity.relative_sensitivities->data[0] - 8.0 / 7.0) <
                  1.0e-6,
          "relative component sensitivity matches the reference value");

    status = numerix_diagnostics_propagate_independent_uncertainty(
        linear_objective, NULL, point, uncertainties, 1.0e-4,
        &propagation, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "first-order uncertainty propagation succeeds");
    check(fabs(propagation.estimated_standard_uncertainty - sqrt(0.52)) <
              1.0e-6,
          "independent uncertainty propagation matches analytic value");
    check(propagation.assumes_independent_inputs,
          "propagation records independence assumption");

    numerix_multivariable_sensitivity_destroy(&sensitivity);
    numerix_vector_destroy(uncertainties);
    numerix_vector_destroy(point);
}

static void test_root_condition_and_convergence_history(void)
{
    numerix_root_condition_report condition;
    numerix_convergence_report convergence;
    numerix_error error;
    numerix_status status;
    const double residuals[] = {1.0, 0.1, 0.01, 0.001};

    status = numerix_diagnostics_root_condition(
        root_function, square_derivative, NULL, sqrt(2.0), 0.01, NULL,
        &condition, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "root condition estimate succeeds");
    check(condition.condition_available,
          "simple root derivative condition is available");
    check(fabs(condition.reciprocal_derivative_condition -
               1.0 / (2.0 * sqrt(2.0))) < 1.0e-12,
          "root condition matches reciprocal derivative");

    status = numerix_diagnostics_analyze_convergence(
        residuals, sizeof(residuals) / sizeof(residuals[0]),
        &convergence, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "convergence history analysis succeeds");
    check(convergence.trend == NUMERIX_CONVERGENCE_CONVERGING,
          "decreasing residual history is classified as converging");
    check(convergence.has_empirical_order,
          "sufficient residual history produces empirical order");
    check(convergence.rate_uses_residuals_as_error_proxies,
          "empirical order labels residual proxy limitation");
}

static void test_cancellation_indicator(void)
{
    numerix_cancellation_report report;
    numerix_error error;
    numerix_status status;

    status = numerix_diagnostics_check_subtraction(
        100000000.0, 99999999.0, &report, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "subtraction cancellation assessment succeeds");
    check(report.cancellation_suspected,
          "near-equal large operands raise cancellation indicator");
    check(report.has_lost_decimal_digits_estimate &&
              report.estimated_lost_decimal_digits > 7.0,
          "cancellation report estimates lost decimal digits");
}

static void test_periodic_diagnostics(void)
{
    numerix_periodic_diagnostics *observer = NULL;
    numerix_convergence_report report;
    numerix_execution_step step;
    numerix_error error;
    numerix_status status;
    int updated = 0;
    const double history[] = {1.0, 0.5, 0.25};
    size_t index;

    status = numerix_periodic_diagnostics_create(3, 2, &observer, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "periodic diagnostics observer is created");
    memset(&step, 0, sizeof(step));
    step.valid_fields = NUMERIX_STEP_HAS_RESIDUAL;
    for (index = 0; index < 3; index++)
    {
        step.residual = history[index];
        status = numerix_periodic_diagnostics_observe(
            observer, &step, &report, &updated, &error);
        check(status == NUMERIX_STATUS_SUCCESS,
              "periodic observer accepts residual step");
        if (index == 1)
        {
            check(updated, "observer assesses every configured interval");
        }
    }
    status = numerix_periodic_diagnostics_finish(observer, &report, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "periodic observer final assessment succeeds");
    check(report.trend == NUMERIX_CONVERGENCE_CONVERGING,
          "final periodic assessment detects decreasing residuals");
    numerix_periodic_diagnostics_destroy(observer);
}

static void test_phase4_diagnostic_history(void)
{
    numerix_recommendation_request request;
    numerix_recommendation_request prior_request;
    numerix_diagnostics_report previous;
    numerix_recommendation recommendation;
    numerix_error error;
    numerix_status status;

    memset(&request, 0, sizeof(request));
    memset(&previous, 0, sizeof(previous));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = root_function;
    request.derivative = square_derivative;
    request.problem.root.lower = 1.0;
    request.problem.root.upper = 2.0;
    request.problem.root.initial = 1.5;
    request.problem.root.has_bracket = 1;
    request.problem.root.has_initial = 1;
    previous.category = NUMERIX_PROBLEM_ROOT_FINDING;
    previous.algorithm = NUMERIX_ALGORITHM_NEWTON;
    previous.solver_status = NUMERIX_STATUS_SUCCESS;
    previous.reliability = NUMERIX_RELIABILITY_CAUTION;
    previous.has_residual = 1;
    previous.residual_satisfies_tolerance = 0;

    prior_request = request;
    status = numerix_recommend_algorithm_with_diagnostics(
        &request, &prior_request, &previous, &recommendation, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "historical diagnostic recommendation succeeds");
    check(recommendation.historical_diagnostics_used,
          "matching prior diagnostics influence recommendation");
    check(recommendation.recommended_algorithm == NUMERIX_ALGORITHM_BRENT,
          "unreliable Newton history prefers bracketed Brent");

    request.problem.root.upper = 3.0;
    status = numerix_recommend_algorithm_with_diagnostics(
        &request, &prior_request, &previous, &recommendation, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "unrelated problem recommendation succeeds");
    check(!recommendation.historical_diagnostics_used,
          "changed bracket ignores stale diagnostic evidence");
}

static void test_optimization_and_ode_diagnostics(void)
{
    numerix_options options;
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_execution_output output;
    numerix_diagnostics_report diagnostics;
    numerix_error error;
    numerix_status status;
    const double initial_state[] = {1.0};

    numerix_options_init(&options);
    memset(&request, 0, sizeof(request));
    memset(&execution_options, 0, sizeof(execution_options));
    request.category = NUMERIX_PROBLEM_OPTIMIZATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = square_function;
    request.options = &options;
    request.problem.optimization.bounded = 1;
    request.problem.optimization.lower = -1.0;
    request.problem.optimization.upper = 1.0;
    execution_options.explicit_algorithm =
        NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION;
    status = numerix_execute_recommended(&request, &execution_options,
                                         &output, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "golden-section optimization succeeds");
    status = numerix_diagnostics_analyze(&request, &output, NULL,
                                         &diagnostics, &error);
    check(status == NUMERIX_STATUS_SUCCESS,
          "optimization diagnostics succeed");
    check(diagnostics.has_objective_value,
          "optimization objective value is reported");
    check(diagnostics.has_estimated_error,
          "golden-section interval estimate is reported");
    numerix_diagnostics_report_destroy(&diagnostics);
    numerix_execution_output_destroy(&output);

    memset(&request, 0, sizeof(request));
    memset(&execution_options, 0, sizeof(execution_options));
    request.category = NUMERIX_PROBLEM_ODE;
    request.options = &options;
    request.problem.ode.function = exponential_ode;
    request.problem.ode.initial_state = initial_state;
    request.problem.ode.dimension = 1;
    request.problem.ode.initial_x = 0.0;
    request.problem.ode.step = 0.1;
    request.problem.ode.steps = 5;
    execution_options.explicit_algorithm = NUMERIX_ALGORITHM_ODE_RK4;
    status = numerix_execute_recommended(&request, &execution_options,
                                         &output, &error);
    check(status == NUMERIX_STATUS_SUCCESS, "ODE execution succeeds");
    status = numerix_diagnostics_analyze(&request, &output, NULL,
                                         &diagnostics, &error);
    check(status == NUMERIX_STATUS_SUCCESS, "ODE diagnostics succeed");
    check(diagnostics.has_ode_summary && diagnostics.ode_points == 6,
          "ODE solution history summary is reported");
    check(diagnostics.ode_state_is_finite,
          "ODE finite-state check passes");
    check(!diagnostics.has_estimated_error,
          "fixed-step RK4 does not invent a local error estimate");
    numerix_diagnostics_report_destroy(&diagnostics);
    numerix_execution_output_destroy(&output);
}

int main(void)
{
    test_root_residual();
    test_linear_residual_and_condition();
    test_integration_error_and_sensitivity();
    test_interpolation_data_residual();
    test_managed_execution_diagnostics();
    test_forward_error_and_sensitivity();
    test_root_condition_and_convergence_history();
    test_cancellation_indicator();
    test_periodic_diagnostics();
    test_phase4_diagnostic_history();
    test_optimization_and_ode_diagnostics();

    if (failures != 0)
    {
        fprintf(stderr, "%d diagnostics test(s) failed.\n", failures);
        return 1;
    }

    printf("All diagnostics tests passed.\n");
    return 0;
}
