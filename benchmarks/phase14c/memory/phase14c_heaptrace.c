#define _GNU_SOURCE
#include "phase14c_heaptrace.h"

#include <dlfcn.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* Linux/glibc-only benchmark interposer. Its static table never allocates. */
extern void *__libc_malloc(size_t);
extern void *__libc_calloc(size_t, size_t);
extern void *__libc_realloc(void *, size_t);
extern void __libc_free(void *);
extern void *__libc_memalign(size_t, size_t);

typedef struct pointer_record {
    void *pointer;
    size_t requested;
    unsigned char state; /* 0 empty, 1 live, 2 tombstone */
} pointer_record;

#define RECORD_CAPACITY (1U << 18)
#define RECORD_MASK (RECORD_CAPACITY - 1U)
#define TOMBSTONE ((void *)(uintptr_t)1U)

static pointer_record records[RECORD_CAPACITY];
static atomic_flag table_lock = ATOMIC_FLAG_INIT;
static _Atomic int resolver_state;
static _Atomic int tracking_active;
static _Thread_local int inside_resolver;
static void *(*next_malloc)(size_t);
static void *(*next_calloc)(size_t, size_t);
static void *(*next_realloc)(void *, size_t);
static void (*next_free)(void *);
static void *(*next_aligned_alloc)(size_t, size_t);
static void *(*next_memalign)(size_t, size_t);
static int (*next_posix_memalign)(void **, size_t, size_t);

static uint64_t live_requested;
static uint64_t baseline_live;
static uint64_t peak_additional;
static uint64_t total_requested;
static uint64_t allocation_count;
static uint64_t reallocation_count;
static uint64_t free_count;
static uint64_t untracked_pointer_count;
static int trace_valid = 1;

static void lock_table(void)
{
    while (atomic_flag_test_and_set_explicit(&table_lock, memory_order_acquire)) { }
}

static void unlock_table(void)
{
    atomic_flag_clear_explicit(&table_lock, memory_order_release);
}

static size_t pointer_hash(void *pointer)
{
    uintptr_t value = (uintptr_t)pointer;
    value ^= value >> 29U;
    value *= (uintptr_t)0x9e3779b185ebca87ULL;
    value ^= value >> 32U;
    return (size_t)value & RECORD_MASK;
}

static size_t find_record(void *pointer)
{
    size_t index = pointer_hash(pointer);
    for (size_t probe = 0; probe < RECORD_CAPACITY; ++probe) {
        size_t slot = (index + probe) & RECORD_MASK;
        if (records[slot].state == 0U) return RECORD_CAPACITY;
        if (records[slot].state == 1U && records[slot].pointer == pointer) return slot;
    }
    return RECORD_CAPACITY;
}

static int insert_record(void *pointer, size_t requested)
{
    size_t index = pointer_hash(pointer);
    size_t tombstone = RECORD_CAPACITY;
    for (size_t probe = 0; probe < RECORD_CAPACITY; ++probe) {
        size_t slot = (index + probe) & RECORD_MASK;
        if (records[slot].state == 1U && records[slot].pointer == pointer) {
            records[slot].requested = requested;
            return 1;
        }
        if (records[slot].state == 2U && tombstone == RECORD_CAPACITY) tombstone = slot;
        if (records[slot].state == 0U) {
            if (tombstone != RECORD_CAPACITY) slot = tombstone;
            records[slot].pointer = pointer;
            records[slot].requested = requested;
            records[slot].state = 1U;
            return 1;
        }
    }
    if (tombstone != RECORD_CAPACITY) {
        records[tombstone].pointer = pointer;
        records[tombstone].requested = requested;
        records[tombstone].state = 1U;
        return 1;
    }
    return 0;
}

static void remove_record(size_t slot)
{
    records[slot].pointer = TOMBSTONE;
    records[slot].requested = 0U;
    records[slot].state = 2U;
}

