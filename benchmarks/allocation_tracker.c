/* GNU/ELF linker wrapping. Used only in the separate, untimed memory binary. */
#include "comparison.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TRACKED_POINTERS 65536U

typedef struct allocation_record {
    void *pointer;
    size_t bytes;
} allocation_record;

static allocation_record records[TRACKED_POINTERS];
static benchmark_memory measurements;
static int tracking;
static size_t failure_index = SIZE_MAX;
static size_t allocation_attempts;

void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void *__real_realloc(void *pointer, size_t bytes);
void __real_free(void *pointer);
void *__wrap_malloc(size_t bytes);
void *__wrap_calloc(size_t count, size_t bytes);
void *__wrap_realloc(void *pointer, size_t bytes);
void __wrap_free(void *pointer);

static void forget_allocation(void *pointer)
{
    size_t index;

    if (!tracking || pointer == NULL) {
        return;
    }
    for (index = 0; index < TRACKED_POINTERS; ++index) {
        if (records[index].pointer == pointer) {
            measurements.live_bytes -= records[index].bytes;
            records[index].pointer = NULL;
            return;
        }
    }
}

static void record_allocation(void *pointer, size_t bytes)
{
    size_t index;

    if (!tracking || pointer == NULL) {
        return;
    }
    ++measurements.allocations;
    measurements.allocated_bytes += bytes;
    measurements.live_bytes += bytes;
    if (measurements.live_bytes > measurements.peak_bytes) {
        measurements.peak_bytes = measurements.live_bytes;
    }
    for (index = 0; index < TRACKED_POINTERS; ++index) {
        if (records[index].pointer == NULL) {
            records[index].pointer = pointer;
            records[index].bytes = bytes;
            return;
        }
    }
    measurements.overflow = 1;
}

void benchmark_memory_begin(void)
{
    memset(records, 0, sizeof(records));
    memset(&measurements, 0, sizeof(measurements));
    failure_index = SIZE_MAX;
    allocation_attempts = 0U;
    tracking = 1;
}

void benchmark_memory_fail_at(size_t allocation_index)
{
    failure_index = allocation_index;
}

static int fail_this_allocation(void)
{
    return tracking && allocation_attempts++ == failure_index;
}

benchmark_memory benchmark_memory_snapshot(void)
{
    return measurements;
}

benchmark_memory benchmark_memory_end(void)
{
    tracking = 0;
    return measurements;
}

void *__wrap_malloc(size_t bytes)
{
    void *pointer;

    if (fail_this_allocation()) {
        return NULL;
    }
    pointer = __real_malloc(bytes);

    record_allocation(pointer, bytes);
    return pointer;
}

void *__wrap_calloc(size_t count, size_t bytes)
{
    void *pointer;

    if (fail_this_allocation()) {
        return NULL;
    }
    pointer = __real_calloc(count, bytes);

    if (bytes != 0 && count > SIZE_MAX / bytes) {
        measurements.overflow = 1;
    } else {
        record_allocation(pointer, count * bytes);
    }
    return pointer;
}

void *__wrap_realloc(void *pointer, size_t bytes)
{
    /* Retain the old entry on failure. No benchmark uses realloc(p, 0). */
    void *replacement;
    size_t index;
    size_t old_index = TRACKED_POINTERS;

    if (tracking && pointer != NULL) {
        for (index = 0; index < TRACKED_POINTERS; ++index) {
            if (records[index].pointer == pointer) {
                old_index = index;
                break;
            }
        }
    }
    if (fail_this_allocation()) {
        return NULL;
    }
    replacement = __real_realloc(pointer, bytes);
    if (replacement != NULL || bytes == 0) {
        if (old_index < TRACKED_POINTERS) {
            measurements.live_bytes -= records[old_index].bytes;
            records[old_index].pointer = NULL;
        }
        record_allocation(replacement, bytes);
    }
    return replacement;
}

void __wrap_free(void *pointer)
{
    forget_allocation(pointer);
    __real_free(pointer);
}
