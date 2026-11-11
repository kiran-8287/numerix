# Root discovery architecture

Phase 7 adds a bounded search layer above the existing single-root methods.
`src/root_discovery/root_discovery.c` owns the scan orchestration and uses the
existing request evaluator for callback, expression, and piecewise inputs.

The search forms ordered cut points from a configurable uniform partition and
known piecewise boundaries. It evaluates cut points once, reuses those samples
for neighboring cells, and evaluates midpoints during bounded recursive
subdivision. Each midpoint is checked for zeros and sign changes on both
halves. Curvature and local absolute-value minima decide whether a cell merits
more samples. Tangential candidates use the existing golden-section
minimizer; sign-changing candidates use the Phase 4 recommendation and
fallback orchestration with existing root algorithms.

Piecewise boundaries are never crossed by a refinement bracket. Phase 3's
boundary analysis is called with a bounded one-iteration probe before scanning;
its finite boundary value and the per-side search samples are considered
separately. The actual representation value is required for a boundary root.
For callback poles without known boundary metadata, candidate refinement must
still pass a fresh residual check. Poor-residual refinements are rejected and
their intervals remain unresolved.

All search allocations are bounded by validated configuration caps. The
existing `numerix_context` tracks time and function evaluations. Refinement
iteration totals share the request's maximum. The progress callback runs on
the calling thread and can stop the search at evaluation safe points. Partial
roots are returned on resource exhaustion or cancellation when available.

The result arrays are owned by `numerix_root_search_report` and released by
`numerix_root_search_report_destroy`. Search completeness remains
mathematically unprovable for an arbitrary finite-sampled black-box function;
the report always marks the search inconclusive and stores unresolved regions.

Phase10 refinement uses position tolerances independently of the discovery
residual threshold. A sample-only root has no position error estimate (NAN),
and merging candidates never replaces a position estimate with a function
residual. Discovery remains heuristic and can accept a small sampled value in a
flat/small-scaled function; its explicit residual threshold is not a position
certificate. This distinction also applies to tangential detection and merging.
