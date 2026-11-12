#include "numerix.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static numerix_status fail(numerix_error *error, numerix_status status,
                           const char *message)
{
    numerix_error_set(error, status, message);
    return status;
}
typedef struct scaled_number {
    double significand;
    int exponent;
} scaled_number;

static scaled_number scaled_difference(double left, double right)
{
    scaled_number result = {0.0, 0};
    double scale = fmax(fabs(left), fabs(right));

    if (scale != 0.0) {
        int scale_exponent;
        int difference_exponent;
        double normalized_left;
        double normalized_right;
        double normalized_difference;

        (void)frexp(scale, &scale_exponent);
        normalized_left = scalbn(left, -scale_exponent);
        normalized_right = scalbn(right, -scale_exponent);
        normalized_difference = normalized_left - normalized_right;
        result.significand = frexp(normalized_difference,
                                   &difference_exponent);
        result.exponent = scale_exponent + difference_exponent;
    }
    return result;
}

static scaled_number scaled_product(scaled_number left, double right)
{
    scaled_number result = {0.0, 0};
    int right_exponent;
    int product_exponent;
    double right_significand;

    if (left.significand == 0.0 || right == 0.0) {
        return result;
    }
    right_significand = frexp(right, &right_exponent);
    result.significand = frexp(left.significand * right_significand,
                               &product_exponent);
    result.exponent = left.exponent + right_exponent + product_exponent;
    return result;
}

static scaled_number scaled_sum(scaled_number left, scaled_number right)
{
    scaled_number result;
    int exponent;
    int adjustment;
    double sum;

    if (left.significand == 0.0) {
        return right;
    }
    if (right.significand == 0.0) {
        return left;
    }
    exponent = left.exponent > right.exponent ? left.exponent : right.exponent;
    sum = scalbn(left.significand, left.exponent - exponent) +
          scalbn(right.significand, right.exponent - exponent);
    if (sum == 0.0) {
        result.significand = 0.0;
        result.exponent = 0;
        return result;
    }
    result.significand = frexp(sum, &adjustment);
    result.exponent = exponent + adjustment;
    return result;
}

