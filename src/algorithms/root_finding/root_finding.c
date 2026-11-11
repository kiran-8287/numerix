#include "numerix_internal.h"
#include <float.h>
#include <math.h>
#include <string.h>

static numerix_status setup(const numerix_options *in, numerix_options *out,
                            numerix_result *r, numerix_error *e, const char *name)
{
    numerix_status s;
    if (r == NULL) {
        numerix_error_set(e, NUMERIX_STATUS_INVALID_ARGUMENT, "result is required");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }
    memset(r, 0, sizeof(*r)); r->status = NUMERIX_STATUS_INVALID_ARGUMENT; r->algorithm = name;
    if (in) *out = *in; else numerix_options_init(out);
    s = numerix_options_validate(out, e);
    r->status = s;
    return s;
}
static double tol(const numerix_options *o, double x) { return o->absolute_tolerance + o->relative_tolerance * fabs(x); }
static numerix_status eval(numerix_function function, void *data, double x,
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
                          "non-finite iterate");
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
static numerix_status done(numerix_result *r, numerix_status s, double x, double fx, double err, numerix_error *e)
{
    r->status=s; r->value=x; r->residual=fx; r->estimated_error=err;
    if (e) { if (s == NUMERIX_STATUS_SUCCESS) numerix_error_clear(e); else numerix_error_set(e,s,numerix_status_string(s)); }
    return s;
}
static int sign_change(double fa, double fb) { return (fa <= 0.0 && fb >= 0.0) || (fa >= 0.0 && fb <= 0.0); }

