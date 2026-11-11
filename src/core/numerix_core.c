#include "numerix_internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *numerix_status_string(numerix_status status)
{
    switch (status) {
    case NUMERIX_STATUS_SUCCESS: return "success";
    case NUMERIX_STATUS_INVALID_ARGUMENT: return "invalid argument";
    case NUMERIX_STATUS_INVALID_INPUT: return "invalid input";
    case NUMERIX_STATUS_INVALID_DIMENSIONS: return "invalid dimensions";
    case NUMERIX_STATUS_ALLOCATION_FAILURE: return "allocation failure";
    case NUMERIX_STATUS_NUMERICAL_FAILURE: return "numerical failure";
    case NUMERIX_STATUS_CONVERGENCE_FAILURE: return "convergence failure";
    case NUMERIX_STATUS_SINGULAR_MATRIX: return "singular matrix";
    case NUMERIX_STATUS_UNSUPPORTED_OPERATION: return "unsupported operation";
    case NUMERIX_STATUS_CANCELLED: return "cancelled";
    case NUMERIX_STATUS_LIMIT_REACHED: return "execution limit reached";
    case NUMERIX_STATUS_INTERNAL_ERROR: return "internal error";
    default: return "unknown status";
    }
}

void numerix_error_assign(numerix_error *error, numerix_status code,
                          const char *message)
{
    if (error == NULL) return;
    error->code = code;
    error->value = 0.0;
    error->has_value = 0;
    if (message == NULL) message = numerix_status_string(code);
    (void)snprintf(error->message, sizeof(error->message), "%s", message);
}

void numerix_error_clear(numerix_error *error)
{
    numerix_error_assign(error, NUMERIX_STATUS_SUCCESS, "");
}

numerix_status numerix_error_set(numerix_error *error, numerix_status code,
                                 const char *message)
{
    if (error == NULL) return NUMERIX_STATUS_INVALID_ARGUMENT;
    numerix_error_assign(error, code, message);
    return code;
}

void numerix_options_init(numerix_options *options)
{
    if (options == NULL) return;
    options->absolute_tolerance = 1e-8;
    options->relative_tolerance = 1e-8;
    options->maximum_iterations = 1000;
    options->maximum_function_evaluations = 10000;
    options->maximum_execution_time_seconds = 0.0;
}

numerix_status numerix_options_validate(const numerix_options *options,
                                        numerix_error *error)
{
    if (options == NULL) {
        numerix_error_assign(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "options must not be NULL");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    if (!isfinite(options->absolute_tolerance) || options->absolute_tolerance < 0.0 ||
        !isfinite(options->relative_tolerance) || options->relative_tolerance < 0.0 ||
        (options->absolute_tolerance == 0.0 && options->relative_tolerance == 0.0) ||
        options->maximum_iterations == 0 ||
        options->maximum_function_evaluations == 0 ||
        !isfinite(options->maximum_execution_time_seconds) ||
        options->maximum_execution_time_seconds < 0.0) {
        numerix_error_assign(error, NUMERIX_STATUS_INVALID_INPUT,
                             "options contain an invalid tolerance or execution limit");
        return NUMERIX_STATUS_INVALID_INPUT;
    }
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_scalar_problem_init(numerix_scalar_problem *problem,
                                          numerix_function function,
                                          void *user_data,
                                          numerix_error *error)
{
    if (problem == NULL || function == NULL) {
        numerix_error_assign(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "problem and callback must not be NULL");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    problem->function = function;
    problem->user_data = user_data;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_scalar_problem_evaluate(
    const numerix_scalar_problem *problem, double x, double *value,
    numerix_error *error)
{
    if (problem == NULL || problem->function == NULL || value == NULL || !isfinite(x)) {
        numerix_error_assign(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "problem, callback, finite input, and output are required");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    *value = problem->function(x, problem->user_data);
    if (!isfinite(*value)) {
        numerix_error_assign(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                             "callback returned a non-finite value");
        return NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void *numerix_allocate(size_t size)
{
    if (size == 0) return NULL;
    return malloc(size);
}

void numerix_deallocate(void *memory)
{
    free(memory);
}

numerix_status numerix_context_create(const numerix_options *options,
                                      numerix_context **context,
                                      numerix_error *error)
{
    numerix_options defaults;
    numerix_status status;
    numerix_context *created;
    if (context == NULL) {
        numerix_error_assign(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "context output must not be NULL");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    *context = NULL;
    if (options == NULL) {
        numerix_options_init(&defaults);
        options = &defaults;
    }
    status = numerix_options_validate(options, error);
    if (status != NUMERIX_STATUS_SUCCESS) return status;
    created = numerix_allocate(sizeof(*created));
    if (created == NULL) {
        numerix_error_assign(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                             "could not allocate execution context");
        return NUMERIX_STATUS_ALLOCATION_FAILURE;
    }
    created->options = *options;
    created->status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(&created->error);
    created->iterations = 0;
    created->function_evaluations = 0;
    created->timer.start_seconds = numerix_platform_monotonic_seconds();
    created->cancelled = 0;
    *context = created;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void numerix_context_destroy(numerix_context *context)
{
    numerix_deallocate(context);
}

numerix_status numerix_context_record_iteration(numerix_context *context)
{
    if (context == NULL) return NUMERIX_STATUS_INVALID_ARGUMENT;
    if (context->iterations == SIZE_MAX) return NUMERIX_STATUS_LIMIT_REACHED;
    ++context->iterations;
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_context_record_function_evaluation(numerix_context *context)
{
    if (context == NULL) return NUMERIX_STATUS_INVALID_ARGUMENT;
    if (context->function_evaluations == SIZE_MAX) return NUMERIX_STATUS_LIMIT_REACHED;
    ++context->function_evaluations;
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_context_reset_counters(numerix_context *context)
{
    if (context == NULL) return NUMERIX_STATUS_INVALID_ARGUMENT;
    context->iterations = 0;
    context->function_evaluations = 0;
    return NUMERIX_STATUS_SUCCESS;
}

size_t numerix_context_iteration_count(const numerix_context *context)
{ return context == NULL ? 0 : context->iterations; }

size_t numerix_context_function_evaluation_count(const numerix_context *context)
{ return context == NULL ? 0 : context->function_evaluations; }

double numerix_context_elapsed_seconds(const numerix_context *context)
{
    double elapsed;
    if (context == NULL) return 0.0;
    elapsed = numerix_platform_monotonic_seconds() - context->timer.start_seconds;
    return elapsed < 0.0 ? 0.0 : elapsed;
}

numerix_status numerix_initialize(numerix_error *error)
{
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
