/* Reproducible API-level benchmarks. Problem definitions are in benchmarks/README.md. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "comparison.h"

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef NUMERIX_HAVE_GSL
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_min.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_roots.h>
#include <gsl/gsl_version.h>
#endif

#define MAX_SAMPLES 101U
#define PI 3.14159265358979323846264338327950288

typedef enum category {
    ROOT, LINEAR, DIFFERENTIATION, INTEGRATION, INTERPOLATION, OPTIMIZATION, ODE,
    CAPABILITY
} category;

typedef struct problem {
    category category;
    const char *name;
    int variant;
    int difficulty;
    double lower;
    double upper;
    double initial;
    double reference;
    double step;
    double scale;
    double absolute_tolerance;
    double relative_tolerance;
    size_t size;
} problem;

typedef struct observation {
    int status;
    double value;
    double absolute_error;
    double relative_error;
    double residual;
    double estimated_error;
    double backward_error;
    double trajectory_error;
    double objective;
    size_t evaluations;
    size_t derivatives;
    size_t iterations;
    int accuracy_pass;
    size_t direct_result_bytes;
    size_t direct_workspace_peak_bytes;
} observation;

typedef struct workload {
    const problem *problem;
    const char *method;
    const char *phase;
    const char *comparison;
    int gsl;
    numerix_options options;
    size_t evaluations;
    size_t derivatives;
    double last_x;
    double last_value;
    double *matrix_data;
    double *right_data;
    numerix_matrix matrix;
    numerix_vector right;
    numerix_lu lu;
    numerix_interpolant interpolant;
    numerix_expression *expression;
    double nodes[16];
    double values[16];
#ifdef NUMERIX_HAVE_GSL
    gsl_root_fsolver *root;
    gsl_root_fdfsolver *newton;
    gsl_min_fminimizer *minimum;
    gsl_integration_workspace *quadrature;
    gsl_interp *interpolation;
    gsl_matrix *gsl_lu;
    gsl_permutation *permutation;
    gsl_odeiv2_step *ode;
#endif
} workload;

static volatile double result_sink;
static size_t sample_count = 21U;
static int memory_only;
static const char *case_filter;
static const char *library_filter;

static double seconds_now(void)
{
#ifdef _WIN32
    LARGE_INTEGER count;
    LARGE_INTEGER frequency;

    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&count)) {
        fputs("benchmark timer failed\n", stderr);
        exit(2);
    }
    return (double)count.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec value;

    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (double)value.tv_sec + (double)value.tv_nsec * 1e-9;
#endif
}

static double scalar_value(const problem *p, double x)
{
    switch (p->category) {
        case ROOT:
            switch (p->variant) {
                case 0:
                    return x * x - 2.0;
                case 1:
                    return cos(x) - x;
                case 2:
                    return (x - 0.3) * (x - 0.3) * (x - 0.3);
                case 3:
                    return p->scale * (x * x - 2.0);
                case 4:
                    return (x - 1.0) * (x - 2.0) * (x - 4.0);
                case 5:
                    return x - 1e-9;
                default:
                    return x * x + 1.0;
            }
        case DIFFERENTIATION:
        case INTEGRATION:
            switch (p->variant) {
                case 0:
                    return x * x * x * x;
                case 1:
                    return sin(x);
                case 2:
                    return exp(x);
                case 3:
                    return 1.0 / (1.0 + 100.0 * x * x);
                case 4:
                    return sqrt(x);
                default:
                    return sin(40.0 * x);
            }
        case OPTIMIZATION:
            if (p->variant == 1) {
                return exp(x) - x;
            }
            return p->scale * (x - p->reference) * (x - p->reference);
        case CAPABILITY:
            if (p->variant == 11) {
                return x * x + 1.0;
            }
            if (p->variant == 12) {
                return (x - 1.0) * (x - 2.0) * (x - 4.0);
            }
            if (p->variant == 13) {
                return (x - 0.3) * (x - 0.3);
            }
            return x * x - 2.0;
        default:
            return x * x + 1.0;
    }
}

static double counted_function(double x, void *data)
{
    workload *w = data;

    ++w->evaluations;
    w->last_x = x;
    w->last_value = scalar_value(w->problem, x);
    return w->last_value;
}

#ifdef NUMERIX_HAVE_GSL
static double observed_function_value(workload *w, double x)
{
    if (x == w->last_x) {
        return w->last_value;
    }
    return counted_function(x, w);
}
#endif

static double counted_derivative(double x, void *data)
{
    workload *w = data;
    const problem *p = w->problem;

    ++w->derivatives;
    switch (p->variant) {
        case 1:
            return -sin(x) - 1.0;
        case 2:
            return 3.0 * (x - 0.3) * (x - 0.3);
        case 3:
            return p->scale * 2.0 * x;
        case 4:
            return (x - 2.0) * (x - 4.0) + (x - 1.0) * (x - 4.0) +
                   (x - 1.0) * (x - 2.0);
        case 5:
            return 1.0;
        default:
            return 2.0 * x;
    }
}

static numerix_status ode_function(double t, const double *state, size_t dimension,
                                   double *derivative, void *data)
{
    workload *w = data;

    (void)t;
    ++w->evaluations;
    if (dimension == 2U) {
        derivative[0] = state[1];
        derivative[1] = -state[0];
    } else {
        derivative[0] = w->problem->scale * state[0];
    }
    return NUMERIX_STATUS_SUCCESS;
}

#ifdef NUMERIX_HAVE_GSL
static void gsl_function_derivative(double x, void *data, double *value,
                                     double *derivative)
{
    *value = counted_function(x, data);
    *derivative = counted_derivative(x, data);
}

static int gsl_ode_function(double t, const double state[], double derivative[],
                            void *data)
{
    workload *w = data;
    size_t dimension = w->problem->variant == 2 ? 2U : 1U;

    return (int)ode_function(t, state, dimension, derivative, data);
}
#endif

static void observation_init(observation *o)
{
    memset(o, 0, sizeof(*o));
    o->value = NAN;
    o->absolute_error = NAN;
    o->relative_error = NAN;
    o->residual = NAN;
    o->estimated_error = NAN;
    o->backward_error = NAN;
    o->trajectory_error = NAN;
    o->objective = NAN;
    o->accuracy_pass = -1;
}

static int prepare_inputs(workload *w)
{
    const problem *p = w->problem;
    size_t row;
    size_t column;

    numerix_options_init(&w->options);
    w->options.absolute_tolerance = p->absolute_tolerance;
    w->options.relative_tolerance = p->relative_tolerance;
    w->options.maximum_iterations = 100000U;
    w->options.maximum_function_evaluations = 1000000U;
    if (p->category == ROOT || p->category == OPTIMIZATION) {
        w->options.maximum_iterations = 1000U;
    }
    if (p->category == LINEAR) {
        w->matrix_data = calloc(p->size * p->size, sizeof(double));
        w->right_data = calloc(p->size, sizeof(double));
        if (w->matrix_data == NULL || w->right_data == NULL) {
            return 0;
        }
        for (row = 0; row < p->size; ++row) {
            for (column = 0; column < p->size; ++column) {
                double element;

                if (p->variant == 1) {
                    element = p->scale / (double)(row + column + 1U);
                } else {
                    element = p->scale * (row == column ? 2.0 :
                              0.25 / (double)(row + column + 1U));
                }
                w->matrix_data[row * p->size + column] = element;
                w->right_data[row] += element; /* Known solution is all ones. */
            }
        }
        w->matrix.rows = p->size;
        w->matrix.columns = p->size;
        w->matrix.data = w->matrix_data;
        w->right.size = p->size;
        w->right.data = w->right_data;
    }
    if (p->category == INTERPOLATION) {
        for (row = 0; row < p->size; ++row) {
            double fraction = (double)row / (double)(p->size - 1U);

            if (p->variant == 1) {
                fraction *= fraction;
            }
            w->nodes[row] = fraction;
            w->values[row] = p->scale * (fraction * fraction * fraction -
                                       2.0 * fraction + 1.0);
        }
    }
    return 1;
}

