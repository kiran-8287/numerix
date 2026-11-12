#include "phase14c_heaptrace.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_vector.h>
#include <stdio.h>

int main(void)
{
    gsl_set_error_handler_off();
    phase14c_heaptrace_stats stats;
    phase14c_heaptrace_begin();
    gsl_vector *vector = gsl_vector_alloc(100U);
    if (vector == NULL) return 2;
    gsl_vector_set_all(vector, 1.0);
    gsl_vector_free(vector);
    phase14c_heaptrace_end(&stats);
    printf("GSL 2.8 tracked: valid=%d baseline=%llu peak=%llu total=%llu alloc=%llu free=%llu untracked=%llu\n",
        stats.valid,
        (unsigned long long)stats.baseline_live_requested_bytes,
        (unsigned long long)stats.peak_additional_live_requested_bytes,
        (unsigned long long)stats.total_requested_bytes,
        (unsigned long long)stats.allocation_count,
        (unsigned long long)stats.free_count,
        (unsigned long long)stats.untracked_pointer_count);
    return !(stats.valid && stats.peak_additional_live_requested_bytes >= 800U &&
        stats.allocation_count >= 1U && stats.free_count >= 1U &&
        stats.untracked_pointer_count == 0U);
}