typedef enum bracket_method { BIS, FALSE_POS } bracket_method;
static numerix_status bracketed(numerix_function f, void *d, double a, double b,
    const numerix_options *in, numerix_result *r, numerix_error *e, bracket_method method)
{
    numerix_options o; numerix_status s; double fa=NAN,fb=NAN,prev=0;
    size_t i; const char *name=method==BIS?"bisection":"regula_falsi";
    s=setup(in,&o,r,e,name); if(s) return s;
    if(!f || !isfinite(a)||!isfinite(b)||!(a<b)) return done(r,NUMERIX_STATUS_INVALID_ARGUMENT,0,0,0,e);
    s=eval(f,d,a,&fa,r,&o,e); if(s) return done(r,s,a,fa,0,e);
    s=eval(f,d,b,&fb,r,&o,e); if(s) return done(r,s,b,fb,0,e);
    if(fa==0) return done(r,NUMERIX_STATUS_SUCCESS,a,fa,0,e);
    if(fb==0) return done(r,NUMERIX_STATUS_SUCCESS,b,fb,0,e);
    if(!sign_change(fa,fb)) return done(r,NUMERIX_STATUS_INVALID_INPUT,a,fa,b-a,e);
    prev=b;
    for(i=0;i<o.maximum_iterations;i++) {
        double x,fx=NAN;
        if(method==BIS) x=a+(b-a)*0.5;
        else {
            x=(a*fb-b*fa)/(fb-fa);
            if(!isfinite(x)||x<=a+0.05*(b-a)||x>=b-0.05*(b-a)||x==prev) x=a+(b-a)*0.5;
        }
        prev=x;
        s=eval(f,d,x,&fx,r,&o,e); if(s) return done(r,s,x,NAN,fabs(b-a),e);
        r->iterations=i+1;
        if(fx==0.0 || fabs(b-a)*0.5 <= tol(&o,x)) return done(r,NUMERIX_STATUS_SUCCESS,x,fx,fabs(b-a)*0.5,e);
        if(sign_change(fa,fx)) { b=x; fb=fx; } else { a=x; fa=fx; }
    }
    return done(r,NUMERIX_STATUS_CONVERGENCE_FAILURE,a+(b-a)*0.5,NAN,fabs(b-a)*0.5,e);
}
numerix_status numerix_root_bisection(numerix_function f,void*d,double a,double b,const numerix_options*o,numerix_result*r,numerix_error*e)
{ return bracketed(f,d,a,b,o,r,e,BIS); }
numerix_status numerix_root_regula_falsi(numerix_function f,void*d,double a,double b,const numerix_options*o,numerix_result*r,numerix_error*e)
{ return bracketed(f,d,a,b,o,r,e,FALSE_POS); }
numerix_status numerix_root_brent(
    numerix_function function, void *data, double a, double b,
    const numerix_options *input, numerix_result *result, numerix_error *error)
{
    numerix_options options;
    numerix_status status;
    double fa = NAN;
    double fb = NAN;
    double c;
    double fc;
    size_t iteration;

    status = setup(input, &options, result, error, "brent_dekker");
    if (status != NUMERIX_STATUS_SUCCESS) return status;
    if (function == NULL || !isfinite(a) || !isfinite(b) || !(a < b)) {
        return done(result, NUMERIX_STATUS_INVALID_ARGUMENT, 0.0, NAN, 0.0, error);
    }

    status = eval(function, data, a, &fa, result, &options, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return done(result, status, a, NAN, 0.0, error);
    }
    status = eval(function, data, b, &fb, result, &options, error);
    if (status != NUMERIX_STATUS_SUCCESS) {
        return done(result, status, b, NAN, 0.0, error);
    }
    if (fa == 0.0) return done(result, NUMERIX_STATUS_SUCCESS, a, fa, 0.0, error);
    if (fb == 0.0) return done(result, NUMERIX_STATUS_SUCCESS, b, fb, 0.0, error);
    if (!sign_change(fa, fb)) {
        return done(result, NUMERIX_STATUS_INVALID_INPUT, a, fa, b - a, error);
    }

    c = a;
    fc = fa;
    for (iteration = 0; iteration < options.maximum_iterations; ++iteration) {
        double width = b - a;
        double midpoint = a + width * 0.5;
        double candidate = midpoint;
        double scale = fmax(fabs(fa), fmax(fabs(fb), fabs(fc)));
        double normalized_a = fa / scale;
        double normalized_b = fb / scale;
        double normalized_c = fc / scale;
        double lower_guard = a + width * 0.1;
        double upper_guard = b - width * 0.1;
        double fx = NAN;

        if (width * 0.5 <= tol(&options, midpoint)) {
            double best_x = fabs(fa) <= fabs(fb) ? a : b;
            double best_f = fabs(fa) <= fabs(fb) ? fa : fb;
            return done(result, NUMERIX_STATUS_SUCCESS, best_x, best_f,
                        width * 0.5, error);
        }

        if (fa != fb && fa != fc && fb != fc) {
            double weight_a = normalized_b * normalized_c /
                              ((normalized_a - normalized_b) *
                               (normalized_a - normalized_c));
            double weight_b = normalized_a * normalized_c /
                              ((normalized_b - normalized_a) *
                               (normalized_b - normalized_c));
            double weight_c = normalized_a * normalized_b /
                              ((normalized_c - normalized_a) *
                               (normalized_c - normalized_b));
            double inverse_quadratic = a * weight_a + b * weight_b + c * weight_c;

            if (isfinite(inverse_quadratic) &&
                inverse_quadratic > lower_guard && inverse_quadratic < upper_guard) {
                candidate = inverse_quadratic;
            }
        } else if (fa != fb) {
            double secant = a + width * normalized_a /
                                  (normalized_a - normalized_b);
            if (isfinite(secant) && secant > lower_guard && secant < upper_guard) {
                candidate = secant;
            }
        }

        status = eval(function, data, candidate, &fx, result, &options, error);
        if (status != NUMERIX_STATUS_SUCCESS) {
            return done(result, status, candidate, NAN, width * 0.5, error);
        }
        result->iterations = iteration + 1;
        if (fx == 0.0 || fabs(fx) <= options.absolute_tolerance) {
            return done(result, NUMERIX_STATUS_SUCCESS, candidate, fx,
                        width * 0.5, error);
        }

        if (sign_change(fa, fx)) {
            c = b;
            fc = fb;
            b = candidate;
            fb = fx;
        } else {
            c = a;
            fc = fa;
            a = candidate;
            fa = fx;
        }
    }

    {
        double best_x = fabs(fa) <= fabs(fb) ? a : b;
        double best_f = fabs(fa) <= fabs(fb) ? fa : fb;
        return done(result, NUMERIX_STATUS_CONVERGENCE_FAILURE, best_x, best_f,
                    fabs(b - a) * 0.5, error);
    }
}

numerix_status numerix_root_secant(numerix_function f,void*d,double x0,double x1,const numerix_options*in,numerix_result*r,numerix_error*e)
{
    numerix_options o; numerix_status s; double f0,f1; size_t i;
    s=setup(in,&o,r,e,"secant"); if(s)return s;
    if(!f||!isfinite(x0)||!isfinite(x1)||x0==x1)return done(r,NUMERIX_STATUS_INVALID_ARGUMENT,x1,0,0,e);
    s=eval(f,d,x0,&f0,r,&o,e);if(s)return done(r,s,x0,0,0,e);
    s=eval(f,d,x1,&f1,r,&o,e);if(s)return done(r,s,x1,0,0,e);
    for(i=0;i<o.maximum_iterations;i++){
        double den=f1-f0,x2,f2;
        if(fabs(den)<=DBL_EPSILON*fmax(1.0,fmax(fabs(f0),fabs(f1))))return done(r,NUMERIX_STATUS_NUMERICAL_FAILURE,x1,f1,fabs(x1-x0),e);
        x2=x1-f1*(x1-x0)/den;
        s=eval(f,d,x2,&f2,r,&o,e);if(s)return done(r,s,x1,f1,fabs(x1-x0),e);
        r->iterations=i+1;
        if(fabs(f2)<=o.absolute_tolerance || fabs(x2-x1)<=tol(&o,x2))return done(r,NUMERIX_STATUS_SUCCESS,x2,f2,fabs(x2-x1),e);
        x0=x1;f0=f1;x1=x2;f1=f2;
    }
    return done(r,NUMERIX_STATUS_CONVERGENCE_FAILURE,x1,f1,fabs(x1-x0),e);
}

