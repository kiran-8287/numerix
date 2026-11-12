#include "numerix_internal.h"
#include <math.h>
#include <string.h>

typedef enum { FORWARD, BACKWARD, CENTRAL, FIVE, RICHARDSON, SECOND } diff_kind;
static numerix_status d_eval(numerix_function function, void *data, double x,
                             double *value, numerix_result *result,
                             const numerix_options *options, numerix_error *error)
{
    if (result->function_evaluations >= options->maximum_function_evaluations) {
        numerix_error_set(error, NUMERIX_STATUS_LIMIT_REACHED,
                          "function evaluation limit reached");
        return NUMERIX_STATUS_LIMIT_REACHED;
    }
    if (!isfinite(x)) {
        numerix_error_set(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                          "difference point is non-finite");
        return NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    *value = function(x, data);
    ++result->function_evaluations;
    if (!isfinite(*value)) {
        numerix_error_set(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                          "callback returned a non-finite value");
        return NUMERIX_STATUS_NUMERICAL_FAILURE;
    }
    return NUMERIX_STATUS_SUCCESS;
}
static numerix_status deriv(numerix_function function, void *data, double x,
                            double h, const numerix_options *input,
                            numerix_result *result, numerix_error *error,
                            diff_kind kind)
{
    numerix_options options;
    double coordinates[5];
    double fm2 = 0.0;
    double fm = 0.0;
    double f0 = 0.0;
    double fp = 0.0;
    double fp2 = 0.0;
    double value = 0.0;
    size_t coordinate_count = 0U;
    size_t index;
    numerix_status status;

    if (result == NULL) {
        return numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                 "result is required");
    }
    memset(result, 0, sizeof(*result));
    result->estimated_error = NAN;
    result->residual = NAN;
    result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
    result->algorithm = kind == FORWARD ? "forward_difference" :
        kind == BACKWARD ? "backward_difference" :
        kind == CENTRAL ? "central_difference" :
        kind == FIVE ? "five_point_central" :
        kind == RICHARDSON ? "richardson_derivative" :
        "central_second_derivative";
    if (input != NULL) {
        options = *input;
    } else {
        numerix_options_init(&options);
    }
    status = numerix_options_validate(&options, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        result->status = status;
        return status;
    }
    if (function == NULL || !isfinite(x) || !isfinite(h) || h <= 0.0) {
        result->status = numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
            "callback, finite x, and positive finite h are required");
        return result->status;
    }

    switch (kind) {
        case FORWARD:
            coordinates[0] = x;
            coordinates[1] = x + h;
            coordinate_count = 2U;
            break;
        case BACKWARD:
            coordinates[0] = x - h;
            coordinates[1] = x;
            coordinate_count = 2U;
            break;
        case CENTRAL:
        case SECOND:
            coordinates[0] = x - h;
            coordinates[1] = x;
            coordinates[2] = x + h;
            coordinate_count = 3U;
            break;
        case FIVE:
            coordinates[0] = x - 2.0 * h;
            coordinates[1] = x - h;
            coordinates[2] = x;
            coordinates[3] = x + h;
            coordinates[4] = x + 2.0 * h;
            coordinate_count = 5U;
            break;
        case RICHARDSON:
            coordinates[0] = x - h;
            coordinates[1] = x - h / 2.0;
            coordinates[2] = x;
            coordinates[3] = x + h / 2.0;
            coordinates[4] = x + h;
            coordinate_count = 5U;
            break;
    }
    for (index = 0; index < coordinate_count; ++index) {
        size_t previous;

        if (!isfinite(coordinates[index])) {
            result->status = numerix_error_set(error,
                NUMERIX_STATUS_INVALID_INPUT,
                "requested step produces a non-finite stencil coordinate");
            return result->status;
        }
        for (previous = 0; previous < index; ++previous) {
            if (coordinates[index] == coordinates[previous]) {
                result->status = numerix_error_set(error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    "requested step produces duplicate stencil coordinates");
                return result->status;
            }
        }
    }

#define EVALUATE(point, output) do { \
    status = d_eval(function, data, (point), &(output), result, &options, error); \
    if (status != NUMERIX_STATUS_SUCCESS) { \
        result->status = status; \
        return status; \
    } \
} while (0)
    if (kind == FORWARD || kind == BACKWARD || kind == CENTRAL ||
        kind == RICHARDSON || kind == SECOND) {
        EVALUATE(x, f0);
    }
    if (kind == FORWARD || kind == CENTRAL || kind == RICHARDSON ||
        kind == SECOND) {
        EVALUATE(x + h, fp);
    }
    if (kind == BACKWARD || kind == CENTRAL || kind == RICHARDSON ||
        kind == SECOND) {
        EVALUATE(x - h, fm);
    }
    if (kind == FIVE) {
        EVALUATE(x - 2.0 * h, fm2);
        EVALUATE(x - h, fm);
        EVALUATE(x + h, fp);
        EVALUATE(x + 2.0 * h, fp2);
        value = (fm2 - 8.0 * fm + 8.0 * fp - fp2) / (12.0 * h);
        result->estimated_error = fabs((fp - fm) / (2.0 * h) - value);
    } else if (kind == FORWARD) {
        value = (fp - f0) / h;
    } else if (kind == BACKWARD) {
        value = (f0 - fm) / h;
    } else if (kind == CENTRAL) {
        value = (fp - fm) / (2.0 * h);
    } else if (kind == SECOND) {
        value = (fp - 2.0 * f0 + fm) / (h * h);
    } else {
        double coarse = (fp - fm) / (2.0 * h);
        double fph;
        double fmh;
        double fine;

        EVALUATE(x + h / 2.0, fph);
        EVALUATE(x - h / 2.0, fmh);
        fine = (fph - fmh) / h;
        value = (4.0 * fine - coarse) / 3.0;
        result->estimated_error = fabs(value - fine);
    }
#undef EVALUATE
    if (!isfinite(value)) {
        result->status = numerix_error_set(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                           "finite difference overflow");
        return result->status;
    }
    result->value = value;
    result->status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
numerix_status numerix_diff_forward(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,FORWARD);}
numerix_status numerix_diff_backward(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,BACKWARD);}
numerix_status numerix_diff_central(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,CENTRAL);}
numerix_status numerix_diff_central_five_point(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,FIVE);}
numerix_status numerix_diff_richardson(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,RICHARDSON);}
numerix_status numerix_diff_second(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,SECOND);}
