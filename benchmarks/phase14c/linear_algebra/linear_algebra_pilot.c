#define _GNU_SOURCE
#include "numerix.h"

#include <cblas.h>
#include <lapacke.h>
#include <openblas_config.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern void openblas_set_num_threads(int num_threads);
extern int openblas_get_num_threads(void);

#define TIMING_BATCHES 31
#define WARMUP_CALLS 6
#define TARGET_BATCH_NS 2000000.0
#define MAX_BATCH_CALLS 10000U
#define ABS_TARGET 1.0e-12
#define REL_TARGET 1.0e-12

typedef enum operation {
    OP_ADD,
    OP_GEMM,
    OP_LU_FACTOR,
    OP_LU_SOLVE
} operation;

typedef struct problem {
    const char *id;
    const char *family;
    const char *definition;
    operation op;
    int m;
    int n;
    int k;
    double *a;
    double *b;
    double *reference;
    double *column_a;
    numerix_matrix matrix_a;
    numerix_matrix matrix_b;
    numerix_vector rhs;
} problem;

typedef struct route route;
typedef int (*route_call)(problem *, double *);
struct route {
    const char *library;
    const char *version;
    const char *numerix_method;
    const char *competitor_method;
    const char *comparison_class;
    const char *timing_scope;
    route_call call;
};

static const char *run_id;
static FILE *timing_csv;
static FILE *accuracy_csv;
static FILE *setup_csv;
static double sink_value;
static uint64_t random_state = UINT64_C(2026111201);
static numerix_lu solve_num_factors;

static void csv_write_quoted(FILE *stream, const char *value)
{
    fputc('"', stream);
    for (const char *cursor = value; *cursor != '\0'; ++cursor) {
        if (*cursor == '"') fputc('"', stream);
        fputc(*cursor, stream);
    }
    fputc('"', stream);
}

static uint64_t random_u64(void)
{
    uint64_t x = random_state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    random_state = x;
    return x;
}

static double now_seconds(clockid_t clock_id)
{
    struct timespec value;

    if (clock_gettime(clock_id, &value) != 0) {
        return NAN;
    }
    return (double)value.tv_sec + (double)value.tv_nsec * 1.0e-9;
}

static int all_finite(const double *values, size_t count)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        if (!isfinite(values[index])) {
            return 0;
        }
    }
    return 1;
}

static int run_numerix_add(problem *p, double *checksum)
{
    numerix_matrix *out = NULL;
    numerix_error error;
    numerix_status status = numerix_matrix_add(&p->matrix_a, &p->matrix_b,
                                               &out, &error);

    if (status == NUMERIX_STATUS_SUCCESS) {
        *checksum += out->data[0];
        numerix_matrix_destroy(out);
    }
    return (int)status;
}

static int run_openblas_add(problem *p, double *checksum)
{
    size_t count = (size_t)p->m * (size_t)p->n;
    double *out;

    if (!all_finite(p->a, count) || !all_finite(p->b, count)) {
        return -1001;
    }
    out = (double *)malloc(count * sizeof(*out));
    if (out == NULL) {
        return -1002;
    }
    cblas_dcopy((int)count, p->a, 1, out, 1);
    cblas_daxpy((int)count, 1.0, p->b, 1, out, 1);
    if (!all_finite(out, count)) {
        free(out);
        return -1003;
    }
    *checksum += out[0];
    free(out);
    return 0;
}

static int run_numerix_gemm(problem *p, double *checksum)
{
    numerix_matrix *out = NULL;
    numerix_error error;
    numerix_status status = numerix_matrix_multiply(&p->matrix_a,
                                                    &p->matrix_b,
                                                    &out, &error);

    if (status == NUMERIX_STATUS_SUCCESS) {
        *checksum += out->data[0];
        numerix_matrix_destroy(out);
    }
    return (int)status;
}

static int run_openblas_gemm(problem *p, double *checksum)
{
    size_t count = (size_t)p->m * (size_t)p->n;
    double *out;

    if (!all_finite(p->a, (size_t)p->m * (size_t)p->k) ||
        !all_finite(p->b, (size_t)p->k * (size_t)p->n)) {
        return -1001;
    }
    out = (double *)calloc(count, sizeof(*out));
    if (out == NULL) {
        return -1002;
    }
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                p->m, p->n, p->k, 1.0,
                p->a, p->k, p->b, p->n, 0.0, out, p->n);
    if (!all_finite(out, count)) {
        free(out);
        return -1003;
    }
    *checksum += out[0];
    free(out);
    return 0;
}

static int run_numerix_lu_factor(problem *p, double *checksum)
{
    numerix_lu lu = {0};
    numerix_error error;
    numerix_status status = numerix_lu_decompose(&p->matrix_a, NULL, &lu,
                                                 &error);

    if (status == NUMERIX_STATUS_SUCCESS) {
        *checksum += lu.data[0] + (double)lu.permutation[0];
        numerix_lu_destroy(&lu);
    }
    return (int)status;
}

