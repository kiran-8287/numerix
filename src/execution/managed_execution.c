#include "numerix_internal.h"
#include "numerix_execution_internal.h"
#include "../intelligence/intelligence_internal.h"

#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

struct numerix_managed_execution
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_execution_state state;
    numerix_status status;
    numerix_status callback_status;
    numerix_context *context;
    atomic_int cancel_requested;
    atomic_int pause_requested;
    atomic_int limit_reached;
    double start_seconds;
    double finish_seconds;
    numerix_algorithm algorithm;
    numerix_algorithm initial_algorithm;
    int started;
    numerix_result result;
    numerix_error error;
    numerix_root_selection_report root_history;
    size_t derivative_evaluations;
    int has_result;
    int has_approximation;
    double latest_approximation;
    double latest_residual;
    int bisection_initialized;
    double lower;
    double upper;
    double lower_value;
    double upper_value;
};

static double execution_now(void)
{
    return numerix_platform_monotonic_seconds();
}

static numerix_status managed_stop_check(void *user_data)
{
    numerix_managed_execution *execution = user_data;

    if (atomic_load_explicit(&execution->cancel_requested,
                             memory_order_acquire) != 0)
    {
        return NUMERIX_STATUS_CANCELLED;
    }
    if (atomic_load_explicit(&execution->limit_reached,
                             memory_order_acquire) != 0)
    {
        return NUMERIX_STATUS_LIMIT_REACHED;
    }
    if (execution->request.options != NULL &&
        execution->request.options->maximum_execution_time_seconds > 0.0 &&
        execution->start_seconds > 0.0 &&
        execution_now() - execution->start_seconds >=
            execution->request.options->maximum_execution_time_seconds)
    {
        atomic_store_explicit(&execution->limit_reached, 1,
                              memory_order_release);
        return NUMERIX_STATUS_LIMIT_REACHED;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static void emit_event(
    numerix_managed_execution *execution, numerix_execution_event_type type)
{
    numerix_execution_event event;

    if (execution->options.event_callback == NULL)
    {
        return;
    }
    event.type = type;
    event.state = execution->state;
    event.algorithm = execution->algorithm;
    event.status = execution->status;
    event.iteration = execution->result.iterations;
    event.function_evaluations =
        numerix_context_function_evaluation_count(execution->context);
    execution->options.event_callback(
        execution, &event, execution->options.callback_user_data);
}

static void finish_execution(
    numerix_managed_execution *execution, numerix_status status)
{
    if (status != NUMERIX_STATUS_SUCCESS &&
        execution->has_approximation &&
        (!isfinite(execution->result.value) ||
         !isfinite(execution->result.residual)))
    {
        execution->result.value = execution->latest_approximation;
        execution->result.residual = execution->latest_residual;
    }
    execution->status = status;
    execution->result.status = status;
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_error_clear(&execution->error);
    }
    else if (execution->error.code == NUMERIX_STATUS_SUCCESS)
    {
        numerix_error_set(&execution->error, status,
                          numerix_status_string(status));
    }
    execution->finish_seconds = execution_now();

    if (status == NUMERIX_STATUS_SUCCESS)
    {
        execution->state = NUMERIX_EXECUTION_COMPLETED;
        emit_event(execution, NUMERIX_EVENT_COMPLETED);
    }
    else if (status == NUMERIX_STATUS_CANCELLED)
    {
        execution->state = NUMERIX_EXECUTION_CANCELLED;
        emit_event(execution, NUMERIX_EVENT_CANCELLED);
    }
    else if (status == NUMERIX_STATUS_LIMIT_REACHED)
    {
        execution->state = NUMERIX_EXECUTION_LIMIT_REACHED;
        emit_event(execution, NUMERIX_EVENT_LIMIT_REACHED);
    }
    else
    {
        execution->state = NUMERIX_EXECUTION_FAILED;
        emit_event(execution, NUMERIX_EVENT_FAILED);
    }
}

static void capture_execution_error(
    numerix_managed_execution *execution, numerix_status status,
    const numerix_error *error)
{
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_error_clear(&execution->error);
    }
    else if (error != NULL && error->code != NUMERIX_STATUS_SUCCESS)
    {
        execution->error = *error;
    }
    else if (execution->error.code == NUMERIX_STATUS_SUCCESS)
    {
        numerix_error_set(&execution->error, status,
                          numerix_status_string(status));
    }
}