static int prepare_library(workload *w)
{
    const problem *p = w->problem;
    numerix_error error;

    if (!w->gsl) {
        if (p->category == LINEAR && strcmp(w->phase, "solve") == 0) {
            return numerix_lu_decompose(&w->matrix, &w->options, &w->lu, &error) == 0;
        }
        if (p->category == INTERPOLATION && strcmp(w->method, "newton") == 0 &&
            strcmp(w->phase, "evaluate") == 0) {
            return numerix_interp_newton_create(w->nodes, w->values, p->size,
                                               &w->interpolant, &error) == 0;
        }
        if (p->category == CAPABILITY && strcmp(w->method, "expression") == 0) {
            return numerix_expression_compile("sin(x)+x^2", &w->expression, &error) == 0;
        }
        return 1;
    }
#ifdef NUMERIX_HAVE_GSL
    switch (p->category) {
        case ROOT:
            if (strcmp(w->method, "newton") == 0) {
                w->newton = gsl_root_fdfsolver_alloc(gsl_root_fdfsolver_newton);
                return w->newton != NULL;
            }
            w->root = gsl_root_fsolver_alloc(strcmp(w->method, "bisection") == 0 ?
                      gsl_root_fsolver_bisection : gsl_root_fsolver_brent);
            return w->root != NULL;
        case LINEAR:
            if (strcmp(w->phase, "solve") == 0) {
                int sign;
                gsl_matrix_const_view input = gsl_matrix_const_view_array(
                    w->matrix_data, p->size, p->size);

                w->gsl_lu = gsl_matrix_alloc(p->size, p->size);
                w->permutation = gsl_permutation_alloc(p->size);
                if (w->gsl_lu == NULL || w->permutation == NULL) {
                    return 0;
                }
                gsl_matrix_memcpy(w->gsl_lu, &input.matrix);
                return gsl_linalg_LU_decomp(w->gsl_lu, w->permutation, &sign) == 0;
            }
            return 1;
        case INTEGRATION:
            w->quadrature = gsl_integration_workspace_alloc(10000U);
            return w->quadrature != NULL;
        case INTERPOLATION:
            if (strcmp(w->phase, "evaluate") == 0) {
                w->interpolation = gsl_interp_alloc(strcmp(w->method, "linear") == 0 ?
                                   gsl_interp_linear : gsl_interp_polynomial, p->size);
                return w->interpolation != NULL && gsl_interp_init(
                       w->interpolation, w->nodes, w->values, p->size) == 0;
            }
            return 1;
        case OPTIMIZATION:
            w->minimum = gsl_min_fminimizer_alloc(gsl_min_fminimizer_goldensection);
            return w->minimum != NULL;
        case ODE:
            w->ode = gsl_odeiv2_step_alloc(gsl_odeiv2_step_rk4,
                                          p->variant == 2 ? 2U : 1U);
            return w->ode != NULL;
        default:
            return 1;
    }
#else
    return 0;
#endif
}

static void destroy_library(workload *w)
{
    numerix_lu_destroy(&w->lu);
    numerix_interpolant_destroy(&w->interpolant);
    numerix_expression_destroy(w->expression);
    w->expression = NULL;
#ifdef NUMERIX_HAVE_GSL
    if (w->root != NULL) {
        gsl_root_fsolver_free(w->root);
    }
    if (w->newton != NULL) {
        gsl_root_fdfsolver_free(w->newton);
    }
    if (w->minimum != NULL) {
        gsl_min_fminimizer_free(w->minimum);
    }
    if (w->quadrature != NULL) {
        gsl_integration_workspace_free(w->quadrature);
    }
    if (w->interpolation != NULL) {
        gsl_interp_free(w->interpolation);
    }
    if (w->gsl_lu != NULL) {
        gsl_matrix_free(w->gsl_lu);
    }
    if (w->permutation != NULL) {
        gsl_permutation_free(w->permutation);
    }
    if (w->ode != NULL) {
        gsl_odeiv2_step_free(w->ode);
    }
#endif
}

