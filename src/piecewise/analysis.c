#include "piecewise_internal.h"

#include <float.h>
#include <math.h>

static numerix_status diagnostic_error(
    numerix_error *error, numerix_status status, const char *message)
{
    numerix_error_set(error, status, message);
    return status;
}

static numerix_status normalized_options(
    const numerix_options *input, numerix_options *options,
    numerix_error *error)
{
    if (input == NULL) {
        numerix_options_init(options);
        return NUMERIX_STATUS_SUCCESS;
    }
    *options = *input;
    return numerix_options_validate(options, error);
}

static const numerix_piecewise_segment *find_side_segment(
    const numerix_piecewise_function *function, double boundary, int direction)
{
    size_t index;

    if (function == NULL) {
        return NULL;
    }
    for (index = 0; index < function->count; ++index) {
        const numerix_piecewise_segment *segment = &function->segments[index];
        if (direction < 0 && segment->lower_bound < boundary &&
            segment->upper_bound >= boundary) {
            return segment;
        }
        if (direction > 0 && segment->lower_bound <= boundary &&
            segment->upper_bound > boundary) {
            return segment;
        }
    }
    return NULL;
}

numerix_status numerix_piecewise_estimate_one_sided_limit(
    const numerix_piecewise_function *function, double boundary, int direction,
    const numerix_options *input, double *limit,
    numerix_diagnostic_confidence *confidence, numerix_error *error)
{
    numerix_options options;
    const numerix_piecewise_segment *segment;
    double distance;
    double step;
    double previous = NAN;
    double previous_point = NAN;
    size_t stable_count = 0;
    size_t growth_count = 0;
    size_t iteration;
    numerix_status status;

    if (function == NULL || limit == NULL || confidence == NULL ||
        !isfinite(boundary) || (direction != -1 && direction != 1)) {
        return diagnostic_error(
            error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "invalid one-sided limit request");
    }
    *limit = NAN;
    *confidence = NUMERIX_DIAGNOSTIC_INCONCLUSIVE;
    status = normalized_options(input, &options, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return status;
    }
    segment = find_side_segment(function, boundary, direction);
    if (segment == NULL) {
        return diagnostic_error(
            error, NUMERIX_STATUS_INVALID_INPUT,
            "no defined segment exists on the requested side");
    }

    distance = direction < 0 ? boundary - segment->lower_bound
                             : segment->upper_bound - boundary;
    if (isfinite(distance)) {
        step = distance * 0.25;
    } else {
        step = fmax(1.0, fabs(boundary)) * 0.01;
    }
    if (!(step > 0.0) || !isfinite(step)) {
        step = fmax(1.0, fabs(boundary)) * 0.01;
    }

    for (iteration = 0; iteration < options.maximum_iterations; ++iteration) {
        double point = boundary + (double)direction * step;
        double value = NAN;
        double tolerance;

        if (point == boundary) {
            point = nextafter(boundary, direction < 0 ? -INFINITY : INFINITY);
        }
        if (point <= segment->lower_bound || point >= segment->upper_bound) {
            point = nextafter(boundary,
                              direction < 0 ? -INFINITY : INFINITY);
        }
        if (point == previous_point) {
            if (growth_count >= 1) {
                *limit = copysign(INFINITY, previous);
                *confidence = NUMERIX_DIAGNOSTIC_SUSPECTED;
                numerix_error_clear(error);
                return NUMERIX_STATUS_SUCCESS;
            }
            break;
        }
        status = numerix_piecewise_evaluate_segment(
            segment, point, &value, error);
        if (status != NUMERIX_STATUS_SUCCESS || !isfinite(value)) {
            if (growth_count >= 1 && isfinite(previous)) {
                *limit = copysign(INFINITY, previous);
                *confidence = NUMERIX_DIAGNOSTIC_SUSPECTED;
                numerix_error_clear(error);
                return NUMERIX_STATUS_SUCCESS;
            }
            *limit = value;
            *confidence = NUMERIX_DIAGNOSTIC_INCONCLUSIVE;
            return status == NUMERIX_STATUS_SUCCESS
                       ? diagnostic_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                          "one-sided sample was non-finite")
                       : status;
        }

        if (isfinite(previous)) {
            tolerance = options.absolute_tolerance +
                        options.relative_tolerance * fmax(fabs(value), fabs(previous));
            if (fabs(value - previous) <= tolerance) {
                ++stable_count;
                if (stable_count >= 3) {
                    *limit = value;
                    *confidence = NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED;
                    numerix_error_clear(error);
                    return NUMERIX_STATUS_SUCCESS;
                }
            } else {
                stable_count = 0;
            }
            if (fabs(value) > 1.5 * fmax(fabs(previous), DBL_MIN)) {
                ++growth_count;
                if (growth_count >= 3) {
                    *limit = copysign(INFINITY, value);
                    *confidence = NUMERIX_DIAGNOSTIC_SUSPECTED;
                    numerix_error_clear(error);
                    return NUMERIX_STATUS_SUCCESS;
                }
            } else {
                growth_count = 0;
            }
        }

        previous = value;
        previous_point = point;
        step *= 0.5;
        if (step == 0.0) {
            break;
        }
    }

    *limit = previous;
    *confidence = NUMERIX_DIAGNOSTIC_INCONCLUSIVE;
    return diagnostic_error(
        error, NUMERIX_STATUS_CONVERGENCE_FAILURE,
        "one-sided limit did not converge within the iteration limit");
}

