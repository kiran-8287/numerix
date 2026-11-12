#define _POSIX_C_SOURCE 200809L

#include "phase14_json.h"
#include "numerix.h"

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#ifdef P14_HAVE_GSL
#include <gsl/gsl_blas.h>
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_min.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_roots.h>
#endif

#ifdef P14_HAVE_CMINPACK
#include <cminpack.h>
#endif

#ifdef P14_HAVE_NLOPT
#include <nlopt.h>
#endif

#ifdef P14_HAVE_LAPACKE
#include <lapacke.h>
#endif

#ifdef P14_HAVE_OPENBLAS
#include <cblas.h>
#endif

#ifdef P14_HAVE_SUNDIALS
#include <arkode/arkode_butcher_erk.h>
#include <arkode/arkode_erkstep.h>
#include <cvode/cvode.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_context.h>
#include <sunnonlinsol/sunnonlinsol_fixedpoint.h>
#endif

#define P14_LINE_CAPACITY (2U * 1024U * 1024U)
#define P14_MAX_NODES 256U
#define P14_VALIDATION_POINTS 101U

typedef enum p14_module {
    P14_ROOT,
    P14_LINEAR_ALGEBRA,
    P14_DIFFERENTIATION,
    P14_INTERPOLATION,
    P14_INTEGRATION,
    P14_OPTIMIZATION,
    P14_ODE,
    P14_UNKNOWN
} p14_module;

typedef struct p14_problem {
    char id[80];
    char family[80];
    p14_module module;
    int model;
    size_t n;
    size_t dimension;
    size_t steps;
    double parameter[8];
    double lower;
    double upper;
    double initial;
    double second_initial;
    double x;
    double h;
    double t0;
    double tf;
    double fixed_step;
    double requested_abs_tolerance;
    double requested_rel_tolerance;
    double absolute_tolerance;
    double relative_tolerance;
    double reference[8];
    size_t reference_size;
    double *matrix;
    double *rhs;
    double *x_reference;
    double *nodes;
    double *values;
    size_t node_count;
    double *validation_x;
    double *validation_y;
    size_t validation_count;
    double *initial_state;
    int expected_singular;
} p14_problem;

typedef struct p14_outcome {
    int status;
    int native_status;
    int converged;
    double value;
    double estimate;
    double error;
    double secondary_error;
    double reference_norm;
    double residual;
    size_t iterations;
    size_t function_evaluations;
    size_t rhs_evaluations;
    char method[48];
} p14_outcome;

static p14_problem *p14_active_problem;
static size_t p14_function_calls;
static size_t p14_gradient_calls;
static size_t p14_rhs_calls;
static size_t p14_interpolation_calls;
static int p14_count_work;