static void update_peak_locked(void)
{
    if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) {
        uint64_t additional = live_requested > baseline_live ? live_requested - baseline_live : 0U;
        if (additional > peak_additional) peak_additional = additional;
    }
}

static void resolve_function_symbol(void *target, size_t target_size,
                                    const char *symbol_name)
{
    void *symbol = dlsym(RTLD_NEXT, symbol_name);
    /* POSIX defines dlsym results for function symbols; copying the object
       representation avoids a non-ISO C object/function pointer cast. */
    if (symbol != NULL && target_size == sizeof(symbol)) {
        memcpy(target, &symbol, sizeof(symbol));
    }
}

static void record_allocation(void *pointer, size_t requested, int is_reallocation,
                              void *old_pointer)
{
    if (pointer == NULL) return;
    lock_table();
    size_t old_size = 0U;
    if (old_pointer != NULL) {
        size_t old_slot = find_record(old_pointer);
        if (old_slot != RECORD_CAPACITY) {
            old_size = records[old_slot].requested;
            if (old_pointer != pointer) remove_record(old_slot);
        } else if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) {
            ++untracked_pointer_count;
            trace_valid = 0;
        }
    }
    if (old_size <= live_requested) live_requested -= old_size;
    else { live_requested = 0U; trace_valid = 0; }
    if (!insert_record(pointer, requested)) {
        trace_valid = 0;
        if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) ++untracked_pointer_count;
    }
    if (UINT64_MAX - live_requested < (uint64_t)requested) {
        live_requested = UINT64_MAX;
        trace_valid = 0;
    } else {
        live_requested += (uint64_t)requested;
    }
    if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) {
        total_requested += (uint64_t)requested;
        if (is_reallocation) ++reallocation_count;
        else ++allocation_count;
        update_peak_locked();
    }
    unlock_table();
}

static size_t record_free(void *pointer)
{
    if (pointer == NULL) return 0U;
    lock_table();
    size_t slot = find_record(pointer);
    size_t requested = 0U;
    if (slot != RECORD_CAPACITY) {
        requested = records[slot].requested;
        remove_record(slot);
        if (requested <= live_requested) live_requested -= (uint64_t)requested;
        else { live_requested = 0U; trace_valid = 0; }
        if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) ++free_count;
        update_peak_locked();
    } else if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) {
        ++untracked_pointer_count;
        trace_valid = 0;
    }
    unlock_table();
    return requested;
}

static void resolve_next_symbols(void)
{
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&resolver_state, &expected, 1,
            memory_order_acq_rel, memory_order_acquire)) {
        inside_resolver = 1;
        resolve_function_symbol(&next_malloc, sizeof(next_malloc), "malloc");
        resolve_function_symbol(&next_calloc, sizeof(next_calloc), "calloc");
        resolve_function_symbol(&next_realloc, sizeof(next_realloc), "realloc");
        resolve_function_symbol(&next_free, sizeof(next_free), "free");
        resolve_function_symbol(&next_aligned_alloc, sizeof(next_aligned_alloc), "aligned_alloc");
        resolve_function_symbol(&next_memalign, sizeof(next_memalign), "memalign");
        resolve_function_symbol(&next_posix_memalign, sizeof(next_posix_memalign), "posix_memalign");
        inside_resolver = 0;
        atomic_store_explicit(&resolver_state, 2, memory_order_release);
    }
}

void phase14c_heaptrace_begin(void)
{
    /* Resolve wrappers before the measured scope so dlsym initialization is
       not attributed to the numerical operation. */
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) {
        resolve_next_symbols();
    }
    lock_table();
    baseline_live = live_requested;
    peak_additional = 0U;
    total_requested = 0U;
    allocation_count = 0U;
    reallocation_count = 0U;
    free_count = 0U;
    untracked_pointer_count = 0U;
    trace_valid = 1;
    atomic_store_explicit(&tracking_active, 1, memory_order_release);
    unlock_table();
}