static void root_operation(workload *w, observation *o)
{
    const problem *p = w->problem;
    numerix_result result;
    numerix_error error;

    if (!w->gsl) {
        if (strcmp(w->method, "newton") == 0) {
            o->status = (int)numerix_root_newton(counted_function, counted_derivative,
                w, p->initial, &w->options, &result, &error);
        } else if (strcmp(w->method, "bisection") == 0) {
            o->status = (int)numerix_root_bisection(counted_function, w, p->lower,
                p->upper, &w->options, &result, &error);
        } else {
            o->status = (int)numerix_root_brent(counted_function, w, p->lower,
                p->upper, &w->options, &result, &error);
        }
        o->value = result.value;
        o->iterations = result.iterations;
        o->estimated_error = result.estimated_error;
        return;
    }
#ifdef NUMERIX_HAVE_GSL
    {
        gsl_function function = {counted_function, w};
        gsl_function_fdf derivative = {counted_function, counted_derivative,
                                       gsl_function_derivative, w};
        size_t iteration;
        double value = p->initial;
        double previous;
        double previous_step = INFINITY;
        int is_newton = strcmp(w->method, "newton") == 0;

        if (is_newton) {
            o->status = gsl_root_fdfsolver_set(w->newton, &derivative, value);
        } else {
            o->status = gsl_root_fsolver_set(w->root, &function, p->lower, p->upper);
        }
        if (o->status != GSL_SUCCESS) {
            return;
        }
        /* Phase 10: a nonzero residual is not a position tolerance. */
        if (is_newton && observed_function_value(w, value) == 0.0) {
            o->value = value;
            return;
        }
        for (iteration = 0; iteration < 1000U; ++iteration) {
            previous = value;
            o->status = is_newton ? gsl_root_fdfsolver_iterate(w->newton) :
                                   gsl_root_fsolver_iterate(w->root);
            ++o->iterations;
            value = is_newton ? gsl_root_fdfsolver_root(w->newton) :
                               gsl_root_fsolver_root(w->root);
            if (o->status != GSL_SUCCESS) {
                break;
            }
            if (is_newton)
            {
                double step = fabs(value - previous);
                double ratio = step / previous_step;

                o->estimated_error = isfinite(previous_step) && ratio < 1.0
                    ? fmax(step, step * ratio / (1.0 - ratio)) : INFINITY;
                if (observed_function_value(w, value) == 0.0 ||
                    (step != 0.0 && o->estimated_error <= p->absolute_tolerance +
                     p->relative_tolerance * fabs(value)))
                {
                    break;
                }
                if (step == 0.0)
                {
                    o->status = GSL_ENOPROG;
                    break;
                }
                previous_step = step;
            }
            else
            {
                double lower = gsl_root_fsolver_x_lower(w->root);
                double upper = gsl_root_fsolver_x_upper(w->root);

                o->estimated_error = fmax(value - lower, upper - value);
                if (o->estimated_error <= p->absolute_tolerance +
                    p->relative_tolerance * fabs(value) ||
                    (w->last_x == value && w->last_value == 0.0))
                {
                    break;
                }
            }
        }
        if (iteration == 1000U) {
            o->status = GSL_EMAXITER;
        }
        o->value = value;
    }
#endif
}

static void linear_accuracy(workload *w, const double *solution, observation *o)
{
    size_t row;
    size_t column;
    double matrix_norm = 0.0;
    double right_norm = 0.0;
    double solution_norm = 0.0;
    size_t n = w->problem->size;

    o->absolute_error = 0.0;
    o->residual = 0.0;
    for (row = 0; row < n; ++row) {
        long double residual = -(long double)w->right_data[row];
        double row_norm = 0.0;

        for (column = 0; column < n; ++column) {
            residual += (long double)w->matrix_data[row * n + column] *
                        (long double)solution[column];
            row_norm += fabs(w->matrix_data[row * n + column]);
        }
        o->residual = fmax(o->residual, (double)fabsl(residual));
        o->absolute_error = fmax(o->absolute_error, fabs(solution[row] - 1.0));
        matrix_norm = fmax(matrix_norm, row_norm);
        right_norm = fmax(right_norm, fabs(w->right_data[row]));
        solution_norm = fmax(solution_norm, fabs(solution[row]));
    }
    o->relative_error = o->absolute_error;
    o->backward_error = o->residual / (matrix_norm * solution_norm + right_norm);
}

static void linear_operation(workload *w, observation *o, int accuracy)
{
    int factor = strcmp(w->phase, "solve") != 0;
    int solve = strcmp(w->phase, "factor") != 0;
    numerix_error error;
    size_t n = w->problem->size;

    if (!w->gsl) {
        numerix_lu local = {0};
        numerix_vector *solution = NULL;

        if (factor) {
            o->status = (int)numerix_lu_decompose(&w->matrix, &w->options,
                                                &local, &error);
        }
        if (solve && o->status == 0) {
            o->status = (int)numerix_lu_solve(factor ? &local : &w->lu,
                                             &w->right, &solution, &error);
        }
        if (solution != NULL) {
            o->value = solution->data[0];
            if (accuracy) {
                linear_accuracy(w, solution->data, o);
            }
        }
        numerix_vector_destroy(solution);
        numerix_lu_destroy(&local);
        return;
    }
#ifdef NUMERIX_HAVE_GSL
    {
        gsl_matrix *matrix = w->gsl_lu;
        gsl_permutation *permutation = w->permutation;
        gsl_vector *solution = NULL;
        gsl_vector_const_view right = gsl_vector_const_view_array(w->right_data, n);
        int sign;

        if (factor) {
            gsl_matrix_const_view input = gsl_matrix_const_view_array(w->matrix_data, n, n);

            matrix = gsl_matrix_alloc(n, n);
            permutation = gsl_permutation_alloc(n);
            if (matrix == NULL || permutation == NULL) {
                o->status = GSL_ENOMEM;
            } else {
                gsl_matrix_memcpy(matrix, &input.matrix);
                o->status = gsl_linalg_LU_decomp(matrix, permutation, &sign);
            }
        }
        if (solve && o->status == 0) {
            solution = gsl_vector_alloc(n);
            o->status = solution == NULL ? GSL_ENOMEM :
                       gsl_linalg_LU_solve(matrix, permutation, &right.vector, solution);
        }
        if (solution != NULL) {
            o->value = solution->data[0];
            if (accuracy && o->status == 0) {
                linear_accuracy(w, solution->data, o);
            }
            gsl_vector_free(solution);
        }
        if (factor) {
            if (matrix != NULL) {
                gsl_matrix_free(matrix);
            }
            if (permutation != NULL) {
                gsl_permutation_free(permutation);
            }
        }
    }
#else
    (void)n;
#endif
}

