# Phase 14B per-library method capability and measurement matrix

The machine-readable matrix has one row per verified NUMERIX public method and candidate library. Capability and measurement status are separate: API availability does not mean a Phase 14B measurement exists. Direct algorithm comparisons and best-available routes remain distinct. The Phase 14B root runner measured NUMERIX, GSL, and CMINPACK where applicable; the other six modules have not yet been benchmarked in this execution.

The mapping follows the pinned Phase 14 capability audit in [phase_14_library_capability_matrix.md](phase_14_library_capability_matrix.md), native adapter APIs, and the NUMERIX public-method inventory. GSL Heun/RK2 equivalence is deliberately left approximate because the tableau equivalence has not been verified.

| Module | Method | NUMERIX | GSL | CMINPACK | LAPACK reference | LAPACK + OpenBLAS | NLopt | ARKODE | CVODE |
|---|---|---|---|---|---|---|---|---|---|
| root | Bisection | direct equivalent | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Regula Falsi | direct equivalent | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Secant | direct equivalent | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Newton-Raphson | direct equivalent | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Safeguarded Newton | direct equivalent | best available | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Brent | direct equivalent | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Automatic method selection | direct equivalent | not supported | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Solver portfolio | direct equivalent | not supported | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| root | Multiple-root discovery | direct equivalent | best available | not supported | not applicable | not applicable | not applicable | not applicable | not applicable |
| linear_algebra | Vector lifecycle | direct equivalent | available supporting api | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Vector copy | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Vector addition | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Vector subtraction | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Vector scaling | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| linear_algebra | Vector dot product | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| linear_algebra | Vector 2-norm | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| linear_algebra | Vector infinity norm | direct equivalent | approximate wrapper | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix lifecycle | direct equivalent | available supporting api | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix copy | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix addition | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix subtraction | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix scaling | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix multiplication | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| linear_algebra | Matrix transpose | direct equivalent | direct equivalent | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | Matrix-vector multiply | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| linear_algebra | Gaussian elimination solve | direct equivalent | best available | not applicable | best available | best available | not applicable | not applicable | not applicable |
| linear_algebra | Gauss-Jordan solve | direct equivalent | not supported | not applicable | not supported | not supported | not applicable | not applicable | not applicable |
| linear_algebra | LU factorization | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| linear_algebra | LU solve | direct equivalent | direct equivalent | not applicable | direct equivalent | direct equivalent | not applicable | not applicable | not applicable |
| differentiation | Forward difference | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| differentiation | Backward difference | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| differentiation | Central difference | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| differentiation | Five-point difference | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| differentiation | Richardson extrapolation | direct equivalent | approximate wrapper | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| differentiation | Second derivative | direct equivalent | approximate wrapper | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| differentiation | Piecewise central difference | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| interpolation | Linear interpolation | direct equivalent | direct equivalent | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| interpolation | Lagrange interpolation | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| interpolation | Newton construction | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| interpolation | Newton evaluation | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| integration | Composite trapezoidal rule | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| integration | Composite Simpson 1/3 rule | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| integration | Adaptive Simpson | direct equivalent | best available | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| integration | Piecewise trapezoidal integration | direct equivalent | not supported | not applicable | not applicable | not applicable | not applicable | not applicable | not applicable |
| optimization | Golden-section bounded minimization | direct equivalent | best available | not applicable | not applicable | not applicable | best available | not applicable | not applicable |
| optimization | Gradient descent | direct equivalent | best available | not applicable | not applicable | not applicable | best available | not applicable | not applicable |
| ode | Euler fixed step | direct equivalent | direct equivalent | not applicable | not applicable | not applicable | not applicable | not supported | best available |
| ode | Heun fixed step | direct equivalent | approximate equivalent | not applicable | not applicable | not applicable | not applicable | not supported | best available |
| ode | Classical RK4 fixed step | direct equivalent | direct equivalent | not applicable | not applicable | not applicable | not applicable | direct equivalent | best available |
