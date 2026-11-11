#include "numerix_internal.h"
#include "numerix_execution_internal.h"
#include "../intelligence/intelligence_internal.h"
#include "../piecewise/piecewise_internal.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMERIX_ROOT_DISCOVERY_MAX_PANELS 4096U
#define NUMERIX_ROOT_DISCOVERY_MAX_DEPTH 32U
#define NUMERIX_ROOT_DISCOVERY_MAX_SUBDIVISIONS 65536U
#define NUMERIX_ROOT_DISCOVERY_MAX_CANDIDATES 4096U

typedef struct discovery_sample {
    double x;
    double value;
    numerix_status status;
} discovery_sample;

typedef struct discovery_context {
    const numerix_recommendation_request *request;
    const numerix_root_discovery_options *discovery_options;
    numerix_root_search_report *report;
    numerix_context *execution_context;
    numerix_options base_options;
    numerix_status stop_status;
    numerix_error callback_error;
    size_t last_progress_evaluation;
    size_t piecewise_boundary_count;
    size_t piecewise_boundary_capacity;
    double *piecewise_boundaries;
    size_t root_capacity;
    size_t unresolved_capacity;
    size_t current_workspace_bytes;
    size_t root_storage_allocation_count;
    size_t unresolved_storage_allocation_count;
    size_t workspace_allocation_count;
} discovery_context;

static numerix_status discovery_error(
    numerix_error *error,
    numerix_status status,
    const char *message)
{
    return numerix_error_set(error, status, message);
}

static void update_storage_metrics(discovery_context *context)
{
    numerix_root_search_report *report = context->report;

    report->root_storage_capacity = context->root_capacity;
    report->unresolved_storage_capacity = context->unresolved_capacity;
    report->result_storage_bytes =
        context->root_capacity * sizeof(*report->roots) +
        context->unresolved_capacity * sizeof(*report->unresolved);
    report->root_storage_allocation_count =
        context->root_storage_allocation_count;
    report->unresolved_storage_allocation_count =
        context->unresolved_storage_allocation_count;
    report->workspace_allocation_count = context->workspace_allocation_count;
}

static void *discovery_allocate(
    discovery_context *context,
    size_t bytes)
{
    void *allocation;

    if (bytes == 0U || bytes > SIZE_MAX - context->current_workspace_bytes) {
        context->stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
        numerix_error_set(
            &context->callback_error,
            context->stop_status,
            "root discovery workspace size overflowed");
        return NULL;
    }

    allocation = numerix_allocate(bytes);
    if (allocation == NULL) {
        context->stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
        numerix_error_set(
            &context->callback_error,
            context->stop_status,
            "could not allocate root discovery workspace");
        return NULL;
    }

    context->current_workspace_bytes += bytes;
    if (context->current_workspace_bytes >
        context->report->workspace_peak_bytes) {
        context->report->workspace_peak_bytes =
            context->current_workspace_bytes;
    }
    if (context->workspace_allocation_count < SIZE_MAX) {
        ++context->workspace_allocation_count;
    }
    update_storage_metrics(context);
    return allocation;
}

static void discovery_deallocate(
    discovery_context *context,
    void *allocation,
    size_t bytes)
{
    if (allocation == NULL) {
        return;
    }

    numerix_deallocate(allocation);
    if (bytes <= context->current_workspace_bytes) {
        context->current_workspace_bytes -= bytes;
    } else {
        context->current_workspace_bytes = 0U;
    }
}

static size_t grown_capacity(size_t current, size_t required, size_t maximum)
{
    size_t capacity = current;

    if (capacity == 0U) {
        capacity = maximum < 4U ? maximum : 4U;
    }
    while (capacity < required) {
        if (capacity > maximum / 2U) {
            capacity = maximum;
        } else {
            capacity *= 2U;
        }
    }
    return capacity;
}

static numerix_status reserve_roots(
    discovery_context *context,
    size_t required)
{
    numerix_root_search_report *report = context->report;
    numerix_discovered_root *roots;
    size_t capacity;
    size_t bytes;

    if (required <= context->root_capacity) {
        return NUMERIX_STATUS_SUCCESS;
    }
    if (required > context->discovery_options->maximum_candidates) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        return context->stop_status;
    }

    capacity = grown_capacity(
        context->root_capacity,
        required,
        context->discovery_options->maximum_candidates);
    if (capacity > SIZE_MAX / sizeof(*roots)) {
        context->stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
        return context->stop_status;
    }
    bytes = capacity * sizeof(*roots);
    roots = discovery_allocate(context, bytes);
    if (roots == NULL) {
        return context->stop_status;
    }
    if (report->root_count > 0U) {
        memcpy(roots, report->roots, report->root_count * sizeof(*roots));
    }

    discovery_deallocate(
        context,
        report->roots,
        context->root_capacity * sizeof(*report->roots));
    report->roots = roots;
    context->root_capacity = capacity;
    if (context->root_storage_allocation_count < SIZE_MAX) {
        ++context->root_storage_allocation_count;
    }
    update_storage_metrics(context);
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status reserve_unresolved(
    discovery_context *context,
    size_t required)
{
    numerix_root_search_report *report = context->report;
    numerix_root_unresolved_interval *unresolved;
    size_t capacity;
    size_t bytes;

    if (required <= context->unresolved_capacity) {
        return NUMERIX_STATUS_SUCCESS;
    }
    if (required > context->discovery_options->maximum_candidates) {
        return NUMERIX_STATUS_LIMIT_REACHED;
    }

    capacity = grown_capacity(
        context->unresolved_capacity,
        required,
        context->discovery_options->maximum_candidates);
    if (capacity > SIZE_MAX / sizeof(*unresolved)) {
        context->stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
        return context->stop_status;
    }
    bytes = capacity * sizeof(*unresolved);
    unresolved = discovery_allocate(context, bytes);
    if (unresolved == NULL) {
        return context->stop_status;
    }
    if (report->stored_unresolved_count > 0U) {
        memcpy(
            unresolved,
            report->unresolved,
            report->stored_unresolved_count * sizeof(*unresolved));
    }

    discovery_deallocate(
        context,
        report->unresolved,
        context->unresolved_capacity * sizeof(*report->unresolved));
    report->unresolved = unresolved;
    context->unresolved_capacity = capacity;
    if (context->unresolved_storage_allocation_count < SIZE_MAX) {
        ++context->unresolved_storage_allocation_count;
    }
    update_storage_metrics(context);
    return NUMERIX_STATUS_SUCCESS;
}

static int sample_has_value(const discovery_sample *sample)
{
    return sample->status == NUMERIX_STATUS_SUCCESS &&
           isfinite(sample->value);
}

static int opposite_sign(double left, double right)
{
    if (left == 0.0 || right == 0.0) {
        return 0;
    }
    return signbit(left) != signbit(right);
}

static double interval_midpoint(double lower, double upper)
{
    return lower * 0.5 + upper * 0.5;
}

static double interpolate_interval(double lower, double upper, double fraction)
{
    return lower * (1.0 - fraction) + upper * fraction;
}

static numerix_status notify_progress(
    discovery_context *context,
    int force)
{
    size_t evaluations;
    size_t interval;
    numerix_status status;

    if (context->discovery_options->progress_callback == NULL) {
        return NUMERIX_STATUS_SUCCESS;
    }

    evaluations = numerix_context_function_evaluation_count(
        context->execution_context);
    interval = context->discovery_options->progress_interval;
    if (!force && interval > 0 &&
        evaluations - context->last_progress_evaluation < interval) {
        return NUMERIX_STATUS_SUCCESS;
    }

    context->last_progress_evaluation = evaluations;
    status = context->discovery_options->progress_callback(
        context->report,
        context->discovery_options->progress_user_data);
    if (status != NUMERIX_STATUS_SUCCESS) {
        context->stop_status = status;
        numerix_error_set(
            &context->callback_error,
            status,
            status == NUMERIX_STATUS_CANCELLED
                ? "root discovery was cancelled by the progress callback"
                : "root discovery progress callback returned a failure status");
    }
    return status;
}

