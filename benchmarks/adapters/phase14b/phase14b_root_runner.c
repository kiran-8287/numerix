#define main phase14_legacy_main
#include "../phase14/phase14_runner.c"
#undef main

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279502884
#endif

/* Phase 14B-only method-level root adapter. The included Phase 14 source is
 * read-only and supplies the frozen workload parser and callback definitions. */

typedef struct root_discovery_metrics {
    size_t expected;
    size_t discovered;
    size_t missed;
    size_t false_positives;
    size_t unresolved;
    double max_location_error;
    int heuristic;
} root_discovery_metrics;

static int root_count_work;
static FILE *root_discovery_detail_file;
static const char *root_detail_run_id;
static const char *root_detail_library;
static const char *root_detail_method;
static void root_write_discovery_detail(const p14_problem *p,size_t index,
    const char *record_type,double location,double lower,double upper,
    const char *kind,int verified,int has_multiplicity,unsigned int multiplicity,
    int confidence,double residual,const char *unresolved_reason);

static double phase14b_root_value(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    if(p->model==100) return x<p->parameter[0]?-1.0:1.0; /* jump; no zero exists */
    if(p->model==101) return NAN; /* callback failure fixture */
    return p14_root_value(x,data);
}

static int phase14b_has_negative_sign(double value)
{
    return copysign(1.0, value) < 0.0;
}

static double phase14b_root_derivative(double x, void *data)
{
    p14_problem *p=(p14_problem *)data;
    if(p->model==100||p->model==101) return NAN;
    return p14_root_derivative(x,data);
}

#ifdef P14_HAVE_GSL
static double phase14b_gsl_root_callback(double x,void *data)
{ return phase14b_root_value(x,data); }
static double phase14b_gsl_root_derivative(double x,void *data)
{ return phase14b_root_derivative(x,data); }
static void phase14b_gsl_root_fdf(double x,void *data,double *f,double *df)
{ *f=phase14b_root_value(x,data); *df=phase14b_root_derivative(x,data); }
#endif

static size_t root_iteration_cap(const p14_problem *p)
{ return strstr(p->id,"MAXITER1")!=NULL?1U:500U; }

static size_t root_evaluation_cap(const p14_problem *p)
{ return strstr(p->id,"EVALCAP1")!=NULL?1U:100000U; }

static numerix_status root_cancel_progress(const numerix_root_search_report *report,void *data)
{
    size_t threshold=data?*(const size_t *)data:8U;
    return report->function_evaluations>=threshold?NUMERIX_STATUS_CANCELLED:NUMERIX_STATUS_SUCCESS;
}

static int root_expected_locations(const p14_problem *p, double *roots,
                                   size_t capacity, size_t *count)
{
    size_t n = 0;
    double center = p->parameter[0];
    if (p->model == 1) {
        if (n < capacity) roots[n++] = center;
        if (center + p->parameter[3] >= p->lower &&
            center + p->parameter[3] <= p->upper && n < capacity)
            roots[n++] = center + p->parameter[3];
    } else if (p->model == 100 || p->model == 101) {
        n=0;
    } else if (p->model == 4) {
        double k = p->parameter[2];
        long first, last;
        if (!(k > 0.0) || !isfinite(k)) return 0;
        first = (long)ceil((p->lower - center) * k / M_PI);
        last = (long)floor((p->upper - center) * k / M_PI);
        for (long j = first; j <= last && n < capacity; ++j)
            roots[n++] = center + (double)j * M_PI / k;
    } else {
        if (center >= p->lower && center <= p->upper && n < capacity)
            roots[n++] = center;
    }
    *count = n;
    return 1;
}

static int root_is_discovery(const char *method)
{
    return strcmp(method, "multiple_root_discovery") == 0;
}

static int root_is_direct(const char *method)
{
    return strcmp(method, "bisection") == 0 || strcmp(method, "regula_falsi") == 0 ||
           strcmp(method, "secant") == 0 || strcmp(method, "newton") == 0 ||
           strcmp(method, "safeguarded_newton") == 0 || strcmp(method, "brent") == 0;
}

