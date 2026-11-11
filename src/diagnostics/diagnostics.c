#include "numerix.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMERIX_CONDITION_ESTIMATE_MAX_SIZE 256U

static numerix_status diagnostics_error(
    numerix_error *error,
    numerix_status status,
    const char *message)
{
    return numerix_error_set(error, status, message);
}

static void add_warning(
    numerix_diagnostics_report *report,
    const char *message)
{
    size_t index = report->warning_count;

    if (index >= sizeof(report->warning) / sizeof(report->warning[0]))
    {
        return;
    }

    (void)snprintf(report->warning[index], sizeof(report->warning[index]),
                   "%s", message);
    report->warning_count++;
}

static int valid_matrix(const numerix_matrix *matrix)
{
    size_t index;
    size_t count;

    if (matrix == NULL || matrix->data == NULL || matrix->rows == 0 ||
        matrix->columns == 0 || matrix->rows > SIZE_MAX / matrix->columns)
    {
        return 0;
    }

    count = matrix->rows * matrix->columns;
    for (index = 0; index < count; index++)
    {
        if (!isfinite(matrix->data[index]))
        {
            return 0;
        }
    }

    return 1;
}

static int valid_vector(const numerix_vector *vector)
{
    size_t index;

    if (vector == NULL || vector->data == NULL || vector->size == 0)
    {
        return 0;
    }

    for (index = 0; index < vector->size; index++)
    {
        if (!isfinite(vector->data[index]))
        {
            return 0;
        }
    }

    return 1;
}

static double vector_norm_infinity(const numerix_vector *vector)
{
    size_t index;
    double norm = 0.0;

    for (index = 0; index < vector->size; index++)
    {
        double magnitude = fabs(vector->data[index]);

        if (magnitude > norm)
        {
            norm = magnitude;
        }
    }

    return norm;
}

static double matrix_norm_infinity(const numerix_matrix *matrix)
{
    size_t row;
    size_t column;
    double norm = 0.0;

    for (row = 0; row < matrix->rows; row++)
    {
        long double row_sum = 0.0L;

        for (column = 0; column < matrix->columns; column++)
        {
            row_sum += fabsl((long double)matrix->data[
                row * matrix->columns + column]);
        }

        if (row_sum > (long double)norm)
        {
            norm = row_sum > (long double)DBL_MAX
                       ? INFINITY
                       : (double)row_sum;
        }
    }

    return norm;
}

