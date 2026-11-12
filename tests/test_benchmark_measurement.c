#include "measurement.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void test_precision_metadata(void)
{
    numerix_benchmark_precision precision;

    numerix_benchmark_get_precision(&precision);
    CHECK(precision.storage_bits == sizeof(double) * CHAR_BIT);
    CHECK(precision.significand_bits == DBL_MANT_DIG);
    CHECK(precision.min_exponent == DBL_MIN_EXP);
    CHECK(precision.max_exponent == DBL_MAX_EXP);
}

static void test_counterbalanced_order(void)
{
    size_t index;
    size_t seed_a_numerix_first = 0U;
    size_t seed_a_gsl_first = 0U;
    size_t seed_b_numerix_first = 0U;
    int seed_a_schedule[12];
    uint64_t seed_a = UINT64_C(42);
    uint64_t seed_b = UINT64_C(43);

    for (index = 0; index < 12U; ++index) {
        int first_a = numerix_benchmark_numerix_runs_first(seed_a, index);

        seed_a_schedule[index] = first_a;
        seed_a_numerix_first += first_a != 0 ? 1U : 0U;
        seed_a_gsl_first += first_a == 0 ? 1U : 0U;
        seed_b_numerix_first +=
            numerix_benchmark_numerix_runs_first(seed_b, index) != 0 ? 1U : 0U;
        CHECK(first_a == numerix_benchmark_numerix_runs_first(seed_a, index));
        CHECK(numerix_benchmark_numerix_runs_first(seed_a, index) !=
              numerix_benchmark_numerix_runs_first(seed_b, index));
    }
    CHECK(seed_a_numerix_first == 6U);
    CHECK(seed_a_gsl_first == 6U);
    CHECK(seed_b_numerix_first == 6U);
    for (index = 0; index < 12U; index += 2U) {
        CHECK(seed_a_schedule[index] != seed_a_schedule[index + 1U]);
    }
}

static void test_configuration_identity(void)
{
    numerix_benchmark_configuration configuration = {
        "root", "polynomial", "bisection", "execute", "same_algorithm",
        "analytic_root", 0, 1, 0U, 0.0, 2.0, 1.0, 0.0, 1.0,
        1e-10, 1e-10, 1.4142135623730951
    };
    uint64_t identifier = numerix_benchmark_configuration_id(&configuration);

    CHECK(identifier != 0U);
    CHECK(identifier == numerix_benchmark_configuration_id(&configuration));
    configuration.absolute_tolerance *= 10.0;
    CHECK(identifier != numerix_benchmark_configuration_id(&configuration));
}

static void test_statistics_from_raw_samples(void)
{
    double samples[] = {5.0, 1.0, 4.0, 2.0, 3.0};
    double even_samples[] = {4.0, 1.0, 3.0, 2.0};
    double large_samples[] = {0.0, DBL_MAX};
    double invalid_samples[] = {1.0, NAN, 3.0};
    numerix_benchmark_statistics statistics;

    CHECK(numerix_benchmark_calculate_statistics(samples, 5U, 10U,
                                                 &statistics));
    CHECK(statistics.minimum == 1.0);
    CHECK(statistics.median == 3.0);
    CHECK(statistics.mean == 3.0);
    CHECK(fabs(statistics.sample_standard_deviation - sqrt(2.5)) < 1e-14);
    CHECK(statistics.percentile_95 == 5.0);
    CHECK(statistics.maximum == 5.0);
    CHECK(statistics.sample_count == 5U);
    CHECK(statistics.batch_size == 10U);
    CHECK(numerix_benchmark_calculate_statistics(even_samples, 4U, 7U,
                                                 &statistics));
    CHECK(statistics.median == 2.5);
    CHECK(numerix_benchmark_calculate_statistics(large_samples, 2U, 1U,
                                                 &statistics));
    CHECK(isfinite(statistics.mean));
    CHECK(isfinite(statistics.sample_standard_deviation));
    CHECK(fabs(statistics.mean / (DBL_MAX / 2.0) - 1.0) <=
          4.0 * DBL_EPSILON);
    CHECK(fabs(statistics.sample_standard_deviation /
               (DBL_MAX / sqrt(2.0)) - 1.0) <= 4.0 * DBL_EPSILON);
    CHECK(!numerix_benchmark_calculate_statistics(invalid_samples, 3U, 1U,
                                                  &statistics));
}

int main(void)
{
    test_precision_metadata();
    test_counterbalanced_order();
    test_configuration_identity();
    test_statistics_from_raw_samples();
    if (failures != 0) {
        return 1;
    }
    puts("All benchmark measurement checks passed.");
    return 0;
}