static int run_lapack_lu_factor(problem *p, double *checksum)
{
    const size_t count = (size_t)p->n * (size_t)p->n;
    double *factors;
    lapack_int *pivots;
    lapack_int info;
    int row;
    int column;

    if (!all_finite(p->a, count)) {
        return -1001;
    }
    factors = (double *)malloc(count * sizeof(*factors));
    pivots = (lapack_int *)malloc((size_t)p->n * sizeof(*pivots));
    if (factors == NULL || pivots == NULL) {
        free(factors);
        free(pivots);
        return -1002;
    }
    /* Explicit, timed row-major -> column-major conversion. */
    for (row = 0; row < p->n; ++row) {
        for (column = 0; column < p->n; ++column) {
            factors[(size_t)row + (size_t)column * (size_t)p->n] =
                p->a[(size_t)row * (size_t)p->n + (size_t)column];
        }
    }
    info = LAPACKE_dgetrf(LAPACK_COL_MAJOR, (lapack_int)p->n,
                          (lapack_int)p->n, factors,
                          (lapack_int)p->n, pivots);
    if (info == 0) {
        *checksum += factors[0] + (double)pivots[0];
    }
    free(factors);
    free(pivots);
    return (int)info;
}

static int run_numerix_lu_solve(problem *p, double *checksum)
{
    numerix_vector *solution = NULL;
    numerix_error error;
    numerix_status status = numerix_lu_solve(&solve_num_factors, &p->rhs,
                                              &solution, &error);
    if (status == NUMERIX_STATUS_SUCCESS) {
        *checksum += solution->data[0];
        numerix_vector_destroy(solution);
    }
    return (int)status;
}

typedef struct lapack_solve_context {
    double *factors;
    lapack_int *pivots;
} lapack_solve_context;

static lapack_solve_context solve_context;

static int prepare_lapack_solve(problem *p)
{
    size_t count = (size_t)p->n * (size_t)p->n;
    int row;
    int column;
    lapack_int info;

    solve_context.factors = (double *)malloc(count * sizeof(double));
    solve_context.pivots = (lapack_int *)malloc((size_t)p->n * sizeof(lapack_int));
    if (solve_context.factors == NULL || solve_context.pivots == NULL) {
        free(solve_context.factors);
        free(solve_context.pivots);
        memset(&solve_context, 0, sizeof(solve_context));
        return -1002;
    }
    for (row = 0; row < p->n; ++row) {
        for (column = 0; column < p->n; ++column) {
            solve_context.factors[(size_t)row +
                (size_t)column * (size_t)p->n] =
                p->a[(size_t)row * (size_t)p->n + (size_t)column];
        }
    }
    info = LAPACKE_dgetrf(LAPACK_COL_MAJOR, (lapack_int)p->n,
                          (lapack_int)p->n, solve_context.factors,
                          (lapack_int)p->n, solve_context.pivots);
    return (int)info;
}

static int prepare_numerix_solve(problem *p)
{
    numerix_error error;
    numerix_status status = numerix_lu_decompose(&p->matrix_a, NULL,
                                                  &solve_num_factors, &error);
    return (int)status;
}

static void write_setup_record(problem *p, const char *library,
                               const char *library_version,
                               const char *setup_operation, int (*setup)(problem *))
{
    double wall_start = now_seconds(CLOCK_MONOTONIC_RAW);
    double cpu_start = now_seconds(CLOCK_THREAD_CPUTIME_ID);
    int native_status = setup(p);
    double wall_elapsed = now_seconds(CLOCK_MONOTONIC_RAW) - wall_start;
    double cpu_elapsed = now_seconds(CLOCK_THREAD_CPUTIME_ID) - cpu_start;

    fprintf(setup_csv, "%s,linear_algebra,%s,%s,%s,%s,%.12g,%.12g,%s,%d,%s\n",
            run_id, p->id, library, library_version, setup_operation,
            wall_elapsed, cpu_elapsed,
            native_status == 0 ? "success" : "failure", native_status,
            "one-time saved-factor preparation; excluded from solve-only raw_timing");
    if (native_status != 0) {
        fprintf(stderr, "factor setup failed for %s (%s): %d\n", p->id,
                library, native_status);
        exit(6);
    }
}

static int run_lapack_lu_solve(problem *p, double *checksum)
{
    double *solution = (double *)malloc((size_t)p->n * sizeof(*solution));
    lapack_int info;

    if (solution == NULL) {
        return -1002;
    }
    if (!all_finite(p->b, (size_t)p->n)) {
        free(solution);
        return -1001;
    }
    memcpy(solution, p->b, (size_t)p->n * sizeof(*solution));
    info = LAPACKE_dgetrs(LAPACK_COL_MAJOR, 'N', (lapack_int)p->n, 1,
                          solve_context.factors, (lapack_int)p->n,
                          solve_context.pivots, solution, (lapack_int)p->n);
    if (info == 0 && all_finite(solution, (size_t)p->n)) {
        *checksum += solution[0];
    } else if (info == 0) {
        info = -1003;
    }
    free(solution);
    return (int)info;
}

