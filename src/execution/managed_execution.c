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
    numerix_execution_output output;
    size_t derivative_evaluations;
    int has_result;
    int has_output;
    int has_approximation;
    int recommendation_counters_recorded;
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

static numerix_status managed_stop_check(
    void *user_data, const numerix_recommendation *recommendation)
{
    numerix_managed_execution *execution = user_data;

    if (recommendation != NULL &&
        !execution->recommendation_counters_recorded)
    {
        (void)numerix_context_accumulate_function_evaluations(
            execution->context, recommendation->probe_evaluations);
        execution->derivative_evaluations +=
            recommendation->derivative_probe_evaluations;
        execution->recommendation_counters_recorded = 1;
    }
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
    if (execution->request.category == NUMERIX_PROBLEM_ROOT_FINDING &&
        status != NUMERIX_STATUS_SUCCESS &&
        execution->has_approximation &&
        (!isfinite(execution->result.value) ||
         !isfinite(execution->result.residual)))
    {
        execution->result.value = execution->latest_approximation;
        execution->result.residual = execution->latest_residual;
    }
    execution->status = status;
    execution->result.status = status;
    if (execution->request.category == NUMERIX_PROBLEM_ROOT_FINDING)
    {
        execution->output.category = execution->request.category;
        execution->output.algorithm = execution->algorithm;
        execution->output.scalar_result = execution->result;
        execution->output.root_selection = execution->root_history;
        execution->output.selected_automatically =
            execution->options.root_selection.mode ==
            NUMERIX_SELECTION_AUTOMATIC;
        execution->output.recommendation =
            execution->root_history.recommendation;
        execution->has_output = 1;
    }
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

static numerix_status managed_budget_check(
    numerix_managed_execution *execution)
{
    numerix_options options;
    size_t function_evaluations =
        numerix_context_function_evaluation_count(execution->context);

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
    if (function_evaluations >= options.maximum_function_evaluations ||
        execution->derivative_evaluations >=
            options.maximum_function_evaluations - function_evaluations)
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
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status evaluate_managed(
    numerix_managed_execution *execution, double x, double *value)
{
    numerix_status status;

    status = managed_budget_check(execution);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
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
    numerix_status stop_status;
    double value;

    stop_status = managed_budget_check(execution);
    if (stop_status != NUMERIX_STATUS_SUCCESS ||
        execution->request.derivative == NULL)
    {
        return NAN;
    }
    value = execution->request.derivative(
        x, execution->request.user_data);
    execution->derivative_evaluations += 1;
    return value;
}

static double managed_objective(const double *values, size_t dimension,
                               void *user_data)
{
    numerix_managed_execution *execution = user_data;
    numerix_objective_function objective =
        execution->request.problem.optimization.objective;
    double value;

    if (managed_budget_check(execution) != NUMERIX_STATUS_SUCCESS ||
        objective == NULL)
    {
        return NAN;
    }
    value = objective(values, dimension, execution->request.user_data);
    (void)numerix_context_record_function_evaluation(execution->context);
    return value;
}

static numerix_status managed_gradient(const double *values,
                                       size_t dimension, double *gradient,
                                       void *user_data)
{
    numerix_managed_execution *execution = user_data;
    numerix_gradient_function function =
        execution->request.problem.optimization.gradient;
    numerix_status status;

    status = managed_budget_check(execution);
    if (status != NUMERIX_STATUS_SUCCESS || function == NULL)
    {
        return status == NUMERIX_STATUS_SUCCESS
                   ? NUMERIX_STATUS_INVALID_ARGUMENT
                   : status;
    }
    status = function(values, dimension, gradient,
                      execution->request.user_data);
    execution->derivative_evaluations += 1;
    return status;
}

static numerix_status managed_ode_function(
    double x, const double *state, size_t dimension, double *derivative,
    void *user_data)
{
    numerix_managed_execution *execution = user_data;
    numerix_ode_function function = execution->request.problem.ode.function;
    numerix_status status;

    status = managed_budget_check(execution);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    status = function(x, state, dimension, derivative,
                      execution->request.user_data);
    (void)numerix_context_record_function_evaluation(execution->context);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        execution->callback_status = status;
    }
    return status;
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

static numerix_status run_general_execution(
    numerix_managed_execution *execution, numerix_error *error)
{
    numerix_recommendation_request request = execution->request;
    numerix_execution_options options = execution->options.execution;
    numerix_options numerical_options;
    numerix_status status;
    int scalar_function_managed = 0;

    execution->output.category = request.category;

    if (execution->request.options == NULL)
    {
        numerix_options_init(&numerical_options);
    }
    else
    {
        numerical_options = *execution->request.options;
    }
    if (request.category == NUMERIX_PROBLEM_ODE &&
        request.problem.ode.steps > numerical_options.maximum_iterations)
    {
        status = NUMERIX_STATUS_LIMIT_REACHED;
        execution->output.category = request.category;
        execution->output.scalar_result.status = status;
        execution->result = execution->output.scalar_result;
        execution->result.status = status;
        execution->has_output = 1;
        numerix_error_set(error, status,
                          "ODE step count exceeds the managed iteration limit");
        capture_execution_error(execution, status, error);
        finish_execution(execution, status);
        return status;
    }

    if (((request.category == NUMERIX_PROBLEM_DIFFERENTIATION ||
          request.category == NUMERIX_PROBLEM_INTEGRATION) &&
         request.representation != NUMERIX_REPRESENTATION_PIECEWISE) ||
        (request.category == NUMERIX_PROBLEM_OPTIMIZATION &&
         request.function != NULL))
    {
        scalar_function_managed = 1;
        request.representation = NUMERIX_REPRESENTATION_CALLBACK;
        request.function = managed_function;
        request.user_data = execution;
        request.expression = NULL;
        request.piecewise_function = NULL;
        request.derivative = request.derivative == NULL
                                 ? NULL
                                 : managed_derivative;
    }
    if (request.category == NUMERIX_PROBLEM_OPTIMIZATION)
    {
        request.user_data = execution;
        if (request.problem.optimization.objective != NULL)
        {
            request.problem.optimization.objective = managed_objective;
        }
        if (request.problem.optimization.gradient != NULL)
        {
            request.problem.optimization.gradient = managed_gradient;
        }
    }
    if (request.category == NUMERIX_PROBLEM_ODE)
    {
        request.problem.ode.function = managed_ode_function;
        request.user_data = execution;
    }

    status = numerix_execute_recommended(
        &request, &options, &execution->output, error);
    execution->has_output = 1;
    execution->algorithm = execution->output.algorithm;
    execution->initial_algorithm = execution->output.algorithm;
    execution->result = execution->output.scalar_result;
    if (request.category == NUMERIX_PROBLEM_ODE)
    {
        execution->result.status = status;
        execution->result.algorithm =
            numerix_algorithm_name(execution->output.algorithm);
        if (execution->output.has_ode_result &&
            execution->output.ode_result.points > 0)
        {
            execution->result.iterations =
                execution->output.ode_result.points - 1;
        }
    }
    if (!scalar_function_managed &&
        (request.category == NUMERIX_PROBLEM_INTEGRATION ||
         request.category == NUMERIX_PROBLEM_DIFFERENTIATION))
    {
        (void)numerix_context_accumulate_function_evaluations(
            execution->context,
            execution->output.scalar_result.function_evaluations);
    }
    execution->result.function_evaluations =
        numerix_context_function_evaluation_count(execution->context);
    execution->result.execution_time_seconds =
        fmax(0.0, execution_now() - execution->start_seconds);
    if (execution->result.iterations > 0)
    {
        (void)numerix_context_accumulate_iterations(
            execution->context, execution->result.iterations);
    }
    execution->has_result =
        status == NUMERIX_STATUS_SUCCESS &&
        (execution->output.vector_result != NULL ||
         execution->output.has_ode_result ||
         isfinite(execution->result.value));

    if (atomic_load_explicit(&execution->cancel_requested,
                             memory_order_acquire) != 0)
    {
        status = NUMERIX_STATUS_CANCELLED;
        numerix_error_set(error, status, "execution was cancelled");
    }
    else if (atomic_load_explicit(&execution->limit_reached,
                                  memory_order_acquire) != 0)
    {
        status = NUMERIX_STATUS_LIMIT_REACHED;
        numerix_error_set(error, status, "execution resource limit reached");
    }
    else if (execution->callback_status != NUMERIX_STATUS_SUCCESS)
    {
        status = execution->callback_status;
        if (error != NULL)
        {
            *error = execution->error;
        }
    }
    execution->result.status = status;
    execution->output.scalar_result = execution->result;
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

    if (out != NULL)
    {
        *out = NULL;
    }
    if (out == NULL || request == NULL ||
        request->category < NUMERIX_PROBLEM_ROOT_FINDING ||
        request->category > NUMERIX_PROBLEM_ODE)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "managed execution request is invalid");
    }
    if ((request->category == NUMERIX_PROBLEM_ROOT_FINDING ||
         request->category == NUMERIX_PROBLEM_DIFFERENTIATION ||
         request->category == NUMERIX_PROBLEM_INTEGRATION) &&
        !numerix_request_has_function(request))
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "managed request requires a scalar function");
    }
    if (request->category == NUMERIX_PROBLEM_ODE &&
        request->problem.ode.function == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "managed ODE request requires a callback");
    }
    if (request->category == NUMERIX_PROBLEM_OPTIMIZATION &&
        !numerix_request_has_function(request) &&
        request->problem.optimization.objective == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "managed optimization request is incomplete");
    }
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
    execution->output.category = request->category;
    execution->state = NUMERIX_EXECUTION_READY;
    execution->status = NUMERIX_STATUS_SUCCESS;
    if (options == NULL)
    {
        execution->algorithm = NUMERIX_ALGORITHM_NONE;
    }
    else if (request->category == NUMERIX_PROBLEM_ROOT_FINDING)
    {
        execution->algorithm = options->root_selection.explicit_algorithm;
    }
    else
    {
        execution->algorithm = options->execution.explicit_algorithm;
    }
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
    if (execution->options.execution.enable_root_fallback != 0 &&
        execution->options.execution.enable_root_fallback != 1)
    {
        numerix_context_destroy(execution->context);
        numerix_deallocate(execution);
        return numerix_error_set(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "execution fallback flag must be zero or one");
    }
    if (request->category == NUMERIX_PROBLEM_ROOT_FINDING &&
        execution->options.root_selection.mode == NUMERIX_SELECTION_EXPLICIT &&
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
        execution->request.category == NUMERIX_PROBLEM_ROOT_FINDING &&
        execution->options.root_selection.enable_fallback == 0)
    {
        status = run_bisection(execution, error);
        return status;
    }
    if (execution->request.category != NUMERIX_PROBLEM_ROOT_FINDING)
    {
        return run_general_execution(execution, error);
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
            &execution->request, &managed_request, &selection, &root_result,
            &report, error,
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
        execution->request.category != NUMERIX_PROBLEM_ROOT_FINDING ||
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

static int execution_is_terminal(numerix_execution_state state)
{
    return state == NUMERIX_EXECUTION_COMPLETED ||
           state == NUMERIX_EXECUTION_FAILED ||
           state == NUMERIX_EXECUTION_CANCELLED ||
           state == NUMERIX_EXECUTION_LIMIT_REACHED;
}

static int execution_iterations_available(
    const numerix_managed_execution *execution)
{
    if (execution->request.category == NUMERIX_PROBLEM_ROOT_FINDING)
    {
        return execution->algorithm == NUMERIX_ALGORITHM_BISECTION ||
               execution_is_terminal(execution->state);
    }
    if (execution_is_terminal(execution->state) && execution->has_output &&
        (execution->request.category == NUMERIX_PROBLEM_INTEGRATION ||
         execution->request.category == NUMERIX_PROBLEM_OPTIMIZATION ||
         execution->request.category == NUMERIX_PROBLEM_ODE))
    {
        return 1;
    }
    return 0;
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
        execution_iterations_available(execution);
    progress->algorithm_available =
        execution->algorithm != NUMERIX_ALGORITHM_NONE;
    progress->function_evaluations =
        numerix_context_function_evaluation_count(execution->context);
    progress->derivative_evaluations = execution->derivative_evaluations;
    if (execution->request.category == NUMERIX_PROBLEM_ROOT_FINDING)
    {
        progress->has_approximation = execution->has_approximation;
        progress->has_residual = execution->has_approximation;
        progress->approximation = execution->latest_approximation;
        progress->residual = execution->latest_residual;
    }
    else if (execution->has_output && execution->has_result)
    {
        progress->has_approximation =
            isfinite(execution->output.scalar_result.value);
        progress->approximation = execution->output.scalar_result.value;
        if (execution->request.category == NUMERIX_PROBLEM_LINEAR_ALGEBRA &&
            isfinite(execution->output.scalar_result.residual))
        {
            progress->has_residual = 1;
            progress->residual = execution->output.scalar_result.residual;
        }
        if (execution->request.category == NUMERIX_PROBLEM_INTEGRATION &&
            isfinite(execution->output.scalar_result.estimated_error))
        {
            progress->has_estimated_error = 1;
            progress->estimated_error =
                execution->output.scalar_result.estimated_error;
        }
    }
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
    summary->iterations_available =
        execution_iterations_available(execution);
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

numerix_status numerix_managed_execution_get_output(
    const numerix_managed_execution *execution,
    const numerix_execution_output **output, numerix_error *error)
{
    if (execution == NULL || output == NULL)
    {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "execution and output pointer are required");
    }
    *output = &execution->output;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void numerix_managed_execution_destroy(
    numerix_managed_execution *execution)
{
    if (execution != NULL)
    {
        numerix_execution_output_destroy(&execution->output);
        numerix_context_destroy(execution->context);
    }
    numerix_deallocate(execution);
}
