#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* f(x) = x^2 - 2, safeguarded-Newton friendly: bracket [0,2], derivative available. */
static double f(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
}

static double df(double x, void *user_data)
{
    (void)user_data;
    return 2.0 * x;
}

static void print_recommendation(const numerix_recommendation *rec)
{
    printf("Recommended algorithm: %s\n",
           numerix_algorithm_name(rec->recommended_algorithm));
    printf("Suitability:           %d\n", (int)rec->suitability);
    printf("Reason:                %s\n", rec->reason[0]);
    if (rec->limitation[0][0] != '\0')
        printf("Limitation:            %s\n", rec->limitation[0]);
    printf("Sign-changing bracket: %s\n",
           rec->sign_changing_bracket ? "yes" : "no");
    printf("Derivative available:  %s\n",
           rec->derivative_available ? "yes" : "no");
}

int main(void)
{
    numerix_recommendation_request request;
    numerix_recommendation recommendation;
    numerix_root_selection_report report;
    numerix_result result;
    numerix_error error;
    numerix_status status;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = f;
    request.derivative = df;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.initial = 1.0;
    request.problem.root.has_bracket = 1;
    request.problem.root.has_initial = 1;

    /* Recommendation: which algorithm does NUMERIX suggest? */
    status = numerix_recommend_algorithm(
        &request, &recommendation, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Recommendation failed: %s\n", error.message);
        return 1;
    }
    print_recommendation(&recommendation);

    /* Automatic execution: let NUMERIX pick and run the algorithm. */
    status = numerix_root_solve_auto(
        &request, NULL, &result, &report, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "Automatic solve failed: %s\n", error.message);
        return 1;
    }

    printf("\nAutomatic execution result:\n");
    printf("  Initially selected: %s\n",
           numerix_algorithm_name(report.initially_selected));
    printf("  Final algorithm:    %s\n",
           numerix_algorithm_name(report.final_algorithm));
    printf("  Fallback used:      %s\n",
           report.fallback_used ? "yes" : "no");
    printf("  Root:               %.12f\n", result.value);
    printf("  Expected:           %.12f\n", sqrt(2.0));
    printf("  Residual:           %.3e\n", result.residual);
    printf("  Iterations:         %zu\n", result.iterations);

    return fabs(result.value - sqrt(2.0)) < 1e-6 ? 0 : 1;
}
