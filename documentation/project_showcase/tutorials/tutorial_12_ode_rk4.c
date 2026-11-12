#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* y1' = y2, y2' = -y1  (harmonic oscillator) */
static numerix_status oscillator(double x, const double *y, size_t dimension,
                                  double *dydx, void *user_data)
{
    (void)x;
    (void)user_data;
    if (dimension < 2)
        return NUMERIX_STATUS_INVALID_DIMENSIONS;
    dydx[0] = y[1];
    dydx[1] = -y[0];
    return NUMERIX_STATUS_SUCCESS;
}

int main(void)
{
    numerix_ode_solution solution;
    numerix_error error;
    numerix_status status;
    double y0[2] = {1.0, 0.0};
    double step = 0.1;
    size_t steps = 63;  /* reaches x ≈ 6.3 ≈ 2*pi */
    size_t last;

    memset(&solution, 0, sizeof(solution));

    status = numerix_ode_rk4(
        oscillator, NULL, 0.0, y0, 2, step, steps, &solution, &error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        fprintf(stderr, "RK4 failed: %s\n", error.message);
        return 1;
    }

    last = solution.points - 1;
    printf("Final time:     %.6f\n", solution.x[last]);
    printf("Final y[0]:     %.6f  (expected cos(6.3) ≈ 0.999859)\n", solution.y[2 * last]);
    printf("Final y[1]:     %.6f  (expected -sin(6.3) ≈ -0.016814)\n", solution.y[2 * last + 1]);
    printf("Function evaluations: %zu\n", solution.function_evaluations);

    /* Analytical check: y1(2π) = cos(2π) = 1, y2(2π) = -sin(2π) = 0.
       We stop at 6.3, slightly before 2π ≈ 6.2832. */
    printf("Analytical at x=6.3: y1=cos(6.3)≈%.6f, y2=-sin(6.3)≈%.6f\n",
           cos(6.3), -sin(6.3));

    numerix_ode_solution_destroy(&solution);

    return 0;
}
