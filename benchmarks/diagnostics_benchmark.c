#include "numerix.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define BENCHMARK_REPETITIONS 10000U

static double root_function(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
}

static double scalar_function(double x, void *user_data)
{
    (void)user_data;
    return x * x + 1.0;
}

static numerix_status ode_function(double x, const double *y,
                                   size_t dimension, double *derivative,
                                   void *user_data)
{
    size_t index;

    (void)x;
    (void)user_data;
    for (index = 0; index < dimension; index++)
    {
        derivative[index] = y[index];
    }
    return NUMERIX_STATUS_SUCCESS;
}

static int run_repetitions(
    const char *case_name,
    const numerix_recommendation_request *request,
    const numerix_execution_options *execution_options,
    const numerix_diagnostics_options *diagnostic_options,
    int diagnostics_enabled,
    double *milliseconds_per_run)
{
    clock_t start;
    size_t repetition;

    start = clock();
    for (repetition = 0; repetition < BENCHMARK_REPETITIONS; repetition++)
    {
        numerix_execution_output output;
        numerix_error error;
        numerix_status status;

        status = numerix_execute_recommended(
            request, execution_options, &output, &error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            fprintf(stderr, "%s execution failed: %s\n", case_name,
                    error.message);
            return 0;
        }

        if (diagnostics_enabled)
        {
            numerix_diagnostics_report report;

            status = numerix_diagnostics_analyze(
                request, &output, diagnostic_options, &report, &error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                fprintf(stderr, "%s diagnostics failed: %s\n", case_name,
                        error.message);
                numerix_execution_output_destroy(&output);
                return 0;
            }
            numerix_diagnostics_report_destroy(&report);
        }

        numerix_execution_output_destroy(&output);
    }

    *milliseconds_per_run =
        1000.0 * (double)(clock() - start) /
        (double)CLOCKS_PER_SEC / (double)BENCHMARK_REPETITIONS;
    return 1;
}

static int benchmark_case(
    const char *name,
    const numerix_recommendation_request *request,
    const numerix_execution_options *execution_options,
    int condition_option,
    int root_condition_option)
{
    numerix_diagnostics_options basic_options = {0, 0};
    numerix_diagnostics_options full_options = {
        condition_option, root_condition_option
    };
    double disabled_time;
    double basic_time;
    double full_time;
    size_t additional_function_evaluations = 0;
    size_t additional_derivative_evaluations = 0;

    if (!run_repetitions(name, request, execution_options, &basic_options,
                         0, &disabled_time) ||
        !run_repetitions(name, request, execution_options, &basic_options,
                         1, &basic_time) ||
        !run_repetitions(name, request, execution_options, &full_options,
                         1, &full_time))
    {
        return 0;
    }

    {
        numerix_execution_output output;
        numerix_diagnostics_report report;
        numerix_error error;
        numerix_status status = numerix_execute_recommended(
            request, execution_options, &output, &error);

        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return 0;
        }
        status = numerix_diagnostics_analyze(
            request, &output, &full_options, &report, &error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            numerix_execution_output_destroy(&output);
            return 0;
        }
        additional_function_evaluations =
            report.additional_function_evaluations;
        additional_derivative_evaluations =
            report.additional_derivative_evaluations;
        numerix_diagnostics_report_destroy(&report);
        numerix_execution_output_destroy(&output);
    }

    printf("%-16s disabled=%9.5f ms basic=%9.5f ms full=%9.5f ms extra f/d=%lu/%lu\n",
           name, disabled_time, basic_time, full_time,
           (unsigned long)additional_function_evaluations,
           (unsigned long)additional_derivative_evaluations);
    return 1;
}

