# Managed root execution API

Use `numerix_managed_execution_create` with a root-finding
`numerix_recommendation_request`, then call `numerix_managed_execution_run`.
The request's callback, expression or piecewise function and any associated
data are borrowed until `numerix_managed_execution_destroy`.

```c
numerix_recommendation_request request = {0};
numerix_managed_execution_options managed_options = {0};
numerix_managed_execution *execution = NULL;
numerix_execution_summary summary;
numerix_error error;

request.category = NUMERIX_PROBLEM_ROOT_FINDING;
request.representation = NUMERIX_REPRESENTATION_CALLBACK;
request.function = my_function;
request.problem.root.lower = 0.0;
request.problem.root.upper = 2.0;
request.problem.root.has_bracket = 1;
managed_options.root_selection.mode = NUMERIX_SELECTION_AUTOMATIC;

if (numerix_managed_execution_create(
        &request, &managed_options, &execution, &error) ==
    NUMERIX_STATUS_SUCCESS)
{
    numerix_status status = numerix_managed_execution_run(execution, &error);
    numerix_managed_execution_get_summary(execution, &summary, &error);
    /* Inspect status, summary.result and counters here. */
    numerix_managed_execution_destroy(execution);
}
```

For observable steps and genuine pause/resume, select Bisection explicitly and
provide a `step_callback`. Each step pointer is borrowed only until that
callback returns. The callback may call `numerix_managed_execution_request_pause`;
`run` returns with state `NUMERIX_EXECUTION_PAUSED`. Later,
`numerix_managed_execution_resume` continues the preserved bracket.
`examples/managed_execution_demo.c` demonstrates this flow.

`numerix_managed_execution_request_cancel` is thread-safe and cooperative. It
is checked before the next function evaluation or Bisection step. Cancellation
does not interrupt a callback already executing. Querying progress from a step
or event callback is supported. Do not query or destroy the handle concurrently
with its run thread. Callbacks execute synchronously and must not assume
re-entrant execution.

Resource limits come from `numerix_options`. Evaluation and iteration limits
are enforced by the numerical routines and managed evaluation adapter. Time
limits use the platform monotonic clock and are checked at safe evaluation
boundaries. No strict memory limit is provided. Percent completion is marked
unavailable because total work is not generally known.

The managed API currently supports root finding only. Phase 4 root fallback is
monitored, and its per-attempt history is returned in the summary. Cancellation
and resource limits prevent any subsequent fallback attempt. Portfolio
statistics, per-step details for other root algorithms and managed execution
for other problem categories are not yet available. When fallback is enabled
for explicit Bisection, Bisection uses the library solver and therefore does
not emit steps or support pause/resume.
