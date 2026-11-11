#include "numerix.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct numerix_periodic_diagnostics {
    size_t capacity;
    size_t interval;
    size_t count;
    size_t total_observations;
    double *history;
};

static numerix_status analysis_error(
    numerix_error *error,
    numerix_status status,
    const char *message)
{
    return numerix_error_set(error, status, message);
}

static double stable_norm(const double *values, size_t count)
{
    size_t index;
    double scale = 0.0;
    double sum = 1.0;

    for (index = 0; index < count; index++)
    {
        double magnitude = fabs(values[index]);

        if (magnitude == 0.0)
        {
            continue;
        }
        if (scale < magnitude)
        {
            double ratio = scale / magnitude;
            sum = 1.0 + sum * ratio * ratio;
            scale = magnitude;
        }
        else
        {
            double ratio = magnitude / scale;
            sum += ratio * ratio;
        }
    }

    return scale == 0.0 ? 0.0 : scale * sqrt(sum);
}

static int valid_finite_vector(const numerix_vector *vector)
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

static int valid_finite_matrix(const numerix_matrix *matrix)
{
    size_t index;
    size_t element_count;

    if (matrix == NULL || matrix->data == NULL || matrix->rows == 0 ||
        matrix->columns == 0 || matrix->rows > SIZE_MAX / matrix->columns)
    {
        return 0;
    }

    element_count = matrix->rows * matrix->columns;
    for (index = 0; index < element_count; index++)
    {
        if (!isfinite(matrix->data[index]))
        {
            return 0;
        }
    }

    return 1;
}

numerix_status numerix_diagnostics_forward_error_scalar(
    double computed,
    double reference,
    numerix_forward_error *report,
    numerix_error *error)
{
    long double difference;

    if (report == NULL || !isfinite(computed) || !isfinite(reference))
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "finite computed and reference scalar values are required");
    }

    memset(report, 0, sizeof(*report));
    difference = fabsl((long double)computed - (long double)reference);
    if (difference > (long double)DBL_MAX)
    {
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "scalar forward error exceeds double precision range");
    }

    report->absolute_error = (double)difference;
    report->reference_description = "caller-supplied scalar reference";
    if (reference != 0.0)
    {
        report->relative_error = report->absolute_error / fabs(reference);
        report->has_relative_error = isfinite(report->relative_error);
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_diagnostics_forward_error_vector(
    const numerix_vector *computed,
    const numerix_vector *reference,
    numerix_forward_error *report,
    numerix_error *error)
{
    numerix_vector difference;
    double *difference_data;
    size_t index;
    double reference_norm;

    if (report == NULL || !valid_finite_vector(computed) ||
        !valid_finite_vector(reference) || computed->size != reference->size ||
        computed->size > SIZE_MAX / sizeof(double))
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                              "finite computed and reference vectors of equal dimension are required");
    }

    difference_data = malloc(computed->size * sizeof(double));
    if (difference_data == NULL)
    {
        return analysis_error(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                              "forward-error workspace allocation failed");
    }

    for (index = 0; index < computed->size; index++)
    {
        long double delta = (long double)computed->data[index] -
                            (long double)reference->data[index];

        if (fabsl(delta) > (long double)DBL_MAX)
        {
            free(difference_data);
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "vector forward error exceeds double precision range");
        }

        difference_data[index] = (double)delta;
    }

    difference.size = computed->size;
    difference.data = difference_data;
    memset(report, 0, sizeof(*report));
    report->absolute_error = stable_norm(difference.data, difference.size);
    reference_norm = stable_norm(reference->data, reference->size);
    if (!isfinite(report->absolute_error) || !isfinite(reference_norm))
    {
        free(difference_data);
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "vector forward-error norm overflowed");
    }
    report->reference_description = "caller-supplied vector reference";
    if (reference_norm > 0.0)
    {
        report->relative_error = report->absolute_error / reference_norm;
        report->has_relative_error = isfinite(report->relative_error);
    }

    free(difference_data);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_diagnostics_forward_error_matrix(
    const numerix_matrix *computed,
    const numerix_matrix *reference,
    numerix_forward_error *report,
    numerix_error *error)
{
    numerix_vector computed_view;
    numerix_vector reference_view;
    numerix_status status;

    if (report == NULL || !valid_finite_matrix(computed) ||
        !valid_finite_matrix(reference) || computed->rows != reference->rows ||
        computed->columns != reference->columns)
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                              "finite computed and reference matrices of equal dimensions are required");
    }

    computed_view.size = computed->rows * computed->columns;
    computed_view.data = computed->data;
    reference_view.size = reference->rows * reference->columns;
    reference_view.data = reference->data;
    status = numerix_diagnostics_forward_error_vector(
        &computed_view, &reference_view, report, error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        report->reference_description = "caller-supplied matrix reference";
    }

    return status;
}

