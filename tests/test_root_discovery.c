#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

typedef enum test_function_kind {
    TEST_TWO_ROOTS = 0,
    TEST_THREE_ROOTS,
    TEST_DOUBLE_ROOT,
    TEST_TRIPLE_ROOT,
    TEST_QUADRUPLE_ROOT,
    TEST_POSITIVE_STATIONARY,
    TEST_CLOSE_ROOTS,
    TEST_NO_ROOTS,
    TEST_ENDPOINT_LOWER,
    TEST_ENDPOINT_BOTH,
    TEST_INTERIOR_AND_ENDPOINT,
    TEST_IDENTICALLY_ZERO
} test_function_kind;

typedef struct test_function {
    test_function_kind kind;
} test_function;

typedef struct progress_state {
    size_t calls;
    int cancel;
} progress_state;

static double evaluate_test_function(double x, void *user_data)
{
    const test_function *function = user_data;

    switch (function->kind) {
    case TEST_TWO_ROOTS:
        return (x - 1.0) * (x - 2.0);
    case TEST_THREE_ROOTS:
        return (x - 1.0) * (x - 2.0) * (x - 4.0);
    case TEST_DOUBLE_ROOT:
        return (x - 1.0) * (x - 1.0);
    case TEST_TRIPLE_ROOT:
        return (x - 1.0) * (x - 1.0) * (x - 1.0);
    case TEST_QUADRUPLE_ROOT:
        return (x - 1.0) * (x - 1.0) * (x - 1.0) * (x - 1.0);
    case TEST_POSITIVE_STATIONARY:
        return (x - 1.0) * (x - 1.0) + 0.01;
    case TEST_CLOSE_ROOTS:
        return (x - 1.0) * (x - 1.001);
    case TEST_NO_ROOTS:
        return x * x + 1.0;
    case TEST_ENDPOINT_LOWER:
        return x * x - 1.0;
    case TEST_ENDPOINT_BOTH:
        return x * (x - 1.0);
    case TEST_INTERIOR_AND_ENDPOINT:
        return x * (x - 0.5);
    case TEST_IDENTICALLY_ZERO:
        return 0.0;
    }

    return NAN;
}

static double pole_function(double x, void *user_data)
{
    (void)user_data;
    return 1.0 / (x - 1.0);
}

static double linear_root_function(double x, void *user_data)
{
    (void)user_data;
    return x - 1.0;
}

static double linear_root_derivative(double x, void *user_data)
{
    (void)x;
    (void)user_data;
    return 1.0;
}

static double slow_linear_root(double x, void *user_data)
{
    clock_t started = clock();
    double elapsed = 0.0;

    (void)user_data;
    while (started != (clock_t)-1 && elapsed < 0.02) {
        elapsed = (double)(clock() - started) / (double)CLOCKS_PER_SEC;
    }
    return x - 1.0;
}

