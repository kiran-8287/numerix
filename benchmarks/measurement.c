#include "measurement.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void numerix_benchmark_get_precision(numerix_benchmark_precision *precision)
{
    if (precision != NULL) {
        precision->storage_bits = (unsigned int)(sizeof(double) * CHAR_BIT);
        precision->significand_bits = DBL_MANT_DIG;
        precision->min_exponent = DBL_MIN_EXP;
        precision->max_exponent = DBL_MAX_EXP;
    }
}

int numerix_benchmark_numerix_runs_first(uint64_t seed, size_t pair_index)
{
    /* Alternate within adjacent pairs; the seed selects each block's start. */
    size_t block = pair_index / 2U;
    unsigned int initial_orientation = (unsigned int)(seed & UINT64_C(1));
    unsigned int orientation = initial_orientation ^ (unsigned int)(block & 1U);
    unsigned int position = (unsigned int)(pair_index & 1U);

    return (orientation ^ position) == 0U;
}

uint64_t numerix_benchmark_configuration_id(
    const numerix_benchmark_configuration *configuration)
{
    char serialized[2048];
    int length;
    size_t index;
    uint64_t hash = UINT64_C(14695981039346656037);

    if (configuration == NULL || configuration->category == NULL ||
        configuration->case_id == NULL || configuration->method == NULL ||
        configuration->phase == NULL || configuration->comparison_mode == NULL ||
        configuration->reference_method == NULL) {
        return 0U;
    }
    length = snprintf(serialized, sizeof(serialized),
        "category=%s|case=%s|variant=%d|difficulty=%d|size=%zu|"
        "lower=%a|upper=%a|initial=%a|step=%a|scale=%a|"
        "absolute_tolerance=%a|relative_tolerance=%a|expected=%a|"
        "reference_method=%s|method=%s|phase=%s|comparison=%s",
        configuration->category, configuration->case_id,
        configuration->variant, configuration->difficulty, configuration->size,
        configuration->lower, configuration->upper, configuration->initial,
        configuration->step, configuration->scale,
        configuration->absolute_tolerance, configuration->relative_tolerance,
        configuration->expected_value, configuration->reference_method,
        configuration->method, configuration->phase,
        configuration->comparison_mode);
    if (length < 0 || (size_t)length >= sizeof(serialized)) {
        return 0U;
    }
    for (index = 0; index < (size_t)length; ++index) {
        hash ^= (unsigned char)serialized[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash == 0U ? 1U : hash;
}

static int compare_double(const void *left, const void *right)
{
    double a = *(const double *)left;
    double b = *(const double *)right;

    return (a > b) - (a < b);
}

int numerix_benchmark_calculate_statistics(
    const double *samples, size_t sample_count, size_t batch_size,
    numerix_benchmark_statistics *statistics)
{
    double *ordered;
    double mean = 0.0;
    double sum_squares = 0.0;
    double scale = 0.0;
    size_t index;
    size_t percentile_index;

    if (samples == NULL || statistics == NULL || sample_count < 2U ||
        batch_size == 0U || sample_count > SIZE_MAX / sizeof(double)) {
        return 0;
    }
    ordered = malloc(sample_count * sizeof(double));
    if (ordered == NULL) {
        return 0;
    }
    memcpy(ordered, samples, sample_count * sizeof(double));
    for (index = 0; index < sample_count; ++index) {
        if (!isfinite(samples[index]) || samples[index] < 0.0) {
            free(ordered);
            return 0;
        }
        if (samples[index] > scale) {
            scale = samples[index];
        }
    }
    if (scale != 0.0) {
        for (index = 0; index < sample_count; ++index) {
            double normalized = samples[index] / scale;
            double difference = normalized - mean;

            mean += difference / (double)(index + 1U);
            sum_squares += difference * (normalized - mean);
        }
        mean *= scale;
        sum_squares = sqrt(sum_squares / (double)(sample_count - 1U)) * scale;
        if (!isfinite(mean) || !isfinite(sum_squares)) {
            free(ordered);
            return 0;
        }
    }
    qsort(ordered, sample_count, sizeof(double), compare_double);
    percentile_index = (size_t)ceil(0.95 * (double)sample_count);
    if (percentile_index == 0U) {
        percentile_index = 1U;
    }
    --percentile_index;
    statistics->minimum = ordered[0];
    if (sample_count % 2U == 0U) {
        size_t upper_index = sample_count / 2U;
        size_t lower_index = upper_index - 1U;

        statistics->median = ordered[lower_index] / 2.0 +
                             ordered[upper_index] / 2.0;
    } else {
        statistics->median = ordered[sample_count / 2U];
    }
    statistics->mean = mean;
    statistics->sample_standard_deviation = sum_squares;
    statistics->percentile_95 = ordered[percentile_index];
    statistics->maximum = ordered[sample_count - 1U];
    statistics->sample_count = sample_count;
    statistics->batch_size = batch_size;
    free(ordered);
    return 1;
}