static numerix_status check_search_limits(discovery_context *context)
{
    size_t evaluations;
    double maximum_time;

    if (context->stop_status != NUMERIX_STATUS_SUCCESS) {
        return context->stop_status;
    }

    maximum_time = context->base_options.maximum_execution_time_seconds;
    if (maximum_time > 0.0 &&
        numerix_context_elapsed_seconds(context->execution_context) >=
            maximum_time) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        numerix_error_set(
            &context->callback_error,
            context->stop_status,
            "root discovery reached its execution time limit");
        return context->stop_status;
    }

    evaluations = numerix_context_function_evaluation_count(
        context->execution_context);
    if (evaluations >= context->base_options.maximum_function_evaluations) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        numerix_error_set(
            &context->callback_error,
            context->stop_status,
            "root discovery reached its function evaluation limit");
        return context->stop_status;
    }

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status evaluate_request(
    discovery_context *context,
    double x,
    double *value)
{
    numerix_status status;
    numerix_error evaluation_error;

    status = check_search_limits(context);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }

    status = numerix_context_record_function_evaluation(
        context->execution_context);
    if (status != NUMERIX_STATUS_SUCCESS) {
        context->stop_status = status;
        return status;
    }

    numerix_error_clear(&evaluation_error);
    status = numerix_request_evaluate(
        context->request, x, value, &evaluation_error);
    context->report->function_evaluations =
        numerix_context_function_evaluation_count(
            context->execution_context);
    if (status != NUMERIX_STATUS_SUCCESS || !isfinite(*value)) {
        numerix_status progress_status;

        ++context->report->evaluation_failures;
        if (status == NUMERIX_STATUS_SUCCESS) {
            status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            numerix_error_set(
                &evaluation_error,
                status,
                "function evaluation returned a non-finite value");
        }
        context->callback_error = evaluation_error;
        progress_status = notify_progress(context, 0);
        if (progress_status != NUMERIX_STATUS_SUCCESS) {
            return progress_status;
        }
        if (context->base_options.maximum_execution_time_seconds > 0.0 &&
            numerix_context_elapsed_seconds(context->execution_context) >=
                context->base_options.maximum_execution_time_seconds) {
            context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
            numerix_error_set(
                &context->callback_error,
                context->stop_status,
                "root discovery reached its execution time limit");
            return context->stop_status;
        }
        return status;
    }

    status = notify_progress(context, 0);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }

    if (context->base_options.maximum_execution_time_seconds > 0.0 &&
        numerix_context_elapsed_seconds(context->execution_context) >=
            context->base_options.maximum_execution_time_seconds) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        numerix_error_set(
            &context->callback_error,
            context->stop_status,
            "root discovery reached its execution time limit");
        return context->stop_status;
    }

    return NUMERIX_STATUS_SUCCESS;
}

static double budgeted_function(double x, void *user_data)
{
    discovery_context *context = user_data;
    double value = NAN;
    numerix_status status;

    if (context == NULL) {
        return NAN;
    }

    status = evaluate_request(context, x, &value);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return NAN;
    }
    return value;
}

static double budgeted_derivative(double x, void *user_data)
{
    discovery_context *context = user_data;

    if (context == NULL || context->request->derivative == NULL ||
        check_search_limits(context) != NUMERIX_STATUS_SUCCESS) {
        return NAN;
    }

    return context->request->derivative(
        x,
        context->request->user_data);
}

static numerix_status controlled_root_stop(
    void *user_data,
    const numerix_recommendation *recommendation)
{
    discovery_context *context = user_data;

    (void)recommendation;
    return check_search_limits(context);
}

static void add_unresolved_interval(
    discovery_context *context,
    double lower,
    double upper,
    numerix_status reason)
{
    numerix_root_search_report *report = context->report;
    size_t capacity = context->discovery_options->maximum_candidates;

    if (report->unresolved_count < SIZE_MAX) {
        ++report->unresolved_count;
    }
    if (report->stored_unresolved_count < capacity) {
        size_t required = report->stored_unresolved_count + 1U;
        numerix_status status = reserve_unresolved(context, required);

        if (status == NUMERIX_STATUS_SUCCESS) {
            numerix_root_unresolved_interval *interval =
                &report->unresolved[report->stored_unresolved_count];

            interval->lower = lower;
            interval->upper = upper;
            interval->reason = reason;
            ++report->stored_unresolved_count;
        } else {
            report->unresolved_storage_truncated = 1;
            context->stop_status = status;
        }
    } else {
        report->unresolved_storage_truncated = 1;
    }
}

static numerix_status begin_candidate(discovery_context *context)
{
    if (context->report->candidate_count >=
        context->discovery_options->maximum_candidates) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        numerix_error_set(
            &context->callback_error,
            context->stop_status,
            "root discovery reached its candidate limit");
        return context->stop_status;
    }

    ++context->report->candidate_count;
    return NUMERIX_STATUS_SUCCESS;
}

static double root_merge_tolerance(
    const discovery_context *context,
    double left,
    double right)
{
    double scale = fmax(fabs(left), fabs(right));

    return context->discovery_options->merge_absolute_tolerance +
           context->discovery_options->merge_relative_tolerance * scale;
}

static int same_root_candidate(
    const discovery_context *context,
    const numerix_discovered_root *root,
    double value,
    double lower,
    double upper)
{
    double tolerance = root_merge_tolerance(context, root->result.value, value);
    int intervals_overlap =
        lower <= root->candidate_upper + tolerance &&
        upper >= root->candidate_lower - tolerance;

    return intervals_overlap &&
           fabs(root->result.value - value) <= tolerance;
}

static void add_root_diagnostic(
    numerix_diagnostics_report *diagnostics,
    const char *identifier,
    numerix_diagnostic_severity severity,
    numerix_diagnostic_category category,
    const char *description,
    const char *cause,
    const char *consequence,
    const char *suggestion,
    int has_metric,
    double metric,
    double threshold)
{
    numerix_diagnostic_record *record;
    size_t index;

    if (diagnostics->warning_count >= 4U) {
        return;
    }

    index = diagnostics->warning_count;
    record = &diagnostics->diagnostics[index];
    (void)snprintf(
        record->identifier,
        sizeof(record->identifier),
        "%s",
        identifier);
    record->severity = severity;
    record->category = category;
    record->problem_category = NUMERIX_PROBLEM_ROOT_FINDING;
    record->algorithm = diagnostics->algorithm;
    (void)snprintf(
        record->description,
        sizeof(record->description),
        "%s",
        description);
    (void)snprintf(
        diagnostics->warning[index],
        sizeof(diagnostics->warning[index]),
        "%s",
        description);
    record->has_metric = has_metric;
    record->metric = metric;
    record->has_threshold = has_metric;
    record->threshold = threshold;
    (void)snprintf(
        record->possible_cause,
        sizeof(record->possible_cause),
        "%s",
        cause);
    (void)snprintf(
        record->consequence,
        sizeof(record->consequence),
        "%s",
        consequence);
    (void)snprintf(
        record->suggested_action,
        sizeof(record->suggested_action),
        "%s",
        suggestion);
    record->is_estimated = 1;
    record->is_heuristic = 1;
    ++diagnostics->warning_count;
}

