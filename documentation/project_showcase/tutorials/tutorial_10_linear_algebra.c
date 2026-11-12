#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Solve:
 *   2x + y = 5
 *   x  + 3y = 6
 * Expected: x = 1.8, y = 1.4
 */

int main(void)
{
    numerix_matrix *a = NULL;
    numerix_vector *b = NULL;
    numerix_vector *x = NULL;
    numerix_error error;
    numerix_status status;
    double residual = 0.0;
    size_t i;

    /* Create the 2x2 matrix. */
    status = numerix_matrix_create(2, 2, &a, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "matrix create failed: %s\n", error.message);
        return 1;
    }
    a->data[0] = 2.0; a->data[1] = 1.0;   /* row 0: 2x +  y */
    a->data[2] = 1.0; a->data[3] = 3.0;   /* row 1:  x + 3y */

    /* Create the right-hand-side vector. */
    status = numerix_vector_create(2, &b, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "vector create failed: %s\n", error.message);
        numerix_matrix_destroy(a);
        return 1;
    }
    b->data[0] = 5.0;
    b->data[1] = 6.0;

    /* Call Gaussian elimination. */
    status = numerix_linear_solve_gaussian(a, b, NULL, &x, &residual, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "linear solve failed: %s\n", error.message);
        numerix_vector_destroy(b);
        numerix_matrix_destroy(a);
        return 1;
    }

    printf("Solution:\n");
    for (i = 0; i < x->size; ++i)
        printf("  x[%zu] = %.10f\n", i, x->data[i]);
    printf("Expected: x[0] = 1.8, x[1] = 1.4\n");
    if (residual != 0.0 || isfinite(residual))
        printf("Residual: %.3e\n", residual);

    /* Save values before destroying (avoid use-after-free). */
    double x0 = x->data[0];
    double x1 = x->data[1];

    /* Destroy all allocated objects on the success path. */
    numerix_vector_destroy(x);
    numerix_vector_destroy(b);
    numerix_matrix_destroy(a);

    /* Verify: x=1.8, y=1.4 → 2(1.8)+1.4=5.0, 1.8+3(1.4)=6.0 ✓ */
    return (fabs(x0 - 1.8) < 1e-6 && fabs(x1 - 1.4) < 1e-6) ? 0 : 1;
}