static numerix_status root_numerix_once(p14_problem *p, const char *method,
                                        p14_outcome *out,
                                        root_discovery_metrics *dm)
{
    numerix_options options;
    numerix_error error;
    numerix_result result;
    numerix_status status = NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    p14_outcome_init(out);
    memset(&result, 0, sizeof(result));
    memset(&error, 0, sizeof(error));
    if (dm != NULL) memset(dm, 0, sizeof(*dm));
    p14_options(&options, p->absolute_tolerance, p->relative_tolerance);
    options.maximum_iterations = root_iteration_cap(p);
    options.maximum_function_evaluations = root_evaluation_cap(p);
    p14_active_problem = p;
    if (strcmp(method, "bisection") == 0)
        status = numerix_root_bisection(phase14b_root_value,p,p->lower,p->upper,&options,&result,&error);
    else if (strcmp(method, "regula_falsi") == 0)
        status = numerix_root_regula_falsi(phase14b_root_value,p,p->lower,p->upper,&options,&result,&error);
    else if (strcmp(method, "secant") == 0)
        status = numerix_root_secant(phase14b_root_value,p,p->initial,p->second_initial,&options,&result,&error);
    else if (strcmp(method, "newton") == 0)
        status = numerix_root_newton(phase14b_root_value,phase14b_root_derivative,p,p->initial,&options,&result,&error);
    else if (strcmp(method, "safeguarded_newton") == 0)
        status = numerix_root_safeguarded_newton(phase14b_root_value,phase14b_root_derivative,p,
                    p->lower,p->upper,&options,&result,&error);
    else if (strcmp(method, "brent") == 0)
        status = numerix_root_brent(phase14b_root_value,p,p->lower,p->upper,&options,&result,&error);
    else if (strcmp(method, "auto_selection") == 0 || strcmp(method, "portfolio") == 0 ||
             root_is_discovery(method)) {
        numerix_recommendation_request request;
        memset(&request, 0, sizeof(request));
        request.category = NUMERIX_PROBLEM_ROOT_FINDING;
        request.representation = NUMERIX_REPRESENTATION_CALLBACK;
        request.function = phase14b_root_value;
        request.user_data = p;
        request.derivative = phase14b_root_derivative;
        request.options = &options;
        request.problem.root.lower = p->lower;
        request.problem.root.upper = p->upper;
        request.problem.root.initial = p->initial;
        request.problem.root.second_initial = p->second_initial;
        request.problem.root.has_bracket = 1;
        request.problem.root.has_initial = 1;
        request.problem.root.has_second_initial = 1;
        if (strcmp(method, "auto_selection") == 0) {
            numerix_root_selection_options selection = { NUMERIX_SELECTION_AUTOMATIC,
                                                         NUMERIX_ALGORITHM_NONE, 1 };
            numerix_root_selection_report report;
            memset(&report, 0, sizeof(report));
            status = numerix_root_solve_auto(&request,&selection,&result,&report,&error);
            out->iterations = report.total_iterations;
            out->function_evaluations = report.total_function_evaluations;
            out->estimate = result.estimated_error;
            snprintf(out->method,sizeof(out->method),"numerix_auto_selection");
        } else if (strcmp(method, "portfolio") == 0) {
            numerix_root_portfolio_options portfolio;
            numerix_root_portfolio_report report;
            portfolio.algorithm_mask = NUMERIX_ROOT_ALGORITHM_MASK(NUMERIX_ALGORITHM_BISECTION) |
                NUMERIX_ROOT_ALGORITHM_MASK(NUMERIX_ALGORITHM_REGULA_FALSI) |
                NUMERIX_ROOT_ALGORITHM_MASK(NUMERIX_ALGORITHM_SECANT) |
                NUMERIX_ROOT_ALGORITHM_MASK(NUMERIX_ALGORITHM_NEWTON) |
                NUMERIX_ROOT_ALGORITHM_MASK(NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON) |
                NUMERIX_ROOT_ALGORITHM_MASK(NUMERIX_ALGORITHM_BRENT);
            memset(&report, 0, sizeof(report));
            status = numerix_root_solve_portfolio(&request,&portfolio,&result,&report,&error);
            out->iterations = report.total_iterations;
            out->function_evaluations = report.total_function_evaluations;
            out->estimate = result.estimated_error;
            snprintf(out->method,sizeof(out->method),"numerix_solver_portfolio");
        } else {
            numerix_root_discovery_options discovery;
            numerix_root_search_report report;
            double expected[128];
            size_t expected_count = 0;
            numerix_root_discovery_options_init(&discovery);
            discovery.initial_panels = 512;
            discovery.maximum_depth = 24;
            discovery.maximum_subdivisions = 65536;
            discovery.maximum_candidates = 4096;
            discovery.residual_tolerance = fmax(p->absolute_tolerance, 1e-12);
            discovery.detect_tangential = 1;
            discovery.estimate_multiplicity = 1;
            discovery.estimate_condition = 1;
            if(strstr(p->id,"CANCEL")!=NULL) {
                static const size_t cancel_after=8U;
                discovery.progress_interval=1U;
                discovery.progress_callback=root_cancel_progress;
                discovery.progress_user_data=(void *)&cancel_after;
            }
            memset(&report, 0, sizeof(report));
            status = numerix_root_find_all(&request,p->lower,p->upper,&discovery,&report,&error);
            if (dm != NULL) {
                (void)root_expected_locations(p,expected,128,&expected_count);
                dm->expected=expected_count; dm->discovered=report.root_count;
                dm->unresolved=report.unresolved_count; dm->heuristic=report.search_inconclusive;
                dm->max_location_error=0.0;
                for (size_t i=0;i<expected_count;i++) {
                    double nearest=INFINITY;
                    for (size_t j=0;j<report.root_count;j++)
                        nearest=fmin(nearest,fabs(expected[i]-report.roots[j].result.value));
                    if (!isfinite(nearest) || nearest > p->absolute_tolerance+
                        p->relative_tolerance*fabs(expected[i])) ++dm->missed;
                    else dm->max_location_error=fmax(dm->max_location_error,nearest);
                }
                dm->false_positives = report.root_count > expected_count ? report.root_count-expected_count : 0;
                out->error=dm->missed ? INFINITY : dm->max_location_error;
                out->reference_norm=1.0;
                out->secondary_error=(double)dm->false_positives;
                out->residual=0.0;
                for(size_t j=0;j<report.root_count;j++)
                    out->residual=fmax(out->residual,fabs(phase14b_root_value(report.roots[j].result.value,p)));
                out->iterations=report.refinement_iterations;
                out->function_evaluations=report.function_evaluations;
                out->estimate=report.elapsed_seconds;
            for(size_t j=0;j<report.root_count;j++) {
                const numerix_discovered_root *root=&report.roots[j];
                root_write_discovery_detail(p,j,"root",root->result.value,
                    root->candidate_lower,root->candidate_upper,
                    numerix_algorithm_name(root->refinement_algorithm),
                    root->verification==NUMERIX_ROOT_RESIDUAL_VERIFIED,
                    root->has_multiplicity_estimate,root->estimated_multiplicity,
                    (int)root->multiplicity_confidence,
                    fabs(phase14b_root_value(root->result.value,p)),"");
            }
            for(size_t j=0;j<report.stored_unresolved_count;j++) {
                const numerix_root_unresolved_interval *interval=&report.unresolved[j];
                root_write_discovery_detail(p,j,"unresolved",NAN,interval->lower,
                    interval->upper,"",0,0,0,0,NAN,numerix_status_string(interval->reason));
            }
            }
            out->value=(double)report.root_count;
            snprintf(out->method,sizeof(out->method),"numerix_multiple_root_discovery");
            out->status=(int)status; out->native_status=(int)status;
            out->converged=status==NUMERIX_STATUS_SUCCESS;
            out->function_evaluations=report.function_evaluations;
            numerix_root_search_report_destroy(&report);
            return status;
        }
    }
    out->status=(int)status; out->native_status=(int)status;
    out->converged=status==NUMERIX_STATUS_SUCCESS;
    out->value=result.value; out->iterations=out->iterations ? out->iterations : result.iterations;
    out->function_evaluations=out->function_evaluations ? out->function_evaluations : result.function_evaluations;
    out->estimate=out->estimate ? out->estimate : result.estimated_error;
    out->error=fabs(result.value-p->reference[0]); out->reference_norm=fabs(p->reference[0]);
    out->residual=fabs(phase14b_root_value(result.value,p)); out->secondary_error=out->residual;
    if (out->method[0]=='\0') snprintf(out->method,sizeof(out->method),"numerix_%s",method);
    return status;
}