static double p14_monotonic_seconds(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return NAN;
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static double p14_cpu_seconds(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) return NAN;
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static size_t p14_get(const p14_json_doc *doc, size_t object, const char *key)
{
    return p14_json_object_get(doc, object, key);
}

static double p14_number(const p14_json_doc *doc, size_t object,
                         const char *key, double fallback)
{
    return p14_json_number(doc, p14_get(doc, object, key), fallback);
}

static size_t p14_number_array(const p14_json_doc *doc, size_t object,
                               const char *key, double *values,
                               size_t capacity)
{
    size_t written = 0;
    size_t token = p14_get(doc, object, key);
    if (token == (size_t)-1 ||
        !p14_json_array_to_doubles(doc, token, values, capacity, &written)) return 0;
    return written;
}

static int p14_parse_matrix(const p14_json_doc *doc, size_t token,
                            size_t n, double **out)
{
    size_t i;
    double *matrix;
    if (n == 0 || n > 512 || token >= doc->count ||
        doc->tokens[token].kind != P14_JSON_ARRAY) return 0;
    matrix = (double *)calloc(n * n, sizeof(*matrix));
    if (matrix == NULL) return 0;
    for (i = 0; i < n; ++i) {
        size_t row = p14_json_array_get(doc, token, i);
        size_t written = 0;
        if (row == (size_t)-1 ||
            !p14_json_array_to_doubles(doc, row, matrix + i * n, n, &written) ||
            written != n) {
            free(matrix);
            return 0;
        }
    }
    *out = matrix;
    return 1;
}

static int p14_module_from_id(const char *id)
{
    if (strncmp(id, "P14-ROOT-", 9) == 0) return P14_ROOT;
    if (strncmp(id, "P14-LINEAR_ALGEBRA-", 19) == 0) return P14_LINEAR_ALGEBRA;
    if (strncmp(id, "P14-DIFFERENTIATION-", 20) == 0) return P14_DIFFERENTIATION;
    if (strncmp(id, "P14-INTERPOLATION-", 18) == 0) return P14_INTERPOLATION;
    if (strncmp(id, "P14-INTEGRATION-", 16) == 0) return P14_INTEGRATION;
    if (strncmp(id, "P14-OPTIMIZATION-", 17) == 0) return P14_OPTIMIZATION;
    if (strncmp(id, "P14-ODE-", 8) == 0) return P14_ODE;
    return P14_UNKNOWN;
}

static void p14_outcome_init(p14_outcome *out)
{
    memset(out,0,sizeof(*out));
    out->value=NAN;
    out->estimate=NAN;
    out->error=NAN;
    out->secondary_error=NAN;
    out->reference_norm=NAN;
    out->residual=NAN;
}

static int p14_parse_problem(const char *line, p14_problem *problem)
{
    p14_json_doc doc = {0};
    const char *parse_stage = "top-level fields";
    size_t root_token;
    size_t input_token;
    size_t reference_token;
    const char *id_start;
    size_t id_length;
    double values[P14_MAX_NODES];
    size_t n;

    memset(problem, 0, sizeof(*problem));
    if (!p14_json_parse(&doc, line)) return 0;
    root_token = 0;
    input_token = p14_get(&doc, root_token, "input_parameters");
    reference_token = p14_get(&doc, root_token, "reference_value");
    if (input_token == (size_t)-1 || reference_token == (size_t)-1) goto fail;

    {
        parse_stage = "problem ID";
        size_t id_token = p14_get(&doc, root_token, "problem_id");
        if (id_token == (size_t)-1 || doc.tokens[id_token].kind != P14_JSON_STRING) goto fail;
        id_start = doc.text + doc.tokens[id_token].start + 1;
        id_length = doc.tokens[id_token].end - doc.tokens[id_token].start - 2;
        if (id_length >= sizeof(problem->id)) goto fail;
        memcpy(problem->id, id_start, id_length);
        problem->id[id_length] = '\0';
    }
    {
        parse_stage = "problem family";
        size_t family_token=p14_get(&doc,root_token,"family");
        size_t family_length;
        if(family_token==(size_t)-1||doc.tokens[family_token].kind!=P14_JSON_STRING)goto fail;
        family_length=doc.tokens[family_token].end-doc.tokens[family_token].start-2;
        if(family_length>=sizeof(problem->family))goto fail;
        memcpy(problem->family,doc.text+doc.tokens[family_token].start+1,family_length);
        problem->family[family_length]='\0';
    }
    problem->module = (p14_module)p14_module_from_id(problem->id);
    parse_stage = "module and model";
    problem->model = (int)p14_number(&doc, input_token, "_model", -1);
    problem->absolute_tolerance=p14_number(&doc,root_token,"absolute_tolerance",1e-8);
    problem->relative_tolerance=p14_number(&doc,root_token,"relative_tolerance",1e-8);
    if (problem->module == P14_UNKNOWN || problem->model < 0) goto fail;

    if (problem->module == P14_ROOT) {
        parse_stage = "root inputs";
        problem->parameter[0] = p14_number(&doc, input_token, "root", 0.0);
        problem->parameter[1] = p14_number(&doc, input_token, "scale", 1.0);
        problem->parameter[2] = p14_number(&doc, input_token, "shape", 1.0);
        problem->parameter[3] = p14_number(&doc, input_token, "shift", 1.0);
        problem->lower = p14_number(&doc, input_token, "lower", -1.0);
        problem->upper = p14_number(&doc, input_token, "upper", 1.0);
        problem->initial = p14_number(&doc, input_token, "initial", 0.0);
        problem->second_initial = p14_number(&doc, input_token, "second_initial", 0.5);
        problem->reference[0] = p14_json_number(&doc, reference_token, NAN);
        problem->reference_size = 1;
    } else if (problem->module == P14_LINEAR_ALGEBRA) {
        parse_stage = "linear algebra inputs";
        problem->n = (size_t)p14_number(&doc, input_token, "n", 0.0);
        parse_stage = "linear algebra matrix";
        if (!p14_parse_matrix(&doc, p14_get(&doc, input_token, "matrix"), problem->n,
                              &problem->matrix)) goto fail;
        parse_stage = "linear algebra allocations";
        problem->rhs = (double *)calloc(problem->n, sizeof(*problem->rhs));
        problem->x_reference = (double *)calloc(problem->n, sizeof(*problem->x_reference));
        if (problem->rhs == NULL || problem->x_reference == NULL) goto fail;
        parse_stage = "linear algebra vectors";
        if (p14_number_array(&doc, input_token, "rhs", problem->rhs, problem->n) != problem->n ||
            p14_number_array(&doc, input_token, "x_reference", problem->x_reference,
                             problem->n) != problem->n) goto fail;
        parse_stage = "linear algebra reference";
        problem->expected_singular = p14_json_string_equals(&doc,
            p14_get(&doc, input_token, "condition_number_reference"), "inf");
        n = p14_json_array_get(&doc, reference_token, 0);
        if (reference_token >= doc.count || doc.tokens[reference_token].kind != P14_JSON_ARRAY) goto fail;
        problem->reference_size = problem->n;
        for (size_t i = 0; i < problem->n; ++i) {
            size_t item = p14_json_array_get(&doc, reference_token, i);
            if (item == (size_t)-1) goto fail;
            problem->x_reference[i] = p14_json_number(&doc, item, NAN);
        }
        (void)n;
    } else if (problem->module == P14_DIFFERENTIATION) {
        parse_stage = "differentiation inputs";
        problem->x = p14_number(&doc, input_token, "x", 0.0);
        problem->h = p14_number(&doc, input_token, "step", 1e-4);
        problem->parameter[0] = p14_number(&doc, input_token, "a", 1.0);
        problem->parameter[1] = p14_number(&doc, input_token, "center", 0.0);
        problem->reference[0] = p14_json_number(&doc, reference_token, NAN);
        problem->reference_size = 1;
    } else if (problem->module == P14_INTERPOLATION) {
        parse_stage = "interpolation inputs";
        problem->n = (size_t)p14_number(&doc, input_token, "n", 0.0);
        problem->node_count = problem->n;
        problem->validation_count = p14_number_array(&doc, input_token, "validation_x",
                                                       values, P14_MAX_NODES);
        if (problem->node_count == 0 || problem->node_count > P14_MAX_NODES ||
            problem->validation_count == 0 || problem->validation_count > P14_MAX_NODES) goto fail;
        problem->nodes = (double *)calloc(problem->node_count, sizeof(double));
        problem->values = (double *)calloc(problem->node_count, sizeof(double));
        problem->validation_x = (double *)calloc(problem->validation_count, sizeof(double));
        problem->validation_y = (double *)calloc(problem->validation_count, sizeof(double));
        if (problem->nodes == NULL || problem->values == NULL || problem->validation_x == NULL ||
            problem->validation_y == NULL) goto fail;
        if (p14_number_array(&doc, input_token, "nodes", problem->nodes, problem->node_count) != problem->node_count ||
            p14_number_array(&doc, input_token, "values", problem->values, problem->node_count) != problem->node_count ||
            p14_number_array(&doc, input_token, "validation_x", problem->validation_x,
                             problem->validation_count) != problem->validation_count ||
            p14_number_array(&doc, input_token, "validation_y", problem->validation_y,
                             problem->validation_count) != problem->validation_count) goto fail;
        problem->parameter[0] = p14_number(&doc, input_token, "a", 1.0);
    } else if (problem->module == P14_INTEGRATION) {
        parse_stage = "integration inputs";
        problem->parameter[0] = p14_number(&doc, input_token, "a", 1.0);
        problem->parameter[1] = p14_number(&doc, input_token, "b", 0.0);
        problem->parameter[2] = p14_number(&doc, input_token, "center", 0.5);
        problem->lower = p14_number(&doc, input_token, "lower", 0.0);
        problem->upper = p14_number(&doc, input_token, "upper", 1.0);
        problem->requested_abs_tolerance = p14_number(&doc, input_token,
            "requested_abs_tolerance", 1e-8);
        problem->requested_rel_tolerance = p14_number(&doc, input_token,
            "requested_rel_tolerance", 1e-8);
        problem->n = (size_t)p14_number(&doc, input_token, "fixed_panels", 256.0);
        problem->reference[0] = p14_json_number(&doc, reference_token, NAN);
        problem->reference_size = 1;
    } else if (problem->module == P14_OPTIMIZATION) {
        parse_stage = "optimization inputs";
        size_t params = p14_get(&doc, input_token, "parameters");
        if (params == (size_t)-1 ||
            p14_json_array_to_doubles(&doc, params, problem->parameter, 6, &n) == 0 || n != 6) goto fail;
        problem->lower = p14_number(&doc, input_token, "lower", -1.0);
        problem->upper = p14_number(&doc, input_token, "upper", 1.0);
        problem->initial = p14_number(&doc, input_token, "initial", 0.0);
        problem->reference[0] = p14_number(&doc, input_token, "reference_minimizer", 0.0);
        problem->reference[1] = p14_json_number(&doc, reference_token, NAN);
        problem->reference_size = 2;
    } else if (problem->module == P14_ODE) {
        parse_stage = "ODE inputs";
        size_t params = p14_get(&doc, input_token, "parameters");
        if (params == (size_t)-1 ||
            p14_json_array_to_doubles(&doc, params, problem->parameter, 3, &n) == 0 || n != 3) goto fail;
        problem->dimension = (size_t)p14_number(&doc, input_token, "dimension", 1.0);
        problem->steps = (size_t)p14_number(&doc, input_token, "fixed_steps", 1.0);
        problem->t0 = p14_number(&doc, input_token, "t0", 0.0);
        problem->tf = p14_number(&doc, input_token, "tf", 1.0);
        problem->fixed_step = p14_number(&doc, input_token, "fixed_step", 0.01);
        problem->requested_abs_tolerance = p14_number(&doc, input_token, "adaptive_atol", 1e-10);
        problem->requested_rel_tolerance = p14_number(&doc, input_token, "adaptive_rtol", 1e-8);
        problem->initial_state = (double *)calloc(problem->dimension, sizeof(double));
        if (problem->initial_state == NULL || problem->dimension > 8 ||
            p14_number_array(&doc, input_token, "initial_state", problem->initial_state,
                             problem->dimension) != problem->dimension) goto fail;
        if (reference_token >= doc.count || doc.tokens[reference_token].kind != P14_JSON_ARRAY) goto fail;
        problem->reference_size = problem->dimension;
        for (size_t i = 0; i < problem->dimension; ++i) {
            size_t item = p14_json_array_get(&doc, reference_token, i);
            if (item == (size_t)-1) goto fail;
            problem->reference[i] = p14_json_number(&doc, item, NAN);
        }
    }
    p14_json_destroy(&doc);
    return 1;

fail:
    fprintf(stderr, "Phase 14 input parse failed at %s.\n", parse_stage);
    p14_json_destroy(&doc);
    free(problem->matrix);
    free(problem->rhs);
    free(problem->x_reference);
    free(problem->nodes);
    free(problem->values);
    free(problem->validation_x);
    free(problem->validation_y);
    free(problem->initial_state);
    memset(problem, 0, sizeof(*problem));
    return 0;
}

static void p14_problem_destroy(p14_problem *problem)
{
    free(problem->matrix);
    free(problem->rhs);
    free(problem->x_reference);
    free(problem->nodes);
    free(problem->values);
    free(problem->validation_x);
    free(problem->validation_y);
    free(problem->initial_state);
    memset(problem, 0, sizeof(*problem));
}

static double p14_root_value(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    double u = x - p->parameter[0];
    double a = p->parameter[1];
    double k = p->parameter[2];
    double d = p->parameter[3];
    if (p14_count_work) ++p14_function_calls;
    switch (p->model) {
    case 0: return a * u;
    case 1: return a * u * (u - d);
    case 2: return a * (u + 0.08 * k * u * u * u);
    case 3: return a * u * (1.0 + 0.03 * k * u * u * u * u);
    case 4: return a * sin(k * u);
    case 5: return a * expm1(k * u);
    case 6: return a * log((x + d) / (p->parameter[0] + d));
    case 7: return a * u / (1.0 + k * u * u);
    case 8: return a * (u + 0.1 * sin(k * u));
    case 9: return a * tanh(k * u);
    case 10: return a * atan(k * u);
    case 11: return a * sinh(k * u);
    case 12: return a * u * (1.0 + 0.05 * u + 0.02 * u * u);
    case 13: return a * u * u;
    case 14: return a * u * u * u;
    case 15: return a * u * u * u * u * u;
    case 16: return a * (u + 0.03 * u * u * u);
    case 17: return a * 1e-12 * u;
    case 18: return a * u * (1.0 + 0.2 * cos(k * u));
    default: return a * u * (1.0 + 0.1 * u * u);
    }
}

static double p14_root_derivative(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    double u = x - p->parameter[0];
    double a = p->parameter[1];
    double k = p->parameter[2];
    double d = p->parameter[3];
    if (p14_count_work) ++p14_gradient_calls;
    switch (p->model) {
    case 0: return a;
    case 1: return a * (2.0 * u - d);
    case 2: return a * (1.0 + 0.24 * k * u * u);
    case 3: return a * (1.0 + 0.15 * k * u * u * u * u);
    case 4: return a * k * cos(k * u);
    case 5: return a * k * exp(k * u);
    case 6: return a / (x + d);
    case 7: return a * (1.0 - k * u * u) / pow(1.0 + k * u * u, 2.0);
    case 8: return a * (1.0 + 0.1 * k * cos(k * u));
    case 9: return a * k / pow(cosh(k * u), 2.0);
    case 10: return a * k / (1.0 + k * k * u * u);
    case 11: return a * k * cosh(k * u);
    case 12: return a * (1.0 + 0.1 * u + 0.06 * u * u);
    case 13: return 2.0 * a * u;
    case 14: return 3.0 * a * u * u;
    case 15: return 5.0 * a * u * u * u * u;
    case 16: return a * (1.0 + 0.09 * u * u);
    case 17: return a * 1e-12;
    case 18: return a * (1.0 + 0.2 * cos(k * u) - 0.2 * k * u * sin(k * u));
    default: return a * (1.0 + 0.3 * u * u);
    }
}

static double p14_diff_value(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    double a = p->parameter[0];
    double c = p->parameter[1];
    double u = x - c;
    if (p14_count_work) ++p14_function_calls;
    switch (p->model) {
    case 0: return x*x*x + a*x*x + 0.3*x;
    case 1: return sin(a*x);
    case 2: return cos(a*x);
    case 3: return exp(a*x);
    case 4: return log1p(a*x);
    case 5: return sqrt(1.0+a*x);
    case 6: return 1.0/(1.0+a*x*x);
    case 7: return exp(-a*u*u);
    case 8: return tanh(a*x);
    case 9: return atan(a*x);
    case 10: return x*exp(-a*x);
    case 11: return sin(a*x*x);
    case 12: return exp(a*sin(x));
    case 13: return log1p(a*x*x);
    case 14: return x/(1.0+a*x*x);
    case 15: return x*x*x*x*x+a*x*x;
    case 16: return sqrt(x*x+a);
    case 17: return cosh(a*x);
    case 18: return asinh(a*x);
    default: return 1.0-exp(-a*x);
    }
}

static double p14_integration_value(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    double a = p->parameter[0];
    double b = p->parameter[1];
    double c = p->parameter[2];
    double u = (x-c)/a;
    if (p14_count_work) ++p14_function_calls;
    switch (p->model) {
    case 0: return x*x*x*x+a*x*x+b*x;
    case 1: return sin(a*x);
    case 2: return cos(a*x);
    case 3: return exp(a*x);
    case 4: return log1p(a*x);
    case 5: return 1.0/(1.0+(a*x)*(a*x));
    case 6: case 7: return exp(-u*u);
    case 8: return sin(a*x)/(1.0+x*x);
    case 9: return sqrt(x);
    case 10: return 1.0/sqrt(x);
    case 11: return log(x);
    case 12: return 1.0/(1.0+exp(-a*(x-c)));
    case 13: return 1.0/(1.0+a*x);
    case 14: return fabs(x-c);
    case 15: return 1e-8*sin(a*x);
    case 16: return 1e8*(x*x+1.0);
    case 17: return tanh(a*(x-c));
    case 18: { double s=sin(a*x); return s*s; }
    default: return exp(-a*x)*cos(b*x);
    }
}

static double p14_optimization_value(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    double m=p->parameter[0], s=p->parameter[1], r=p->parameter[2], k=p->parameter[3];
    double u=x-m;
    if (p14_count_work) ++p14_function_calls;
    switch (p->model) {
    case 0: return s*u*u;
    case 1: return s*u*u*u*u;
    case 2: return s*(u*u+0.3*u*u*u+0.2*u*u*u*u);
    case 3: case 4: case 15: return s*(u*u+r*(1.0-cos(k*u)));
    case 5: return s*u*u*u*u+0.01*u*u;
    case 6: return s*u*u+0.01*u*u*u*u;
    case 7: return s*u*u;
    case 8: return s*(exp(0.5*u)+exp(-0.5*u)-2.0);
    case 9: return s*(log(cosh(u))+0.5*u*u);
    case 10: return s*u*u/(1.0+0.1*u*u);
    case 11: case 12: return s*u*u;
    case 13: return 1e-8*u*u;
    case 14: return 1e8*u*u;
    case 16: return s*(u*u+0.2*(1.0-cos(2.0*u)));
    case 17: return s*(sqrt(u*u+1e-4)-0.01+u*u);
    case 18: { double v=expm1(fabs(u)); return s*v*v; }
    default: return s*(u*u+0.01*u*u*u*u+0.05*(1.0-cos(10.0*u)));
    }
}

static int p14_ode_rhs(double t, const double *y, double *dydt, p14_problem *p)
{
    double a=p->parameter[0], b=p->parameter[1], c=p->parameter[2];
    if (p14_count_work) ++p14_rhs_calls;
    switch (p->model) {
    case 0: dydt[0]=a; break;
    case 1: dydt[0]=a*y[0]; break;
    case 2: dydt[0]=a*y[0]+b; break;
    case 3: dydt[0]=a*t; break;
    case 4: dydt[0]=a*t*t+b; break;
    case 5: dydt[0]=y[1]; dydt[1]=-a*a*y[0]; break;
    case 6: dydt[0]=y[1]; dydt[1]=-a*a*y[0]-2.0*b*a*y[1]; break;
    case 7: dydt[0]=a*y[0]; dydt[1]=-b*y[1]; break;
    case 8: dydt[0]=a*y[0]*(1.0-y[0]/b); break;
    case 9: dydt[0]=a*y[0]+b*cos(c*t); break;
    case 10: dydt[0]=a*y[0]*y[0]; break;
    case 11: dydt[0]=a*exp(-b*t*t); break;
    case 12: dydt[0]=a*sin(b*t); break;
    case 13: dydt[0]=a*cos(b*t); break;
    case 14: dydt[0]=a*exp(b*t); break;
    case 15: dydt[0]=a*y[0]-b*y[0]*y[1]; dydt[1]=-c*y[1]+a*y[0]*y[1]; break;
    case 16: dydt[0]=y[1]; dydt[1]=a*(1.0-y[0]*y[0])*y[1]-y[0]; break;
    case 17: dydt[0]=-b*y[0]*y[1]; dydt[1]=b*y[0]*y[1]-c*y[1]; dydt[2]=c*y[1]; break;
    case 18: dydt[0]=-a*y[0]; dydt[1]=-b*y[1]; break;
    default: dydt[0]=y[1]; dydt[1]=-a*sin(y[0])-b*y[1]; break;
    }
    return 0;
}

static numerix_status p14_numerix_ode_rhs(double t, const double *y,
                                           size_t dimension, double *dydt,
                                           void *data)
{
    p14_problem *p = (p14_problem *)data;
    if (p == NULL || dimension != p->dimension) {
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    return (numerix_status)p14_ode_rhs(t, y, dydt, p);
}

#ifdef P14_HAVE_GSL
static double p14_gsl_root_callback(double x, void *data)
{
    return p14_root_value(x, (p14_problem *)data);
}

static double p14_gsl_root_derivative(double x, void *data)
{
    return p14_root_derivative(x,data);
}

static void p14_gsl_root_fdf(double x, void *data, double *value,
                             double *derivative)
{
    *value=p14_root_value(x,data);
    *derivative=p14_root_derivative(x,data);
}

static double p14_gsl_diff_callback(double x, void *data)
{
    return p14_diff_value(x, (p14_problem *)data);
}

static double p14_gsl_integral_callback(double x, void *data)
{
    return p14_integration_value(x, (p14_problem *)data);
}

static double p14_gsl_objective_callback(double x, void *data)
{
    return p14_optimization_value(x, (p14_problem *)data);
}
#endif

#ifdef P14_HAVE_CMINPACK
static int p14_cminpack_root_callback(void *data, int n, const double *x,
                                      double *fvec, int iflag)
{
    (void)iflag;
    if (n != 1) return -1;
    fvec[0] = p14_root_value(x[0], (p14_problem *)data);
    return 0;
}
#endif

#ifdef P14_HAVE_NLOPT
static double p14_nlopt_objective(unsigned n, const double *x, double *gradient,
                                  void *data)
{
    p14_problem *p=(p14_problem *)data;
    if (n != 1) return HUGE_VAL;
    if (gradient != NULL) {
        const double dx=sqrt(DBL_EPSILON)*(1.0+fabs(x[0]));
        if (p14_count_work) ++p14_gradient_calls;
        gradient[0]=(p14_optimization_value(x[0]+dx,p)-p14_optimization_value(x[0]-dx,p))/(2.0*dx);
    }
    return p14_optimization_value(x[0],p);
}
#endif

#ifdef P14_HAVE_GSL
static int p14_gsl_ode_callback(double t, const double y[], double dydt[], void *data)
{
    return p14_ode_rhs(t, y, dydt, (p14_problem *)data);
}

static int p14_gsl_interpolation(const p14_problem *p, p14_outcome *out)
{
    gsl_interp *interpolator;
    gsl_interp_accel *accelerator;
    double sum2=0.0, max_abs=0.0, refnorm=0.0;
    int status=0;
    interpolator=gsl_interp_alloc(gsl_interp_polynomial, p->node_count);
    accelerator=gsl_interp_accel_alloc();
    if (interpolator==NULL || accelerator==NULL) {
        if(interpolator)gsl_interp_free(interpolator);
        if(accelerator)gsl_interp_accel_free(accelerator);
        return 1;
    }
    status=gsl_interp_init(interpolator,p->nodes,p->values,p->node_count);
    if (status==0) {
        for (size_t i=0;i<p->validation_count;i++) {
            double value=NAN;
            status=gsl_interp_eval_e(interpolator,p->nodes,p->values,p->validation_x[i],accelerator,&value);
            if (status!=0 || !isfinite(value)) break;
            if (p14_count_work) ++p14_interpolation_calls;
            double error=fabs(value-p->validation_y[i]);
            refnorm=fmax(refnorm,fabs(p->validation_y[i]));
            if (error>max_abs) max_abs=error;
            sum2+=error*error;
        }
    }
    out->error=max_abs;
    out->secondary_error=p->validation_count ? sqrt(sum2/(double)p->validation_count) : NAN;
    out->reference_norm=refnorm;
    out->value=0.0;
    out->status=status;
    out->function_evaluations=p14_interpolation_calls;
    snprintf(out->method,sizeof(out->method),"gsl_interp_polynomial");
    gsl_interp_free(interpolator);
    gsl_interp_accel_free(accelerator);
    return status;
}

static int p14_gsl_linear_solve(const p14_problem *p, p14_outcome *out)
{
    gsl_matrix *a=gsl_matrix_alloc(p->n,p->n);
    gsl_vector *b=gsl_vector_alloc(p->n);
    gsl_vector *x=gsl_vector_alloc(p->n);
    gsl_permutation *perm=gsl_permutation_alloc(p->n);
    int sign=0, status=GSL_ENOMEM;
    double max_error=0.0, residual=0.0, refnorm=0.0;
    if (a==NULL || b==NULL || x==NULL || perm==NULL) goto done;
    for (size_t i=0;i<p->n;i++) {
        gsl_vector_set(b,i,p->rhs[i]);
        for (size_t j=0;j<p->n;j++) gsl_matrix_set(a,i,j,p->matrix[i*p->n+j]);
    }
    status=gsl_linalg_LU_decomp(a,perm,&sign);
    if (status==0) status=gsl_linalg_LU_solve(a,perm,b,x);
    if (status==0) {
        for (size_t i=0;i<p->n;i++) {
            double e=fabs(gsl_vector_get(x,i)-p->x_reference[i]);
            double rr=-p->rhs[i];
            if(e>max_error)max_error=e;
            refnorm=fmax(refnorm,fabs(p->x_reference[i]));
            for(size_t j=0;j<p->n;j++)rr+=p->matrix[i*p->n+j]*gsl_vector_get(x,j);
            residual=fmax(residual,fabs(rr));
        }
        out->error=max_error; out->reference_norm=refnorm; out->residual=residual;
        out->secondary_error=residual;
        out->value=0.0;
    }
    out->status=status;
    snprintf(out->method,sizeof(out->method),"gsl_lu_solve");
done:
    if(a)gsl_matrix_free(a);
    if(b)gsl_vector_free(b);
    if(x)gsl_vector_free(x);
    if(perm)gsl_permutation_free(perm);
    return status;
}
#endif

static void p14_options(numerix_options *options, double atol, double rtol)
{
    options->absolute_tolerance=atol;
    options->relative_tolerance=rtol;
    options->maximum_iterations=10000;
    options->maximum_function_evaluations=100000;
    options->maximum_execution_time_seconds=0.0;
}

static int p14_run_numerix(p14_problem *p, p14_outcome *out)
{
    numerix_options options;
    numerix_error error;
    numerix_result result;
    numerix_status status=NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    p14_outcome_init(out);
    memset(&result,0,sizeof(result));
    memset(&error,0,sizeof(error));
    p14_options(&options,p->absolute_tolerance,p->relative_tolerance);
    p14_active_problem=p;

    switch(p->module) {
    case P14_ROOT:
        if (p->model>=13 && p->model<=15) {
            status=numerix_root_newton(p14_root_value,p14_root_derivative,p,p->initial,
                                       &options,&result,&error);
            snprintf(out->method,sizeof(out->method),"numerix_newton");
        } else {
            status=numerix_root_brent(p14_root_value,p,p->lower,p->upper,
                                      &options,&result,&error);
            snprintf(out->method,sizeof(out->method),"numerix_brent");
        }
        out->value=result.value; out->iterations=result.iterations;
        out->function_evaluations=result.function_evaluations; out->estimate=result.estimated_error;
        out->residual=fabs(p14_root_value(result.value,p));
        out->secondary_error=out->residual;
        break;
    case P14_DIFFERENTIATION:
        status=numerix_diff_central(p14_diff_value,p,p->x,p->h,&options,&result,&error);
        snprintf(out->method,sizeof(out->method),"numerix_central");
        out->value=result.value; out->estimate=result.estimated_error;
        out->function_evaluations=result.function_evaluations;
        break;
    case P14_LINEAR_ALGEBRA: {
        numerix_matrix *a=NULL;
        numerix_vector *b=NULL,*x=NULL;
        if (numerix_matrix_create(p->n,p->n,&a,&error)!=NUMERIX_STATUS_SUCCESS ||
            numerix_vector_create(p->n,&b,&error)!=NUMERIX_STATUS_SUCCESS) {
            status=NUMERIX_STATUS_ALLOCATION_FAILURE;
            numerix_matrix_destroy(a); numerix_vector_destroy(b); break;
        }
        memcpy(a->data,p->matrix,p->n*p->n*sizeof(double));
        memcpy(b->data,p->rhs,p->n*sizeof(double));
        status=numerix_linear_solve_gaussian(a,b,&options,&x,&out->residual,&error);
        if(status==NUMERIX_STATUS_SUCCESS && x!=NULL) {
            double e=0.0, ref=0.0;
            for(size_t i=0;i<p->n;i++) { e=fmax(e,fabs(x->data[i]-p->x_reference[i])); ref=fmax(ref,fabs(p->x_reference[i])); }
        out->error=e; out->reference_norm=ref; out->secondary_error=out->residual;
        }
        out->value=0.0;
        snprintf(out->method,sizeof(out->method),"numerix_gaussian_solve");
        numerix_vector_destroy(x); numerix_vector_destroy(b); numerix_matrix_destroy(a);
        break;
    }
    case P14_INTERPOLATION: {
        numerix_interpolant interpolant;
        double sum2=0.0,max_abs=0.0,refnorm=0.0;
        memset(&interpolant,0,sizeof(interpolant));
        status=numerix_interp_newton_create(p->nodes,p->values,p->node_count,&interpolant,&error);
        if(status==NUMERIX_STATUS_SUCCESS) {
            for(size_t i=0;i<p->validation_count;i++) {
                numerix_result value;
                status=numerix_interp_newton_evaluate(&interpolant,p->validation_x[i],1,&value,&error);
                if(status!=NUMERIX_STATUS_SUCCESS) break;
                double e=fabs(value.value-p->validation_y[i]);
                refnorm=fmax(refnorm,fabs(p->validation_y[i]));
                max_abs=fmax(max_abs,e); sum2+=e*e;
                if(p14_count_work)++p14_interpolation_calls;
            }
        }
        out->error=max_abs; out->secondary_error=sqrt(sum2/(double)p->validation_count);
        out->reference_norm=refnorm;
        snprintf(out->method,sizeof(out->method),"numerix_newton_polynomial");
        numerix_interpolant_destroy(&interpolant);
        break;
    }
    case P14_INTEGRATION:
        p14_options(&options,p->requested_abs_tolerance,p->requested_rel_tolerance);
        status=numerix_integrate_adaptive_simpson(p14_integration_value,p,p->lower,p->upper,
                      &options,24,&result,&error);
        snprintf(out->method,sizeof(out->method),"numerix_adaptive_simpson");
        out->value=result.value; out->estimate=result.estimated_error;
        out->iterations=result.iterations; out->function_evaluations=result.function_evaluations;
        break;
    case P14_OPTIMIZATION: {
        p14_options(&options,1e-9,1e-9);
        status=numerix_optimize_golden_section(p14_optimization_value,p,p->lower,p->upper,
                                               &options,&result,&error);
        snprintf(out->method,sizeof(out->method),"numerix_golden_section");
        out->value=result.value; out->iterations=result.iterations;
        out->function_evaluations=result.function_evaluations;
        out->estimate=result.estimated_error;
        out->error=fabs(result.value-p->reference[0]);
        out->secondary_error=fabs(p14_optimization_value(result.value,p)-p->reference[1]);
        out->reference_norm=fabs(p->reference[0]);
        break;
    }
    case P14_ODE: {
        numerix_ode_solution solution;
        memset(&solution,0,sizeof(solution));
        status=numerix_ode_rk4(p14_numerix_ode_rhs,p,p->t0,p->initial_state,p->dimension,
                              p->fixed_step,p->steps,&solution,&error);
        snprintf(out->method,sizeof(out->method),"numerix_rk4_fixed");
        if(status==NUMERIX_STATUS_SUCCESS && solution.points>0) {
            double e=0.0,ref=0.0;
            size_t last=solution.points-1;
            for(size_t i=0;i<p->dimension;i++) {
                double v=solution.y[last*p->dimension+i];
                e=fmax(e,fabs(v-p->reference[i])); ref=fmax(ref,fabs(p->reference[i]));
            }
            out->error=e; out->reference_norm=ref;
            out->rhs_evaluations=solution.function_evaluations;
        }
        numerix_ode_solution_destroy(&solution);
        break;
    }
    default: break;
    }

    out->status=(int)status;
    out->native_status=(int)status;
    out->converged=(status==NUMERIX_STATUS_SUCCESS);
    out->function_evaluations=out->function_evaluations ? out->function_evaluations : p14_function_calls;
    if (out->status==NUMERIX_STATUS_SUCCESS && p->module==P14_ROOT) {
        out->error=fabs(out->value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
        out->secondary_error=fabs(p14_root_value(out->value,p));
    } else if (out->status==NUMERIX_STATUS_SUCCESS && p->module==P14_DIFFERENTIATION) {
        out->error=fabs(out->value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
    } else if (out->status==NUMERIX_STATUS_SUCCESS && p->module==P14_INTEGRATION) {
        out->error=fabs(out->value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
    }
    return out->status;
}

#ifdef P14_HAVE_GSL
static int p14_run_gsl(p14_problem *p, p14_outcome *out)
{
    int status=GSL_EUNSUP;
    p14_outcome_init(out);
    p14_active_problem=p;
    gsl_set_error_handler_off();
    switch(p->module) {
    case P14_ROOT: {
        double f_lo=p14_root_value(p->lower,p), f_hi=p14_root_value(p->upper,p);
        if (p->model>=13 && p->model<=15) {
            gsl_root_fdfsolver *solver=gsl_root_fdfsolver_alloc(gsl_root_fdfsolver_newton);
            gsl_function_fdf fdf={p14_gsl_root_callback,p14_gsl_root_derivative,
                                  p14_gsl_root_fdf,p};
            double x=p->initial;
            status=solver ? gsl_root_fdfsolver_set(solver,&fdf,x) : GSL_ENOMEM;
            if(status==GSL_SUCCESS) {
                for(size_t it=0;it<10000;it++) {
                    double previous=x;
                    status=gsl_root_fdfsolver_iterate(solver);
                    out->iterations=it+1;
                    x=gsl_root_fdfsolver_root(solver);
                    if(status!=GSL_SUCCESS) break;
                    status=gsl_root_test_delta(x,previous,1e-10,1e-10);
                    if(status==GSL_SUCCESS) break;
                    status=GSL_CONTINUE;
                }
            }
            if(solver)gsl_root_fdfsolver_free(solver);
            out->value=x;
            snprintf(out->method,sizeof(out->method),"gsl_newton");
        } else {
            gsl_root_fsolver *solver=gsl_root_fsolver_alloc(gsl_root_fsolver_brent);
            gsl_function f={p14_gsl_root_callback,p};
            status=solver ? gsl_root_fsolver_set(solver,&f,p->lower,p->upper) : GSL_ENOMEM;
            if(status==GSL_SUCCESS) {
                for(size_t it=0;it<10000;it++) {
                    status=gsl_root_fsolver_iterate(solver);
                    if(status!=GSL_SUCCESS) break;
                    out->value=gsl_root_fsolver_root(solver);
                    out->iterations=it+1;
                    status=gsl_root_test_interval(gsl_root_fsolver_x_lower(solver),
                        gsl_root_fsolver_x_upper(solver),1e-10,1e-10);
                    if(status==GSL_SUCCESS)break;
                    if(status!=GSL_CONTINUE)break;
                }
                if(status==GSL_CONTINUE)status=GSL_EMAXITER;
            }
            if(solver)gsl_root_fsolver_free(solver);
            snprintf(out->method,sizeof(out->method),"gsl_brent");
            (void)f_lo; (void)f_hi;
        }
        out->error=fabs(out->value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
        out->residual=fabs(p14_root_value(out->value,p));
        break;
    }
    case P14_DIFFERENTIATION: {
        gsl_function f={p14_gsl_diff_callback,p};
        status=gsl_deriv_central(&f,p->x,p->h,&out->value,&out->estimate);
        out->error=fabs(out->value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
        snprintf(out->method,sizeof(out->method),"gsl_deriv_central");
        break;
    }
    case P14_LINEAR_ALGEBRA:
        status=p14_gsl_linear_solve(p,out);
        break;
    case P14_INTERPOLATION:
        status=p14_gsl_interpolation(p,out);
        break;
    case P14_INTEGRATION: {
        gsl_function f={p14_gsl_integral_callback,p};
        gsl_integration_workspace *w=gsl_integration_workspace_alloc(4096);
        if(w==NULL){status=GSL_ENOMEM;break;}
        status=gsl_integration_qag(&f,p->lower,p->upper,p->requested_abs_tolerance,
            p->requested_rel_tolerance,4096,GSL_INTEG_GAUSS61,w,&out->value,&out->estimate);
        out->iterations=w->size;
        gsl_integration_workspace_free(w);
        out->error=fabs(out->value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
        snprintf(out->method,sizeof(out->method),"gsl_qag_gauss_kronrod_61");
        break;
    }
    case P14_OPTIMIZATION: {
        const size_t grid=65;
        size_t best=0;
        double bestx=p->lower, bestf=HUGE_VAL;
        gsl_function f={p14_gsl_objective_callback,p};
        gsl_min_fminimizer *solver=NULL;
        for(size_t i=0;i<grid;i++) {
            double x=p->lower+(p->upper-p->lower)*(double)i/(double)(grid-1);
            double value=p14_optimization_value(x,p);
            if(value<bestf){bestf=value;bestx=x;best=i;}
        }
        if(best==0 || best+1==grid) {
            status=GSL_SUCCESS;
            out->value=bestx;
            snprintf(out->method,sizeof(out->method),"gsl_grid_boundary_minimum");
        } else {
            double lo=p->lower+(p->upper-p->lower)*(double)(best-1)/(double)(grid-1);
            double hi=p->lower+(p->upper-p->lower)*(double)(best+1)/(double)(grid-1);
            solver=gsl_min_fminimizer_alloc(gsl_min_fminimizer_goldensection);
            status=solver ? gsl_min_fminimizer_set(solver,&f,bestx,lo,hi) : GSL_ENOMEM;
            if(status==GSL_SUCCESS) {
                for(size_t it=0;it<10000;it++) {
                    status=gsl_min_fminimizer_iterate(solver);
                    if(status!=GSL_SUCCESS)break;
                    out->iterations=it+1;
                    double xlo=gsl_min_fminimizer_x_lower(solver);
                    double xhi=gsl_min_fminimizer_x_upper(solver);
                    status=gsl_min_test_interval(xlo,xhi,1e-9,1e-9);
                    if(status==GSL_SUCCESS)break;
                    if(status!=GSL_CONTINUE)break;
                }
                out->value=gsl_min_fminimizer_x_minimum(solver);
            }
            if(solver)gsl_min_fminimizer_free(solver);
        }
        out->error=fabs(out->value-p->reference[0]);
        out->secondary_error=fabs(p14_optimization_value(out->value,p)-p->reference[1]);
        out->reference_norm=fabs(p->reference[0]);
        snprintf(out->method,sizeof(out->method),"gsl_goldensection");
        break;
    }
    case P14_ODE: {
        gsl_odeiv2_system sys={p14_gsl_ode_callback,NULL,p->dimension,p};
        gsl_odeiv2_step *step=gsl_odeiv2_step_alloc(gsl_odeiv2_step_rk4,p->dimension);
        double y[8], yerr[8]={0};
        double t=p->t0;
        memcpy(y,p->initial_state,p->dimension*sizeof(double));
        status=step ? GSL_SUCCESS : GSL_ENOMEM;
        for(size_t i=0;status==GSL_SUCCESS && i<p->steps;i++) {
            status=gsl_odeiv2_step_apply(step,t,p->fixed_step,y,yerr,NULL,NULL,&sys);
            t+=p->fixed_step;
        }
        if(status==GSL_SUCCESS) {
            double e=0.0,ref=0.0;
            for(size_t i=0;i<p->dimension;i++){e=fmax(e,fabs(y[i]-p->reference[i]));ref=fmax(ref,fabs(p->reference[i]));}
            out->error=e;out->reference_norm=ref;out->rhs_evaluations=p14_rhs_calls;
        }
        if(step)gsl_odeiv2_step_free(step);
        snprintf(out->method,sizeof(out->method),"gsl_rk4_fixed");
        break;
    }
    default: break;
    }
    out->status=status;
    out->native_status=status;
    out->converged=(status==GSL_SUCCESS);
    out->function_evaluations=p14_function_calls;
    return status;
}
#endif

#ifdef P14_HAVE_CMINPACK
static int p14_run_cminpack(p14_problem *p, p14_outcome *out)
{
    double x[1]={p->initial};
    double fvec[1]={NAN};
    double wa[8]={0.0};
    int status;
    p14_outcome_init(out);
    if(p->module!=P14_ROOT)return NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    status=hybrd1(p14_cminpack_root_callback,p,1,x,fvec,1e-10,wa,8);
    out->value=x[0];
    out->error=fabs(x[0]-p->reference[0]);
    out->reference_norm=fabs(p->reference[0]);
    out->residual=fabs(p14_root_value(x[0],p));
    out->function_evaluations=p14_function_calls;
    out->native_status=status;
    out->status=(status==1) ? 0 : status;
    out->converged=(status==1);
    snprintf(out->method,sizeof(out->method),"cminpack_hybrd1_local");
    return out->status;
}
#endif

#ifdef P14_HAVE_NLOPT
static int p14_run_nlopt(p14_problem *p, p14_outcome *out)
{
    nlopt_opt optimizer;
    double x[1]={p->initial};
    double minimum=NAN;
    double xtol=p->absolute_tolerance+p->relative_tolerance*fabs(p->reference[0]);
    nlopt_result result;
    p14_outcome_init(out);
    if(p->module!=P14_OPTIMIZATION)return NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    optimizer=nlopt_create(NLOPT_GN_DIRECT_L,1);
    if(optimizer==NULL)return NUMERIX_STATUS_ALLOCATION_FAILURE;
    result=nlopt_set_lower_bounds1(optimizer,p->lower);
    if(result>=0)result=nlopt_set_upper_bounds1(optimizer,p->upper);
    if(result>=0)result=nlopt_set_min_objective(optimizer,p14_nlopt_objective,p);
    if(result>=0)result=nlopt_set_xtol_abs1(optimizer,xtol);
    if(result>=0)result=nlopt_set_maxeval(optimizer,4000);
    if(result>=0)result=nlopt_optimize(optimizer,x,&minimum);
    out->value=x[0];
    out->error=fabs(x[0]-p->reference[0]);
    out->secondary_error=fabs(p14_optimization_value(x[0],p)-p->reference[1]);
    out->reference_norm=fabs(p->reference[0]);
    out->function_evaluations=p14_function_calls;
    out->iterations=0;
    out->native_status=(int)result;
    out->converged=(result==NLOPT_SUCCESS || result==NLOPT_STOPVAL_REACHED ||
                    result==NLOPT_FTOL_REACHED || result==NLOPT_XTOL_REACHED);
    out->status=out->converged ? 0 : NUMERIX_STATUS_CONVERGENCE_FAILURE;
    snprintf(out->method,sizeof(out->method),"nlopt_direct_l");
    nlopt_destroy(optimizer);
    return out->status;
}
#endif

#ifdef P14_HAVE_LAPACKE
static int p14_run_lapack(p14_problem *p, p14_outcome *out)
{
    double *a;
    double *b;
    lapack_int *pivot;
    lapack_int info;
    double e=0.0, residual=0.0, ref=0.0;
    p14_outcome_init(out);
    if(p->module!=P14_LINEAR_ALGEBRA)return NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    a=(double *)malloc(p->n*p->n*sizeof(*a));
    b=(double *)malloc(p->n*sizeof(*b));
    pivot=(lapack_int *)malloc(p->n*sizeof(*pivot));
    if(a==NULL||b==NULL||pivot==NULL){free(a);free(b);free(pivot);return NUMERIX_STATUS_ALLOCATION_FAILURE;}
    memcpy(a,p->matrix,p->n*p->n*sizeof(*a));
    memcpy(b,p->rhs,p->n*sizeof(*b));
    info=LAPACKE_dgesv(LAPACK_ROW_MAJOR,(lapack_int)p->n,1,a,(lapack_int)p->n,
                       pivot,b,1);
    if(info==0) {
        for(size_t i=0;i<p->n;i++) {
            e=fmax(e,fabs(b[i]-p->x_reference[i]));
            ref=fmax(ref,fabs(p->x_reference[i]));
            double r=-p->rhs[i];
            for(size_t j=0;j<p->n;j++)r+=p->matrix[i*p->n+j]*b[j];
            residual=fmax(residual,fabs(r));
        }
        out->error=e;out->reference_norm=ref;out->residual=residual;out->secondary_error=residual;
    }
    out->value=0.0;
    out->status=(int)info;
    out->native_status=(int)info;
    out->converged=(info==0);
#ifdef P14_LAPACK_BACKEND_OPENBLAS
    snprintf(out->method,sizeof(out->method),"lapacke_dgesv_openblas_row_major");
#else
    snprintf(out->method,sizeof(out->method),"lapacke_dgesv_reference_row_major");
#endif
    free(a);free(b);free(pivot);
    return out->status;
}
#endif

#ifdef P14_HAVE_SUNDIALS
static int p14_sundials_rhs(sunrealtype t, N_Vector y, N_Vector dydt, void *data)
{
    double *yp=N_VGetArrayPointer(y);
    double *dp=N_VGetArrayPointer(dydt);
    return p14_ode_rhs((double)t,yp,dp,(p14_problem *)data);
}

static int p14_run_sundials(p14_problem *p, p14_outcome *out, int adaptive)
{
    SUNContext context=NULL;
    SUNNonlinearSolver nonlinear_solver=NULL;
    N_Vector y=NULL;
    void *memory=NULL;
    ARKodeButcherTable table=NULL;
    sunrealtype reached=p->t0;
    int status=-1;
    double final_y[8]={0};
    p14_outcome_init(out);
    if(p->module!=P14_ODE)return NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    if(SUNContext_Create(SUN_COMM_NULL,&context)!=SUN_SUCCESS)return NUMERIX_STATUS_ALLOCATION_FAILURE;
    y=N_VNew_Serial((sunindextype)p->dimension,context);
    if(y==NULL){SUNContext_Free(&context);return NUMERIX_STATUS_ALLOCATION_FAILURE;}
    memcpy(N_VGetArrayPointer(y),p->initial_state,p->dimension*sizeof(double));
    if(adaptive) {
        memory=CVodeCreate(CV_ADAMS,context);
        nonlinear_solver=SUNNonlinSol_FixedPoint(y,0,context);
        if(memory!=NULL && nonlinear_solver!=NULL) {
            status=CVodeInit(memory,p14_sundials_rhs,p->t0,y);
            if(status==0)status=CVodeSetNonlinearSolver(memory,nonlinear_solver);
        }
        if(status==0)status=CVodeSStolerances(memory,p->requested_rel_tolerance,
                                             p->requested_abs_tolerance);
        if(status==0)status=CVodeSetUserData(memory,p);
        if(status==0)status=CVodeSetMaxNumSteps(memory,200000);
        if(status==0)status=CVode(memory,p->tf,y,&reached,CV_NORMAL);
        if(status>=0) {
            long int nfe=0;
            CVodeGetNumRhsEvals(memory,&nfe);
            out->rhs_evaluations=(size_t)nfe;
            snprintf(out->method,sizeof(out->method),"sundials_cvode_adams_adaptive");
        }
    } else {
        memory=ERKStepCreate(p14_sundials_rhs,p->t0,y,context);
        if(memory!=NULL) {
            table=ARKodeButcherTable_LoadERK(ARKODE_KUTTA_RK4a_4_4);
            status=table ? ERKStepSetTable(memory,table) : -1;
        }
        if(status==0)status=ARKodeSetUserData(memory,p);
        if(status==0)status=ARKodeSetFixedStep(memory,p->fixed_step);
        if(status==0)status=ARKodeSetMaxNumSteps(memory,(long int)(p->steps+8));
        if(status==0)status=ARKodeEvolve(memory,p->tf,y,&reached,ARK_NORMAL);
        if(status>=0) {
            long int nfe=0;
            ARKodeGetNumRhsEvals(memory,0,&nfe);
            out->rhs_evaluations=(size_t)nfe;
            snprintf(out->method,sizeof(out->method),"sundials_arkode_kutta_rk4a_fixed");
        }
    }
    if(status>=0) {
        memcpy(final_y,N_VGetArrayPointer(y),p->dimension*sizeof(double));
        double e=0.0,ref=0.0;
        for(size_t i=0;i<p->dimension;i++){e=fmax(e,fabs(final_y[i]-p->reference[i]));ref=fmax(ref,fabs(p->reference[i]));}
        out->error=e;out->reference_norm=ref;out->secondary_error=e;
    }
    out->native_status=status;
    out->status=(status>=0)?0:status;
    out->converged=(status==0 || status==1 || status==2);
    if(memory!=NULL) {
        if(adaptive)CVodeFree(&memory);else ARKodeFree(&memory);
    }
    if(nonlinear_solver!=NULL)SUNNonlinSolFree(nonlinear_solver);
    if(table!=NULL)ARKodeButcherTable_Free(table);
    N_VDestroy(y);
    SUNContext_Free(&context);
    return out->status;
}
#endif

static int p14_execute(const char *library, p14_problem *p, p14_outcome *out)
{
    p14_outcome_init(out);
    if(strcmp(library,"NUMERIX")==0)return p14_run_numerix(p,out);
#ifdef P14_HAVE_GSL
    if(strcmp(library,"GSL")==0)return p14_run_gsl(p,out);
#endif
#ifdef P14_HAVE_CMINPACK
    if(strcmp(library,"CMINPACK")==0)return p14_run_cminpack(p,out);
#endif
#ifdef P14_HAVE_NLOPT
    if(strcmp(library,"NLOPT")==0)return p14_run_nlopt(p,out);
#endif
#ifdef P14_HAVE_SUNDIALS
    if(strcmp(library,"SUNDIALS_CVODE")==0)return p14_run_sundials(p,out,1);
    if(strcmp(library,"SUNDIALS_ARKODE")==0)return p14_run_sundials(p,out,0);
#endif
#ifdef P14_HAVE_LAPACKE
#ifdef P14_LAPACK_BACKEND_REFERENCE
    if(strcmp(library,"LAPACK_REFERENCE")==0)return p14_run_lapack(p,out);
#endif
#ifdef P14_LAPACK_BACKEND_OPENBLAS
    if(strcmp(library,"LAPACK_OPENBLAS")==0)return p14_run_lapack(p,out);
#endif
#endif
    snprintf(out->method,sizeof(out->method),"not_built_or_not_eligible");
    out->status=NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    out->native_status=out->status;
    out->converged=0;
    return out->status;
}

static const char *p14_module_name(p14_module module)
{
    switch(module) {
    case P14_ROOT:return "root";
    case P14_LINEAR_ALGEBRA:return "linear_algebra";
    case P14_DIFFERENTIATION:return "differentiation";
    case P14_INTERPOLATION:return "interpolation";
    case P14_INTEGRATION:return "integration";
    case P14_OPTIMIZATION:return "optimization";
    case P14_ODE:return "ode";
    default:return "unknown";
    }
}

static const char *p14_library_version(const char *library)
{
    if(strcmp(library,"NUMERIX")==0)return NUMERIX_VERSION_STRING;
    if(strcmp(library,"GSL")==0)return "2.8";
    if(strcmp(library,"CMINPACK")==0)return "1.3.15";
    if(strcmp(library,"NLOPT")==0)return "2.11.0";
    if(strcmp(library,"SUNDIALS_CVODE")==0 || strcmp(library,"SUNDIALS_ARKODE")==0)return "7.9.0";
    if(strcmp(library,"LAPACK_REFERENCE")==0)return "3.12.1";
    if(strcmp(library,"LAPACK_OPENBLAS")==0)return "LAPACK 3.12.1 + OpenBLAS 0.3.34";
    return "unknown";
}

static const char *p14_comparison_mode(p14_module module,const char *library)
{
    if(module==P14_ROOT) return strcmp(library,"CMINPACK")==0 ? "best_available" : "algorithm_equivalent";
    if(module==P14_DIFFERENTIATION) return "best_available";
    if(module==P14_OPTIMIZATION)
        return "best_available";
    if(module==P14_ODE)
        return strcmp(library,"SUNDIALS_CVODE")==0 ? "best_available" : "algorithm_equivalent";
    return "best_available";
}

static const char *p14_error_kind(p14_module module)
{
    switch(module) {
    case P14_ROOT:return "root_location_absolute_error";
    case P14_LINEAR_ALGEBRA:return "forward_solution_infinity_error";
    case P14_DIFFERENTIATION:return "derivative_absolute_error";
    case P14_INTERPOLATION:return "max_validation_grid_absolute_error";
    case P14_INTEGRATION:return "integral_absolute_error";
    case P14_OPTIMIZATION:return "minimizer_location_absolute_error";
    case P14_ODE:return "final_state_infinity_error";
    default:return "unknown";
    }
}

static size_t p14_input_size(const p14_problem *p)
{
    if(p->module==P14_LINEAR_ALGEBRA)return p->n;
    if(p->module==P14_INTERPOLATION)return p->node_count;
    if(p->module==P14_ODE)return p->steps;
    return 1;
}

static int p14_status_is_finite(const p14_outcome *out)
{
    return isfinite(out->error);
}

static int p14_accuracy_passes(const p14_problem *p,const p14_outcome *out)
{
    return p14_status_is_finite(out) &&
        out->error<=p->absolute_tolerance+p->relative_tolerance*out->reference_norm;
}

static FILE *p14_append_csv(const char *path,const char *header)
{
    FILE *file=fopen(path,"a+");
    if(file==NULL)return NULL;
    if(fseek(file,0,SEEK_END)!=0){fclose(file);return NULL;}
    if(ftell(file)==0 && fprintf(file,"%s\n",header)<0){fclose(file);return NULL;}
    return file;
}

static void p14_write_work_metric(FILE *file,const char *run_id,
                                  const char *library,const p14_problem *problem,
                                  const p14_outcome *out,const char *metric,
                                  size_t value,int available)
{
    fprintf(file,"phase14-v1,%s,%s,%s,%s,%s,%s,%s,",
        run_id,library,p14_module_name(problem->module),problem->id,
        problem->family,out->method,metric);
    if(available)fprintf(file,"%zu,count,untimed_probe,measured\n",value);
    else fprintf(file,",count,untimed_probe,not_collected\n");
}

static void p14_write_work(FILE *file,const char *run_id,const char *library,
                           const p14_problem *problem,const p14_outcome *out)
{
    int iterative=problem->module==P14_ROOT || problem->module==P14_INTEGRATION ||
                  problem->module==P14_OPTIMIZATION;
    int function_count=problem->module==P14_ROOT ||
                       problem->module==P14_DIFFERENTIATION ||
                       problem->module==P14_INTERPOLATION ||
                       problem->module==P14_INTEGRATION ||
                       problem->module==P14_OPTIMIZATION;
    int rhs_count=problem->module==P14_ODE;
    int derivative_count=problem->module==P14_ROOT;
    if ((problem->module==P14_ROOT && strcmp(library,"CMINPACK")==0) ||
        (problem->module==P14_OPTIMIZATION && strcmp(library,"NLOPT")==0)) {
        iterative=0;
    }
    p14_write_work_metric(file,run_id,library,problem,out,"iterations",
                          out->iterations,iterative);
    p14_write_work_metric(file,run_id,library,problem,out,"function_evaluations",
                          out->function_evaluations,function_count);
    p14_write_work_metric(file,run_id,library,problem,out,"rhs_evaluations",
                          out->rhs_evaluations,rhs_count);
    p14_write_work_metric(file,run_id,library,problem,out,"derivative_evaluations",
                          p14_gradient_calls,derivative_count);
}

static int p14_write_accuracy_and_work(const char *output_directory,const char *run_id,
                                       const char *library,const p14_problem *problem,
                                       p14_outcome *out)
{
    char path[1024];
    FILE *accuracy,*work;
    double relative=out->error/fmax(out->reference_norm,DBL_MIN);
    int finite=p14_status_is_finite(out);
    int pass=p14_accuracy_passes(problem,out);
    snprintf(path,sizeof(path),"%s/accuracy.csv",output_directory);
    accuracy=p14_append_csv(path,"schema_version,run_id,library,library_version,module,problem_id,problem_family,method,comparison_mode,status_code,native_status,converged,is_finite,error_kind,absolute_error,relative_error,secondary_error,reference_norm,tolerance_abs,tolerance_rel,accuracy_pass,reported_error_estimate,residual");
    if(accuracy==NULL)return 0;
    fprintf(accuracy,"phase14-v1,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%s,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%d,%.17g,%.17g\n",
        run_id,library,p14_library_version(library),p14_module_name(problem->module),problem->id,
        problem->family,out->method,p14_comparison_mode(problem->module,library),out->status,
        out->native_status,out->converged,finite,p14_error_kind(problem->module),out->error,relative,
        out->secondary_error,out->reference_norm,problem->absolute_tolerance,
        problem->relative_tolerance,pass,out->estimate,out->residual);
    fclose(accuracy);

    snprintf(path,sizeof(path),"%s/work_metrics.csv",output_directory);
    work=p14_append_csv(path,"schema_version,run_id,library,module,problem_id,problem_family,method,metric,value,unit,measurement_phase,value_status");
    if(work==NULL)return 0;
    p14_write_work(work,run_id,library,problem,out);
    fclose(work);
    return 1;
}

static int p14_write_memory(const char *output_directory,const char *run_id,
                            const char *library,const p14_problem *problem)
{
    struct rusage usage;
    char path[1024];
    FILE *file;
    p14_outcome out;
    p14_count_work=1;
    p14_function_calls=p14_gradient_calls=p14_rhs_calls=p14_interpolation_calls=0;
    (void)p14_execute(library,(p14_problem *)problem,&out);
    if(getrusage(RUSAGE_SELF,&usage)!=0)return 0;
    snprintf(path,sizeof(path),"%s/memory.csv",output_directory);
    file=p14_append_csv(path,"schema_version,run_id,library,library_version,module,problem_id,problem_family,input_size,memory_type,peak_rss_kb,requested_heap_bytes,allocation_count,reallocation_count,free_count,measurement_method,measurement_scope");
    if(file==NULL)return 0;
    fprintf(file,"phase14-v1,%s,%s,%s,%s,%s,%s,%zu,peak_process_rss,%ld,,,,,getrusage_RUSAGE_SELF_isolated_process,process_peak_includes_harness_and_library_setup\n",
        run_id,library,p14_library_version(library),p14_module_name(problem->module),problem->id,
        problem->family,p14_input_size(problem),usage.ru_maxrss);
    fclose(file);
    return 1;
}

static int p14_append_timing(const char *output_directory,const char *run_id,
                             const char *library,const p14_problem *problem,
                             size_t batch_index,size_t batch_size,double wall,double cpu,
                             const p14_outcome *out)
{
    char path[1024];
    FILE *file;
    snprintf(path,sizeof(path),"%s/raw_timing.csv",output_directory);
    file=p14_append_csv(path,"schema_version,run_id,library,library_version,module,problem_id,problem_family,method,comparison_mode,measurement_type,input_size,batch_index,batch_size,wall_elapsed_seconds,cpu_elapsed_seconds,status_code,native_status,converged");
    if(file==NULL)return 0;
    fprintf(file,"phase14-v1,%s,%s,%s,%s,%s,%s,%s,%s,end_to_end,%zu,%zu,%zu,%.17g,%.17g,%d,%d,%d\n",
        run_id,library,p14_library_version(library),p14_module_name(problem->module),problem->id,
        problem->family,out->method,p14_comparison_mode(problem->module,library),p14_input_size(problem),
        batch_index,batch_size,wall,cpu,out->status,out->native_status,out->converged);
    fclose(file);
    return 1;
}

static int p14_run_selected(const char *input_path,const char *problem_id,
                            const char *library,const char *output_directory,
                            const char *run_id,size_t batches,size_t batch_size,
                            int memory_only)
{
    FILE *input;
    char *line=(char *)malloc(P14_LINE_CAPACITY);
    p14_problem problem;
    int found=0;
    if(line==NULL)return 2;
    input=fopen(input_path,"r");
    if(input==NULL){free(line);perror(input_path);return 2;}
    while(fgets(line,(int)P14_LINE_CAPACITY,input)!=NULL) {
        if(strstr(line,problem_id)==NULL)continue;
        if(!p14_parse_problem(line,&problem)) {
            fclose(input);free(line);fprintf(stderr,"invalid input record: %s\n",problem_id);return 2;
        }
        found=1;break;
    }
    fclose(input);free(line);
    if(!found){fprintf(stderr,"problem ID not found: %s\n",problem_id);return 2;}
    if(memory_only) {
        int ok=p14_write_memory(output_directory,run_id,library,&problem);
        p14_problem_destroy(&problem);
        return ok?0:2;
    }

    p14_count_work=1;
    p14_function_calls=p14_gradient_calls=p14_rhs_calls=p14_interpolation_calls=0;
    p14_outcome accurate;
    (void)p14_execute(library,&problem,&accurate);
    accurate.function_evaluations=accurate.function_evaluations ? accurate.function_evaluations : p14_function_calls;
    if(!p14_write_accuracy_and_work(output_directory,run_id,library,&problem,&accurate)) {
        p14_problem_destroy(&problem);return 2;
    }

    p14_count_work=0;
    for(size_t warm=0;warm<2;warm++) {
        p14_outcome ignored;
        p14_function_calls=p14_gradient_calls=p14_rhs_calls=p14_interpolation_calls=0;
        (void)p14_execute(library,&problem,&ignored);
    }
    for(size_t batch=1;batch<=batches;batch++) {
        p14_outcome current;
        double wall_start,cpu_start,wall,cpu;
        int combined_status=0;
        int all_converged=1;
        wall_start=p14_monotonic_seconds();
        cpu_start=p14_cpu_seconds();
        for(size_t repeat=0;repeat<batch_size;repeat++) {
            p14_function_calls=p14_gradient_calls=p14_rhs_calls=p14_interpolation_calls=0;
            (void)p14_execute(library,&problem,&current);
            if(current.status!=0)combined_status=current.status;
            if(!current.converged)all_converged=0;
        }
        wall=p14_monotonic_seconds()-wall_start;
        cpu=p14_cpu_seconds()-cpu_start;
        current.status=combined_status;
        current.converged=all_converged;
        if(!p14_append_timing(output_directory,run_id,library,&problem,batch,batch_size,
                              wall,cpu,&current)) {
            p14_problem_destroy(&problem);return 2;
        }
    }
    p14_problem_destroy(&problem);
    return 0;
}

int main(int argc,char **argv)
{
    size_t batches,batch_size;
    int memory_only=0;
    if(argc!=8 && argc!=9) {
        fprintf(stderr,"usage: %s INPUTS.jsonl PROBLEM_ID LIBRARY BATCHES BATCH_SIZE OUTPUT_DIR RUN_ID [--memory]\n",argv[0]);
        return 2;
    }
    if(argc==9) {
        if(strcmp(argv[8],"--memory")!=0){fprintf(stderr,"unknown option: %s\n",argv[8]);return 2;}
        memory_only=1;
    }
    batches=(size_t)strtoul(argv[4],NULL,10);
    batch_size=(size_t)strtoul(argv[5],NULL,10);
    if(memory_only){batches=0;batch_size=0;}
    if(!memory_only && (batches<21 || batch_size==0)) {
        fprintf(stderr,"timing runs require at least 21 batches and a positive batch size\n");return 2;
    }
    return p14_run_selected(argv[1],argv[2],argv[3],argv[6],argv[7],batches,batch_size,memory_only);
}

