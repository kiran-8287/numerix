# Execution management (Phase 5)

The Phase 5 manager lives in `src/execution/managed_execution.c`. It builds on
the existing public problem descriptions, Phase 4 root recommendation and
selection routines, and the platform monotonic clock in
`src/core/numerix_platform.c`. It does not create a global manager or retain
unbounded history. One opaque `numerix_managed_execution` owns its counters,
latest result and fixed-size summary until the caller destroys it.

## Implemented scope

Managed execution currently supports scalar root-finding requests represented
as callbacks, compiled expressions or piecewise expressions. The normal run
path invokes Phase 4 selection. The caller may select a root algorithm
explicitly. Root solvers continue to enforce their existing iteration,
evaluation and elapsed-time limits. The manager counts evaluations across the
recommendation probes and solver invocation and observes cancellation before
each managed function evaluation.

Explicit Bisection has a managed implementation that preserves its bracket,
endpoint values and counters in the opaque handle. It emits immutable step
snapshots and can pause at an iteration boundary. Calling resume continues the
saved bracket; it does not restart Bisection. Other root algorithms use their
existing implementation and expose their final counters and result. Their
internal iteration snapshots are not exposed.

The lifecycle is `READY -> RUNNING -> COMPLETED`, `FAILED`, `CANCELLED`, or
`LIMIT_REACHED`. Explicit Bisection can additionally move
`RUNNING -> PAUSED -> READY -> RUNNING`. A terminal handle cannot be run again.
Cancellation is a C11 atomic request, observed at the next managed evaluation
or Bisection boundary. Cancellation requested while paused is returned when
the caller resumes the handle.

## Progress, events and ownership

Progress reports actual evaluation and iteration counters, elapsed monotonic
time, latest valid scalar sample and (for Bisection) the current interval.
There is no percentage because total work is not known. Derivative counts are
tracked for the managed root path when the user provides a derivative. Other
operation counters are not fabricated.

Event and step callbacks run synchronously on the thread calling `run` or
`resume`, without internal locks. Their event/step pointers are valid only for
the callback duration. The callback user data and all problem input pointers
are borrowed through `numerix_managed_execution_destroy`. Callbacks can safely
request cancellation. Only cancellation requests are safe from another
thread; concurrent queries and destruction are not safe. The user callback
itself must obey its own synchronization rules.

The execution object is the in-memory history. The caller controls its
lifetime; there is no process-wide history store. Summary copies are plain
values and own no additional allocations.

## Limits and known boundaries

The manager relies on the numerical options already present in
`numerix_options`: maximum iterations, function evaluations and execution
duration. Allocation limits are not implemented because NUMERIX's allocation
API does not currently track all allocations. Time checks use the existing
Windows performance counter or POSIX monotonic clock and occur at evaluation
boundaries. A user callback already in progress cannot be interrupted.

Phase 4 root fallback is monitored through a private stop-check extension. A
cancellation or managed time/evaluation limit is checked after each attempt
and before another fallback can begin. Per-attempt results are retained in
the summary's `root_history`. Portfolio execution remains unmanaged. If
fallback is enabled for explicit Bisection, the library Bisection routine is
used so that fallback can proceed; step snapshots and pause/resume are then
unavailable. Root-solver iteration step callbacks,
integration, linear algebra, optimization, ODE and differentiation are not
managed by this initial execution API. Consequently there are no claims of
pause/resume support for those algorithms. Bisection is the only genuinely
resumable and step-observable algorithm in Phase 5.

The ordinary Phase 0-4 direct APIs remain unchanged. The managed layer adds a
separate opt-in API and does not add instrumentation overhead to direct calls.

## Overhead sample

A local MinGW GCC 6.3 `-O2` comparison ran 100,000 direct Bisection solves and
100,000 managed explicit Bisection solves with both event and step callbacks
disabled. Two runs measured 0.102-0.103 seconds direct and 0.138-0.139 seconds
managed, about 1.35x total time. This includes managed handle allocation,
initialization, summary tracking and destruction, so it is an end-to-end API
comparison rather than a per-evaluation instrumentation cost. It is a small
single-machine sample, not a general performance guarantee.