static void initialize_root_diagnostics(
    numerix_discovered_root *root,
    double residual_tolerance)
{
    numerix_diagnostics_report *diagnostics = &root->diagnostics;

    memset(diagnostics, 0, sizeof(*diagnostics));
    diagnostics->category = NUMERIX_PROBLEM_ROOT_FINDING;
    diagnostics->solver_status = root->result.status;
    diagnostics->algorithm = root->refinement_algorithm;
    diagnostics->has_residual = 1;
    diagnostics->absolute_residual = fabs(root->result.residual);
    diagnostics->has_residual_tolerance = 1;
    diagnostics->requested_residual_tolerance = residual_tolerance;
    diagnostics->residual_satisfies_tolerance =
        diagnostics->absolute_residual <= residual_tolerance;
    diagnostics->reliability = diagnostics->residual_satisfies_tolerance
                                   ? NUMERIX_RELIABILITY_ACCEPTABLE
                                   : NUMERIX_RELIABILITY_LOW_CONFIDENCE;

    if (root->kind == NUMERIX_ROOT_TANGENTIAL) {
        add_root_diagnostic(
            diagnostics,
            "NUMERIX.ROOT.TANGENTIAL_CANDIDATE",
            NUMERIX_DIAGNOSTIC_NOTICE,
            NUMERIX_DIAGNOSTIC_CONVERGENCE,
            "The root was detected from local minimum evidence without a sign change.",
            "Finite samples cannot prove that the local minimum is an exact zero.",
            "The reported point satisfies the configured residual tolerance.",
            "Inspect the function near the candidate if stronger assurance is required.",
            1,
            fabs(root->result.residual),
            residual_tolerance);
        diagnostics->reliability = NUMERIX_RELIABILITY_CAUTION;
    }
}

static void insert_discovered_root(
    discovery_context *context,
    double value,
    double residual,
    double lower,
    double upper,
    numerix_root_discovery_kind kind,
    numerix_root_verification verification,
    numerix_algorithm algorithm,
    const numerix_result *refined_result,
    const numerix_root_selection_report *selection_report)
{
    numerix_root_search_report *report = context->report;
    numerix_discovered_root *root = NULL;
    size_t index;
    unsigned int evidence = 1U << (unsigned int)kind;

    for (index = 0; index < report->root_count; ++index) {
        if (same_root_candidate(
                context,
                &report->roots[index],
                value,
                lower,
                upper)) {
            root = &report->roots[index];
            break;
        }
    }

    if (root != NULL) {
        double separation = fabs(root->result.value - value);
        double position_tolerance =
            context->base_options.absolute_tolerance +
            context->base_options.relative_tolerance *
                fmax(fabs(root->result.value), fabs(value));
        int merge_limited = separation > position_tolerance;

        ++report->merged_candidates;
        ++root->merged_detections;
        root->evidence |= evidence;
        root->candidate_lower = fmin(root->candidate_lower, lower);
        root->candidate_upper = fmax(root->candidate_upper, upper);
        if (fabs(residual) < fabs(root->result.residual)) {
            root->result.value = value;
            root->result.residual = residual;
            root->result.estimated_error = refined_result != NULL
                ? refined_result->estimated_error : NAN;
            root->kind = kind;
            root->verification = verification;
            root->refinement_algorithm = algorithm;
            if (refined_result != NULL) {
                root->result.iterations = refined_result->iterations;
                root->result.function_evaluations =
                    refined_result->function_evaluations + 1U;
                root->result.execution_time_seconds =
                    refined_result->execution_time_seconds;
                root->result.algorithm = refined_result->algorithm;
            }
            if (selection_report != NULL) {
                root->refinement_history = *selection_report;
            }
            initialize_root_diagnostics(
                root,
                context->discovery_options->residual_tolerance);
        }
        if (merge_limited) {
            root->verification = NUMERIX_ROOT_ACCEPTED_WITH_LIMITATIONS;
            add_root_diagnostic(
                &root->diagnostics,
                "NUMERIX.ROOT.MERGE_TOLERANCE_LIMITATION",
                NUMERIX_DIAGNOSTIC_NOTICE,
                NUMERIX_DIAGNOSTIC_CONVERGENCE,
                "Distinct candidate locations were consolidated by the configured merge tolerance.",
                "The merge tolerance exceeds the distance between these candidate estimates.",
                "A closely spaced root may have been combined with this result.",
                "Reduce the merge tolerances and repeat the search to inspect nearby roots.",
                1,
                separation,
                position_tolerance);
            root->diagnostics.reliability = NUMERIX_RELIABILITY_CAUTION;
        }
        return;
    }

    if (report->root_count >= context->discovery_options->maximum_candidates) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        return;
    }

    if (reserve_roots(context, report->root_count + 1U) !=
        NUMERIX_STATUS_SUCCESS) {
        return;
    }

    root = &report->roots[report->root_count];
    memset(root, 0, sizeof(*root));
    root->result.status = NUMERIX_STATUS_SUCCESS;
    root->result.value = value;
    root->result.residual = residual;
    root->result.estimated_error = refined_result != NULL
        ? refined_result->estimated_error : NAN;
    root->result.algorithm = algorithm == NUMERIX_ALGORITHM_NONE
                                 ? "sample verification"
                                 : numerix_algorithm_name(algorithm);
    root->refinement_algorithm = algorithm;
    root->kind = kind;
    root->verification = verification;
    root->candidate_lower = lower;
    root->candidate_upper = upper;
    root->merged_detections = 1U;
    root->evidence = evidence;
    if (refined_result != NULL) {
        root->result.iterations = refined_result->iterations;
        root->result.function_evaluations =
            refined_result->function_evaluations + 1U;
        root->result.execution_time_seconds =
            refined_result->execution_time_seconds;
    } else {
        root->result.function_evaluations = 1U;
    }
    if (selection_report != NULL) {
        root->refinement_history = *selection_report;
    }
    initialize_root_diagnostics(
        root,
        context->discovery_options->residual_tolerance);
    ++report->root_count;
}

static numerix_status accept_candidate_value(
    discovery_context *context,
    double value,
    double residual,
    double lower,
    double upper,
    numerix_root_discovery_kind kind,
    numerix_algorithm algorithm,
    const numerix_result *refined_result,
    const numerix_root_selection_report *selection_report)
{
    numerix_status status;

    status = begin_candidate(context);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }

    if (!isfinite(value) || !isfinite(residual) ||
        fabs(residual) > context->discovery_options->residual_tolerance) {
        ++context->report->rejected_candidates;
        return NUMERIX_STATUS_SUCCESS;
    }

    insert_discovered_root(
        context,
        value,
        residual,
        lower,
        upper,
        kind,
        NUMERIX_ROOT_RESIDUAL_VERIFIED,
        algorithm,
        refined_result,
        selection_report);
    return context->stop_status;
}

