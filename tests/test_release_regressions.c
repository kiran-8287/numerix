#include "numerix.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static double nonfinite(double x, void *data)
{
    (void)x;
    (void)data;
    return NAN;
}

static void test_optional_error_output(void)
{
    numerix_result result;
    numerix_error error;
    numerix_status with_error;
    numerix_status without_error;
    numerix_vector *vector = NULL;

    CHECK(numerix_error_set(NULL, NUMERIX_STATUS_SINGULAR_MATRIX, NULL) ==
          NUMERIX_STATUS_SINGULAR_MATRIX);
    with_error = numerix_root_bisection(nonfinite, NULL, 0.0, 1.0,
                                        NULL, &result, &error);
    without_error = numerix_root_bisection(nonfinite, NULL, 0.0, 1.0,
                                           NULL, &result, NULL);
    CHECK(with_error == NUMERIX_STATUS_NUMERICAL_FAILURE);
    CHECK(without_error == with_error);
    CHECK(result.status == without_error);
    CHECK(numerix_vector_create(0U, &vector, NULL) == NUMERIX_STATUS_INVALID_ARGUMENT);
    CHECK(vector == NULL);
}

static void test_null_lu_matrix(void)
{
    numerix_lu factorization;
    numerix_error error;

    memset(&factorization, 0, sizeof(factorization));
    CHECK(numerix_lu_decompose(NULL, NULL, &factorization, &error) ==
          NUMERIX_STATUS_INVALID_DIMENSIONS);
    CHECK(factorization.data == NULL);
    CHECK(factorization.permutation == NULL);
    CHECK(factorization.size == 0U);
    numerix_lu_destroy(&factorization);
    CHECK(numerix_lu_decompose(NULL, NULL, &factorization, NULL) ==
          NUMERIX_STATUS_INVALID_DIMENSIONS);
    numerix_lu_destroy(&factorization);
}

static void test_lu_output_and_permutation(void)
{
    numerix_lu factorization = {0};
    numerix_vector sentinel = {0};
    numerix_vector *solution = &sentinel;
    double entries[] = {2.0, 0.0, 0.0, 3.0};
    double right_entries[] = {2.0, 3.0};
    numerix_matrix matrix = {2U, 2U, entries};
    numerix_vector right = {2U, right_entries};
    numerix_error error;
    size_t saved_permutation;

    CHECK(numerix_lu_solve(NULL, &right, &solution, &error) ==
          NUMERIX_STATUS_INVALID_DIMENSIONS);
    CHECK(solution == NULL);
    CHECK(numerix_lu_decompose(&matrix, NULL, &factorization, &error) ==
          NUMERIX_STATUS_SUCCESS);
    if (factorization.data == NULL) {
        return;
    }
    saved_permutation = factorization.permutation[0];
    factorization.permutation[0] = 2U;
    CHECK(numerix_lu_solve(&factorization, &right, &solution, &error) ==
          NUMERIX_STATUS_INVALID_INPUT);
    CHECK(solution == NULL);
    factorization.permutation[0] = saved_permutation;
    CHECK(numerix_lu_solve(&factorization, &right, NULL, &error) ==
          NUMERIX_STATUS_INVALID_ARGUMENT);
    CHECK(numerix_lu_solve(&factorization, &right, &solution, &error) ==
          NUMERIX_STATUS_SUCCESS);
    if (solution != NULL) {
        CHECK(solution->data[0] == 1.0 && solution->data[1] == 1.0);
    }
    numerix_vector_destroy(solution);
    numerix_lu_destroy(&factorization);
}

int main(void)
{
    test_optional_error_output();
    test_null_lu_matrix();
    test_lu_output_and_permutation();
    if (failures != 0) {
        return 1;
    }
    puts("All release regression checks passed.");
    return 0;
}
