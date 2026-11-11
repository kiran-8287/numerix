# Release API and compatibility review

The public API remains in `include/numerix.h`. The 0.2.0-rc1 hardening/distribution
milestones add no public function signatures, algorithm families or struct fields.
Names and types remain unchanged; corrected root stopping/status behavior is an
intentional contract correction.

## Initialize, call, inspect, destroy

- Use `numerix_options_init` and `numerix_root_discovery_options_init` for
  defaults. Most numerical calls accept NULL shared options for defaults.
- Zero-initialize recommendation requests, execution options and public owner
  structs before first use. Fill the tagged category/representation and matching
  union member. Zero execution selection means automatic selection; fill explicit
  selection fields when reproducibility requires a specific method.
- Check every returned status. An error pointer may be NULL. Phase 9 fixes
  `numerix_error_set(NULL, code, ...)` to return `code`, so omitting an optional
  error output no longer changes a numerical failure into INVALID_ARGUMENT.
  This deliberately corrects observable error-path behavior from prior versions.
- Scalar results are inline and own no storage. Their algorithm/name strings
  are borrowed static strings. Public metrics are method-specific: unknown
  error estimates may be NAN; iterations/time are not populated uniformly by
  every method. Consult the category guide and do not infer unsupported metrics
  from zero-valued fields.
- Create functions return caller-owned handles. Destroy vectors, matrices,
  contexts, expressions and piecewise objects with the matching destroy API.
  `numerix_lu_solve` creates a new vector; its output is now cleared on invalid
  input and out-of-range permutation entries are rejected before indexing.
- `numerix_lu`, `numerix_interpolant`, `numerix_ode_solution`,
  `numerix_diagnostics_report`, `numerix_multivariable_sensitivity`,
  `numerix_execution_output` and `numerix_root_search_report` contain owned
  allocations. **Destroy before reusing the same struct as an output**; calls
  initialize output storage and do not release a previous live value for you.
  Initialize diagnostic/sensitivity structs to zero before a call if destroying
  after an early validation failure: not every function initializes every output
  before rejecting arguments. Discovery specifically requires destroying partial
  reports after failure as well as successful reports.
- Inputs, callbacks, user data, and request buffers are borrowed. Managed
  executions borrow inputs until destroyed; their returned output resources are
  owned by the execution and become invalid when it is destroyed. Callback
  event/step pointers live only for the callback. Immutable expressions can be
  shared; mutable callback contexts should be separate per active invocation.
- Public dense matrix storage is contiguous row-major. Positive dimensions,
  finite data and compatible shapes are required. Do not edit a successful LU
  factorization's size or pointers; validation cannot infer buffer lengths from
  arbitrary user-supplied pointers.

## Accuracy and resource limits

Absolute/relative tolerances are interpreted by the chosen method, not as a
uniform forward-error guarantee. Root position and residual units are separate;
Phase 10 removed small-residual stopping. Open methods still use heuristic local
position evidence. Check independently scaled residuals, conditioning and brackets. Fixed quadrature, differentiation and ODE routines
use caller-chosen grids/steps and cannot promise tolerance-controlled accuracy.
Estimated errors are method-dependent estimates, not certified bounds.

Raw numerical entry points do not uniformly enforce the shared time-limit field.
Use managed execution for supported cooperative cancellation/time monitoring;
only explicit Bisection supports stepwise pause/resume. Do not race queries or
destruction with running callbacks. No general parallel execution is implemented.
Discovery and multiplicity/discontinuity classifications are numerical evidence,
not completeness or mathematical proofs.

## ABI and source compatibility

Phase 8 expanded public diagnostics/root-search structures. A precompiled client
using an earlier size/layout must be rebuilt with the matching header and archive.
No ABI compatibility claim or stable binary interface is made for 0.2.0-rc1. Phases 9–12
preserve the current struct layouts and existing source signatures. The corrected
NULL-error status and new validation of invalid LU inputs are intentional behavior
changes. Linux and Windows may have different pointer widths/alignment and struct
sizes; do not compare their allocation-byte counts as though layouts were identical.

## Remaining API limitations

The library has no custom allocator, shared-library ABI, or uniform initializer
for every public aggregate. CMake now installs the NUMERIX::numerix package target.
Some metrics/status initialization conventions differ between older numerical
entry points. These are documented limitations for the release candidate; an
architecture rewrite or broad API rename was deliberately outside this milestone.

See [tolerance semantics](tolerances.md) and the
[compatibility policy](../user_guide/compatibility.md) for current contracts.