#ifdef P14_HAVE_GSL
static numerix_status root_gsl_once(p14_problem *p, const char *method,
                                    p14_outcome *out,
                                    root_discovery_metrics *dm)
{
    int status=GSL_EUNSUP;
    p14_outcome_init(out); p14_active_problem=p; gsl_set_error_handler_off();
    if (strcmp(method,"bisection")==0 || strcmp(method,"regula_falsi")==0 ||
        strcmp(method,"brent")==0 || strcmp(method,"safeguarded_newton")==0) {
        int reached_tolerance=0;
        const gsl_root_fsolver_type *type = strcmp(method,"bisection")==0 ? gsl_root_fsolver_bisection :
            strcmp(method,"regula_falsi")==0 ? gsl_root_fsolver_falsepos : gsl_root_fsolver_brent;
        gsl_root_fsolver *solver=gsl_root_fsolver_alloc(type);
        gsl_function f={phase14b_gsl_root_callback,p};
        status=solver ? gsl_root_fsolver_set(solver,&f,p->lower,p->upper) : GSL_ENOMEM;
        for(size_t it=0; status==GSL_SUCCESS && it<root_iteration_cap(p); ++it) {
            status=gsl_root_fsolver_iterate(solver);
            if(status!=GSL_SUCCESS) break;
            out->value=gsl_root_fsolver_root(solver); out->iterations=it+1;
            {
                int test=gsl_root_test_interval(gsl_root_fsolver_x_lower(solver),
                gsl_root_fsolver_x_upper(solver),p->absolute_tolerance,p->relative_tolerance);
                if(test==GSL_SUCCESS) { reached_tolerance=1; break; }
                if(test!=GSL_CONTINUE) { status=test; break; }
                status=GSL_SUCCESS;
            }
        }
        if(status==GSL_SUCCESS && !reached_tolerance) status=GSL_EMAXITER;
        if(solver)gsl_root_fsolver_free(solver);
        snprintf(out->method,sizeof(out->method),"gsl_%s",strcmp(method,"safeguarded_newton")==0?"brent_best_available":method);
    } else if(strcmp(method,"newton")==0 || strcmp(method,"secant")==0) {
        int reached_tolerance=0;
        const gsl_root_fdfsolver_type *type=strcmp(method,"newton")==0 ?
            gsl_root_fdfsolver_newton : gsl_root_fdfsolver_secant;
        gsl_root_fdfsolver *solver=gsl_root_fdfsolver_alloc(type);
        gsl_function_fdf fdf={phase14b_gsl_root_callback,phase14b_gsl_root_derivative,phase14b_gsl_root_fdf,p};
        double x=p->initial, previous;
        status=solver ? gsl_root_fdfsolver_set(solver,&fdf,x) : GSL_ENOMEM;
        for(size_t it=0; status==GSL_SUCCESS && it<root_iteration_cap(p); ++it) {
            previous=x; status=gsl_root_fdfsolver_iterate(solver);
            if(status!=GSL_SUCCESS) break;
            x=gsl_root_fdfsolver_root(solver); out->value=x; out->iterations=it+1;
            {
                int test=gsl_root_test_delta(x,previous,p->absolute_tolerance,p->relative_tolerance);
                if(test==GSL_SUCCESS) { reached_tolerance=1; break; }
                if(test!=GSL_CONTINUE) { status=test; break; }
                status=GSL_SUCCESS;
            }
        }
        if(status==GSL_SUCCESS && !reached_tolerance)status=GSL_EMAXITER;
        if(solver)gsl_root_fdfsolver_free(solver);
        snprintf(out->method,sizeof(out->method),"gsl_%s",method);
    } else if(root_is_discovery(method)) {
        double known[128], found_locations[128]; size_t expected=0; size_t found=0, missed=0, falsepos=0;
        if(!isfinite(p->lower)||!isfinite(p->upper)||!(p->lower<p->upper)) {
            status=GSL_EINVAL;
            out->status=NUMERIX_STATUS_INVALID_INPUT;out->native_status=status;
            out->converged=0;out->function_evaluations=p14_function_calls;
            snprintf(out->method,sizeof(out->method),"gsl_4096_panel_brent_scan_heuristic");
            return NUMERIX_STATUS_INVALID_INPUT;
        }
        double previous_x=p->lower, previous_f=phase14b_root_value(previous_x,p);
        (void)root_expected_locations(p,known,128,&expected);
        for(size_t i=1;i<=4096;i++) {
            double x=p->lower+(p->upper-p->lower)*(double)i/4096.0;
            double fx=phase14b_root_value(x,p);
            if(isfinite(previous_f)&&isfinite(fx) &&
               (fx==0.0 || phase14b_has_negative_sign(fx)!=
                               phase14b_has_negative_sign(previous_f))) {
                double root=x;
                if(fx!=0.0) {
                    gsl_root_fsolver *solver=gsl_root_fsolver_alloc(gsl_root_fsolver_brent);
                    gsl_function f={phase14b_gsl_root_callback,p};
                    int st=solver?gsl_root_fsolver_set(solver,&f,previous_x,x):GSL_ENOMEM;
                    for(size_t it=0;st==GSL_SUCCESS&&it<200;it++) {
                        st=gsl_root_fsolver_iterate(solver); if(st!=GSL_SUCCESS)break;
                        root=gsl_root_fsolver_root(solver);
                        st=gsl_root_test_interval(gsl_root_fsolver_x_lower(solver),gsl_root_fsolver_x_upper(solver),1e-12,1e-12);
                    if(st==GSL_SUCCESS) break;
                    if(st!=GSL_CONTINUE) break;
                    }
                    if(solver)gsl_root_fsolver_free(solver);
                }
                if(found==0 || fabs(root-found_locations[found-1])>1e-7*(1.0+fabs(root))) {
                    if(found<128) found_locations[found++]=root;
                }
            }
            previous_x=x; previous_f=fx;
        }
        for(size_t i=0;i<expected;i++) {
            double nearest=INFINITY;
            for(size_t j=0;j<found;j++) nearest=fmin(nearest,fabs(known[i]-found_locations[j]));
            if(nearest>1e-7*(1.0+fabs(known[i])))++missed;
        }
        for(size_t j=0;j<found;j++) {
            double nearest=INFINITY;
            for(size_t i=0;i<expected;i++)nearest=fmin(nearest,fabs(found_locations[j]-known[i]));
            if(nearest>1e-7*(1.0+fabs(found_locations[j])))++falsepos;
        }
        if(dm){dm->expected=expected;dm->discovered=found;dm->missed=missed;dm->false_positives=falsepos;dm->heuristic=1;dm->max_location_error=missed?INFINITY:0.0;}
        out->value=(double)found;out->error=missed?INFINITY:0.0;out->reference_norm=1.0;
        out->secondary_error=(double)falsepos;out->residual=0.0;
        for(size_t j=0;j<found;j++)out->residual=fmax(out->residual,fabs(phase14b_root_value(found_locations[j],p)));
        for(size_t j=0;j<found;j++)
            root_write_discovery_detail(p,j,"root",found_locations[j],NAN,NAN,
                "fixed_resolution_sign_change_scan",0,0,0,0,
                fabs(phase14b_root_value(found_locations[j],p)),"");
        out->status=GSL_SUCCESS;out->native_status=status=GSL_SUCCESS;
        out->converged=1;out->function_evaluations=p14_function_calls;
        snprintf(out->method,sizeof(out->method),"gsl_4096_panel_brent_scan_heuristic");
    }
    if (!root_is_discovery(method)) {
        out->native_status=status;
        out->status=status==GSL_SUCCESS?NUMERIX_STATUS_SUCCESS:
            status==GSL_EINVAL?NUMERIX_STATUS_INVALID_INPUT:
            status==GSL_ENOMEM?NUMERIX_STATUS_ALLOCATION_FAILURE:
            status==GSL_EMAXITER?NUMERIX_STATUS_LIMIT_REACHED:
            NUMERIX_STATUS_CONVERGENCE_FAILURE;
        out->converged=(status==GSL_SUCCESS);
        out->error=fabs(out->value-p->reference[0]);out->reference_norm=fabs(p->reference[0]);
        out->residual=fabs(phase14b_root_value(out->value,p));out->secondary_error=out->residual;
    }
    out->function_evaluations=p14_function_calls;
    out->iterations=out->iterations;
    return (numerix_status)(status==GSL_SUCCESS?NUMERIX_STATUS_SUCCESS:
        status==GSL_EINVAL?NUMERIX_STATUS_INVALID_INPUT:NUMERIX_STATUS_CONVERGENCE_FAILURE);
}
#endif

