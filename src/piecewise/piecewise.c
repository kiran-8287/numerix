#include "piecewise_internal.h"

#include <math.h>
#include <string.h>

static numerix_status piecewise_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

numerix_status numerix_piecewise_create(
    numerix_piecewise_function **function, numerix_error *error)
{
    numerix_piecewise_function *created;

    if (function == NULL) {
        return piecewise_error(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "piecewise function output is required");
    }

    *function = NULL;
    created = numerix_allocate(sizeof(*created));
    if (created == NULL) {
        return piecewise_error(
            error, NUMERIX_STATUS_ALLOCATION_FAILURE,
            "could not allocate piecewise function");
    }

    created->segments = NULL;
    created->count = 0;
    created->capacity = 0;
    *function = created;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status reserve_segment(
    numerix_piecewise_function *function, numerix_error *error)
{
    size_t new_capacity;
    numerix_piecewise_segment *new_segments;

    if (function->count >= NUMERIX_PIECEWISE_MAX_SEGMENTS) {
        return piecewise_error(
            error, NUMERIX_STATUS_LIMIT_REACHED,
            "piecewise segment limit reached");
    }
    if (function->count < function->capacity) {
        return NUMERIX_STATUS_SUCCESS;
    }

    new_capacity = function->capacity == 0 ? 8 : function->capacity * 2;
    if (new_capacity > NUMERIX_PIECEWISE_MAX_SEGMENTS) {
        new_capacity = NUMERIX_PIECEWISE_MAX_SEGMENTS;
    }
    new_segments = numerix_allocate(new_capacity * sizeof(*new_segments));
    if (new_segments == NULL) {
        return piecewise_error(
            error, NUMERIX_STATUS_ALLOCATION_FAILURE,
            "could not grow piecewise segment storage");
    }
    if (function->count > 0) {
        memcpy(new_segments, function->segments,
               function->count * sizeof(*new_segments));
    }
    numerix_deallocate(function->segments);
    function->segments = new_segments;
    function->capacity = new_capacity;
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_piecewise_add_expression(
    numerix_piecewise_function *function, double lower_bound,
    double upper_bound, int include_lower, int include_upper,
    const char *source, numerix_error *error)
{
    numerix_expression *expression = NULL;
    numerix_piecewise_segment *previous;
    numerix_status status;

    if (function == NULL || source == NULL) {
        return piecewise_error(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "piecewise function and expression are required");
    }
    if (isnan(lower_bound) || isnan(upper_bound) ||
        !(lower_bound < upper_bound) ||
        (isinf(lower_bound) && lower_bound > 0.0) ||
        (isinf(upper_bound) && upper_bound < 0.0) ||
        (include_lower != 0 && include_lower != 1) ||
        (include_upper != 0 && include_upper != 1) ||
        (isinf(lower_bound) && include_lower) ||
        (isinf(upper_bound) && include_upper)) {
        return piecewise_error(
            error, NUMERIX_STATUS_INVALID_INPUT,
            "invalid piecewise interval or boundary inclusion flag");
    }

    if (function->count > 0) {
        previous = &function->segments[function->count - 1];
        if (lower_bound < previous->lower_bound ||
            lower_bound < previous->upper_bound ||
            (lower_bound == previous->upper_bound &&
             include_lower && previous->include_upper)) {
            return piecewise_error(
                error, NUMERIX_STATUS_INVALID_INPUT,
                "piecewise segments must be ordered and non-overlapping");
        }
    }

    status = numerix_expression_compile(source, &expression, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    status = reserve_segment(function, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        numerix_expression_destroy(expression);
        return status;
    }

    function->segments[function->count].lower_bound = lower_bound;
    function->segments[function->count].upper_bound = upper_bound;
    function->segments[function->count].include_lower = include_lower;
    function->segments[function->count].include_upper = include_upper;
    function->segments[function->count].expression = expression;
    ++function->count;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

const numerix_piecewise_segment *numerix_piecewise_find_segment(
    const numerix_piecewise_function *function, double x)
{
    size_t index;

    if (function == NULL || !isfinite(x)) {
        return NULL;
    }
    for (index = 0; index < function->count; ++index) {
        const numerix_piecewise_segment *segment = &function->segments[index];
        int above_lower = x > segment->lower_bound ||
                          (x == segment->lower_bound && segment->include_lower);
        int below_upper = x < segment->upper_bound ||
                          (x == segment->upper_bound && segment->include_upper);
        if (above_lower && below_upper) {
            return segment;
        }
    }
    return NULL;
}

numerix_status numerix_piecewise_evaluate_segment(
    const numerix_piecewise_segment *segment, double x,
    double *value, numerix_error *error)
{
    if (segment == NULL || value == NULL) {
        return piecewise_error(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "piecewise segment and output value are required");
    }
    return numerix_expression_evaluate(segment->expression, x, value, error);
}

numerix_status numerix_piecewise_evaluate(
    const numerix_piecewise_function *function, double x,
    double *value, numerix_error *error)
{
    const numerix_piecewise_segment *segment;

    if (function == NULL || value == NULL || !isfinite(x)) {
        return piecewise_error(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "finite input, piecewise function, and output are required");
    }
    segment = numerix_piecewise_find_segment(function, x);
    if (segment == NULL) {
        return piecewise_error(
            error, NUMERIX_STATUS_INVALID_INPUT,
            "input lies outside the defined piecewise intervals");
    }
    return numerix_piecewise_evaluate_segment(segment, x, value, error);
}

void numerix_piecewise_destroy(numerix_piecewise_function *function)
{
    size_t index;

    if (function == NULL) {
        return;
    }
    for (index = 0; index < function->count; ++index) {
        numerix_expression_destroy(function->segments[index].expression);
    }
    numerix_deallocate(function->segments);
    numerix_deallocate(function);
}
