#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            fprintf(stderr, "FAIL %s:%d: %s\n", \
                    __FILE__, __LINE__, #condition); \
            failures += 1; \
        } \
    } while (0)

typedef struct observer_state
{
    numerix_managed_execution *execution;
    size_t steps;
    size_t events;
    int pause_after_first_step;
} observer_state;

typedef struct cancelling_input
{
    numerix_managed_execution *execution;
    size_t calls;
    size_t cancel_on_call;
} cancelling_input;

typedef struct cancelling_ode_input
{
    numerix_managed_execution *execution;
    size_t calls;
} cancelling_ode_input;

static double quadratic(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
}

static double square(double x, void *user_data)
{
    (void)user_data;
    return x * x;
}

static double shifted_square(double x, void *user_data)
{
    (void)user_data;
    return (x - 3.0) * (x - 3.0);
}

static double vector_objective(const double *values, size_t dimension,
                               void *user_data)
{
    size_t index;
    double sum = 0.0;

    (void)user_data;
    for (index = 0; index < dimension; ++index)
    {
        double difference = values[index] - 3.0;
        sum += difference * difference;
    }
    return sum;
}

static numerix_status vector_gradient(const double *values, size_t dimension,
                                     double *gradient, void *user_data)
{
    size_t index;

    (void)user_data;
    for (index = 0; index < dimension; ++index)
    {
        gradient[index] = 2.0 * (values[index] - 3.0);
    }
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status exponential_ode(double x, const double *state,
                                     size_t dimension, double *derivative,
                                     void *user_data)
{
    size_t index;

    (void)x;
    (void)user_data;
    for (index = 0; index < dimension; ++index)
    {
        derivative[index] = state[index];
    }
    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status cancelling_ode(double x, const double *state,
                                     size_t dimension, double *derivative,
                                     void *user_data)
{
    cancelling_ode_input *input = user_data;
    size_t index;

    (void)x;
    input->calls += 1;
    if (input->calls == 1)
    {
        (void)numerix_managed_execution_request_cancel(input->execution);
    }
    for (index = 0; index < dimension; ++index)
    {
        derivative[index] = state[index];
    }
    return NUMERIX_STATUS_SUCCESS;
}

static double cancelling_quadratic(double x, void *user_data)
{
    cancelling_input *input = user_data;

    input->calls += 1;
    if (input->calls == input->cancel_on_call)
    {
        (void)numerix_managed_execution_request_cancel(input->execution);
    }
    return x * x - 2.0;
}

static double cancelling_integrand(double x, void *user_data)
{
    cancelling_input *input = user_data;

    input->calls += 1;
    if (input->calls == input->cancel_on_call)
    {
        (void)numerix_managed_execution_request_cancel(input->execution);
    }
    return x * x;
}

static void observe_event(numerix_managed_execution *execution,
                          const numerix_execution_event *event,
                          void *user_data)
{
    observer_state *observer = user_data;

    (void)execution;
    CHECK(event != NULL);
    observer->events += 1;
}

static void observe_step(numerix_managed_execution *execution,
                         const numerix_execution_step *step,
                         void *user_data)
{
    observer_state *observer = user_data;

    CHECK(step->number == observer->steps + 1);
    CHECK(step->algorithm == NUMERIX_ALGORITHM_BISECTION);
    CHECK((step->valid_fields & NUMERIX_STEP_HAS_INTERVAL) != 0);
    CHECK(step->interval_lower < step->interval_upper);
    CHECK(step->midpoint >= step->interval_lower);
    CHECK(step->midpoint <= step->interval_upper);
    observer->steps += 1;
    if (observer->pause_after_first_step && observer->steps == 1)
    {
        CHECK(numerix_managed_execution_request_pause(execution) ==
              NUMERIX_STATUS_SUCCESS);
    }
}

static void initialize_root_request(numerix_recommendation_request *request)
{
    memset(request, 0, sizeof(*request));
    request->category = NUMERIX_PROBLEM_ROOT_FINDING;
    request->representation = NUMERIX_REPRESENTATION_CALLBACK;
    request->function = quadratic;
    request->problem.root.lower = 0.0;
    request->problem.root.upper = 2.0;
    request->problem.root.has_bracket = 1;
}

static void test_bisection_pause_resume(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_progress progress;
    numerix_execution_summary summary;
    numerix_error error;
    observer_state observer;
    numerix_status status;

    initialize_root_request(&request);
    memset(&observer, 0, sizeof(observer));
    observer.pause_after_first_step = 1;
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    options.event_callback = observe_event;
    options.step_callback = observe_step;
    options.callback_user_data = &observer;

    status = numerix_managed_execution_create(
        &request, &options, &execution, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    observer.execution = execution;
    status = numerix_managed_execution_run(execution, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_progress(
              execution, &progress, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(progress.state == NUMERIX_EXECUTION_PAUSED);
    CHECK(progress.iterations == 1);
    CHECK(progress.function_evaluations == 3);
    CHECK(progress.percentage_available == 0);

    observer.pause_after_first_step = 0;
    status = numerix_managed_execution_resume(execution, &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_COMPLETED);
    CHECK(summary.has_result);
    CHECK(fabs(summary.result.value - sqrt(2.0)) < 1e-6);
    CHECK(summary.iterations > progress.iterations);
    CHECK(observer.steps == summary.iterations);
    CHECK(observer.events > 2);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_INVALID_ARGUMENT);
    numerix_managed_execution_destroy(execution);
}

static void test_cancel_before_start(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;

    initialize_root_request(&request);
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_request_cancel(execution) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_CANCELLED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_CANCELLED);
    CHECK(summary.status == NUMERIX_STATUS_CANCELLED);
    CHECK(!summary.has_result);
    numerix_managed_execution_destroy(execution);
}

static void test_managed_automatic_root(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;

    initialize_root_request(&request);
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_COMPLETED);
    CHECK(summary.final_algorithm != NUMERIX_ALGORITHM_NONE);
    CHECK(summary.function_evaluations > 0);
    CHECK(fabs(summary.result.value - sqrt(2.0)) < 1e-6);
    numerix_managed_execution_destroy(execution);
}

static void test_limits_and_callback_cancellation(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_options numerical_options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;
    cancelling_input input;

    initialize_root_request(&request);
    numerix_options_init(&numerical_options);
    numerical_options.maximum_function_evaluations = 2;
    request.options = &numerical_options;
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_LIMIT_REACHED);
    CHECK(summary.function_evaluations == 2);
    numerix_managed_execution_destroy(execution);

    initialize_root_request(&request);
    memset(&input, 0, sizeof(input));
    request.function = cancelling_quadratic;
    request.user_data = &input;
    options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_NONE;
    input.cancel_on_call = 1;
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    input.execution = execution;
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_CANCELLED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_CANCELLED);
    CHECK(summary.function_evaluations > 0);
    CHECK(input.calls == 2);
    numerix_managed_execution_destroy(execution);
}

