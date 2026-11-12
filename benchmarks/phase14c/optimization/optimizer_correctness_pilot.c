#include "numerix.h"
#include <nlopt.h>

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMON_OBJECTIVE_BUDGET 200
#define ACCURACY_ABS_TOLERANCE 1e-5
#define ACCURACY_REL_TOLERANCE 1e-5
#define PILOT_RUN_ID "phase14c-opt-correctness-pilot-20261112"

typedef struct objective_context {
    const char *case_name;
    size_t objective_evaluations;
    size_t gradient_evaluations;
} objective_context;

typedef struct pilot_observation {
    double x[2];
    size_t dimension;
    double objective;
    int is_finite;
    size_t objective_evaluations;
    size_t gradient_evaluations;
    int native_status;
    const char *native_status_name;
    const char *normalized_status;
    int converged;
} pilot_observation;

static double scalar_center = 0.375;
static double vector_center[2] = {1.25, -0.75};

static double scalar_objective(double x, void *data)
{
    objective_context *context = (objective_context *)data;
    ++context->objective_evaluations;
    return (x - scalar_center) * (x - scalar_center) + 1.25;
}

static double vector_objective(const double *x, size_t dimension, void *data)
{
    objective_context *context = (objective_context *)data;
    ++context->objective_evaluations;
    if (dimension != 2U) return NAN;
    return (x[0] - vector_center[0]) * (x[0] - vector_center[0]) +
           2.0 * (x[1] - vector_center[1]) * (x[1] - vector_center[1]) + 0.5;
}

static numerix_status vector_gradient(const double *x, size_t dimension,
                                      double *gradient, void *data)
{
    objective_context *context = (objective_context *)data;
    if (dimension != 2U) return NUMERIX_STATUS_INVALID_DIMENSIONS;
    ++context->gradient_evaluations;
    gradient[0] = 2.0 * (x[0] - vector_center[0]);
    gradient[1] = 4.0 * (x[1] - vector_center[1]);
    return NUMERIX_STATUS_SUCCESS;
}

static double nlopt_objective(unsigned dimension, const double *x,
                              double *gradient, void *data)
{
    objective_context *context = (objective_context *)data;
    if (strcmp(context->case_name, "golden_section") == 0) {
        ++context->objective_evaluations;
        if (dimension != 1U) return NAN;
        if (gradient != NULL) gradient[0] = 2.0 * (x[0] - scalar_center);
        return (x[0] - scalar_center) * (x[0] - scalar_center) + 1.25;
    }
    ++context->objective_evaluations;
    if (dimension != 2U) return NAN;
    if (gradient != NULL) {
        ++context->gradient_evaluations;
        gradient[0] = 2.0 * (x[0] - vector_center[0]);
        gradient[1] = 4.0 * (x[1] - vector_center[1]);
    }
    return (x[0] - vector_center[0]) * (x[0] - vector_center[0]) +
           2.0 * (x[1] - vector_center[1]) * (x[1] - vector_center[1]) + 0.5;
}

static const char *numerix_status_name(numerix_status status)
{
    return numerix_status_string(status);
}

static const char *numerix_normalized_status(numerix_status status)
{
    return status == NUMERIX_STATUS_SUCCESS ? "converged" :
           status == NUMERIX_STATUS_LIMIT_REACHED ? "limit_reached" : "failure";
}

static const char *nlopt_normalized_status(nlopt_result status)
{
    if (status == NLOPT_MAXEVAL_REACHED || status == NLOPT_MAXTIME_REACHED) {
        return "limit_reached";
    }
    if (status >= NLOPT_SUCCESS && status <= NLOPT_XTOL_REACHED) {
        return "converged";
    }
    return "failure";
}

static int is_nlopt_converged(nlopt_result status)
{
    return status >= NLOPT_SUCCESS && status <= NLOPT_XTOL_REACHED;
}

static void csv_quote(const char *value)
{
    const unsigned char *cursor = (const unsigned char *)value;
    putchar('"');
    while (*cursor != '\0') {
        if (*cursor == '"') putchar('"');
        putchar((int)*cursor++);
    }
    putchar('"');
}