static numerix_status progress_callback(
    const numerix_root_search_report *report,
    void *user_data)
{
    progress_state *state = user_data;

    (void)report;
    ++state->calls;
    if (state->cancel && state->calls >= 2U) {
        return NUMERIX_STATUS_CANCELLED;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static void initialize_request(
    numerix_recommendation_request *request,
    test_function *function,
    const numerix_options *options)
{
    memset(request, 0, sizeof(*request));
    request->category = NUMERIX_PROBLEM_ROOT_FINDING;
    request->representation = NUMERIX_REPRESENTATION_CALLBACK;
    request->function = evaluate_test_function;
    request->user_data = function;
    request->options = options;
    request->problem.root.has_bracket = 1;
}

static int contains_root(
    const numerix_root_search_report *report,
    double expected,
    double tolerance)
{
    size_t index;

    for (index = 0; index < report->root_count; ++index) {
        if (fabs(report->roots[index].result.value - expected) <= tolerance) {
            return 1;
        }
    }
    return 0;
}

static void run_function_case(
    test_function_kind kind,
    double lower,
    double upper,
    size_t expected_roots,
    const double *expected_values)
{
    test_function function = {kind};
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;
    size_t index;
    numerix_status status;

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    status = numerix_root_find_all(
        &request,
        lower,
        upper,
        &options,
        &report,
        &error);
    CHECK(status == NUMERIX_STATUS_SUCCESS);
    CHECK(report.completed_normally);
    CHECK(report.search_inconclusive);
    CHECK(report.root_count == expected_roots);
    for (index = 0; index < expected_roots; ++index) {
        CHECK(contains_root(&report, expected_values[index], 1.0e-5));
    }
    numerix_root_search_report_destroy(&report);
}

static void test_multiple_roots_and_multiplicity(void)
{
    const double two_expected[] = {1.0, 2.0};
    const double three_expected[] = {1.0, 2.0, 4.0};
    const double one_expected[] = {1.0};
    test_function function = {TEST_DOUBLE_ROOT};
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    run_function_case(TEST_TWO_ROOTS, 0.0, 3.0, 2U, two_expected);
    run_function_case(TEST_THREE_ROOTS, 0.0, 5.0, 3U, three_expected);
    run_function_case(TEST_DOUBLE_ROOT, 0.0, 2.0, 1U, one_expected);
    run_function_case(TEST_TRIPLE_ROOT, 0.0, 2.0, 1U, one_expected);
    run_function_case(TEST_QUADRUPLE_ROOT, 0.0, 2.0, 1U, one_expected);

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    if (report.root_count == 1U) {
        CHECK(report.roots[0].has_multiplicity_estimate);
        CHECK(report.roots[0].estimated_multiplicity == 2U);
        CHECK(report.roots[0].multiplicity_confidence ==
              NUMERIX_DIAGNOSTIC_SUSPECTED);
    }
    numerix_root_search_report_destroy(&report);

    function.kind = TEST_TRIPLE_ROOT;
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    if (report.root_count == 1U) {
        CHECK(report.roots[0].has_multiplicity_estimate);
        CHECK(report.roots[0].estimated_multiplicity == 3U);
    }
    numerix_root_search_report_destroy(&report);

    function.kind = TEST_QUADRUPLE_ROOT;
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    if (report.root_count == 1U) {
        CHECK(report.roots[0].has_multiplicity_estimate);
        CHECK(report.roots[0].estimated_multiplicity == 4U);
    }
    numerix_root_search_report_destroy(&report);
}

static void test_endpoints_close_roots_and_no_roots(void)
{
    const double lower_endpoint[] = {1.0};
    const double both_endpoints[] = {0.0, 1.0};
    const double interior_and_endpoint[] = {0.0, 0.5};
    const double close_expected[] = {1.0, 1.001};
    const double no_expected[] = {0.0};
    test_function function = {TEST_CLOSE_ROOTS};
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    run_function_case(
        TEST_ENDPOINT_LOWER,
        1.0,
        3.0,
        1U,
        lower_endpoint);
    run_function_case(
        TEST_ENDPOINT_LOWER,
        -3.0,
        -1.0,
        1U,
        (const double[]){-1.0});
    run_function_case(
        TEST_ENDPOINT_BOTH,
        0.0,
        1.0,
        2U,
        both_endpoints);
    run_function_case(
        TEST_INTERIOR_AND_ENDPOINT,
        0.0,
        1.0,
        2U,
        interior_and_endpoint);
    run_function_case(
        TEST_NO_ROOTS,
        -2.0,
        2.0,
        0U,
        no_expected);

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    options.initial_panels = 31U;
    options.merge_absolute_tolerance = 1.0e-6;
    options.merge_relative_tolerance = 1.0e-10;
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 2U);
    CHECK(contains_root(&report, close_expected[0], 2.0e-5));
    CHECK(contains_root(&report, close_expected[1], 2.0e-5));
    numerix_root_search_report_destroy(&report);

    options.merge_absolute_tolerance = 0.01;
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    CHECK(report.merged_candidates > 0U);
    if (report.root_count == 1U) {
        CHECK(report.roots[0].verification ==
              NUMERIX_ROOT_ACCEPTED_WITH_LIMITATIONS);
    }
    numerix_root_search_report_destroy(&report);

    function.kind = TEST_IDENTICALLY_ZERO;
    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, -1.0, 1.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 0U);
    CHECK(report.unresolved_count > 0U);
    CHECK(report.roots == NULL);
    CHECK(report.root_storage_capacity == 0U);
    CHECK(report.root_storage_allocation_count > 0U);
    CHECK(report.unresolved_storage_capacity > 0U);
    numerix_root_search_report_destroy(&report);
}

