#define _POSIX_C_SOURCE 200809L
#include <gsl/gsl_deriv.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_interp.h>
#include <gsl/gsl_version.h>
#include <math.h>
#include <numerix.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Small API/measurement smoke pilot only. It is not a benchmark study. */
typedef int (*call_fn)(void *, double *, long long *);
typedef struct {
    const char *library, *version, *method, *competitor, *class_name;
    const char *work_name;
    call_fn call;
    void *context;
} route;

static const char *run_id;
static const char *out_root;
static volatile double output_sink;

static double now(clockid_t clock_id)
{
    struct timespec ts;
    if (clock_gettime(clock_id, &ts) != 0) return NAN;
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static FILE *open_csv(const char *module, const char *filename)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/raw/%s/%s/%s", out_root, module, run_id, filename);
    return fopen(path, "w");
}

static void write_timing_header(FILE *f)
{
    fputs("run_id,module,problem_id,library,library_version,numerix_method,competitor_method,comparison_type,batch_index,batch_order,batch_size,warmup_count,wall_elapsed_seconds,cpu_elapsed_seconds,status,native_status,work_counter_name,work_counter\n", f);
}

static void write_accuracy_header(FILE *f)
{
    fputs("run_id,module,problem_id,library,library_version,method,competitor_method,comparison_type,native_status,status,finite_result,reference,reference_method,metric,absolute_error,relative_error,target_absolute,target_relative,pass,work_counter_name,work_counter\n", f);
}

static void write_accuracy(FILE *f, const char *module, const char *problem,
                           const char *library, const char *version,
                           const char *method, const char *competitor,
                           const char *classification, int native_status,
                           double value, double reference, const char *ref_method,
                           const char *metric, double atol, double rtol,
                           const char *work_name, long long work)
{
    int finite = isfinite(value);
    double absolute = finite ? fabs(value - reference) : NAN;
    double relative = finite && reference != 0.0 ? absolute / fabs(reference) : NAN;
    int passed = finite && native_status == 0 && absolute <= atol + rtol * fabs(reference);
    fprintf(f, "%s,%s,%s,%s,%s,%s,%s,%s,%d,%s,%d,%.17g,%s,%s,",
            run_id, module, problem, library, version, method, competitor,
            classification, native_status, native_status == 0 ? "success" : "failure",
            finite, reference, ref_method, metric);
    if (finite) fprintf(f, "%.17g,", absolute); else fputs(",", f);
    if (isfinite(relative)) fprintf(f, "%.17g,", relative); else fputs(",", f);
    fprintf(f, "%.17g,%.17g,%d,%s,%lld\n", atol, rtol, passed, work_name, work);
}

static void timed_batch(FILE *f, const char *module, const char *problem,
                        route *r, int batch_index, const char *order,
                        int batch_size, int warmups)
{
    double wall0 = now(CLOCK_MONOTONIC), cpu0 = now(CLOCK_PROCESS_CPUTIME_ID);
    double value = NAN;
    int native_status = 0;
    long long work_total = 0;
    int i;
    for (i = 0; i < batch_size; ++i) {
        long long work = 0;
        int status = r->call(r->context, &value, &work);
        if (status != 0 && native_status == 0) native_status = status;
        work_total += work;
        output_sink += isfinite(value) ? value : 0.0;
    }
    double cpu1 = now(CLOCK_PROCESS_CPUTIME_ID), wall1 = now(CLOCK_MONOTONIC);
    fprintf(f, "%s,%s,%s,%s,%s,%s,%s,%s,%d,%s,%d,%d,%.12g,%.12g,%s,%d,%s,%lld\n",
            run_id, module, problem, r->library, r->version, r->method,
            r->competitor, r->class_name, batch_index, order, batch_size,
            warmups, wall1 - wall0, cpu1 - cpu0,
            native_status == 0 ? "success" : "failure", native_status, r->work_name, work_total);
}