static double route_batch(problem *p, const route *r, unsigned int calls,
                          int *native_status, double *checksum)
{
    double wall_start = now_seconds(CLOCK_MONOTONIC_RAW);
    unsigned int index;

    *native_status = 0;
    *checksum = 0.0;
    for (index = 0; index < calls; ++index) {
        int status = r->call(p, checksum);

        if (status != 0) {
            *native_status = status;
            return NAN;
        }
    }
    return now_seconds(CLOCK_MONOTONIC_RAW) - wall_start;
}

/* Execute one wall-clock and one CPU-clock pass independently to avoid
 * double-invoking within a single elapsed-time sample. */
static void write_batch(problem *p, const route *r, int batch_index,
                        const char *order, unsigned int calls)
{
    double wall_start = now_seconds(CLOCK_MONOTONIC_RAW);
    double cpu_start = now_seconds(CLOCK_THREAD_CPUTIME_ID);
    double checksum = 0.0;
    double wall_elapsed;
    double cpu_elapsed;
    unsigned int index;
    int native_status = 0;
    int status = 0;

    for (index = 0; index < calls; ++index) {
        status = r->call(p, &checksum);
        if (status != 0) {
            native_status = status;
            break;
        }
    }
    wall_elapsed = now_seconds(CLOCK_MONOTONIC_RAW) - wall_start;
    cpu_elapsed = now_seconds(CLOCK_THREAD_CPUTIME_ID) - cpu_start;
    sink_value += checksum;
    fprintf(timing_csv,
            "%s,linear_algebra,%s,%s,%s,%s,%s,%s,%d,%s,%u,%d,%.12g,%.12g,%s,%d,%u,%s,",
            run_id, p->id, r->library, r->version, r->numerix_method,
            r->competitor_method, r->comparison_class, batch_index, order,
            calls, WARMUP_CALLS, wall_elapsed, cpu_elapsed,
            status == 0 ? "success" : "failure", native_status,
            index, r->timing_scope);
    csv_write_quoted(timing_csv,
            p->op == OP_ADD ? "{\"matrix_elements\":65536}" :
            p->op == OP_GEMM ? "{\"dimensions_mnk\":\"64x64x64\",\"flop_estimate\":524288}" :
            p->op == OP_LU_FACTOR ? "{\"matrix_dimension\":64,\"flop_estimate\":174763}" :
                                    "{\"matrix_dimension\":64,\"flop_estimate\":8192}");
    fputc('\n', timing_csv);
}

static void measure_problem(problem *p, route *routes, size_t route_count)
{
    size_t route_index;
    unsigned int calls[4] = {1U, 1U, 1U, 1U};
    size_t counterbalance_start = (size_t)(random_u64() & UINT64_C(1));

    for (route_index = 0; route_index < route_count; ++route_index) {
        double elapsed;
        double checksum;
        int native_status;
        unsigned int warmup;
        unsigned int calibration_calls = 1U;

        /* Five fixed warmups plus one calibration call, recorded as six. */
        for (warmup = 0; warmup < WARMUP_CALLS - 1U; ++warmup) {
            if (routes[route_index].call(p, &checksum) != 0) {
                fprintf(stderr, "warmup failed: %s / %s\n", p->id,
                        routes[route_index].competitor_method);
                exit(4);
            }
        }
        elapsed = route_batch(p, &routes[route_index], calibration_calls,
                              &native_status, &checksum);
        if (!isfinite(elapsed) || elapsed <= 0.0 || native_status != 0) {
            fprintf(stderr, "timer calibration failed: %s / %s\n", p->id,
                    routes[route_index].competitor_method);
            exit(5);
        }
        calls[route_index] = (unsigned int)ceil(TARGET_BATCH_NS /
                                                 (elapsed * 1.0e9));
        if (calls[route_index] < 1U) calls[route_index] = 1U;
        if (calls[route_index] > MAX_BATCH_CALLS) {
            calls[route_index] = MAX_BATCH_CALLS;
        }
    }

    for (int batch = 0; batch < TIMING_BATCHES; ++batch) {
        size_t order[4] = {0U, 1U, 2U, 3U};
        size_t index;
        char order_label[256] = {0};
        size_t used = 0U;

        /* Two-route methods use a seeded random start then strict alternation;
         * larger route sets use seeded Fisher-Yates. Order is persisted. */
        if (route_count == 2U) {
            size_t first = counterbalance_start ^ (size_t)(batch & 1);
            order[0] = first;
            order[1] = 1U - first;
        } else {
            random_state ^= (uint64_t)batch + UINT64_C(0x9e3779b97f4a7c15);
            for (index = route_count; index > 1U; --index) {
                size_t other = (size_t)(random_u64() % index);
                size_t tmp = order[index - 1U];
                order[index - 1U] = order[other];
                order[other] = tmp;
            }
        }
        for (index = 0; index < route_count; ++index) {
            size_t route_id = order[index];

            if (index != 0U) {
                used += (size_t)snprintf(order_label + used,
                                         sizeof(order_label) - used, ">");
            }
            used += (size_t)snprintf(order_label + used,
                                     sizeof(order_label) - used, "%s",
                                     routes[route_id].library);
        }
        for (index = 0; index < route_count; ++index) {
            size_t route_id = order[index];
            write_batch(p, &routes[route_id], batch, order_label,
                        calls[route_id]);
        }
    }
}

