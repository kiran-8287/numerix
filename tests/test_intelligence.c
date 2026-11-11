#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static double quadratic(double x, void *data)
{
    (void)data;
    return x * x - 2.0;
}

static double quadratic_derivative(double x, void *data)
{
    (void)data;
    return 2.0 * x;
}

static double zero_derivative_root(double x, void *data)
{
    (void)data;
    return (x - 1.0) * (x - 1.0) * (x - 1.0) + 0.5;
}

static double zero_derivative(double x, void *data)
{
    (void)data;
    return 3.0 * (x - 1.0) * (x - 1.0);
}

static double scalar_objective(double x, void *data)
{
    (void)data;
    return (x - 2.0) * (x - 2.0);
}

static numerix_status ode_rhs(
    double x, const double *state, size_t dimension,
    double *derivative, void *data)
{
    (void)x;
    (void)data;
    if (dimension != 1)
    {
        return NUMERIX_STATUS_INVALID_DIMENSIONS;
    }
    derivative[0] = state[0];
    return NUMERIX_STATUS_SUCCESS;
}

static double vector_objective(const double *values, size_t dimension, void *data)
{
    (void)data;
    if (dimension != 1)
    {
        return NAN;
    }
    return (values[0] - 3.0) * (values[0] - 3.0);
}

static numerix_status vector_gradient(
    const double *values, size_t dimension, double *gradient, void *data)
{
    (void)data;
    if (dimension != 1)
    {
        return NUMERIX_STATUS_INVALID_DIMENSIONS;
    }
    gradient[0] = 2.0 * (values[0] - 3.0);
    return NUMERIX_STATUS_SUCCESS;
}

static void initialize_root_request(numerix_recommendation_request *request)
{
    memset(request, 0, sizeof(*request));
    request->category = NUMERIX_PROBLEM_ROOT_FINDING;
    request->representation = NUMERIX_REPRESENTATION_CALLBACK;
    request->function = quadratic;
    request->derivative = quadratic_derivative;
    request->problem.root.lower = 0.0;
    request->problem.root.upper = 2.0;
    request->problem.root.initial = 1.0;
    request->problem.root.second_initial = 2.0;
    request->problem.root.has_bracket = 1;
    request->problem.root.has_initial = 1;
    request->problem.root.has_second_initial = 1;
}

