#include "numerix_internal.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static numerix_status integration_error(numerix_error *error, numerix_status status,
                                        const char *message)
{
    numerix_error_set(error, status, message);
    return status;
}

static numerix_status initialize_integration(
    numerix_function function, double lower, double upper,
    const numerix_options *input, numerix_options *options,
    numerix_result *result, numerix_error *error, const char *algorithm)
{
    if (result == NULL)
    {
        return integration_error(error, NUMERIX_STATUS_INVALID_ARGUMENT, "result required");
    }
    memset(result, 0, sizeof(*result));
    result->value = NAN;
    result->estimated_error = NAN;
    result->residual = NAN;
    result->algorithm = algorithm;
    result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
    if (function == NULL || !isfinite(lower) || !isfinite(upper))
    {
        return integration_error(error, result->status, "finite bounds and callback required");
    }
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

static numerix_status evaluate_integrand(
    numerix_function function, void *data, double position, double *value,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    if (result->function_evaluations >= options->maximum_function_evaluations)
    {
        return integration_error(error, NUMERIX_STATUS_LIMIT_REACHED, "evaluation limit reached");
    }
    if (!isfinite(position))
    {
        return integration_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE, "non-finite sample point");
    }
    *value = function(position, data);
    ++result->function_evaluations;
    if (!isfinite(*value))
    {
        return integration_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE, "non-finite integrand");
    }
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status finish_integration(
    numerix_result *result, numerix_status status, double value,
    double estimate, double start, numerix_error *error)
{
    if (status == NUMERIX_STATUS_SUCCESS && (!isfinite(value) || isinf(estimate)))
    {
        status = NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    result->status = status;
    result->value = status == NUMERIX_STATUS_SUCCESS ? value : NAN;
    result->estimated_error = estimate;
    result->execution_time_seconds = fmax(0.0, numerix_platform_monotonic_seconds() - start);
    numerix_error_set(error, status, status == NUMERIX_STATUS_SUCCESS ? "" : numerix_status_string(status));
    return status;
}

static numerix_status fixed_integration(
    numerix_function function, void *data, double lower, double upper, size_t panels,
    const numerix_options *input, numerix_result *result, numerix_error *error, int simpson)
{
    numerix_options options;
    numerix_status status;
    double start = numerix_platform_monotonic_seconds();
    double step;
    long double sum = 0.0L;
    long double correction = 0.0L;
    size_t index;

    status = initialize_integration(function, lower, upper, input, &options, result, error,
        simpson ? "composite_simpson_1_3" : "composite_trapezoid");
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    if (panels == 0U || (simpson && panels % 2U != 0U))
    {
        return finish_integration(result, NUMERIX_STATUS_INVALID_INPUT, NAN, NAN, start, error);
    }
    if (panels > options.maximum_iterations || panels == SIZE_MAX)
    {
        return finish_integration(result, NUMERIX_STATUS_LIMIT_REACHED, NAN, NAN, start, error);
    }
    if (lower == upper)
    {
        return finish_integration(result, NUMERIX_STATUS_SUCCESS, 0.0, 0.0, start, error);
    }
    step = (upper - lower) / (double)panels;
    if (!isfinite(step) || step == 0.0)
    {
        return finish_integration(result, NUMERIX_STATUS_INVALID_INPUT, NAN, NAN, start, error);
    }
    for (index = 0; index <= panels; ++index)
    {
        double value = NAN;
        double position = index == panels ? upper : lower + step * (double)index;
        double weight = index == 0U || index == panels ? 1.0 :
            simpson ? (index % 2U != 0U ? 4.0 : 2.0) : 2.0;

        status = evaluate_integrand(function, data, position, &value, &options, result, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return finish_integration(result, status, NAN, NAN, start, error);
        }
        {
            long double adjusted = (long double)weight * value - correction;
            long double next = sum + adjusted;

            correction = (next - sum) - adjusted;
            sum = next;
        }
    }
    result->iterations = panels;
    return finish_integration(result, NUMERIX_STATUS_SUCCESS,
        (double)(sum * step / (simpson ? 3.0L : 2.0L)), NAN, start, error);
}

numerix_status numerix_integrate_trapezoid(
    numerix_function function, void *data, double lower, double upper, size_t panels,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return fixed_integration(function, data, lower, upper, panels, options, result, error, 0);
}

numerix_status numerix_integrate_simpson(
    numerix_function function, void *data, double lower, double upper, size_t panels,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    return fixed_integration(function, data, lower, upper, panels, options, result, error, 1);
}

typedef struct simpson_panel
{
    double lower;
    double upper;
    double left_value;
    double middle_value;
    double right_value;
    double left_middle_value;
    double right_middle_value;
    double left_integral;
    double right_integral;
    double integral;
    double error;
    size_t depth;
} simpson_panel;

static double simpson_rule(double lower, double upper, double left, double middle, double right)
{
    return (double)(((long double)upper - lower) *
        ((long double)left + 4.0L * middle + right) / 6.0L);
}

static numerix_status assess_panel(
    simpson_panel *panel, double whole, numerix_function function, void *data,
    const numerix_options *options, numerix_result *result, numerix_error *error)
{
    double middle = panel->lower + (panel->upper - panel->lower) * 0.5;
    double left_middle = panel->lower + (middle - panel->lower) * 0.5;
    double right_middle = middle + (panel->upper - middle) * 0.5;
    double difference;
    numerix_status status;

    if (left_middle == panel->lower || right_middle == panel->upper ||
        middle == panel->lower || middle == panel->upper)
    {
        return NUMERIX_STATUS_CONVERGENCE_FAILURE;
    }
    status = evaluate_integrand(function, data, left_middle, &panel->left_middle_value,
                                options, result, error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = evaluate_integrand(function, data, right_middle, &panel->right_middle_value,
                                    options, result, error);
    }
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    panel->left_integral = simpson_rule(panel->lower, middle,
        panel->left_value, panel->left_middle_value, panel->middle_value);
    panel->right_integral = simpson_rule(middle, panel->upper,
        panel->middle_value, panel->right_middle_value, panel->right_value);
    difference = panel->left_integral + panel->right_integral - whole;
    panel->integral = panel->left_integral + panel->right_integral + difference / 15.0;
    /* Full successive-rule difference is conservative versus the smooth-only /15 model. */
    panel->error = fmax(fabs(difference), 16.0 * DBL_EPSILON * fabs(panel->integral));
    if (!isfinite(panel->integral) || !isfinite(panel->error))
    {
        return NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static void push_panel(simpson_panel *heap, size_t *count, simpson_panel panel)
{
    size_t position = (*count)++;

    while (position > 0U)
    {
        size_t parent = (position - 1U) / 2U;

        if (heap[parent].error >= panel.error)
        {
            break;
        }
        heap[position] = heap[parent];
        position = parent;
    }
    heap[position] = panel;
}

static simpson_panel pop_panel(simpson_panel *heap, size_t *count)
{
    simpson_panel first = heap[0];
    simpson_panel last = heap[--(*count)];
    size_t position = 0U;

    while (position * 2U + 1U < *count)
    {
        size_t child = position * 2U + 1U;

        if (child + 1U < *count && heap[child + 1U].error > heap[child].error)
        {
            ++child;
        }
        if (last.error >= heap[child].error)
        {
            break;
        }
        heap[position] = heap[child];
        position = child;
    }
    if (*count != 0U)
    {
        heap[position] = last;
    }
    return first;
}

numerix_status numerix_integrate_adaptive_simpson(
    numerix_function function, void *data, double lower, double upper,
    const numerix_options *input, size_t maximum_depth,
    numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    simpson_panel initial = {0};
    simpson_panel *heap = NULL;
    size_t count = 0U;
    size_t capacity = 32U;
    long double integral;
    long double error_sum;
    double estimate = NAN;
    double start = numerix_platform_monotonic_seconds();

    status = initialize_integration(function, lower, upper, input, &options,
                                    result, error, "adaptive_simpson");
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    if (maximum_depth == 0U || maximum_depth > 64U || !isfinite(upper - lower))
    {
        return finish_integration(result, NUMERIX_STATUS_INVALID_ARGUMENT, NAN, NAN, start, error);
    }
    if (lower == upper)
    {
        return finish_integration(result, NUMERIX_STATUS_SUCCESS, 0.0, 0.0, start, error);
    }
    initial.lower = fmin(lower, upper);
    initial.upper = fmax(lower, upper);
    status = evaluate_integrand(function, data, initial.lower, &initial.left_value,
                                &options, result, error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = evaluate_integrand(function, data, initial.lower + (initial.upper - initial.lower) * 0.5,
                                    &initial.middle_value, &options, result, error);
    }
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = evaluate_integrand(function, data, initial.upper, &initial.right_value,
                                    &options, result, error);
    }
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        status = assess_panel(&initial, simpson_rule(initial.lower, initial.upper,
            initial.left_value, initial.middle_value, initial.right_value),
            function, data, &options, result, error);
    }
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return finish_integration(result, status, NAN, NAN, start, error);
    }
    heap = malloc(capacity * sizeof(*heap));
    if (heap == NULL)
    {
        return finish_integration(result, NUMERIX_STATUS_ALLOCATION_FAILURE, NAN, NAN, start, error);
    }
    push_panel(heap, &count, initial);
    integral = initial.integral;
    error_sum = initial.error;
    for (;;)
    {
        simpson_panel parent;
        simpson_panel left = {0};
        simpson_panel right = {0};
        long double tolerance = fmax(options.absolute_tolerance,
                                    options.relative_tolerance * fabs((double)integral));
        double middle;

        estimate = (double)error_sum;
        if (error_sum <= tolerance)
        {
            status = NUMERIX_STATUS_SUCCESS;
            break;
        }
        if (tolerance < 16.0L * DBL_EPSILON * fabsl(integral))
        {
            status = NUMERIX_STATUS_CONVERGENCE_FAILURE;
            break;
        }
        if (count == 0U || result->iterations >= options.maximum_iterations)
        {
            status = NUMERIX_STATUS_LIMIT_REACHED;
            break;
        }
        parent = pop_panel(heap, &count);
        if (parent.depth >= maximum_depth)
        {
            /* Its contribution stays in the global sum; refine other panels. */
            continue;
        }
        if (count + 2U > capacity)
        {
            simpson_panel *replacement;

            if (capacity > SIZE_MAX / 2U / sizeof(*heap))
            {
                status = NUMERIX_STATUS_ALLOCATION_FAILURE;
                break;
            }
            capacity *= 2U;
            replacement = realloc(heap, capacity * sizeof(*heap));
            if (replacement == NULL)
            {
                status = NUMERIX_STATUS_ALLOCATION_FAILURE;
                break;
            }
            heap = replacement;
        }
        middle = parent.lower + (parent.upper - parent.lower) * 0.5;
        left.lower = parent.lower;
        left.upper = middle;
        left.left_value = parent.left_value;
        left.middle_value = parent.left_middle_value;
        left.right_value = parent.middle_value;
        left.depth = parent.depth + 1U;
        right.lower = middle;
        right.upper = parent.upper;
        right.left_value = parent.middle_value;
        right.middle_value = parent.right_middle_value;
        right.right_value = parent.right_value;
        right.depth = left.depth;
        status = assess_panel(&left, parent.left_integral, function, data, &options, result, error);
        if (status == NUMERIX_STATUS_SUCCESS)
        {
            status = assess_panel(&right, parent.right_integral, function, data, &options, result, error);
        }
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            break;
        }
        integral += (long double)left.integral + right.integral - parent.integral;
        error_sum = fmaxl(0.0L, error_sum + (long double)left.error + right.error - parent.error);
        ++result->iterations;
        push_panel(heap, &count, left);
        push_panel(heap, &count, right);
    }
    free(heap);
    result->residual = NAN;
    return finish_integration(result, status, (double)(lower < upper ? integral : -integral),
                              estimate, start, error);
}
