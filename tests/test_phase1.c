#include "numerix.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%s:%d failed: %s\n",__FILE__,__LINE__,#c); failures++; } } while(0)
#define NEAR(a,b,t) (fabs((double)(a) - (double)(b)) <= (double)(t) * fmax(1.0, fabs((double)(b))))
static double poly(double x,void*d){(void)d;return x*x-2.0;}
static double dpoly(double x,void*d){(void)d;return 2*x;}
static double square(double x,void*d){(void)d;return x*x;}
static double sine(double x,void*d){(void)d;return sin(x);}
static double quad(double x,void*d){(void)d;return (x-1.25)*(x-1.25)+2;}
static double badfun(double x,void*d){(void)x;(void)d;return NAN;}
static double identity(double x,void*d){(void)d;return x;}
static double constant(double x,void*d){(void)x;(void)d;return 1.0;}
static double flat_root(double x,void*d){(void)d;return x*x*x+1.0;}
static double flat_derivative(double x,void*d){(void)d;return 3.0*x*x;}
static numerix_status ode_exp(double x,const double*y,size_t n,double*dy,void*d){size_t i;(void)x;(void)d;for(i=0;i<n;i++)dy[i]=y[i];return NUMERIX_STATUS_SUCCESS;}
static double objective(const double*x,size_t n,void*d){size_t i;double s=0;(void)d;for(i=0;i<n;i++){double q=x[i]-(double)(i+1);s+=q*q;}return s;}
static numerix_status gradient(const double*x,size_t n,double*g,void*d){size_t i;(void)d;for(i=0;i<n;i++)g[i]=2*(x[i]-(double)(i+1));return NUMERIX_STATUS_SUCCESS;}