static void emit_row(const char *problem_id, const char *library,
                     const char *library_version, const char *numerix_method,
                     const char *competitor_method, const char *comparison_type,
                     const pilot_observation *observation,
                     const double *reference_x, double reference_objective,
                     const char *accuracy_metric, const char *input_json)
{
    char reference_json[256];
    char observed_json[256];
    char work_json[256];
    double location_error = INFINITY;
    double reference_norm = 0.0;
    double objective_gap = INFINITY;
    double relative_location_error = INFINITY;
    double normalized_objective_gap = INFINITY;
    int accuracy_pass = 0;
    size_t index;

    for (index = 0; index < observation->dimension; ++index) {
        reference_norm += reference_x[index] * reference_x[index];
    }
    reference_norm = sqrt(reference_norm);
    if (observation->is_finite) {
        double squared_error = 0.0;
        for (index = 0; index < observation->dimension; ++index) {
            const double difference = observation->x[index] - reference_x[index];
            squared_error += difference * difference;
        }
        location_error = sqrt(squared_error);
        relative_location_error = location_error /
            (reference_norm > DBL_MIN ? reference_norm : 1.0);
        objective_gap = fabs(observation->objective - reference_objective);
        normalized_objective_gap = objective_gap / fmax(fabs(reference_objective), 1.0);
        accuracy_pass = location_error <= ACCURACY_ABS_TOLERANCE +
                            ACCURACY_REL_TOLERANCE * reference_norm &&
                        objective_gap <= ACCURACY_ABS_TOLERANCE +
                            ACCURACY_REL_TOLERANCE * fabs(reference_objective);
    }
    if (observation->dimension == 1U) {
        (void)snprintf(reference_json, sizeof(reference_json),
                       "{\"x\":[%.17g],\"objective\":%.17g}",
                       reference_x[0], reference_objective);
        if (observation->is_finite) {
            (void)snprintf(observed_json, sizeof(observed_json),
                           "{\"x\":[%.17g],\"objective\":%.17g}",
                           observation->x[0], observation->objective);
        } else {
            (void)snprintf(observed_json, sizeof(observed_json),
                           "{\"x\":null,\"objective\":null}");
        }
    } else {
        (void)snprintf(reference_json, sizeof(reference_json),
                       "{\"x\":[%.17g,%.17g],\"objective\":%.17g}",
                       reference_x[0], reference_x[1], reference_objective);
        if (observation->is_finite) {
            (void)snprintf(observed_json, sizeof(observed_json),
                           "{\"x\":[%.17g,%.17g],\"objective\":%.17g}",
                           observation->x[0], observation->x[1], observation->objective);
        } else {
            (void)snprintf(observed_json, sizeof(observed_json),
                           "{\"x\":null,\"objective\":null}");
        }
    }
    (void)snprintf(work_json, sizeof(work_json),
                   "{\"objective_evaluations\":%zu,\"gradient_evaluations\":%zu,\"objective_budget\":%d}",
                   observation->objective_evaluations,
                   observation->gradient_evaluations, COMMON_OBJECTIVE_BUDGET);

    csv_quote(PILOT_RUN_ID); putchar(',');
    csv_quote("optimization"); putchar(',');
    csv_quote(problem_id); putchar(',');
    csv_quote(library); putchar(',');
    csv_quote(library_version); putchar(',');
    csv_quote(numerix_method); putchar(',');
    csv_quote(competitor_method); putchar(',');
    csv_quote(comparison_type); putchar(',');
    csv_quote(observation->normalized_status); putchar(',');
    printf("%d,", observation->native_status);
    csv_quote(observation->native_status_name); putchar(',');
    printf("%d,%d,", observation->converged, observation->is_finite);
    csv_quote(accuracy_metric); putchar(',');
    csv_quote(reference_json); putchar(',');
    csv_quote(observed_json); putchar(',');
    printf("%.17g,%.17g,%.17g,%.17g,%.17g,%d,",
           location_error, relative_location_error, normalized_objective_gap,
           ACCURACY_ABS_TOLERANCE, ACCURACY_REL_TOLERANCE, accuracy_pass);
    csv_quote(work_json); putchar(',');
    csv_quote(input_json); putchar('\n');
}