void phase14c_heaptrace_end(phase14c_heaptrace_stats *stats)
{
    if (stats == NULL) return;
    lock_table();
    atomic_store_explicit(&tracking_active, 0, memory_order_release);
    stats->baseline_live_requested_bytes = baseline_live;
    stats->peak_additional_live_requested_bytes = peak_additional;
    if (live_requested >= baseline_live) {
        uint64_t delta = live_requested - baseline_live;
        stats->current_live_delta_bytes = delta > (uint64_t)INT64_MAX ? INT64_MAX : (int64_t)delta;
    } else {
        uint64_t delta = baseline_live - live_requested;
        stats->current_live_delta_bytes = delta > (uint64_t)INT64_MAX ? INT64_MIN : -(int64_t)delta;
    }
    stats->total_requested_bytes = total_requested;
    stats->allocation_count = allocation_count;
    stats->reallocation_count = reallocation_count;
    stats->free_count = free_count;
    stats->untracked_pointer_count = untracked_pointer_count;
    stats->valid = trace_valid;
    unlock_table();
}

void *malloc(size_t size)
{
    if (inside_resolver) return __libc_malloc(size);
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    void *pointer = next_malloc != NULL ? next_malloc(size) : __libc_malloc(size);
    record_allocation(pointer, size, 0, NULL);
    return pointer;
}

void *calloc(size_t count, size_t size)
{
    if (inside_resolver) return __libc_calloc(count, size);
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    void *pointer = next_calloc != NULL ? next_calloc(count, size) : __libc_calloc(count, size);
    size_t requested = (size != 0U && count > SIZE_MAX / size) ? SIZE_MAX : count * size;
    record_allocation(pointer, requested, 0, NULL);
    return pointer;
}

void *realloc(void *old_pointer, size_t size)
{
    if (inside_resolver) return __libc_realloc(old_pointer, size);
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    void *pointer = next_realloc != NULL ? next_realloc(old_pointer, size) : __libc_realloc(old_pointer, size);
    if (pointer != NULL) record_allocation(pointer, size, 1, old_pointer);
    else if (size == 0U && old_pointer != NULL) {
        (void)record_free(old_pointer);
        lock_table();
        if (atomic_load_explicit(&tracking_active, memory_order_relaxed) != 0) ++reallocation_count;
        unlock_table();
    }
    return pointer;
}

void free(void *pointer)
{
    if (pointer == NULL) return;
    if (inside_resolver) { __libc_free(pointer); return; }
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    (void)record_free(pointer);
    if (next_free != NULL) next_free(pointer); else __libc_free(pointer);
}

void *aligned_alloc(size_t alignment, size_t size)
{
    if (inside_resolver) return __libc_memalign(alignment, size);
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    void *pointer = next_aligned_alloc != NULL ? next_aligned_alloc(alignment, size) : __libc_memalign(alignment, size);
    record_allocation(pointer, size, 0, NULL);
    return pointer;
}

void *memalign(size_t alignment, size_t size)
{
    if (inside_resolver) return __libc_memalign(alignment, size);
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    void *pointer = next_memalign != NULL ? next_memalign(alignment, size) : __libc_memalign(alignment, size);
    record_allocation(pointer, size, 0, NULL);
    return pointer;
}

int posix_memalign(void **result, size_t alignment, size_t size)
{
    if (inside_resolver) {
        void *pointer = __libc_memalign(alignment, size);
        if (pointer == NULL) return 12;
        *result = pointer;
        return 0;
    }
    if (atomic_load_explicit(&resolver_state, memory_order_acquire) != 2) resolve_next_symbols();
    if (next_posix_memalign != NULL) {
        int status = next_posix_memalign(result, alignment, size);
        if (status == 0) record_allocation(*result, size, 0, NULL);
        return status;
    }
    void *pointer = __libc_memalign(alignment, size);
    if (pointer == NULL) return 12;
    *result = pointer;
    record_allocation(pointer, size, 0, NULL);
    return 0;
}