static int benchmark_linear_size(size_t dimension)
{
    numerix_options options;
    numerix_execution_options execution_options;
    numerix_recommendation_request request;
    numerix_matrix *matrix = NULL;
    numerix_vector *right_hand_side = NULL;
    numerix_error error;
    size_t row;
    char case_name[40];
    int passed;

    if (numerix_matrix_create(dimension, dimension, &matrix, &error) !=
            NUMERIX_STATUS_SUCCESS ||
        numerix_vector_create(dimension, &right_hand_side, &error) !=
            NUMERIX_STATUS_SUCCESS)
    {
        numerix_matrix_destroy(matrix);
        numerix_vector_destroy(right_hand_side);
        fprintf(stderr, "linear benchmark setup failed: %s\n", error.message);
        return 0;
    }

    for (row = 0; row < dimension; row++)
    {
        matrix->data[row * dimension + row] = 1.0 + (double)row;
        right_hand_side->data[row] = 1.0;
    }

    numerix_options_init(&options);
    memset(&execution_options, 0, sizeof(execution_options));
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_LINEAR_ALGEBRA;
    request.options = &options;
    request.problem.linear.matrix = matrix;
    request.problem.linear.right_hand_side = right_hand_side;
    (void)snprintf(case_name, sizeof(case_name), "linear %ux%u",
                   (unsigned int)dimension, (unsigned int)dimension);
    passed = benchmark_case(case_name, &request, &execution_options, 1, 0);

    numerix_vector_destroy(right_hand_side);
    numerix_matrix_destroy(matrix);
    return passed;
}

int main(void)
{
    numerix_options options;
    numerix_execution_options execution_options;
    numerix_recommendation_request request;
    double initial_state[] = {1.0};
    int passed = 1;

    numerix_options_init(&options);
    memset(&execution_options, 0, sizeof(execution_options));
    memset(&request, 0, sizeof(request));
    request.options = &options;
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = root_function;
    request.problem.root.lower = 1.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;
    passed &= benchmark_case("root finding", &request, &execution_options,
                             0, 1);

    passed &= benchmark_linear_size(8);
    passed &= benchmark_linear_size(32);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_INTEGRATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = scalar_function;
    request.options = &options;
    request.problem.integration.lower = 0.0;
    request.problem.integration.upper = 1.0;
    memset(&execution_options, 0, sizeof(execution_options));
    execution_options.explicit_algorithm = NUMERIX_ALGORITHM_INTEGRATE_SIMPSON;
    execution_options.panels = 100;
    passed &= benchmark_case("integration", &request, &execution_options,
                             0, 0);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_DIFFERENTIATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = scalar_function;
    request.options = &options;
    request.problem.differentiation.x = 1.0;
    request.problem.differentiation.step = 1.0e-3;
    memset(&execution_options, 0, sizeof(execution_options));
    execution_options.explicit_algorithm = NUMERIX_ALGORITHM_DIFF_RICHARDSON;
    passed &= benchmark_case("differentiation", &request,
                             &execution_options, 0, 0);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_OPTIMIZATION;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = scalar_function;
    request.options = &options;
    request.problem.optimization.bounded = 1;
    request.problem.optimization.lower = -1.0;
    request.problem.optimization.upper = 1.0;
    memset(&execution_options, 0, sizeof(execution_options));
    execution_options.explicit_algorithm =
        NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION;
    passed &= benchmark_case("optimization", &request,
                             &execution_options, 0, 0);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ODE;
    request.options = &options;
    request.problem.ode.function = ode_function;
    request.problem.ode.initial_state = initial_state;
    request.problem.ode.dimension = 1;
    request.problem.ode.initial_x = 0.0;
    request.problem.ode.step = 0.01;
    request.problem.ode.steps = 100;
    memset(&execution_options, 0, sizeof(execution_options));
    execution_options.explicit_algorithm = NUMERIX_ALGORITHM_ODE_RK4;
    passed &= benchmark_case("ODE RK4", &request, &execution_options, 0, 0);

    if (!passed)
    {
        return 1;
    }

    printf("Method: %u repetitions per mode; process CPU time from clock().\n",
           BENCHMARK_REPETITIONS);
    printf("Basic diagnostics are post-run; full mode enables condition estimates where configured.\n");
    return 0;
}