static numerix_status evaluate_managed(
    numerix_managed_execution *execution, double x, double *value)
{
    numerix_options options;
    numerix_status status;

    if (atomic_load_explicit(&execution->cancel_requested,
                             memory_order_acquire) != 0)
    {
        return NUMERIX_STATUS_CANCELLED;
    }
    if (execution->request.options == NULL)
    {
        numerix_options_init(&options);
    }
    else
    {
        options = *execution->request.options;
    }
    if (numerix_context_function_evaluation_count(execution->context) >=
            options.maximum_function_evaluations ||
        execution->derivative_evaluations >=
            options.maximum_function_evaluations -
                numerix_context_function_evaluation_count(execution->context))
    {
        atomic_store_explicit(&execution->limit_reached, 1,
                              memory_order_release);
        return NUMERIX_STATUS_LIMIT_REACHED;
    }
    if (options.maximum_execution_time_seconds > 0.0 &&
        execution_now() - execution->start_seconds >=
            options.maximum_execution_time_seconds)
    {
        atomic_store_explicit(&execution->limit_reached, 1,
                              memory_order_release);
        return NUMERIX_STATUS_LIMIT_REACHED;
    }

    status = numerix_request_evaluate(
        &execution->request, x, value, &execution->error);
    (void)numerix_context_record_function_evaluation(execution->context);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        execution->callback_status = status;
    }
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        execution->latest_approximation = x;
        execution->latest_residual = *value;
        execution->has_approximation = 1;
    }
    return status;
}

static double managed_function(double x, void *user_data)
{
    numerix_managed_execution *execution = user_data;
    double value = NAN;

    if (evaluate_managed(execution, x, &value) != NUMERIX_STATUS_SUCCESS)
    {
        return NAN;
    }
    return value;
}

static double managed_derivative(double x, void *user_data)
{
    numerix_managed_execution *execution = user_data;
    numerix_options options;
    numerix_status stop_status;
    double value;

    stop_status = managed_stop_check(execution);
    if (stop_status != NUMERIX_STATUS_SUCCESS ||
        execution->request.derivative == NULL)
    {
        return NAN;
    }
    if (execution->request.options == NULL)
    {
        numerix_options_init(&options);
    }
    else
    {
        options = *execution->request.options;
    }
    if (numerix_context_function_evaluation_count(execution->context) >=
            options.maximum_function_evaluations ||
        execution->derivative_evaluations >=
            options.maximum_function_evaluations -
                numerix_context_function_evaluation_count(execution->context))
    {
        atomic_store_explicit(&execution->limit_reached, 1,
                              memory_order_release);
        return NAN;
    }
    value = execution->request.derivative(
        x, execution->request.user_data);
    execution->derivative_evaluations += 1;
    return value;
}

static void emit_bisection_step(numerix_managed_execution *execution,
                                double midpoint, double value)
{
    numerix_execution_step step;

    if (execution->options.step_callback == NULL)
    {
        return;
    }
    memset(&step, 0, sizeof(step));
    step.number = execution->result.iterations;
    step.algorithm = NUMERIX_ALGORITHM_BISECTION;
    step.valid_fields = NUMERIX_STEP_HAS_APPROXIMATION |
                        NUMERIX_STEP_HAS_RESIDUAL |
                        NUMERIX_STEP_HAS_INTERVAL |
                        NUMERIX_STEP_HAS_MIDPOINT;
    step.iteration = execution->result.iterations;
    step.function_evaluations =
        numerix_context_function_evaluation_count(execution->context);
    step.approximation = midpoint;
    step.residual = value;
    step.interval_lower = execution->lower;
    step.interval_upper = execution->upper;
    step.midpoint = midpoint;
    execution->options.step_callback(
        execution, &step, execution->options.callback_user_data);
}

