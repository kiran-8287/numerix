# Core data structures

- **`numerix_status`**: stable categories for success, caller input errors, allocation failure, numerical/convergence failure, unsupported operations, cancellation, limits, and internal errors. `numerix_status_string` gives a static readable name.
- **`numerix_error`**: status plus a fixed-capacity message copied with bounded formatting. `value` and `has_value` reserve optional numeric context; Phase 0 APIs leave it unset.
- **`numerix_function`**: `double (*)(double x, void *user_data)`. The callback and data are borrowed. A callback must remain valid for the lifetime of its problem.
- **`numerix_scalar_problem`**: scalar callback and borrowed user data. Evaluation rejects invalid inputs and non-finite callback results.
- **`numerix_options`**: absolute and relative tolerances, iteration and evaluation limits, and optional elapsed-time limit. Tolerances must be finite and nonnegative, at least one must be positive, count limits must be positive, and the time limit must be finite and nonnegative. A zero time limit disables timing limits.
- **`numerix_result`**: common value/status, error estimate, residual, counters, elapsed time, and borrowed algorithm label. It is a foundation; specialized algorithms may define safer specialized result types.
- **`numerix_context`**: opaque publicly and owned by the caller after successful creation. It copies options and tracks status/error, counters, monotonic start time, and a reserved cancellation flag. Phase 0 exposes counter operations but no cancellation workflow.

Default options are absolute tolerance `1e-8`, relative tolerance `1e-8`, 1000 iterations, 10000 function evaluations, and no time limit.
