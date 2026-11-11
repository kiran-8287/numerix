#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double quadratic(double x, void *data)
{
    (void)data;
    return x * x - 2.0;
}

static double quadratic_derivative(double x, void *data)
{
    (void)data;
    return 2.0 * x;
}

static double flat_start_function(double x, void *data)
{
    (void)data;
    return (x - 1.0) * (x - 1.0) * (x - 1.0) + 0.5;
}

static double flat_start_derivative(double x, void *data)
{
    (void)data;
    return 3.0 * (x - 1.0) * (x - 1.0);
}

static void print_recommendation(const numerix_recommendation *recommendation)
{
    printf("Recommended: %s\n",
           numerix_algorithm_name(recommendation->recommended_algorithm));
    printf("Reason: %s\n", recommendation->reason[0]);
    printf("Limitation: %s\n", recommendation->limitation[0]);
}

int main(void)
{
    numerix_recommendation_request request;
    numerix_recommendation recommendation;
    numerix_root_selection_options selection;
    numerix_root_selection_report report;
    numerix_root_portfolio_report portfolio;
    numerix_root_portfolio_options portfolio_options;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = quadratic;
    request.derivative = quadratic_derivative;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.initial = 1.0;
    request.problem.root.has_bracket = 1;
    request.problem.root.has_initial = 1;

    status = numerix_recommend_algorithm(&request, &recommendation, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "recommendation failed: %s\n", error.message);
        return 1;
    }
    print_recommendation(&recommendation);

    status = numerix_root_solve_auto(
        &request, NULL, &result, &report, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "automatic solve failed: %s\n", error.message);
        return 1;
    }
    printf("Automatic root: %.12f using %s\n",
           result.value, numerix_algorithm_name(report.final_algorithm));

    portfolio_options.algorithm_mask = 0U;
    status = numerix_root_solve_portfolio(
        &request, &portfolio_options, &result, &portfolio, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        printf("Portfolio compared %zu sequential methods; selected %s\n",
               portfolio.attempt_count,
               numerix_algorithm_name(
                   portfolio.attempts[portfolio.selected_attempt].algorithm));
    }

    request.function = flat_start_function;
    request.derivative = flat_start_derivative;
    selection.mode = NUMERIX_SELECTION_EXPLICIT;
    selection.explicit_algorithm = NUMERIX_ALGORITHM_NEWTON;
    selection.enable_fallback = 1;
    status = numerix_root_solve_auto(
        &request, &selection, &result, &report, &error);
    if (status == NUMERIX_STATUS_SUCCESS)
    {
        printf("Fallback used after %s; final method %s found %.12f\n",
               numerix_algorithm_name(report.initially_selected),
               numerix_algorithm_name(report.final_algorithm), result.value);
    }
    return 0;
}