static numerix_status run_bisection(numerix_managed_execution *execution,
                                    numerix_error *error)
{
    numerix_options options;
    numerix_status status;

    if (execution->request.options == NULL)
    {
        numerix_options_init(&options);
    }
    else
    {
        options = *execution->request.options;
    }
    if (!execution->bisection_initialized)
    {
        execution->lower = execution->request.problem.root.lower;
        execution->upper = execution->request.problem.root.upper;
        status = evaluate_managed(
            execution, execution->lower, &execution->lower_value);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            goto finished;
        }
        status = evaluate_managed(
            execution, execution->upper, &execution->upper_value);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            goto finished;
        }
        execution->bisection_initialized = 1;
        if (execution->lower_value == 0.0 || execution->upper_value == 0.0)
        {
            double root = execution->lower_value == 0.0
                              ? execution->lower
                              : execution->upper;
            double residual = execution->lower_value == 0.0
                                  ? execution->lower_value
                                  : execution->upper_value;
            execution->result.value = root;
            execution->result.residual = residual;
            execution->result.estimated_error = 0.0;
            execution->has_result = 1;
            status = NUMERIX_STATUS_SUCCESS;
            goto finished;
        }
        if ((execution->lower_value < 0.0) ==
            (execution->upper_value < 0.0))
        {
            status = NUMERIX_STATUS_INVALID_INPUT;
            numerix_error_set(error, status,
                              "Bisection requires a sign-changing interval");
            goto finished;
        }
    }

    execution->state = NUMERIX_EXECUTION_RUNNING;
    while (execution->result.iterations < options.maximum_iterations)
    {
        double midpoint;
        double midpoint_value;

        if (atomic_load_explicit(&execution->cancel_requested,
                                 memory_order_acquire) != 0)
        {
            status = NUMERIX_STATUS_CANCELLED;
            goto finished;
        }
        if (atomic_exchange_explicit(&execution->pause_requested, 0,
                                     memory_order_acq_rel) != 0)
        {
            execution->state = NUMERIX_EXECUTION_PAUSED;
            emit_event(execution, NUMERIX_EVENT_PAUSED);
            numerix_error_clear(error);
            return NUMERIX_STATUS_SUCCESS;
        }

        midpoint = execution->lower +
                   (execution->upper - execution->lower) * 0.5;
        if (midpoint == execution->lower || midpoint == execution->upper)
        {
            status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            numerix_error_set(error, status,
                              "Bisection interval can no longer be reduced");
            goto finished;
        }
        status = evaluate_managed(execution, midpoint, &midpoint_value);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            goto finished;
        }
        execution->result.iterations += 1;
        execution->result.function_evaluations =
            numerix_context_function_evaluation_count(execution->context);
        (void)numerix_context_record_iteration(execution->context);
        execution->result.value = midpoint;
        execution->result.residual = midpoint_value;
        execution->result.estimated_error =
            (execution->upper - execution->lower) * 0.5;
        execution->has_result = 1;

        if (midpoint_value == 0.0)
        {
            status = NUMERIX_STATUS_SUCCESS;
            emit_bisection_step(execution, midpoint, midpoint_value);
            goto finished;
        }
        if ((execution->lower_value < 0.0) != (midpoint_value < 0.0))
        {
            execution->upper = midpoint;
            execution->upper_value = midpoint_value;
        }
        else
        {
            execution->lower = midpoint;
            execution->lower_value = midpoint_value;
        }
        emit_bisection_step(execution, midpoint, midpoint_value);

        if ((execution->upper - execution->lower) * 0.5 <=
            options.absolute_tolerance +
                options.relative_tolerance * fabs(midpoint))
        {
            status = NUMERIX_STATUS_SUCCESS;
            goto finished;
        }
    }
    status = NUMERIX_STATUS_LIMIT_REACHED;

