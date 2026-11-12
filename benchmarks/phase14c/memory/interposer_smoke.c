#define _GNU_SOURCE
#include "phase14c_heaptrace.h"

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    volatile void *baseline = malloc(64U);
    if (baseline == NULL) return 2;
    phase14c_heaptrace_stats stats;
    phase14c_heaptrace_begin();
    void *first = malloc(120U);
    void *second = calloc(5U, 80U);
    void *aligned = aligned_alloc(64U, 128U);
    void *posix_aligned = NULL;
    void *mem_aligned = memalign(32U, 64U);
    void *failure_probe = malloc(32U);
    if (first == NULL || second == NULL || aligned == NULL || mem_aligned == NULL ||
        failure_probe == NULL || posix_memalign(&posix_aligned, 64U, 96U) != 0) return 3;
    first = realloc(first, 250U);
    if (first == NULL) return 4;
    volatile size_t impossible_size = SIZE_MAX;
    void *failed_realloc = realloc(failure_probe, impossible_size);
    if (failed_realloc != NULL) return 5;
    free(second);
    free(first);
    free(aligned);
    free(posix_aligned);
    free(mem_aligned);
    free(failure_probe);
    phase14c_heaptrace_end(&stats);
    free((void *)baseline);
    printf("valid=%d baseline=%llu peak=%llu total=%llu alloc=%llu realloc=%llu free=%llu delta=%lld untracked=%llu\n",
        stats.valid,
        (unsigned long long)stats.baseline_live_requested_bytes,
        (unsigned long long)stats.peak_additional_live_requested_bytes,
        (unsigned long long)stats.total_requested_bytes,
        (unsigned long long)stats.allocation_count,
        (unsigned long long)stats.reallocation_count,
        (unsigned long long)stats.free_count,
        (long long)stats.current_live_delta_bytes,
        (unsigned long long)stats.untracked_pointer_count);
    return !(stats.valid && stats.peak_additional_live_requested_bytes == 970U &&
        stats.total_requested_bytes == 1090U && stats.allocation_count == 6U &&
        stats.reallocation_count == 1U && stats.free_count == 6U &&
        stats.current_live_delta_bytes == 0 && stats.untracked_pointer_count == 0U);
}
