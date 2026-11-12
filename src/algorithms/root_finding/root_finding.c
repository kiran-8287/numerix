#include "numerix_internal.h"

#include <float.h>
#include <math.h>
#include <string.h>

static numerix_status initialize_root(
    const numerix_options *input, numerix_options *options,
    numerix_result *result, numerix_error *error, const char *algorithm)
{
    if (result == NULL)
    {
        numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT, "result required");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    memset(result, 0, sizeof(*result));
    result->value = NAN;
    result->residual = NAN;
    result->estimated_error = NAN;
    result->algorithm = algorithm;
    if (input != NULL)
    {
        *options = *input;
    }
    else
    {
        numerix_options_init(options);
    }
    result->status = numerix_options_validate(options, error);
    return result->status;
}

static double position_tolerance(const numerix_options *options, double position)
{
    long double tolerance = (long double)options->absolute_tolerance +
        (long double)options->relative_tolerance * fabs(position);

    return tolerance > DBL_MAX ? DBL_MAX : (double)tolerance;
}

static double interval_midpoint(double lower, double upper)
{
    double width = upper - lower;

    if (isfinite(width))
    {
        return lower + width * 0.5;
    }
    return lower * 0.5 + upper * 0.5;
}

static double position_bound(double position, double lower, double upper)
{
    return fmax(position - lower, upper - position);
}

static int brackets_zero(double left, double right)
{
    return (left < 0.0) != (right < 0.0);
}

