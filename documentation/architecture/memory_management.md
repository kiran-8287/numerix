# Memory management

`numerix_allocate` allocates with the C runtime allocator; size zero returns `NULL`. `numerix_deallocate` accepts `NULL` and otherwise releases memory allocated by the library-compatible allocator. Callers own returned allocations and must release them exactly once.

`numerix_context_create` initializes its output to `NULL`, validates options, and only publishes a context after successful allocation and initialization. On success, the caller owns the opaque context and must call `numerix_context_destroy` once. Destruction accepts `NULL`.

Scalar problems borrow the callback and `user_data`; neither is copied or freed by NUMERIX. Results currently require no allocation. Error messages are stored inline and do not transfer ownership.

For error paths, initialize output handles to `NULL` before work, release any locally acquired resources before returning, and do not free borrowed callback data. No custom allocator or hidden ownership transfer is used.
