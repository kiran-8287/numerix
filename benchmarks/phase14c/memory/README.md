# Phase 14C allocation visibility probe

`phase14c_heaptrace.c` is a Linux/glibc-only interposer for requested heap
bytes. It wraps `malloc`, `calloc`, `realloc`, `aligned_alloc`, `memalign`,
`posix_memalign`, and `free`; a fixed static hash table avoids allocating from
the tracker itself. The tracker reports peak live requested bytes above the
pre-operation baseline, total requested bytes, allocation/reallocation/free
counts, and an invalid flag when a pointer cannot be attributed.

This is a **memory-only instrumentation path**. Its lock and pointer table
change timing behavior, so timings must use an uninstrumented binary. It does
not observe direct `mmap` workspaces, custom allocators that bypass the wrapped
glibc entry points, GPU memory, or memory mapped by the operating system. A
module/library route can be reported only after its own positive control and
allocation visibility have been checked; the smoke probes below do not
validate every SUNDIALS, NLopt, OpenBLAS, or LAPACK code path.

## Reproduce the validated positive controls

Run from the repository root. The probes use the isolated GSL 2.8 prefix and
the existing NUMERIX GCC release archive; they do not modify the system
installation.

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -Ibenchmarks/phase14c/memory \
  benchmarks/phase14c/memory/phase14c_heaptrace.c \
  benchmarks/phase14c/memory/interposer_smoke.c -ldl \
  -o /tmp/numerix-phase14c-heaptrace-smoke
/tmp/numerix-phase14c-heaptrace-smoke

gcc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -Ibenchmarks/phase14c/memory \
  -I/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean/include \
  benchmarks/phase14c/memory/phase14c_heaptrace.c \
  benchmarks/phase14c/memory/gsl_visibility_smoke.c \
  -L/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean/lib \
  -Wl,-rpath,/home/142401034/.local/numerix-deps/gsl-2.8-o3-clean/lib \
  -lgsl -lgslcblas -lm -ldl -o /tmp/numerix-phase14c-gsl-heaptrace-smoke
/tmp/numerix-phase14c-gsl-heaptrace-smoke
ldd /tmp/numerix-phase14c-gsl-heaptrace-smoke | rg 'gsl|cblas'

gcc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -Ibenchmarks/phase14c/memory -Iinclude \
  benchmarks/phase14c/memory/phase14c_heaptrace.c \
  benchmarks/phase14c/memory/numerix_visibility_smoke.c \
  build/linux-gcc-release/libnumerix.a -lm -ldl \
  -o /tmp/numerix-phase14c-num-heaptrace-smoke
/tmp/numerix-phase14c-num-heaptrace-smoke
```

Validated outputs on this host:

- Tracker control: `valid=1`, peak 970 B, 1,090 B total requested, 6
  allocations, 1 successful reallocation, 6 frees, no untracked pointers. The
  control covers aligned, POSIX-aligned, and `memalign` allocations plus a
  failed oversized `realloc` that must preserve its original pointer.
- GSL 2.8 `gsl_vector_alloc(100)`: `valid=1`, peak 856 B, 856 B total requested,
  3 allocations/frees, no untracked pointers; `ldd` resolved both GSL libraries
  from the isolated 2.8 prefix.
- NUMERIX `numerix_vector_add` on 100-element input vectors: `valid=1`, peak
  816 B, 816 B total requested, 2 allocations/frees, no untracked pointers,
  and the output check passed.

All three probes compile with `-Wpedantic -Werror`; `dlsym` symbols are copied
into function-pointer objects without non-ISO pointer casts. These
known-allocation checks establish that the interposer sees ordinary glibc heap
allocations made by the probe, GSL vector construction, and this NUMERIX
operation. They do not establish full allocator visibility for every
method in the seven modules. For memory runs, allocate the frozen input and
reusable harness objects before `phase14c_heaptrace_begin`, include output and
workspace allocations consistently, stop tracking only after the operation's
owned temporary/output objects have been released, and write `unavailable`
when the trace is invalid or the route uses an unobserved allocator.
