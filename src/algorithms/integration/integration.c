#include "numerix_internal.h"
#include <math.h>
#include <string.h>

static numerix_status fail(numerix_error *error, numerix_status status,
                           const char *message)
{
    return numerix_error_set(error, status, message);
}

static numerix_status setup(numerix_function function, double a, double b,
                            const numerix_options *input, numerix_options *options,
                            numerix_result *result, numerix_error *error,
                            const char *algorithm)
{
    numerix_status status;
    if (result == NULL) return fail(error, NUMERIX_STATUS_INVALID_ARGUMENT, "result required");
    memset(result, 0, sizeof(*result));
    result->algorithm = algorithm;
    if (function == NULL || !isfinite(a) || !isfinite(b)) {
        result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return fail(error, result->status, "finite interval and callback required");
    }
    if (input != NULL) *options = *input;
    else numerix_options_init(options);
    status = numerix_options_validate(options, error);
    result->status = status;
    return status;
}

static numerix_status evaluate(numerix_function function, void *data, double x,
                               double *value, const numerix_options *options,
                               numerix_result *result, numerix_error *error)
{
    if (result->function_evaluations >= options->maximum_function_evaluations)
        return fail(error, NUMERIX_STATUS_LIMIT_REACHED, "function evaluation limit reached");
    if (!isfinite(x)) return fail(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                  "integration sample point is non-finite");
    *value = function(x, data);
    ++result->function_evaluations;
    if (!isfinite(*value)) return fail(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                       "integrand returned a non-finite value");
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status finish(numerix_result *result, numerix_status status,
                             double value, double error_estimate, double start,
                             numerix_error *error)
{
    if (status == NUMERIX_STATUS_SUCCESS &&
        (!isfinite(value) || isinf(error_estimate))) {
        status = NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    if (status != NUMERIX_STATUS_SUCCESS) value = NAN;
    result->status = status;
    result->value = value;
    result->estimated_error = error_estimate;
    result->execution_time_seconds = fmax(0.0, numerix_platform_monotonic_seconds() - start);
    if (status == NUMERIX_STATUS_SUCCESS) numerix_error_clear(error);
    else numerix_error_set(error, status, numerix_status_string(status));
    return status;
}

static double now(void) { return numerix_platform_monotonic_seconds(); }

numerix_status numerix_integrate_trapezoid(
    numerix_function function, void *data, double a, double b, size_t panels,
    const numerix_options *input, numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    double start, h, fa, fb, sum, correction = 0.0;
    size_t i;
    status = setup(function, a, b, input, &options, result, error, "composite_trapezoid");
    if (status != NUMERIX_STATUS_SUCCESS) return status;
    start = now();
    if (panels == 0) return finish(result, NUMERIX_STATUS_INVALID_ARGUMENT, 0, NAN, start, error);
    if (panels > options.maximum_iterations) return finish(result, NUMERIX_STATUS_LIMIT_REACHED, 0, NAN, start, error);
    if (a == b) return finish(result, NUMERIX_STATUS_SUCCESS, 0, 0, start, error);
    h = (b - a) / (double)panels;
    if (!isfinite(h) || h == 0.0) return finish(result, NUMERIX_STATUS_INVALID_INPUT, 0, NAN, start, error);
    status = evaluate(function, data, a, &fa, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    status = evaluate(function, data, b, &fb, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    sum = 0.5 * (fa + fb);
    for (i = 1; i < panels; ++i) {
        double y, adjusted, next;
        status = evaluate(function, data, a + h * (double)i, &y, &options, result, error);
        if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
        adjusted = y - correction; next = sum + adjusted;
        correction = (next - sum) - adjusted; sum = next;
    }
    result->iterations = panels;
    return finish(result, NUMERIX_STATUS_SUCCESS, h * sum, NAN, start, error);
}

numerix_status numerix_integrate_simpson(
    numerix_function function, void *data, double a, double b, size_t panels,
    const numerix_options *input, numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    double start, h, fa, fb, sum, correction = 0.0;
    size_t i;
    status = setup(function, a, b, input, &options, result, error, "composite_simpson_1_3");
    if (status != NUMERIX_STATUS_SUCCESS) return status;
    start = now();
    if (panels == 0 || panels % 2 != 0) return finish(result, NUMERIX_STATUS_INVALID_INPUT, 0, NAN, start, error);
    if (panels > options.maximum_iterations) return finish(result, NUMERIX_STATUS_LIMIT_REACHED, 0, NAN, start, error);
    if (a == b) return finish(result, NUMERIX_STATUS_SUCCESS, 0, 0, start, error);
    h = (b - a) / (double)panels;
    if (!isfinite(h) || h == 0.0) {
        return finish(result, NUMERIX_STATUS_INVALID_INPUT, 0.0, NAN, start, error);
    }
    status = evaluate(function, data, a, &fa, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    status = evaluate(function, data, b, &fb, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    sum = fa + fb;
    for (i = 1; i < panels; ++i) {
        double y, adjusted, next;
        status = evaluate(function, data, a + h * (double)i, &y, &options, result, error);
        if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
        adjusted = (i % 2 ? 4.0 : 2.0) * y - correction; next = sum + adjusted;
        correction = (next - sum) - adjusted; sum = next;
    }
    result->iterations = panels;
    return finish(result, NUMERIX_STATUS_SUCCESS, h * sum / 3.0, NAN, start, error);
}

typedef struct adaptive_state {
    numerix_function function;
    void *data;
    const numerix_options *options;
    numerix_result *result;
    numerix_error *error;
    size_t maximum_depth;
    numerix_status status;
    double error_sum;
} adaptive_state;

static double adaptive_step(adaptive_state *state, double a, double b,
                            double fa, double fm, double fb, double whole,
                            double tolerance, size_t depth)
{
    double middle = a + (b - a) * 0.5;
    double left_middle = a + (middle - a) * 0.5;
    double right_middle = middle + (b - middle) * 0.5;
    double fl, fr, left, right, delta;
    if (state->status != NUMERIX_STATUS_SUCCESS) return 0.0;
    if (depth >= state->maximum_depth) {
        state->status = NUMERIX_STATUS_CONVERGENCE_FAILURE;
        return 0.0;
    }
    if (state->result->iterations >= state->options->maximum_iterations) {
        state->status = NUMERIX_STATUS_LIMIT_REACHED;
        return 0.0;
    }
    ++state->result->iterations;
    state->status = evaluate(state->function, state->data, left_middle, &fl,
                             state->options, state->result, state->error);
    if (state->status != NUMERIX_STATUS_SUCCESS) return 0.0;
    state->status = evaluate(state->function, state->data, right_middle, &fr,
                             state->options, state->result, state->error);
    if (state->status != NUMERIX_STATUS_SUCCESS) return 0.0;
    left = (middle - a) * (fa + 4.0 * fl + fm) / 6.0;
    right = (b - middle) * (fm + 4.0 * fr + fb) / 6.0;
    delta = left + right - whole;
    if (fabs(delta) <= 15.0 * tolerance) {
        state->error_sum += fabs(delta) / 15.0;
        return left + right + delta / 15.0;
    }
    return adaptive_step(state, a, middle, fa, fl, fm, left, tolerance * 0.5, depth + 1) +
           adaptive_step(state, middle, b, fm, fr, fb, right, tolerance * 0.5, depth + 1);
}

numerix_status numerix_integrate_adaptive_simpson(
    numerix_function function, void *data, double a, double b,
    const numerix_options *input, size_t maximum_depth,
    numerix_result *result, numerix_error *error)
{
    numerix_options options;
    adaptive_state state;
    numerix_status status;
    double start, fa, fm, fb, whole, value;
    status = setup(function, a, b, input, &options, result, error, "adaptive_simpson");
    if (status != NUMERIX_STATUS_SUCCESS) return status;
    start = now();
    if (maximum_depth == 0 || maximum_depth > 64)
        return finish(result, NUMERIX_STATUS_INVALID_ARGUMENT, 0, NAN, start, error);
    if (a == b) return finish(result, NUMERIX_STATUS_SUCCESS, 0, 0, start, error);
    status = evaluate(function, data, a, &fa, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    status = evaluate(function, data, a + (b - a) * 0.5, &fm, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    status = evaluate(function, data, b, &fb, &options, result, error);
    if (status != NUMERIX_STATUS_SUCCESS) return finish(result, status, 0, NAN, start, error);
    whole = (b - a) * (fa + 4.0 * fm + fb) / 6.0;
    state.function = function; state.data = data; state.options = &options;
    state.result = result; state.error = error; state.maximum_depth = maximum_depth;
    state.status = NUMERIX_STATUS_SUCCESS; state.error_sum = 0.0;
    value = adaptive_step(&state, a, b, fa, fm, fb, whole,
                          options.absolute_tolerance + options.relative_tolerance * fabs(whole), 0);
    result->residual = state.error_sum;
    return finish(result, state.status, value, state.error_sum, start, error);
}