static void scalar_operation(workload *w, observation *o)
{
    const problem *p = w->problem;
    numerix_result result;
    numerix_error error;

    if (!w->gsl) {
        if (p->category == DIFFERENTIATION) {
            o->status = (int)numerix_diff_richardson(counted_function, w,
                p->initial, p->step, &w->options, &result, &error);
        } else if (strcmp(w->method, "trapezoid") == 0) {
            o->status = (int)numerix_integrate_trapezoid(counted_function, w,
                p->lower, p->upper, p->size, &w->options, &result, &error);
        } else if (strcmp(w->method, "simpson") == 0) {
            o->status = (int)numerix_integrate_simpson(counted_function, w,
                p->lower, p->upper, p->size, &w->options, &result, &error);
        } else {
            o->status = (int)numerix_integrate_adaptive_simpson(counted_function,
                w, p->lower, p->upper, &w->options, 32U, &result, &error);
        }
        o->value = result.value;
        o->estimated_error = result.estimated_error;
        o->iterations = result.iterations;
        return;
    }
#ifdef NUMERIX_HAVE_GSL
    {
        gsl_function function = {counted_function, w};

        if (p->category == DIFFERENTIATION) {
            o->status = gsl_deriv_central(&function, p->initial, p->step,
                                          &o->value, &o->estimated_error);
        } else {
            o->status = gsl_integration_qag(&function, p->lower, p->upper,
                p->absolute_tolerance, p->relative_tolerance, 10000U,
                GSL_INTEG_GAUSS21, w->quadrature, &o->value, &o->estimated_error);
            o->iterations = w->quadrature->size;
        }
    }
#endif
}

static void interpolation_operation(workload *w, observation *o)
{
    const problem *p = w->problem;
    int construct = strcmp(w->phase, "construct") == 0;
    numerix_error error;
    numerix_result result;

    if (!w->gsl) {
        if (construct) {
            numerix_interpolant interpolant = {0};

            o->status = (int)numerix_interp_newton_create(w->nodes, w->values,
                         p->size, &interpolant, &error);
            numerix_interpolant_destroy(&interpolant);
            return;
        }
        if (strcmp(w->method, "linear") == 0) {
            o->status = (int)numerix_interp_linear(w->nodes[0], w->values[0],
                w->nodes[1], w->values[1], p->initial, &result, &error);
        } else if (strcmp(w->method, "lagrange") == 0) {
            o->status = (int)numerix_interp_lagrange(w->nodes, w->values, p->size,
                                                   p->initial, 0, &result, &error);
        } else {
            o->status = (int)numerix_interp_newton_evaluate(&w->interpolant,
                                         p->initial, 0, &result, &error);
        }
        o->value = result.value;
        return;
    }
#ifdef NUMERIX_HAVE_GSL
    if (construct) {
        gsl_interp *interpolation = gsl_interp_alloc(gsl_interp_polynomial, p->size);

        o->status = interpolation == NULL ? GSL_ENOMEM : gsl_interp_init(
            interpolation, w->nodes, w->values, p->size);
        if (interpolation != NULL) {
            gsl_interp_free(interpolation);
        }
    } else {
        o->status = gsl_interp_eval_e(w->interpolation, w->nodes, w->values,
                                      p->initial, NULL, &o->value);
    }
#endif
}

static void optimization_operation(workload *w, observation *o)
{
    const problem *p = w->problem;
    numerix_result result;
    numerix_error error;

    if (!w->gsl) {
        o->status = (int)numerix_optimize_golden_section(counted_function, w,
                       p->lower, p->upper, &w->options, &result, &error);
        o->value = result.value;
        o->objective = result.residual;
        o->iterations = result.iterations;
        o->estimated_error = result.estimated_error;
        return;
    }
#ifdef NUMERIX_HAVE_GSL
    {
        gsl_function function = {counted_function, w};
        size_t iteration;

        o->status = gsl_min_fminimizer_set(w->minimum, &function, p->initial,
                                          p->lower, p->upper);
        for (iteration = 0; o->status == GSL_SUCCESS && iteration < 1000U; ++iteration) {
            double lower = gsl_min_fminimizer_x_lower(w->minimum);
            double upper = gsl_min_fminimizer_x_upper(w->minimum);

            if (upper - lower <= p->absolute_tolerance + p->relative_tolerance *
                                fmax(fabs(lower), fabs(upper))) {
                break;
            }
            o->status = gsl_min_fminimizer_iterate(w->minimum);
            ++o->iterations;
        }
        if (iteration == 1000U) {
            o->status = GSL_EMAXITER;
        }
        o->value = gsl_min_fminimizer_x_minimum(w->minimum);
        o->objective = gsl_min_fminimizer_f_minimum(w->minimum);
        o->estimated_error = (gsl_min_fminimizer_x_upper(w->minimum) -
                              gsl_min_fminimizer_x_lower(w->minimum)) * 0.5;
    }
#endif
}

