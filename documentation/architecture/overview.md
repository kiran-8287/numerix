# Architecture overview

NUMERIX is a C11 static library with one public header, include/numerix.h.
Applications consume that interface; private headers and algorithm state stay
under src/. The numerical families and existing ownership boundaries are preserved.

## Modules

- Core: status/error/options, allocation, scalar callbacks, execution contexts
  and Windows/POSIX monotonic timing.
- Algorithms: roots, dense linear algebra, finite differences, interpolation,
  quadrature, optimization and fixed-step vector ODEs.
- Expression: lexer, bounded recursive-descent parser, immutable AST, evaluator
  and callback/error adapter.
- Piecewise: ordered owned expression segments, interval inclusion, one-sided
  limit evidence, boundary classification and guarded numerical adapters.
- Intelligence: tagged request inspection, deterministic recommendations,
  explicit overrides, root fallback and sequential portfolios.
- Execution: managed dispatch, cooperative cancellation/time/evaluation limits,
  event/progress summaries and resumable explicit Bisection sessions.
- Diagnostics: post-run residual/error/conditioning/sensitivity analysis,
  convergence observers and structured, qualified numerical evidence.
- Root discovery: bounded finite sampling, candidate/refinement orchestration,
  dynamic result storage, unresolved regions and heuristic multiplicity evidence.
- CLI/examples: linked demonstrations and input/error checks, without a separate
  numerical implementation or general interactive application framework.

## Boundaries and flow

Callers supply borrowed callbacks/data or representations, options and typed
requests. Raw algorithms return scalar or owned specialized outputs. Intelligence
selects/dispatches those algorithms; managed execution wraps supported calls and
owns their output resources. Diagnostics observe existing results and can make
additional, counted evaluations. Discovery shares the solver/diagnostics machinery
while retaining its own residual/merge criteria and resource accounting.

Matrices, datasets and ODE states retain their own representations; they are not
forced into a scalar callback. Destroy functions release owned resources; borrowed
inputs remain valid for the documented call/session lifetime. Only cancellation
requests are generally thread-safe during a managed run; other queries/destruction
must not race. No general parallel execution architecture was introduced.

## Reliability and distribution

Phase 10 separated root position/residual units and made adaptive Simpson globally
error-budgeted with bounded heap storage. These are reliability changes within
existing numerical methods. Sampling and error estimates remain heuristic; they
cannot certify arbitrary callbacks or discover every root.

Make and CMake compile the same production modules. GSL is optional benchmark-only.
CMake exports the relocatable NUMERIX::numerix target; only the public header,
static archive, package metadata and user/API/architecture documentation are
installed. Raw development results/history stay in the repository. Static build
success and source compatibility do not establish a stable shared-library ABI.