static void warm(route *r, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        double value = NAN;
        long long work = 0;
        (void)r->call(r->context, &value, &work);
        output_sink += isfinite(value) ? value : 0.0;
    }
}

static void measure_pair(FILE *f, const char *module, const char *problem,
                         route *num, route *comp, int batches, int batch_size,
                         int warmups)
{
    int b;
    warm(num, warmups);
    warm(comp, warmups);
    for (b = 0; b < batches; ++b) {
        if (b % 2 == 0) {
            timed_batch(f, module, problem, num, b + 1, "NUMERIX_first", batch_size, warmups);
            timed_batch(f, module, problem, comp, b + 1, "GSL_second", batch_size, warmups);
        } else {
            timed_batch(f, module, problem, comp, b + 1, "GSL_first", batch_size, warmups);
            timed_batch(f, module, problem, num, b + 1, "NUMERIX_second", batch_size, warmups);
        }
    }
}

typedef struct { int kind; long long calls; } function_context;
static double callback(double x, void *opaque)
{
    function_context *c = opaque;
    ++c->calls;
    if (c->kind == 0) return sin(x);
    if (c->kind == 1) return exp(0.2 * x);
    if (c->kind == 2) return x * x * x - 2.0 * x;
    if (c->kind == 3) return exp(x);
    return x * x * x * x;
}

typedef struct { const char *id; int kind; double x, h; } diff_problem;
typedef struct { diff_problem p; int method; } diff_context;
typedef struct { diff_problem p; int method; function_context fc; gsl_function gf; } gsl_diff_context;
static const char *diff_names[] = {"forward", "backward", "central", "five_point", "richardson", "second"};
static const char *gsl_diff_names[] = {"gsl_deriv_forward", "gsl_deriv_backward", "gsl_deriv_central", "gsl_deriv_central", "gsl_deriv_central", "three_point_second_derivative_wrapper"};
static const char *diff_classes[] = {"best_available", "best_available", "best_available", "best_available", "best_available", "best_available_wrapper"};

static double derivative_reference(diff_problem p, int second)
{
    if (p.kind == 0) return second ? -sin(p.x) : cos(p.x);
    if (p.kind == 1) return second ? 0.04 * exp(0.2 * p.x) : 0.2 * exp(0.2 * p.x);
    return second ? 6.0 * p.x : 3.0 * p.x * p.x - 2.0;
}

static int run_num_diff(void *opaque, double *value, long long *work)
{
    diff_context *c = opaque;
    function_context fc = {c->p.kind, 0};
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status s;
    numerix_options_init(&options);
    switch (c->method) {
        case 0: s = numerix_diff_forward(callback, &fc, c->p.x, c->p.h, &options, &result, &error); break;
        case 1: s = numerix_diff_backward(callback, &fc, c->p.x, c->p.h, &options, &result, &error); break;
        case 2: s = numerix_diff_central(callback, &fc, c->p.x, c->p.h, &options, &result, &error); break;
        case 3: s = numerix_diff_central_five_point(callback, &fc, c->p.x, c->p.h, &options, &result, &error); break;
        case 4: s = numerix_diff_richardson(callback, &fc, c->p.x, c->p.h, &options, &result, &error); break;
        default: s = numerix_diff_second(callback, &fc, c->p.x, c->p.h, &options, &result, &error); break;
    }
    *value = result.value;
    *work = (long long)result.function_evaluations;
    return (int)s;
}

static int run_gsl_diff(void *opaque, double *value, long long *work)
{
    gsl_diff_context *c = opaque;
    double abs_error = NAN;
    int status;
    c->fc.calls = 0;
    if (c->method == 0) status = gsl_deriv_forward(&c->gf, c->p.x, c->p.h, value, &abs_error);
    else if (c->method == 1) status = gsl_deriv_backward(&c->gf, c->p.x, c->p.h, value, &abs_error);
    else if (c->method < 5) status = gsl_deriv_central(&c->gf, c->p.x, c->p.h, value, &abs_error);
    else {
        double fm = callback(c->p.x - c->p.h, &c->fc);
        double f0 = callback(c->p.x, &c->fc);
        double fp = callback(c->p.x + c->p.h, &c->fc);
        *value = (fp - 2.0 * f0 + fm) / (c->p.h * c->p.h);
        status = isfinite(*value) ? GSL_SUCCESS : GSL_EFAILED;
    }
    *work = c->fc.calls;
    return status;
}