static void ode_accuracy(const problem *p, const double *times, const double *states,
                         size_t dimension, observation *o)
{
    size_t index;
    size_t component;

    o->trajectory_error = 0.0;
    for (index = 0; index <= p->size; ++index) {
        double local_error = 0.0;

        for (component = 0; component < dimension; ++component) {
            double reference = dimension == 2U ?
                (component == 0 ? cos(times[index]) : -sin(times[index])) :
                exp(p->scale * times[index]);

            local_error = fmax(local_error,
                fabs(states[index * dimension + component] - reference));
        }
        o->trajectory_error = fmax(o->trajectory_error, local_error);
        if (index == p->size) {
            o->absolute_error = local_error;
        }
    }
    o->relative_error = o->absolute_error / fabs(p->reference);
}

static void ode_operation(workload *w, observation *o, int accuracy)
{
    const problem *p = w->problem;
    size_t dimension = p->variant == 2 ? 2U : 1U;
    double initial[2] = {1.0, 0.0};
    numerix_ode_solution solution = {0};
    numerix_error error;

    if (!w->gsl) {
        if (strcmp(w->method, "euler") == 0) {
            o->status = (int)numerix_ode_euler(ode_function, w, 0.0, initial,
                dimension, p->step, p->size, &solution, &error);
        } else if (strcmp(w->method, "heun") == 0) {
            o->status = (int)numerix_ode_heun(ode_function, w, 0.0, initial,
                dimension, p->step, p->size, &solution, &error);
        } else {
            o->status = (int)numerix_ode_rk4(ode_function, w, 0.0, initial,
                dimension, p->step, p->size, &solution, &error);
        }
        if (o->status == 0) {
            o->value = solution.y[p->size * dimension];
            o->iterations = p->size;
            if (accuracy) {
                ode_accuracy(p, solution.x, solution.y, dimension, o);
            }
        }
        numerix_ode_solution_destroy(&solution);
        return;
    }
#ifdef NUMERIX_HAVE_GSL
    {
        gsl_odeiv2_system system = {gsl_ode_function, NULL, dimension, w};
        double state[2] = {1.0, 0.0};
        double error_estimate[2];
        double *times = malloc((p->size + 1U) * sizeof(double));
        double *states = malloc((p->size + 1U) * dimension * sizeof(double));
        double t = 0.0;
        size_t index;

        if (times == NULL || states == NULL) {
            o->status = GSL_ENOMEM;
        } else {
            gsl_odeiv2_step_reset(w->ode);
            times[0] = 0.0;
            memcpy(states, state, dimension * sizeof(double));
            for (index = 0; index < p->size; ++index) {
                o->status = gsl_odeiv2_step_apply(w->ode, t, p->step, state,
                                                 error_estimate, NULL, NULL, &system);
                if (o->status != GSL_SUCCESS) {
                    break;
                }
                t += p->step;
                times[index + 1U] = t;
                memcpy(states + (index + 1U) * dimension, state,
                       dimension * sizeof(double));
                ++o->iterations;
            }
            o->value = state[0];
            if (accuracy && o->status == 0) {
                ode_accuracy(p, times, states, dimension, o);
            }
        }
        free(times);
        free(states);
    }
#endif
}

static void capability_operation(workload *w, observation *o)
{
    numerix_error error;
    numerix_recommendation_request request;
    numerix_execution_output output;
    numerix_diagnostics_report diagnostics;
    numerix_managed_execution *managed = NULL;
    numerix_execution_summary summary;

    if (strcmp(w->method, "expression") == 0) {
        o->status = (int)numerix_expression_evaluate(w->expression, 0.7,
                                                    &o->value, &error);
        return;
    }
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.function = counted_function;
    request.user_data = w;
    request.options = &w->options;
    request.problem.root.lower = w->problem->lower;
    request.problem.root.upper = w->problem->upper;
    request.problem.root.has_bracket = 1;
    if (strcmp(w->method, "discovery") == 0) {
        numerix_root_discovery_options options;
        numerix_root_search_report report;
        size_t index;

        numerix_root_discovery_options_init(&options);
        options.initial_panels = w->problem->size;
        o->status = (int)numerix_root_find_all(&request, w->problem->lower,
            w->problem->upper, &options, &report, &error);
        o->value = (double)report.root_count;
        o->iterations = report.refinement_iterations;
        o->direct_result_bytes = report.result_storage_bytes;
        o->direct_workspace_peak_bytes = report.workspace_peak_bytes;
        o->residual = 0.0;
        for (index = 0; index < report.root_count; ++index) {
            o->residual = fmax(o->residual, fabs(report.roots[index].result.residual));
        }
        numerix_root_search_report_destroy(&report);
        return;
    }
    if (strcmp(w->method, "managed") == 0) {
        o->status = (int)numerix_managed_execution_create(&request, NULL, &managed, &error);
        if (o->status == 0) {
            o->status = (int)numerix_managed_execution_run(managed, &error);
        }
        if (o->status == 0) {
            o->status = (int)numerix_managed_execution_get_summary(managed, &summary, &error);
            o->value = summary.result.value;
            o->iterations = summary.iterations;
        }
        numerix_managed_execution_destroy(managed);
        return;
    }
    o->status = (int)numerix_execute_recommended(&request, NULL, &output, &error);
    if (o->status == 0) {
        o->value = output.scalar_result.value;
        o->iterations = output.scalar_result.iterations;
        if (strcmp(w->method, "diagnostics") == 0) {
            o->status = (int)numerix_diagnostics_analyze(&request, &output, NULL,
                                                       &diagnostics, &error);
            if (o->status == 0) {
                o->residual = diagnostics.absolute_residual;
                numerix_diagnostics_report_destroy(&diagnostics);
            }
        }
    }
    numerix_execution_output_destroy(&output);
}