static numerix_status root_once(p14_problem *p,const char *library,const char *method,
                                p14_outcome *out,root_discovery_metrics *dm)
{
    p14_count_work=root_count_work;p14_function_calls=p14_gradient_calls=0;
    if(dm!=NULL)memset(dm,0,sizeof(*dm));
    if(strcmp(library,"NUMERIX")==0)
        return root_numerix_once(p,method,out,dm);
#ifdef P14_HAVE_GSL
    if(strcmp(library,"GSL")==0)
        return root_gsl_once(p,method,out,dm);
#endif
#ifdef P14_HAVE_CMINPACK
    if(strcmp(library,"CMINPACK")==0 && strcmp(method,"newton")==0) {
        int status=p14_run_cminpack(p,out);
        out->native_status=status==NUMERIX_STATUS_SUCCESS?1:out->native_status;
        out->status=status==NUMERIX_STATUS_SUCCESS?NUMERIX_STATUS_SUCCESS:NUMERIX_STATUS_CONVERGENCE_FAILURE;
        out->converged=status==NUMERIX_STATUS_SUCCESS;
        snprintf(out->method,sizeof(out->method),"cminpack_hybrd1_best_available");
        return status==0?NUMERIX_STATUS_SUCCESS:NUMERIX_STATUS_CONVERGENCE_FAILURE;
    }
#endif
    p14_outcome_init(out);out->status=NUMERIX_STATUS_UNSUPPORTED_OPERATION;
    out->native_status=out->status;out->converged=0;
    snprintf(out->method,sizeof(out->method),"unsupported");
    return NUMERIX_STATUS_UNSUPPORTED_OPERATION;
}

