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
static numerix_status deriv(numerix_function f,void*d,double x,double h,const numerix_options*in,numerix_result*r,numerix_error*e,diff_kind kind){numerix_options o;double fm2=0,fm=0,f0=0,fp=0,fp2=0,v=0; numerix_status s;if(!r)return numerix_error_set(e,NUMERIX_STATUS_INVALID_ARGUMENT,"result is required");memset(r,0,sizeof(*r));r->estimated_error=NAN;r->status=NUMERIX_STATUS_INVALID_ARGUMENT;r->algorithm=kind==FORWARD?"forward_difference":kind==BACKWARD?"backward_difference":kind==CENTRAL?"central_difference":kind==FIVE?"five_point_central":kind==RICHARDSON?"richardson_derivative":"central_second_derivative";if(in)o=*in;else numerix_options_init(&o);s=numerix_options_validate(&o,e);if(s)return r->status=s;if(!f||!isfinite(x)||!isfinite(h)||h<=0)return r->status=numerix_error_set(e,NUMERIX_STATUS_INVALID_ARGUMENT,"callback, finite x, and positive finite h are required");
#define EVAL(xx,ff) do{s=d_eval(f,d,(xx),&(ff),r,&o,e);if(s){r->status=s;return s;}}while(0)
    if(kind==FORWARD||kind==BACKWARD||kind==CENTRAL||kind==RICHARDSON||kind==SECOND)EVAL(x,f0);
    if(kind==FORWARD||kind==CENTRAL||kind==RICHARDSON||kind==SECOND)EVAL(x+h,fp);
    if(kind==BACKWARD||kind==CENTRAL||kind==RICHARDSON||kind==SECOND)EVAL(x-h,fm);
    if(kind==FIVE){EVAL(x-2*h,fm2);EVAL(x-h,fm);EVAL(x+h,fp);EVAL(x+2*h,fp2);v=(fm2-8*fm+8*fp-fp2)/(12*h);r->estimated_error=fabs((fp-fm)/(2*h)-v);}
    else if(kind==FORWARD)v=(fp-f0)/h;
    else if(kind==BACKWARD)v=(f0-fm)/h;
    else if(kind==CENTRAL)v=(fp-fm)/(2*h);
    else if(kind==SECOND)v=(fp-2*f0+fm)/(h*h);
    else {double coarse=(fp-fm)/(2*h),fph,fmh,fine;EVAL(x+h/2,fph);EVAL(x-h/2,fmh);fine=(fph-fmh)/h;v=(4.0*fine-coarse)/3.0;r->estimated_error=fabs(v-fine);}
#undef EVAL
    if(!isfinite(v))return r->status=numerix_error_set(e,NUMERIX_STATUS_NUMERICAL_FAILURE,"finite difference overflow");
    r->value=v;r->residual=0;r->status=NUMERIX_STATUS_SUCCESS;numerix_error_clear(e);return NUMERIX_STATUS_SUCCESS;}
numerix_status numerix_diff_forward(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,FORWARD);}
numerix_status numerix_diff_backward(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,BACKWARD);}
numerix_status numerix_diff_central(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,CENTRAL);}
numerix_status numerix_diff_central_five_point(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,FIVE);}
numerix_status numerix_diff_richardson(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,RICHARDSON);}
numerix_status numerix_diff_second(numerix_function f,void*d,double x,double h,const numerix_options*o,numerix_result*r,numerix_error*e){return deriv(f,d,x,h,o,r,e,SECOND);}
