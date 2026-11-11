#include "numerix.h"

#include <stdio.h>
#include <string.h>

typedef struct demo_progress {
    size_t callbacks;
    size_t cancel_after;
} demo_progress;

static double multiple_root_function(double x, void *user_data)
{
    (void)user_data;
    return (x - 1.0) * (x - 2.0) * (x - 4.0);
}

static double repeated_root_function(double x, void *user_data)
{
    (void)user_data;
    return (x - 1.0) * (x - 1.0);
}

static double smooth_search_function(double x, void *user_data)
{
    (void)user_data;
    return x * x + 1.0;
}

static numerix_status cancel_progress(
    const numerix_root_search_report *report,
    void *user_data)
{
    demo_progress *progress = user_data;

    (void)report;
    ++progress->callbacks;
    if (progress->callbacks >= progress->cancel_after) {
        return NUMERIX_STATUS_CANCELLED;
    }
    return NUMERIX_STATUS_SUCCESS;
}

static void print_report(
    const char *label,
    numerix_status status,
    const numerix_root_search_report *report)
{
    size_t index;

    printf("%s: %s, %zu roots, %zu evaluations, %zu subdivisions\n",
           label,
           numerix_status_string(status),
           report->root_count,
           report->function_evaluations,
           report->subdivisions);
    for (index = 0; index < report->root_count; ++index) {
        const numerix_discovered_root *root = &report->roots[index];

        printf("  x = %.12g, |f(x)| = %.3g",
               root->result.value,
               root->diagnostics.absolute_residual);
        if (root->has_multiplicity_estimate) {
            printf(", estimated multiplicity %u",
                   root->estimated_multiplicity);
        }
        printf("\n");
    }
    if (report->root_count == 0U && status == NUMERIX_STATUS_SUCCESS) {
        puts("  No roots were detected under this search configuration.");
    }
    if (report->unresolved_count > 0U) {
        printf("  %zu unresolved intervals were recorded.\n",
               report->unresolved_count);
    }
    if (report->search_inconclusive) {
        puts("  Completeness is not guaranteed for arbitrary functions.");
    }
}

static int run_search(
    const char *label,
    const numerix_recommendation_request *request,
    double lower,
    double upper,
    const numerix_root_discovery_options *options)
{
    numerix_root_search_report report;
    numerix_error error;
    numerix_status status;

    status = numerix_root_find_all(
        request,
        lower,
        upper,
        options,
        &report,
        &error);
    print_report(label, status, &report);
    if (status != NUMERIX_STATUS_SUCCESS &&
        status != NUMERIX_STATUS_CANCELLED &&
        status != NUMERIX_STATUS_LIMIT_REACHED) {
        fprintf(stderr, "  Search error: %s\n", error.message);
    }
    numerix_root_search_report_destroy(&report);
    return status == NUMERIX_STATUS_SUCCESS ||
           status == NUMERIX_STATUS_CANCELLED ||
           status == NUMERIX_STATUS_LIMIT_REACHED;
}

int main(void)
{
    numerix_recommendation_request request;
    numerix_root_discovery_options discovery_options;
    numerix_options execution_options;
    numerix_piecewise_function *piecewise = NULL;
    numerix_error error;
    demo_progress progress;
    int successful = 1;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = multiple_root_function;
    numerix_root_discovery_options_init(&discovery_options);
    discovery_options.initial_panels = 48U;
    numerix_options_init(&execution_options);
    execution_options.maximum_function_evaluations = 5000U;
    request.options = &execution_options;
    successful &= run_search(
        "Three sign-changing roots",
        &request,
        0.0,
        5.0,
        &discovery_options);

    request.function = repeated_root_function;
    request.options = NULL;
    successful &= run_search(
        "Repeated tangential root",
        &request,
        0.0,
        2.0,
        &discovery_options);

    if (numerix_piecewise_create(&piecewise, &error) !=
        NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "Could not create piecewise function: %s\n",
                error.message);
        return 1;
    }
    if (numerix_piecewise_add_expression(
            piecewise,
            -2.0,
            0.0,
            1,
            0,
            "x+1",
            &error) != NUMERIX_STATUS_SUCCESS ||
        numerix_piecewise_add_expression(
            piecewise,
            0.0,
            2.0,
            1,
            1,
            "x-1",
            &error) != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "Could not create piecewise expressions: %s\n",
                error.message);
        numerix_piecewise_destroy(piecewise);
        return 1;
    }
    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_PIECEWISE;
    request.piecewise_function = piecewise;
    successful &= run_search(
        "Piecewise search split at its boundary",
        &request,
        -2.0,
        2.0,
        &discovery_options);
    numerix_piecewise_destroy(piecewise);

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = smooth_search_function;
    numerix_options_init(&execution_options);
    execution_options.maximum_function_evaluations = 1000U;
    request.options = &execution_options;
    memset(&progress, 0, sizeof(progress));
    progress.cancel_after = 2U;
    discovery_options.progress_interval = 1U;
    discovery_options.progress_callback = cancel_progress;
    discovery_options.progress_user_data = &progress;
    successful &= run_search(
        "Cancellable no-root search",
        &request,
        -4.0,
        4.0,
        &discovery_options);

    return successful ? 0 : 1;
}
