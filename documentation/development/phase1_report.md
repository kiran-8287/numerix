# Phase 1 report

## Implemented

Phase 1 adds the following algorithms to the Phase 0 C11 library:

- **Root finding:** bisection, false position with stagnation bisection, secant, Newton-Raphson, bracketed safeguarded Newton, and a conservative Brent-style combination of inverse quadratic interpolation, secant, and bisection.
- **Linear algebra:** dynamic dense vectors and row-major matrices; vector/matrix copy, arithmetic, products, norms and transpose; partial-pivoted Gaussian solve, Gauss-Jordan solve, reusable packed LU decomposition and solves, and infinity-norm residuals.
- **Differentiation:** forward, backward, central, five-point central first derivatives, Richardson extrapolation, and central second derivative.
- **Interpolation:** two-point linear interpolation, direct Lagrange evaluation, and reusable Newton divided differences with nested polynomial evaluation. Extrapolation is opt-in for polynomial methods.
- **Integration:** composite trapezoid, composite Simpson 1/3, and adaptive Simpson with depth, iteration, and evaluation limits.
- **Optimization:** bounded golden-section minimization and multidimensional gradient descent with Armijo backtracking.
- **ODE:** vector-system fixed-step Euler, Heun, and classical RK4 with owned solution histories.

## Public API

The new declarations are in `include/numerix.h`: `numerix_root_*`; `numerix_vector_*`, `numerix_matrix_*`, `numerix_linear_solve_*`, and `numerix_lu_*`; `numerix_diff_*`; `numerix_interp_*`; `numerix_integrate_*`; `numerix_optimize_*`; and `numerix_ode_*`. Dynamic results have matching destroy functions. The Phase 0 scalar callback, options, statuses, errors, context, CLI, and tests remain in place.

## Implementation files

- `src/algorithms/root_finding/root_finding.c`
- `src/algorithms/linear_algebra/linear_algebra.c`
- `src/algorithms/differentiation/differentiation.c`
- `src/algorithms/interpolation/interpolation.c`
- `src/algorithms/integration/integration.c`
- `src/algorithms/optimization/optimization.c`
- `src/algorithms/ode/ode.c`
- `tests/test_phase1.c`
- Seven `documentation/api/*.md` guides

CMake and Make include every algorithm module. The Phase 1 test executable can run all assertions or an individual named category. CTest registers each category independently.

## Verification performed

Environment: Windows, MinGW GCC 6.3.0. The build used `-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow`.

- `mingw32-make clean` — passed.
- `mingw32-make all` — passed; built the static archive and CLI without compiler warnings.
- `mingw32-make test` — passed Phase 0 unit and integration runs, and all seven Phase 1 categories (`root_finding`, `linear_algebra`, `differentiation`, `interpolation`, `integration`, `optimization`, `ode`).
- `.\numerix_cli.exe` — passed; printed version 0.1.0 and successful initialization.
- Tests compare against analytical values: roots of `x²-2`, pivoted known linear system, polynomial/sine derivatives, known interpolating polynomial, exact polynomial and sine integrals, known bounded/vector minima, and the exponential ODE solution. The test source records the tolerances used.
- Address/undefined sanitizer build was attempted with `-fsanitize=address,undefined`; MinGW could not link because `-lasan` and `-lubsan` runtimes are absent.
- CMake and CTest are not installed in this environment; their configured build/test paths were not run. Linux and Clang were not available for verification.

## Limitations

The Brent-style method is a conservative implementation and is not asserted to be byte-for-byte or behaviorally identical to a reference package. False-position adds a midpoint safeguard against stagnation. Gradient descent is local and uses a fixed initial step with Armijo backtracking. Polynomial interpolation may be ill-conditioned at high degree. Fixed quadrature does not estimate error (`estimated_error` is `NAN`); adaptive Simpson's estimate is heuristic. ODE methods are fixed step and explicit. Linear pivot tests are scale-relative but do not estimate condition numbers. Sanitizer and Linux validation remain outstanding.

Expression parsing, piecewise functions, intelligent method selection, fallback, advanced diagnostics, adaptive ODE integration, and other Phase 2 features remain deferred.