static double max_abs(const double *values, size_t count)
{
    double maximum = 0.0;
    size_t index;

    for (index = 0; index < count; ++index) {
        double value = fabs(values[index]);
        if (value > maximum) maximum = value;
    }
    return maximum;
}

static int check_matrix_add(problem *p, const double *computed,
                            double *absolute_error, double *reference_norm)
{
    size_t index;
    double maximum = 0.0;
    double maximum_reference = 0.0;

    for (index = 0; index < (size_t)p->m * (size_t)p->n; ++index) {
        double ref = p->reference[index];
        double error = fabs(computed[index] - ref);
        if (!isfinite(computed[index])) return 0;
        if (error > maximum) maximum = error;
        if (fabs(ref) > maximum_reference) maximum_reference = fabs(ref);
    }
    *absolute_error = maximum;
    *reference_norm = maximum_reference;
    return 1;
}

static int accuracy_num_add(problem *p, double *abs_error, double *ref_norm)
{
    numerix_matrix *out = NULL;
    numerix_error error;
    numerix_status status = numerix_matrix_add(&p->matrix_a, &p->matrix_b,
                                               &out, &error);
    int finite_result = status == NUMERIX_STATUS_SUCCESS &&
                        all_finite(out->data, (size_t)p->m * (size_t)p->n);
    int ok = finite_result && check_matrix_add(p, out->data, abs_error, ref_norm);
    numerix_matrix_destroy(out);
    return ok ? (int)status : (status == 0 ? -1003 : (int)status);
}

static int accuracy_blas_add(problem *p, double *abs_error, double *ref_norm)
{
    size_t count = (size_t)p->m * (size_t)p->n;
    double *out = (double *)malloc(count * sizeof(*out));
    int status = 0;

    if (out == NULL) return -1002;
    memcpy(out, p->a, count * sizeof(*out));
    cblas_daxpy((int)count, 1.0, p->b, 1, out, 1);
    if (!check_matrix_add(p, out, abs_error, ref_norm)) status = -1003;
    free(out);
    return status;
}

static int accuracy_num_gemm(problem *p, double *abs_error, double *ref_norm)
{
    numerix_matrix *out = NULL;
    numerix_error error;
    numerix_status status = numerix_matrix_multiply(&p->matrix_a,
                                                    &p->matrix_b,
                                                    &out, &error);
    size_t count = (size_t)p->m * (size_t)p->n;
    double error_max = 0.0;
    int finite_result = status == NUMERIX_STATUS_SUCCESS &&
                        all_finite(out->data, count);
    int ok = finite_result;

    if (ok) {
        for (size_t index = 0; index < count; ++index) {
            double error_value = fabs(out->data[index] - p->reference[index]);
            if (error_value > error_max) error_max = error_value;
        }
    }
    *abs_error = error_max;
    *ref_norm = max_abs(p->reference, count);
    numerix_matrix_destroy(out);
    return ok ? (int)status : (status == 0 ? -1003 : (int)status);
}

static int accuracy_blas_gemm(problem *p, double *abs_error, double *ref_norm)
{
    size_t count = (size_t)p->m * (size_t)p->n;
    double *out = (double *)calloc(count, sizeof(*out));
    double error_max = 0.0;
    int status = 0;

    if (out == NULL) return -1002;
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                p->m, p->n, p->k, 1.0,
                p->a, p->k, p->b, p->n, 0.0, out, p->n);
    if (!all_finite(out, count)) status = -1003;
    for (size_t index = 0; status == 0 && index < count; ++index) {
        double error_value = fabs(out[index] - p->reference[index]);
        if (error_value > error_max) error_max = error_value;
    }
    *abs_error = error_max;
    *ref_norm = max_abs(p->reference, count);
    free(out);
    return status;
}

static double factor_residual_num(problem *p, int *status)
{
    numerix_lu lu = {0};
    numerix_error error;
    long double maximum = 0.0L;
    numerix_status code = numerix_lu_decompose(&p->matrix_a, NULL, &lu, &error);

    *status = (int)code;
    if (code == NUMERIX_STATUS_SUCCESS) {
        for (int row = 0; row < p->n; ++row) {
            for (int col = 0; col < p->n; ++col) {
                long double lu_value = 0.0L;
                long double pa_value =
                    (long double)p->a[lu.permutation[row * 1U] * (size_t)p->n +
                                      (size_t)col];
                for (int index = 0; index < p->n; ++index) {
                    long double l = row > index ?
                        (long double)lu.data[(size_t)row * (size_t)p->n +
                                             (size_t)index] :
                        (row == index ? 1.0L : 0.0L);
                    long double u = index <= col ?
                        (long double)lu.data[(size_t)index * (size_t)p->n +
                                             (size_t)col] : 0.0L;
                    lu_value += l * u;
                }
                if (fabsl(pa_value - lu_value) > maximum) {
                    maximum = fabsl(pa_value - lu_value);
                }
            }
        }
        numerix_lu_destroy(&lu);
    }
    return (double)maximum;
}