finished:
    execution->result.algorithm = "bisection";
    execution->result.function_evaluations =
        numerix_context_function_evaluation_count(execution->context);
    execution->result.execution_time_seconds =
        fmax(0.0, execution_now() - execution->start_seconds);
    execution->result.status = status;
    if (status == NUMERIX_STATUS_CANCELLED)
    {
        numerix_error_set(error, status, "execution was cancelled");
    }
    else if (status == NUMERIX_STATUS_LIMIT_REACHED)
    {
        numerix_error_set(error, status, "execution resource limit reached");
    }
    else if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_error_clear(error);
    }
    capture_execution_error(execution, status, error);
    finish_execution(execution, status);
    return status;
}

numerix_status numerix_managed_execution_create(
    const numerix_recommendation_request *request,
    const numerix_managed_execution_options *options,
    numerix_managed_execution **out, numerix_error *error)
{
    numerix_managed_execution *execution;
    numerix_options numerical_options;
    numerix_status status;

    if (out == NULL || request == NULL ||
        request->category != NUMERIX_PROBLEM_ROOT_FINDING ||
        !numerix_request_has_function(request))
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "managed execution supports root problems");
    }
    *out = NULL;
    if (request->options == NULL)
    {
        numerix_options_init(&numerical_options);
    }
    else
    {
        numerical_options = *request->options;
    }
    status = numerix_options_validate(&numerical_options, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    execution = numerix_allocate(sizeof(*execution));
    if (execution == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                                 "could not allocate managed execution");
    }
    memset(execution, 0, sizeof(*execution));
    status = numerix_context_create(
        &numerical_options, &execution->context, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        numerix_deallocate(execution);
        return status;
    }
    execution->request = *request;
    execution->state = NUMERIX_EXECUTION_READY;
    execution->status = NUMERIX_STATUS_SUCCESS;
    execution->algorithm = options == NULL
                               ? NUMERIX_ALGORITHM_NONE
                               : options->root_selection.explicit_algorithm;
    execution->initial_algorithm = execution->algorithm;
    if (options != NULL)
    {
        execution->options = *options;
    }
    else
    {
        memset(&execution->options, 0, sizeof(execution->options));
        execution->options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;
    }
    if (execution->options.root_selection.enable_fallback != 0 &&
        execution->options.root_selection.enable_fallback != 1)
    {
        numerix_context_destroy(execution->context);
        numerix_deallocate(execution);
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "fallback flag must be zero or one");
    }
    if (execution->options.root_selection.mode == NUMERIX_SELECTION_EXPLICIT &&
        execution->options.root_selection.explicit_algorithm ==
            NUMERIX_ALGORITHM_BISECTION &&
        (!request->problem.root.has_bracket ||
         !isfinite(request->problem.root.lower) ||
         !isfinite(request->problem.root.upper) ||
         !(request->problem.root.lower < request->problem.root.upper)))
    {
        numerix_context_destroy(execution->context);
        numerix_deallocate(execution);
        return numerix_error_set(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "explicit Bisection requires a finite increasing bracket");
    }
    atomic_init(&execution->cancel_requested, 0);
    atomic_init(&execution->pause_requested, 0);
    atomic_init(&execution->limit_reached, 0);
    execution->result.value = NAN;
    execution->result.residual = NAN;
    execution->result.estimated_error = NAN;
    numerix_error_clear(&execution->error);
    *out = execution;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_managed_execution_run(
    numerix_managed_execution *execution, numerix_error *error)
{
    numerix_status status;

    if (execution == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "execution is required");
    }
    if (execution->state == NUMERIX_EXECUTION_COMPLETED ||
        execution->state == NUMERIX_EXECUTION_FAILED ||
        execution->state == NUMERIX_EXECUTION_CANCELLED ||
        execution->state == NUMERIX_EXECUTION_LIMIT_REACHED)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "execution has already terminated");
    }
    if (execution->state == NUMERIX_EXECUTION_RUNNING)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "execution is already running");
    }
    if (execution->start_seconds == 0.0)
    {
        execution->start_seconds = execution_now();
    }
    if (atomic_load_explicit(&execution->cancel_requested,
                             memory_order_acquire) != 0)
    {
        finish_execution(execution, NUMERIX_STATUS_CANCELLED);
        return numerix_error_set(error, NUMERIX_STATUS_CANCELLED,
                                 "execution cancelled before start");
    }

    execution->state = NUMERIX_EXECUTION_RUNNING;
    if (!execution->started)
    {
        execution->started = 1;
        emit_event(execution, NUMERIX_EVENT_STARTED);
    }
    if (execution->algorithm == NUMERIX_ALGORITHM_BISECTION &&
        execution->options.root_selection.enable_fallback == 0)
    {
        status = run_bisection(execution, error);
        return status;
    }

    {
        numerix_recommendation_request managed_request = execution->request;
        numerix_root_selection_options selection =
            execution->options.root_selection;
        numerix_root_selection_report report;
        numerix_result root_result;

        memset(&root_result, 0, sizeof(root_result));
        managed_request.representation = NUMERIX_REPRESENTATION_CALLBACK;
        managed_request.function = managed_function;
        managed_request.user_data = execution;
        managed_request.derivative = execution->request.derivative == NULL
                                         ? NULL
                                         : managed_derivative;
        managed_request.expression = NULL;
        managed_request.piecewise_function = NULL;
        status = numerix_root_solve_auto_controlled(
            &managed_request, &selection, &root_result, &report, error,
            managed_stop_check, execution);
        execution->result = root_result;
        execution->root_history = report;
        execution->algorithm = report.final_algorithm;
        execution->initial_algorithm = report.initially_selected;
        execution->result.iterations = report.total_iterations;
        if (numerix_context_accumulate_iterations(
                execution->context, report.total_iterations) !=
            NUMERIX_STATUS_SUCCESS)
        {
            status = NUMERIX_STATUS_LIMIT_REACHED;
            execution->result.status = status;
        }
        execution->result.function_evaluations =
            numerix_context_function_evaluation_count(execution->context);
        execution->result.execution_time_seconds =
            fmax(0.0, execution_now() - execution->start_seconds);
        execution->has_result = status == NUMERIX_STATUS_SUCCESS &&
                                report.attempt_count > 0 &&
                                isfinite(execution->result.value);
        if (atomic_load_explicit(&execution->cancel_requested,
                                 memory_order_acquire) != 0)
        {
            status = NUMERIX_STATUS_CANCELLED;
            numerix_error_set(error, status, "execution was cancelled");
        }
        else if (execution->callback_status != NUMERIX_STATUS_SUCCESS)
        {
            status = execution->callback_status;
            if (error != NULL)
            {
                *error = execution->error;
            }
        }
        else if (atomic_load_explicit(&execution->limit_reached,
                                      memory_order_acquire) != 0)
        {
            status = NUMERIX_STATUS_LIMIT_REACHED;
            numerix_error_set(error, status, "execution resource limit reached");
        }
        capture_execution_error(execution, status, error);
        finish_execution(execution, status);
    }
    return status;
}