static int run_differentiation(FILE *timing, FILE *accuracy)
{
    static const diff_problem problems[] = {
        {"D-PILOT-SIN-01", 0, 0.7, 0.01},
        {"D-PILOT-EXP-01", 1, 1.2, 0.005},
        {"D-PILOT-CUBIC-01", 2, 0.3, 0.002}
    };
    size_t p;
    int m;
    write_timing_header(timing);
    write_accuracy_header(accuracy);
    for (p = 0; p < sizeof(problems) / sizeof(problems[0]); ++p) {
        for (m = 0; m < 6; ++m) {
            diff_context nctx = {problems[p], m};
            gsl_diff_context gctx;
            route nr = {"NUMERIX", NUMERIX_VERSION_STRING, diff_names[m], gsl_diff_names[m], diff_classes[m], "function_evaluations", run_num_diff, &nctx};
            route gr = {"GSL", GSL_VERSION, diff_names[m], gsl_diff_names[m], diff_classes[m], "callback_evaluations", run_gsl_diff, &gctx};
            double nvalue = NAN, gvalue = NAN;
            long long nwork = 0, gwork = 0;
            int ns, gs;
            gctx.p = problems[p]; gctx.method = m;
            gctx.fc.kind = problems[p].kind; gctx.fc.calls = 0;
            gctx.gf.function = callback; gctx.gf.params = &gctx.fc;
            measure_pair(timing, "differentiation", problems[p].id, &nr, &gr, 31, 512, 24);
            ns = run_num_diff(&nctx, &nvalue, &nwork);
            gs = run_gsl_diff(&gctx, &gvalue, &gwork);
            {
                int second = m == 5;
                double ref = derivative_reference(problems[p], second);
                write_accuracy(accuracy, "differentiation", problems[p].id, "NUMERIX", NUMERIX_VERSION_STRING,
                    diff_names[m], gsl_diff_names[m], diff_classes[m], ns, nvalue, ref,
                    second ? "analytic second derivative" : "analytic first derivative", "absolute derivative error", 1e-5, 1e-5, "function_evaluations", nwork);
                write_accuracy(accuracy, "differentiation", problems[p].id, "GSL", GSL_VERSION,
                    gsl_diff_names[m], gsl_diff_names[m], diff_classes[m], gs, gvalue, ref,
                    second ? "analytic second derivative" : "analytic first derivative", "absolute derivative error", 1e-5, 1e-5, "callback_evaluations", gwork);
            }
        }
    }
    return 0;
}

typedef struct { const char *id; int kind; size_t n; double x[5], y[5], q, reference; } interp_problem;
enum { I_LINEAR, I_LAGRANGE, I_NEWTON_SETUP, I_NEWTON_EVAL };
static const char *interp_names[] = {"linear", "lagrange", "newton_create", "newton_evaluate"};
static const char *gsl_interp_names[] = {"gsl_interp_linear", "gsl_interp_polynomial", "gsl_interp_polynomial_init", "gsl_interp_eval_polynomial"};
static const char *interp_classes[] = {"direct_functional", "best_available", "best_available", "best_available"};

static double interp_function(int kind, double x)
{
    if (kind == 0) return x * x * x - 2.0 * x + 1.0;
    if (kind == 1) return exp(x);
    return 1.0 / (1.0 + x * x);
}

typedef struct { interp_problem *p; int method; numerix_interpolant newton; } interp_context;
typedef struct { interp_problem *p; int method; gsl_interp *interp; gsl_interp_accel *accel; double linear_x[2], linear_y[2]; } gsl_interp_context;

