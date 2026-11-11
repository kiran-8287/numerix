#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

#define BENCHMARK_REPETITIONS 1000U

typedef enum benchmark_function_kind {
    BENCHMARK_SINGLE_ROOT = 0,
    BENCHMARK_MULTIPLE_ROOTS,
    BENCHMARK_REPEATED_ROOT,
    BENCHMARK_CLOSE_ROOTS,
    BENCHMARK_NO_ROOTS,
    BENCHMARK_POLE
} benchmark_function_kind;

typedef struct benchmark_function {
    benchmark_function_kind kind;
} benchmark_function;

static double evaluate_benchmark_function(double x, void *user_data)
{
    const benchmark_function *function = user_data;

    switch (function->kind) {
    case BENCHMARK_SINGLE_ROOT:
        return x - 1.25;
    case BENCHMARK_MULTIPLE_ROOTS:
        return (x - 1.0) * (x - 2.0) * (x - 4.0);
    case BENCHMARK_REPEATED_ROOT:
        return (x - 1.0) * (x - 1.0) * (x - 1.0) * (x - 1.0);
    case BENCHMARK_CLOSE_ROOTS:
        return (x - 1.0) * (x - 1.001);
    case BENCHMARK_NO_ROOTS:
        return x * x + 1.0;
    case BENCHMARK_POLE:
        return 1.0 / (x - 1.0);
    }

    return NAN;
}

static void run_case(
    const char *name,
    benchmark_function_kind kind,
    double lower,
    double upper,
    size_t panels)
{
    benchmark_function function = {kind};
    numerix_recommendation_request request = {0};
    numerix_root_discovery_options discovery_options;
    numerix_root_search_report report;
    numerix_error error;
    numerix_status status;
    clock_t started;
    clock_t finished;
    double cpu_seconds;
    size_t repetition;

    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.representation = NUMERIX_REPRESENTATION_CALLBACK;
    request.function = evaluate_benchmark_function;
    request.user_data = &function;
    numerix_root_discovery_options_init(&discovery_options);
    discovery_options.initial_panels = panels;

    started = clock();
    status = NUMERIX_STATUS_SUCCESS;
    for (repetition = 0; repetition < BENCHMARK_REPETITIONS; ++repetition) {
        if (repetition > 0U) {
            numerix_root_search_report_destroy(&report);
        }
        status = numerix_root_find_all(
            &request,
            lower,
            upper,
            &discovery_options,
            &report,
            &error);
        if (status != NUMERIX_STATUS_SUCCESS) {
            break;
        }
    }
    finished = clock();
    if (repetition == 0U || started == (clock_t)-1 ||
        finished == (clock_t)-1) {
        cpu_seconds = 0.0;
    } else {
        cpu_seconds = (double)(finished - started) /
            (double)CLOCKS_PER_SEC / (double)repetition;
    }

    printf(
        "%-16s panels=%3zu status=%-20s roots=%2zu evals=%5zu "
        "subdivisions=%5zu candidates=%4zu rejected=%4zu "
        "iterations=%4zu elapsed=%.6fs avg_cpu=%.9fs result=%zuB "
        "root_cap=%zu unresolved_cap=%zu result_allocs=%zu/%zu "
        "workspace_peak=%zuB workspace_allocs=%zu\n",
        name,
        panels,
        numerix_status_string(status),
        report.root_count,
        report.function_evaluations,
        report.subdivisions,
        report.candidate_count,
        report.rejected_candidates,
        report.refinement_iterations,
        report.elapsed_seconds,
        cpu_seconds,
        report.result_storage_bytes,
        report.root_storage_capacity,
        report.unresolved_storage_capacity,
        report.root_storage_allocation_count,
        report.unresolved_storage_allocation_count,
        report.workspace_peak_bytes,
        report.workspace_allocation_count);

    if (status != NUMERIX_STATUS_SUCCESS) {
        fprintf(stderr, "  benchmark call returned: %s\n", error.message);
    }
    for (size_t root_index = 0; root_index < report.root_count; ++root_index) {
        printf("  root[%zu]=%.12g residual=%.3g\n",
               root_index,
               report.roots[root_index].result.value,
               report.roots[root_index].result.residual);
    }
    numerix_root_search_report_destroy(&report);
}

int main(void)
{
    const size_t resolutions[] = {16U, 32U, 64U};
    size_t index;

    puts("NUMERIX Phase 8 root discovery benchmark");
    printf(
        "Average CPU time over %u repetitions; no external solver comparison is performed.\n",
        BENCHMARK_REPETITIONS);
    for (index = 0; index < sizeof(resolutions) / sizeof(resolutions[0]); ++index) {
        size_t panels = resolutions[index];

        run_case("single root", BENCHMARK_SINGLE_ROOT, 0.0, 2.0, panels);
        run_case("multiple roots", BENCHMARK_MULTIPLE_ROOTS, 0.0, 5.0, panels);
        run_case("repeated root", BENCHMARK_REPEATED_ROOT, 0.0, 2.0, panels);
        run_case("close roots", BENCHMARK_CLOSE_ROOTS, 0.0, 2.0, panels);
        run_case("no roots", BENCHMARK_NO_ROOTS, -2.0, 2.0, panels);
        run_case("pole", BENCHMARK_POLE, 0.0, 2.0, panels);
    }

    return 0;
}
