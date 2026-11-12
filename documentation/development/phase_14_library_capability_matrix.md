# Phase 14 library capability matrix

This matrix is based on upstream documentation and the pinned source trees under the user-local Phase 14 dependency workspace. “Build status” is updated after each local source build and native-API smoke test; source availability alone is not counted as a completed comparison.

| Library/configuration | Language and API used | Module coverage | Candidate algorithms/API | Pinned version | License | Build status | Comparison mode |
|---|---|---|---|---|---|---|---|
| NUMERIX | C public API | All seven | Current public algorithms in `include/numerix.h` | 0.2.0-rc1 | Project license | Existing local build | Reference implementation |
| GNU Scientific Library (GSL) | Native C API | Root, linear algebra, differentiation, interpolation, integration, minimization, ODE | Brent and Newton roots; LU solve; first derivative; polynomial interpolation; QAG; golden-section minimization; fixed RK4 and adaptive ODE stepping | 2.8 | GPL-3.0-or-later | Existing optimized user-local build; representative native C smoke passed for all seven modules | Equivalent for Brent/Newton roots and fixed RK4; best-available for other measured methods |
| Reference LAPACK + reference BLAS + LAPACKE | Fortran implementation with native C LAPACKE/CBLAS interfaces | Linear algebra | Measured path: row-major `LAPACKE_dgesv`; factorization-only and solve-only paths are not yet collected | 3.12.1 | Modified BSD | Built and installed; representative `dgesv` native smoke passed | Best-available combined dense solve; does not claim LU factorization timing |
| OpenBLAS with bundled LAPACK/LAPACKE | Fortran/C implementation with native C LAPACKE/CBLAS interfaces | Linear algebra | Measured path: row-major `LAPACKE_dgesv` backed by OpenBLAS's LAPACK/BLAS; Haswell AVX2; one runtime thread | 0.3.34 | BSD-3-Clause | Corrected all-precision build installed; representative `dgesv` native smoke passed | Best-available combined dense solve; row-major conversion is included |
| NLopt | Native C API (implementation includes C/C++ components) | Optimization | Bounded one-dimensional `NLOPT_GN_DIRECT_L` | 2.11.0 | LGPL-2.1-or-later with component-specific notices | Built and installed; representative native C smoke passed | Best-available; no golden-section equivalent selected |
| SUNDIALS | Native C API | ODE | CVODE Adams with serial fixed-point nonlinear solver; ARKODE fixed-step explicit Kutta-RK4a; serial vectors | 7.9.0 | BSD-3-Clause | Built and installed; representative CVODE and ARKODE native smoke passed | Fixed-step RK4 is algorithm-equivalent; CVODE is an adaptive best-available comparison |
| CMINPACK | Native C API | Root finding | MINPACK `hybrd1`, one-variable nonlinear-system formulation | 1.3.15 | Modified BSD-style five-condition notice | Built and installed as double-precision static library; representative native C smoke passed | Best-available local nonlinear-system method; not equivalent to bracketed methods |

## Method and scope notes

- GSL is the only broad native-C library in this study with directly applicable APIs across all seven NUMERIX modules. Its module APIs and method availability are documented in the [GSL reference manual](https://www.gnu.org/software/gsl/doc/html/).
- LAPACKE is the C interface to LAPACK. The reference configuration uses the Netlib reference BLAS; the optimized configuration uses OpenBLAS's BLAS and bundled LAPACK/LAPACKE sources. The labels distinguish these implementations. LAPACK upstream documents its C interfaces and [3.12.1 release](https://github.com/Reference-LAPACK/lapack/releases).
- OpenBLAS is not described as “OpenBLAS alone” for LAPACK routines. This build includes its bundled LAPACK and LAPACKE implementation. The [OpenBLAS upstream project](https://github.com/OpenMathLib/OpenBLAS) is the source of the pinned implementation.
- The current adapters measure global Newton interpolation against GSL polynomial interpolation, and adaptive Simpson against GSL QAG. Both are best-available comparisons.
- NLopt has no selected algorithm claimed to be golden-section equivalent. Its bounded one-dimensional solvers are compared as best-available methods using the same objective, bounds, reference minimum, and accuracy target.
- SUNDIALS CVODE uses the Adams method with a serial fixed-point nonlinear solver. Adaptive CVODE results are not labelled equivalent to NUMERIX fixed-step RK4. ARKODE's fixed RK comparison uses the same verified tableau and step policy.
- SUNDIALS ARKODE `Kutta-RK4a` was source-checked: its four stages use `1/2`, `1/2`, and `1` stage locations, with weights `1/6, 1/3, 1/3, 1/6`, matching the classical NUMERIX RK4 tableau. The ARKODE adapter will hold the same fixed step and interval.
- CMINPACK's scalar use is the one-variable nonlinear-system case. It has no sign-bracket guarantee and is reported separately from bisection/Brent comparisons.

## Additional candidates reviewed

| Candidate | Decision | Reason |
|---|---|---|
| ALGLIB | Excluded from the native-C benchmark | Upstream lists C++, C#, Java, Python, and Delphi interfaces, but no documented native C interface for the compared routines. Its generated C-family sources do not by themselves establish a supported C ABI. See the [official ALGLIB language overview](https://www.alglib.net/). |
| Boost.Math | Excluded | The upstream library is a C++14 library; calling it would add a C++ wrapper/runtime path to the native-C comparison. See [Boost.Math](https://www.boost.org/library/master/math/). |
| Cuba | Excluded from the one-dimensional finite-interval quadrature comparison | Cuba is designed for multidimensional integration; that is a different workload than NUMERIX's current finite-interval one-dimensional quadrature. See the [Cuba paper and scope](https://arxiv.org/abs/hep-ph/0404043). |
| Cephes | Excluded as a direct module competitor | Cephes provides mathematical and special-function routines, with some additional linear algebra and ODE components, but not a comparable set of the seven current NUMERIX module interfaces. See the [Netlib Cephes index](https://netlib.org/cephes/). |

There is no second specialist native-C library selected for differentiation, interpolation, or finite one-dimensional quadrature. For these modules, GSL remains the additional broad C-library comparator; the report will identify the narrower competitive landscape instead of adding a language-mismatched library.

## Pinned source revisions

| Source tree | Tag | Commit |
|---|---|---|
| Reference LAPACK | `v3.12.1` | `6ec7f2bc4ecf4c4a93496aa2fa519575bc0e39ca` |
| OpenBLAS | `v0.3.34` | `e0166008be8e466242aa76b2ff75ce3f0fbf574a` |
| NLopt | `v2.11.0` | `88c424d4f458412787df96fcc95218acbca224fd` |
| SUNDIALS | `v7.9.0` | `312fc0f3684f27209ca9dc9249d194436eb41a7a` |
| CMINPACK | `v1.3.15` | `64c5635addd72fb54f44db8546c191feeb70e338` |
