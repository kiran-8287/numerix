#include "numerix.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static numerix_status fail(numerix_error *error, numerix_status status,
                           const char *message)
{
    numerix_error_set(error, status, message);
    return status;
}

void numerix_ode_solution_destroy(numerix_ode_solution *solution)
{
    if (solution != NULL) {
        free(solution->x);
        free(solution->y);
        memset(solution, 0, sizeof(*solution));
    }
}

static numerix_status call_derivative(numerix_ode_function function, void *data,
                                      double x, const double *state,
                                      size_t dimension, double *derivative,
                                      numerix_ode_solution *solution,
                                      numerix_error *error)
{
    size_t index;
    numerix_status status = function(x, state, dimension, derivative, data);

    ++solution->function_evaluations;
    if (status != NUMERIX_STATUS_SUCCESS) {
        return fail(error, status, "ODE callback failed");
    }
    for (index = 0; index < dimension; ++index) {
        if (!isfinite(derivative[index])) {
            return fail(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                        "ODE callback returned non-finite derivative");
        }
    }
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status solve(numerix_ode_function function, void *data,
                            double initial_x, const double *initial_state,
                            size_t dimension, double step, size_t steps,
                            numerix_ode_solution *solution,
                            numerix_error *error, int method)
{
    size_t index;
    size_t component;
    size_t points;
    size_t total_values;
    double *k1 = NULL;
    double *k2 = NULL;
    double *k3 = NULL;
    double *k4 = NULL;
    double *temporary = NULL;
    numerix_status status = NUMERIX_STATUS_SUCCESS;

    if (solution == NULL) {
        return fail(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                    "solution output required");
    }
    memset(solution, 0, sizeof(*solution));
    if (function == NULL || initial_state == NULL || dimension == 0U ||
        steps == 0U || !isfinite(initial_x) || !isfinite(step) || step == 0.0 ||
        steps == SIZE_MAX ||
        dimension > SIZE_MAX / sizeof(double) ||
        steps + 1U > SIZE_MAX / sizeof(double) ||
        dimension > SIZE_MAX / (steps + 1U) ||
        dimension * (steps + 1U) > SIZE_MAX / sizeof(double)) {
        return fail(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                    "finite initial state, nonzero step, dimension, and step count required");
    }
    for (component = 0; component < dimension; ++component) {
        if (!isfinite(initial_state[component])) {
            return fail(error, NUMERIX_STATUS_INVALID_INPUT,
                        "initial state must be finite");
        }
    }
    points = steps + 1U;
    total_values = dimension * points;
    solution->x = malloc(points * sizeof(double));
    solution->y = malloc(total_values * sizeof(double));
    k1 = malloc(dimension * sizeof(double));
    if (method >= 1) {
        k2 = malloc(dimension * sizeof(double));
    }
    if (method >= 2) {
        k3 = malloc(dimension * sizeof(double));
        k4 = malloc(dimension * sizeof(double));
    }
    temporary = malloc(dimension * sizeof(double));
    if (solution->x == NULL || solution->y == NULL || k1 == NULL ||
        (method >= 1 && k2 == NULL) ||
        (method >= 2 && (k3 == NULL || k4 == NULL)) || temporary == NULL) {
        status = fail(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                      "ODE workspace allocation failed");
        goto cleanup;
    }
    solution->points = points;
    solution->dimension = dimension;
    solution->status = NUMERIX_STATUS_SUCCESS;
    memcpy(solution->y, initial_state, dimension * sizeof(double));
    solution->x[0] = initial_x;

    for (index = 0; index < steps; ++index) {
        double x = solution->x[index];
        double next_x = x + step;
        double half_step = 0.5 * step;
        double midpoint_x = x + half_step;
        const double *state = solution->y + index * dimension;
        double *next_state = solution->y + (index + 1U) * dimension;

        if (!isfinite(next_x) || next_x == x) {
            status = fail(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                          "ODE step does not advance the independent variable");
            goto cleanup;
        }
        if (method == 2 && (!isfinite(midpoint_x) || midpoint_x == x ||
                            midpoint_x == next_x)) {
            status = fail(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                          "RK4 midpoint coordinate is not distinct");
            goto cleanup;
        }
        status = call_derivative(function, data, x, state, dimension, k1,
                                 solution, error);
        if (status != NUMERIX_STATUS_SUCCESS) {
            goto cleanup;
        }
        if (method == 0) {
            for (component = 0; component < dimension; ++component) {
                next_state[component] = state[component] + step * k1[component];
            }
        } else if (method == 1) {
            for (component = 0; component < dimension; ++component) {
                temporary[component] = state[component] + step * k1[component];
            }
            status = call_derivative(function, data, next_x, temporary,
                                     dimension, k2, solution, error);
            if (status != NUMERIX_STATUS_SUCCESS) {
                goto cleanup;
            }
            for (component = 0; component < dimension; ++component) {
                next_state[component] = state[component] +
                    step * (k1[component] + k2[component]) * 0.5;
            }
        } else {
            for (component = 0; component < dimension; ++component) {
                temporary[component] = state[component] + half_step * k1[component];
            }
            status = call_derivative(function, data, midpoint_x, temporary,
                                     dimension, k2, solution, error);
            if (status != NUMERIX_STATUS_SUCCESS) {
                goto cleanup;
            }
            for (component = 0; component < dimension; ++component) {
                temporary[component] = state[component] + half_step * k2[component];
            }
            status = call_derivative(function, data, midpoint_x, temporary,
                                     dimension, k3, solution, error);
            if (status != NUMERIX_STATUS_SUCCESS) {
                goto cleanup;
            }
            for (component = 0; component < dimension; ++component) {
                temporary[component] = state[component] + step * k3[component];
            }
            status = call_derivative(function, data, next_x, temporary,
                                     dimension, k4, solution, error);
            if (status != NUMERIX_STATUS_SUCCESS) {
                goto cleanup;
            }
            for (component = 0; component < dimension; ++component) {
                next_state[component] = state[component] + step *
                    (k1[component] + 2.0 * k2[component] +
                     2.0 * k3[component] + k4[component]) / 6.0;
            }
        }
        for (component = 0; component < dimension; ++component) {
            if (!isfinite(next_state[component])) {
                status = fail(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                              "ODE state became non-finite");
                goto cleanup;
            }
        }
        solution->x[index + 1U] = next_x;
    }
    solution->status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(error);
    free(k1);
    free(k2);
    free(k3);
    free(k4);
    free(temporary);
    return NUMERIX_STATUS_SUCCESS;

cleanup:
    free(k1);
    free(k2);
    free(k3);
    free(k4);
    free(temporary);
    numerix_ode_solution_destroy(solution);
    solution->status = status;
    return status;
}

numerix_status numerix_ode_euler(numerix_ode_function function, void *data,
                                 double initial_x, const double *initial_state,
                                 size_t dimension, double step, size_t steps,
                                 numerix_ode_solution *solution,
                                 numerix_error *error)
{
    return solve(function, data, initial_x, initial_state, dimension, step,
                 steps, solution, error, 0);
}

numerix_status numerix_ode_heun(numerix_ode_function function, void *data,
                                double initial_x, const double *initial_state,
                                size_t dimension, double step, size_t steps,
                                numerix_ode_solution *solution,
                                numerix_error *error)
{
    return solve(function, data, initial_x, initial_state, dimension, step,
                 steps, solution, error, 1);
}

numerix_status numerix_ode_rk4(numerix_ode_function function, void *data,
                               double initial_x, const double *initial_state,
                               size_t dimension, double step, size_t steps,
                               numerix_ode_solution *solution,
                               numerix_error *error)
{
    return solve(function, data, initial_x, initial_state, dimension, step,
                 steps, solution, error, 2);
}
