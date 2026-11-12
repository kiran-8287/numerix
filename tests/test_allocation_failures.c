/* Fault injection is isolated to a GNU/ELF test binary; no public allocator API. */
#include "../benchmarks/comparison.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static double search_function(double x, void *data)
{
    int variant = *(int *)data;

    switch (variant) {
        case 0:
            return x * x + 1.0;
        case 1:
            return x * x - 2.0;
        case 2:
            return (x - 1.0) * (x - 2.0) * (x - 4.0);
        case 3:
            return 0.0;
        case 4:
        {
            size_t index;
            double value = 1.0;

            for (index = 0; index < 6U; ++index) {
                value *= x - (0.4 + 0.7 * (double)index);
            }
            return value;
        }
        default:
            return 1.0 / (x - 0.125);
    }
}

static int test_discovery_failures(int variant)
{
    numerix_recommendation_request request;
    numerix_root_search_report report;
    numerix_error error;
    numerix_status status;
    benchmark_memory memory;
    size_t allocation_count;
    size_t index;

    memset(&request, 0, sizeof(request));
    request.category = NUMERIX_PROBLEM_ROOT_FINDING;
    request.function = search_function;
    request.user_data = &variant;
    benchmark_memory_begin();
    status = numerix_root_find_all(&request, 0.0, 5.0, NULL, &report, &error);
    numerix_root_search_report_destroy(&report);
    memory = benchmark_memory_end();
    if (status != NUMERIX_STATUS_SUCCESS || memory.live_bytes != 0U || memory.overflow) {
        fprintf(stderr, "baseline discovery failed for variant %d\n", variant);
        return 0;
    }
    allocation_count = memory.allocations;
    for (index = 0; index < allocation_count; ++index) {
        benchmark_memory_begin();
        benchmark_memory_fail_at(index);
        status = numerix_root_find_all(&request, 0.0, 5.0, NULL, &report, &error);
        if (status != NUMERIX_STATUS_SUCCESS && status != NUMERIX_STATUS_ALLOCATION_FAILURE) {
            fprintf(stderr, "unexpected discovery status %d at allocation %zu\n",
                    (int)status, index);
            numerix_root_search_report_destroy(&report);
            (void)benchmark_memory_end();
            return 0;
        }
        numerix_root_search_report_destroy(&report);
        memory = benchmark_memory_end();
        if (memory.live_bytes != 0U || memory.overflow) {
            fprintf(stderr, "leaked %zu bytes for variant %d at allocation %zu\n",
                    memory.live_bytes, variant, index);
            return 0;
        }
    }
    printf("Discovery variant %d: injected failure at each of %zu allocations\n",
           variant, allocation_count);
    return 1;
}

static int test_expression_failures(void)
{
    size_t allocation_count;
    size_t index;
    numerix_expression *expression = NULL;
    numerix_error error;
    numerix_status status;
    benchmark_memory memory;

    benchmark_memory_begin();
    status = numerix_expression_compile("sin(x)+x^2-exp(-x)", &expression, &error);
    numerix_expression_destroy(expression);
    memory = benchmark_memory_end();
    if (status != NUMERIX_STATUS_SUCCESS || memory.live_bytes != 0U) {
        return 0;
    }
    allocation_count = memory.allocations;
    for (index = 0; index < allocation_count; ++index) {
        expression = NULL;
        benchmark_memory_begin();
        benchmark_memory_fail_at(index);
        status = numerix_expression_compile("sin(x)+x^2-exp(-x)", &expression, &error);
        numerix_expression_destroy(expression);
        memory = benchmark_memory_end();
        if (status != NUMERIX_STATUS_ALLOCATION_FAILURE || memory.live_bytes != 0U ||
            memory.overflow) {
            fprintf(stderr, "expression failure cleanup failed at allocation %zu\n", index);
            return 0;
        }
    }
    printf("Expression: injected failure at each of %zu allocations\n", allocation_count);
    return 1;
}

static double endpoint_integrand(double position, void *data)
{
    (void)data;
    return sqrt(position);
}

static int test_integration_failures(void)
{
    numerix_result result;
    numerix_error error;
    numerix_options options;
    numerix_status status;
    benchmark_memory memory;
    size_t index;
    size_t count;

    numerix_options_init(&options);
    options.absolute_tolerance = 1e-10;
    options.relative_tolerance = 1e-10;
    benchmark_memory_begin();
    status = numerix_integrate_adaptive_simpson(endpoint_integrand, NULL,
        0.0, 1.0, &options, 32U, &result, &error);
    memory = benchmark_memory_end();
    if (status != NUMERIX_STATUS_SUCCESS || memory.live_bytes != 0U)
    {
        return 0;
    }
    count = memory.allocations;
    for (index = 0; index < count; ++index)
    {
        benchmark_memory_begin();
        benchmark_memory_fail_at(index);
        status = numerix_integrate_adaptive_simpson(endpoint_integrand, NULL,
            0.0, 1.0, &options, 32U, &result, &error);
        memory = benchmark_memory_end();
        if (status != NUMERIX_STATUS_ALLOCATION_FAILURE || memory.live_bytes != 0U)
        {
            fprintf(stderr, "integration allocation failure cleanup failed at %zu\n", index);
            return 0;
        }
    }
    printf("Adaptive Simpson: injected failure at each of %zu allocations\n", count);
    return 1;
}

static int test_lu_allocation_failures(void)
{
    double matrix_values[] = {4.0, 1.0, 2.0, 3.0};
    numerix_matrix matrix = {2U, 2U, matrix_values};
    numerix_lu factorization;
    numerix_error error;
    numerix_status status;
    benchmark_memory memory;
    size_t allocation_count;
    size_t index;

    memset(&factorization, 0, sizeof(factorization));
    benchmark_memory_begin();
    status = numerix_lu_decompose(&matrix, NULL, &factorization, &error);
    numerix_lu_destroy(&factorization);
    memory = benchmark_memory_end();
    if (status != NUMERIX_STATUS_SUCCESS || memory.live_bytes != 0U ||
        memory.overflow) {
        fprintf(stderr, "baseline LU factorization failed\n");
        return 0;
    }
    allocation_count = memory.allocations;
    for (index = 0; index < allocation_count; ++index) {
        memset(&factorization, 0, sizeof(factorization));
        benchmark_memory_begin();
        benchmark_memory_fail_at(index);
        status = numerix_lu_decompose(&matrix, NULL, &factorization, &error);
        numerix_lu_destroy(&factorization);
        memory = benchmark_memory_end();
        if (status != NUMERIX_STATUS_ALLOCATION_FAILURE ||
            memory.live_bytes != 0U || memory.overflow ||
            factorization.data != NULL || factorization.permutation != NULL ||
            factorization.size != 0U) {
            fprintf(stderr, "LU allocation cleanup failed at allocation %zu\n",
                    index);
            return 0;
        }
    }
    printf("LU decomposition: injected failure at each of %zu allocations\n",
           allocation_count);
    return 1;
}

int main(void)
{
    int variant;

    if (!test_expression_failures() || !test_integration_failures() ||
        !test_lu_allocation_failures()) {
        return 1;
    }
    for (variant = 0; variant < 6; ++variant) {
        if (!test_discovery_failures(variant)) {
            return 1;
        }
    }
    puts("All allocation-failure checks passed.");
    return 0;
}