static void test_stationary_false_positive_and_pole(void)
{
    test_function stationary = {TEST_POSITIVE_STATIONARY};
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    initialize_request(&request, &stationary, NULL);
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 0U);
    CHECK(report.rejected_candidates > 0U);
    numerix_root_search_report_destroy(&report);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = pole_function;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 0U);
    CHECK(report.evaluation_failures > 0U);
    CHECK(report.suspected_discontinuities > 0U);
    numerix_root_search_report_destroy(&report);
}

static void test_expression_and_piecewise_representations(void)
{
    numerix_expression *expression = NULL;
    numerix_piecewise_function *piecewise = NULL;
    numerix_piecewise_function *boundary_root = NULL;
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    CHECK(numerix_expression_compile("x^2-2", &expression, &error) ==
          NUMERIX_STATUS_SUCCESS);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_EXPRESSION;
    request.expression = expression;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    CHECK(contains_root(&report, sqrt(2.0), 1.0e-6));
    numerix_root_search_report_destroy(&report);
    numerix_expression_destroy(expression);

    CHECK(numerix_piecewise_create(&piecewise, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              piecewise, -2.0, 0.0, 1, 0, "x+0.001", &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              piecewise, 0.0, 2.0, 1, 1, "x-1", &error) ==
          NUMERIX_STATUS_SUCCESS);
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, -2.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 2U);
    CHECK(contains_root(&report, -0.001, 1.0e-6));
    CHECK(contains_root(&report, 1.0, 1.0e-6));
    CHECK(report.known_boundaries == 1U);
    numerix_root_search_report_destroy(&report);
    numerix_piecewise_destroy(piecewise);

    CHECK(numerix_piecewise_create(&boundary_root, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              boundary_root, -2.0, 0.0, 1, 0, "x", &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              boundary_root, 0.0, 2.0, 1, 1, "x", &error) ==
          NUMERIX_STATUS_SUCCESS);
    request.piecewise_function = boundary_root;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, -2.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    if (report.root_count == 1U) {
        CHECK(report.roots[0].kind == NUMERIX_ROOT_PIECEWISE_BOUNDARY);
        CHECK(report.roots[0].result.value == 0.0);
    }
    numerix_root_search_report_destroy(&report);
    numerix_piecewise_destroy(boundary_root);

    CHECK(numerix_piecewise_create(&piecewise, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              piecewise, 0.0, 1.0, 1, 0, "x-1", &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(numerix_piecewise_add_expression(
              piecewise, 1.0, 2.0, 0, 1, "x-1", &error) ==
          NUMERIX_STATUS_SUCCESS);
    request.piecewise_function = piecewise;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 0U);
    CHECK(report.suspected_discontinuities > 0U);
    numerix_root_search_report_destroy(&report);
    numerix_piecewise_destroy(piecewise);
}

static void test_resource_limits_and_cancellation(void)
{
    test_function function = {TEST_TWO_ROOTS};
    numerix_options execution_options;
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;
    progress_state progress;

    numerix_options_init(&execution_options);
    execution_options.maximum_function_evaluations = 10U;
    initialize_request(&request, &function, &execution_options);
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 3.0, &options, &report, &error) ==
          NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(report.resource_limit_reached);
    CHECK(report.function_evaluations <= 10U);
    numerix_root_search_report_destroy(&report);

    {
        test_function repeated = {TEST_DOUBLE_ROOT};

        initialize_request(&request, &repeated, NULL);
        numerix_root_discovery_options_init(&options);
        options.initial_panels = 1U;
        options.maximum_subdivisions = 1U;
        options.curvature_threshold = 0.1;
        CHECK(numerix_root_find_all(
                  &request, 0.5, 1.5, &options, &report, &error) ==
              NUMERIX_STATUS_LIMIT_REACHED);
        CHECK(report.resource_limit_reached);
        CHECK(report.subdivisions == 1U);
        numerix_root_search_report_destroy(&report);
    }

    numerix_options_init(&execution_options);
    execution_options.maximum_execution_time_seconds = 0.001;
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = slow_linear_root;
    request.options = &execution_options;
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(report.resource_limit_reached);
    CHECK(report.function_evaluations == 1U);
    numerix_root_search_report_destroy(&report);

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    options.maximum_candidates = 1U;
    CHECK(numerix_root_find_all(
              &request, 0.0, 3.0, &options, &report, &error) ==
          NUMERIX_STATUS_LIMIT_REACHED);
    CHECK(report.resource_limit_reached);
    CHECK(report.root_count <= 1U);
    numerix_root_search_report_destroy(&report);

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    memset(&progress, 0, sizeof(progress));
    progress.cancel = 1;
    options.progress_interval = 1U;
    options.progress_callback = progress_callback;
    options.progress_user_data = &progress;
    CHECK(numerix_root_find_all(
              &request, 0.0, 3.0, &options, &report, &error) ==
          NUMERIX_STATUS_CANCELLED);
    CHECK(!report.completed_normally);
    CHECK(progress.calls >= 2U);
    numerix_root_search_report_destroy(&report);
}