static void test_root_recommendation_and_selection(void)
{
    numerix_recommendation_request request;
    numerix_root_selection_options selection;
    numerix_root_selection_report report;
    numerix_recommendation recommendation;
    numerix_result result;
    numerix_error error;

    initialize_root_request(&request);
    request.derivative = NULL;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm == NUMERIX_ALGORITHM_BRENT);
    CHECK(recommendation.sign_changing_bracket);
    CHECK(recommendation.bracket_was_checked);
    CHECK(!recommendation.derivative_available);
    CHECK(recommendation.probe_evaluations == 2);
    CHECK(recommendation.reason[0][0] != '\0');

    CHECK(numerix_root_solve_auto(
        &request, NULL, &result, &report, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(report.initially_selected == NUMERIX_ALGORITHM_BRENT);
    CHECK(report.attempt_count == 1);
    CHECK(fabs(result.value - sqrt(2.0)) < 1.0e-8);

    request.function = zero_derivative_root;
    request.derivative = zero_derivative;
    request.problem.root.initial = 1.0;
    selection.mode = NUMERIX_SELECTION_EXPLICIT;
    selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    selection.enable_fallback = 0;
    CHECK(numerix_root_solve_auto(
        &request, &selection, &result, &report, &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(report.initially_selected == NUMERIX_ALGORITHM_BISECTION);
    CHECK(!report.fallback_used);

    selection.explicit_algorithm = NUMERIX_ALGORITHM_NEWTON;
    selection.enable_fallback = 1;
    CHECK(numerix_root_solve_auto(
        &request, &selection, &result, &report, &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(report.initially_selected == NUMERIX_ALGORITHM_NEWTON);
    CHECK(report.recommendation.derivative_checked);
    CHECK(report.recommendation.derivative_appears_zero_or_non_finite);
    CHECK(report.recommendation.recommended_algorithm == NUMERIX_ALGORITHM_BRENT);
    CHECK(report.fallback_used);
    CHECK(report.attempt_count == 2);
    CHECK(report.attempts[0].status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(report.attempts[1].algorithm == NUMERIX_ALGORITHM_BRENT);
    CHECK(report.attempts[0].error.code == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(fabs(zero_derivative_root(result.value, NULL)) < 1.0e-7);

    selection.enable_fallback = 0;
    CHECK(numerix_root_solve_auto(
        &request, &selection, &result, &report, &error) ==
        NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(report.attempt_count == 1);
    CHECK(!report.fallback_used);
}

static void test_invalid_root_and_portfolio(void)
{
    numerix_recommendation_request request;
    numerix_root_portfolio_report report;
    numerix_root_selection_report selection_report;
    numerix_root_portfolio_options portfolio_options;
    numerix_result result;
    numerix_error error;

    initialize_root_request(&request);
    CHECK(numerix_root_solve_portfolio(
        &request, NULL, &result, &report, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(report.attempt_count >= 3);
    CHECK(report.has_selected_attempt);
    CHECK(report.successful_results_agree);
    CHECK(fabs(result.value - sqrt(2.0)) < 1.0e-8);
    CHECK(report.total_function_evaluations >= result.function_evaluations);

    portfolio_options.algorithm_mask = 1U << NUMERIX_ALGORITHM_BISECTION;
    request.problem.root.upper = 1.0;
    CHECK(numerix_root_solve_portfolio(
        &request, &portfolio_options, &result, &report, &error) ==
        NUMERIX_STATUS_INVALID_INPUT);
    CHECK(!report.has_selected_attempt);
    CHECK(report.attempt_count == 1);
    CHECK(report.attempts[0].status == NUMERIX_STATUS_INVALID_INPUT);

    request.problem.root.has_bracket = 0;
    request.problem.root.has_initial = 0;
    request.problem.root.has_second_initial = 0;
    CHECK(numerix_recommend_algorithm(
        &request, &(numerix_recommendation){0}, &error) ==
        NUMERIX_STATUS_INVALID_INPUT);
    CHECK(numerix_root_solve_auto(
        &request, NULL, &result, &selection_report, &error) ==
        NUMERIX_STATUS_INVALID_INPUT);
    CHECK(selection_report.attempt_count == 0);
    CHECK(!selection_report.fallback_used);

    initialize_root_request(&request);
    {
        numerix_options options;
        numerix_options_init(&options);
        options.maximum_function_evaluations = 1;
        request.options = &options;
        CHECK(numerix_root_solve_auto(
            &request, NULL, &result, &selection_report, &error) ==
            NUMERIX_STATUS_LIMIT_REACHED);
        CHECK(selection_report.attempt_count == 0);
        CHECK(!selection_report.fallback_used);

        portfolio_options.algorithm_mask =
            1U << NUMERIX_ALGORITHM_BRENT;
        CHECK(numerix_root_solve_portfolio(
            &request, &portfolio_options, &result, &report, &error) ==
            NUMERIX_STATUS_LIMIT_REACHED);
        CHECK(report.attempt_count == 1);
        CHECK(report.attempts[0].status == NUMERIX_STATUS_LIMIT_REACHED);
    }
}

static void test_category_recommendations(void)
{
    numerix_recommendation_request request;
    numerix_recommendation recommendation;
    numerix_error error;
    double matrix_data[] = {4.0, 1.0, 1.0, 3.0};
    double rhs_data[] = {1.0, 2.0};
    numerix_matrix matrix = {2, 2, matrix_data};
    numerix_vector rhs = {2, rhs_data};
    double x_values[] = {0.0, 1.0, 2.0};
    double y_values[] = {0.0, 1.0, 4.0};
    double ode_state[] = {1.0};

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_LINEAR_ALGEBRA;
    request.problem.linear.matrix = &matrix;
    request.problem.linear.right_hand_side = &rhs;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm == NUMERIX_ALGORITHM_LU);
    matrix_data[0] = 1.0;
    matrix_data[1] = 2.0;
    matrix_data[2] = 2.0;
    matrix_data[3] = 4.0;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SINGULAR_MATRIX);
    matrix_data[0] = 4.0;
    matrix_data[1] = 1.0;
    matrix_data[2] = 1.0;
    matrix_data[3] = 3.0;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_DIFFERENTIATION;
    request.function = quadratic;
    request.problem.differentiation.x = 1.0;
    request.problem.differentiation.step = 1.0e-4;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm == NUMERIX_ALGORITHM_DIFF_CENTRAL);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTERPOLATION;
    request.problem.interpolation.x = x_values;
    request.problem.interpolation.y = y_values;
    request.problem.interpolation.count = 3;
    request.problem.interpolation.query = 1.5;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm == NUMERIX_ALGORITHM_INTERP_NEWTON);
    x_values[2] = 1.0;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_INVALID_INPUT);
    x_values[2] = 2.0;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.function = quadratic;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 1.0;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm ==
          NUMERIX_ALGORITHM_INTEGRATE_ADAPTIVE_SIMPSON);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_OPTIMIZATION;
    request.function = scalar_objective;
    request.problem.optimization.bounded = 1;
    request.problem.optimization.lower = -1.0;
    request.problem.optimization.upper = 5.0;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm ==
          NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ODE;
    request.problem.ode.function = ode_rhs;
    request.problem.ode.initial_state = ode_state;
    request.problem.ode.dimension = 1;
    request.problem.ode.step = 0.01;
    request.problem.ode.steps = 10;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(recommendation.recommended_algorithm == NUMERIX_ALGORITHM_ODE_RK4);
    ode_state[0] = NAN;
    CHECK(numerix_recommend_algorithm(
        &request, &recommendation, &error) == NUMERIX_STATUS_INVALID_INPUT);
    ode_state[0] = 1.0;

    {
        double initial_data[] = {0.0};
        numerix_vector initial = {1, initial_data};
        memset(&request, 0, sizeof(request));
        request.category = NUMERIX_PROBLEM_OPTIMIZATION;
        request.problem.optimization.objective = vector_objective;
        request.problem.optimization.gradient = vector_gradient;
        request.problem.optimization.initial = &initial;
        request.problem.optimization.initial_step = 0.25;
        CHECK(numerix_recommend_algorithm(
            &request, &recommendation, &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(recommendation.recommended_algorithm ==
              NUMERIX_ALGORITHM_OPTIMIZE_GRADIENT_DESCENT);
    }
}

static void test_expression_and_piecewise_execution(void)
{
    numerix_recommendation_request request;
    numerix_execution_output output;
    numerix_error error;
    numerix_expression *expression = NULL;
    numerix_piecewise_function *piecewise = NULL;

    CHECK(numerix_expression_compile("x^2-2", &expression, &error) ==
          NUMERIX_STATUS_SUCCESS);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_EXPRESSION;
    request.expression = expression;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;
    CHECK(numerix_execute_recommended(
        &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output.selected_automatically);
    CHECK(output.algorithm == NUMERIX_ALGORITHM_BRENT);
    CHECK(fabs(output.scalar_result.value - sqrt(2.0)) < 1.0e-8);
    numerix_execution_output_destroy(&output);
    numerix_expression_destroy(expression);

    CHECK(numerix_piecewise_create(&piecewise, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
        piecewise, -3.0, -1.0, 1, 1, "x+2", &error) ==
        NUMERIX_STATUS_SUCCESS);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    request.problem.root.lower = -3.0;
    request.problem.root.upper = -1.0;
    request.problem.root.has_bracket = 1;
    CHECK(numerix_execute_recommended(
        &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(output.scalar_result.value + 2.0) < 1.0e-8);
    numerix_execution_output_destroy(&output);
    numerix_piecewise_destroy(piecewise);

    CHECK(numerix_piecewise_create(&piecewise, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
        piecewise, -1.0, 0.0, 1, 0, "-1", &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
        piecewise, 0.0, 1.0, 1, 1, "1", &error) ==
        NUMERIX_STATUS_SUCCESS);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    request.problem.root.lower = -1.0;
    request.problem.root.upper = 1.0;
    request.problem.root.has_bracket = 1;
    CHECK(numerix_recommend_algorithm(
        &request, &output.recommendation, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output.recommendation.crosses_piecewise_boundaries);
    CHECK(output.recommendation.piecewise_boundary_count == 1);
    {
        double boundaries[4];
        size_t boundary_count = 0;
        CHECK(numerix_piecewise_get_boundaries(
            piecewise, boundaries, 4, &boundary_count, &error) ==
            NUMERIX_STATUS_SUCCESS);
        CHECK(boundary_count == 3);
        CHECK(boundaries[0] == -1.0);
        CHECK(boundaries[1] == 0.0);
        CHECK(boundaries[2] == 1.0);
    }
    CHECK(output.recommendation.suitability == NUMERIX_SUITABILITY_CONDITIONAL);
    CHECK(numerix_execute_recommended(
        &request, NULL, &output, &error) != NUMERIX_STATUS_SUCCESS);
    numerix_execution_output_destroy(&output);
    numerix_piecewise_destroy(piecewise);
}

static void test_generic_execution(void)
{
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_execution_output output;
    numerix_error error;
    numerix_expression *expression = NULL;
    double matrix_data[] = {4.0, 1.0, 1.0, 3.0};
    double right_data[] = {1.0, 2.0};
    numerix_matrix matrix = {2, 2, matrix_data};
    numerix_vector right = {2, right_data};

    CHECK(numerix_expression_compile("x^2", &expression, &error) ==
          NUMERIX_STATUS_SUCCESS);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.representation = NUMERIX_REPRESENTATION_EXPRESSION;
    request.expression = expression;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 1.0;
    memset(&execution_options, 0, sizeof(execution_options));
    execution_options.panels = 100;
    CHECK(numerix_execute_recommended(
        &request, &execution_options, &output, &error) ==
        NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(output.scalar_result.value - (1.0 / 3.0)) < 1.0e-6);
    numerix_execution_output_destroy(&output);

    request.category = NUMERIX_PROBLEM_DIFFERENTIATION;
    request.problem.differentiation.x = 1.0;
    request.problem.differentiation.step = 1.0e-4;
    CHECK(numerix_execute_recommended(
        &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(output.scalar_result.value - 2.0) < 1.0e-7);
    numerix_execution_output_destroy(&output);
    numerix_expression_destroy(expression);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_LINEAR_ALGEBRA;
    request.problem.linear.matrix = &matrix;
    request.problem.linear.right_hand_side = &right;
    CHECK(numerix_execute_recommended(
        &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output.vector_result != NULL);
    if (output.vector_result != NULL)
    {
        CHECK(fabs(output.vector_result->data[0] - (1.0 / 11.0)) < 1.0e-10);
        CHECK(fabs(output.vector_result->data[1] - (7.0 / 11.0)) < 1.0e-10);
    }
    numerix_execution_output_destroy(&output);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_OPTIMIZATION;
    request.function = scalar_objective;
    request.problem.optimization.bounded = 1;
    request.problem.optimization.lower = -1.0;
    request.problem.optimization.upper = 5.0;
    CHECK(numerix_execute_recommended(
        &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(output.scalar_result.value - 2.0) < 1.0e-5);
    numerix_execution_output_destroy(&output);

    {
        double x_values[] = {0.0, 1.0, 2.0};
        double y_values[] = {0.0, 1.0, 4.0};
        memset(&request, 0, sizeof(request));
        request.category = NUMERIX_PROBLEM_INTERPOLATION;
        request.problem.interpolation.x = x_values;
        request.problem.interpolation.y = y_values;
        request.problem.interpolation.count = 3;
        request.problem.interpolation.query = 1.5;
        CHECK(numerix_execute_recommended(
            &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(isfinite(output.scalar_result.value));
        numerix_execution_output_destroy(&output);
    }

    {
        double initial_state[] = {1.0};
        memset(&request, 0, sizeof(request));
        request.category = NUMERIX_PROBLEM_ODE;
        request.problem.ode.function = ode_rhs;
        request.problem.ode.initial_state = initial_state;
        request.problem.ode.dimension = 1;
        request.problem.ode.step = 0.01;
        request.problem.ode.steps = 10;
        CHECK(numerix_execute_recommended(
            &request, NULL, &output, &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(output.has_ode_result);
        CHECK(output.ode_result.points == 11);
        numerix_execution_output_destroy(&output);
    }
}

int main(void)
{
    test_root_recommendation_and_selection();
    test_invalid_root_and_portfolio();
    test_category_recommendations();
    test_expression_and_piecewise_execution();
    test_generic_execution();

    if (failures > 0)
    {
        fprintf(stderr, "%d intelligence check(s) failed.\n", failures);
        return 1;
    }
    puts("All intelligence checks passed.");
    return 0;
}
