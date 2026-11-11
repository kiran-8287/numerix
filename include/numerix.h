#ifndef NUMERIX_H
#define NUMERIX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUMERIX_VERSION_MAJOR 0
#define NUMERIX_VERSION_MINOR 1
#define NUMERIX_VERSION_PATCH 0
#define NUMERIX_VERSION_STRING "0.1.0"
#define NUMERIX_ERROR_MESSAGE_CAPACITY 256

typedef enum numerix_status {
    NUMERIX_STATUS_SUCCESS = 0,
    NUMERIX_STATUS_INVALID_ARGUMENT,
    NUMERIX_STATUS_INVALID_INPUT,
    NUMERIX_STATUS_INVALID_DIMENSIONS,
    NUMERIX_STATUS_ALLOCATION_FAILURE,
    NUMERIX_STATUS_NUMERICAL_FAILURE,
    NUMERIX_STATUS_CONVERGENCE_FAILURE,
    NUMERIX_STATUS_SINGULAR_MATRIX,
    NUMERIX_STATUS_UNSUPPORTED_OPERATION,
    NUMERIX_STATUS_CANCELLED,
    NUMERIX_STATUS_LIMIT_REACHED,
    NUMERIX_STATUS_INTERNAL_ERROR
} numerix_status;

typedef struct numerix_error {
    numerix_status code;
    char message[NUMERIX_ERROR_MESSAGE_CAPACITY];
    double value;
    int has_value;
} numerix_error;

typedef double (*numerix_function)(double x, void *user_data);

typedef struct numerix_scalar_problem {
    numerix_function function;
    void *user_data; /* Borrowed; the caller retains ownership. */
} numerix_scalar_problem;

typedef struct numerix_options {
    double absolute_tolerance;
    double relative_tolerance;
    size_t maximum_iterations;
    size_t maximum_function_evaluations;
    double maximum_execution_time_seconds; /* 0 disables the time limit. */
} numerix_options;

typedef struct numerix_result {
    numerix_status status;
    double value;
    double estimated_error;
    double residual;
    size_t iterations;
    size_t function_evaluations;
    double execution_time_seconds;
    const char *algorithm; /* Borrowed static string or NULL. */
} numerix_result;

typedef struct numerix_context numerix_context;

/* Returns a static readable name for a status code. */
const char *numerix_status_string(numerix_status status);
/* Clears an error, or records a bounded, copied message and status. */
void numerix_error_clear(numerix_error *error);
numerix_status numerix_error_set(numerix_error *error, numerix_status code,
                                 const char *message);
/* Initializes recommended defaults or validates all shared option fields. */
void numerix_options_init(numerix_options *options);
numerix_status numerix_options_validate(const numerix_options *options,
                                        numerix_error *error);
/* Problem borrows the callback and user_data; neither is freed by NUMERIX. */
numerix_status numerix_scalar_problem_init(numerix_scalar_problem *problem,
                                          numerix_function function,
                                          void *user_data,
                                          numerix_error *error);
numerix_status numerix_scalar_problem_evaluate(
    const numerix_scalar_problem *problem, double x, double *value,
    numerix_error *error);
/* Allocation ownership belongs to the caller; size zero returns NULL. */
void *numerix_allocate(size_t size);
void numerix_deallocate(void *memory);
/* Context is opaque and caller-owned after success. NULL options use defaults. */
numerix_status numerix_context_create(const numerix_options *options,
                                      numerix_context **context,
                                      numerix_error *error);
void numerix_context_destroy(numerix_context *context);
numerix_status numerix_context_record_iteration(numerix_context *context);
numerix_status numerix_context_record_function_evaluation(
    numerix_context *context);
numerix_status numerix_context_reset_counters(numerix_context *context);
size_t numerix_context_iteration_count(const numerix_context *context);
size_t numerix_context_function_evaluation_count(
    const numerix_context *context);
double numerix_context_elapsed_seconds(const numerix_context *context);
numerix_status numerix_initialize(numerix_error *error);

#ifdef __cplusplus
}
#endif
#endif