static void test_pause_cancel_and_fallback_control(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_error error;
    cancelling_input input;

    initialize_root_request(&request);
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    options.root_selection.enable_fallback = 1;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    numerix_managed_execution_destroy(execution);
    execution = NULL;

    memset(&input, 0, sizeof(input));
    request.function = cancelling_quadratic;
    request.user_data = &input;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    input.execution = execution;
    input.cancel_on_call = 3;
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_CANCELLED);
    {
        numerix_execution_summary summary;
        CHECK(numerix_managed_execution_get_summary(
                  execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(summary.root_history.attempt_count == 1);
        CHECK(summary.fallback_attempts == 0);
    }
    numerix_managed_execution_destroy(execution);

    initialize_root_request(&request);
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_request_pause(execution) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_request_cancel(execution) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_resume(execution, &error) ==
          NUMERIX_STATUS_CANCELLED);
    numerix_managed_execution_destroy(execution);
}

static void test_total_iteration_limit_stops_fallback(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_options numerical_options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;

    initialize_root_request(&request);
    numerix_options_init(&numerical_options);
    numerical_options.maximum_iterations = 1;
    request.options = &numerical_options;
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;
    options.root_selection.enable_fallback = 1;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_LIMIT_REACHED);
    CHECK(summary.root_history.attempt_count == 1);
    CHECK(summary.fallback_attempts == 0);
    numerix_managed_execution_destroy(execution);

    initialize_root_request(&request);
    numerix_options_init(&numerical_options);
    numerical_options.maximum_execution_time_seconds = 1e-300;
    request.options = &numerical_options;
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_LIMIT_REACHED);
    numerix_managed_execution_destroy(execution);
}

