#include <cblas.h>
#include <math.h>
#include <stdio.h>

extern void dgetrf_(const int *m, const int *n, double *a, const int *lda,
                    int *ipiv, int *info);
extern void dgetrs_(const char *trans, const int *n, const int *nrhs,
                    const double *a, const int *lda, const int *ipiv,
                    double *b, const int *ldb, int *info);
extern char *openblas_get_config(void);
extern int openblas_get_num_threads(void);

int main(void)
{
    const double left[] = {1.0, 2.0, 3.0, 4.0};
    const double right[] = {5.0, 6.0, 7.0, 8.0};
    double product[4] = {0.0, 0.0, 0.0, 0.0};
    const int m = 2, n = 2, k = 2, lda = 2;
    int pivots[2] = {0, 0};
    int info = -1;
    int nrhs = 1;
    char trans = 'N';
    /* Column-major storage for the row-major mathematical matrix [[0,2],[1,3]]. */
    double factors[4] = {0.0, 1.0, 2.0, 3.0};
    double solution[2] = {4.0, 7.0};

    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k,
                1.0, left, k, right, n, 0.0, product, n);
    if (fabs(product[0] - 19.0) > 1e-12 ||
        fabs(product[1] - 22.0) > 1e-12 ||
        fabs(product[2] - 43.0) > 1e-12 ||
        fabs(product[3] - 50.0) > 1e-12) {
        fputs("CBLAS row-major DGEMM probe failed\n", stderr);
        return 1;
    }

    dgetrf_(&n, &n, factors, &lda, pivots, &info);
    if (info != 0) {
        fprintf(stderr, "LAPACK DGETRF probe failed: info=%d\n", info);
        return 2;
    }
    dgetrs_(&trans, &n, &nrhs, factors, &lda, pivots, solution, &n, &info);
    if (info != 0 || fabs(solution[0] - 1.0) > 1e-12 ||
        fabs(solution[1] - 2.0) > 1e-12) {
        fprintf(stderr, "LAPACK DGETRS probe failed: info=%d x=(%.17g,%.17g)\n",
                info, solution[0], solution[1]);
        return 3;
    }

    printf("%s\nOpenBLAS threads=%d\nCBLAS DGEMM row-major=PASS\n"
           "LAPACK DGETRF/DGETRS column-major=PASS x=(%.1f,%.1f)\n",
           openblas_get_config(), openblas_get_num_threads(), solution[0], solution[1]);
    return 0;
}