numerix_status numerix_diagnostics_root_condition(
    numerix_function function,
    numerix_function derivative,
    void *user_data,
    double root_approximation,
    double step,
    const numerix_options *options,
    numerix_root_condition_report *report,
    numerix_error *error)
{
    numerix_result derivative_result;
    numerix_status status;
    double derivative_value;
    double uncertainty;

    if (function == NULL || report == NULL || !isfinite(root_approximation))
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "callback, finite root approximation, and report are required");
    }

    memset(report, 0, sizeof(*report));
    if (derivative != NULL)
    {
        derivative_value = derivative(root_approximation, user_data);
        if (!isfinite(derivative_value))
        {
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "derivative callback returned a non-finite value");
        }

        uncertainty = 0.0;
        report->derivative_evaluations = 1U;
        report->derivative_was_supplied = 1;
    }
    else
    {
        status = numerix_diff_richardson(
            function, user_data, root_approximation, step, options,
            &derivative_result, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return status;
        }

        derivative_value = derivative_result.value;
        uncertainty = derivative_result.estimated_error;
        report->function_evaluations = derivative_result.function_evaluations;
    }

    report->evaluation_point = root_approximation;
    report->derivative = derivative_value;
    report->derivative_uncertainty = uncertainty;
    report->perturbation_step = derivative == NULL ? step : 0.0;
    if (fabs(derivative_value) > fmax(uncertainty, DBL_MIN))
    {
        report->reciprocal_derivative_condition = 1.0 / fabs(derivative_value);
        report->condition_available =
            isfinite(report->reciprocal_derivative_condition);
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void numerix_multivariable_sensitivity_destroy(
    numerix_multivariable_sensitivity *report)
{
    if (report == NULL)
    {
        return;
    }

    numerix_vector_destroy(report->gradient);
    numerix_vector_destroy(report->relative_sensitivities);
    memset(report, 0, sizeof(*report));
}

numerix_status numerix_diagnostics_multivariable_sensitivity(
    numerix_objective_function objective,
    void *user_data,
    const numerix_vector *point,
    double relative_step,
    numerix_multivariable_sensitivity *report,
    numerix_error *error)
{
    double *trial;
    double uncertainty_norm = 0.0;
    double nominal_value;
    size_t index;
    numerix_status status;

    if (objective == NULL || report == NULL || !valid_finite_vector(point) ||
        !isfinite(relative_step) || relative_step <= 0.0 ||
        relative_step >= 1.0 || point->size > SIZE_MAX / sizeof(double))
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "objective, finite point, positive relative step below one, and report are required");
    }

    memset(report, 0, sizeof(*report));
    status = numerix_vector_create(point->size, &report->gradient, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    nominal_value = objective(point->data, point->size, user_data);
    report->function_evaluations++;
    if (!isfinite(nominal_value))
    {
        numerix_multivariable_sensitivity_destroy(report);
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "objective returned a non-finite value at the sensitivity point");
    }
    report->nominal_value = nominal_value;
    if (fabs(nominal_value) > DBL_MIN)
    {
        status = numerix_vector_create(
            point->size, &report->relative_sensitivities, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            numerix_multivariable_sensitivity_destroy(report);
            return status;
        }
        report->has_relative_sensitivities = 1;
    }

    trial = malloc(point->size * sizeof(double));
    if (trial == NULL)
    {
        numerix_multivariable_sensitivity_destroy(report);
        return analysis_error(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                              "sensitivity workspace allocation failed");
    }
    memcpy(trial, point->data, point->size * sizeof(double));

    for (index = 0; index < point->size; index++)
    {
        double step = relative_step * fmax(1.0, fabs(point->data[index]));
        double coarse_plus;
        double coarse_minus;
        double fine_plus;
        double fine_minus;
        double coarse;
        double fine;
        double estimate;

        trial[index] = point->data[index] + step;
        if (trial[index] == point->data[index] || !isfinite(trial[index]))
        {
            free(trial);
            numerix_multivariable_sensitivity_destroy(report);
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "sensitivity step is not representable at an input coordinate");
        }
        coarse_plus = objective(trial, point->size, user_data);
        report->function_evaluations++;

        trial[index] = point->data[index] - step;
        coarse_minus = objective(trial, point->size, user_data);
        report->function_evaluations++;

        trial[index] = point->data[index] + step * 0.5;
        fine_plus = objective(trial, point->size, user_data);
        report->function_evaluations++;

        trial[index] = point->data[index] - step * 0.5;
        fine_minus = objective(trial, point->size, user_data);
        report->function_evaluations++;
        trial[index] = point->data[index];

        if (!isfinite(coarse_plus) || !isfinite(coarse_minus) ||
            !isfinite(fine_plus) || !isfinite(fine_minus))
        {
            free(trial);
            numerix_multivariable_sensitivity_destroy(report);
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "objective returned a non-finite value during sensitivity estimation");
        }

        coarse = (coarse_plus - coarse_minus) / (2.0 * step);
        fine = (fine_plus - fine_minus) / step;
        estimate = (4.0 * fine - coarse) / 3.0;
        if (!isfinite(estimate))
        {
            free(trial);
            numerix_multivariable_sensitivity_destroy(report);
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "sensitivity estimate overflowed");
        }

        report->gradient->data[index] = estimate;
        if (report->has_relative_sensitivities)
        {
            double relative = point->data[index] * estimate / nominal_value;

            if (isfinite(relative))
            {
                report->relative_sensitivities->data[index] = relative;
            }
            else
            {
                report->has_relative_sensitivities = 0;
            }
        }
        uncertainty_norm = hypot(uncertainty_norm, fabs(estimate - fine));
        trial[index] = point->data[index];
    }

    free(trial);
    report->input_dimension = point->size;
    report->relative_step = relative_step;
    report->estimated_uncertainty_norm = uncertainty_norm;
    if (!isfinite(report->estimated_uncertainty_norm))
    {
        numerix_multivariable_sensitivity_destroy(report);
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "sensitivity uncertainty estimate overflowed");
    }
    report->uncertainty_is_estimated = 1;
    if (!report->has_relative_sensitivities)
    {
        numerix_vector_destroy(report->relative_sensitivities);
        report->relative_sensitivities = NULL;
    }
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_diagnostics_propagate_independent_uncertainty(
    numerix_objective_function objective,
    void *user_data,
    const numerix_vector *point,
    const numerix_vector *input_uncertainties,
    double relative_step,
    numerix_error_propagation_report *report,
    numerix_error *error)
{
    numerix_multivariable_sensitivity sensitivity;
    double *contributions;
    size_t index;
    numerix_status status;

    if (report == NULL || objective == NULL || !valid_finite_vector(point) ||
        !valid_finite_vector(input_uncertainties) ||
        input_uncertainties->size != point->size)
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                              "point and uncertainty vectors must be valid and have equal dimensions");
    }

    for (index = 0; index < input_uncertainties->size; index++)
    {
        if (input_uncertainties->data[index] < 0.0)
        {
            return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                  "input standard uncertainties must be nonnegative");
        }
    }

    status = numerix_diagnostics_multivariable_sensitivity(
        objective, user_data, point, relative_step, &sensitivity, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    contributions = malloc(point->size * sizeof(double));
    if (contributions == NULL)
    {
        numerix_multivariable_sensitivity_destroy(&sensitivity);
        return analysis_error(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                              "uncertainty propagation workspace allocation failed");
    }

    for (index = 0; index < point->size; index++)
    {
        contributions[index] = sensitivity.gradient->data[index] *
                               input_uncertainties->data[index];
        if (!isfinite(contributions[index]))
        {
            free(contributions);
            numerix_multivariable_sensitivity_destroy(&sensitivity);
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "propagated uncertainty overflowed");
        }
    }

    memset(report, 0, sizeof(*report));
    report->output_value = sensitivity.nominal_value;
    report->estimated_standard_uncertainty =
        stable_norm(contributions, point->size);
    if (!isfinite(report->estimated_standard_uncertainty))
    {
        free(contributions);
        numerix_multivariable_sensitivity_destroy(&sensitivity);
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "propagated output uncertainty overflowed");
    }
    report->has_relative_uncertainty =
        fabs(report->output_value) > DBL_MIN;
    if (report->has_relative_uncertainty)
    {
        report->relative_uncertainty =
            report->estimated_standard_uncertainty /
            fabs(report->output_value);
        if (!isfinite(report->relative_uncertainty))
        {
            report->has_relative_uncertainty = 0;
        }
    }
    report->function_evaluations = sensitivity.function_evaluations;
    report->assumes_independent_inputs = 1;
    report->uses_local_linearization = 1;

    free(contributions);
    numerix_multivariable_sensitivity_destroy(&sensitivity);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_diagnostics_analyze_convergence(
    const double *residual_history,
    size_t observation_count,
    numerix_convergence_report *report,
    numerix_error *error)
{
    size_t index;
    size_t increasing = 0;
    size_t decreasing = 0;
    size_t stagnant = 0;
    size_t sign_changes = 0;
    double first_magnitude;
    double previous_magnitude;
    double final_magnitude;

    if (residual_history == NULL || report == NULL || observation_count == 0)
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "nonempty residual history and report are required");
    }

    memset(report, 0, sizeof(*report));
    report->observations = observation_count;
    for (index = 0; index < observation_count; index++)
    {
        if (!isfinite(residual_history[index]))
        {
            return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "residual history contains a non-finite value");
        }
        if (index > 0 && signbit(residual_history[index]) !=
                             signbit(residual_history[index - 1]))
        {
            sign_changes++;
        }
    }

    if (observation_count < 3)
    {
        report->trend = NUMERIX_CONVERGENCE_INCONCLUSIVE;
        numerix_error_clear(error);
        return NUMERIX_STATUS_SUCCESS;
    }

    first_magnitude = fabs(residual_history[0]);
    previous_magnitude = first_magnitude;
    for (index = 1; index < observation_count; index++)
    {
        double magnitude = fabs(residual_history[index]);

        if (magnitude > previous_magnitude * (1.0 + 8.0 * DBL_EPSILON))
        {
            increasing++;
        }
        else if (magnitude < previous_magnitude * (1.0 - 8.0 * DBL_EPSILON))
        {
            decreasing++;
        }
        else
        {
            stagnant++;
        }
        previous_magnitude = magnitude;
    }

    final_magnitude = fabs(residual_history[observation_count - 1]);
    if (first_magnitude > 0.0)
    {
        report->final_residual_ratio = final_magnitude / first_magnitude;
        report->has_residual_ratio = isfinite(report->final_residual_ratio);
    }

    if (final_magnitude == 0.0)
    {
        report->trend = NUMERIX_CONVERGENCE_CONVERGING;
    }
    else if (increasing >= 2 && increasing > decreasing)
    {
        report->trend = sign_changes >= 2
                            ? NUMERIX_CONVERGENCE_OSCILLATORY
                            : NUMERIX_CONVERGENCE_DIVERGING;
    }
    else if (sign_changes >= 2 && decreasing >= 2 &&
             final_magnitude < first_magnitude)
    {
        report->trend = NUMERIX_CONVERGENCE_OSCILLATORY;
    }
    else if (decreasing > increasing && final_magnitude < first_magnitude)
    {
        report->trend = NUMERIX_CONVERGENCE_CONVERGING;
        if (decreasing + increasing > 0 &&
            (double)decreasing / (double)(decreasing + increasing) >= 0.75)
        {
            double average_ratio = pow(
                final_magnitude / first_magnitude,
                1.0 / (double)(observation_count - 1U));
            if (average_ratio > 0.8)
            {
                report->trend = NUMERIX_CONVERGENCE_SLOW;
            }
        }
    }
    else if (stagnant >= 2)
    {
        report->trend = NUMERIX_CONVERGENCE_STAGNATING;
    }
    else
    {
        report->trend = NUMERIX_CONVERGENCE_INCONCLUSIVE;
    }

    if (observation_count >= 3)
    {
        double older = fabs(residual_history[observation_count - 3]);
        double previous = fabs(residual_history[observation_count - 2]);
        double current = fabs(residual_history[observation_count - 1]);

        if (older > 0.0 && previous > 0.0 && current > 0.0 &&
            previous != older && current != previous)
        {
            double numerator = log(current / previous);
            double denominator = log(previous / older);
            double order = numerator / denominator;

            if (isfinite(order) && order > 0.0)
            {
                report->empirical_residual_order = order;
                report->has_empirical_order = 1;
                report->rate_uses_residuals_as_error_proxies = 1;
            }
        }
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

const char *numerix_convergence_trend_name(numerix_convergence_trend trend)
{
    switch (trend)
    {
        case NUMERIX_CONVERGENCE_INCONCLUSIVE:
            return "inconclusive";
        case NUMERIX_CONVERGENCE_CONVERGING:
            return "converging";
        case NUMERIX_CONVERGENCE_SLOW:
            return "slow convergence";
        case NUMERIX_CONVERGENCE_OSCILLATORY:
            return "oscillatory";
        case NUMERIX_CONVERGENCE_STAGNATING:
            return "stagnating";
        case NUMERIX_CONVERGENCE_DIVERGING:
            return "diverging";
        default:
            return "unknown";
    }
}

numerix_status numerix_diagnostics_check_subtraction(
    double a,
    double b,
    numerix_cancellation_report *report,
    numerix_error *error)
{
    long double difference;
    double operand_scale;
    double scaled_sum;
    double relative_remainder;

    if (report == NULL || !isfinite(a) || !isfinite(b))
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "finite subtraction operands and report are required");
    }

    memset(report, 0, sizeof(*report));
    difference = (long double)a - (long double)b;
    if (fabsl(difference) > (long double)DBL_MAX)
    {
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "subtraction result exceeds double precision range");
    }

    report->difference = (double)difference;
    operand_scale = fmax(fabs(a), fabs(b));
    if (operand_scale > 0.0)
    {
        scaled_sum = fabs(a) / operand_scale + fabs(b) / operand_scale;
        relative_remainder = (double)(fabsl(difference) /
                                      (long double)operand_scale) / scaled_sum;
        report->cancellation_suspected =
            relative_remainder <= sqrt(DBL_EPSILON);
        if (difference == 0.0L)
        {
            report->cancellation_condition_indicator = INFINITY;
            report->estimated_lost_decimal_digits = INFINITY;
        }
        else
        {
            report->cancellation_condition_indicator =
                scaled_sum / relative_remainder;
            report->estimated_lost_decimal_digits =
                log10(scaled_sum) - log10(relative_remainder);
        }
        report->has_condition_indicator = 1;
        report->has_lost_decimal_digits_estimate = 1;
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_periodic_diagnostics_create(
    size_t history_capacity,
    size_t every_n_observations,
    numerix_periodic_diagnostics **observer,
    numerix_error *error)
{
    numerix_periodic_diagnostics *created;

    if (observer == NULL || history_capacity == 0 ||
        every_n_observations == 0 ||
        history_capacity > SIZE_MAX / sizeof(double))
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "positive history capacity, observation interval, and output are required");
    }

    *observer = NULL;
    created = calloc(1, sizeof(*created));
    if (created == NULL)
    {
        return analysis_error(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                              "periodic diagnostics allocation failed");
    }

    created->history = malloc(history_capacity * sizeof(double));
    if (created->history == NULL)
    {
        free(created);
        return analysis_error(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                              "periodic history allocation failed");
    }

    created->capacity = history_capacity;
    created->interval = every_n_observations;
    *observer = created;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_periodic_diagnostics_observe(
    numerix_periodic_diagnostics *observer,
    const numerix_execution_step *step,
    numerix_convergence_report *report,
    int *assessment_updated,
    numerix_error *error)
{
    if (observer == NULL || step == NULL || report == NULL ||
        assessment_updated == NULL)
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "observer, step, report, and update flag are required");
    }

    *assessment_updated = 0;
    if ((step->valid_fields & NUMERIX_STEP_HAS_RESIDUAL) == 0U)
    {
        numerix_error_clear(error);
        return NUMERIX_STATUS_SUCCESS;
    }
    if (!isfinite(step->residual))
    {
        return analysis_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "execution step contains a non-finite residual");
    }

    if (observer->count < observer->capacity)
    {
        observer->history[observer->count++] = step->residual;
    }
    else
    {
        memmove(observer->history, observer->history + 1,
                (observer->capacity - 1U) * sizeof(double));
        observer->history[observer->capacity - 1U] = step->residual;
    }
    observer->total_observations++;

    if (observer->total_observations % observer->interval == 0U)
    {
        numerix_status status = numerix_diagnostics_analyze_convergence(
            observer->history, observer->count, report, error);

        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return status;
        }
        *assessment_updated = 1;
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_periodic_diagnostics_finish(
    numerix_periodic_diagnostics *observer,
    numerix_convergence_report *report,
    numerix_error *error)
{
    if (observer == NULL || report == NULL)
    {
        return analysis_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                              "observer and convergence report are required");
    }
    if (observer->count == 0)
    {
        memset(report, 0, sizeof(*report));
        report->trend = NUMERIX_CONVERGENCE_INCONCLUSIVE;
        numerix_error_clear(error);
        return NUMERIX_STATUS_SUCCESS;
    }

    return numerix_diagnostics_analyze_convergence(
        observer->history, observer->count, report, error);
}

void numerix_periodic_diagnostics_destroy(
    numerix_periodic_diagnostics *observer)
{
    if (observer == NULL)
    {
        return;
    }

    free(observer->history);
    free(observer);
}
