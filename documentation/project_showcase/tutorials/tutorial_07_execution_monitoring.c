#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double square_minus_two(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
}

static void print_step(numerix_managed_execution *execution,
                       const numerix_execution_step *step,
                       void *user_data)
{
    (void)execution;
    (void)user_data;
    printf("step %zu: interval=[%.8f, %.8f], midpoint=%.8f, f=%.3e\n",
           step->number, step->interval_lower, step->interval_upper,
           step->midpoint, step->residual);
}

int main(void)
{
    numerix_recommendation_request request;
    numerix_managed_execution_options options;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_error error;
    numerix_status status;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = square_minus_two;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;

    memset(&options, 0, sizeof(options));
    options.root_selection.mode = NUMERIX_SELECTION_EXPLICIT;
    options.root_selection.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;
    options.step_callback = print_step;

    status = numerix_managed_execution_create(
        &request, &options, &execution, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "create failed: %s\n", error.message);
        return 1;
    }

    /* Run once to completion (Bisection step callback fires at each step). */
    status = numerix_managed_execution_run(execution, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "run failed: %s\n", error.message);
        numerix_managed_execution_destroy(execution);
        return 1;
    }

    status = numerix_managed_execution_get_summary(
        execution, &summary, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "summary failed: %s\n", error.message);
        numerix_managed_execution_destroy(execution);
        return 1;
    }

    printf("root: %.12f\n", summary.result.value);
    if (isfinite(summary.result.residual))
        printf("residual: %.3e\n", summary.result.residual);
    printf("iterations: %zu\n", summary.iterations);
    printf("function evaluations: %zu\n", summary.function_evaluations);

    numerix_managed_execution_destroy(execution);

    return fabs(summary.result.value - sqrt(2.0)) < 1e-6 ? 0 : 1;
}