static numerix_status refine_sign_change(
    discovery_context *context,
    const discovery_sample *left,
    const discovery_sample *right)
{
    numerix_recommendation_request refinement_request;
    numerix_root_selection_options selection_options;
    numerix_root_selection_report selection_report;
    numerix_options bounded_options;
    numerix_result result;
    numerix_error error;
    size_t used_evaluations;
    size_t used_iterations;
    numerix_status status;

    if (!sample_has_value(left) || !sample_has_value(right) ||
        !opposite_sign(left->value, right->value)) {
        return NUMERIX_STATUS_SUCCESS;
    }

    context->report->current_lower = left->x;
    context->report->current_upper = right->x;

    status = begin_candidate(context);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }

    used_evaluations = numerix_context_function_evaluation_count(
        context->execution_context);
    used_iterations = numerix_context_iteration_count(
        context->execution_context);
    if (used_evaluations >=
        context->base_options.maximum_function_evaluations) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            context->stop_status);
        return context->stop_status;
    }
    if (used_iterations >= context->base_options.maximum_iterations) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            context->stop_status);
        return context->stop_status;
    }

    bounded_options = context->base_options;
    bounded_options.maximum_iterations -= used_iterations;
    bounded_options.maximum_function_evaluations -= used_evaluations;
    refinement_request = *context->request;
    refinement_request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    refinement_request.function = budgeted_function;
    refinement_request.derivative = context->request->derivative == NULL
                                        ? NULL
                                        : budgeted_derivative;
    refinement_request.user_data = context;
    refinement_request.expression = NULL;
    refinement_request.piecewise_function = NULL;
    refinement_request.options = &bounded_options;
    refinement_request.problem.root.lower = left->x;
    refinement_request.problem.root.upper = right->x;
    refinement_request.problem.root.initial = interval_midpoint(
        left->x,
        right->x);
    refinement_request.problem.root.second_initial = right->x;
    refinement_request.problem.root.has_bracket = 1;
    refinement_request.problem.root.has_initial = 1;
    refinement_request.problem.root.has_second_initial = 1;

    selection_options = context->discovery_options->refinement;
    numerix_error_clear(&error);
    status = numerix_root_solve_auto_controlled(
        &refinement_request,
        &refinement_request,
        &selection_options,
        &result,
        &selection_report,
        &error,
        controlled_root_stop,
        context);

    if (selection_report.total_iterations > 0) {
        (void)numerix_context_accumulate_iterations(
            context->execution_context,
            selection_report.total_iterations);
        context->report->refinement_iterations +=
            selection_report.total_iterations;
    }
    context->report->function_evaluations =
        numerix_context_function_evaluation_count(
            context->execution_context);

    if (context->stop_status != NUMERIX_STATUS_SUCCESS) {
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            context->stop_status);
        return context->stop_status;
    }

    if (status != NUMERIX_STATUS_SUCCESS || !isfinite(result.value)) {
        if (status == NUMERIX_STATUS_LIMIT_REACHED) {
            context->stop_status = status;
        } else if (status == NUMERIX_STATUS_NUMERICAL_FAILURE ||
                   status == NUMERIX_STATUS_CONVERGENCE_FAILURE) {
            ++context->report->suspected_discontinuities;
        }
        ++context->report->rejected_candidates;
        add_unresolved_interval(context, left->x, right->x, status);
        return context->stop_status;
    }

    status = evaluate_request(context, result.value, &result.residual);
    if (status != NUMERIX_STATUS_SUCCESS) {
        if (status == NUMERIX_STATUS_LIMIT_REACHED ||
            status == NUMERIX_STATUS_CANCELLED) {
            return status;
        }
        ++context->report->rejected_candidates;
        ++context->report->suspected_discontinuities;
        add_unresolved_interval(context, left->x, right->x, status);
        return NUMERIX_STATUS_SUCCESS;
    }

    if (fabs(result.residual) >
        context->discovery_options->residual_tolerance) {
        ++context->report->rejected_candidates;
        ++context->report->suspected_discontinuities;
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            NUMERIX_STATUS_NUMERICAL_FAILURE);
        return NUMERIX_STATUS_SUCCESS;
    }

    insert_discovered_root(
        context,
        result.value,
        result.residual,
        left->x,
        right->x,
        NUMERIX_ROOT_SIGN_CHANGING,
        NUMERIX_ROOT_RESIDUAL_VERIFIED,
        selection_report.final_algorithm,
        &result,
        &selection_report);
    return context->stop_status;
}

static double absolute_value_objective(double x, void *user_data)
{
    double value = budgeted_function(x, user_data);

    return fabs(value);
}

static numerix_status refine_tangential_candidate(
    discovery_context *context,
    const discovery_sample *left,
    const discovery_sample *middle,
    const discovery_sample *right)
{
    numerix_options bounded_options;
    numerix_result minimum;
    numerix_root_selection_report unused_selection;
    size_t used_evaluations;
    size_t used_iterations;
    double x_tolerance;
    double residual;
    numerix_status status;

    if (!context->discovery_options->detect_tangential ||
        !sample_has_value(left) || !sample_has_value(middle) ||
        !sample_has_value(right)) {
        return NUMERIX_STATUS_SUCCESS;
    }

    if (!(fabs(middle->value) <= fabs(left->value) &&
          fabs(middle->value) <= fabs(right->value))) {
        return NUMERIX_STATUS_SUCCESS;
    }

    context->report->current_lower = left->x;
    context->report->current_upper = right->x;

    status = begin_candidate(context);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }

    used_evaluations = numerix_context_function_evaluation_count(
        context->execution_context);
    used_iterations = numerix_context_iteration_count(
        context->execution_context);
    if (used_evaluations >= context->base_options.maximum_function_evaluations ||
        used_iterations >= context->base_options.maximum_iterations) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            context->stop_status);
        return context->stop_status;
    }

    bounded_options = context->base_options;
    bounded_options.maximum_iterations -= used_iterations;
    bounded_options.maximum_function_evaluations -= used_evaluations;
    x_tolerance = bounded_options.absolute_tolerance +
                  bounded_options.relative_tolerance *
                      fmax(fabs(left->x), fabs(right->x));
    bounded_options.absolute_tolerance = fmax(
        x_tolerance,
        DBL_EPSILON * fmax(fabs(left->x), fabs(right->x)));
    bounded_options.relative_tolerance = 0.0;

    memset(&minimum, 0, sizeof(minimum));
    memset(&unused_selection, 0, sizeof(unused_selection));
    status = numerix_optimize_golden_section(
        absolute_value_objective,
        context,
        left->x,
        right->x,
        &bounded_options,
        &minimum,
        &context->callback_error);
    if (minimum.iterations > 0) {
        (void)numerix_context_accumulate_iterations(
            context->execution_context,
            minimum.iterations);
        context->report->refinement_iterations += minimum.iterations;
    }

    if (context->stop_status != NUMERIX_STATUS_SUCCESS) {
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            context->stop_status);
        return context->stop_status;
    }
    if (status != NUMERIX_STATUS_SUCCESS || !isfinite(minimum.value)) {
        if (status == NUMERIX_STATUS_LIMIT_REACHED) {
            context->stop_status = status;
        }
        ++context->report->rejected_candidates;
        add_unresolved_interval(context, left->x, right->x, status);
        return context->stop_status;
    }

    status = evaluate_request(context, minimum.value, &residual);
    if (status != NUMERIX_STATUS_SUCCESS) {
        if (status == NUMERIX_STATUS_LIMIT_REACHED ||
            status == NUMERIX_STATUS_CANCELLED) {
            return status;
        }
        ++context->report->rejected_candidates;
        add_unresolved_interval(context, left->x, right->x, status);
        return NUMERIX_STATUS_SUCCESS;
    }

    if (fabs(residual) > context->discovery_options->residual_tolerance) {
        ++context->report->rejected_candidates;
        return NUMERIX_STATUS_SUCCESS;
    }

    minimum.residual = residual;
    minimum.algorithm = "golden-section minimum of |f(x)|";
    insert_discovered_root(
        context,
        minimum.value,
        residual,
        fmax(left->x, minimum.value - minimum.estimated_error),
        fmin(right->x, minimum.value + minimum.estimated_error),
        NUMERIX_ROOT_TANGENTIAL,
        NUMERIX_ROOT_RESIDUAL_VERIFIED,
        NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION,
        &minimum,
        NULL);
    return context->stop_status;
}

static int interval_needs_subdivision(
    const discovery_context *context,
    const discovery_sample *left,
    const discovery_sample *middle,
    const discovery_sample *right)
{
    double left_abs;
    double middle_abs;
    double right_abs;
    double scale;
    double curvature;
    double linear_midpoint;
    double smaller_endpoint;
    int local_minimum;
    int slope_turn;

    if (!sample_has_value(left) || !sample_has_value(middle) ||
        !sample_has_value(right)) {
        return 0;
    }

    left_abs = fabs(left->value);
    middle_abs = fabs(middle->value);
    right_abs = fabs(right->value);
    smaller_endpoint = fmin(left_abs, right_abs);
    local_minimum = middle_abs <= left_abs && middle_abs <= right_abs &&
                    middle_abs <= context->discovery_options->residual_tolerance +
                                      context->discovery_options->tangential_reduction *
                                          smaller_endpoint;
    slope_turn =
        ((middle->value - left->value) >= 0.0 &&
         (right->value - middle->value) <= 0.0) ||
        ((middle->value - left->value) <= 0.0 &&
         (right->value - middle->value) >= 0.0);
    linear_midpoint = left->value * 0.5 + right->value * 0.5;
    scale = fmax(fmax(left_abs, middle_abs), fmax(right_abs, DBL_MIN));
    curvature = fabs(middle->value - linear_midpoint) / scale;

    return local_minimum ||
           (slope_turn && middle_abs <= smaller_endpoint) ||
           curvature >= context->discovery_options->curvature_threshold;
}