static numerix_status newton_impl(numerix_function f,numerix_function df,void*d,double x,
    int safeguarded,double lo,double hi,const numerix_options*in,numerix_result*r,numerix_error*e)
{
    numerix_options o; numerix_status s; double fx,fl=0,fh=0; size_t i;
    s=setup(in,&o,r,e,safeguarded?"safeguarded_newton":"newton_raphson");if(s)return s;
    if(!f||!df||!isfinite(x)||(safeguarded&&(!isfinite(lo)||!isfinite(hi)||!(lo<hi)||x<lo||x>hi)))return done(r,NUMERIX_STATUS_INVALID_ARGUMENT,x,0,0,e);
    s=eval(f,d,x,&fx,r,&o,e);if(s)return done(r,s,x,NAN,0,e);
    if(safeguarded){s=eval(f,d,lo,&fl,r,&o,e);if(s)return done(r,s,x,NAN,0,e);s=eval(f,d,hi,&fh,r,&o,e);if(s)return done(r,s,x,NAN,0,e);
      if(fl==0)return done(r,NUMERIX_STATUS_SUCCESS,lo,fl,0,e);
      if(fh==0)return done(r,NUMERIX_STATUS_SUCCESS,hi,fh,0,e);
      if(fx==0)return done(r,NUMERIX_STATUS_SUCCESS,x,fx,0,e);
      if(!sign_change(fl,fh))return done(r,NUMERIX_STATUS_INVALID_INPUT,x,fx,hi-lo,e);}
    else if(fabs(fx)<=o.absolute_tolerance)return done(r,NUMERIX_STATUS_SUCCESS,x,fx,0,e);
    for(i=0;i<o.maximum_iterations;i++){
        double deriv,next,fn;
        if(r->function_evaluations>=o.maximum_function_evaluations)return done(r,NUMERIX_STATUS_LIMIT_REACHED,x,fx,0,e);
        deriv=df(x,d);
        if(!isfinite(deriv))return done(r,NUMERIX_STATUS_NUMERICAL_FAILURE,x,fx,0,e);
        ++r->function_evaluations;
        if(fabs(deriv)<=DBL_EPSILON*fmax(1.0,fabs(fx))){
            if(!safeguarded)return done(r,NUMERIX_STATUS_NUMERICAL_FAILURE,x,fx,0,e);
            next=lo+(hi-lo)*0.5;
        }else{
            next=x-fx/deriv;
            if(safeguarded&&(!isfinite(next)||next<=lo||next>=hi))next=lo+(hi-lo)*0.5;
        }
        if(!isfinite(next))return done(r,NUMERIX_STATUS_NUMERICAL_FAILURE,x,fx,0,e);
        s=eval(f,d,next,&fn,r,&o,e);if(s)return done(r,s,x,fx,fabs(next-x),e);
        r->iterations=i+1;
        if(safeguarded){if(sign_change(fl,fn)){hi=next;fh=fn;}else{lo=next;fl=fn;}}
        if (fabs(fn) <= o.absolute_tolerance) {
            return done(r, NUMERIX_STATUS_SUCCESS, next, fn, fabs(next - x), e);
        }
        if (safeguarded && fabs(hi - lo) * 0.5 <= tol(&o, next)) {
            return done(r, NUMERIX_STATUS_SUCCESS, next, fn, fabs(hi - lo) * 0.5, e);
        }
        if (!safeguarded && fabs(next - x) <= tol(&o, next)) {
            return done(r, NUMERIX_STATUS_SUCCESS, next, fn, fabs(next - x), e);
        }
        x=next;fx=fn;
    }
    return done(r,NUMERIX_STATUS_CONVERGENCE_FAILURE,x,fx,0,e);
}
numerix_status numerix_root_newton(numerix_function f,numerix_function df,void*d,double x,const numerix_options*o,numerix_result*r,numerix_error*e)
{ return newton_impl(f,df,d,x,0,0,0,o,r,e); }
numerix_status numerix_root_safeguarded_newton(numerix_function f,numerix_function df,void*d,double lo,double hi,const numerix_options*o,numerix_result*r,numerix_error*e)
{ return newton_impl(f,df,d,lo+(hi-lo)*0.5,1,lo,hi,o,r,e); }