static double factor_residual_lapack(problem *p, int *status)
{
    size_t count = (size_t)p->n * (size_t)p->n;
    double *factors = (double *)malloc(count * sizeof(double));
    double *permuted = (double *)malloc(count * sizeof(double));
    lapack_int *pivots = (lapack_int *)malloc((size_t)p->n * sizeof(lapack_int));
    long double maximum = 0.0L;

    if (factors == NULL || permuted == NULL || pivots == NULL) {
        free(factors); free(permuted); free(pivots); *status = -1002; return NAN;
    }
    for (int row = 0; row < p->n; ++row) {
        for (int col = 0; col < p->n; ++col) {
            factors[(size_t)row + (size_t)col * (size_t)p->n] =
                p->a[(size_t)row * (size_t)p->n + (size_t)col];
        }
    }
    *status = (int)LAPACKE_dgetrf(LAPACK_COL_MAJOR, (lapack_int)p->n,
                                  (lapack_int)p->n, factors,
                                  (lapack_int)p->n, pivots);
    if (*status == 0) {
        memcpy(permuted, p->a, count * sizeof(double));
        for (int step = 0; step < p->n; ++step) {
            int pivot = (int)pivots[step] - 1;
            if (pivot != step) {
                for (int col = 0; col < p->n; ++col) {
                    double temp = permuted[(size_t)step * (size_t)p->n + (size_t)col];
                    permuted[(size_t)step * (size_t)p->n + (size_t)col] =
                        permuted[(size_t)pivot * (size_t)p->n + (size_t)col];
                    permuted[(size_t)pivot * (size_t)p->n + (size_t)col] = temp;
                }
            }
        }
        for (int row = 0; row < p->n; ++row) {
            for (int col = 0; col < p->n; ++col) {
                long double lu_value = 0.0L;
                long double pa_value = (long double)permuted[(size_t)row *
                    (size_t)p->n + (size_t)col];
                for (int index = 0; index < p->n; ++index) {
                    long double l = row > index ?
                        (long double)factors[(size_t)row + (size_t)index * (size_t)p->n] :
                        (row == index ? 1.0L : 0.0L);
                    long double u = index <= col ?
                        (long double)factors[(size_t)index + (size_t)col * (size_t)p->n] : 0.0L;
                    lu_value += l * u;
                }
                if (fabsl(pa_value - lu_value) > maximum) maximum = fabsl(pa_value - lu_value);
            }
        }
    }
    free(factors); free(permuted); free(pivots);
    return (double)maximum;
}

static double solve_error_num(problem *p, int *status)
{
    numerix_lu lu = {0};
    numerix_vector *solution = NULL;
    numerix_error error;
    double maximum = 0.0;
    numerix_status code = numerix_lu_decompose(&p->matrix_a, NULL, &lu, &error);

    if (code == NUMERIX_STATUS_SUCCESS) {
        code = numerix_lu_solve(&lu, &p->rhs, &solution, &error);
    }
    *status = (int)code;
    if (code == NUMERIX_STATUS_SUCCESS && all_finite(solution->data, (size_t)p->n)) {
        for (int i = 0; i < p->n; ++i) {
            double error_value = fabs(solution->data[i] - p->reference[i]);
            if (error_value > maximum) maximum = error_value;
        }
    } else if (code == NUMERIX_STATUS_SUCCESS) {
        *status = -1003;
        maximum = NAN;
    } else {
        maximum = NAN;
    }
    numerix_vector_destroy(solution);
    numerix_lu_destroy(&lu);
    return maximum;
}

static double solve_error_lapack(problem *p, int *status)
{
    double *factors = (double *)malloc((size_t)p->n * (size_t)p->n * sizeof(double));
    lapack_int *pivots = (lapack_int *)malloc((size_t)p->n * sizeof(lapack_int));
    double *solution = (double *)malloc((size_t)p->n * sizeof(double));
    double maximum = 0.0;

    if (!factors || !pivots || !solution) {
        free(factors); free(pivots); free(solution); *status = -1002; return NAN;
    }
    for (int row = 0; row < p->n; ++row) {
        for (int col = 0; col < p->n; ++col) {
            factors[(size_t)row + (size_t)col * (size_t)p->n] =
                p->a[(size_t)row * (size_t)p->n + (size_t)col];
        }
    }
    *status = (int)LAPACKE_dgetrf(LAPACK_COL_MAJOR, (lapack_int)p->n,
                                  (lapack_int)p->n, factors,
                                  (lapack_int)p->n, pivots);
    memcpy(solution, p->b, (size_t)p->n * sizeof(double));
    if (*status == 0) {
        *status = (int)LAPACKE_dgetrs(LAPACK_COL_MAJOR, 'N',
                                      (lapack_int)p->n, 1, factors,
                                      (lapack_int)p->n, pivots, solution,
                                      (lapack_int)p->n);
    }
    if (*status == 0 && all_finite(solution, (size_t)p->n)) {
        for (int i = 0; i < p->n; ++i) {
            double error_value = fabs(solution[i] - p->reference[i]);
            if (error_value > maximum) maximum = error_value;
        }
    } else if (*status == 0) {
        *status = -1003;
        maximum = NAN;
    } else {
        maximum = NAN;
    }
    free(factors); free(pivots); free(solution);
    return maximum;
}

