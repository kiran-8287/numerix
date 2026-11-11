#include "piecewise_internal.h"

#include <math.h>
#include <string.h>

static numerix_status adapter_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

numerix_status numerix_piecewise_callback_context_init(
    numerix_piecewise_callback_context *context,
    const numerix_piecewise_function *function, numerix_error *error)
{
    if (context == NULL || function == NULL) {
        return adapter_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "callback context and piecewise function are required");
    }
    context->function = function;
    context->last_status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(&context->last_error);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

double numerix_piecewise_callback(double x, void *user_data)
{
    numerix_piecewise_callback_context *context = user_data;
    double value = NAN;

    if (context == NULL || context->function == NULL) {
        if (context != NULL) {
            context->last_status = NUMERIX_STATUS_INVALID_ARGUMENT;
            numerix_error_set(&context->last_error, context->last_status,
                              "piecewise callback context is not initialized");
        }
        return NAN;
    }

    context->last_status = numerix_piecewise_evaluate(
        context->function, x, &value, &context->last_error);
    return context->last_status == NUMERIX_STATUS_SUCCESS ? value : NAN;
}

numerix_status numerix_piecewise_callback_status(
    const numerix_piecewise_callback_context *context)
{
    return context == NULL ? NUMERIX_STATUS_INVALID_ARGUMENT : context->last_status;
}

const numerix_error *numerix_piecewise_callback_error(
    const numerix_piecewise_callback_context *context)
{
    return context == NULL ? NULL : &context->last_error;
}

typedef struct segment_callback_context {
    const numerix_piecewise_segment *segment;
    numerix_status last_status;
    numerix_error last_error;
} segment_callback_context;

static double segment_callback(double x, void *user_data)
{
    segment_callback_context *context = user_data;
    double value = NAN;

    context->last_status = numerix_piecewise_evaluate_segment(
        context->segment, x, &value, &context->last_error);
    return context->last_status == NUMERIX_STATUS_SUCCESS ? value : NAN;
}

static double move_into_segment(
    double point, const numerix_piecewise_segment *segment, int lower_edge)
{
    if (lower_edge && point == segment->lower_bound && !segment->include_lower) {
        return nextafter(point, segment->upper_bound);
    }
    if (!lower_edge && point == segment->upper_bound && !segment->include_upper) {
        return nextafter(point, segment->lower_bound);
    }
    return point;
}