static void operation(workload *w, observation *o, int accuracy)
{
    observation_init(o);
    w->evaluations = 0;
    w->derivatives = 0;
    w->last_x = NAN;
    w->last_value = NAN;
    switch (w->problem->category) {
        case ROOT:
            root_operation(w, o);
            break;
        case LINEAR:
            linear_operation(w, o, accuracy);
            break;
        case DIFFERENTIATION:
        case INTEGRATION:
            scalar_operation(w, o);
            break;
        case INTERPOLATION:
            interpolation_operation(w, o);
            break;
        case OPTIMIZATION:
            optimization_operation(w, o);
            break;
        case ODE:
            ode_operation(w, o, accuracy);
            break;
        case CAPABILITY:
            capability_operation(w, o);
            break;
    }
    o->evaluations = w->evaluations;
    o->derivatives = w->derivatives;
    if (isfinite(o->value)) {
        result_sink = o->value;
    }
}

static void assess_accuracy(workload *w, observation *o)
{
    const problem *p = w->problem;
    double threshold = p->absolute_tolerance + p->relative_tolerance * fabs(p->reference);

    if (o->status != 0) {
        o->accuracy_pass = 0;
        return;
    }
    if (strcmp(w->phase, "factor") == 0 || strcmp(w->phase, "construct") == 0) {
        return;
    }
    if (!isfinite(o->absolute_error)) {
        o->absolute_error = fabs(o->value - p->reference);
        if (p->reference != 0.0) {
            o->relative_error = o->absolute_error / fabs(p->reference);
        }
    }
    if (p->category == ROOT) {
        o->residual = fabs(scalar_value(p, o->value));
    }
    if (p->category == LINEAR) {
        threshold = 1e-8;
    } else if (p->category == DIFFERENTIATION) {
        threshold = 1e-7 * fmax(1.0, fabs(p->reference));
    } else if (p->category == ODE) {
        threshold = strcmp(w->method, "euler") == 0 ? 0.02 :
                    strcmp(w->method, "heun") == 0 ? 1e-4 : 1e-7;
    }
    o->accuracy_pass = isfinite(o->absolute_error) && o->absolute_error <= threshold;
}

static int compare_double(const void *left, const void *right)
{
    double a = *(const double *)left;
    double b = *(const double *)right;

    return (a > b) - (a < b);
}

static double time_batch(workload *w, size_t batch)
{
    size_t index;
    observation o;
    double start = seconds_now();

    for (index = 0; index < batch; ++index) {
        operation(w, &o, 0);
    }
    return seconds_now() - start;
}

static void emit_workload(workload *w)
{
    static const char *categories[] = {"root", "linear", "differentiation", "integration",
        "interpolation", "optimization", "ode", "capability"};
    double samples[MAX_SAMPLES] = {0.0};
    double mean = NAN;
    double standard_deviation = NAN;
    double minimum = NAN;
    double median = NAN;
    double percentile = NAN;
    size_t batch = 1U;
    size_t index;
    size_t successes = 0;
    size_t accuracy_successes = 0;
    size_t trials = 0;
    observation o;
    benchmark_memory memory = {0};
    int memory_available = 0;
    size_t setup_live_bytes = 0;
    const char *library = w->gsl ? "GSL" : "NUMERIX";

    if ((case_filter != NULL && strcmp(case_filter, w->problem->name) != 0) ||
        (library_filter != NULL && strcmp(library_filter, library) != 0)) {
        return;
    }
    if (!prepare_inputs(w)) {
        fputs("benchmark input allocation failed\n", stderr);
        exit(2);
    }
#ifdef NUMERIX_BENCH_ALLOCATIONS
    benchmark_memory_begin();
    memory_available = 1;
#endif
    if (!prepare_library(w)) {
        observation_init(&o);
        o.status = -100;
        fputs("benchmark library setup failed\n", stderr);
    } else {
#ifdef NUMERIX_BENCH_ALLOCATIONS
        setup_live_bytes = benchmark_memory_snapshot().live_bytes;
#endif
        operation(w, &o, 1);
        assess_accuracy(w, &o);
        if (!memory_only) {
            /* Warm-up and powers-of-two batching until >=2 ms per sample. */
            while (time_batch(w, batch) < 0.002 && batch < 1048576U) {
                batch *= 2U;
            }
            for (index = 0; index < sample_count; ++index) {
                samples[index] = time_batch(w, batch) * 1e9 / (double)batch;
                operation(w, &o, 1);
                assess_accuracy(w, &o);
                successes += o.status == 0 ? 1U : 0U;
                accuracy_successes += o.accuracy_pass == 1 ? 1U : 0U;
            }
            trials = sample_count;
            mean = 0.0;
            for (index = 0; index < sample_count; ++index) {
                mean += samples[index] / (double)sample_count;
            }
            standard_deviation = 0.0;
            for (index = 0; index < sample_count; ++index) {
                double difference = samples[index] - mean;

                standard_deviation += difference * difference;
            }
            standard_deviation = sqrt(standard_deviation / (double)(sample_count - 1U));
            qsort(samples, sample_count, sizeof(double), compare_double);
            minimum = samples[0];
            median = samples[sample_count / 2U];
            percentile = samples[(size_t)ceil(0.95 * (double)sample_count) - 1U];
        }
    }
    destroy_library(w);
#ifdef NUMERIX_BENCH_ALLOCATIONS
    memory = benchmark_memory_end();
    if (memory.overflow) {
        fputs("allocation tracker capacity or size overflow\n", stderr);
        exit(2);
    }
#endif
    printf("%s,%s,%d,%zu,%s,%s,%s,%s,%d,%d,%zu,%zu,%zu,%zu,"
           "%.9g,%.9g,%.9g,%.9g,%.9g,%.17g,%.17g,%.17g,%.17g,%.17g,"
           "%.17g,%.17g,%.17g,%zu,%zu,%zu,%d,%.17g,%.17g,%.17g,"
           "%d,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",
           categories[w->problem->category], w->problem->name, w->problem->difficulty,
           w->problem->size, w->method, w->phase, library, w->comparison,
           o.status, o.accuracy_pass, trials, successes, accuracy_successes, batch,
           minimum, median, mean, standard_deviation, percentile,
           o.value, w->problem->reference, o.absolute_error, o.relative_error,
           o.residual, o.estimated_error, o.backward_error, o.trajectory_error,
           o.evaluations, o.derivatives, o.iterations, (int)(sizeof(double) * 8U),
           w->problem->absolute_tolerance, w->problem->relative_tolerance, o.objective,
           memory_available, memory.allocations, memory.allocated_bytes,
           memory.peak_bytes, memory.live_bytes, setup_live_bytes,
           o.direct_result_bytes, o.direct_workspace_peak_bytes);
    free(w->matrix_data);
    free(w->right_data);
}

