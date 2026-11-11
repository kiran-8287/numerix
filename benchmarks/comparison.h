#ifndef NUMERIX_COMPARISON_H
#define NUMERIX_COMPARISON_H

#include "numerix.h"

#include <stddef.h>

typedef struct benchmark_memory {
    size_t allocations;
    size_t allocated_bytes;
    size_t live_bytes;
    size_t peak_bytes;
    int overflow;
} benchmark_memory;

void benchmark_memory_begin(void);
void benchmark_memory_fail_at(size_t allocation_index);
benchmark_memory benchmark_memory_end(void);
benchmark_memory benchmark_memory_snapshot(void);

#endif