static int root_applicable(p14_problem *p,const char *method)
{
    double fl=phase14b_root_value(p->lower,p), fu=phase14b_root_value(p->upper,p);
    int bracket=isfinite(fl)&&isfinite(fu)&&
        (fl==0.0||fu==0.0||phase14b_has_negative_sign(fl)!=
                              phase14b_has_negative_sign(fu));
    if(strcmp(method,"bisection")==0||strcmp(method,"regula_falsi")==0||
       strcmp(method,"brent")==0||strcmp(method,"safeguarded_newton")==0)
        return bracket && p->lower<p->upper && p->model!=100 && p->model!=101;
    if(strcmp(method,"multiple_root_discovery")==0)
        return isfinite(p->lower)&&isfinite(p->upper)&&p->lower<p->upper;
    if(strcmp(method,"auto_selection")==0||strcmp(method,"portfolio")==0)
        return isfinite(p->lower)&&isfinite(p->upper)&&p->lower<p->upper;
    if(strcmp(method,"newton")==0)return 1;
    if(strcmp(method,"secant")==0)return p->initial!=p->second_initial;
    return 1;
}

static int root_comparison_mode(const char *library,const char *method)
{
    if(strcmp(library,"NUMERIX")==0 &&
       (strcmp(method,"auto_selection")==0||strcmp(method,"portfolio")==0||root_is_discovery(method)))return 2;
    if(strcmp(library,"CMINPACK")==0)return 2;
    if(strcmp(method,"safeguarded_newton")==0&&strcmp(library,"GSL")==0)return 2;
    if(root_is_discovery(method)&&strcmp(library,"GSL")==0)return 2;
    return root_is_direct(method)?1:2;
}