static void roots(void){numerix_result r;numerix_error e;numerix_options o;numerix_options_init(&o);o.absolute_tolerance=1e-11;o.relative_tolerance=1e-11;
CHECK(numerix_root_bisection(poly,NULL,0,2,&o,&r,&e)==NUMERIX_STATUS_SUCCESS);CHECK(NEAR(r.value,sqrt(2),1e-9));
CHECK(numerix_root_regula_falsi(poly,NULL,0,2,&o,&r,&e)==NUMERIX_STATUS_SUCCESS);CHECK(NEAR(r.value,sqrt(2),1e-8));
CHECK(numerix_root_secant(poly,NULL,1,2,&o,&r,&e)==NUMERIX_STATUS_SUCCESS);CHECK(NEAR(r.value,sqrt(2),1e-8));
CHECK(numerix_root_newton(poly,dpoly,NULL,1,&o,&r,&e)==NUMERIX_STATUS_SUCCESS);CHECK(NEAR(r.value,sqrt(2),1e-9));
CHECK(numerix_root_safeguarded_newton(poly,dpoly,NULL,0,2,&o,&r,&e)==NUMERIX_STATUS_SUCCESS);CHECK(NEAR(r.value,sqrt(2),1e-8));
CHECK(numerix_root_brent(poly,NULL,0,2,&o,&r,&e)==NUMERIX_STATUS_SUCCESS);CHECK(NEAR(r.value,sqrt(2),1e-8));
CHECK(numerix_root_bisection(poly,NULL,2,0,&o,&r,&e)==NUMERIX_STATUS_INVALID_ARGUMENT);
CHECK(numerix_root_bisection(poly,NULL,0,1,&o,&r,&e)==NUMERIX_STATUS_INVALID_INPUT);
CHECK(numerix_root_bisection(badfun,NULL,0,2,&o,&r,&e)==NUMERIX_STATUS_NUMERICAL_FAILURE);
CHECK(numerix_root_bisection(identity,NULL,-1,0,&o,&r,&e)==NUMERIX_STATUS_SUCCESS&&r.value==0);
CHECK(numerix_root_secant(constant,NULL,0,1,&o,&r,&e)==NUMERIX_STATUS_NUMERICAL_FAILURE);
CHECK(numerix_root_newton(flat_root,flat_derivative,NULL,0,&o,&r,&e)==NUMERIX_STATUS_NUMERICAL_FAILURE);
}
static void linear(void){numerix_error e;numerix_options o;numerix_matrix*a=NULL,*t=NULL,*prod=NULL,*acopy=NULL,*sum=NULL,*diff=NULL,*scaledm=NULL;numerix_vector*b=NULL,*x=NULL,*y=NULL,*copy=NULL,*mv=NULL,*scaledv=NULL,*sub=NULL;numerix_lu lu;double res=99;numerix_options_init(&o);CHECK(numerix_matrix_create(2,2,&a,&e)==0);a->data[0]=0;a->data[1]=2;a->data[2]=1;a->data[3]=3;CHECK(numerix_vector_create(2,&b,&e)==0);b->data[0]=4;b->data[1]=7;
CHECK(numerix_linear_solve_gaussian(a,b,&o,&x,&res,&e)==0);CHECK(NEAR(x->data[0],1,1e-12)&&NEAR(x->data[1],2,1e-12)&&res<1e-12);numerix_vector_destroy(x);x=NULL;
CHECK(numerix_linear_solve_gauss_jordan(a,b,&o,&x,&res,&e)==0);CHECK(NEAR(x->data[0],1,1e-12)&&NEAR(x->data[1],2,1e-12));
CHECK(numerix_lu_decompose(a,&o,&lu,&e)==0);CHECK(numerix_lu_solve(&lu,b,&y,&e)==0);CHECK(NEAR(y->data[0],1,1e-12));{numerix_vector*again=NULL;CHECK(numerix_lu_solve(&lu,b,&again,&e)==0);CHECK(NEAR(again->data[1],2,1e-12));numerix_vector_destroy(again);}CHECK(lu.permutation[0]==1);numerix_lu_destroy(&lu);
CHECK(numerix_matrix_transpose(a,&t,&e)==0);CHECK(numerix_matrix_multiply(a,t,&prod,&e)==0);CHECK(prod->rows==2&&prod->columns==2);CHECK(numerix_matrix_copy(a,&acopy,&e)==0);CHECK(numerix_matrix_add(a,acopy,&sum,&e)==0);CHECK(numerix_matrix_subtract(sum,a,&diff,&e)==0);CHECK(numerix_matrix_scale(diff,0.5,&scaledm,&e)==0);CHECK(NEAR(scaledm->data[3],1.5,1e-12));CHECK(numerix_matrix_vector_multiply(a,x,&mv,&e)==0);CHECK(NEAR(mv->data[0],4,1e-12));
{numerix_vector*z=NULL;double dot;CHECK(numerix_vector_add(b,b,&z,&e)==0);CHECK(numerix_vector_dot(z,b,&dot,&e)==0&&NEAR(dot,130,1e-12));CHECK(NEAR(numerix_vector_norm_inf(b),7,1e-12));CHECK(NEAR(numerix_vector_norm2(b),sqrt(65),1e-12));CHECK(numerix_vector_copy(b,&copy,&e)==0);CHECK(numerix_vector_subtract(copy,b,&sub,&e)==0);CHECK(numerix_vector_scale(sub,2,&scaledv,&e)==0);CHECK(numerix_vector_norm2(scaledv)==0);numerix_vector_destroy(z);}
{numerix_matrix*sing=NULL;numerix_vector*rhs=NULL,*answer=NULL;CHECK(numerix_matrix_create(2,2,&sing,&e)==0);sing->data[0]=1;sing->data[1]=2;sing->data[2]=2;sing->data[3]=4;CHECK(numerix_vector_create(2,&rhs,&e)==0);rhs->data[0]=3;rhs->data[1]=6;CHECK(numerix_linear_solve_gaussian(sing,rhs,&o,&answer,&res,&e)==NUMERIX_STATUS_SINGULAR_MATRIX);CHECK(numerix_linear_solve_gauss_jordan(sing,rhs,&o,&answer,&res,&e)==NUMERIX_STATUS_SINGULAR_MATRIX);numerix_matrix_destroy(sing);numerix_vector_destroy(rhs);}
numerix_matrix_destroy(a);numerix_matrix_destroy(t);numerix_matrix_destroy(prod);numerix_matrix_destroy(acopy);numerix_matrix_destroy(sum);numerix_matrix_destroy(diff);numerix_matrix_destroy(scaledm);numerix_vector_destroy(b);numerix_vector_destroy(x);numerix_vector_destroy(y);numerix_vector_destroy(copy);numerix_vector_destroy(mv);numerix_vector_destroy(scaledv);numerix_vector_destroy(sub);
}
static void differentiation(void){numerix_result r;numerix_error e;numerix_options o;numerix_options_init(&o);CHECK(numerix_diff_forward(square,NULL,2,1e-4,&o,&r,&e)==0);CHECK(NEAR(r.value,4,1e-4));CHECK(numerix_diff_backward(square,NULL,2,1e-4,&o,&r,&e)==0);CHECK(NEAR(r.value,4,1e-4));CHECK(numerix_diff_central(square,NULL,2,1e-3,&o,&r,&e)==0);CHECK(NEAR(r.value,4,1e-6));CHECK(numerix_diff_central_five_point(square,NULL,2,1e-2,&o,&r,&e)==0);CHECK(NEAR(r.value,4,1e-9));CHECK(numerix_diff_richardson(sine,NULL,0.4,0.1,&o,&r,&e)==0);CHECK(NEAR(r.value,cos(0.4),1e-6));CHECK(numerix_diff_second(square,NULL,2,1e-3,&o,&r,&e)==0);CHECK(NEAR(r.value,2,1e-5));CHECK(numerix_diff_forward(square,NULL,2,0,&o,&r,&e)==NUMERIX_STATUS_INVALID_ARGUMENT);}
static void interpolation(void){double x[]={0,1,2,3},y[]={1,6,17,34};numerix_interpolant p;numerix_result r;numerix_error e;CHECK(numerix_interp_linear(0,1,1,6,0.5,&r,&e)==0);CHECK(NEAR(r.value,3.5,1e-12));CHECK(numerix_interp_lagrange(x,y,4,1.5,0,&r,&e)==0);CHECK(NEAR(r.value,10.75,1e-12));CHECK(numerix_interp_newton_create(x,y,4,&p,&e)==0);CHECK(numerix_interp_newton_evaluate(&p,2.5,0,&r,&e)==0);CHECK(NEAR(r.value,24.75,1e-12));CHECK(numerix_interp_newton_evaluate(&p,4,0,&r,&e)==NUMERIX_STATUS_INVALID_INPUT);numerix_interpolant_destroy(&p);{double xd[]={0,0},yd[]={1,2};CHECK(numerix_interp_lagrange(xd,yd,2,0,1,&r,&e)==NUMERIX_STATUS_INVALID_INPUT);}}
static void integration(void){numerix_result r;numerix_error e;numerix_options o;numerix_options_init(&o);CHECK(numerix_integrate_trapezoid(square,NULL,0,1,1000,&o,&r,&e)==0);CHECK(NEAR(r.value,1.0/3,1e-6));CHECK(numerix_integrate_trapezoid(square,NULL,1,0,1000,&o,&r,&e)==0);CHECK(NEAR(r.value,-1.0/3,1e-6));CHECK(numerix_integrate_simpson(square,NULL,0,1,10,&o,&r,&e)==0);CHECK(NEAR(r.value,1.0/3,1e-12));CHECK(numerix_integrate_simpson(square,NULL,0,1,3,&o,&r,&e)==NUMERIX_STATUS_INVALID_INPUT);CHECK(numerix_integrate_adaptive_simpson(sine,NULL,0,3.141592653589793,&o,20,&r,&e)==0);CHECK(NEAR(r.value,2,1e-9));CHECK(numerix_integrate_adaptive_simpson(badfun,NULL,0,1,&o,10,&r,&e)==NUMERIX_STATUS_NUMERICAL_FAILURE);}
static void optimization(void){numerix_result r;numerix_error e;numerix_options o;numerix_vector initial,*solution=NULL;numerix_options_init(&o);CHECK(numerix_optimize_golden_section(quad,NULL,-5,5,&o,&r,&e)==0);CHECK(NEAR(r.value,1.25,1e-6));{double values[]={8,-2};initial.size=2;initial.data=values;CHECK(numerix_optimize_gradient_descent(objective,gradient,NULL,&initial,0.5,&o,&solution,&r,&e)==0);CHECK(solution&&NEAR(solution->data[0],1,1e-5)&&NEAR(solution->data[1],2,1e-5));numerix_vector_destroy(solution);}}
static void ode(void){double y0[]={1},y2[]={1,2};numerix_ode_solution s;numerix_error e;CHECK(numerix_ode_euler(ode_exp,NULL,0,y0,1,0.01,100,&s,&e)==0);CHECK(NEAR(s.y[100],exp(1),0.01));numerix_ode_solution_destroy(&s);CHECK(numerix_ode_heun(ode_exp,NULL,0,y0,1,0.1,10,&s,&e)==0);CHECK(NEAR(s.y[10],exp(1),0.002));numerix_ode_solution_destroy(&s);CHECK(numerix_ode_rk4(ode_exp,NULL,0,y0,1,0.1,10,&s,&e)==0);CHECK(NEAR(s.y[10],exp(1),1e-5));CHECK(s.function_evaluations==40);numerix_ode_solution_destroy(&s);CHECK(numerix_ode_rk4(ode_exp,NULL,0,y2,2,0.1,10,&s,&e)==0);CHECK(NEAR(s.y[21],2*exp(1),1e-5));numerix_ode_solution_destroy(&s);CHECK(numerix_ode_rk4(ode_exp,NULL,0,y0,1,0,10,&s,&e)==NUMERIX_STATUS_INVALID_ARGUMENT);}
int main(int argc,char**argv){int all=argc==1;if(all||strcmp(argv[1],"root_finding")==0)roots();if(all||strcmp(argv[1],"linear_algebra")==0)linear();if(all||strcmp(argv[1],"differentiation")==0)differentiation();if(all||strcmp(argv[1],"interpolation")==0)interpolation();if(all||strcmp(argv[1],"integration")==0)integration();if(all||strcmp(argv[1],"optimization")==0)optimization();if(all||strcmp(argv[1],"ode")==0)ode();if(!all&&strcmp(argv[1],"root_finding")&&strcmp(argv[1],"linear_algebra")&&strcmp(argv[1],"differentiation")&&strcmp(argv[1],"interpolation")&&strcmp(argv[1],"integration")&&strcmp(argv[1],"optimization")&&strcmp(argv[1],"ode")){fprintf(stderr,"unknown test category: %s\n",argv[1]);return EXIT_FAILURE;}if(failures){fprintf(stderr,"%d Phase 1 assertions failed\n",failures);return EXIT_FAILURE;}puts("Phase 1 category passed.");return EXIT_SUCCESS;}