static void test_optional_root_conditioning(void)
{
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = linear_root_function;
    request.derivative = linear_root_derivative;
    numerix_root_discovery_options_init(&options);
    options.estimate_condition = 1;
    CHECK(numerix_root_find_all(
              &request, 0.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 1U);
    if (report.root_count == 1U) {
        CHECK(report.roots[0].conditioning.derivative_was_supplied);
        CHECK(report.roots[0].conditioning.condition_available);
        CHECK(report.roots[0].diagnostics.has_condition_estimate);
    }
    numerix_root_search_report_destroy(&report);
}

static void test_result_storage_grows_with_results(void)
{
    test_function function = {TEST_NO_ROOTS};
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, -2.0, 2.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 0U);
    CHECK(report.stored_unresolved_count == 0U);
    CHECK(report.root_storage_capacity == 0U);
    CHECK(report.unresolved_storage_capacity == 0U);
    CHECK(report.result_storage_bytes == 0U);
    CHECK(report.roots == NULL);
    CHECK(report.unresolved == NULL);
    CHECK(report.workspace_peak_bytes > 0U);
    CHECK(report.workspace_allocation_count == 2U);
    numerix_root_search_report_destroy(&report);

    function.kind = TEST_TWO_ROOTS;
    initialize_request(&request, &function, NULL);
    CHECK(numerix_root_find_all(
              &request, 0.0, 3.0, &options, &report, &error) ==
          NUMERIX_STATUS_SUCCESS);
    CHECK(report.root_count == 2U);
    CHECK(report.root_storage_capacity == 4U);
    CHECK(report.root_storage_allocation_count == 1U);
    CHECK(report.unresolved_storage_capacity <= options.maximum_candidates);
    CHECK(report.result_storage_bytes ==
          report.root_storage_capacity * sizeof(*report.roots) +
              report.unresolved_storage_capacity * sizeof(*report.unresolved));
    numerix_root_search_report_destroy(&report);
}

static void test_invalid_intervals_and_options(void)
{
    test_function function = {TEST_TWO_ROOTS};
    numerix_recommendation_request request;
    numerix_root_discovery_options options;
    numerix_root_search_report report;
    numerix_error error;

    initialize_request(&request, &function, NULL);
    numerix_root_discovery_options_init(&options);
    CHECK(numerix_root_find_all(
              &request, NAN, 3.0, &options, &report, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    options.initial_panels = 0U;
    CHECK(numerix_root_find_all(
              &request, 0.0, 3.0, &options, &report, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(report.roots == NULL);
}

int main(void)
{
    test_multiple_roots_and_multiplicity();
    test_endpoints_close_roots_and_no_roots();
    test_stationary_false_positive_and_pole();
    test_expression_and_piecewise_representations();
    test_resource_limits_and_cancellation();
    test_optional_root_conditioning();
    test_result_storage_grows_with_results();
    test_invalid_intervals_and_options();

    if (failures != 0) {
        fprintf(stderr, "%d root discovery checks failed\n", failures);
        return 1;
    }

    puts("All root discovery checks passed.");
    return 0;
}