static FILE *root_csv(const char *path,const char *header)
{
    FILE *f=fopen(path,"a+"); if(!f)return NULL;
    fseek(f,0,SEEK_END); if(ftell(f)==0)fprintf(f,"%s\n",header); return f;
}

static void root_write_discovery_detail(const p14_problem *p,size_t index,
    const char *record_type,double location,double lower,double upper,
    const char *kind,int verified,int has_multiplicity,unsigned int multiplicity,
    int confidence,double residual,const char *unresolved_reason)
{
    if(root_discovery_detail_file==NULL||!root_count_work)return;
    fprintf(root_discovery_detail_file,
        "phase14b-root-detail-v1,%s,%s,%s,%s,%s,%zu,%.17g,%.17g,%.17g,%s,%d,%d,%u,%d,%.17g,%s\n",
        root_detail_run_id?root_detail_run_id:"NA",
        root_detail_library?root_detail_library:"NA",
        root_detail_method?root_detail_method:"NA",p->id,record_type,index,
        location,lower,upper,kind?kind:"",verified,has_multiplicity,multiplicity,
        confidence,residual,unresolved_reason?unresolved_reason:"");
}

static int root_result_accuracy_passes(p14_problem *p,const char *method,
                                       const p14_outcome *out,
                                       const root_discovery_metrics *dm)
{
    double lo=phase14b_root_value(p->lower,p),hi=phase14b_root_value(p->upper,p);
    double fscale=0.0, residual_tolerance;
    if(!out->converged||!isfinite(out->error)||!isfinite(out->residual))return 0;
    if(out->error>p->absolute_tolerance+p->relative_tolerance*fabs(p->reference[0]))return 0;
    if(isfinite(lo))fscale=fmax(fscale,fabs(lo));
    if(isfinite(hi))fscale=fmax(fscale,fabs(hi));
    residual_tolerance=p->absolute_tolerance+p->relative_tolerance*fscale;
    if(out->residual>residual_tolerance)return 0;
    if(root_is_discovery(method)&&(!dm||dm->missed>0||dm->false_positives>0))return 0;
    return 1;
}

