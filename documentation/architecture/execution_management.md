# Execution management (Phase 5)

The Phase 5 manager lives in `src/execution/managed_execution.c`. It builds on
the existing public problem descriptions, Phase 4 root recommendation and
selection routines, and the platform monotonic clock in
`src/core/numerix_platform.c`. It does not create a global manager or retain
unbounded history. One opaque `numerix_managed_execution` owns its counters,
latest result and fixed-size summary until the caller destroys it.

## Implemented scope

Managed execution dispatches all seven Phase 4 problem categories through the
existing execution engine. Callback, expression and piecewise scalar requests
are preserved. Root problems use Phase 4 selection and optional fallback;
non-root explicit methods are selected with
`numerix_managed_execution_options.execution.explicit_algorithm`. The manager
uses the existing opaque `numerix_context` for centralized function and
iteration counts and the platform monotonic clock for execution time.

Cancellation is polled around managed scalar, objective, gradient and ODE
callbacks. Automatic root recommendation performs its bounded endpoint and
optional derivative probes using the original representation, then checks
cancellation before launching an algorithm. The linear algebra and
interpolation routines do not expose cooperative checkpoints, so a
cancellation requested while they are running is observed after the routine
returns. Piecewise-aware differentiation and
piecewise integration retain their Phase 3 adapters, which also do not expose
managed checkpoints. Existing numerical iteration, evaluation and time limits
are reused; the manager additionally enforces aggregate root iteration and
callback evaluation budgets across fallback attempts.

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

Progress reports actual callback and derivative counts, elapsed monotonic
time, and iteration counts when available. Root methods expose the most recent
sample, and Bisection exposes its current bracket. Completed scalar methods
expose their result and estimated error when supplied by the algorithm.
`percentage_available` is false because total work is not known. Linear
algebra and ODE expose their final allocated output through
`numerix_managed_execution_get_output`; they do not invent a scalar progress
approximation.

Event and step callbacks run synchronously on the thread calling `run` or
`resume`, without internal locks. Their event/step pointers are valid only for
the callback duration. The callback user data and all problem input pointers
are borrowed through `numerix_managed_execution_destroy`. Callbacks can safely
request cancellation. Only cancellation requests are safe from another
thread; concurrent queries and destruction are not safe. The user callback
itself must obey its own synchronization rules. User callbacks can request
cancellation from their supported callback context.

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
unavailable. Portfolio execution remains unmanaged. Steps and genuine
pause/resume are implemented only for explicit Bisection. Other methods expose
their final results and counters but not internal stage snapshots or
checkpoints. Strict memory limits are not implemented because allocations
inside the existing algorithms are not comprehensively tracked.

The ordinary Phase 0-4 direct APIs remain unchanged. The managed layer adds a
separate opt-in API and does not add instrumentation overhead to direct calls.

## Overhead sample

A local MinGW GCC 6.3 `-O2` comparison ran 500,000 direct Bisection solves and
500,000 managed explicit Bisection solves with both event and step callbacks
disabled. Two runs measured 0.505-0.510 seconds direct and 0.800-0.805 seconds
managed, about 1.58x total time. This includes handle and shared-context
allocation, initialization, summary tracking and destruction, so it is an
end-to-end API comparison rather than a per-evaluation instrumentation cost.
It is a small single-machine sample, not a general performance guarantee.
