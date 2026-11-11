# Memory management

`numerix_allocate` allocates with the C runtime allocator; size zero returns `NULL`. `numerix_deallocate` accepts `NULL` and otherwise releases memory allocated by the library-compatible allocator. Callers own returned allocations and must release them exactly once.

`numerix_context_create` initializes its output to `NULL`, validates options, and only publishes a context after successful allocation and initialization. On success, the caller owns the opaque context and must call `numerix_context_destroy` once. Destruction accepts `NULL`.

Scalar problems borrow the callback and `user_data`; neither is copied or freed by NUMERIX. The common `numerix_result` is inline and does not own allocations. Error messages are stored inline and do not transfer ownership.

Phase 1 vector and matrix results are allocated objects; use `numerix_vector_destroy` or `numerix_matrix_destroy`. `numerix_lu_decompose` and `numerix_interp_newton_create` initialize caller-provided structs and allocate their owned arrays; release successful values with their matching destroy function. ODE solvers allocate both arrays in `numerix_ode_solution`; use `numerix_ode_solution_destroy` after success.

Compiled expressions are opaque caller-owned objects released with `numerix_expression_destroy`. Expression callback contexts borrow an expression and do not allocate; the expression must outlive each callback use. Use a separate mutable callback context per concurrent numerical call.

For error paths, initialize output handles to `NULL` before work, release any locally acquired resources before returning, and do not free borrowed callback data. No custom allocator or hidden ownership transfer is used.