static void run_case(const problem *p, const char *method, const char *phase,
                     const char *comparison, int include_gsl)
{
    workload w;
    int library;
    int library_count = 1;

#ifdef NUMERIX_HAVE_GSL
    library_count += include_gsl;
#else
    (void)include_gsl;
#endif
    for (library = 0; library < library_count; ++library) {
        memset(&w, 0, sizeof(w));
        w.problem = p;
        w.method = method;
        w.phase = phase;
        w.comparison = comparison;
        w.gsl = library;
        emit_workload(&w);
    }
}

static void run_suite(void)
{
    size_t index;
    size_t dimension;
    int variant;
    problem p;
    problem roots[] = {
        {ROOT, "polynomial", 0, 1, 0, 2, 1, 1.4142135623730950488, 0, 1, 1e-10, 1e-10, 0},
        {ROOT, "cosine", 1, 2, 0, 1, 0.5, 0.7390851332151606417, 0, 1, 1e-10, 1e-10, 0},
        {ROOT, "flat_cubic", 2, 3, 0, 1, 0.8, 0.3, 0, 1, 1e-10, 1e-10, 0},
        {ROOT, "small_scale", 3, 4, 0, 2, 1, 1.4142135623730950488, 0, 1e-14, 1e-10, 1e-10, 0},
        {ROOT, "large_scale", 3, 4, 0, 2, 1, 1.4142135623730950488, 0, 1e14, 1e-10, 1e-10, 0},
        {ROOT, "multiple_bracket", 4, 2, 1.5, 2.5, 1.8, 2, 0, 1, 1e-10, 1e-10, 0},
        {ROOT, "near_endpoint", 5, 3, 0, 1, 0.5, 1e-9, 0, 1, 1e-12, 0, 0},
        {ROOT, "no_bracket", 6, 3, -1, 1, 0, NAN, 0, 1, 1e-10, 1e-10, 0}
    };

    for (index = 0; index < sizeof(roots) / sizeof(roots[0]); ++index) {
        run_case(&roots[index], "bisection", "execute", "equivalent_family", 1);
        run_case(&roots[index], "brent", "execute", "different_safeguards", 1);
        if (roots[index].variant != 6) {
            run_case(&roots[index], "newton", "execute", "equivalent_family", 1);
        }
    }
    for (dimension = 8U; dimension <= 128U; dimension *= dimension == 8U ? 4U : 2U) {
        const double scales[] = {1.0, 1e-12, 1e12};
        const char *names[] = {"dense_well", "dense_small", "dense_large"};

        for (index = 0; index < 3U; ++index) {
            p = (problem){LINEAR, names[index], 0, dimension == 128U ? 4 : 1,
                          0, 0, 0, 1, 0, scales[index], 1e-10, 1e-10, dimension};
            run_case(&p, "pivoted_lu", "factor", "api_equivalent", 1);
            run_case(&p, "pivoted_lu", "solve", "api_equivalent", 1);
            run_case(&p, "pivoted_lu", "total", "api_equivalent", 1);
        }
        p = (problem){LINEAR, "hilbert", 1, 4, 0, 0, 0, 1, 0, 1, 1e-10, 1e-10, dimension};
        /* Rejected NUMERIX factorization is retained; solve setup cannot succeed. */
        run_case(&p, "pivoted_lu", "factor", "ill_conditioned_unranked", 1);
        run_case(&p, "pivoted_lu", "total", "ill_conditioned_unranked", 1);
    }
    for (variant = 0; variant < 4; ++variant) {
        const char *names[] = {"quartic_derivative", "sine_derivative", "exp_derivative",
                               "rational_derivative"};
        double references[] = {4.0 * 0.7 * 0.7 * 0.7, cos(0.7), exp(0.7),
                                -140.0 / (50.0 * 50.0)};

        p = (problem){DIFFERENTIATION, names[variant], variant, 2, 0, 0, 0.7,
                      references[variant], 1e-3, 1, 1e-10, 1e-10, 0};
        run_case(&p, "richardson_vs_adaptive", "execute", "different_algorithm", 1);
    }
    for (variant = 0; variant < 6; ++variant) {
        const char *names[] = {"quartic_integral", "sine_integral", "exp_integral",
                               "curved_integral", "endpoint_sqrt", "oscillatory"};
        double references[] = {0.2, 2.0, 1.7182818284590452354,
                                atan(10.0) / 10.0, 2.0 / 3.0, (1.0 - cos(40.0)) / 40.0};

        p = (problem){INTEGRATION, names[variant], variant, variant >= 3 ? 3 : 1,
                      0, variant == 1 ? PI : 1, 0, references[variant], 0, 1,
                      1e-10, 1e-10, 1024U};
        run_case(&p, "adaptive_simpson_vs_qag21", "execute", "different_algorithm", 1);
        run_case(&p, "trapezoid", "execute", "numerix_only", 0);
        run_case(&p, "simpson", "execute", "numerix_only", 0);
    }
    for (variant = 0; variant < 3; ++variant) {
        const char *names[] = {"uniform_cubic", "nonuniform_cubic", "scaled_cubic"};

        p = (problem){INTERPOLATION, names[variant], variant == 1 ? 1 : 0, 2,
                      0, 1, 0.37, 0, 0, variant == 2 ? 1e10 : 1,
                      1e-10, 1e-10, 8U};
        p.reference = p.scale * (0.37 * 0.37 * 0.37 - 2.0 * 0.37 + 1.0);
        run_case(&p, "newton", "construct", "equivalent_representation", 1);
        run_case(&p, "newton", "evaluate", "equivalent_representation", 1);
        run_case(&p, "lagrange", "evaluate", "different_representation", 1);
    }
    p = (problem){INTERPOLATION, "linear_segment", 0, 1, 0, 1, 0.37, 0.63,
                  0, 1, 1e-10, 1e-10, 2U};
    run_case(&p, "linear", "evaluate", "api_equivalent", 1);
    for (variant = 0; variant < 4; ++variant) {
        const char *names[] = {"convex_minimum", "nonlinear_minimum", "shifted_minimum",
                               "scaled_minimum"};
        double optimum[] = {0.3, 0, 1.7, 0.3};

        p = (problem){OPTIMIZATION, names[variant], variant == 1 ? 1 : 0,
                      variant == 3 ? 3 : 1, variant == 1 ? -1 : 0, 2,
                      variant == 2 ? 1.5 : 0.5, optimum[variant], 0,
                      variant == 3 ? 1e12 : 1, 1e-6, 0, 0};
        run_case(&p, "golden_section", "execute", "equivalent_family", 1);
    }
    for (variant = 0; variant < 3; ++variant) {
        const char *names[] = {"exponential_growth", "exponential_decay", "harmonic"};

        p = (problem){ODE, names[variant], variant, 2, 0, 1, 0,
                      variant == 2 ? cos(1.0) : exp(variant == 1 ? -2.0 : 1.0),
                      0.01, variant == 1 ? -2.0 : 1.0, 1e-10, 1e-10, 100U};
        run_case(&p, "rk4_vs_step_doubling", "execute", "different_work", 1);
        run_case(&p, "euler", "execute", "numerix_only", 0);
        run_case(&p, "heun", "execute", "numerix_only", 0);
    }
    p = (problem){CAPABILITY, "expression_evaluation", 0, 1, 0, 0, 0,
                  sin(0.7) + 0.49, 0, 1, 1e-10, 1e-10, 0};
    run_case(&p, "expression", "evaluate", "numerix_only", 0);
    /* Capability root callbacks use the same polynomial as the direct API. */
    p = roots[0];
    p.category = CAPABILITY;
    p.name = "automatic_execution";
    p.reference = sqrt(2.0);
    run_case(&p, "automatic", "execute", "numerix_only", 0);
    p.name = "managed_execution";
    run_case(&p, "managed", "execute", "numerix_only", 0);
    p.name = "diagnostic_execution";
    run_case(&p, "diagnostics", "execute", "numerix_only", 0);
    for (dimension = 16U; dimension <= 64U; dimension *= 2U) {
        const char *names[] = {"discovery_one", "discovery_empty", "discovery_three",
                               "discovery_repeated"};
        double references[] = {1.0, 0.0, 3.0, 1.0};

        for (variant = 0; variant < 4; ++variant) {
            p = (problem){CAPABILITY, names[variant], 10 + variant, 3, 0, 5, 0,
                          references[variant], 0, 1, 1e-8, 1e-8, dimension};
            run_case(&p, "discovery", "execute", "numerix_only", 0);
        }
    }
}

