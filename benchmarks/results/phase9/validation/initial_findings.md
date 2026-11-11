# Findings before final validation

The initial repository was clean at 29e2f7ac536e0d71f38a0e59ad148556b1b0c84f,
matching origin/master. The baseline GCC 8.5 Make-O2 suite and CMake Debug
17-test suite passed. Earlier Windows results are documented history, not
independently rerun results here.

A tiny caller linked against the untouched baseline CMake archive reproduced:

- `numerix_lu_decompose(NULL,NULL,&lu,&error)`: process returncode -11 (SIGSEGV).
- Bisection on a nonfinite callback with optional error=NULL: status 1
  (INVALID_ARGUMENT), instead of 5 (NUMERICAL_FAILURE).

Focused release tests now cover these cases and LU output/permutation validation.
Initial -O3/-Werror checks exposed ambiguous external status-return paths in root
and differentiation evaluation helpers. Explicit known status returns resolve
these without changing solver arithmetic. Clang-Werror exposed an implicit
floating-to-bool test in the vector norm, changed to explicit `value != 0.0`.

Static analysis also identified uninitialized failure-output evidence and an
unused optimizer assignment. The implementation initializes root attempt outputs
and boundary confidence, makes error helpers' known return statuses explicit,
and removes that assignment. No solver family was replaced or tuned for speed.

Initial validation-runner mistakes (wrong Valgrind build-directory spelling) were
corrected; final logs describe actual completed runs. Analyzer exit0 alone is not
reported as a warning-free pass. Remaining analyzer findings are classified in
static_analysis_review.md.