static void test_managed_integration_and_ode(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    const numerix_execution_output *output = NULL;
    numerix_options numerical_options;
    numerix_error error;
    double initial_state[1] = {1.0};

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = square;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 1.0;
    request.problem.integration.panels = 4;
    numerix_options_init(&numerical_options);
    request.options = &numerical_options;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm =
        NUMERIX_ALGORITHM_INTEGRATE_SIMPSON;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.category == NUMERIX_PROBLEM_INTEGRATION);
    CHECK(summary.function_evaluations == 5);
    CHECK(summary.iterations_available);
    CHECK(fabs(summary.result.value - 1.0 / 3.0) < 1e-12);
    CHECK(numerix_managed_execution_get_output(
              execution, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output->algorithm == NUMERIX_ALGORITHM_INTEGRATE_SIMPSON);
    numerix_managed_execution_destroy(execution);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ODE;
    request.problem.ode.function = exponential_ode;
    request.problem.ode.initial_state = initial_state;
    request.problem.ode.dimension = 1;
    request.problem.ode.initial_x = 0.0;
    request.problem.ode.step = 0.1;
    request.problem.ode.steps = 4;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm = NUMERIX_ALGORITHM_ODE_EULER;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.iterations == 4);
    CHECK(summary.function_evaluations == 4);
    CHECK(summary.iterations_available);
    CHECK(numerix_managed_execution_get_output(
              execution, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output->has_ode_result);
    CHECK(fabs(output->ode_result.y[output->ode_result.points - 1] -
               1.4641) < 1e-12);
    numerix_managed_execution_destroy(execution);
}