int main(int argc, char **argv)
{
    int index;

#ifdef NUMERIX_HAVE_GSL
    gsl_set_error_handler_off();
#endif
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--samples") == 0 && index + 1 < argc) {
            char *end;
            unsigned long count;

            errno = 0;
            count = strtoul(argv[++index], &end, 10);
            if (errno != 0 || *end != '\0' || count < 3U || count > MAX_SAMPLES ||
                count % 2U == 0U) {
                fputs("samples must be an odd integer from 3 through 101\n", stderr);
                return 2;
            }
            sample_count = (size_t)count;
        } else if (strcmp(argv[index], "--memory") == 0) {
            memory_only = 1;
        } else if (strcmp(argv[index], "--case") == 0 && index + 1 < argc) {
            case_filter = argv[++index];
        } else if (strcmp(argv[index], "--library") == 0 && index + 1 < argc) {
            library_filter = argv[++index];
        } else if (strcmp(argv[index], "--versions") == 0) {
            printf("NUMERIX=%s\n", NUMERIX_VERSION_STRING);
#ifdef NUMERIX_HAVE_GSL
            printf("GSL=%s\n", gsl_version);
#else
            puts("GSL=unavailable (NUMERIX-only build)");
#endif
            printf("double_mantissa_bits=%d\n", DBL_MANT_DIG);
            return 0;
        } else {
            fprintf(stderr, "Usage: %s [--samples ODD] [--memory] [--case NAME] "
                    "[--library NUMERIX|GSL] [--versions]\n", argv[0]);
            return 2;
        }
    }
#ifdef NUMERIX_BENCH_ALLOCATIONS
    if (!memory_only) {
        fputs("use --memory with the allocation-instrumented binary\n", stderr);
        return 2;
    }
#else
    if (memory_only) {
        fputs("allocation instrumentation is not enabled in this binary\n", stderr);
        return 2;
    }
#endif
    puts("category,case,difficulty,size,method,phase,library,comparison,status,"
         "accuracy_pass,trials,successes,accuracy_successes,batch,min_ns,median_ns,"
         "mean_ns,stddev_ns,p95_ns,value,reference,absolute_error,relative_error,"
         "residual,estimated_error,backward_error,trajectory_error,function_evaluations,"
         "derivative_evaluations,iterations,precision_bits,absolute_tolerance,"
         "relative_tolerance,objective,memory_measured,allocations,allocated_bytes,"
         "peak_live_bytes,live_bytes_after_destroy,setup_live_bytes,"
         "direct_result_bytes,direct_workspace_peak_bytes");
    run_suite();
    return ferror(stdout) ? 2 : 0;
}