static int values_close(double left, double right, const numerix_options *options)
{
    double scale = fmax(fabs(left), fabs(right));
    return fabs(left - right) <=
           options->absolute_tolerance + options->relative_tolerance * scale;
}

numerix_status numerix_piecewise_analyze_boundary(
    const numerix_piecewise_function *function, double boundary,
    const numerix_options *input,
    numerix_piecewise_boundary_report *report, numerix_error *error)
{
    numerix_options options;
    numerix_diagnostic_confidence left_confidence = NUMERIX_DIAGNOSTIC_INCONCLUSIVE;
    numerix_diagnostic_confidence right_confidence = NUMERIX_DIAGNOSTIC_INCONCLUSIVE;
    numerix_status options_status;

    if (function == NULL || report == NULL || !isfinite(boundary)) {
        return diagnostic_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                "finite boundary and report are required");
    }
    options_status = normalized_options(input, &options, error);
    if (options_status != NUMERIX_STATUS_SUCCESS) {
        return options_status;
    }

    report->boundary = boundary;
    report->left_limit = NAN;
    report->right_limit = NAN;
    report->function_value = NAN;
    report->left_status = numerix_piecewise_estimate_one_sided_limit(
        function, boundary, -1, &options, &report->left_limit,
        &left_confidence, NULL);
    report->right_status = numerix_piecewise_estimate_one_sided_limit(
        function, boundary, 1, &options, &report->right_limit,
        &right_confidence, NULL);
    report->function_status = numerix_piecewise_evaluate(
        function, boundary, &report->function_value, NULL);
    report->type = NUMERIX_DISCONTINUITY_UNKNOWN;
    report->confidence = NUMERIX_DIAGNOSTIC_INCONCLUSIVE;

    if (isinf(report->left_limit) || isinf(report->right_limit)) {
        report->type = NUMERIX_DISCONTINUITY_INFINITE;
        report->confidence = NUMERIX_DIAGNOSTIC_SUSPECTED;
    } else if (isfinite(report->left_limit) && isfinite(report->right_limit)) {
        if (!values_close(report->left_limit, report->right_limit, &options)) {
            report->type = NUMERIX_DISCONTINUITY_JUMP;
            report->confidence = NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED;
        } else if (report->function_status != NUMERIX_STATUS_SUCCESS ||
                   !isfinite(report->function_value) ||
                   !values_close(report->function_value, report->left_limit,
                                 &options)) {
            report->type = NUMERIX_DISCONTINUITY_REMOVABLE;
            report->confidence = NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED;
        } else if (left_confidence == NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED &&
                   right_confidence == NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED) {
            report->type = NUMERIX_DISCONTINUITY_CONTINUOUS;
            report->confidence = NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED;
        }
    }

    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

static int boundary_seen(const double *boundaries, size_t count, double value)
{
    size_t index;

    for (index = 0; index < count; ++index) {
        if (boundaries[index] == value) {
            return 1;
        }
    }
    return 0;
}

