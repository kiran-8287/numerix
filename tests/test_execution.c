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

static double quadratic(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
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
    CHECK(input.calls == 1);
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
}

int main(void)
{
    test_bisection_pause_resume();
    test_cancel_before_start();
    test_managed_automatic_root();
    test_limits_and_callback_cancellation();
    test_pause_cancel_and_fallback_control();
    test_total_iteration_limit_stops_fallback();

    if (failures != 0)
    {
        fprintf(stderr, "%d execution checks failed.\n", failures);
        return 1;
    }
    puts("All managed execution checks passed.");
    return 0;
}
