# Advanced root discovery

`numerix_root_find_all` searches a finite interval for numerically detectable
roots. It is a separate orchestration API; the existing individual root solvers
continue to solve one root from a suitable guess or bracket.

The function accepts the existing `numerix_recommendation_request` so callback,
compiled-expression, and piecewise representations share the same evaluator.
Set `request.category` to `NUMERIX_PROBLEM_ROOT_FINDING`, set the appropriate
representation fields, and pass finite increasing interval bounds directly to
`numerix_root_find_all`. The interval arguments replace the request's root
bracket for this search.

## Minimal callback example

```c
#include <stdio.h>
#include <string.h>
#include "numerix.h"

static double two_roots(double x, void *user_data)
{
    (void)user_data;
    return (x - 1.0) * (x - 2.0);
}

numerix_recommendation_request request = {0};
numerix_root_discovery_options options;
numerix_root_search_report report;
numerix_error error;

request.category = NUMERIX_PROBLEM_ROOT_FINDING;
request.representation = NUMERIX_REPRESENTATION_CALLBACK;
request.function = two_roots;
numerix_root_discovery_options_init(&options);

numerix_status status = numerix_root_find_all(
    &request, 0.0, 3.0, &options, &report, &error);
if (status == NUMERIX_STATUS_SUCCESS) {
    size_t index;
    for (index = 0; index < report.root_count; ++index) {
        printf("root %.12g, residual %.3g\n",
               report.roots[index].result.value,
               report.roots[index].result.residual);
    }
}
numerix_root_search_report_destroy(&report);
```

The returned report owns its `roots` and `unresolved` arrays. Always call
`numerix_root_search_report_destroy`, including when the search returns a limit,
cancellation, or another error after producing partial results. The request,
function data, expression, and piecewise function are borrowed and must remain
valid for the duration of the synchronous call.

## Search strategy

The default configuration uses 32 initial panels, a maximum subdivision depth
of 12, up to 4096 adaptive subdivisions, and up to 256 candidate detections.
Defaults are available through `numerix_root_discovery_options_init`.

Each interval is sampled at its endpoints and midpoint. Opposite-sign sample
pairs are refined with Phase 4's existing root recommendation and fallback
path, which selects a suitable existing single-root method. Adaptive
subdivision follows normalized midpoint curvature and local-minimum evidence.
Tangential candidates are refined by minimizing `|f(x)|` with the existing
golden-section optimizer, then checking the original function residual. A
stationary point is not accepted solely because it is stationary or has a
small value. Bracket refinement asks the solver for a tighter residual than the
final acceptance threshold when the configured tolerances allow it; the
request's position tolerances still bound interval-width stopping.

Exact sampled zeros and both search endpoints are checked. Piecewise segment
boundaries from Phase 3 are inserted into the partition. Interior samples are
moved one representable value into the corresponding side, and no solver
bracket crosses a known piecewise boundary. Boundary classification is probed
with the Phase 3 analyzer; exact function evaluation at a boundary determines
whether that point itself is a root. An undefined open boundary is not reported
as a root. A sign-changing bracket that refines to a poor residual is rejected
and recorded as an unresolved, suspected-discontinuity region.

Candidate roots are merged only when their candidate intervals overlap within
the configured absolute and relative merge tolerances and their approximations
are close under the same scale-aware tolerance. A merge that combines
locations farther apart than the request's solver position tolerance is marked
`NUMERIX_ROOT_ACCEPTED_WITH_LIMITATIONS`, and a diagnostic warns that a nearby
root may have been combined. Reduce the merge tolerances to investigate close
roots.

Multiplicity is an optional ratio-based estimate from symmetric nearby
samples. An estimated integer order from 1 through 8 is labeled with
`NUMERIX_DIAGNOSTIC_SUSPECTED`; it is not an exact symbolic multiplicity. The
optional conditioning estimate reuses
`numerix_diagnostics_root_condition`. Residual and tangential-candidate
diagnostics use the existing Phase 6 report structures.

## Options and resource controls

`numerix_root_discovery_options` configures `initial_panels`, `maximum_depth`,
`maximum_subdivisions`, `maximum_candidates`, absolute `residual_tolerance`,
normalized `curvature_threshold`, `tangential_reduction`, and scale-aware root
merge tolerances. The implementation caps initial panels at 4096, depth at 32,
subdivisions at 65536, and candidate storage at 4096 to bound search work and
memory.

The request's `numerix_options` supplies maximum function evaluations,
refinement iterations, execution time, and solver position tolerances. These
limits are shared across sampling, refinement, verification, multiplicity
estimation, and optional condition estimation. A limit or cancellation returns
partial roots and sets `report.status`, `resource_limit_reached`, and
`completed_normally` accordingly.

The optional progress callback is called synchronously at search entry and
exit, and after the configured number of evaluations. It receives a borrowed,
read-only report snapshot. Return `NUMERIX_STATUS_CANCELLED` to stop; any other
non-success status also stops the search. This callback provides Phase 5 style
progress and cancellation for this synchronous API. It does not create a
managed-execution handle or a background task.

`unresolved_count` is the total number of intervals that could not be
classified; up to `maximum_candidates` intervals are stored in `unresolved`.
`unresolved_storage_truncated` indicates that the stored list is partial.
Candidate, rejected, merged, evaluation, subdivision, depth, failure,
boundary, and refinement counts are reported separately.

The `roots` and `unresolved` arrays grow geometrically from a small initial
capacity and never exceed `maximum_candidates`. Their capacities and actual
retained array bytes are reported by `root_storage_capacity`,
`unresolved_storage_capacity`, and `result_storage_bytes`. Empty result lists
do not retain corresponding result arrays after the search classifies them.
`workspace_peak_bytes` and
`workspace_allocation_count` cover buffers directly owned by root discovery,
including the result arrays and temporary partition/sample arrays. They
exclude nested diagnostic allocations, execution-context and solver internals,
allocator metadata, and process resident memory.

## Interpreting results and limitations

An empty root list means **no roots were detected under the configured search
strategy and tolerances**. It does not prove that the interval contains no
roots. `search_inconclusive` is always true because finite sampling cannot
prove completeness for arbitrary black-box functions. Roots may be missed when
they are narrower than the panel/subdivision resolution, when several roots
cancel between samples without triggering curvature evidence, when evaluation
fails near a candidate, or when time/evaluation/candidate/subdivision limits
stop the search. Highly oscillatory functions are especially difficult.

The absolute residual tolerance is measured in function-output units and is
independent of the x-position tolerance. A function with a nonzero minimum
smaller than this configured tolerance can be accepted as an approximate root;
choose a residual tolerance appropriate to the scale of the function. A
callback should represent a stable function during a search. Samples are
reused inside the search, so stateful callbacks can produce inconsistent
results.

Infinite interval bounds are rejected. Removable limits do not substitute for
the actual function value: an undefined point is not a root unless the
representation defines the function there.

## Compilable example and benchmark

Build the public-API examples with `make all` or the CMake build. Run
`numerix_root_discovery_demo` for multiple, repeated, piecewise, and cancelled
search examples. Run `make benchmark-root-discovery` to compare root counts,
evaluation counts, subdivision counts, runtime averaged over 1,000 searches, and
the reported direct-buffer allocation metrics at several initial scan
resolutions. Memory values exclude allocator overhead and process resident
memory.