static numerix_status analyze_linear_system(
    const numerix_recommendation_request *request,
    const numerix_execution_output *output,
    const numerix_diagnostics_options *options,
    numerix_diagnostics_report *report,
    numerix_error *error)
{
    const numerix_matrix *matrix = request->problem.linear.matrix;
    const numerix_vector *right_hand_side =
        request->problem.linear.right_hand_side;
    const numerix_vector *solution = output->vector_result;
    numerix_vector *residual = NULL;
    size_t row;
    size_t column;
    double norm_a;
    double norm_x;
    double norm_b;
    double norm_r;
    double scale;
    numerix_status status;

    if (solution == NULL && output->scalar_result.status !=
                                 NUMERIX_STATUS_SUCCESS)
    {
        add_warning(report,
                    "The linear solver did not return a solution vector, so residual and backward-error analysis are unavailable.");
        return NUMERIX_STATUS_SUCCESS;
    }

    if (!valid_matrix(matrix) || !valid_vector(right_hand_side) ||
        !valid_vector(solution) || matrix->rows != matrix->columns ||
        right_hand_side->size != matrix->rows ||
        solution->size != matrix->columns)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                                 "linear diagnostics require a valid square system and solution");
    }

    status = numerix_vector_create(matrix->rows, &residual, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    for (row = 0; row < matrix->rows; row++)
    {
        long double value = (long double)right_hand_side->data[row];

        for (column = 0; column < matrix->columns; column++)
        {
            value -= (long double)matrix->data[
                         row * matrix->columns + column] *
                     (long double)solution->data[column];
        }

        if (fabsl(value) > (long double)DBL_MAX)
        {
            numerix_vector_destroy(residual);
            return diagnostics_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                     "linear residual overflowed double precision");
        }

        residual->data[row] = (double)value;
    }

    norm_a = matrix_norm_infinity(matrix);
    norm_x = vector_norm_infinity(solution);
    norm_b = vector_norm_infinity(right_hand_side);
    norm_r = vector_norm_infinity(residual);
    report->residual_vector = residual;
    report->has_residual_vector = 1;
    report->has_residual = 1;
    report->absolute_residual = norm_r;
    report->has_relative_residual = norm_b > 0.0;
    if (report->has_relative_residual)
    {
        report->relative_residual = norm_r / norm_b;
        if (!isfinite(report->relative_residual))
        {
            report->has_relative_residual = 0;
        }
    }

    report->has_backward_error = 1;
    scale = fmax(norm_a, norm_b);
    if (scale == 0.0)
    {
        report->normwise_backward_error = norm_r == 0.0 ? 0.0 : INFINITY;
    }
    else
    {
        double scaled_denominator = (norm_a / scale) * norm_x +
                                    (norm_b / scale);

        report->normwise_backward_error =
            (norm_r / scale) / scaled_denominator;
    }

    if (options == NULL || options->estimate_linear_condition != 0)
    {
        numerix_lu factorization;
        double *inverse_row_sums;

        if (matrix->rows > NUMERIX_CONDITION_ESTIMATE_MAX_SIZE)
        {
            add_warning(report,
                        "Condition estimate skipped because the matrix exceeds the implementation size limit.");
        }
        else
        {
            status = numerix_lu_decompose(matrix, request->options,
                                          &factorization, error);
            if (status == NUMERIX_STATUS_SUCCESS)
            {
                inverse_row_sums = calloc(matrix->rows, sizeof(double));
                if (inverse_row_sums == NULL)
                {
                    numerix_lu_destroy(&factorization);
                    numerix_diagnostics_report_destroy(report);
                    return diagnostics_error(error,
                        NUMERIX_STATUS_ALLOCATION_FAILURE,
                        "condition estimate workspace allocation failed");
                }

                for (column = 0; column < matrix->columns; column++)
                {
                    numerix_vector basis;
                    numerix_vector *inverse_column = NULL;

                    basis.size = matrix->rows;
                    basis.data = calloc(matrix->rows, sizeof(double));
                    if (basis.data == NULL)
                    {
                        free(inverse_row_sums);
                        numerix_lu_destroy(&factorization);
                        numerix_diagnostics_report_destroy(report);
                        return diagnostics_error(error,
                            NUMERIX_STATUS_ALLOCATION_FAILURE,
                            "condition estimate vector allocation failed");
                    }

                    basis.data[column] = 1.0;
                    status = numerix_lu_solve(&factorization, &basis,
                                              &inverse_column, error);
                    free(basis.data);
                    if (status != NUMERIX_STATUS_SUCCESS)
                    {
                        free(inverse_row_sums);
                        numerix_lu_destroy(&factorization);
                        numerix_diagnostics_report_destroy(report);
                        return status;
                    }

                    for (row = 0; row < matrix->rows; row++)
                    {
                        inverse_row_sums[row] +=
                            fabs(inverse_column->data[row]);
                    }

                    numerix_vector_destroy(inverse_column);
                }

                {
                    double inverse_norm = 0.0;

                    for (row = 0; row < matrix->rows; row++)
                    {
                        if (inverse_row_sums[row] > inverse_norm)
                        {
                            inverse_norm = inverse_row_sums[row];
                        }
                    }

                    report->condition_estimate = norm_a * inverse_norm;
                    report->has_condition_estimate = 1;
                }

                free(inverse_row_sums);
                numerix_lu_destroy(&factorization);
            }
            else
            {
                add_warning(report,
                            "Condition estimate is unavailable because LU factorization reported a singular or numerically singular matrix.");
                numerix_error_clear(error);
            }
        }
    }

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status evaluate_scalar_problem(
    const numerix_recommendation_request *request,
    double x,
    double *value,
    numerix_error *error)
{
    if (request->representation == NUMERIX_REPRESENTATION_EXPRESSION)
    {
        if (request->expression == NULL)
        {
            return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                     "expression input is unavailable for residual evaluation");
        }

        return numerix_expression_evaluate(request->expression, x, value,
                                           error);
    }

    if (request->representation == NUMERIX_REPRESENTATION_PIECEWISE)
    {
        if (request->piecewise_function == NULL)
        {
            return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                     "piecewise input is unavailable for residual evaluation");
        }

        return numerix_piecewise_evaluate(request->piecewise_function, x,
                                          value, error);
    }

    if (request->function == NULL)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "callback input is unavailable for residual evaluation");
    }

    *value = request->function(x, request->user_data);
    if (!isfinite(*value))
    {
        return diagnostics_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                 "residual callback returned a non-finite value");
    }

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status analyze_interpolation(
    const numerix_recommendation_request *request,
    numerix_diagnostics_report *report,
    numerix_error *error)
{
    numerix_interpolant interpolant;
    size_t index;
    numerix_status status;
    double maximum = 0.0;

    memset(&interpolant, 0, sizeof(interpolant));
    if (request->problem.interpolation.x == NULL ||
        request->problem.interpolation.y == NULL ||
        request->problem.interpolation.count == 0)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "interpolation data is unavailable");
    }

    status = numerix_interp_newton_create(
        request->problem.interpolation.x, request->problem.interpolation.y,
        request->problem.interpolation.count, &interpolant, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    for (index = 0; index < request->problem.interpolation.count; index++)
    {
        numerix_result evaluated;
        double difference;

        status = numerix_interp_newton_evaluate(
            &interpolant, request->problem.interpolation.x[index], 1,
            &evaluated, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            numerix_interpolant_destroy(&interpolant);
            return status;
        }

        difference = fabs(evaluated.value -
                          request->problem.interpolation.y[index]);
        if (difference > maximum)
        {
            maximum = difference;
        }
    }

    numerix_interpolant_destroy(&interpolant);
    report->has_data_residual = 1;
    report->maximum_data_residual = maximum;
    report->has_residual = 1;
    report->absolute_residual = maximum;
    return NUMERIX_STATUS_SUCCESS;
}

