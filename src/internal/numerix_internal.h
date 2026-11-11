#ifndef NUMERIX_INTERNAL_H
#define NUMERIX_INTERNAL_H

#include "numerix.h"
#include <time.h>

typedef struct numerix_timer {
    double start_seconds;
} numerix_timer;

struct numerix_context {
    numerix_options options;
    numerix_status status;
    numerix_error error;
    size_t iterations;
    size_t function_evaluations;
    numerix_timer timer;
    int cancelled;
};

double numerix_platform_monotonic_seconds(void);
void numerix_error_assign(numerix_error *error, numerix_status code,
                          const char *message);
numerix_status numerix_context_accumulate_iterations(
    numerix_context *context, size_t iterations);
numerix_status numerix_context_accumulate_function_evaluations(
    numerix_context *context, size_t evaluations);

#endif
