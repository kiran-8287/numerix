# Linear algebra public API audit

Scope: declarations in `include/numerix.h`, implementations in
`src/algorithms/linear_algebra/linear_algebra.c`, and existing source test
references. This is a source audit; the test suite was not run as part of the
Phase 14C pilot.

The 21 grouped inventory rows cover all 23 current linear algebra public
symbols: 18 numerical functions and vector, matrix, and LU lifecycle APIs.
Every declared symbol in the inventory was found in the public header. The
pilot's `verification.json` also checks the inventory rows and public symbol
declarations. The implementation file is compact, with several functions on
one line; the references below point to those definitions.

| Public API | Implementation | Existing test reference |
|---|---|---|
| `numerix_vector_create` | `linear_algebra.c:15` | `test_phase1.c:38,43`; invalid-size case `test_release_regressions.c:40-41` |
| `numerix_vector_destroy` | `linear_algebra.c:16` | `test_phase1.c:43,45`; used for returned vectors throughout `test_phase13b_regressions.c` |
| `numerix_vector_copy` | `linear_algebra.c:17` | `test_phase1.c:43` |
| `numerix_vector_add` | `linear_algebra.c:18-19` | `test_phase1.c:43` |
| `numerix_vector_subtract` | `linear_algebra.c:18,20` | `test_phase1.c:43` |
| `numerix_vector_scale` | `linear_algebra.c:21` | `test_phase1.c:43` |
| `numerix_vector_dot` | `linear_algebra.c:22` | `test_phase1.c:43` |
| `numerix_vector_norm2` | `linear_algebra.c:23-49` | `test_phase1.c:43` |
| `numerix_vector_norm_inf` | `linear_algebra.c:50` | `test_phase1.c:43` |
| `numerix_matrix_create` | `linear_algebra.c:51` | `test_phase1.c:38,44` |
| `numerix_matrix_destroy` | `linear_algebra.c:52` | `test_phase1.c:45`; `test_phase1.c:44` cleanup |
| `numerix_matrix_copy` | `linear_algebra.c:53` | `test_phase1.c:42` |
| `numerix_matrix_add` | `linear_algebra.c:54-55` | `test_phase1.c:42` |
| `numerix_matrix_subtract` | `linear_algebra.c:54,56` | `test_phase1.c:42` |
| `numerix_matrix_scale` | `linear_algebra.c:57` | `test_phase1.c:42` |
| `numerix_matrix_multiply` | `linear_algebra.c:58` | `test_phase1.c:42` |
| `numerix_matrix_transpose` | `linear_algebra.c:59` | `test_phase1.c:42` |
| `numerix_matrix_vector_multiply` | `linear_algebra.c:60` | `test_phase1.c:42` |
| `numerix_linear_solve_gaussian` | `linear_algebra.c:249` | `test_phase1.c:39,44`; scaled, singular, and near-singular cases in `test_phase13b_regressions.c:151-159,192-237` |
| `numerix_linear_solve_gauss_jordan` | `linear_algebra.c:250-407` | `test_phase1.c:40,44`; scaled and pivot cases in `test_phase13b_regressions.c:163-171,204-253` |
| `numerix_lu_decompose` | `linear_algebra.c:185` (factor routine at `:62-183`) | `test_phase1.c:41`; overflow cleanup `test_phase13b_regressions.c:256-270`; allocation failures `test_allocation_failures.c:162-195` |
| `numerix_lu_destroy` | `linear_algebra.c:184` | `test_phase1.c:41`; release/null and allocation cleanup `test_release_regressions.c:44-58,95`; `test_allocation_failures.c:175-195` |
| `numerix_lu_solve` | `linear_algebra.c:186-247` | `test_phase1.c:41`; invalid factors/permutation and solution cases `test_release_regressions.c:61-95` |

## Method notes and limitations

- Vector/matrix arithmetic returns newly allocated outputs; end-to-end wrappers
  in the pilot include output allocation and release on both implementations.
- `numerix_vector_dot` uses compensated accumulation; `DDOT` is therefore a
  best-available comparison. `numerix_vector_norm2` uses scaled sum of squares;
  `DNRM2` is the corresponding competitor route.
- `numerix_matrix_multiply` is a scalar `i-k-j` loop; the pilot pairs it with
  row-major CBLAS DGEMM. The pilot exercises 64×64 square GEMM only.
- NUMERIX LU uses scaled partial pivoting. LAPACK DGETRF uses partial pivoting
  and 1-based pivots, so LU factorization is labeled best available. The pilot
  converts row-major data to column-major inside the timed LAPACK route and
  checks the factorization identity.
- LU solve uses each implementation's own saved factors and times solution
  allocation. One-time factorization and conversion setup are recorded
  separately. Only one right-hand side is in the pilot.
- The pilot measures no vector methods, transpose, GEMV, Gaussian elimination,
  Gauss-Jordan, lifecycle operation, or allocator totals. All pilot memory rows
  are explicitly unavailable; no memory value is implied to be zero.
- Existing tests provide source-level coverage references, but this audit does
  not establish that tests were executed in this task.
