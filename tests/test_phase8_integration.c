#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static size_t failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void test_expression_discovery_execution_and_diagnostics(void)
{
    numerix_expression *expression = NULL;
    numerix_recommendation_request request;
    numerix_root_discovery_options discovery_options;
    numerix_root_search_report discovery_report;
    numerix_managed_execution *execution = NULL;
    numerix_execution_summary summary;
    numerix_diagnostics_report diagnostics;
    const numerix_execution_output *output = NULL;
    numerix_error error;

    CHECK(numerix_expression_compile("x^2-2", &expression, &error) ==
          NUMERIX_STATUS_SUCCESS);
    if (expression == NULL) {
        return;
    }

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_EXPRESSION;
    request.expression = expression;
    request.problem.root.lower = 0.0;
    request.problem.root.upper = 2.0;
    request.problem.root.has_bracket = 1;

    numerix_root_discovery_options_init(&discovery_options);
    CHECK(numerix_root_find_all(
              &request,
              request.problem.root.lower,
              request.problem.root.upper,
              &discovery_options,
              &discovery_report,
              &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(discovery_report.root_count == 1U);
    if (discovery_report.root_count == 1U) {
        CHECK(fabs(discovery_report.roots[0].result.value - sqrt(2.0)) <
              1.0e-8);
    }
    CHECK(discovery_report.result_storage_bytes > 0U);
    numerix_root_search_report_destroy(&discovery_report);

    CHECK(numerix_managed_execution_create(
              &request,
              NULL,
              &execution,
              &error) == NUMERIX_STATUS_SUCCESS);
    if (execution == NULL) {
        numerix_expression_destroy(expression);
        return;
    }
    CHECK(numerix_managed_execution_run(execution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_managed_execution_get_summary(
              execution,
              &summary,
              &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(summary.state == NUMERIX_EXECUTION_COMPLETED);
    CHECK(summary.has_result);
    CHECK(numerix_managed_execution_get_output(
              execution,
              &output,
              &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(output != NULL);
    if (output != NULL) {
        CHECK(output->selected_automatically);
        CHECK(fabs(output->scalar_result.value - sqrt(2.0)) < 1.0e-8);
        CHECK(numerix_diagnostics_analyze(
                  &request,
                  output,
                  NULL,
                  &diagnostics,
                  &error) == NUMERIX_STATUS_SUCCESS);
        CHECK(diagnostics.has_residual);
        CHECK(diagnostics.absolute_residual < 1.0e-8);
        numerix_diagnostics_report_destroy(&diagnostics);
    }

    numerix_managed_execution_destroy(execution);
    numerix_expression_destroy(expression);
}

static void test_piecewise_boundary_discovery_and_analysis(void)
{
    numerix_piecewise_function *piecewise = NULL;
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report discovery_report;
    numerix_piecewise_boundary_report boundary_report;
    numerix_error error;

    CHECK(numerix_piecewise_create(&piecewise, &error) ==
          NUMERIX_STATUS_SUCCESS);
    if (piecewise == NULL) {
        return;
    }
    CHECK(numerix_piecewise_add_expression(
              piecewise, -2.0, 0.0, 1, 0, "x", &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              piecewise, 0.0, 2.0, 1, 1, "x", &error) ==
          NUMERIX_STATUS_SUCCESS);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request,
              -2.0,
              2.0,
              &options,
              &discovery_report,
              &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(discovery_report.root_count == 1U);
    CHECK(discovery_report.known_boundaries == 1U);
    if (discovery_report.root_count == 1U) {
        CHECK(discovery_report.roots[0].kind ==
              NUMERIX_ROOT_PIECEWISE_BOUNDARY);
    }
    numerix_root_search_report_destroy(&discovery_report);

    CHECK(numerix_piecewise_analyze_boundary(
              piecewise,
              0.0,
              NULL,
              &boundary_report,
              &error) == NUMERIX_STATUS_SUCCESS);
    CHECK(boundary_report.type == NUMERIX_DISCONTINUITY_CONTINUOUS);
    CHECK(fabs(boundary_report.left_limit) < 1.0e-6);
    CHECK(fabs(boundary_report.right_limit) < 1.0e-6);
    numerix_piecewise_destroy(piecewise);
}

int main(void)
{
    test_expression_discovery_execution_and_diagnostics();
    test_piecewise_boundary_discovery_and_analysis();

    if (failures > 0U) {
        fprintf(stderr, "%zu Phase 8 integration checks failed\n", failures);
        return 1;
    }

    puts("All Phase 8 integration checks passed.");
    return 0;
}