static int run_num_interp(void *opaque, double *value, long long *work)
{
    interp_context *c = opaque;
    numerix_result r;
    numerix_error e;
    numerix_status s;
    interp_problem *p = c->p;
    if (c->method == I_NEWTON_SETUP) {
        numerix_interpolant temporary = {0};
        s = numerix_interp_newton_create(p->x, p->y, p->n, &temporary, &e);
        numerix_interpolant_destroy(&temporary);
        *value = NAN; *work = (long long)p->n; return (int)s;
    }
    if (c->method == I_LINEAR)
        s = numerix_interp_linear(p->x[0], p->y[0], p->x[p->n - 1], p->y[p->n - 1], p->q, &r, &e);
    else if (c->method == I_LAGRANGE)
        s = numerix_interp_lagrange(p->x, p->y, p->n, p->q, 0, &r, &e);
    else
        s = numerix_interp_newton_evaluate(&c->newton, p->q, 0, &r, &e);
    *value = r.value; *work = (long long)(c->method == I_LINEAR ? 2 : p->n); return (int)s;
}

static int init_gsl_interp(gsl_interp_context *c)
{
    interp_problem *p = c->p;
    const gsl_interp_type *type = c->method == I_LINEAR ? gsl_interp_linear : gsl_interp_polynomial;
    size_t n = p->n;
    const double *x = p->x, *y = p->y;
    if (c->method == I_LINEAR) {
        n = 2; c->linear_x[0] = p->x[0]; c->linear_x[1] = p->x[p->n - 1];
        c->linear_y[0] = p->y[0]; c->linear_y[1] = p->y[p->n - 1];
        x = c->linear_x; y = c->linear_y;
    }
    c->interp = gsl_interp_alloc(type, n);
    if (c->interp == NULL) return GSL_ENOMEM;
    {
        int s = gsl_interp_init(c->interp, x, y, n);
        if (s != GSL_SUCCESS) { gsl_interp_free(c->interp); c->interp = NULL; return s; }
    }
    if (c->method != I_NEWTON_SETUP) {
        c->accel = gsl_interp_accel_alloc();
        if (c->accel == NULL) { gsl_interp_free(c->interp); c->interp = NULL; return GSL_ENOMEM; }
    }
    return GSL_SUCCESS;
}

static void destroy_gsl_interp(gsl_interp_context *c)
{
    if (c->accel) { gsl_interp_accel_free(c->accel); c->accel = NULL; }
    if (c->interp) { gsl_interp_free(c->interp); c->interp = NULL; }
}

static int run_gsl_interp(void *opaque, double *value, long long *work)
{
    gsl_interp_context *c = opaque;
    int s;
    if (c->method == I_NEWTON_SETUP) {
        gsl_interp_context temp = *c;
        temp.interp = NULL; temp.accel = NULL;
        s = init_gsl_interp(&temp);
        destroy_gsl_interp(&temp);
        *value = NAN; *work = (long long)c->p->n; return s;
    }
    {
        double *x = c->method == I_LINEAR ? c->linear_x : c->p->x;
        double *y = c->method == I_LINEAR ? c->linear_y : c->p->y;
        size_t n = c->method == I_LINEAR ? 2 : c->p->n;
        s = gsl_interp_eval_e(c->interp, x, y, c->p->q, c->accel, value);
        *work = (long long)n;
    }
    return s;
}