static void write_accuracy_record(problem *p, const route *r, int native_status,
                                  int finite_result, double abs_error,
                                  double ref_norm, const char *metric,
                                  const char *reference,
                                  const char *precision,
                                  const char *work_json)
{
    double rel_error = isfinite(abs_error) ?
                       abs_error / fmax(ref_norm, 1.0) : NAN;
    int passed = native_status == 0 && finite_result &&
                 abs_error <= ABS_TARGET + REL_TARGET * ref_norm;

    fprintf(accuracy_csv,
            "%s,linear_algebra,%s,%s,%s,%s,%s,%s,%s,%d,%s,",
            run_id, p->id, r->library, r->version, r->numerix_method,
            r->competitor_method, r->comparison_class,
            native_status == 0 ? "success" : "failure", native_status,
            finite_result ? "true" : "false");
    csv_write_quoted(accuracy_csv, reference); fputc(',', accuracy_csv);
    csv_write_quoted(accuracy_csv, precision); fputc(',', accuracy_csv);
    csv_write_quoted(accuracy_csv, metric);
    fprintf(accuracy_csv,
            ",%.12g,%.12g,%.12g,%.12g,%.12g,%s,",
            abs_error, rel_error, rel_error, ABS_TARGET, REL_TARGET,
            passed ? "true" : "false");
    csv_write_quoted(accuracy_csv, work_json);
    fputc('\n', accuracy_csv);
}

static void run_accuracy(problem *p, route *routes, size_t route_count)
{
    for (size_t r = 0; r < route_count; ++r) {
        double absolute_error = NAN;
        double reference_norm = NAN;
        int status = 0;
        int finite_result = 0;
        const char *metric;
        const char *reference;
        const char *precision;
        const char *work;

        if (p->op == OP_ADD) {
            absolute_error = 0.0;
            reference_norm = max_abs(p->reference, (size_t)p->m * (size_t)p->n);
            status = r == 0 ? accuracy_num_add(p, &absolute_error, &reference_norm) :
                              accuracy_blas_add(p, &absolute_error, &reference_norm);
            metric = "maximum elementwise absolute/relative error";
            reference = "exact integer elementwise sum";
            precision = "exact int64 manifest arithmetic; bounded integral inputs";
            work = "{\"matrix_elements\":65536}";
            finite_result = status == 0;
        } else if (p->op == OP_GEMM) {
            status = r == 0 ? accuracy_num_gemm(p, &absolute_error, &reference_norm) :
                              accuracy_blas_gemm(p, &absolute_error, &reference_norm);
            metric = "maximum elementwise absolute/relative error";
            reference = "exact integer matrix product";
            precision = "int64 products/sums exact for this bounded pilot input";
            work = "{\"dimensions_mnk\":\"64x64x64\",\"output_elements\":4096}";
            finite_result = status == 0;
        } else if (p->op == OP_LU_FACTOR) {
            absolute_error = r == 0 ? factor_residual_num(p, &status) :
                                     factor_residual_lapack(p, &status);
            reference_norm = max_abs(p->a, (size_t)p->n * (size_t)p->n);
            metric = "max |P*A-L*U| / max(1,max|A|)";
            reference = "factorization identity from exact integer input matrix";
            precision = "binary64 factors multiplied in long double; exact A";
            work = "{\"matrix_dimension\":64,\"factor_entries\":4096}";
            finite_result = status == 0 && isfinite(absolute_error);
        } else {
            absolute_error = r == 0 ? solve_error_num(p, &status) :
                                     solve_error_lapack(p, &status);
            reference_norm = max_abs(p->reference, (size_t)p->n);
            metric = "maximum solution component error / max(1,max|x_ref|)";
            reference = "known exact solution x=(1,2,1,...,1)";
            precision = "analytical solution for integer block matrix";
            work = "{\"matrix_dimension\":64,\"rhs_count\":1}";
            finite_result = status == 0 && isfinite(absolute_error);
        }
        write_accuracy_record(p, &routes[r], status, finite_result,
                              absolute_error, reference_norm, metric,
                              reference, precision, work);
    }
}