static void classify_report(numerix_diagnostics_report *report)
{
    if (report->solver_status != NUMERIX_STATUS_SUCCESS)
    {
        report->reliability = NUMERIX_RELIABILITY_UNRELIABLE;
        add_warning(report,
                    "The numerical solver did not report success; the output requires independent review.");
        return;
    }

    if ((report->has_residual && !isfinite(report->absolute_residual)) ||
        (report->has_estimated_error &&
         !isfinite(report->estimated_absolute_error)) ||
        (report->has_backward_error &&
         !isfinite(report->normwise_backward_error)) ||
        (report->has_condition_estimate &&
         !isfinite(report->condition_estimate)))
    {
        report->reliability = NUMERIX_RELIABILITY_LOW_CONFIDENCE;
        add_warning(report,
                    "A required numerical diagnostic is non-finite.");
        return;
    }

    if ((report->has_residual && !report->residual_satisfies_tolerance) ||
        (report->has_estimated_error &&
         !report->estimated_error_satisfies_tolerance) ||
        (report->has_condition_estimate &&
         report->condition_estimate > 1.0 / sqrt(DBL_EPSILON)))
    {
        report->reliability = NUMERIX_RELIABILITY_CAUTION;
        add_warning(report,
                    "Available residual, error, or conditioning evidence warrants caution; this is not a proof of solution inaccuracy.");
        return;
    }

    if (report->has_residual || report->has_estimated_error ||
        report->has_backward_error || report->has_data_residual)
    {
        report->reliability = NUMERIX_RELIABILITY_ACCEPTABLE;
        return;
    }

    report->reliability = NUMERIX_RELIABILITY_INCONCLUSIVE;
    add_warning(report,
                "The current algorithm output does not expose enough evidence for a reliability classification.");
}

