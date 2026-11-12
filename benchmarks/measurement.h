#ifndef NUMERIX_BENCHMARK_MEASUREMENT_H
#define NUMERIX_BENCHMARK_MEASUREMENT_H

#include <stddef.h>
#include <stdint.h>

typedef struct numerix_benchmark_precision {
    unsigned int storage_bits;
    int significand_bits;
    int min_exponent;
    int max_exponent;
} numerix_benchmark_precision;

typedef struct numerix_benchmark_configuration {
    const char *category;
    const char *case_id;
    const char *method;
    const char *phase;
    const char *comparison_mode;
    const char *reference_method;
    int variant;
    int difficulty;
    size_t size;
    double lower;
    double upper;
    double initial;
    double step;
    double scale;
    double absolute_tolerance;
    double relative_tolerance;
    double expected_value;
} numerix_benchmark_configuration;

typedef struct numerix_benchmark_statistics {
    double minimum;
    double median;
    double mean;
    double sample_standard_deviation;
    double percentile_95;
    double maximum;
    size_t sample_count;
    size_t batch_size;
} numerix_benchmark_statistics;

void numerix_benchmark_get_precision(numerix_benchmark_precision *precision);
int numerix_benchmark_numerix_runs_first(uint64_t seed, size_t pair_index);
uint64_t numerix_benchmark_configuration_id(
    const numerix_benchmark_configuration *configuration);
int numerix_benchmark_calculate_statistics(
    const double *samples, size_t sample_count, size_t batch_size,
    numerix_benchmark_statistics *statistics);

#endif