static void initialize_problem(problem *p, operation op)
{
    size_t a_count;
    size_t b_count;

    memset(p, 0, sizeof(*p));
    p->op = op;
    if (op == OP_ADD) {
        p->id = "LA-PILOT-ADD-256-INT-v1";
        p->family = "bounded signed integer matrix elementwise";
        p->definition = "A[i]=((7*i)%31)-15; B[i]=((13*i)%23)-11; 256x256";
        p->m = 256; p->n = 256; p->k = 0;
    } else if (op == OP_GEMM) {
        p->id = "LA-PILOT-GEMM-64-INT-v1";
        p->family = "bounded signed integer square matrix product";
        p->definition = "A[i,j]=((i+3*j)%17)-8; B[i,j]=((5*i+j)%13)-6; 64x64";
        p->m = 64; p->n = 64; p->k = 64;
    } else if (op == OP_LU_FACTOR) {
        p->id = "LA-PILOT-LUFACTOR-64-BLOCKPIVOT-v1";
        p->family = "pivot-requiring block-diagonal nonsingular system";
        p->definition = "A[0:2,0:2]=[[0,2],[1,3]]; remaining diagonal is 1; n=64";
        p->m = 64; p->n = 64; p->k = 0;
    } else {
        p->id = "LA-PILOT-LUSOLVE-64-BLOCKPIVOT-v1";
        p->family = "known-solution pivot-requiring linear system";
        p->definition = "A is pivot block [[0,2],[1,3]] plus I; x=(1,2,1,...,1), b=A*x; n=64";
        p->m = 64; p->n = 64; p->k = 0;
    }
    a_count = (size_t)p->m * (size_t)p->n;
    b_count = op == OP_GEMM ? (size_t)p->k * (size_t)p->n : a_count;
    p->a = (double *)calloc(a_count, sizeof(double));
    p->b = (double *)calloc(b_count, sizeof(double));
    p->reference = (double *)calloc(a_count, sizeof(double));
    if (op == OP_LU_FACTOR || op == OP_LU_SOLVE) {
        p->column_a = (double *)calloc(a_count, sizeof(double));
    }
    if (!p->a || !p->b || !p->reference ||
        ((op == OP_LU_FACTOR || op == OP_LU_SOLVE) && !p->column_a)) {
        fputs("allocation failure in pilot problem construction\n", stderr);
        exit(3);
    }
    p->matrix_a.rows = (size_t)p->m;
    p->matrix_a.columns = (size_t)p->n;
    p->matrix_a.data = p->a;
    p->matrix_b.rows = op == OP_GEMM ? (size_t)p->k : (size_t)p->m;
    p->matrix_b.columns = (size_t)p->n;
    p->matrix_b.data = p->b;
    p->rhs.size = (size_t)p->n;
    p->rhs.data = p->b;

    if (op == OP_ADD) {
        for (size_t i = 0; i < a_count; ++i) {
            int64_t av = (int64_t)((7U * i) % 31U) - 15;
            int64_t bv = (int64_t)((13U * i) % 23U) - 11;
            p->a[i] = (double)av;
            p->b[i] = (double)bv;
            p->reference[i] = (double)(av + bv);
        }
    } else if (op == OP_GEMM) {
        for (int row = 0; row < p->m; ++row) {
            for (int col = 0; col < p->k; ++col) {
                p->a[(size_t)row * (size_t)p->k + (size_t)col] =
                    (double)(((row + 3 * col) % 17) - 8);
            }
        }
        for (int row = 0; row < p->k; ++row) {
            for (int col = 0; col < p->n; ++col) {
                p->b[(size_t)row * (size_t)p->n + (size_t)col] =
                    (double)(((5 * row + col) % 13) - 6);
            }
        }
        for (int row = 0; row < p->m; ++row) {
            for (int col = 0; col < p->n; ++col) {
                int64_t value = 0;
                for (int inner = 0; inner < p->k; ++inner) {
                    int64_t av = (int64_t)p->a[(size_t)row *
                        (size_t)p->k + (size_t)inner];
                    int64_t bv = (int64_t)p->b[(size_t)inner *
                        (size_t)p->n + (size_t)col];
                    value += av * bv;
                }
                p->reference[(size_t)row * (size_t)p->n + (size_t)col] =
                    (double)value;
            }
        }
    } else {
        p->a[1] = 2.0;
        p->a[(size_t)p->n] = 1.0;
        p->a[(size_t)p->n + 1U] = 3.0;
        for (int i = 2; i < p->n; ++i) {
            p->a[(size_t)i * (size_t)p->n + (size_t)i] = 1.0;
        }
        p->reference[0] = 1.0;
        p->reference[1] = 2.0;
        for (int i = 2; i < p->n; ++i) p->reference[i] = 1.0;
        p->b[0] = 4.0;
        p->b[1] = 7.0;
        for (int i = 2; i < p->n; ++i) p->b[i] = 1.0;
    }
}

static void destroy_problem(problem *p)
{
    free(p->a);
    free(p->b);
    free(p->reference);
    free(p->column_a);
    memset(p, 0, sizeof(*p));
}