numerix_status numerix_piecewise_integrate_trapezoid(
    const numerix_piecewise_function *function, double lower, double upper,
    size_t panels_per_segment, const numerix_options *options,
    numerix_result *result, numerix_error *error)
{
    size_t index;
    double cursor = lower;
    double total = 0.0;
    size_t evaluations = 0;
    size_t iterations = 0;
    int integrated = 0;

    if (result == NULL) {
        return adapter_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "integration result is required");
    }
    memset(result, 0, sizeof(*result));
    result->algorithm = "piecewise_composite_trapezoid";
    result->value = NAN;
    result->estimated_error = NAN;
    if (function == NULL || !isfinite(lower) || !isfinite(upper) ||
        !(lower < upper) || panels_per_segment == 0 || function->count == 0) {
        result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return adapter_error(error, result->status,
                             "invalid piecewise integration request");
    }

    for (index = 0; index < function->count; ++index) {
        const numerix_piecewise_segment *segment = &function->segments[index];
        double segment_lower = fmax(lower, segment->lower_bound);
        double segment_upper = fmin(upper, segment->upper_bound);
        double integration_lower;
        double integration_upper;
        numerix_result partial;
        numerix_status status;
        segment_callback_context callback_context;

        if (!(segment_lower < segment_upper)) {
            continue;
        }
        if (segment_lower > cursor) {
            result->status = NUMERIX_STATUS_INVALID_INPUT;
            return adapter_error(error, result->status,
                                 "integration interval contains an undefined gap");
        }

        integration_lower = move_into_segment(
            segment_lower, segment,
            segment_lower == segment->lower_bound);
        integration_upper = move_into_segment(
            segment_upper, segment,
            segment_upper == segment->upper_bound);
        if (!(integration_lower < integration_upper)) {
            result->status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            return adapter_error(error, result->status,
                                 "segment is too narrow for finite integration");
        }

        callback_context.segment = segment;
        callback_context.last_status = NUMERIX_STATUS_SUCCESS;
        numerix_error_clear(&callback_context.last_error);
        status = numerix_integrate_trapezoid(
            segment_callback, &callback_context, integration_lower,
            integration_upper, panels_per_segment, options, &partial, error);
        evaluations += partial.function_evaluations;
        iterations += partial.iterations;
        if (status != NUMERIX_STATUS_SUCCESS) {
            result->status = status;
            result->function_evaluations = evaluations;
            result->iterations = iterations;
            if (callback_context.last_status != NUMERIX_STATUS_SUCCESS && error != NULL) {
                *error = callback_context.last_error;
            }
            return status;
        }
        total += partial.value;
        if (!isfinite(total)) {
            result->status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            result->function_evaluations = evaluations;
            result->iterations = iterations;
            return adapter_error(error, result->status,
                                 "piecewise integral overflowed");
        }
        cursor = segment_upper;
        integrated = 1;
    }

    if (!integrated || cursor < upper) {
        result->status = NUMERIX_STATUS_INVALID_INPUT;
        return adapter_error(error, result->status,
                             "integration interval is not fully covered by segments");
    }

    result->status = NUMERIX_STATUS_SUCCESS;
    result->value = total;
    result->function_evaluations = evaluations;
    result->iterations = iterations;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_piecewise_diff_central(
    const numerix_piecewise_function *function, double x, double step,
    const numerix_options *options, numerix_result *result,
    numerix_error *error)
{
    size_t index;
    numerix_options diagnostic_options;
    numerix_piecewise_callback_context context;
    numerix_status status;

    if (function == NULL || result == NULL || !isfinite(x) ||
        !isfinite(step) || step <= 0.0) {
        return adapter_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "invalid piecewise derivative request");
    }
    if (options == NULL) {
        numerix_options_init(&diagnostic_options);
    } else {
        diagnostic_options = *options;
    }
    for (index = 0; index < function->count; ++index) {
        double boundaries[2] = {
            function->segments[index].lower_bound,
            function->segments[index].upper_bound
        };
        size_t boundary_index;

        for (boundary_index = 0; boundary_index < 2; ++boundary_index) {
            double boundary = boundaries[boundary_index];
            numerix_piecewise_boundary_report report;

            if (!isfinite(boundary) || boundary < x - step || boundary > x + step) {
                continue;
            }
            if (boundary_index == 1 && boundary == boundaries[0]) {
                continue;
            }
            status = numerix_piecewise_analyze_boundary(
                function, boundary, options, &report, error);
            if (status != NUMERIX_STATUS_SUCCESS) {
                return status;
            }
            if (report.type != NUMERIX_DISCONTINUITY_CONTINUOUS || boundary != x) {
                memset(result, 0, sizeof(*result));
                result->status = NUMERIX_STATUS_INVALID_INPUT;
                result->algorithm = "piecewise_central_difference";
                result->value = NAN;
                return adapter_error(error, result->status,
                                     "difference stencil crosses a piecewise boundary");
            }
            {
                double center_value;
                double left_value;
                double right_value;
                double left_slope;
                double right_slope;
                double tolerance;
                double scale;

                status = numerix_piecewise_evaluate(function, x, &center_value, error);
                if (status == NUMERIX_STATUS_SUCCESS) {
                    status = numerix_piecewise_evaluate(
                        function, x - step, &left_value, error);
                }
                if (status == NUMERIX_STATUS_SUCCESS) {
                    status = numerix_piecewise_evaluate(
                        function, x + step, &right_value, error);
                }
                if (status != NUMERIX_STATUS_SUCCESS) {
                    return status;
                }
                left_slope = (center_value - left_value) / step;
                right_slope = (right_value - center_value) / step;
                scale = fmax(fabs(left_slope), fabs(right_slope));
                tolerance = 10.0 * (diagnostic_options.absolute_tolerance +
                                    diagnostic_options.relative_tolerance * scale);
                if (!isfinite(left_slope) || !isfinite(right_slope) ||
                    fabs(left_slope - right_slope) > tolerance) {
                    memset(result, 0, sizeof(*result));
                    result->status = NUMERIX_STATUS_INVALID_INPUT;
                    result->algorithm = "piecewise_central_difference";
                    result->value = NAN;
                    return adapter_error(error, result->status,
                                         "one-sided derivative estimates disagree at boundary");
                }
            }
        }
    }

    status = numerix_piecewise_callback_context_init(&context, function, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    status = numerix_diff_central(
        numerix_piecewise_callback, &context, x, step,
        options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS &&
        context.last_status != NUMERIX_STATUS_SUCCESS && error != NULL) {
        *error = context.last_error;
    }
    return status;
}