static numerix_status process_interval(
    discovery_context *context,
    const discovery_sample *left,
    const discovery_sample *right,
    size_t depth)
{
    discovery_sample middle;
    numerix_status status;
    int left_sign_change;
    int right_sign_change;
    int should_subdivide;
    double midpoint;

    if (context->stop_status != NUMERIX_STATUS_SUCCESS) {
        return context->stop_status;
    }

    context->report->current_lower = left->x;
    context->report->current_upper = right->x;
    if (depth > context->report->maximum_depth_reached) {
        context->report->maximum_depth_reached = depth;
    }

    if (sample_has_value(left) && left->value == 0.0) {
        status = accept_candidate_value(
            context,
            left->x,
            left->value,
            left->x,
            left->x,
            NUMERIX_ROOT_SAMPLED_ZERO,
            NUMERIX_ALGORITHM_NONE,
            NULL,
            NULL);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
    }
    if (sample_has_value(right) && right->value == 0.0) {
        status = accept_candidate_value(
            context,
            right->x,
            right->value,
            right->x,
            right->x,
            NUMERIX_ROOT_SAMPLED_ZERO,
            NUMERIX_ALGORITHM_NONE,
            NULL,
            NULL);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
    }

    midpoint = interval_midpoint(left->x, right->x);
    if (!(midpoint > left->x && midpoint < right->x)) {
        if (!sample_has_value(left) || !sample_has_value(right)) {
            add_unresolved_interval(
                context,
                left->x,
                right->x,
                NUMERIX_STATUS_NUMERICAL_FAILURE);
        }
        return NUMERIX_STATUS_SUCCESS;
    }

    middle.x = midpoint;
    middle.value = NAN;
    middle.status = evaluate_request(context, midpoint, &middle.value);
    if (middle.status == NUMERIX_STATUS_LIMIT_REACHED ||
        middle.status == NUMERIX_STATUS_CANCELLED) {
        return middle.status;
    }

    if (!sample_has_value(&middle)) {
        if (sample_has_value(left) && sample_has_value(right) &&
            opposite_sign(left->value, right->value)) {
            ++context->report->suspected_discontinuities;
        }
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            middle.status);
        return NUMERIX_STATUS_SUCCESS;
    }

    if (!sample_has_value(left) || !sample_has_value(right)) {
        ++context->report->suspected_discontinuities;
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            NUMERIX_STATUS_NUMERICAL_FAILURE);
    }

    if (middle.value == 0.0) {
        status = accept_candidate_value(
            context,
            middle.x,
            middle.value,
            middle.x,
            middle.x,
            NUMERIX_ROOT_SAMPLED_ZERO,
            NUMERIX_ALGORITHM_NONE,
            NULL,
            NULL);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
    }

    left_sign_change = sample_has_value(left) &&
                       opposite_sign(left->value, middle.value);
    right_sign_change = sample_has_value(right) &&
                        opposite_sign(middle.value, right->value);
    if (left_sign_change) {
        status = refine_sign_change(context, left, &middle);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
    }
    if (right_sign_change) {
        status = refine_sign_change(context, &middle, right);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
    }

    should_subdivide = interval_needs_subdivision(
        context,
        left,
        &middle,
        right);
    if (!should_subdivide) {
        return NUMERIX_STATUS_SUCCESS;
    }

    if (depth >= context->discovery_options->maximum_depth) {
        status = refine_tangential_candidate(context, left, &middle, right);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            NUMERIX_STATUS_CONVERGENCE_FAILURE);
        return NUMERIX_STATUS_SUCCESS;
    }

    if (context->report->subdivisions >=
        context->discovery_options->maximum_subdivisions) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        add_unresolved_interval(
            context,
            left->x,
            right->x,
            context->stop_status);
        return context->stop_status;
    }

    ++context->report->subdivisions;
    status = process_interval(context, left, &middle, depth + 1U);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    return process_interval(context, &middle, right, depth + 1U);
}

static int compare_double_values(const void *left, const void *right)
{
    double left_value = *(const double *)left;
    double right_value = *(const double *)right;

    if (left_value < right_value) {
        return -1;
    }
    if (left_value > right_value) {
        return 1;
    }
    return 0;
}

static int compare_discovered_roots(const void *left, const void *right)
{
    const numerix_discovered_root *left_root = left;
    const numerix_discovered_root *right_root = right;

    if (left_root->result.value < right_root->result.value) {
        return -1;
    }
    if (left_root->result.value > right_root->result.value) {
        return 1;
    }
    return 0;
}

static int is_piecewise_boundary(
    const discovery_context *context,
    double x)
{
    size_t index;

    for (index = 0; index < context->piecewise_boundary_count; ++index) {
        if (context->piecewise_boundaries[index] == x) {
            return 1;
        }
    }
    return 0;
}

static size_t piecewise_defined_sides(
    const numerix_piecewise_function *function,
    double boundary)
{
    size_t index;
    size_t sides = 0;

    for (index = 0; index < function->count; ++index) {
        const numerix_piecewise_segment *segment = &function->segments[index];

        if (segment->lower_bound < boundary &&
            segment->upper_bound >= boundary) {
            ++sides;
        }
        if (segment->lower_bound <= boundary &&
            segment->upper_bound > boundary) {
            ++sides;
        }
    }
    return sides;
}