static int write_root_accuracy(const char *outdir,const char *runid,const char *library,
                               const char *requested,p14_problem *p,int applicable,
                               const p14_outcome *o,const root_discovery_metrics *dm)
{
    char path[1024];FILE *f;
    snprintf(path,sizeof(path),"%s/accuracy.csv",outdir);
    f=root_csv(path,"schema_version,run_id,library,requested_method,actual_method,module,problem_id,family,comparison_class,applicable,status,native_status,converged,accuracy_pass,error_kind,absolute_error,relative_error,residual,iterations,function_evaluations,derivative_evaluations,expected_roots,discovered_roots,missed_roots,false_positives,unresolved_intervals,unresolved_status,search_heuristic");
    if(!f)return 0;
    fprintf(f,"phase14b-root-v1,%s,%s,%s,%s,root,%s,%s,%s,%d,%d,%d,%d,%d,%s,%.17g,%.17g,%.17g,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%s,%d\n",
        runid,library,requested,o->method,p->id,p->family,root_comparison_mode(library,requested)==1?"direct":"best_available",
        applicable,o->status,o->native_status,o->converged,
        root_result_accuracy_passes(p,requested,o,dm),
        root_is_discovery(requested)?"root_set_location_and_count":"root_location_absolute",
        o->error,o->error/fmax(fabs(p->reference[0]),DBL_MIN),o->residual,o->iterations,o->function_evaluations,p14_gradient_calls,
        dm?dm->expected:0,dm?dm->discovered:0,dm?dm->missed:0,dm?dm->false_positives:0,dm?dm->unresolved:0,
        !root_is_discovery(requested)?"not_applicable":strcmp(library,"NUMERIX")==0?"measured":"not_available",
        dm?dm->heuristic:0);
    fclose(f);return 1;
}

static int append_root_timing(const char *outdir,const char *runid,const char *library,
                              const char *requested,const p14_problem *p,size_t batch,
                              size_t repeats,double wall,double cpu,const p14_outcome *o)
{
    char path[1024];FILE *f;snprintf(path,sizeof(path),"%s/raw_timing.csv",outdir);
    f=root_csv(path,"schema_version,run_id,library,requested_method,actual_method,module,problem_id,family,comparison_class,batch_index,batch_size,wall_seconds,cpu_seconds,status,converged,measurement_type");
    if(!f)return 0;
    fprintf(f,"phase14b-root-v1,%s,%s,%s,%s,root,%s,%s,%s,%zu,%zu,%.17g,%.17g,%d,%d,solver_call_including_library_workspace\n",
        runid,library,requested,o->method,p->id,p->family,root_comparison_mode(library,requested)==1?"direct":"best_available",batch,repeats,wall,cpu,o->status,o->converged);
    fclose(f);return 1;
}

static int append_root_work(const char *outdir,const char *runid,const char *library,
                            const char *requested,const p14_problem *p,const p14_outcome *o)
{
    char path[1024];FILE *f;snprintf(path,sizeof(path),"%s/work_metrics.csv",outdir);
    f=root_csv(path,"schema_version,run_id,library,method,problem_id,metric,value,unit,status");if(!f)return 0;
    fprintf(f,"phase14b-root-v1,%s,%s,%s,%s,iterations,%zu,count,measured\n",runid,library,requested,p->id,o->iterations);
    fprintf(f,"phase14b-root-v1,%s,%s,%s,%s,function_evaluations,%zu,count,measured\n",runid,library,requested,p->id,o->function_evaluations);
    fprintf(f,"phase14b-root-v1,%s,%s,%s,%s,derivative_evaluations,%zu,count,measured\n",runid,library,requested,p->id,p14_gradient_calls);
    fclose(f);return 1;
}

