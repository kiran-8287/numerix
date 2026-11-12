#ifndef PHASE14C_HEAPTRACE_H
#define PHASE14C_HEAPTRACE_H

#include <stdint.h>

typedef struct phase14c_heaptrace_stats {
    uint64_t baseline_live_requested_bytes;
    uint64_t peak_additional_live_requested_bytes;
    int64_t current_live_delta_bytes;
    uint64_t total_requested_bytes;
    uint64_t allocation_count;
    uint64_t reallocation_count;
    uint64_t free_count;
    uint64_t untracked_pointer_count;
    int valid;
} phase14c_heaptrace_stats;

/* Enable tracking after inputs and reusable objects have been initialized. */
void phase14c_heaptrace_begin(void);
/* Stop tracking and return the isolated operation-scope counters. */
void phase14c_heaptrace_end(phase14c_heaptrace_stats *stats);

#endif