static numerix_status analyze_piecewise_boundaries(
    discovery_context *context)
{
    size_t index;

    if (context->request->representation != NUMERIX_REPRESENTATION_PIECEWISE) {
        return NUMERIX_STATUS_SUCCESS;
    }

    for (index = 0; index < context->piecewise_boundary_count; ++index) {
        numerix_options analysis_options = context->base_options;
        numerix_piecewise_boundary_report boundary_report;
        numerix_error analysis_error;
        double boundary = context->piecewise_boundaries[index];
        size_t expected_evaluations = piecewise_defined_sides(
            context->request->piecewise_function,
            boundary) + 1U;
        size_t used_evaluations = numerix_context_function_evaluation_count(
            context->execution_context);
        numerix_status status;

        if (used_evaluations >
            context->base_options.maximum_function_evaluations ||
            expected_evaluations >
                context->base_options.maximum_function_evaluations -
                    used_evaluations) {
            add_unresolved_interval(
                context,
                boundary,
                boundary,
                NUMERIX_STATUS_LIMIT_REACHED);
            continue;
        }

        if (check_search_limits(context) != NUMERIX_STATUS_SUCCESS) {
            return context->stop_status;
        }

        analysis_options.maximum_iterations = 1U;
        numerix_error_clear(&analysis_error);
        status = numerix_piecewise_analyze_boundary(
            context->request->piecewise_function,
            boundary,
            &analysis_options,
            &boundary_report,
            &analysis_error);
        (void)numerix_context_accumulate_function_evaluations(
            context->execution_context,
            expected_evaluations);
        context->report->function_evaluations =
            numerix_context_function_evaluation_count(
                context->execution_context);

        if (status == NUMERIX_STATUS_SUCCESS) {
            if (boundary_report.left_status ==
                NUMERIX_STATUS_NUMERICAL_FAILURE) {
                ++context->report->evaluation_failures;
            }
            if (boundary_report.right_status ==
                NUMERIX_STATUS_NUMERICAL_FAILURE) {
                ++context->report->evaluation_failures;
            }
            if (boundary_report.function_status != NUMERIX_STATUS_SUCCESS) {
                ++context->report->evaluation_failures;
            }
        }

        if (status == NUMERIX_STATUS_SUCCESS &&
            (boundary_report.function_status != NUMERIX_STATUS_SUCCESS ||
             boundary_report.type == NUMERIX_DISCONTINUITY_REMOVABLE ||
             boundary_report.type == NUMERIX_DISCONTINUITY_INFINITE)) {
            ++context->report->suspected_discontinuities;
        }

        status = notify_progress(context, 0);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return status;
        }
    }

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status prepare_piecewise_boundaries(
    discovery_context *context)
{
    size_t total_count = 0;
    size_t index;
    size_t filtered_count = 0;
    numerix_status status;

    if (context->request->representation != NUMERIX_REPRESENTATION_PIECEWISE) {
        return NUMERIX_STATUS_SUCCESS;
    }

    status = numerix_piecewise_get_boundaries(
        context->request->piecewise_function,
        NULL,
        0,
        &total_count,
        NULL);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    if (total_count == 0) {
        return NUMERIX_STATUS_SUCCESS;
    }
    if (total_count > SIZE_MAX / sizeof(double)) {
        return NUMERIX_STATUS_ALLOCATION_FAILURE;
    }

    context->piecewise_boundary_capacity = total_count;
    context->piecewise_boundaries = discovery_allocate(
        context,
        total_count * sizeof(*context->piecewise_boundaries));
    if (context->piecewise_boundaries == NULL) {
        return NUMERIX_STATUS_ALLOCATION_FAILURE;
    }
    status = numerix_piecewise_get_boundaries(
        context->request->piecewise_function,
        context->piecewise_boundaries,
        total_count,
        &total_count,
        NULL);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }

    for (index = 0; index < total_count; ++index) {
        double boundary = context->piecewise_boundaries[index];

        if (boundary > context->report->lower &&
            boundary < context->report->upper) {
            context->piecewise_boundaries[filtered_count] = boundary;
            ++filtered_count;
        }
    }
    context->piecewise_boundary_count = filtered_count;
    context->report->known_boundaries = filtered_count;
    return analyze_piecewise_boundaries(context);
}