static int run_interpolation(FILE *timing, FILE *accuracy)
{
    interp_problem p[3] = {
        {"I-PILOT-CUBIC-01", 0, 4, {-1,0,1,2,0}, {0,1,0,5,0}, 0.35, 0},
        {"I-PILOT-EXP-01", 1, 5, {-1,-0.5,0.5,1,1.5}, {0}, 0.2, 0},
        {"I-PILOT-RATIONAL-01", 2, 5, {-1,-0.5,0,0.5,1}, {0}, 0.25, 0}
    };
    size_t k, j;
    int m;
    for (k = 0; k < 3; ++k) {
        for (j = 0; j < p[k].n; ++j) p[k].y[j] = interp_function(p[k].kind, p[k].x[j]);
        p[k].reference = interp_function(p[k].kind, p[k].q);
    }
    write_timing_header(timing); write_accuracy_header(accuracy);
    for (k = 0; k < 3; ++k) {
        for (m = 0; m < 4; ++m) {
            interp_context nctx = {&p[k], m, {0}};
            gsl_interp_context gctx = {&p[k], m, NULL, NULL, {0}, {0}};
            route nr = {"NUMERIX", NUMERIX_VERSION_STRING, interp_names[m], gsl_interp_names[m], interp_classes[m], "nodes_used", run_num_interp, &nctx};
            route gr = {"GSL", GSL_VERSION, interp_names[m], gsl_interp_names[m], interp_classes[m], "nodes_used", run_gsl_interp, &gctx};
            double nv = NAN, gv = NAN; long long nw = 0, gw = 0; int ns, gs;
            if (m == I_NEWTON_EVAL && numerix_interp_newton_create(p[k].x, p[k].y, p[k].n, &nctx.newton, NULL) != NUMERIX_STATUS_SUCCESS) return 1;
            if (m != I_NEWTON_SETUP && init_gsl_interp(&gctx) != GSL_SUCCESS) return 2;
            measure_pair(timing, "interpolation", p[k].id, &nr, &gr, 31, m == I_NEWTON_SETUP ? 64 : 512, 16);
            ns = run_num_interp(&nctx, &nv, &nw); gs = run_gsl_interp(&gctx, &gv, &gw);
            if (m != I_NEWTON_SETUP) {
                write_accuracy(accuracy, "interpolation", p[k].id, "NUMERIX", NUMERIX_VERSION_STRING,
                    interp_names[m], gsl_interp_names[m], interp_classes[m], ns, nv, p[k].reference,
                    "analytic function evaluated independently at query", "absolute interpolation error", 1e-7, 1e-7, "nodes_used", nw);
                write_accuracy(accuracy, "interpolation", p[k].id, "GSL", GSL_VERSION,
                    gsl_interp_names[m], gsl_interp_names[m], interp_classes[m], gs, gv, p[k].reference,
                    "analytic function evaluated independently at query", "absolute interpolation error", 1e-7, 1e-7, "nodes_used", gw);
            }
            if (m == I_NEWTON_EVAL) numerix_interpolant_destroy(&nctx.newton);
            destroy_gsl_interp(&gctx);
        }
    }
    return 0;
}

typedef struct { const char *id; int kind; double a, b, reference; } integral_problem;
enum { INT_TRAPEZOID, INT_SIMPSON, INT_ADAPTIVE };
static const char *integration_names[] = {"composite_trapezoid", "composite_simpson", "adaptive_simpson"};
static const char *gsl_integral_names[] = {"gsl_integration_glfixed_65", "gsl_integration_glfixed_65", "gsl_integration_qag_61"};
static const char *integration_classes[] = {"best_available", "best_available", "best_available"};

typedef struct { integral_problem p; int method; numerix_options options; } integral_context;
typedef struct {
    integral_problem p; int method; int end_to_end; function_context fc; gsl_function gf;
    gsl_integration_glfixed_table *fixed; gsl_integration_workspace *workspace;
} gsl_integral_context;

static int run_num_integral(void *opaque, double *value, long long *work)
{
    integral_context *c = opaque;
    numerix_result r; numerix_error e; numerix_status s;
    if (c->method == INT_TRAPEZOID)
        s = numerix_integrate_trapezoid(callback, &(function_context){c->p.kind,0}, c->p.a, c->p.b, 64, &c->options, &r, &e);
    else if (c->method == INT_SIMPSON)
        s = numerix_integrate_simpson(callback, &(function_context){c->p.kind,0}, c->p.a, c->p.b, 64, &c->options, &r, &e);
    else
        s = numerix_integrate_adaptive_simpson(callback, &(function_context){c->p.kind,0}, c->p.a, c->p.b, &c->options, 20, &r, &e);
    *value = r.value; *work = (long long)r.function_evaluations; return (int)s;
}

