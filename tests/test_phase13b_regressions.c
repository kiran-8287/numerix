#include "numerix.h"

#include <float.h>
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

static double root_callback(double x, void *data)
{
    double root = *(const double *)data;

    return x - root;
}

static double identity_callback(double x, void *data)
{
    (void)data;
    return x;
}

static double shifted_identity_callback(double x, void *data)
{
    const double *origin = data;

    return x - *origin;
}

static double adjacent_bracket_callback(double x, void *data)
{
    double lower = *(const double *)data;

    return x == lower ? -1.0 : 1.0;
}

typedef struct ode_counter {
    size_t calls;
} ode_counter;

static numerix_status constant_ode(double x, const double *state,
                                  size_t dimension, double *derivative,
                                  void *data)
{
    ode_counter *counter = data;
    size_t index;

    (void)x;
    (void)state;
    ++counter->calls;
    for (index = 0; index < dimension; ++index) {
        derivative[index] = 1.0;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static void test_bisection_budget_position_convergence(void)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    double root = 3e-9;
    numerix_status status;
    numerix_recommendation_request request;
    numerix_root_selection_options selection;
    numerix_root_selection_report report;

    numerix_options_init(&options);
    options.absolute_tolerance = 1e-8;
    options.relative_tolerance = 0.0;
    options.maximum_function_evaluations = 2U;
    status = numerix_root_bisection(root_callback, &root, 0.0, 8e-9,
                                    &options, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.function_evaluations == 2U);
    CHECK(fabs(result.value - 4e-9) <= result.estimated_error);
    CHECK(result.estimated_error <= options.absolute_tolerance);
    CHECK(isnan(result.residual));

    root = 1.2;
    status = numerix_root_bisection(root_callback, &root, 0.0, 2.0,
                                    &options, &result, &error);
    CHECK(status == NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(result.function_evaluations == 2U);

    options.maximum_function_evaluations = 3U;
    root = 1.0;
    status = numerix_root_bisection(root_callback, &root, 0.0, 2.0,
                                    &options, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.function_evaluations == 3U);
    CHECK(result.value == root);
    CHECK(result.residual == 0.0);

    {
        double lower = 1.0;
        double upper = nextafter(lower, INFINITY);

        options.absolute_tolerance = upper - lower;
        options.maximum_function_evaluations = 2U;
        status = numerix_root_bisection(adjacent_bracket_callback, &lower,
            lower, upper, &options, &result, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(result.value == lower);
        CHECK(result.residual == -1.0);
        CHECK(result.function_evaluations == 2U);
    }

    root = 3e-9;
    options.absolute_tolerance = 1e-8;
    options.maximum_function_evaluations = 2U;
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = root_callback;
    request.user_data = &root;
    request.options = &options;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 8e-9;
    request.problem.root.has_bracket = 1;
    selection.mode = NUMERIX_SELECTION_EXPLICIT;
    selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    selection.enable_fallback = 0;
    status = numerix_root_solve_auto(&request, &selection, &result,
                                     &report, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(report.final_status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.function_evaluations == 2U);
    CHECK(isnan(result.residual));
}

static void check_scalar_scaled_system(double scale)
{
    double entries[] = {scale};
    double right_entries[] = {scale};
    numerix_matrix matrix = {1U, 1U, entries};
    numerix_vector right = {1U, right_entries};
    numerix_vector *solution = NULL;
    numerix_error error;
    double residual = NAN;
    numerix_status status;

    status = numerix_linear_solve_gaussian(&matrix, &right, NULL,
                                           &solution, &residual, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution != NULL);
    if (solution != NULL) {
        CHECK(fabs(solution->data[0] - 1.0) <= 4.0 * DBL_EPSILON);
    }
    CHECK(residual == 0.0);
    numerix_vector_destroy(solution);

    solution = NULL;
    residual = NAN;
    status = numerix_linear_solve_gauss_jordan(&matrix, &right, NULL,
                                               &solution, &residual, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution != NULL);
    if (solution != NULL) {
        CHECK(fabs(solution->data[0] - 1.0) <= 4.0 * DBL_EPSILON);
    }
    CHECK(residual == 0.0);
    numerix_vector_destroy(solution);
}

static void test_scale_aware_pivoting(void)
{
    double tiny = 1e-310;
    double subnormal = nextafter(0.0, 1.0);
    double entries[] = {tiny, 0.0, 0.0, 1e300};
    double right_entries[] = {2.0 * tiny, -3.0e300};
    numerix_matrix matrix = {2U, 2U, entries};
    numerix_vector right = {2U, right_entries};
    numerix_vector *solution = NULL;
    numerix_error error;
    double residual = NAN;
    numerix_status status;

    check_scalar_scaled_system(1e-310);
    check_scalar_scaled_system(subnormal);
    check_scalar_scaled_system(1e300);
    check_scalar_scaled_system(2.5);

    status = numerix_linear_solve_gaussian(&matrix, &right, NULL,
                                           &solution, &residual, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution != NULL);
    if (solution != NULL) {
        CHECK(fabs(solution->data[0] - 2.0) <= 4.0 * DBL_EPSILON);
        CHECK(fabs(solution->data[1] + 3.0) <= 4.0 * DBL_EPSILON);
    }
    CHECK(residual == 0.0);
    numerix_vector_destroy(solution);
    solution = NULL;
    residual = NAN;
    status = numerix_linear_solve_gauss_jordan(&matrix, &right, NULL,
                                               &solution, &residual, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution != NULL);
    if (solution != NULL) {
        CHECK(fabs(solution->data[0] - 2.0) <= 4.0 * DBL_EPSILON);
        CHECK(fabs(solution->data[1] + 3.0) <= 4.0 * DBL_EPSILON);
    }
    CHECK(residual == 0.0);
    numerix_vector_destroy(solution);

    {
        double singular_entries[] = {1.0, 2.0, 2.0, 4.0};
        double singular_rhs[] = {1.0, 2.0};
        numerix_matrix singular = {2U, 2U, singular_entries};
        numerix_vector singular_right = {2U, singular_rhs};

        status = numerix_linear_solve_gaussian(&singular, &singular_right, NULL,
                                               &solution, NULL, &error);
        CHECK(status == NUMERIX_STATUS_SINGULAR_MATRIX);
        CHECK(solution == NULL);
    }
    {
        double near_entries[] = {1.0, 1.0, 1.0, 1.0 + DBL_EPSILON};
        double near_rhs[] = {2.0, 2.0 + DBL_EPSILON};
        numerix_matrix nearly_singular = {2U, 2U, near_entries};
        numerix_vector nearly_singular_right = {2U, near_rhs};

        status = numerix_linear_solve_gaussian(&nearly_singular,
                                               &nearly_singular_right, NULL,
                                               &solution, NULL, &error);
        CHECK(status == NUMERIX_STATUS_SINGULAR_MATRIX);
        CHECK(solution == NULL);
    }
    {
        double pivot_entries[] = {0.0, 1.0, 2.0, 3.0};
        double pivot_rhs[] = {2.0, 8.0};
        numerix_matrix pivot_matrix = {2U, 2U, pivot_entries};
        numerix_vector pivot_right = {2U, pivot_rhs};

        status = numerix_linear_solve_gauss_jordan(&pivot_matrix, &pivot_right,
                                                   NULL, &solution, NULL, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(solution != NULL);
        if (solution != NULL) {
            CHECK(fabs(solution->data[0] - 1.0) <= 4.0 * DBL_EPSILON);
            CHECK(fabs(solution->data[1] - 2.0) <= 4.0 * DBL_EPSILON);
        }
        numerix_vector_destroy(solution);
    }
}

static void test_lu_rejects_overflowed_factors(void)
{
    double entries[] = {0.75 * DBL_MAX, 0.75 * DBL_MAX,
                        0.75 * DBL_MAX, -0.75 * DBL_MAX};
    numerix_matrix matrix = {2U, 2U, entries};
    numerix_lu factorization = {0};
    numerix_error error;
    numerix_status status;

    status = numerix_lu_decompose(&matrix, NULL, &factorization, &error);
    CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(factorization.data == NULL);
    CHECK(factorization.permutation == NULL);
    CHECK(factorization.size == 0U);
    numerix_lu_destroy(&factorization);
}

static void check_collapsed_stencil(numerix_status (*differentiate)(
    numerix_function, void *, double, double, const numerix_options *,
    numerix_result *, numerix_error *))
{
    numerix_result result;
    numerix_error error;
    numerix_status status;

    status = differentiate(identity_callback, NULL, 1e16, 0.25, NULL,
                           &result, &error);
    CHECK(status == NUMERIX_STATUS_INVALID_INPUT);
    CHECK(result.status == NUMERIX_STATUS_INVALID_INPUT);
    CHECK(result.function_evaluations == 0U);
}

static void test_differentiation_rejects_collapsed_stencils(void)
{
    numerix_result result;
    numerix_error error;
    numerix_status status;
    double x = 1e16;
    double h = 2.0;

    check_collapsed_stencil(numerix_diff_forward);
    check_collapsed_stencil(numerix_diff_backward);
    check_collapsed_stencil(numerix_diff_central);
    check_collapsed_stencil(numerix_diff_central_five_point);
    check_collapsed_stencil(numerix_diff_richardson);
    check_collapsed_stencil(numerix_diff_second);

    status = numerix_diff_central(identity_callback, NULL, x, h, NULL,
                                  &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - 1.0) <= 4.0 * DBL_EPSILON);
    CHECK(isnan(result.residual));
    status = numerix_diff_central(identity_callback, NULL,
                                  nextafter(x, INFINITY), h, NULL,
                                  &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - 1.0) <= 4.0 * DBL_EPSILON);
    status = numerix_diff_central_five_point(shifted_identity_callback, &x,
                                             x, h, NULL, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - 1.0) <= 4.0 * DBL_EPSILON);
    status = numerix_diff_richardson(shifted_identity_callback, &x, x, 4.0,
                                     NULL, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value - 1.0) <= 4.0 * DBL_EPSILON);

    h = nextafter(0.0, 1.0);
    status = numerix_diff_central(identity_callback, NULL, 0.0, h, NULL,
                                  &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == 1.0);
    status = numerix_diff_richardson(identity_callback, NULL, 0.0, h,
                                     NULL, &result, &error);
    CHECK(status == NUMERIX_STATUS_INVALID_INPUT);
}

static void check_ode_stagnation(numerix_status (*integrate)(
    numerix_ode_function, void *, double, const double *, size_t,
    double, size_t, numerix_ode_solution *, numerix_error *), double step)
{
    double initial[] = {1.0, 2.0};
    ode_counter counter = {0U};
    numerix_ode_solution solution = {0};
    numerix_error error;
    numerix_status status;

    status = integrate(constant_ode, &counter, 1e16, initial, 2U,
                       step, 1U, &solution, &error);
    CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(solution.status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(solution.points == 0U);
    CHECK(solution.x == NULL);
    CHECK(solution.y == NULL);
    CHECK(counter.calls == 0U);
    numerix_ode_solution_destroy(&solution);
}

static void test_ode_rejects_stagnant_and_collapsed_stages(void)
{
    numerix_status (*methods[])(numerix_ode_function, void *, double,
        const double *, size_t, double, size_t, numerix_ode_solution *,
        numerix_error *) = {
            numerix_ode_euler, numerix_ode_heun, numerix_ode_rk4
        };
    double initial = 1.0;
    ode_counter counter = {0U};
    numerix_ode_solution solution = {0};
    numerix_error error;
    numerix_status status;
    size_t method;

    for (method = 0; method < sizeof(methods) / sizeof(methods[0]); ++method) {
        check_ode_stagnation(methods[method], 0.25);
        check_ode_stagnation(methods[method], -0.25);
    }

    status = numerix_ode_euler(constant_ode, &counter, 1e16, &initial, 1U,
                               2.0, 1U, &solution, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution.points == 2U);
    CHECK(solution.x[1] == 1e16 + 2.0);
    numerix_ode_solution_destroy(&solution);

    counter.calls = 0U;
    status = numerix_ode_heun(constant_ode, &counter, 0.0, &initial, 1U,
                              -0.25, 2U, &solution, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution.x[2] == -0.5);
    CHECK(fabs(solution.y[2] - 0.5) <= 4.0 * DBL_EPSILON);
    numerix_ode_solution_destroy(&solution);

    counter.calls = 0U;
    status = numerix_ode_rk4(constant_ode, &counter, 1e16, &initial, 1U,
                             2.0, 1U, &solution, &error);
    CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(solution.points == 0U);
    CHECK(counter.calls == 0U);
    numerix_ode_solution_destroy(&solution);

    status = numerix_ode_rk4(constant_ode, &counter, 1e16, &initial, 1U,
                             4.0, 1U, &solution, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution.x[1] == 1e16 + 4.0);
    CHECK(fabs(solution.y[1] - 5.0) <= 4.0 * DBL_EPSILON);
    numerix_ode_solution_destroy(&solution);
}

static void test_overflow_resistant_linear_interpolation(void)
{
    numerix_result result;
    numerix_error error;
    numerix_status status;
    double subnormal = nextafter(0.0, 1.0);

    status = numerix_interp_linear(-1e308, -1e308, 1e308, 1e308,
                                   0.0, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == 0.0);

    status = numerix_interp_linear(2.0, 10.0, 4.0, 20.0,
                                   3.0, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == 15.0);
    CHECK(isnan(result.residual));
    status = numerix_interp_linear(0.0, 1.0, 1.0, 2.0,
                                   -1.0, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == 0.0);
    status = numerix_interp_linear(0.0, 1.0, 1.0, 2.0,
                                   2.0, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == 3.0);

    status = numerix_interp_linear(1e-300, 2e-300, 4e-300, 8e-300,
                                   2.5e-300, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(result.value / 5e-300 - 1.0) <= 8.0 * DBL_EPSILON);
    status = numerix_interp_linear(0.0, 0.0, 2.0 * subnormal,
                                   2.0 * subnormal, subnormal,
                                   &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == subnormal);

    status = numerix_interp_linear(-1e308, -1e308, 1e308, 1e308,
                                   nextafter(-1e308, INFINITY), &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(isfinite(result.value));
    CHECK(result.value >= -1e308 && result.value <= 1e308);

    {
        double x0 = 1e308;
        double x1 = nextafter(x0, INFINITY);

        status = numerix_interp_linear(x0, -5.0, x1, 7.0, x0,
                                       &result, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(result.value == -5.0);
        status = numerix_interp_linear(x0, -5.0, x1, 7.0, x1,
                                       &result, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(result.value == 7.0);
    }

    status = numerix_interp_linear(0.0, DBL_MAX, 1.0, -DBL_MAX,
                                   2.0, &result, &error);
    CHECK(status == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(!isfinite(result.value));

    {
        double nodes[] = {0.0, 1.0};
        double values[] = {1.0, 2.0};
        numerix_interpolant interpolant = {0};

        status = numerix_interp_lagrange(nodes, values, 2U, 0.5, 0,
                                         &result, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(isnan(result.residual));
        status = numerix_interp_newton_create(nodes, values, 2U,
                                              &interpolant, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        status = numerix_interp_newton_evaluate(&interpolant, 0.5, 0,
                                                &result, &error);
        CHECK(status == NUMERIX_STATUS_SUCCESS);
        CHECK(isnan(result.residual));
        numerix_interpolant_destroy(&interpolant);
    }
}

static void test_unavailable_residual_is_not_zero(void)
{
    numerix_result result;
    numerix_error error;
    numerix_status status;
    numerix_recommendation_request request;
    numerix_execution_output output;
    numerix_diagnostics_report report;
    double root = 1.0;
    double matrix_entries[] = {1.0};
    double right_entries[] = {1.0};
    numerix_matrix matrix = {1U, 1U, matrix_entries};
    numerix_vector right = {1U, right_entries};
    numerix_vector *solution = NULL;
    double residual = NAN;

    status = numerix_diff_central(identity_callback, NULL, 1.0, 0.1,
                                  NULL, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(isnan(result.residual));

    status = numerix_root_bisection(root_callback, &root, 0.0, 2.0,
                                    NULL, &result, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(result.value == 1.0);
    CHECK(result.residual == 0.0);

    status = numerix_linear_solve_gaussian(&matrix, &right, NULL,
                                           &solution, &residual, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(solution != NULL);
    CHECK(residual == 0.0);
    numerix_vector_destroy(solution);

    memset(&request, 0, sizeof(request));
    memset(&output, 0, sizeof(output));
    request.category = NUMERIX_PROBLEM_DIFFERENTIATION;
    output.category = NUMERIX_PROBLEM_DIFFERENTIATION;
    output.scalar_result.status = NUMERIX_STATUS_SUCCESS;
    output.scalar_result.value = 1.0;
    output.scalar_result.residual = NAN;
    output.scalar_result.estimated_error = NAN;
    status = numerix_diagnostics_analyze(&request, &output, NULL,
                                         &report, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(report.has_residual == 0);
    CHECK(isnan(report.absolute_residual));
    CHECK(report.has_estimated_error == 0);
    numerix_diagnostics_report_destroy(&report);
}

int main(void)
{
    test_bisection_budget_position_convergence();
    test_scale_aware_pivoting();
    test_lu_rejects_overflowed_factors();
    test_differentiation_rejects_collapsed_stencils();
    test_ode_rejects_stagnant_and_collapsed_stages();
    test_overflow_resistant_linear_interpolation();
    test_unavailable_residual_is_not_zero();
    if (failures != 0) {
        return 1;
    }
    puts("All Phase 13B regression checks passed.");
    return 0;
}