static numerix_status estimate_root_multiplicity(
    discovery_context *context,
    numerix_discovered_root *root)
{
    double scale;
    double minimum_step;
    double interval_width;
    double step;
    double values[4];
    double estimates[2];
    double points[4];
    size_t index;
    numerix_status status;

    if (!context->discovery_options->estimate_multiplicity ||
        root->kind == NUMERIX_ROOT_PIECEWISE_BOUNDARY) {
        return NUMERIX_STATUS_SUCCESS;
    }

    scale = fmax(1.0, fabs(root->result.value));
    minimum_step = pow(DBL_EPSILON, 0.25) * scale;
    interval_width = root->candidate_upper - root->candidate_lower;
    step = fmax(minimum_step, interval_width * 2.0);
    if (!(step > 0.0) || !isfinite(step) ||
        root->result.value - 2.0 * step <= context->report->lower ||
        root->result.value + 2.0 * step >= context->report->upper) {
        return NUMERIX_STATUS_SUCCESS;
    }

    points[0] = root->result.value - 2.0 * step;
    points[1] = root->result.value - step;
    points[2] = root->result.value + step;
    points[3] = root->result.value + 2.0 * step;
    for (index = 0; index < 4U; ++index) {
        status = evaluate_request(context, points[index], &values[index]);
        if (status != NUMERIX_STATUS_SUCCESS) {
            if (status == NUMERIX_STATUS_LIMIT_REACHED ||
                status == NUMERIX_STATUS_CANCELLED) {
                return status;
            }
            return NUMERIX_STATUS_SUCCESS;
        }
    }

    if (values[0] == 0.0 || values[1] == 0.0 ||
        values[2] == 0.0 || values[3] == 0.0) {
        return NUMERIX_STATUS_SUCCESS;
    }

    estimates[0] = log2(fabs(values[0] / values[1]));
    estimates[1] = log2(fabs(values[3] / values[2]));
    if (!isfinite(estimates[0]) || !isfinite(estimates[1])) {
        return NUMERIX_STATUS_SUCCESS;
    }

    {
        double average = (estimates[0] + estimates[1]) * 0.5;
        double nearest = nearbyint(average);

        if (nearest >= 1.0 && nearest <= 8.0 &&
            fabs(estimates[0] - estimates[1]) <= 0.45 &&
            fabs(average - nearest) <= 0.35) {
            root->has_multiplicity_estimate = 1;
            root->estimated_multiplicity = (unsigned int)nearest;
            root->multiplicity_confidence = NUMERIX_DIAGNOSTIC_SUSPECTED;
            if (root->estimated_multiplicity > 1U) {
                root->kind = NUMERIX_ROOT_REPEATED;
            }
        }
    }

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status estimate_root_condition(
    discovery_context *context,
    numerix_discovered_root *root)
{
    numerix_options bounded_options;
    numerix_root_condition_report condition;
    numerix_error error;
    size_t used_evaluations;
    size_t used_iterations;
    double step;
    numerix_function derivative = NULL;
    numerix_status status;

    if (!context->discovery_options->estimate_condition) {
        return NUMERIX_STATUS_SUCCESS;
    }

    used_evaluations = numerix_context_function_evaluation_count(
        context->execution_context);
    used_iterations = numerix_context_iteration_count(
        context->execution_context);
    if (used_evaluations >= context->base_options.maximum_function_evaluations ||
        used_iterations >= context->base_options.maximum_iterations) {
        context->stop_status = NUMERIX_STATUS_LIMIT_REACHED;
        return context->stop_status;
    }

    bounded_options = context->base_options;
    bounded_options.maximum_function_evaluations -= used_evaluations;
    bounded_options.maximum_iterations -= used_iterations;
    step = cbrt(DBL_EPSILON) * fmax(1.0, fabs(root->result.value));
    if (context->request->derivative != NULL) {
        derivative = budgeted_derivative;
    }

    numerix_error_clear(&error);
    status = numerix_diagnostics_root_condition(
        budgeted_function,
        derivative,
        context,
        root->result.value,
        step,
        &bounded_options,
        &condition,
        &error);
    if (context->stop_status != NUMERIX_STATUS_SUCCESS) {
        return context->stop_status;
    }
    if (status != NUMERIX_STATUS_SUCCESS) {
        add_root_diagnostic(
            &root->diagnostics,
            "NUMERIX.ROOT.CONDITION_UNAVAILABLE",
            NUMERIX_DIAGNOSTIC_NOTICE,
            NUMERIX_DIAGNOSTIC_CONDITIONING,
            "Root conditioning could not be estimated reliably.",
            "The derivative or local finite-difference stencil was inconclusive.",
            "Sensitivity near this root is unknown.",
            "Inspect the local function scale and derivative behaviour.",
            0,
            0.0,
            0.0);
        return NUMERIX_STATUS_SUCCESS;
    }

    root->conditioning = condition;
    if (condition.condition_available) {
        root->diagnostics.has_condition_estimate = 1;
        root->diagnostics.condition_estimate =
            condition.reciprocal_derivative_condition;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static int validate_discovery_options(
    const numerix_root_discovery_options *options)
{
    if (options->initial_panels == 0 ||
        options->initial_panels > NUMERIX_ROOT_DISCOVERY_MAX_PANELS ||
        options->maximum_depth == 0 ||
        options->maximum_depth > NUMERIX_ROOT_DISCOVERY_MAX_DEPTH ||
        options->maximum_subdivisions == 0 ||
        options->maximum_subdivisions >
            NUMERIX_ROOT_DISCOVERY_MAX_SUBDIVISIONS ||
        options->maximum_candidates == 0 ||
        options->maximum_candidates >
            NUMERIX_ROOT_DISCOVERY_MAX_CANDIDATES ||
        !isfinite(options->residual_tolerance) ||
        options->residual_tolerance < 0.0 ||
        !isfinite(options->curvature_threshold) ||
        options->curvature_threshold <= 0.0 ||
        options->curvature_threshold > 2.0 ||
        !isfinite(options->tangential_reduction) ||
        options->tangential_reduction <= 0.0 ||
        options->tangential_reduction >= 1.0 ||
        !isfinite(options->merge_absolute_tolerance) ||
        options->merge_absolute_tolerance < 0.0 ||
        !isfinite(options->merge_relative_tolerance) ||
        options->merge_relative_tolerance < 0.0 ||
        (options->merge_absolute_tolerance == 0.0 &&
         options->merge_relative_tolerance == 0.0) ||
        (options->detect_tangential != 0 &&
         options->detect_tangential != 1) ||
        (options->estimate_multiplicity != 0 &&
         options->estimate_multiplicity != 1) ||
        (options->estimate_condition != 0 &&
         options->estimate_condition != 1) ||
        (options->refinement.enable_fallback != 0 &&
         options->refinement.enable_fallback != 1) ||
        (options->refinement.mode != NUMERIX_SELECTION_AUTOMATIC &&
         options->refinement.mode != NUMERIX_SELECTION_EXPLICIT)) {
        return 0;
    }

    if (options->refinement.mode == NUMERIX_SELECTION_EXPLICIT &&
        options->refinement.explicit_algorithm != NUMERIX_ALGORITHM_BISECTION &&
        options->refinement.explicit_algorithm != NUMERIX_ALGORITHM_REGULA_FALSI &&
        options->refinement.explicit_algorithm != NUMERIX_ALGORITHM_BRENT &&
        options->refinement.explicit_algorithm != NUMERIX_ALGORITHM_NEWTON &&
        options->refinement.explicit_algorithm != NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON) {
        return 0;
    }

    return 1;
}

void numerix_root_discovery_options_init(
    numerix_root_discovery_options *options)
{
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->initial_panels = 32U;
    options->maximum_depth = 12U;
    options->maximum_subdivisions = 4096U;
    options->maximum_candidates = 256U;
    options->residual_tolerance = 1e-8;
    options->curvature_threshold = 0.25;
    options->tangential_reduction = 0.75;
    options->merge_absolute_tolerance = 1e-8;
    options->merge_relative_tolerance = 1e-10;
    options->detect_tangential = 1;
    options->estimate_multiplicity = 1;
    options->estimate_condition = 0;
    options->refinement.mode = NUMERIX_SELECTION_AUTOMATIC;
    options->refinement.explicit_algorithm = NUMERIX_ALGORITHM_NONE;
    options->refinement.enable_fallback = 1;
    options->progress_interval = 64U;
}

void numerix_root_search_report_destroy(numerix_root_search_report *report)
{
    size_t index;

    if (report == NULL) {
        return;
    }

    for (index = 0; index < report->root_count; ++index) {
        numerix_diagnostics_report_destroy(&report->roots[index].diagnostics);
    }
    numerix_deallocate(report->roots);
    numerix_deallocate(report->unresolved);
    memset(report, 0, sizeof(*report));
}

static void discard_discovered_roots(discovery_context *context)
{
    numerix_root_search_report *report = context->report;
    size_t index;

    for (index = 0; index < report->root_count; ++index) {
        numerix_diagnostics_report_destroy(&report->roots[index].diagnostics);
    }
    report->root_count = 0U;
    discovery_deallocate(
        context,
        report->roots,
        context->root_capacity * sizeof(*report->roots));
    report->roots = NULL;
    context->root_capacity = 0U;
    update_storage_metrics(context);
}

numerix_status numerix_root_find_all(
    const numerix_recommendation_request *request,
    double lower,
    double upper,
    const numerix_root_discovery_options *input_options,
    numerix_root_search_report *report,
    numerix_error *error)
{
    numerix_root_discovery_options default_discovery_options;
    const numerix_root_discovery_options *options;
    numerix_options base_options;
    numerix_root_search_report created_report;
    discovery_context context;
    double *cuts = NULL;
    discovery_sample *cut_samples = NULL;
    size_t boundary_count;
    size_t cut_capacity;
    size_t cut_count = 0;
    size_t unique_count;
    size_t index;
    int sampled_continuum = 0;
    numerix_status status;

    if (report == NULL) {
        return discovery_error(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "root search report output is required");
    }
    memset(report, 0, sizeof(*report));
    report->status = NUMERIX_STATUS_INVALID_ARGUMENT;
    if (request == NULL || request->category != NUMERIX_PROBLEM_ROOT_FINDING ||
        !numerix_request_has_function(request) || !isfinite(lower) ||
        !isfinite(upper) || !(lower < upper)) {
        report->status = NUMERIX_STATUS_INVALID_INPUT;
        return discovery_error(
            error,
            NUMERIX_STATUS_INVALID_INPUT,
            "an evaluable root request and finite increasing search interval are required");
    }

    if (input_options == NULL) {
        numerix_root_discovery_options_init(&default_discovery_options);
        options = &default_discovery_options;
    } else {
        options = input_options;
    }
    if (!validate_discovery_options(options)) {
        report->status = NUMERIX_STATUS_INVALID_INPUT;
        return discovery_error(
            error,
            NUMERIX_STATUS_INVALID_INPUT,
            "root discovery options contain an invalid value");
    }

    if (request->options == NULL) {
        numerix_options_init(&base_options);
    } else {
        base_options = *request->options;
        status = numerix_options_validate(&base_options, error);
        if (status != NUMERIX_STATUS_SUCCESS) {
            report->status = status;
            return status;
        }
    }

    if (options->refinement.mode == NUMERIX_SELECTION_EXPLICIT &&
        (options->refinement.explicit_algorithm == NUMERIX_ALGORITHM_NEWTON ||
         options->refinement.explicit_algorithm ==
             NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON) &&
        request->derivative == NULL) {
        report->status = NUMERIX_STATUS_INVALID_INPUT;
        return discovery_error(
            error,
            NUMERIX_STATUS_INVALID_INPUT,
            "the selected refinement algorithm requires a derivative callback");
    }

    memset(&created_report, 0, sizeof(created_report));
    created_report.lower = lower;
    created_report.upper = upper;
    created_report.current_lower = lower;
    created_report.current_upper = upper;
    created_report.search_inconclusive = 1;
    memset(&context, 0, sizeof(context));
    context.request = request;
    context.discovery_options = options;
    context.report = &created_report;
    context.base_options = base_options;
    context.stop_status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(&context.callback_error);
    status = numerix_context_create(
        &base_options,
        &context.execution_context,
        error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        numerix_root_search_report_destroy(&created_report);
        report->status = status;
        return status;
    }

    status = notify_progress(&context, 1);
    if (status == NUMERIX_STATUS_SUCCESS) {
        status = prepare_piecewise_boundaries(&context);
    }
    if (status != NUMERIX_STATUS_SUCCESS &&
        context.stop_status == NUMERIX_STATUS_SUCCESS) {
        context.stop_status = status;
        numerix_error_set(
            &context.callback_error,
            status,
            "could not prepare piecewise boundary information");
    }

    boundary_count = context.piecewise_boundary_count;
    if (context.stop_status != NUMERIX_STATUS_SUCCESS) {
        cut_capacity = 0U;
    } else if (boundary_count > SIZE_MAX - options->initial_panels - 1U) {
        context.stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
        cut_capacity = 0U;
    } else {
        cut_capacity = options->initial_panels + 1U + boundary_count;
        if (cut_capacity > SIZE_MAX / sizeof(*cuts) ||
            cut_capacity > SIZE_MAX / sizeof(*cut_samples)) {
            context.stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
        } else {
            cuts = discovery_allocate(
                &context,
                cut_capacity * sizeof(*cuts));
            if (cuts != NULL) {
                cut_samples = discovery_allocate(
                    &context,
                    cut_capacity * sizeof(*cut_samples));
            }
            if (cuts == NULL || cut_samples == NULL) {
                context.stop_status = NUMERIX_STATUS_ALLOCATION_FAILURE;
            }
        }
    }

    if (context.stop_status == NUMERIX_STATUS_SUCCESS) {
        for (index = 0; index <= options->initial_panels; ++index) {
            double fraction = (double)index /
                              (double)options->initial_panels;
            cuts[cut_count] = interpolate_interval(lower, upper, fraction);
            ++cut_count;
        }
        for (index = 0; index < boundary_count; ++index) {
            cuts[cut_count] = context.piecewise_boundaries[index];
            ++cut_count;
        }

        qsort(cuts, cut_count, sizeof(*cuts), compare_double_values);
        unique_count = 0;
        for (index = 0; index < cut_count; ++index) {
            if (unique_count == 0 || cuts[index] != cuts[unique_count - 1U]) {
                cuts[unique_count] = cuts[index];
                ++unique_count;
            }
        }
        cut_count = unique_count;

        for (index = 0; index < cut_count; ++index) {
            cut_samples[index].x = cuts[index];
            cut_samples[index].value = NAN;
            cut_samples[index].status = evaluate_request(
                &context,
                cuts[index],
                &cut_samples[index].value);
            if (cut_samples[index].status == NUMERIX_STATUS_LIMIT_REACHED ||
                cut_samples[index].status == NUMERIX_STATUS_CANCELLED) {
                context.stop_status = cut_samples[index].status;
                break;
            }

            if (sample_has_value(&cut_samples[index]) &&
                cut_samples[index].value == 0.0) {
                numerix_root_discovery_kind kind =
                    cuts[index] == lower || cuts[index] == upper
                        ? NUMERIX_ROOT_ENDPOINT
                        : is_piecewise_boundary(&context, cuts[index])
                              ? NUMERIX_ROOT_PIECEWISE_BOUNDARY
                              : NUMERIX_ROOT_SAMPLED_ZERO;
                status = accept_candidate_value(
                    &context,
                    cuts[index],
                    0.0,
                    cuts[index],
                    cuts[index],
                    kind,
                    NUMERIX_ALGORITHM_NONE,
                    NULL,
                    NULL);
                if (status != NUMERIX_STATUS_SUCCESS) {
                    context.stop_status = status;
                    break;
                }
            }
        }

        if (context.stop_status == NUMERIX_STATUS_SUCCESS && cut_count > 1U) {
            sampled_continuum = 1;
            for (index = 0; index < cut_count; ++index) {
                if (!sample_has_value(&cut_samples[index]) ||
                    cut_samples[index].value != 0.0) {
                    sampled_continuum = 0;
                    break;
                }
            }
            if (sampled_continuum) {
                discard_discovered_roots(&context);
                add_unresolved_interval(
                    &context,
                    lower,
                    upper,
                    NUMERIX_STATUS_CONVERGENCE_FAILURE);
            }
        }

        for (index = 1U; index + 1U < cut_count &&
             context.stop_status == NUMERIX_STATUS_SUCCESS &&
             !sampled_continuum; ++index) {
            discovery_sample *left = &cut_samples[index - 1U];
            discovery_sample *middle = &cut_samples[index];
            discovery_sample *right = &cut_samples[index + 1U];

            if (is_piecewise_boundary(&context, left->x) ||
                is_piecewise_boundary(&context, middle->x) ||
                is_piecewise_boundary(&context, right->x)) {
                continue;
            }
            if (!sample_has_value(left) || !sample_has_value(middle) ||
                !sample_has_value(right) ||
                fabs(middle->value) > fabs(left->value) ||
                fabs(middle->value) > fabs(right->value)) {
                continue;
            }

            status = refine_tangential_candidate(
                &context,
                left,
                middle,
                right);
            if (status != NUMERIX_STATUS_SUCCESS) {
                context.stop_status = status;
            }
        }

        for (index = 0; index + 1U < cut_count &&
             context.stop_status == NUMERIX_STATUS_SUCCESS &&
             !sampled_continuum; ++index) {
            discovery_sample left = cut_samples[index];
            discovery_sample right = cut_samples[index + 1U];

            if (is_piecewise_boundary(&context, left.x)) {
                left.x = nextafter(left.x, right.x);
                left.value = NAN;
                left.status = evaluate_request(
                    &context,
                    left.x,
                    &left.value);
            }
            if (is_piecewise_boundary(&context, right.x)) {
                right.x = nextafter(right.x, left.x);
                right.value = NAN;
                right.status = evaluate_request(
                    &context,
                    right.x,
                    &right.value);
            }

            if (left.status == NUMERIX_STATUS_LIMIT_REACHED ||
                left.status == NUMERIX_STATUS_CANCELLED ||
                right.status == NUMERIX_STATUS_LIMIT_REACHED ||
                right.status == NUMERIX_STATUS_CANCELLED) {
                context.stop_status = left.status == NUMERIX_STATUS_SUCCESS
                                          ? right.status
                                          : left.status;
                break;
            }
            if (!(left.x < right.x)) {
                continue;
            }
            status = process_interval(&context, &left, &right, 0U);
            if (status != NUMERIX_STATUS_SUCCESS) {
                context.stop_status = status;
            }
        }
    }

    if (created_report.root_count > 1U) {
        qsort(
            created_report.roots,
            created_report.root_count,
            sizeof(*created_report.roots),
            compare_discovered_roots);
    }

    for (index = 0; index < created_report.root_count &&
         context.stop_status == NUMERIX_STATUS_SUCCESS; ++index) {
        status = estimate_root_multiplicity(
            &context,
            &created_report.roots[index]);
        if (status == NUMERIX_STATUS_SUCCESS) {
            status = estimate_root_condition(
                &context,
                &created_report.roots[index]);
        }
        if (status != NUMERIX_STATUS_SUCCESS) {
            context.stop_status = status;
        }
    }

    created_report.status = context.stop_status;
    created_report.completed_normally =
        context.stop_status == NUMERIX_STATUS_SUCCESS;
    created_report.resource_limit_reached =
        context.stop_status == NUMERIX_STATUS_LIMIT_REACHED;
    created_report.function_evaluations =
        numerix_context_function_evaluation_count(context.execution_context);
    created_report.elapsed_seconds =
        numerix_context_elapsed_seconds(context.execution_context);
    update_storage_metrics(&context);
    (void)notify_progress(&context, 1);
    created_report.status = context.stop_status;
    created_report.completed_normally =
        context.stop_status == NUMERIX_STATUS_SUCCESS;
    created_report.resource_limit_reached =
        context.stop_status == NUMERIX_STATUS_LIMIT_REACHED;
    created_report.function_evaluations =
        numerix_context_function_evaluation_count(context.execution_context);
    created_report.elapsed_seconds =
        numerix_context_elapsed_seconds(context.execution_context);

    status = created_report.status;
    discovery_deallocate(
        &context,
        cuts,
        cuts == NULL ? 0U : cut_capacity * sizeof(*cuts));
    discovery_deallocate(
        &context,
        cut_samples,
        cut_samples == NULL ? 0U : cut_capacity * sizeof(*cut_samples));
    discovery_deallocate(
        &context,
        context.piecewise_boundaries,
        context.piecewise_boundary_capacity *
            sizeof(*context.piecewise_boundaries));
    numerix_context_destroy(context.execution_context);
    *report = created_report;

    if (status == NUMERIX_STATUS_SUCCESS) {
        numerix_error_clear(error);
    } else if (context.callback_error.code != NUMERIX_STATUS_SUCCESS) {
        if (error != NULL) {
            *error = context.callback_error;
        }
    } else {
        numerix_error_set(
            error,
            status,
            "root discovery stopped before the configured search completed");
    }
    return status;
}