static void test_managed_linear_solve_output(void)
{
    numerix_matrix *matrix = NULL;
    numerix_vector *right = NULL;
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    const numerix_execution_output *output = NULL;
    numerix_error error;

    CHECK(numerix_matrix_create(2, 2, &matrix, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_vector_create(2, &right, &error) ==
          NUMERIX_STATUS_SUCCESS);
    matrix->data[0] = 3.0;
    matrix->data[1] = 1.0;
    matrix->data[2] = 1.0;
    matrix->data[3] = 2.0;
    right->data[0] = 9.0;
    right->data[1] = 8.0;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_LINEAR_ALGEBRA;
    request.problem.linear.matrix = matrix;
    request.problem.linear.right_hand_side = right;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm = NUMERIX_ALGORITHM_LU;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.has_result);
    CHECK(!summary.iterations_available);
    CHECK(numerix_managed_execution_get_output(
              execution, &output, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output->vector_result != NULL);
    CHECK(fabs(output->vector_result->data[0] - 2.0) < 1e-12);
    CHECK(fabs(output->vector_result->data[1] - 3.0) < 1e-12);

    numerix_managed_execution_destroy(execution);
    numerix_vector_destroy(right);
    numerix_matrix_destroy(matrix);
}

static void test_managed_differentiation_interpolation_optimization(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;
    double x_values[3] = {0.0, 1.0, 2.0};
    double y_values[3] = {0.0, 1.0, 4.0};

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_DIFFERENTIATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = square;
    request.problem.differentiation.x = 2.0;
    request.problem.differentiation.step = 1e-4;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm = NUMERIX_ALGORITHM_DIFF_CENTRAL;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(summary.result.value - 4.0) < 1e-6);
    CHECK(summary.function_evaluations == 3);
    numerix_managed_execution_destroy(execution);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTERPOLATION;
    request.problem.interpolation.x = x_values;
    request.problem.interpolation.y = y_values;
    request.problem.interpolation.count = 3;
    request.problem.interpolation.query = 1.5;
    memset(&options, 0, sizeof(options));
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(summary.result.value - 2.25) < 1e-12);
    CHECK(!summary.iterations_available);
    numerix_managed_execution_destroy(execution);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_OPTIMIZATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = shifted_square;
    request.problem.optimization.bounded = 1;
    request.problem.optimization.lower = 0.0;
    request.problem.optimization.upper = 5.0;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm =
        NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(summary.result.value - 3.0) < 1e-6);
    CHECK(summary.function_evaluations > 0);
    numerix_managed_execution_destroy(execution);

    {
        numerix_vector *initial = NULL;
        const numerix_execution_output *output = NULL;

        CHECK(numerix_vector_create(1, &initial, &error) ==
              NUMERIX_STATUS_SUCCESS);
        initial->data[0] = 0.0;
        memset(&request, 0, sizeof(request));
        request.category = NUMERIX_PROBLEM_OPTIMIZATION;
        request.problem.optimization.objective = vector_objective;
        request.problem.optimization.gradient = vector_gradient;
        request.problem.optimization.initial = initial;
        request.problem.optimization.initial_step = 0.1;
        memset(&options, 0, sizeof(options));
        options.execution.explicit_algorithm =
            NUMERIX_ALGORITHM_OPTIMIZE_GRADIENT_DESCENT;
        CHECK(numerix_managed_execution_create(
                  &request, &options, &execution, &error) ==
              NUMERIX_STATUS_SUCCESS);
        CHECK(numerix_managed_execution_run(execution, &error) ==
              NUMERIX_STATUS_SUCCESS);
        CHECK(numerix_managed_execution_get_summary(
                  execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(summary.derivative_evaluations > 0);
        CHECK(numerix_managed_execution_get_output(
                  execution, &output, &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(output->vector_result != NULL);
        CHECK(fabs(output->vector_result->data[0] - 3.0) < 1e-4);
        numerix_managed_execution_destroy(execution);
        numerix_vector_destroy(initial);
    }
}

static void test_cancellation_during_integration_and_ode(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;
    cancelling_input integration_input;
    cancelling_ode_input ode_input;
    double initial_state[1] = {1.0};

    memset(&request, 0, sizeof(request));
    memset(&integration_input, 0, sizeof(integration_input));
    integration_input.cancel_on_call = 3;
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = cancelling_integrand;
    request.user_data = &integration_input;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 1.0;
    request.problem.integration.panels = 4;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm =
        NUMERIX_ALGORITHM_INTEGRATE_SIMPSON;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    integration_input.execution = execution;
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_CANCELLED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_CANCELLED);
    CHECK(integration_input.calls == 3);
    numerix_managed_execution_destroy(execution);

    memset(&request, 0, sizeof(request));
    memset(&ode_input, 0, sizeof(ode_input));
    request.category = NUMERIX_PROBLEM_ODE;
    request.user_data = &ode_input;
    request.problem.ode.function = cancelling_ode;
    request.problem.ode.initial_state = initial_state;
    request.problem.ode.dimension = 1;
    request.problem.ode.initial_x = 0.0;
    request.problem.ode.step = 0.1;
    request.problem.ode.steps = 4;
    memset(&options, 0, sizeof(options));
    options.execution.explicit_algorithm = NUMERIX_ALGORITHM_ODE_EULER;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    ode_input.execution = execution;
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_CANCELLED);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_CANCELLED);
    CHECK(ode_input.calls == 1);
    numerix_managed_execution_destroy(execution);
}

static void test_managed_piecewise_routes(void)
{
    numerix_piecewise_function *piecewise = NULL;
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;

    CHECK(numerix_piecewise_create(&piecewise, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              piecewise, 0.0, 2.0, 1, 1, "x - 1", &error) ==
          NUMERIX_STATUS_SUCCESS);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;
    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(summary.result.value - 1.0) < 1e-7);
    numerix_managed_execution_destroy(execution);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 2.0;
    request.problem.integration.panels = 4;
    memset(&options, 0, sizeof(options));
    CHECK(numerix_managed_execution_create(
              &request, &options, &execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution, &summary, &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(fabs(summary.result.value) < 1e-12);
    CHECK(summary.function_evaluations > 0);
    numerix_managed_execution_destroy(execution);
    numerix_piecewise_destroy(piecewise);
}

int main(void)
{
    test_bisection_pause_resume();
    test_cancel_before_start();
    test_managed_automatic_root();
    test_limits_and_callback_cancellation();
    test_pause_cancel_and_fallback_control();
    test_total_iteration_limit_stops_fallback();
    test_managed_integration_and_ode();
    test_managed_linear_solve_output();
    test_managed_differentiation_interpolation_optimization();
    test_cancellation_during_integration_and_ode();
    test_managed_piecewise_routes();

    if (failures != 0)
    {
        fprintf(stderr, "%d execution checks failed.\n", failures);
        return 1;
    }
    puts("All managed execution checks passed.");
    return 0;
}