static int run_root_method(const char *input_path,const char *library,const char *method,
                           const char *outdir,const char *runid,size_t batches,size_t repeats)
{
    FILE *in=fopen(input_path,"r");char *line=malloc(P14_LINE_CAPACITY);p14_problem p;size_t attempted=0;
    char detail_path[1024];
    if(!in||!line){if(in)fclose(in);free(line);return 2;}
    snprintf(detail_path,sizeof(detail_path),"%s/discovery_details.csv",outdir);
    root_discovery_detail_file=root_csv(detail_path,
        "schema_version,run_id,library,requested_method,problem_id,record_type,index,location,candidate_lower,candidate_upper,discovery_kind,verified,multiplicity_estimated,multiplicity,confidence,residual,unresolved_reason");
    if(root_discovery_detail_file==NULL){fclose(in);free(line);return 2;}
    root_detail_run_id=runid;root_detail_library=library;root_detail_method=method;
    while(fgets(line,(int)P14_LINE_CAPACITY,in)) {
        p14_outcome outcome;root_discovery_metrics dm,*dmp=NULL;int applicable;
        if(strncmp(line,"{",1)!=0||!strstr(line,"P14-ROOT-"))continue;
        if(!p14_parse_problem(line,&p)||p.module!=P14_ROOT)continue;
        applicable=root_applicable(&p,method);if(root_is_discovery(method))dmp=&dm;
        root_count_work=1;
        (void)root_once(&p,library,method,&outcome,dmp);++attempted;
        if(!write_root_accuracy(outdir,runid,library,method,&p,applicable,&outcome,dmp)||
           !append_root_work(outdir,runid,library,method,&p,&outcome)){p14_problem_destroy(&p);fclose(in);free(line);return 2;}
        root_count_work=0;p14_count_work=0;
        for(size_t w=0;w<2;w++){p14_outcome warm;(void)root_once(&p,library,method,&warm,NULL);}
        for(size_t b=1;b<=batches;b++) {
            double ws=p14_monotonic_seconds(),cs=p14_cpu_seconds(),we,ce;p14_outcome current;
            int status=0,converged=1;
            for(size_t r=0;r<repeats;r++) {
                root_discovery_metrics timed_dm;
                (void)root_once(&p,library,method,&current,root_is_discovery(method)?&timed_dm:NULL);
                if(current.status!=0) status=current.status;
                if(!current.converged) converged=0;
            }
            we=p14_monotonic_seconds()-ws;ce=p14_cpu_seconds()-cs;current.status=status;current.converged=converged;
            if(!append_root_timing(outdir,runid,library,method,&p,b,repeats,we,ce,&current)){
                p14_problem_destroy(&p);fclose(in);free(line);return 2;
            }
        }
        p14_problem_destroy(&p);
    }
    fclose(in);free(line);
    fclose(root_discovery_detail_file);root_discovery_detail_file=NULL;
    return attempted>0?0:(fprintf(stderr,"no matching root problems in input\n"),2);
}

int main(int argc,char **argv)
{
    size_t batches,repeats;
    if(argc!=8){fprintf(stderr,"usage: %s INPUTS.jsonl LIBRARY METHOD OUT_DIR RUN_ID BATCHES BATCH_SIZE\n",argv[0]);return 2;}
    if(strcmp(argv[3],"multiple_root_discovery")!=0&&!root_is_direct(argv[3])&&
       strcmp(argv[3],"auto_selection")!=0&&strcmp(argv[3],"portfolio")!=0){fprintf(stderr,"unknown root method: %s\n",argv[3]);return 2;}
    batches=(size_t)strtoul(argv[6],NULL,10);repeats=(size_t)strtoul(argv[7],NULL,10);
    if(batches<21||repeats==0){fprintf(stderr,"requires at least 21 batches and positive batch size\n");return 2;}
    return run_root_method(argv[1],argv[2],argv[3],argv[4],argv[5],batches,repeats);
}
