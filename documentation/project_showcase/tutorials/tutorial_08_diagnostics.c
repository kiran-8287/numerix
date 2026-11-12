#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double f(double x, void *user_data)
{
    (void)user_data;
    return x * x - 2.0;
}

int main(void)
{
    numerix_recommendation_request request;
    numerix_execution_options execution_options;
    numerix_diagnostics_options diagnostics_options;
    numerix_diagnostics_report report;
    numerix_error error;
    numerix_status status;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = f;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;

    memset(&execution_options, 0, sizeof(execution_options));
    execution_options.explicit_algorithm = NUMERIX_ALGORITHM_BISECTION;

    /* Execute the solve through the recommended execution path.
       output is stack-allocated; the function fills it in. */
    numerix_execution_output output;
    memset(&output, 0, sizeof(output));

    status = numerix_execute_recommended(
        &request, &execution_options, &output, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "execute_recommended failed: %s\n", error.message);
        return 1;
    }

    printf("Algorithm: %s\n", numerix_algorithm_name(output.algorithm));
    printf("Root:      %.12f\n", output.scalar_result.value);
    printf("Residual:  %.3e\n", output.scalar_result.residual);

    /* Diagnostics: analyze the existing execution without changing it. */
    memset(&diagnostics_options, 0, sizeof(diagnostics_options));
    diagnostics_options.estimate_root_condition = 1;

    status = numerix_diagnostics_analyze(
        &request, &output, &diagnostics_options, &report, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "diagnostics_analyze failed: %s\n", error.message);
        numerix_execution_output_destroy(&output);
        return 1;
    }

    printf("\nDiagnostics:\n");
    printf("  Residual:           %.3e\n", report.absolute_residual);
    printf("  Estimated error:    %.3e\n", report.estimated_absolute_error);
    printf("  Reliability:        %d\n", (int)report.reliability);
    printf("  Condition estimate: %s %.3e\n",
           report.has_condition_estimate ? "available" : "unavailable",
           report.has_condition_estimate ? report.condition_estimate : 0.0);

    if (report.has_residual_tolerance)
        printf("  Residual tolerance: %s (requested %.3e)\n",
               report.residual_satisfies_tolerance ? "satisfied" : "not satisfied",
               report.requested_residual_tolerance);

    numerix_diagnostics_report_destroy(&report);
    numerix_execution_output_destroy(&output);

    return 0;
}
