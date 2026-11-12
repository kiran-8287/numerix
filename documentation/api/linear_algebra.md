# Linear algebra

`numerix_vector` stores `size` doubles. `numerix_matrix` stores a dense row-major `rows × columns` array (`data[row * columns + column]`). Create functions allocate zero-filled data; destroy functions accept `NULL`. Arithmetic functions return newly allocated outputs owned by the caller. Inputs are not modified. All dimensions must be positive and compatible, and inputs must contain finite values.

Vector operations include create/destroy/copy, add, subtract, scalar multiply, dot product, Euclidean norm, and infinity norm. Matrix operations include create/destroy/copy, add, subtract, scalar multiply, matrix product, transpose, and matrix-vector product.

`numerix_linear_solve_gaussian` uses scaled partial-pivoted LU factorization and substitution; `numerix_linear_solve_gauss_jordan` applies the same pivot policy while normalizing each row and eliminating all other rows. Both solve square `Ax=b`, return a newly allocated vector, and optionally write the infinity norm of `Ax-b`.

The pivot policy scales each row by its largest original absolute matrix entry. At each column the solver selects the remaining row with the largest ratio `abs(pivot)/row_scale`; it treats a ratio at or below `n * DBL_EPSILON` as numerically singular. This makes the decision insensitive to a common scaling of the matrix and avoids an absolute `DBL_MIN` floor. Zero rows and exactly singular pivots are rejected. Subnormal nonzero pivots are retained when their relative row quality is usable; the relative ratio can underflow to zero when a pivot is negligible compared with its row scale, in which case rejection is intentional. `numerix_lu.threshold` reports this relative cutoff.

This is a practical scaled-pivot heuristic, not a condition-number estimate or a forward-error guarantee. Strong element growth, cancellation, and ill-conditioned systems can still produce large solution error. The solver does not use `absolute_tolerance` as a pivot cutoff. LU checks generated multipliers and updated factors for non-finite values; an overflowed factorization returns `NUMERIX_STATUS_NUMERICAL_FAILURE` and clears the output structure.

`numerix_lu_decompose` fills a caller-declared `numerix_lu`. Its packed `data` contains unit-lower multipliers below the diagonal and upper factors on/above it. `permutation[i]` gives the original row now at row `i`; `sign` is the row permutation parity. `numerix_lu_solve` applies that permutation and supports repeated right-hand sides. Release the decomposition once with `numerix_lu_destroy`.

Allocation/dimension errors, incompatible shapes, singular pivots, and non-finite arithmetic return explicit statuses. Pivot checks detect numerical singularity at the scale of the input matrix; these solve calls do not return a condition number or forward-error estimate. The separate diagnostics API can estimate conditioning and compare a solution with a known reference.

```c
numerix_vector *x = NULL;
double residual;
numerix_status s = numerix_linear_solve_gaussian(A, b, NULL,
                                                 &x, &residual, &error);
numerix_vector_destroy(x);
```
