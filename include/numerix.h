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

/* Phase 1: scalar root finding. result is written on success and failure. */
numerix_status numerix_root_bisection(numerix_function function, void *user_data,
    double lower, double upper, const numerix_options *options,
    numerix_result *result, numerix_error *error);
numerix_status numerix_root_regula_falsi(numerix_function function, void *user_data,
    double lower, double upper, const numerix_options *options,
    numerix_result *result, numerix_error *error);
numerix_status numerix_root_secant(numerix_function function, void *user_data,
    double x0, double x1, const numerix_options *options,
    numerix_result *result, numerix_error *error);
numerix_status numerix_root_newton(numerix_function function,
    numerix_function derivative, void *user_data, double initial,
    const numerix_options *options, numerix_result *result, numerix_error *error);
numerix_status numerix_root_safeguarded_newton(numerix_function function,
    numerix_function derivative, void *user_data, double lower, double upper,
    const numerix_options *options, numerix_result *result, numerix_error *error);
numerix_status numerix_root_brent(numerix_function function, void *user_data,
    double lower, double upper, const numerix_options *options,
    numerix_result *result, numerix_error *error);

/* Dynamically sized dense vector and row-major matrix types. */
typedef struct numerix_vector { size_t size; double *data; } numerix_vector;
typedef struct numerix_matrix { size_t rows; size_t columns; double *data; } numerix_matrix;
typedef struct numerix_lu { size_t size; double *data; size_t *permutation; int sign; double threshold; } numerix_lu;
numerix_status numerix_vector_create(size_t size, numerix_vector **out, numerix_error *error);
void numerix_vector_destroy(numerix_vector *vector);
numerix_status numerix_vector_copy(const numerix_vector *a, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_add(const numerix_vector *a, const numerix_vector *b, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_subtract(const numerix_vector *a, const numerix_vector *b, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_scale(const numerix_vector *a, double scalar, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_dot(const numerix_vector *a, const numerix_vector *b, double *out, numerix_error *error);
double numerix_vector_norm2(const numerix_vector *a);
double numerix_vector_norm_inf(const numerix_vector *a);
numerix_status numerix_matrix_create(size_t rows, size_t columns, numerix_matrix **out, numerix_error *error);
void numerix_matrix_destroy(numerix_matrix *matrix);
numerix_status numerix_matrix_copy(const numerix_matrix *a, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_add(const numerix_matrix *a, const numerix_matrix *b, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_subtract(const numerix_matrix *a, const numerix_matrix *b, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_scale(const numerix_matrix *a, double scalar, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_multiply(const numerix_matrix *a, const numerix_matrix *b, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_transpose(const numerix_matrix *a, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_vector_multiply(const numerix_matrix *a, const numerix_vector *x, numerix_vector **out, numerix_error *error);
numerix_status numerix_linear_solve_gaussian(const numerix_matrix *a, const numerix_vector *b, const numerix_options *options, numerix_vector **x, double *residual, numerix_error *error);
numerix_status numerix_linear_solve_gauss_jordan(const numerix_matrix *a, const numerix_vector *b, const numerix_options *options, numerix_vector **x, double *residual, numerix_error *error);
numerix_status numerix_lu_decompose(const numerix_matrix *a, const numerix_options *options, numerix_lu *lu, numerix_error *error);
void numerix_lu_destroy(numerix_lu *lu);
numerix_status numerix_lu_solve(const numerix_lu *lu, const numerix_vector *b, numerix_vector **x, numerix_error *error);

/* Numerical differentiation; h must be positive and finite. */
numerix_status numerix_diff_forward(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_backward(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_central(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_central_five_point(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_richardson(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_second(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);

/* Interpolation uses distinct finite x values. Extrapolation is opt-in. */
typedef struct numerix_interpolant { size_t size; double *x; double *coefficients; double minimum; double maximum; } numerix_interpolant;
numerix_status numerix_interp_linear(double x0, double y0, double x1, double y1, double x, numerix_result *r, numerix_error *e);
numerix_status numerix_interp_lagrange(const double *x, const double *y, size_t n, double query, int allow_extrapolation, numerix_result *r, numerix_error *e);
numerix_status numerix_interp_newton_create(const double *x, const double *y, size_t n, numerix_interpolant *p, numerix_error *e);
numerix_status numerix_interp_newton_evaluate(const numerix_interpolant *p, double query, int allow_extrapolation, numerix_result *r, numerix_error *e);
void numerix_interpolant_destroy(numerix_interpolant *p);

/* Finite-interval integration. Fixed rules use caller-selected panels. */
numerix_status numerix_integrate_trapezoid(numerix_function f, void *data, double a, double b, size_t panels, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_integrate_simpson(numerix_function f, void *data, double a, double b, size_t panels, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_integrate_adaptive_simpson(numerix_function f, void *data, double a, double b, const numerix_options *o, size_t max_depth, numerix_result *r, numerix_error *e);

/* Bounded one-dimensional minimization. */
numerix_status numerix_optimize_golden_section(numerix_function f, void *data, double lower, double upper, const numerix_options *o, numerix_result *r, numerix_error *e);
typedef double (*numerix_objective_function)(const double *x, size_t dimension, void *user_data);
typedef numerix_status (*numerix_gradient_function)(const double *x, size_t dimension, double *gradient, void *user_data);
numerix_status numerix_optimize_gradient_descent(numerix_objective_function objective, numerix_gradient_function gradient, void *data, const numerix_vector *initial, double initial_step, const numerix_options *o, numerix_vector **solution, numerix_result *result, numerix_error *e);

/* Vector ODE callback writes dimension derivatives into dydx. */
typedef numerix_status (*numerix_ode_function)(double x, const double *y, size_t dimension, double *dydx, void *user_data);
typedef struct numerix_ode_solution { numerix_status status; size_t points; size_t dimension; double *x; double *y; size_t function_evaluations; } numerix_ode_solution;
numerix_status numerix_ode_euler(numerix_ode_function f, void *data, double x0, const double *y0, size_t dimension, double step, size_t steps, numerix_ode_solution *out, numerix_error *e);
numerix_status numerix_ode_heun(numerix_ode_function f, void *data, double x0, const double *y0, size_t dimension, double step, size_t steps, numerix_ode_solution *out, numerix_error *e);
numerix_status numerix_ode_rk4(numerix_ode_function f, void *data, double x0, const double *y0, size_t dimension, double step, size_t steps, numerix_ode_solution *out, numerix_error *e);
void numerix_ode_solution_destroy(numerix_ode_solution *solution);

/*
 * Compiled expressions are immutable and may be evaluated repeatedly.
 * The caller owns an expression returned by compile and must destroy it.
 */
typedef struct numerix_expression numerix_expression;

numerix_status numerix_expression_compile(
    const char *source,
    numerix_expression **expression,
    numerix_error *error);

numerix_status numerix_expression_evaluate(
    const numerix_expression *expression,
    double x,
    double *value,
    numerix_error *error);

void numerix_expression_destroy(numerix_expression *expression);

/*
 * A callback context borrows its expression. Use one context per active
 * numerical call; it records the latest evaluation error for inspection.
 */
typedef struct numerix_expression_callback_context {
    const numerix_expression *expression;
    numerix_status last_status;
    numerix_error last_error;
} numerix_expression_callback_context;

numerix_status numerix_expression_callback_context_init(
    numerix_expression_callback_context *context,
    const numerix_expression *expression,
    numerix_error *error);

double numerix_expression_callback(
    double x,
    void *user_data);

numerix_status numerix_expression_callback_status(
    const numerix_expression_callback_context *context);

const numerix_error *numerix_expression_callback_error(
    const numerix_expression_callback_context *context);

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