static void run_operation(operation op)
{
    problem p;
    route routes[2];
    size_t route_count = 2;

    initialize_problem(&p, op);
    if (op == OP_ADD) {
        routes[0] = (route){"NUMERIX", "0.2.0-rc1", "numerix_matrix_add",
            "cblas_dcopy+cblas_daxpy", "direct", "end_to_end_api_wrapper",
            run_numerix_add};
        routes[1] = (route){"OpenBLAS", "0.3.34", "numerix_matrix_add",
            "cblas_dcopy+cblas_daxpy", "direct", "end_to_end_api_wrapper",
            run_openblas_add};
    } else if (op == OP_GEMM) {
        routes[0] = (route){"NUMERIX", "0.2.0-rc1", "numerix_matrix_multiply",
            "cblas_dgemm_row_major", "direct", "end_to_end_api_wrapper",
            run_numerix_gemm};
        routes[1] = (route){"OpenBLAS", "0.3.34", "numerix_matrix_multiply",
            "cblas_dgemm_row_major", "direct", "end_to_end_api_wrapper",
            run_openblas_gemm};
    } else if (op == OP_LU_FACTOR) {
        routes[0] = (route){"NUMERIX", "0.2.0-rc1", "numerix_lu_decompose",
            "LAPACKE_dgetrf_col_major", "best_available_scaled_vs_partial_pivot",
            "end_to_end_api_with_layout_conversion", run_numerix_lu_factor};
        routes[1] = (route){"LAPACK+OpenBLAS", "LAPACK 3.12.0 + OpenBLAS 0.3.34",
            "numerix_lu_decompose", "LAPACKE_dgetrf_col_major_with_row_major_conversion",
            "best_available_scaled_vs_partial_pivot",
            "end_to_end_api_with_layout_conversion", run_lapack_lu_factor};
    } else {
        routes[0] = (route){"NUMERIX", "0.2.0-rc1", "numerix_lu_solve",
            "LAPACKE_dgetrs", "direct_lu_solve_family",
            "steady_state_saved_factors_and_allocated_solution",
            run_numerix_lu_solve};
        routes[1] = (route){"LAPACK+OpenBLAS", "LAPACK 3.12.0 + OpenBLAS 0.3.34",
            "numerix_lu_solve", "LAPACKE_dgetrs_precomputed_factors",
            "direct_lu_solve_family",
            "steady_state_saved_factors_and_allocated_solution",
            run_lapack_lu_solve};
    }
    run_accuracy(&p, routes, route_count);
    if (op == OP_LU_SOLVE) {
        write_setup_record(&p, "NUMERIX", "0.2.0-rc1",
                           "numerix_lu_decompose", prepare_numerix_solve);
        write_setup_record(&p, "LAPACK+OpenBLAS",
                           "LAPACK 3.12.0 + OpenBLAS 0.3.34",
                           "row-major-to-column-major conversion + LAPACKE_dgetrf",
                           prepare_lapack_solve);
    }
    measure_problem(&p, routes, route_count);
    if (op == OP_LU_SOLVE) {
        numerix_lu_destroy(&solve_num_factors);
        free(solve_context.factors);
        free(solve_context.pivots);
        memset(&solve_context, 0, sizeof(solve_context));
    }
    destroy_problem(&p);
}

int main(int argc, char **argv)
{
    const char *timing_path;
    const char *accuracy_path;
    const char *setup_path;

    if (argc != 5) {
        fprintf(stderr, "usage: %s RUN_ID RAW_TIMING.csv RAW_ACCURACY.csv SETUP_TIMING.csv\n", argv[0]);
        return 2;
    }
    run_id = argv[1];
    timing_path = argv[2];
    accuracy_path = argv[3];
    setup_path = argv[4];
    timing_csv = fopen(timing_path, "w");
    accuracy_csv = fopen(accuracy_path, "w");
    setup_csv = fopen(setup_path, "w");
    if (!timing_csv || !accuracy_csv || !setup_csv) {
        perror("open CSV output");
        return 2;
    }
    openblas_set_num_threads(1);
    if (openblas_get_num_threads() != 1) {
        fprintf(stderr, "OpenBLAS did not honor single-thread pilot setting: %d\n",
                openblas_get_num_threads());
        return 7;
    }
    fputs("run_id,module,problem_id,library,library_version,numerix_method,competitor_method,comparison_class,batch_index,batch_order,batch_size,warmup_count,monotonic_wall_elapsed_seconds,cpu_elapsed_seconds,status,native_status,calls_completed,timing_scope,work_counters_json\n",
          timing_csv);
    fputs("run_id,module,problem_id,library,library_version,numerix_method,competitor_method,comparison_class,status,native_status,finite_result,reference_value,reference_precision,error_metric,absolute_error,relative_error,normalized_error,target_absolute_error,target_relative_error,accuracy_pass,work_counters_json\n",
          accuracy_csv);
    fputs("run_id,module,problem_id,library,library_version,setup_operation,monotonic_wall_elapsed_seconds,cpu_elapsed_seconds,status,native_status,setup_scope\n",
          setup_csv);

    run_operation(OP_ADD);
    run_operation(OP_GEMM);
    run_operation(OP_LU_FACTOR);
    run_operation(OP_LU_SOLVE);

    fclose(timing_csv);
    fclose(accuracy_csv);
    fclose(setup_csv);
    fprintf(stderr, "OpenBLAS active threads=%d\n", openblas_get_num_threads());
    fprintf(stderr, "pilot checksum sink=%.17g\n", sink_value);
    return 0;
}