numerix_status numerix_diagnostics_analyze(
    const numerix_recommendation_request *request,
    const numerix_execution_output *output,
    const numerix_diagnostics_options *options,
    numerix_diagnostics_report *report,
    numerix_error *error)
{
    numerix_options default_options;
    const numerix_options *solver_options;
    numerix_status status = NUMERIX_STATUS_SUCCESS;

    if (request == NULL || output == NULL || report == NULL)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "diagnostic request, output, and report are required");
    }

    memset(report, 0, sizeof(*report));
    if (options != NULL && options->estimate_linear_condition != 0 &&
        options->estimate_linear_condition != 1)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "condition-estimation option must be zero or one");
    }
    if (request->category != output->category)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "request and execution output categories do not match");
    }

    report->category = request->category;
    report->solver_status = request->category == NUMERIX_PROBLEM_ODE &&
                                    output->has_ode_result
                                ? output->ode_result.status
                                : output->scalar_result.status;
    report->algorithm = output->algorithm;
    solver_options = request->options;
    if (solver_options == NULL)
    {
        numerix_options_init(&default_options);
        solver_options = &default_options;
    }
    else
    {
        status = numerix_options_validate(solver_options, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return status;
        }
    }

    switch (request->category)
    {
        case NUMERIX_PROBLEM_ROOT_FINDING:
        {
            double residual_value;

            report->additional_function_evaluations = 1U;
            status = evaluate_scalar_problem(
                request, output->scalar_result.value, &residual_value, error);
            if (status == NUMERIX_STATUS_SUCCESS)
            {
                report->has_residual = 1;
                report->absolute_residual = fabs(residual_value);
                report->residual_satisfies_tolerance =
                    report->absolute_residual <=
                    solver_options->absolute_tolerance +
                    solver_options->relative_tolerance *
                        fabs(output->scalar_result.value);
            }
            else
            {
                add_warning(report,
                            "Root residual could not be evaluated; callback or representation evaluation failed.");
                numerix_error_clear(error);
                status = NUMERIX_STATUS_SUCCESS;
            }
            break;
        }
        case NUMERIX_PROBLEM_LINEAR_ALGEBRA:
            status = analyze_linear_system(request, output, options,
                                           report, error);
            if (status == NUMERIX_STATUS_SUCCESS)
            {
                report->residual_satisfies_tolerance =
                    report->normwise_backward_error <=
                    solver_options->absolute_tolerance +
                    solver_options->relative_tolerance;
                if (report->has_condition_estimate &&
                    report->condition_estimate >
                        1.0 / sqrt(DBL_EPSILON))
                {
                    add_warning(report,
                                "The computed infinity-norm condition estimate is large; solution sensitivity may be high.");
                }
            }
            break;
        case NUMERIX_PROBLEM_DIFFERENTIATION:
            if (isfinite(output->scalar_result.estimated_error) &&
                output->scalar_result.estimated_error >= 0.0)
            {
                report->has_estimated_error = 1;
                report->estimated_absolute_error =
                    output->scalar_result.estimated_error;
                report->estimated_error_satisfies_tolerance =
                    output->scalar_result.estimated_error <=
                    solver_options->absolute_tolerance +
                    solver_options->relative_tolerance *
                        fabs(output->scalar_result.value);
            }
            break;
        case NUMERIX_PROBLEM_INTERPOLATION:
            status = analyze_interpolation(request, report, error);
            if (status == NUMERIX_STATUS_SUCCESS)
            {
                report->residual_satisfies_tolerance =
                    report->absolute_residual <=
                    solver_options->absolute_tolerance +
                    solver_options->relative_tolerance;
            }
            break;
        case NUMERIX_PROBLEM_INTEGRATION:
            if (isfinite(output->scalar_result.estimated_error) &&
                output->scalar_result.estimated_error >= 0.0)
            {
                report->has_estimated_error = 1;
                report->estimated_absolute_error =
                    output->scalar_result.estimated_error;
                report->has_relative_estimated_error =
                    fabs(output->scalar_result.value) > DBL_MIN;
                if (report->has_relative_estimated_error)
                {
                    report->relative_estimated_error =
                        report->estimated_absolute_error /
                        fabs(output->scalar_result.value);
                    if (!isfinite(report->relative_estimated_error))
                    {
                        report->has_relative_estimated_error = 0;
                    }
                }
                report->estimated_error_satisfies_tolerance =
                    report->estimated_absolute_error <=
                    solver_options->absolute_tolerance +
                    solver_options->relative_tolerance *
                        fabs(output->scalar_result.value);
            }
            else
            {
                add_warning(report,
                            "The selected integration method did not provide a finite error estimate.");
            }
            break;
        case NUMERIX_PROBLEM_OPTIMIZATION:
        case NUMERIX_PROBLEM_ODE:
            add_warning(report,
                        "The current execution API does not expose enough gradient or local-error history for a numerical reliability estimate.");
            break;
        default:
            return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                     "unknown problem category");
    }

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        numerix_diagnostics_report_destroy(report);
        return status;
    }

    classify_report(report);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void numerix_diagnostics_report_destroy(numerix_diagnostics_report *report)
{
    if (report == NULL)
    {
        return;
    }

    numerix_vector_destroy(report->residual_vector);
    memset(report, 0, sizeof(*report));
}

numerix_status numerix_diagnostics_scalar_sensitivity(
    numerix_function function,
    void *user_data,
    double x,
    double step,
    const numerix_options *options,
    numerix_sensitivity_report *report,
    numerix_error *error)
{
    numerix_result derivative;
    double function_value;
    numerix_status status;

    if (report == NULL || function == NULL)
    {
        return diagnostics_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "sensitivity report and callback are required");
    }

    memset(report, 0, sizeof(*report));
    status = numerix_diff_richardson(function, user_data, x, step, options,
                                     &derivative, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    function_value = function(x, user_data);
    if (!isfinite(function_value))
    {
        return diagnostics_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                 "sensitivity reference evaluation is non-finite");
    }

    report->evaluation_point = x;
    report->perturbation = step;
    report->derivative = derivative.value;
    report->estimated_uncertainty = derivative.estimated_error;
    report->function_evaluations = derivative.function_evaluations + 1U;
    report->method = "Richardson central difference";
    if (fabs(function_value) > DBL_MIN)
    {
        report->relative_sensitivity = x * derivative.value / function_value;
        report->has_relative_sensitivity =
            isfinite(report->relative_sensitivity);
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

const char *numerix_reliability_class_name(
    numerix_reliability_class reliability)
{
    switch (reliability)
    {
        case NUMERIX_RELIABILITY_INCONCLUSIVE:
            return "inconclusive";
        case NUMERIX_RELIABILITY_ACCEPTABLE:
            return "acceptable";
        case NUMERIX_RELIABILITY_CAUTION:
            return "caution";
        case NUMERIX_RELIABILITY_LOW_CONFIDENCE:
            return "low confidence";
        case NUMERIX_RELIABILITY_UNRELIABLE:
            return "unreliable";
        default:
            return "unknown";
    }
}
