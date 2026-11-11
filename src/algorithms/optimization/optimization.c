#include "numerix_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static numerix_status fail(numerix_error *error, numerix_status status,
                           const char *message)
{
    return numerix_error_set(error, status, message);
}

numerix_status numerix_optimize_golden_section(
    numerix_function function, void *data, double lower, double upper,
    const numerix_options *input_options, numerix_result *result,
    numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    const double ratio = 0.6180339887498948482;
    double x1, x2, f1, f2, start;

    if (result == NULL) return fail(error, NUMERIX_STATUS_INVALID_ARGUMENT, "result required");
    memset(result, 0, sizeof(*result));
    result->algorithm = "golden_section_search";
    if (input_options != NULL) options = *input_options;
    else numerix_options_init(&options);
    status = numerix_options_validate(&options, error);
    if (status != NUMERIX_STATUS_SUCCESS) return result->status = status;
    if (function == NULL || !isfinite(lower) || !isfinite(upper) || !(lower < upper)) {
        result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return fail(error, result->status, "finite ordered bounds and callback required");
    }
    if (options.maximum_function_evaluations < 2) {
        result->status = NUMERIX_STATUS_LIMIT_REACHED;
        return fail(error, result->status, "function evaluation limit is below initial requirement");
    }
    start = numerix_platform_monotonic_seconds();
    x1 = upper - ratio * (upper - lower);
    x2 = lower + ratio * (upper - lower);
    f1 = function(x1, data);
    f2 = function(x2, data);
    result->function_evaluations = 2;
    if (!isfinite(f1) || !isfinite(f2)) {
        result->status = NUMERIX_STATUS_NUMERICAL_FAILURE;
        return fail(error, result->status, "objective returned a non-finite value");
    }
    while (result->iterations < options.maximum_iterations &&
           fabs(upper - lower) > options.absolute_tolerance +
                                  options.relative_tolerance * fmax(fabs(lower), fabs(upper))) {
        double next, value;
        if (result->function_evaluations >= options.maximum_function_evaluations) break;
        if (f1 <= f2) {
            upper = x2; x2 = x1; f2 = f1;
            next = lower + (1.0 - ratio) * (upper - lower);
            value = function(next, data); x1 = next; f1 = value;
        } else {
            lower = x1; x1 = x2; f1 = f2;
            next = lower + ratio * (upper - lower);
            value = function(next, data); x2 = next; f2 = value;
        }
        ++result->function_evaluations;
        ++result->iterations;
        if (!isfinite(value)) {
            result->status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            return fail(error, result->status, "objective returned a non-finite value");
        }
    }
    result->value = f1 <= f2 ? x1 : x2;
    result->residual = f1 <= f2 ? f1 : f2;
    result->estimated_error = fabs(upper - lower) * 0.5;
    result->execution_time_seconds = fmax(0.0, numerix_platform_monotonic_seconds() - start);
    if (fabs(upper - lower) > options.absolute_tolerance +
                               options.relative_tolerance * fmax(fabs(lower), fabs(upper))) {
        result->status = NUMERIX_STATUS_LIMIT_REACHED;
        return fail(error, result->status, "optimization limit reached before interval converged");
    }
    result->status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

static int valid_vector(const numerix_vector *vector)
{
    size_t i;
    if (vector == NULL || vector->data == NULL || vector->size == 0) return 0;
    for (i = 0; i < vector->size; ++i) if (!isfinite(vector->data[i])) return 0;
    return 1;
}

static double vector_norm(const double *values, size_t count)
{
    size_t i;
    double scale = 0.0, sum = 1.0;
    for (i = 0; i < count; ++i) {
        double value = fabs(values[i]);
        if (value != 0.0) {
            if (scale < value) { double q = scale / value; sum = 1.0 + sum * q * q; scale = value; }
            else { double q = value / scale; sum += q * q; }
        }
    }
    return scale == 0.0 ? 0.0 : scale * sqrt(sum);
}

numerix_status numerix_optimize_gradient_descent(
    numerix_objective_function objective, numerix_gradient_function gradient,
    void *data, const numerix_vector *initial, double initial_step,
    const numerix_options *input_options, numerix_vector **solution,
    numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status, gradient_status;
    double *x = NULL, *g = NULL, *trial = NULL, fx, gradient_norm;
    size_t n, i, j, backtrack;
    if (solution == NULL || result == NULL) return fail(error, NUMERIX_STATUS_INVALID_ARGUMENT, "solution and result required");
    *solution = NULL;
    memset(result, 0, sizeof(*result));
    result->algorithm = "gradient_descent_armijo";
    if (input_options != NULL) options = *input_options;
    else numerix_options_init(&options);
    status = numerix_options_validate(&options, error);
    if (status != NUMERIX_STATUS_SUCCESS) return result->status = status;
    if (objective == NULL || gradient == NULL || !valid_vector(initial) ||
        !isfinite(initial_step) || initial_step <= 0.0) {
        result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return fail(error, result->status, "objective, gradient, finite initial vector, and positive step required");
    }
    n = initial->size;
    if (n > SIZE_MAX / sizeof(double)) {
        result->status = NUMERIX_STATUS_INVALID_DIMENSIONS;
        return fail(error, result->status, "dimension too large");
    }
    if (options.maximum_function_evaluations == 0) {
        result->status = NUMERIX_STATUS_LIMIT_REACHED;
        return fail(error, result->status, "function evaluation limit reached");
    }
    x = malloc(n * sizeof(double)); g = malloc(n * sizeof(double)); trial = malloc(n * sizeof(double));
    if (x == NULL || g == NULL || trial == NULL) { status = NUMERIX_STATUS_ALLOCATION_FAILURE; goto cleanup; }
    memcpy(x, initial->data, n * sizeof(double));
    fx = objective(x, n, data); ++result->function_evaluations;
    if (!isfinite(fx)) { status = NUMERIX_STATUS_NUMERICAL_FAILURE; goto cleanup; }
    for (i = 0; i < options.maximum_iterations; ++i) {
        double alpha = initial_step;
        gradient_status = gradient(x, n, g, data);
        if (gradient_status != NUMERIX_STATUS_SUCCESS) { status = gradient_status; goto cleanup; }
        for (j = 0; j < n; ++j) if (!isfinite(g[j])) { status = NUMERIX_STATUS_NUMERICAL_FAILURE; goto cleanup; }
        gradient_norm = vector_norm(g, n);
        result->iterations = i;
        if (gradient_norm <= options.absolute_tolerance + options.relative_tolerance * fmax(1.0, vector_norm(x, n))) {
            status = NUMERIX_STATUS_SUCCESS; goto finish;
        }
        for (backtrack = 0; backtrack < 60; ++backtrack) {
            double candidate;
            for (j = 0; j < n; ++j) trial[j] = x[j] - alpha * g[j];
            if (result->function_evaluations >= options.maximum_function_evaluations) {
                status = NUMERIX_STATUS_LIMIT_REACHED; goto cleanup;
            }
            candidate = objective(trial, n, data); ++result->function_evaluations;
            if (isfinite(candidate) && candidate <= fx - 1e-4 * alpha * gradient_norm * gradient_norm) {
                memcpy(x, trial, n * sizeof(double)); fx = candidate; break;
            }
            alpha *= 0.5;
        }
        if (backtrack == 60) { status = NUMERIX_STATUS_CONVERGENCE_FAILURE; goto cleanup; }
        result->iterations = i + 1;
    }
    status = NUMERIX_STATUS_LIMIT_REACHED;
    goto cleanup;

finish:
    *solution = malloc(sizeof(**solution));
    if (*solution == NULL) { status = NUMERIX_STATUS_ALLOCATION_FAILURE; goto cleanup; }
    (*solution)->size = n; (*solution)->data = x; x = NULL;
    result->value = fx; result->residual = gradient_norm; result->status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(error);
    free(g); free(trial);
    return NUMERIX_STATUS_SUCCESS;

cleanup:
    free(x); free(g); free(trial);
    result->status = status;
    numerix_error_set(error, status, numerix_status_string(status));
    return status;
}