static int run_numerix_golden(pilot_observation *observation,
                              objective_context *context)
{
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;
    context->case_name = "golden_section";
    context->objective_evaluations = 0U;
    context->gradient_evaluations = 0U;
    numerix_options_init(&options);
    options.absolute_tolerance = ACCURACY_ABS_TOLERANCE;
    options.relative_tolerance = 0.0;
    options.maximum_iterations = 1000U;
    options.maximum_function_evaluations = COMMON_OBJECTIVE_BUDGET;
    status = numerix_optimize_golden_section(scalar_objective, context,
        -2.0, 2.0, &options, &result, &error);
    observation->dimension = 1U;
    observation->x[0] = result.value;
    observation->objective = result.residual;
    observation->is_finite = isfinite(result.value) && isfinite(result.residual);
    observation->objective_evaluations = context->objective_evaluations;
    observation->gradient_evaluations = 0U;
    observation->native_status = (int)status;
    observation->native_status_name = numerix_status_name(status);
    observation->normalized_status = numerix_normalized_status(status);
    observation->converged = status == NUMERIX_STATUS_SUCCESS;
    return 1;
}

static int run_nlopt_golden(pilot_observation *observation,
                            objective_context *context)
{
    nlopt_opt optimizer;
    double x = 0.0;
    double minimum = NAN;
    nlopt_result status;
    context->case_name = "golden_section";
    context->objective_evaluations = 0U;
    context->gradient_evaluations = 0U;
    optimizer = nlopt_create(NLOPT_GN_DIRECT_L, 1U);
    if (optimizer == NULL) return 0;
    if (nlopt_set_lower_bounds1(optimizer, -2.0) < 0 ||
        nlopt_set_upper_bounds1(optimizer, 2.0) < 0 ||
        nlopt_set_min_objective(optimizer, nlopt_objective, context) < 0 ||
        nlopt_set_xtol_abs1(optimizer, ACCURACY_ABS_TOLERANCE) < 0 ||
        nlopt_set_maxeval(optimizer, COMMON_OBJECTIVE_BUDGET) < 0) {
        nlopt_destroy(optimizer);
        return 0;
    }
    status = nlopt_optimize(optimizer, &x, &minimum);
    observation->dimension = 1U;
    observation->x[0] = x;
    observation->objective = minimum;
    observation->is_finite = isfinite(x) && isfinite(minimum);
    observation->objective_evaluations = context->objective_evaluations;
    observation->gradient_evaluations = context->gradient_evaluations;
    observation->native_status = (int)status;
    observation->native_status_name = nlopt_result_to_string(status);
    observation->normalized_status = nlopt_normalized_status(status);
    observation->converged = is_nlopt_converged(status);
    nlopt_destroy(optimizer);
    return 1;
}

static int run_numerix_gradient(pilot_observation *observation,
                                objective_context *context)
{
    const double initial_data[2] = {-3.0, 4.0};
    numerix_vector initial = {2U, (double *)initial_data};
    numerix_vector *solution = NULL;
    numerix_options options;
    numerix_result result;
    numerix_error error;
    numerix_status status;
    context->case_name = "gradient_descent";
    context->objective_evaluations = 0U;
    context->gradient_evaluations = 0U;
    numerix_options_init(&options);
    options.absolute_tolerance = 1e-8;
    options.relative_tolerance = 0.0;
    options.maximum_iterations = 1000U;
    options.maximum_function_evaluations = COMMON_OBJECTIVE_BUDGET;
    status = numerix_optimize_gradient_descent(vector_objective, vector_gradient,
        context, &initial, 0.1, &options, &solution, &result, &error);
    observation->dimension = 2U;
    observation->is_finite = solution != NULL && solution->size == 2U &&
        isfinite(result.value) && isfinite(solution->data[0]) && isfinite(solution->data[1]);
    if (observation->is_finite) {
        observation->x[0] = solution->data[0];
        observation->x[1] = solution->data[1];
        observation->objective = result.value;
    } else {
        observation->x[0] = NAN;
        observation->x[1] = NAN;
        observation->objective = NAN;
    }
    observation->objective_evaluations = context->objective_evaluations;
    observation->gradient_evaluations = context->gradient_evaluations;
    observation->native_status = (int)status;
    observation->native_status_name = numerix_status_name(status);
    observation->normalized_status = numerix_normalized_status(status);
    observation->converged = status == NUMERIX_STATUS_SUCCESS;
    numerix_vector_destroy(solution);
    return 1;
}