static numerix_status evaluate_root(
    numerix_function function, void *data, double position, double *value,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    if (result->function_evaluations >= options->maximum_function_evaluations)
    {
        numerix_error_set(error, NUMERIX_STATUS_LIMIT_REACHED, "evaluation limit reached");
        return NUMERIX_STATUS_LIMIT_REACHED;
    }
    if (!isfinite(position))
    {
        numerix_error_set(error, NUMERIX_STATUS_NUMERICAL_FAILURE, "non-finite iterate");
        return NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    *value = function(position, data);
    ++result->function_evaluations;
    if (!isfinite(*value))
    {
        numerix_error_set(error, NUMERIX_STATUS_NUMERICAL_FAILURE, "non-finite callback");
        return NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status finish_root(
    numerix_result *result, numerix_status status, double position,
    double residual, double estimate, numerix_error *error)
{
    result->status = status;
    result->value = position;
    result->residual = residual;
    result->estimated_error = estimate;
    numerix_error_set(error, status, status == NUMERIX_STATUS_SUCCESS ? "" :
                      numerix_status_string(status));
    return status;
}

/* Same bisection/chord/IQI methods as before; interpolation remains safeguarded. */
static numerix_status solve_bracket(
    numerix_function function, void *data, double lower, double upper,
    const numerix_options *input, numerix_result *result, numerix_error *error,
    int method)
{
    numerix_options options;
    numerix_status status;
    double lower_value = NAN;
    double upper_value = NAN;
    double previous_position;
    double previous_value;
    size_t iteration;
    const char *name = method == 0 ? "bisection" :
                       method == 1 ? "regula_falsi" : "brent_dekker";

    status = initialize_root(input, &options, result, error, name);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    if (function == NULL || !isfinite(lower) || !isfinite(upper) || !(lower < upper))
    {
        return finish_root(result, NUMERIX_STATUS_INVALID_ARGUMENT, NAN, NAN, NAN, error);
    }
    status = evaluate_root(function, data, lower, &lower_value, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return finish_root(result, status, lower, lower_value, NAN, error);
    }
    status = evaluate_root(function, data, upper, &upper_value, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return finish_root(result, status, upper, upper_value, NAN, error);
    }
    if (lower_value == 0.0 || upper_value == 0.0)
    {
        return finish_root(result, NUMERIX_STATUS_SUCCESS,
            lower_value == 0.0 ? lower : upper, 0.0, 0.0, error);
    }
    if (!brackets_zero(lower_value, upper_value))
    {
        return finish_root(result, NUMERIX_STATUS_INVALID_INPUT, lower, lower_value, NAN, error);
    }
    previous_position = lower;
    previous_value = lower_value;
    for (iteration = 0; iteration < options.maximum_iterations; ++iteration)
    {
        double midpoint = interval_midpoint(lower, upper);
        double candidate = midpoint;
        double candidate_value = NAN;
        double bound = position_bound(midpoint, lower, upper);
        int interval_converged = bound <= position_tolerance(&options, midpoint);

        if (method == 0 && interval_converged)
        {
            double midpoint_residual = midpoint == lower ? lower_value :
                midpoint == upper ? upper_value : NAN;

            /* The bracket already certifies the requested position accuracy. */
            result->iterations = iteration;
            return finish_root(result, NUMERIX_STATUS_SUCCESS, midpoint,
                               midpoint_residual, bound, error);
        }
        if (method != 0)
        {
            double best_position = fabs(lower_value) <= fabs(upper_value) ? lower : upper;
            double best_value = fabs(lower_value) <= fabs(upper_value) ? lower_value : upper_value;
            double enclosure = upper - lower;

            if (enclosure <= position_tolerance(&options, best_position))
            {
                return finish_root(result, NUMERIX_STATUS_SUCCESS,
                                   best_position, best_value, enclosure, error);
            }
            interval_converged = 0;
        }
        if (method != 0 && !interval_converged)
        {
            double scale = fmax(fabs(lower_value), fmax(fabs(upper_value), fabs(previous_value)));
            double normalized_lower = lower_value / scale;
            double normalized_upper = upper_value / scale;
            double normalized_previous = previous_value / scale;
            double interpolation = NAN;
            double width = upper - lower;

            if (method == 2 && lower_value != upper_value &&
                lower_value != previous_value && upper_value != previous_value)
            {
                double lower_weight = normalized_upper * normalized_previous /
                    ((normalized_lower - normalized_upper) * (normalized_lower - normalized_previous));
                double upper_weight = normalized_lower * normalized_previous /
                    ((normalized_upper - normalized_lower) * (normalized_upper - normalized_previous));
                double previous_weight = normalized_lower * normalized_upper /
                    ((normalized_previous - normalized_lower) * (normalized_previous - normalized_upper));

                interpolation = lower * lower_weight + upper * upper_weight +
                    previous_position * previous_weight;
            }
            else
            {
                double fraction = normalized_lower / (normalized_lower - normalized_upper);

                interpolation = (1.0 - fraction) * lower + fraction * upper;
            }
            double guard_fraction = method == 1 ? 0.05 : 0.1;

            if (isfinite(width) && isfinite(interpolation) &&
                interpolation > lower + guard_fraction * width &&
                interpolation < upper - guard_fraction * width)
            {
                candidate = interpolation;
            }
        }
        if (!interval_converged && (candidate == lower || candidate == upper))
        {
            return finish_root(result, NUMERIX_STATUS_CONVERGENCE_FAILURE,
                               lower, lower_value, bound, error);
        }
        status = evaluate_root(function, data, candidate, &candidate_value, &options, result, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return finish_root(result, status, candidate, candidate_value, bound, error);
        }
        result->iterations = iteration + 1U;
        if (candidate_value == 0.0 || interval_converged)
        {
            return finish_root(result, NUMERIX_STATUS_SUCCESS, candidate,
                               candidate_value, candidate_value == 0.0 ? 0.0 : bound, error);
        }
        if (brackets_zero(lower_value, candidate_value))
        {
            previous_position = upper;
            previous_value = upper_value;
            upper = candidate;
            upper_value = candidate_value;
        }
        else
        {
            previous_position = lower;
            previous_value = lower_value;
            lower = candidate;
            lower_value = candidate_value;
        }
    }
    return finish_root(result, NUMERIX_STATUS_LIMIT_REACHED, lower, lower_value,
                       upper - lower, error);
}

numerix_status numerix_root_bisection(
    numerix_function function, void *data, double lower, double upper,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return solve_bracket(function, data, lower, upper, options, result, error, 0);
}

numerix_status numerix_root_regula_falsi(
    numerix_function function, void *data, double lower, double upper,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return solve_bracket(function, data, lower, upper, options, result, error, 1);
}

numerix_status numerix_root_brent(
    numerix_function function, void *data, double lower, double upper,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return solve_bracket(function, data, lower, upper, options, result, error, 2);
}

/* An observed contraction estimate, not a certified forward-error bound. */
static double step_estimate(double step, double previous_step)
{
    double ratio;

    if (!isfinite(previous_step) || previous_step == 0.0)
    {
        return INFINITY;
    }
    ratio = step / previous_step;
    if (ratio >= 1.0)
    {
        return INFINITY;
    }
    return fmax(step, step * ratio / (1.0 - ratio));
}

numerix_status numerix_root_secant(
    numerix_function function, void *data, double initial, double second,
    const numerix_options *input, numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    double initial_value = NAN;
    double second_value = NAN;
    double previous_step = INFINITY;
    size_t iteration;

    status = initialize_root(input, &options, result, error, "secant");
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    if (function == NULL || !isfinite(initial) || !isfinite(second) || initial == second)
    {
        return finish_root(result, NUMERIX_STATUS_INVALID_ARGUMENT, second, NAN, NAN, error);
    }
    status = evaluate_root(function, data, initial, &initial_value, &options, result, error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = evaluate_root(function, data, second, &second_value, &options, result, error);
    }
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return finish_root(result, status, second, second_value, NAN, error);
    }
    for (iteration = 0; iteration < options.maximum_iterations; ++iteration)
    {
        double scale = fmax(fabs(initial_value), fabs(second_value));
        double denominator;
        double next;
        double next_value = NAN;
        double step;
        double estimate;

        if (initial_value == 0.0 || second_value == 0.0)
        {
            return finish_root(result, NUMERIX_STATUS_SUCCESS,
                initial_value == 0.0 ? initial : second, 0.0, 0.0, error);
        }
        denominator = second_value / scale - initial_value / scale;
        if (fabs(denominator) <= DBL_EPSILON)
        {
            return finish_root(result, NUMERIX_STATUS_NUMERICAL_FAILURE,
                               second, second_value, NAN, error);
        }
        next = (double)((long double)second - (second_value / scale) /
                        (long double)denominator * ((long double)second - initial));
        if (!isfinite(next) || next == second)
        {
            return finish_root(result, NUMERIX_STATUS_CONVERGENCE_FAILURE,
                               second, second_value, NAN, error);
        }
        status = evaluate_root(function, data, next, &next_value, &options, result, error);
        step = fabs(next - second);
        estimate = step_estimate(step, previous_step);
        result->iterations = iteration + 1U;
        if (status != NUMERIX_STATUS_SUCCESS || next_value == 0.0 ||
            (fabs(next_value) < fabs(second_value) &&
             estimate <= position_tolerance(&options, next)))
        {
            return finish_root(result, status, next, next_value,
                               next_value == 0.0 ? 0.0 : estimate, error);
        }
        previous_step = step;
        initial = second;
        initial_value = second_value;
        second = next;
        second_value = next_value;
    }
    return finish_root(result, NUMERIX_STATUS_LIMIT_REACHED, second, second_value,
                       previous_step, error);
}

static numerix_status solve_newton(
    numerix_function function, numerix_function derivative, void *data, double position,
    int safeguarded, double lower, double upper, const numerix_options *input,
    numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    double value = NAN;
    double lower_value = NAN;
    double upper_value = NAN;
    double previous_step = INFINITY;
    size_t iteration;

    status = initialize_root(input, &options, result, error,
        safeguarded ? "safeguarded_newton" : "newton_raphson");
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    if (function == NULL || derivative == NULL || !isfinite(position) ||
        (safeguarded && (!isfinite(lower) || !isfinite(upper) || !(lower < upper))))
    {
        return finish_root(result, NUMERIX_STATUS_INVALID_ARGUMENT, position, NAN, NAN, error);
    }
    status = evaluate_root(function, data, position, &value, &options, result, error);
    if (status == NUMERIX_STATUS_SUCCESS && safeguarded)
    {
        status = evaluate_root(function, data, lower, &lower_value, &options, result, error);
        if (status == NUMERIX_STATUS_SUCCESS)
        {
            status = evaluate_root(function, data, upper, &upper_value, &options, result, error);
        }
        if (status == NUMERIX_STATUS_SUCCESS && lower_value != 0.0 && upper_value != 0.0 &&
            !brackets_zero(lower_value, upper_value))
        {
            status = NUMERIX_STATUS_INVALID_INPUT;
        }
    }
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return finish_root(result, status, position, value, NAN, error);
    }
    if (safeguarded && (lower_value == 0.0 || upper_value == 0.0))
    {
        return finish_root(result, NUMERIX_STATUS_SUCCESS,
            lower_value == 0.0 ? lower : upper, 0.0, 0.0, error);
    }
    if (safeguarded && value != 0.0)
    {
        if (brackets_zero(lower_value, value))
        {
            upper = position;
            upper_value = value;
        }
        else
        {
            lower = position;
            lower_value = value;
        }
    }
    for (iteration = 0; iteration < options.maximum_iterations; ++iteration)
    {
        double slope = NAN;
        double next;
        double next_value = NAN;
        double step;
        double estimate;

        if (value == 0.0)
        {
            return finish_root(result, NUMERIX_STATUS_SUCCESS, position, value, 0.0, error);
        }
        if (safeguarded)
        {
            double best_position = fabs(lower_value) <= fabs(upper_value) ? lower : upper;
            double best_value = fabs(lower_value) <= fabs(upper_value) ? lower_value : upper_value;

            if (upper - lower <= position_tolerance(&options, best_position))
            {
                return finish_root(result, NUMERIX_STATUS_SUCCESS,
                                   best_position, best_value, upper - lower, error);
            }
        }
        status = evaluate_root(derivative, data, position, &slope, &options, result, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return finish_root(result, status, position, value, NAN, error);
        }
        if (slope == 0.0 && !safeguarded)
        {
            return finish_root(result, NUMERIX_STATUS_NUMERICAL_FAILURE,
                               position, value, NAN, error);
        }
        next = slope == 0.0 ? NAN : position - value / slope;
        if (safeguarded && (!isfinite(next) || next <= lower || next >= upper || next == position))
        {
            next = interval_midpoint(lower, upper);
        }
        if (!isfinite(next) || next == position)
        {
            return finish_root(result, NUMERIX_STATUS_CONVERGENCE_FAILURE,
                               position, value, NAN, error);
        }
        status = evaluate_root(function, data, next, &next_value, &options, result, error);
        step = fabs(next - position);
        estimate = step_estimate(step, previous_step);
        result->iterations = iteration + 1U;
        if (status != NUMERIX_STATUS_SUCCESS || next_value == 0.0 ||
            (!safeguarded && fabs(next_value) < fabs(value) &&
             estimate <= position_tolerance(&options, next)))
        {
            return finish_root(result, status, next, next_value,
                               next_value == 0.0 ? 0.0 : estimate, error);
        }
        if (safeguarded)
        {
            if (brackets_zero(lower_value, next_value))
            {
                upper = next;
                upper_value = next_value;
            }
            else
            {
                lower = next;
                lower_value = next_value;
            }
        }
        previous_step = step;
        position = next;
        value = next_value;
    }
    return finish_root(result, NUMERIX_STATUS_LIMIT_REACHED, position, value, NAN, error);
}

numerix_status numerix_root_newton(
    numerix_function function, numerix_function derivative, void *data, double initial,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return solve_newton(function, derivative, data, initial, 0, 0.0, 0.0, options, result, error);
}

numerix_status numerix_root_safeguarded_newton(
    numerix_function function, numerix_function derivative, void *data, double lower,
    double upper, const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return solve_newton(function, derivative, data, interval_midpoint(lower, upper),
                        1, lower, upper, options, result, error);
}