static int call_gsl_integral(gsl_integral_context *c, double *value, long long *work)
{
    c->fc.calls = 0;
    int status;
    if (c->method == INT_ADAPTIVE) {
        double error = NAN;
        status = gsl_integration_qag(&c->gf, c->p.a, c->p.b, 1e-10, 1e-10,
            128, GSL_INTEG_GAUSS61, c->workspace, value, &error);
    } else {
        *value = gsl_integration_glfixed(&c->gf, c->p.a, c->p.b, c->fixed);
        status = isfinite(*value) ? GSL_SUCCESS : GSL_EFAILED;
    }
    *work = c->fc.calls;
    return status;
}

static int run_gsl_integral(void *opaque, double *value, long long *work)
{
    gsl_integral_context *c = opaque;
    if (!c->end_to_end) return call_gsl_integral(c, value, work);
    if (c->method == INT_ADAPTIVE) {
        gsl_integration_workspace *workspace = gsl_integration_workspace_alloc(128);
        int status;
        if (!workspace) { *value = NAN; *work = 0; return GSL_ENOMEM; }
        c->fc.calls = 0;
        {
            double error = NAN;
            status = gsl_integration_qag(&c->gf, c->p.a, c->p.b, 1e-10, 1e-10,
                128, GSL_INTEG_GAUSS61, workspace, value, &error);
        }
        *work = c->fc.calls;
        gsl_integration_workspace_free(workspace);
        return status;
    } else {
        gsl_integration_glfixed_table *table = gsl_integration_glfixed_table_alloc(65);
        int status;
        if (!table) { *value = NAN; *work = 0; return GSL_ENOMEM; }
        c->fc.calls = 0;
        *value = gsl_integration_glfixed(&c->gf, c->p.a, c->p.b, table);
        *work = c->fc.calls;
        status = isfinite(*value) ? GSL_SUCCESS : GSL_EFAILED;
        gsl_integration_glfixed_table_free(table);
        return status;
    }
}

static void init_gsl_integral(gsl_integral_context *c)
{
    c->fc.kind = c->p.kind; c->fc.calls = 0;
    c->gf.function = callback; c->gf.params = &c->fc;
    if (c->method == INT_ADAPTIVE) c->workspace = gsl_integration_workspace_alloc(128);
    else c->fixed = gsl_integration_glfixed_table_alloc(65);
}

static void destroy_gsl_integral(gsl_integral_context *c)
{
    if (c->fixed) { gsl_integration_glfixed_table_free(c->fixed); c->fixed = NULL; }
    if (c->workspace) { gsl_integration_workspace_free(c->workspace); c->workspace = NULL; }
}