static int run_nlopt_gradient(pilot_observation *observation,
                              objective_context *context)
{
    nlopt_opt optimizer;
    double x[2] = {-3.0, 4.0};
    double minimum = NAN;
    nlopt_result status;
    context->case_name = "gradient_descent";
    context->objective_evaluations = 0U;
    context->gradient_evaluations = 0U;
    optimizer = nlopt_create(NLOPT_LD_LBFGS, 2U);
    if (optimizer == NULL) return 0;
    if (nlopt_set_min_objective(optimizer, nlopt_objective, context) < 0 ||
        nlopt_set_ftol_abs(optimizer, ACCURACY_ABS_TOLERANCE) < 0 ||
        nlopt_set_xtol_abs(optimizer, (double[2]){1e-8, 1e-8}) < 0 ||
        nlopt_set_maxeval(optimizer, COMMON_OBJECTIVE_BUDGET) < 0) {
        nlopt_destroy(optimizer);
        return 0;
    }
    status = nlopt_optimize(optimizer, x, &minimum);
    observation->dimension = 2U;
    observation->x[0] = x[0];
    observation->x[1] = x[1];
    observation->objective = minimum;
    observation->is_finite = isfinite(x[0]) && isfinite(x[1]) && isfinite(minimum);
    observation->objective_evaluations = context->objective_evaluations;
    observation->gradient_evaluations = context->gradient_evaluations;
    observation->native_status = (int)status;
    observation->native_status_name = nlopt_result_to_string(status);
    observation->normalized_status = nlopt_normalized_status(status);
    observation->converged = is_nlopt_converged(status);
    nlopt_destroy(optimizer);
    return 1;
}

int main(void)
{
    int major, minor, bugfix;
    objective_context context = {NULL, 0U, 0U};
    pilot_observation observation;
    const double golden_reference[1] = {0.375};
    const double gradient_reference[2] = {1.25, -0.75};
    const char *golden_input = "bounds=[-2,2];initial_point=0(NLopt input; NUMERIX API has no initial point);objective=(x-0.375)^2+1.25;objective_eval_budget=200";
    const char *gradient_input = "unbounded;initial_point=[-3,4];objective=(x0-1.25)^2+2*(x1+0.75)^2+0.5;objective_eval_budget=200;NUMERIX_initial_step=0.1";

    nlopt_version(&major, &minor, &bugfix);
    if (major != 2 || minor != 10 || bugfix != 1) {
        fprintf(stderr, "Expected NLopt 2.10.1; loaded %d.%d.%d\n", major, minor, bugfix);
        return 2;
    }
    puts("run_id,module,problem_id,library,library_version,numerix_method,competitor_method,comparison_type,normalized_status,native_status,native_status_name,converged,is_finite,accuracy_metric,reference_value_json,observed_value_json,absolute_error,relative_error,normalized_error,tolerance_abs,tolerance_rel,accuracy_pass,work_metrics_json,input_json");

    memset(&observation, 0, sizeof(observation));
    if (!run_numerix_golden(&observation, &context)) return 3;
    emit_row("P14C-OPT-PILOT-GOLDEN-001", "NUMERIX", "0.2.0-rc1",
        "numerix_optimize_golden_section", "nlopt_gn_direct_l", "best_available",
        &observation, golden_reference, 1.25,
        "minimizer Euclidean location error and objective gap", golden_input);
    memset(&observation, 0, sizeof(observation));
    if (!run_nlopt_golden(&observation, &context)) return 4;
    emit_row("P14C-OPT-PILOT-GOLDEN-001", "NLopt", "2.10.1",
        "numerix_optimize_golden_section", "nlopt_gn_direct_l", "best_available",
        &observation, golden_reference, 1.25,
        "minimizer Euclidean location error and objective gap", golden_input);

    memset(&observation, 0, sizeof(observation));
    if (!run_numerix_gradient(&observation, &context)) return 5;
    emit_row("P14C-OPT-PILOT-GRADIENT-001", "NUMERIX", "0.2.0-rc1",
        "numerix_optimize_gradient_descent", "nlopt_ld_lbfgs", "best_available",
        &observation, gradient_reference, 0.5,
        "minimizer Euclidean location error and objective gap", gradient_input);
    memset(&observation, 0, sizeof(observation));
    if (!run_nlopt_gradient(&observation, &context)) return 6;
    emit_row("P14C-OPT-PILOT-GRADIENT-001", "NLopt", "2.10.1",
        "numerix_optimize_gradient_descent", "nlopt_ld_lbfgs", "best_available",
        &observation, gradient_reference, 0.5,
        "minimizer Euclidean location error and objective gap", gradient_input);
    return 0;
}