numerix_status numerix_piecewise_analyze_boundaries(
    const numerix_piecewise_function *function, const numerix_options *options,
    numerix_piecewise_boundary_report *reports, size_t capacity,
    size_t *report_count, numerix_error *error)
{
    double *boundaries;
    size_t boundary_count = 0;
    size_t index;

    if (function == NULL || report_count == NULL ||
        (capacity > 0 && reports == NULL)) {
        return diagnostic_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                "piecewise function and output storage are required");
    }
    *report_count = 0;
    boundaries = numerix_allocate(function->count * 2 * sizeof(*boundaries));
    if (function->count > 0 && boundaries == NULL) {
        return diagnostic_error(error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                                "could not allocate boundary workspace");
    }

    for (index = 0; index < function->count; ++index) {
        double candidates[2] = {
            function->segments[index].lower_bound,
            function->segments[index].upper_bound
        };
        size_t candidate_index;

        for (candidate_index = 0; candidate_index < 2; ++candidate_index) {
            double boundary = candidates[candidate_index];
            if (isfinite(boundary) &&
                !boundary_seen(boundaries, boundary_count, boundary)) {
                boundaries[boundary_count++] = boundary;
            }
        }
    }

    for (index = 0; index < boundary_count; ++index) {
        size_t other;
        for (other = index + 1; other < boundary_count; ++other) {
            if (boundaries[other] < boundaries[index]) {
                double temporary = boundaries[index];
                boundaries[index] = boundaries[other];
                boundaries[other] = temporary;
            }
        }
    }

    if (boundary_count > capacity) {
        numerix_deallocate(boundaries);
        return diagnostic_error(error, NUMERIX_STATUS_LIMIT_REACHED,
                                "boundary report capacity is too small");
    }
    for (index = 0; index < boundary_count; ++index) {
        numerix_piecewise_analyze_boundary(
            function, boundaries[index], options, &reports[index], NULL);
    }
    *report_count = boundary_count;
    numerix_deallocate(boundaries);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_piecewise_scan_interval(
    const numerix_piecewise_function *function, double lower, double upper,
    size_t samples, numerix_piecewise_candidate *candidates,
    size_t capacity, size_t *candidate_count, numerix_error *error)
{
    size_t index;
    size_t found = 0;
    double previous_x;
    double previous_value = NAN;
    double previous_slope = NAN;
    numerix_status previous_status;

    if (function == NULL || candidate_count == NULL ||
        (capacity > 0 && candidates == NULL) || !isfinite(lower) ||
        !isfinite(upper) || !(lower < upper) || !isfinite(upper - lower) ||
        samples < 2) {
        return diagnostic_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                "invalid interval scan request");
    }
    *candidate_count = 0;
    previous_x = lower;
    previous_status = numerix_piecewise_evaluate(
        function, lower, &previous_value, NULL);

    for (index = 1; index <= samples; ++index) {
        double fraction = (double)index / (double)samples;
        double x = lower + (upper - lower) * fraction;
        double value = NAN;
        numerix_status current_status = numerix_piecewise_evaluate(
            function, x, &value, NULL);
        int suspicious = current_status != previous_status ||
                         (current_status == NUMERIX_STATUS_SUCCESS &&
                          (!isfinite(value) ||
                           !isfinite(previous_value)));
        double slope = NAN;

        if (current_status == NUMERIX_STATUS_SUCCESS &&
            previous_status == NUMERIX_STATUS_SUCCESS && x > previous_x) {
            double scale = fmax(1.0, fmax(fabs(value), fabs(previous_value)));
            slope = (value - previous_value) / (x - previous_x);
            if (fabs(value - previous_value) > 1000.0 * scale) {
                suspicious = 1;
            }
            if (isfinite(previous_slope) &&
                fabs(slope) > 8.0 * fmax(1.0, fabs(previous_slope))) {
                suspicious = 1;
            }
        }

        if (suspicious) {
            if (found >= capacity) {
                *candidate_count = found;
                return diagnostic_error(error, NUMERIX_STATUS_LIMIT_REACHED,
                                        "candidate output capacity is too small");
            }
            candidates[found].location = previous_x + (x - previous_x) * 0.5;
            candidates[found].left_value = previous_value;
            candidates[found].right_value = value;
            candidates[found].left_status = previous_status;
            candidates[found].right_status = current_status;
            candidates[found].confidence = NUMERIX_DIAGNOSTIC_SUSPECTED;
            ++found;
        }

        previous_x = x;
        previous_value = value;
        previous_status = current_status;
        if (isfinite(slope)) {
            previous_slope = slope;
        }
    }

    *candidate_count = found;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