static int run_integration(FILE *timing, FILE *accuracy)
{
    static const integral_problem p[] = {
        {"Q-PILOT-SIN-01", 0, 0.0, 3.14159265358979323846, 2.0},
        {"Q-PILOT-EXP-01", 3, 0.0, 1.0, 1.71828182845904523536},
        {"Q-PILOT-X4-01", 4, -1.0, 1.0, 0.4}
    };
    size_t k; int m;
    write_timing_header(timing); write_accuracy_header(accuracy);
    for (k = 0; k < sizeof(p) / sizeof(p[0]); ++k) {
        for (m = 0; m < 3; ++m) {
            integral_context nctx;
            gsl_integral_context gctx = {p[k], m, 0, {0,0}, {NULL,NULL}, NULL, NULL};
            gsl_integral_context gctx_e2e = {p[k], m, 1, {0,0}, {NULL,NULL}, NULL, NULL};
            route nr = {"NUMERIX", NUMERIX_VERSION_STRING, integration_names[m], gsl_integral_names[m], integration_classes[m], "function_evaluations", run_num_integral, &nctx};
            route gr = {"GSL", GSL_VERSION, integration_names[m], gsl_integral_names[m], integration_classes[m], "callback_evaluations", run_gsl_integral, &gctx};
            route gr_e2e = {"GSL", GSL_VERSION, integration_names[m], m == INT_ADAPTIVE ? "gsl_integration_qag_61_end_to_end" : "gsl_integration_glfixed_65_end_to_end", integration_classes[m], "callback_evaluations", run_gsl_integral, &gctx_e2e};
            double nv = NAN, gv = NAN; long long nw = 0, gw = 0; int ns, gs;
            nctx.p = p[k]; nctx.method = m;
            numerix_options_init(&nctx.options);
            nctx.options.absolute_tolerance = 1e-10;
            nctx.options.relative_tolerance = 1e-10;
            init_gsl_integral(&gctx);
            gctx_e2e.fc.kind = p[k].kind;
            gctx_e2e.gf.function = callback;
            gctx_e2e.gf.params = &gctx_e2e.fc;
            if ((!gctx.workspace && m == INT_ADAPTIVE) || (!gctx.fixed && m != INT_ADAPTIVE)) return 3;
            measure_pair(timing, "integration", p[k].id, &nr, &gr, 31, m == INT_ADAPTIVE ? 64 : 128, 8);
            nr.competitor = m == INT_ADAPTIVE ? "gsl_integration_qag_61_end_to_end" : "gsl_integration_glfixed_65_end_to_end";
            measure_pair(timing, "integration", p[k].id, &nr, &gr_e2e, 31, m == INT_ADAPTIVE ? 64 : 128, 8);
            nr.competitor = gsl_integral_names[m];
            ns = run_num_integral(&nctx, &nv, &nw); gs = run_gsl_integral(&gctx, &gv, &gw);
            write_accuracy(accuracy, "integration", p[k].id, "NUMERIX", NUMERIX_VERSION_STRING,
                integration_names[m], gsl_integral_names[m], integration_classes[m], ns, nv, p[k].reference,
                "analytic integral", "absolute integral error", 1e-7, 1e-7, "function_evaluations", nw);
            write_accuracy(accuracy, "integration", p[k].id, "GSL", GSL_VERSION,
                gsl_integral_names[m], gsl_integral_names[m], integration_classes[m], gs, gv, p[k].reference,
                "analytic integral", "absolute integral error", 1e-7, 1e-7, "callback_evaluations", gw);
            destroy_gsl_integral(&gctx); destroy_gsl_integral(&gctx_e2e);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    FILE *d_t, *d_a, *i_t, *i_a, *q_t, *q_a;
    gsl_set_error_handler_off();
    if (argc != 3) { fprintf(stderr, "usage: %s RUN_ID RESULTS_MODULE_COMPARISONS_DIR\n", argv[0]); return 2; }
    run_id = argv[1]; out_root = argv[2];
    d_t = open_csv("differentiation", "raw_timing.csv"); d_a = open_csv("differentiation", "raw_accuracy.csv");
    i_t = open_csv("interpolation", "raw_timing.csv"); i_a = open_csv("interpolation", "raw_accuracy.csv");
    q_t = open_csv("integration", "raw_timing.csv"); q_a = open_csv("integration", "raw_accuracy.csv");
    if (!d_t || !d_a || !i_t || !i_a || !q_t || !q_a) { perror("open pilot raw output"); return 3; }
    if (run_differentiation(d_t, d_a) || run_interpolation(i_t, i_a) || run_integration(q_t, q_a)) return 4;
    fclose(d_t); fclose(d_a); fclose(i_t); fclose(i_a); fclose(q_t); fclose(q_a);
    return output_sink == -HUGE_VAL ? 5 : 0;
}


