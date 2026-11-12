#include "phase14c_heaptrace.h"

#include <numerix.h>

#include <stdio.h>

int main(void)
{
    numerix_error error = {0};
    numerix_vector *left = NULL;
    numerix_vector *right = NULL;
    numerix_vector *sum = NULL;
    if (numerix_vector_create(100U, &left, &error) != NUMERIX_STATUS_SUCCESS ||
        numerix_vector_create(100U, &right, &error) != NUMERIX_STATUS_SUCCESS) return 2;
    for (size_t i = 0; i < 100U; ++i) {
        left->data[i] = (double)i;
        right->data[i] = 2.0 * (double)i;
    }

    phase14c_heaptrace_stats stats;
    phase14c_heaptrace_begin();
    numerix_status status = numerix_vector_add(left, right, &sum, &error);
    if (status != NUMERIX_STATUS_SUCCESS || sum == NULL) return 3;
    int value_ok = sum->size == 100U && sum->data[99] == 297.0;
    numerix_vector_destroy(sum);
    phase14c_heaptrace_end(&stats);
    numerix_vector_destroy(right);
    numerix_vector_destroy(left);

    printf("NUMERIX vector_add tracked: valid=%d baseline=%llu peak=%llu total=%llu alloc=%llu free=%llu untracked=%llu value_ok=%d\n",
        stats.valid,
        (unsigned long long)stats.baseline_live_requested_bytes,
        (unsigned long long)stats.peak_additional_live_requested_bytes,
        (unsigned long long)stats.total_requested_bytes,
        (unsigned long long)stats.allocation_count,
        (unsigned long long)stats.free_count,
        (unsigned long long)stats.untracked_pointer_count,
        value_ok);
    return !(stats.valid && stats.peak_additional_live_requested_bytes >= 800U &&
        stats.allocation_count >= 2U && stats.free_count >= 2U &&
        stats.untracked_pointer_count == 0U && value_ok);
}