numerix_status numerix_managed_execution_request_cancel(
    numerix_managed_execution *execution)
{
    if (execution == NULL)
    {
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    atomic_store_explicit(&execution->cancel_requested, 1,
                          memory_order_release);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_managed_execution_request_pause(
    numerix_managed_execution *execution)
{
    if (execution == NULL ||
        execution->algorithm != NUMERIX_ALGORITHM_BISECTION ||
        execution->options.root_selection.enable_fallback != 0)
    {
        return NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    }
    atomic_store_explicit(&execution->pause_requested, 1,
                          memory_order_release);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_managed_execution_resume(
    numerix_managed_execution *execution, numerix_error *error)
{
    if (execution == NULL || execution->state != NUMERIX_EXECUTION_PAUSED)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "only a paused execution can resume");
    }
    if (atomic_load_explicit(&execution->cancel_requested,
                             memory_order_acquire) != 0)
    {
        finish_execution(execution, NUMERIX_STATUS_CANCELLED);
        return numerix_error_set(error, NUMERIX_STATUS_CANCELLED,
                                 "paused execution was cancelled");
    }
    execution->state = NUMERIX_EXECUTION_READY;
    emit_event(execution, NUMERIX_EVENT_RESUMED);
    return numerix_managed_execution_run(execution, error);
}

numerix_status numerix_managed_execution_get_progress(
    const numerix_managed_execution *execution,
    numerix_execution_progress *progress, numerix_error *error)
{
    if (execution == NULL || progress == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "execution and progress output are required");
    }
    memset(progress, 0, sizeof(*progress));
    progress->state = execution->state;
    progress->category = execution->request.category;
    progress->algorithm = execution->algorithm;
    progress->iterations =
        numerix_context_iteration_count(execution->context);
    progress->iterations_available =
        execution->algorithm == NUMERIX_ALGORITHM_BISECTION ||
        execution->state == NUMERIX_EXECUTION_COMPLETED ||
        execution->state == NUMERIX_EXECUTION_FAILED ||
        execution->state == NUMERIX_EXECUTION_CANCELLED ||
        execution->state == NUMERIX_EXECUTION_LIMIT_REACHED;
    progress->algorithm_available =
        execution->algorithm != NUMERIX_ALGORITHM_NONE;
    progress->function_evaluations = execution->result.function_evaluations;
    progress->derivative_evaluations = execution->derivative_evaluations;
    progress->has_approximation = execution->has_result;
    progress->has_residual = execution->has_result;
    progress->approximation = execution->result.value;
    progress->residual = execution->result.residual;
    progress->has_interval = execution->bisection_initialized;
    progress->interval_lower = execution->lower;
    progress->interval_upper = execution->upper;
    progress->elapsed_seconds = execution->finish_seconds > 0.0
                                    ? execution->finish_seconds -
                                          execution->start_seconds
                                    : fmax(0.0, execution_now() -
                                                   execution->start_seconds);
    progress->percentage_available = 0;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_managed_execution_get_summary(
    const numerix_managed_execution *execution,
    numerix_execution_summary *summary, numerix_error *error)
{
    if (execution == NULL || summary == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "execution and summary output are required");
    }
    memset(summary, 0, sizeof(*summary));
    summary->state = execution->state;
    summary->status = execution->status;
    summary->category = execution->request.category;
    summary->initially_selected = execution->initial_algorithm;
    summary->final_algorithm = execution->algorithm;
    summary->iterations =
        numerix_context_iteration_count(execution->context);
    summary->function_evaluations =
        numerix_context_function_evaluation_count(execution->context);
    summary->derivative_evaluations = execution->derivative_evaluations;
    summary->has_result = execution->has_result;
    summary->result = execution->result;
    summary->error = execution->error;
    summary->root_history = execution->root_history;
    summary->fallback_attempts = execution->root_history.fallback_used
                                     ? execution->root_history.attempt_count - 1
                                     : 0;
    summary->elapsed_seconds = execution->finish_seconds > 0.0
                                   ? execution->finish_seconds -
                                         execution->start_seconds
                                   : fmax(0.0, execution_now() -
                                                  execution->start_seconds);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void numerix_managed_execution_destroy(
    numerix_managed_execution *execution)
{
    if (execution != NULL)
    {
        numerix_context_destroy(execution->context);
    }
    numerix_deallocate(execution);
}