static double scaled_quotient(scaled_number numerator,
                              scaled_number denominator)
{
    double quotient_significand;
    int quotient_exponent;

    if (numerator.significand == 0.0) {
        return 0.0;
    }
    quotient_significand = frexp(numerator.significand / denominator.significand,
                                 &quotient_exponent);
    return scalbn(quotient_significand,
                  numerator.exponent - denominator.exponent + quotient_exponent);
}
static numerix_status validate(const double*x,const double*y,size_t n,double q,int extrap,numerix_error*e,double*mn,double*mx){size_t i,j;if(!x||!y||n<2||!isfinite(q))return fail(e,NUMERIX_STATUS_INVALID_ARGUMENT,"at least two finite data points and a finite query are required");*mn=x[0];*mx=x[0];for(i=0;i<n;i++){if(!isfinite(x[i])||!isfinite(y[i]))return fail(e,NUMERIX_STATUS_INVALID_INPUT,"interpolation data must be finite");if(x[i]<*mn)*mn=x[i];if(x[i]>*mx)*mx=x[i];for(j=0;j<i;j++)if(x[i]==x[j])return fail(e,NUMERIX_STATUS_INVALID_INPUT,"duplicate interpolation abscissas");}if(!extrap&&(q<*mn||q>*mx))return fail(e,NUMERIX_STATUS_INVALID_INPUT,"query outside data range; enable extrapolation explicitly");return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_interp_linear(double x0, double y0, double x1, double y1,
                                     double x, numerix_result *result,
                                     numerix_error *error)
{
    scaled_number denominator;
    scaled_number left_term;
    scaled_number right_term;
    scaled_number numerator;
    double value;

    if (result == NULL) {
        return fail(error, NUMERIX_STATUS_INVALID_ARGUMENT, "result required");
    }
    memset(result, 0, sizeof(*result));
    result->estimated_error = NAN;
    result->residual = NAN;
    result->algorithm = "linear_interpolation";
    if (!isfinite(x0) || !isfinite(x1) || !isfinite(y0) || !isfinite(y1) ||
        !isfinite(x) || x0 == x1) {
        result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return fail(error, result->status,
                    "finite data and distinct abscissas required");
    }

    denominator = scaled_difference(x1, x0);
    left_term = scaled_product(scaled_difference(x1, x), y0);
    right_term = scaled_product(scaled_difference(x, x0), y1);
    numerator = scaled_sum(left_term, right_term);
    value = scaled_quotient(numerator, denominator);
    result->value = value;
    if (!isfinite(value)) {
        result->status = NUMERIX_STATUS_NUMERICAL_FAILURE;
        return fail(error, result->status, "interpolation result is not representable");
    }
    result->status = NUMERIX_STATUS_SUCCESS;
    result->function_evaluations = 0U;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
numerix_status numerix_interp_lagrange(const double*x,const double*y,size_t n,double q,int extrap,numerix_result*r,numerix_error*e){size_t i,j;double mn,mx,sum=0,c=0;numerix_status s;if(!r)return fail(e,NUMERIX_STATUS_INVALID_ARGUMENT,"result required");memset(r,0,sizeof(*r));r->estimated_error=NAN;r->residual=NAN;r->algorithm="lagrange_interpolation";s=validate(x,y,n,q,extrap,e,&mn,&mx);if(s){r->status=s;return s;}for(i=0;i<n;i++){double term=y[i];for(j=0;j<n;j++)if(j!=i)term*=((q-x[j])/(x[i]-x[j]));{double z=term-c,t=sum+z;c=(t-sum)-z;sum=t;}}if(!isfinite(sum)){r->status=NUMERIX_STATUS_NUMERICAL_FAILURE;return fail(e,r->status,"interpolating polynomial overflow");}r->value=sum;r->status=NUMERIX_STATUS_SUCCESS;r->iterations=n;numerix_error_clear(e);return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_interp_newton_create(const double*x,const double*y,size_t n,numerix_interpolant*p,numerix_error*e){size_t i,j;double mn,mx;numerix_status s;if(!p)return fail(e,NUMERIX_STATUS_INVALID_ARGUMENT,"interpolant output required");memset(p,0,sizeof(*p));s=validate(x,y,n,x?x[0]:NAN,1,e,&mn,&mx);if(s)return s;if(n>SIZE_MAX/sizeof(double))return fail(e,NUMERIX_STATUS_INVALID_DIMENSIONS,"too many data points");p->x=malloc(n*sizeof(double));p->coefficients=malloc(n*sizeof(double));if(!p->x||!p->coefficients){numerix_interpolant_destroy(p);return fail(e,NUMERIX_STATUS_ALLOCATION_FAILURE,"interpolant allocation failed");}memcpy(p->x,x,n*sizeof(double));memcpy(p->coefficients,y,n*sizeof(double));p->size=n;p->minimum=mn;p->maximum=mx;for(j=1;j<n;j++)for(i=n;i-->j;){double den=x[i]-x[i-j];p->coefficients[i]=(p->coefficients[i]-p->coefficients[i-1])/den;if(!isfinite(p->coefficients[i])){numerix_interpolant_destroy(p);return fail(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"divided difference overflow");}}numerix_error_clear(e);return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_interp_newton_evaluate(const numerix_interpolant*p,double q,int extrap,numerix_result*r,numerix_error*e){size_t i;double v;numerix_status s;if(!r)return fail(e,NUMERIX_STATUS_INVALID_ARGUMENT,"result required");memset(r,0,sizeof(*r));r->estimated_error=NAN;r->residual=NAN;r->algorithm="newton_divided_difference";if(!p||!p->x||!p->coefficients||p->size<2||!isfinite(q)){r->status=NUMERIX_STATUS_INVALID_ARGUMENT;return fail(e,r->status,"invalid interpolant or query");}if(!extrap&&(q<p->minimum||q>p->maximum)){r->status=NUMERIX_STATUS_INVALID_INPUT;return fail(e,r->status,"query outside data range");}v=p->coefficients[p->size-1];for(i=p->size-1;i-->0;)v=v*(q-p->x[i])+p->coefficients[i];if(!isfinite(v)){r->status=NUMERIX_STATUS_NUMERICAL_FAILURE;return fail(e,r->status,"polynomial evaluation overflow");}r->value=v;r->iterations=p->size-1;r->status=NUMERIX_STATUS_SUCCESS;numerix_error_clear(e);s=NUMERIX_STATUS_SUCCESS;return s;}
void numerix_interpolant_destroy(numerix_interpolant*p){if(p){free(p->x);free(p->coefficients);memset(p,0,sizeof(*p));}}
