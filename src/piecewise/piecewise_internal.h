#ifndef NUMERIX_PIECEWISE_INTERNAL_H
#define NUMERIX_PIECEWISE_INTERNAL_H

#include "numerix_internal.h"
#include "numerix_expression_internal.h"

#define NUMERIX_PIECEWISE_MAX_SEGMENTS 4096U

typedef struct numerix_piecewise_segment {
    double lower_bound;
    double upper_bound;
    int include_lower;
    int include_upper;
    numerix_expression *expression;
} numerix_piecewise_segment;

struct numerix_piecewise_function {
    numerix_piecewise_segment *segments;
    size_t count;
    size_t capacity;
};

numerix_status numerix_piecewise_evaluate_segment(
    const numerix_piecewise_segment *segment, double x,
    double *value, numerix_error *error);

const numerix_piecewise_segment *numerix_piecewise_find_segment(
    const numerix_piecewise_function *function, double x);

#endif
