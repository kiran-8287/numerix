#ifndef NUMERIX_H
#define NUMERIX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUMERIX_VERSION_MAJOR 0
#define NUMERIX_VERSION_MINOR 1
#define NUMERIX_VERSION_PATCH 0
#define NUMERIX_VERSION_STRING "0.1.0"
#define NUMERIX_ERROR_MESSAGE_CAPACITY 256

typedef enum numerix_status {
    NUMERIX_STATUS_SUCCESS = 0,
    NUMERIX_STATUS_INVALID_ARGUMENT,
    NUMERIX_STATUS_INVALID_INPUT,
    NUMERIX_STATUS_INVALID_DIMENSIONS,
    NUMERIX_STATUS_ALLOCATION_FAILURE,
    NUMERIX_STATUS_NUMERICAL_FAILURE,
    NUMERIX_STATUS_CONVERGENCE_FAILURE,
    NUMERIX_STATUS_SINGULAR_MATRIX,
    NUMERIX_STATUS_UNSUPPORTED_OPERATION,
    NUMERIX_STATUS_CANCELLED,
    NUMERIX_STATUS_LIMIT_REACHED,
    NUMERIX_STATUS_INTERNAL_ERROR
} numerix_status;

/* Error output pointers may be NULL; status return values remain authoritative. */
typedef struct numerix_error {
    numerix_status code;
    char message[NUMERIX_ERROR_MESSAGE_CAPACITY];
    double value;
    int has_value;
} numerix_error;

typedef double (*numerix_function)(double x, void *user_data);

typedef struct numerix_scalar_problem {
    numerix_function function;
    void *user_data; /* Borrowed; the caller retains ownership. */
} numerix_scalar_problem;

typedef struct numerix_options {
    double absolute_tolerance;
    double relative_tolerance;
    size_t maximum_iterations;
    size_t maximum_function_evaluations;
    double maximum_execution_time_seconds; /* 0 disables the time limit. */
} numerix_options;

typedef struct numerix_result {
    numerix_status status;
    double value;
    double estimated_error;
    double residual;
    size_t iterations;
    size_t function_evaluations;
    double execution_time_seconds;
    const char *algorithm; /* Borrowed static string or NULL. */
} numerix_result;

typedef struct numerix_context numerix_context;

/* Phase 1: scalar root finding. result is written on success and failure. */
numerix_status numerix_root_bisection(numerix_function function, void *user_data,
    double lower, double upper, const numerix_options *options,
    numerix_result *result, numerix_error *error);
numerix_status numerix_root_regula_falsi(numerix_function function, void *user_data,
    double lower, double upper, const numerix_options *options,
    numerix_result *result, numerix_error *error);
numerix_status numerix_root_secant(numerix_function function, void *user_data,
    double x0, double x1, const numerix_options *options,
    numerix_result *result, numerix_error *error);
numerix_status numerix_root_newton(numerix_function function,
    numerix_function derivative, void *user_data, double initial,
    const numerix_options *options, numerix_result *result, numerix_error *error);
numerix_status numerix_root_safeguarded_newton(numerix_function function,
    numerix_function derivative, void *user_data, double lower, double upper,
    const numerix_options *options, numerix_result *result, numerix_error *error);
numerix_status numerix_root_brent(numerix_function function, void *user_data,
    double lower, double upper, const numerix_options *options,
    numerix_result *result, numerix_error *error);

/* Dynamically sized dense vector and row-major matrix types. */
typedef struct numerix_vector { size_t size; double *data; } numerix_vector;
typedef struct numerix_matrix { size_t rows; size_t columns; double *data; } numerix_matrix;
typedef struct numerix_lu { size_t size; double *data; size_t *permutation; int sign; double threshold; } numerix_lu;
numerix_status numerix_vector_create(size_t size, numerix_vector **out, numerix_error *error);
void numerix_vector_destroy(numerix_vector *vector);
numerix_status numerix_vector_copy(const numerix_vector *a, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_add(const numerix_vector *a, const numerix_vector *b, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_subtract(const numerix_vector *a, const numerix_vector *b, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_scale(const numerix_vector *a, double scalar, numerix_vector **out, numerix_error *error);
numerix_status numerix_vector_dot(const numerix_vector *a, const numerix_vector *b, double *out, numerix_error *error);
double numerix_vector_norm2(const numerix_vector *a);
double numerix_vector_norm_inf(const numerix_vector *a);
numerix_status numerix_matrix_create(size_t rows, size_t columns, numerix_matrix **out, numerix_error *error);
void numerix_matrix_destroy(numerix_matrix *matrix);
numerix_status numerix_matrix_copy(const numerix_matrix *a, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_add(const numerix_matrix *a, const numerix_matrix *b, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_subtract(const numerix_matrix *a, const numerix_matrix *b, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_scale(const numerix_matrix *a, double scalar, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_multiply(const numerix_matrix *a, const numerix_matrix *b, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_transpose(const numerix_matrix *a, numerix_matrix **out, numerix_error *error);
numerix_status numerix_matrix_vector_multiply(const numerix_matrix *a, const numerix_vector *x, numerix_vector **out, numerix_error *error);
numerix_status numerix_linear_solve_gaussian(const numerix_matrix *a, const numerix_vector *b, const numerix_options *options, numerix_vector **x, double *residual, numerix_error *error);
numerix_status numerix_linear_solve_gauss_jordan(const numerix_matrix *a, const numerix_vector *b, const numerix_options *options, numerix_vector **x, double *residual, numerix_error *error);
numerix_status numerix_lu_decompose(const numerix_matrix *a, const numerix_options *options, numerix_lu *lu, numerix_error *error);
void numerix_lu_destroy(numerix_lu *lu);
numerix_status numerix_lu_solve(const numerix_lu *lu, const numerix_vector *b, numerix_vector **x, numerix_error *error);

/* Numerical differentiation; h must be positive and finite. */
numerix_status numerix_diff_forward(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_backward(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_central(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_central_five_point(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_richardson(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_diff_second(numerix_function f, void *data, double x, double h, const numerix_options *o, numerix_result *r, numerix_error *e);

/* Interpolation uses distinct finite x values. Extrapolation is opt-in. */
typedef struct numerix_interpolant { size_t size; double *x; double *coefficients; double minimum; double maximum; } numerix_interpolant;
numerix_status numerix_interp_linear(double x0, double y0, double x1, double y1, double x, numerix_result *r, numerix_error *e);
numerix_status numerix_interp_lagrange(const double *x, const double *y, size_t n, double query, int allow_extrapolation, numerix_result *r, numerix_error *e);
numerix_status numerix_interp_newton_create(const double *x, const double *y, size_t n, numerix_interpolant *p, numerix_error *e);
numerix_status numerix_interp_newton_evaluate(const numerix_interpolant *p, double query, int allow_extrapolation, numerix_result *r, numerix_error *e);
void numerix_interpolant_destroy(numerix_interpolant *p);

/* Finite-interval integration. Fixed rules use caller-selected panels. */
numerix_status numerix_integrate_trapezoid(numerix_function f, void *data, double a, double b, size_t panels, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_integrate_simpson(numerix_function f, void *data, double a, double b, size_t panels, const numerix_options *o, numerix_result *r, numerix_error *e);
numerix_status numerix_integrate_adaptive_simpson(numerix_function f, void *data, double a, double b, const numerix_options *o, size_t max_depth, numerix_result *r, numerix_error *e);

/* Bounded one-dimensional minimization. */
numerix_status numerix_optimize_golden_section(numerix_function f, void *data, double lower, double upper, const numerix_options *o, numerix_result *r, numerix_error *e);
typedef double (*numerix_objective_function)(const double *x, size_t dimension, void *user_data);
typedef numerix_status (*numerix_gradient_function)(const double *x, size_t dimension, double *gradient, void *user_data);
numerix_status numerix_optimize_gradient_descent(numerix_objective_function objective, numerix_gradient_function gradient, void *data, const numerix_vector *initial, double initial_step, const numerix_options *o, numerix_vector **solution, numerix_result *result, numerix_error *e);

/* Vector ODE callback writes dimension derivatives into dydx. */
typedef numerix_status (*numerix_ode_function)(double x, const double *y, size_t dimension, double *dydx, void *user_data);
typedef struct numerix_ode_solution { numerix_status status; size_t points; size_t dimension; double *x; double *y; size_t function_evaluations; } numerix_ode_solution;
numerix_status numerix_ode_euler(numerix_ode_function f, void *data, double x0, const double *y0, size_t dimension, double step, size_t steps, numerix_ode_solution *out, numerix_error *e);
numerix_status numerix_ode_heun(numerix_ode_function f, void *data, double x0, const double *y0, size_t dimension, double step, size_t steps, numerix_ode_solution *out, numerix_error *e);
numerix_status numerix_ode_rk4(numerix_ode_function f, void *data, double x0, const double *y0, size_t dimension, double step, size_t steps, numerix_ode_solution *out, numerix_error *e);
void numerix_ode_solution_destroy(numerix_ode_solution *solution);

/*
 * Compiled expressions are immutable and may be evaluated repeatedly.
 * The caller owns an expression returned by compile and must destroy it.
 */
typedef struct numerix_expression numerix_expression;

numerix_status numerix_expression_compile(
    const char *source,
    numerix_expression **expression,
    numerix_error *error);

numerix_status numerix_expression_evaluate(
    const numerix_expression *expression,
    double x,
    double *value,
    numerix_error *error);

void numerix_expression_destroy(numerix_expression *expression);

/*
 * A callback context borrows its expression. Use one context per active
 * numerical call; it records the latest evaluation error for inspection.
 */
typedef struct numerix_expression_callback_context {
    const numerix_expression *expression;
    numerix_status last_status;
    numerix_error last_error;
} numerix_expression_callback_context;

numerix_status numerix_expression_callback_context_init(
    numerix_expression_callback_context *context,
    const numerix_expression *expression,
    numerix_error *error);

double numerix_expression_callback(
    double x,
    void *user_data);

numerix_status numerix_expression_callback_status(
    const numerix_expression_callback_context *context);

const numerix_error *numerix_expression_callback_error(
    const numerix_expression_callback_context *context);

/* Phase 3: ordered, non-overlapping piecewise expression functions. */
typedef struct numerix_piecewise_function numerix_piecewise_function;

typedef enum numerix_discontinuity_type {
    NUMERIX_DISCONTINUITY_CONTINUOUS = 0,
    NUMERIX_DISCONTINUITY_JUMP,
    NUMERIX_DISCONTINUITY_REMOVABLE,
    NUMERIX_DISCONTINUITY_INFINITE,
    NUMERIX_DISCONTINUITY_UNKNOWN
} numerix_discontinuity_type;

typedef enum numerix_diagnostic_confidence {
    NUMERIX_DIAGNOSTIC_INCONCLUSIVE = 0,
    NUMERIX_DIAGNOSTIC_SUSPECTED,
    NUMERIX_DIAGNOSTIC_NUMERICALLY_CONFIRMED
} numerix_diagnostic_confidence;

typedef struct numerix_piecewise_boundary_report {
    double boundary;
    double left_limit;
    double right_limit;
    double function_value;
    numerix_status left_status;
    numerix_status right_status;
    numerix_status function_status;
    numerix_discontinuity_type type;
    numerix_diagnostic_confidence confidence;
} numerix_piecewise_boundary_report;

typedef struct numerix_piecewise_candidate {
    double location;
    double left_value;
    double right_value;
    numerix_status left_status;
    numerix_status right_status;
    numerix_diagnostic_confidence confidence;
} numerix_piecewise_candidate;

typedef struct numerix_piecewise_callback_context {
    const numerix_piecewise_function *function;
    numerix_status last_status;
    numerix_error last_error;
} numerix_piecewise_callback_context;

/*
 * Added expressions are compiled and owned by the piecewise object. Finish
 * building the object before starting concurrent evaluations.
 */
numerix_status numerix_piecewise_create(
    numerix_piecewise_function **function, numerix_error *error);
numerix_status numerix_piecewise_add_expression(
    numerix_piecewise_function *function, double lower_bound,
    double upper_bound, int include_lower, int include_upper,
    const char *expression, numerix_error *error);
numerix_status numerix_piecewise_evaluate(
    const numerix_piecewise_function *function, double x,
    double *value, numerix_error *error);
void numerix_piecewise_destroy(numerix_piecewise_function *function);
/* Returns the unique finite segment endpoints in increasing order. */
numerix_status numerix_piecewise_get_boundaries(
    const numerix_piecewise_function *function, double *boundaries,
    size_t capacity, size_t *boundary_count, numerix_error *error);

/* direction is -1 for a left limit and +1 for a right limit. */
numerix_status numerix_piecewise_estimate_one_sided_limit(
    const numerix_piecewise_function *function, double boundary, int direction,
    const numerix_options *options, double *limit,
    numerix_diagnostic_confidence *confidence, numerix_error *error);
numerix_status numerix_piecewise_analyze_boundary(
    const numerix_piecewise_function *function, double boundary,
    const numerix_options *options,
    numerix_piecewise_boundary_report *report, numerix_error *error);
numerix_status numerix_piecewise_analyze_boundaries(
    const numerix_piecewise_function *function, const numerix_options *options,
    numerix_piecewise_boundary_report *reports, size_t capacity,
    size_t *report_count, numerix_error *error);

/* Probe an interval uniformly; candidates are numerical suspicion only. */
numerix_status numerix_piecewise_scan_interval(
    const numerix_piecewise_function *function, double lower, double upper,
    size_t samples, numerix_piecewise_candidate *candidates,
    size_t capacity, size_t *candidate_count, numerix_error *error);

numerix_status numerix_piecewise_callback_context_init(
    numerix_piecewise_callback_context *context,
    const numerix_piecewise_function *function, numerix_error *error);
double numerix_piecewise_callback(double x, void *user_data);
numerix_status numerix_piecewise_callback_status(
    const numerix_piecewise_callback_context *context);
const numerix_error *numerix_piecewise_callback_error(
    const numerix_piecewise_callback_context *context);

/* Integration splits at known segment boundaries and rejects interval gaps. */
numerix_status numerix_piecewise_integrate_trapezoid(
    const numerix_piecewise_function *function, double lower, double upper,
    size_t panels_per_segment, const numerix_options *options,
    numerix_result *result, numerix_error *error);
/* Refuses a finite-difference stencil that crosses a suspected boundary. */
numerix_status numerix_piecewise_diff_central(
    const numerix_piecewise_function *function, double x, double step,
    const numerix_options *options, numerix_result *result,
    numerix_error *error);

/* Phase 4: explainable recommendations and root-solver orchestration. */
typedef enum numerix_problem_category {
    NUMERIX_PROBLEM_ROOT_FINDING = 0,
    NUMERIX_PROBLEM_LINEAR_ALGEBRA,
    NUMERIX_PROBLEM_DIFFERENTIATION,
    NUMERIX_PROBLEM_INTERPOLATION,
    NUMERIX_PROBLEM_INTEGRATION,
    NUMERIX_PROBLEM_OPTIMIZATION,
    NUMERIX_PROBLEM_ODE
} numerix_problem_category;

typedef enum numerix_algorithm {
    NUMERIX_ALGORITHM_NONE = 0,
    NUMERIX_ALGORITHM_BISECTION,
    NUMERIX_ALGORITHM_REGULA_FALSI,
    NUMERIX_ALGORITHM_SECANT,
    NUMERIX_ALGORITHM_NEWTON,
    NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON,
    NUMERIX_ALGORITHM_BRENT,
    NUMERIX_ALGORITHM_GAUSSIAN_ELIMINATION,
    NUMERIX_ALGORITHM_GAUSS_JORDAN,
    NUMERIX_ALGORITHM_LU,
    NUMERIX_ALGORITHM_DIFF_FORWARD,
    NUMERIX_ALGORITHM_DIFF_BACKWARD,
    NUMERIX_ALGORITHM_DIFF_CENTRAL,
    NUMERIX_ALGORITHM_DIFF_FIVE_POINT,
    NUMERIX_ALGORITHM_DIFF_RICHARDSON,
    NUMERIX_ALGORITHM_DIFF_SECOND,
    NUMERIX_ALGORITHM_PIECEWISE_DIFF_CENTRAL,
    NUMERIX_ALGORITHM_INTERP_LINEAR,
    NUMERIX_ALGORITHM_INTERP_LAGRANGE,
    NUMERIX_ALGORITHM_INTERP_NEWTON,
    NUMERIX_ALGORITHM_INTEGRATE_TRAPEZOID,
    NUMERIX_ALGORITHM_INTEGRATE_SIMPSON,
    NUMERIX_ALGORITHM_INTEGRATE_ADAPTIVE_SIMPSON,
    NUMERIX_ALGORITHM_PIECEWISE_INTEGRATE_TRAPEZOID,
    NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION,
    NUMERIX_ALGORITHM_OPTIMIZE_GRADIENT_DESCENT,
    NUMERIX_ALGORITHM_ODE_EULER,
    NUMERIX_ALGORITHM_ODE_HEUN,
    NUMERIX_ALGORITHM_ODE_RK4
} numerix_algorithm;

typedef enum numerix_problem_representation {
    NUMERIX_REPRESENTATION_CALLBACK = 0,
    NUMERIX_REPRESENTATION_EXPRESSION,
    NUMERIX_REPRESENTATION_PIECEWISE
} numerix_problem_representation;

typedef enum numerix_suitability {
    NUMERIX_SUITABILITY_UNAVAILABLE = 0,
    NUMERIX_SUITABILITY_CONDITIONAL,
    NUMERIX_SUITABILITY_SUITABLE
} numerix_suitability;

typedef struct numerix_recommendation_request {
    numerix_problem_category category;
    numerix_problem_representation representation;
    numerix_function function;
    void *user_data;
    numerix_function derivative;
    const numerix_expression *expression;
    const numerix_piecewise_function *piecewise_function;
    const numerix_options *options;
    union {
        struct {
            double lower;
            double upper;
            double initial;
            double second_initial;
            int has_bracket;
            int has_initial;
            int has_second_initial;
        } root;
        struct {
            const numerix_matrix *matrix;
            const numerix_vector *right_hand_side;
        } linear;
        struct {
            double x;
            double step;
        } differentiation;
        struct {
            const double *x;
            const double *y;
            size_t count;
            double query;
            int allow_extrapolation;
        } interpolation;
        struct {
            double lower;
            double upper;
            size_t panels;
        } integration;
        struct {
            int bounded;
            double lower;
            double upper;
            numerix_objective_function objective;
            numerix_gradient_function gradient;
            const numerix_vector *initial;
            double initial_step;
        } optimization;
        struct {
            numerix_ode_function function;
            const double *initial_state;
            size_t dimension;
            double initial_x;
            double step;
            size_t steps;
        } ode;
    } problem;
} numerix_recommendation_request;

typedef struct numerix_recommendation {
    numerix_problem_category category;
    numerix_algorithm recommended_algorithm;
    numerix_suitability suitability;
    int sign_changing_bracket;
    int bracket_was_checked;
    int derivative_available;
    int derivative_checked;
    int derivative_appears_zero_or_non_finite;
    int crosses_piecewise_boundaries;
    size_t piecewise_boundary_count;
    size_t probe_evaluations;
    size_t derivative_probe_evaluations;
    size_t alternative_count;
    numerix_algorithm alternatives[4];
    char reason[3][160];
    char limitation[2][160];
    char alternative_reason[4][120];
    int historical_diagnostics_used;
    numerix_algorithm historical_algorithm;
    char historical_reason[160];
} numerix_recommendation;

#define NUMERIX_ROOT_SELECTION_MAX_ATTEMPTS 4

typedef enum numerix_selection_mode {
    NUMERIX_SELECTION_AUTOMATIC = 0,
    NUMERIX_SELECTION_EXPLICIT
} numerix_selection_mode;

typedef struct numerix_root_selection_options {
    numerix_selection_mode mode;
    numerix_algorithm explicit_algorithm;
    int enable_fallback;
} numerix_root_selection_options;

typedef struct numerix_algorithm_attempt {
    numerix_algorithm algorithm;
    numerix_status status;
    int was_fallback;
    char reason[160];
    numerix_error error;
    numerix_result result;
} numerix_algorithm_attempt;

typedef struct numerix_root_selection_report {
    numerix_algorithm initially_selected;
    numerix_algorithm final_algorithm;
    numerix_status recommendation_status;
    numerix_error recommendation_error;
    numerix_status original_failure;
    numerix_status final_status;
    int fallback_enabled;
    int fallback_used;
    size_t attempt_count;
    size_t total_function_evaluations;
    size_t total_iterations;
    numerix_algorithm_attempt attempts[NUMERIX_ROOT_SELECTION_MAX_ATTEMPTS];
    numerix_recommendation recommendation;
} numerix_root_selection_report;

#define NUMERIX_ROOT_PORTFOLIO_MAX_ATTEMPTS 6
#define NUMERIX_ROOT_ALGORITHM_MASK(algorithm) (1U << (unsigned int)(algorithm))

typedef struct numerix_root_portfolio_options {
    unsigned int algorithm_mask;
} numerix_root_portfolio_options;

typedef struct numerix_root_portfolio_report {
    size_t attempt_count;
    size_t selected_attempt;
    int has_selected_attempt;
    int successful_results_agree;
    size_t total_function_evaluations;
    size_t total_iterations;
    size_t recommendation_probe_evaluations;
    numerix_status recommendation_status;
    numerix_error recommendation_error;
    numerix_algorithm_attempt attempts[NUMERIX_ROOT_PORTFOLIO_MAX_ATTEMPTS];
} numerix_root_portfolio_report;

numerix_status numerix_recommend_algorithm(
    const numerix_recommendation_request *request,
    numerix_recommendation *recommendation, numerix_error *error);
const char *numerix_algorithm_name(numerix_algorithm algorithm);
numerix_status numerix_root_solve_auto(
    const numerix_recommendation_request *request,
    const numerix_root_selection_options *selection_options,
    numerix_result *result, numerix_root_selection_report *report,
    numerix_error *error);
numerix_status numerix_root_solve_portfolio(
    const numerix_recommendation_request *request,
    const numerix_root_portfolio_options *portfolio_options,
    numerix_result *best_result, numerix_root_portfolio_report *report,
    numerix_error *error);

typedef struct numerix_execution_options {
    numerix_algorithm explicit_algorithm;
    int enable_root_fallback;
    size_t panels;
    size_t adaptive_depth;
} numerix_execution_options;

typedef struct numerix_execution_output {
    numerix_problem_category category;
    numerix_algorithm algorithm;
    int selected_automatically;
    numerix_recommendation recommendation;
    numerix_result scalar_result;
    numerix_vector *vector_result;
    numerix_ode_solution ode_result;
    int has_ode_result;
    numerix_root_selection_report root_selection;
} numerix_execution_output;

/* Vector and ODE outputs are owned by this object until it is destroyed. */
numerix_status numerix_execute_recommended(
    const numerix_recommendation_request *request,
    const numerix_execution_options *execution_options,
    numerix_execution_output *output, numerix_error *error);
void numerix_execution_output_destroy(numerix_execution_output *output);

/* Phase 6: post-execution numerical diagnostics. */
typedef enum numerix_reliability_class {
    NUMERIX_RELIABILITY_INCONCLUSIVE = 0,
    NUMERIX_RELIABILITY_ACCEPTABLE,
    NUMERIX_RELIABILITY_CAUTION,
    NUMERIX_RELIABILITY_LOW_CONFIDENCE,
    NUMERIX_RELIABILITY_UNRELIABLE
} numerix_reliability_class;

typedef enum numerix_diagnostic_severity {
    NUMERIX_DIAGNOSTIC_INFORMATION = 0,
    NUMERIX_DIAGNOSTIC_NOTICE,
    NUMERIX_DIAGNOSTIC_WARNING,
    NUMERIX_DIAGNOSTIC_ERROR,
    NUMERIX_DIAGNOSTIC_CRITICAL
} numerix_diagnostic_severity;

typedef enum numerix_diagnostic_category {
    NUMERIX_DIAGNOSTIC_RESIDUAL = 0,
    NUMERIX_DIAGNOSTIC_ERROR_ESTIMATION,
    NUMERIX_DIAGNOSTIC_CONDITIONING,
    NUMERIX_DIAGNOSTIC_SENSITIVITY,
    NUMERIX_DIAGNOSTIC_STABILITY,
    NUMERIX_DIAGNOSTIC_FLOATING_POINT,
    NUMERIX_DIAGNOSTIC_CONVERGENCE,
    NUMERIX_DIAGNOSTIC_RESOURCE_LIMIT,
    NUMERIX_DIAGNOSTIC_EXECUTION
} numerix_diagnostic_category;

typedef struct numerix_diagnostic_record {
    char identifier[48];
    numerix_diagnostic_severity severity;
    numerix_diagnostic_category category;
    numerix_problem_category problem_category;
    numerix_algorithm algorithm;
    char description[160];
    int has_metric;
    double metric;
    int has_threshold;
    double threshold;
    char possible_cause[120];
    char consequence[120];
    char suggested_action[120];
    int is_estimated;
    int is_heuristic;
} numerix_diagnostic_record;

typedef struct numerix_diagnostics_options {
    int estimate_linear_condition;
    int estimate_root_condition;
} numerix_diagnostics_options;

typedef struct numerix_diagnostics_report {
    numerix_problem_category category;
    numerix_status solver_status;
    numerix_algorithm algorithm;
    int has_residual;
    size_t additional_function_evaluations;
    size_t additional_derivative_evaluations;
    double absolute_residual;
    int has_residual_tolerance;
    double requested_residual_tolerance;
    int has_relative_residual;
    double relative_residual;
    int residual_satisfies_tolerance;
    int has_estimated_error;
    double estimated_absolute_error;
    int has_estimated_error_tolerance;
    double requested_error_tolerance;
    int has_relative_estimated_error;
    double relative_estimated_error;
    int estimated_error_satisfies_tolerance;
    int has_backward_error;
    double normwise_backward_error;
    int has_condition_estimate;
    double condition_estimate;
    int has_residual_vector;
    numerix_vector *residual_vector;
    int has_data_residual;
    double maximum_data_residual;
    int has_objective_value;
    double objective_value;
    int has_gradient_norm;
    double gradient_norm;
    int has_ode_summary;
    size_t ode_points;
    size_t ode_dimension;
    double ode_maximum_absolute_state;
    int ode_state_is_finite;
    numerix_reliability_class reliability;
    size_t warning_count;
    char warning[4][160];
    numerix_diagnostic_record diagnostics[4];
} numerix_diagnostics_report;

/* Applies only matching prior callback-root evidence; never changes execution overrides. */
numerix_status numerix_recommend_algorithm_with_diagnostics(
    const numerix_recommendation_request *request,
    const numerix_recommendation_request *prior_request,
    const numerix_diagnostics_report *prior_report,
    numerix_recommendation *recommendation, numerix_error *error);

/*
 * Analyze an existing execution without changing it. The report owns an
 * optional residual_vector and must be released with the destroy function.
 * Only diagnostics supported by the request and execution outputs are set.
 */
numerix_status numerix_diagnostics_analyze(
    const numerix_recommendation_request *request,
    const numerix_execution_output *output,
    const numerix_diagnostics_options *options,
    numerix_diagnostics_report *report,
    numerix_error *error);
void numerix_diagnostics_report_destroy(numerix_diagnostics_report *report);

typedef struct numerix_sensitivity_report {
    double evaluation_point;
    double perturbation;
    double derivative;
    double estimated_uncertainty;
    int has_relative_sensitivity;
    double relative_sensitivity;
    size_t function_evaluations;
    const char *method; /* Borrowed static string. */
} numerix_sensitivity_report;

/* Uses Richardson finite differences; the callback is evaluated additionally. */
numerix_status numerix_diagnostics_scalar_sensitivity(
    numerix_function function, void *user_data, double x, double step,
    const numerix_options *options, numerix_sensitivity_report *report,
    numerix_error *error);
const char *numerix_reliability_class_name(
    numerix_reliability_class reliability);

typedef struct numerix_forward_error {
    double absolute_error;
    int has_relative_error;
    double relative_error;
    const char *reference_description; /* Borrowed static string. */
} numerix_forward_error;

numerix_status numerix_diagnostics_forward_error_scalar(
    double computed, double reference, numerix_forward_error *report,
    numerix_error *error);
numerix_status numerix_diagnostics_forward_error_vector(
    const numerix_vector *computed, const numerix_vector *reference,
    numerix_forward_error *report, numerix_error *error);
numerix_status numerix_diagnostics_forward_error_matrix(
    const numerix_matrix *computed, const numerix_matrix *reference,
    numerix_forward_error *report, numerix_error *error);

typedef struct numerix_root_condition_report {
    double evaluation_point;
    double derivative;
    double derivative_uncertainty;
    double reciprocal_derivative_condition;
    double perturbation_step;
    int derivative_was_supplied;
    int condition_available;
    size_t function_evaluations;
    size_t derivative_evaluations;
} numerix_root_condition_report;

numerix_status numerix_diagnostics_root_condition(
    numerix_function function, numerix_function derivative, void *user_data,
    double root_approximation, double step, const numerix_options *options,
    numerix_root_condition_report *report, numerix_error *error);

typedef struct numerix_multivariable_sensitivity {
    size_t input_dimension;
    numerix_vector *gradient;
    int has_relative_sensitivities;
    numerix_vector *relative_sensitivities;
    double nominal_value;
    double estimated_uncertainty_norm;
    size_t function_evaluations;
    double relative_step;
    int uncertainty_is_estimated;
} numerix_multivariable_sensitivity;

/* The scalar objective callback reuses NUMERIX's optimization representation. */
numerix_status numerix_diagnostics_multivariable_sensitivity(
    numerix_objective_function objective, void *user_data,
    const numerix_vector *point, double relative_step,
    numerix_multivariable_sensitivity *report, numerix_error *error);
void numerix_multivariable_sensitivity_destroy(
    numerix_multivariable_sensitivity *report);

typedef struct numerix_error_propagation_report {
    double output_value;
    double estimated_standard_uncertainty;
    int has_relative_uncertainty;
    double relative_uncertainty;
    size_t function_evaluations;
    int assumes_independent_inputs;
    int uses_local_linearization;
} numerix_error_propagation_report;

numerix_status numerix_diagnostics_propagate_independent_uncertainty(
    numerix_objective_function objective, void *user_data,
    const numerix_vector *point, const numerix_vector *input_uncertainties,
    double relative_step, numerix_error_propagation_report *report,
    numerix_error *error);

typedef enum numerix_convergence_trend {
    NUMERIX_CONVERGENCE_INCONCLUSIVE = 0,
    NUMERIX_CONVERGENCE_CONVERGING,
    NUMERIX_CONVERGENCE_SLOW,
    NUMERIX_CONVERGENCE_OSCILLATORY,
    NUMERIX_CONVERGENCE_STAGNATING,
    NUMERIX_CONVERGENCE_DIVERGING
} numerix_convergence_trend;

typedef struct numerix_convergence_report {
    numerix_convergence_trend trend;
    size_t observations;
    int has_residual_ratio;
    double final_residual_ratio;
    int has_empirical_order;
    double empirical_residual_order;
    int rate_uses_residuals_as_error_proxies;
} numerix_convergence_report;

/* Signed residual history; output order follows chronological input order. */
numerix_status numerix_diagnostics_analyze_convergence(
    const double *residual_history, size_t observation_count,
    numerix_convergence_report *report, numerix_error *error);
const char *numerix_convergence_trend_name(numerix_convergence_trend trend);

typedef struct numerix_cancellation_report {
    double difference;
    int has_condition_indicator;
    double cancellation_condition_indicator;
    int has_lost_decimal_digits_estimate;
    double estimated_lost_decimal_digits;
    int cancellation_suspected;
} numerix_cancellation_report;

/* Observes cancellation risk for NUMERIX-controlled subtraction a - b. */
numerix_status numerix_diagnostics_check_subtraction(
    double a, double b, numerix_cancellation_report *report,
    numerix_error *error);

/* Phase 5: managed root execution and resumable Bisection sessions. */
typedef struct numerix_managed_execution numerix_managed_execution;

typedef enum numerix_execution_state
{
    NUMERIX_EXECUTION_CREATED = 0,
    NUMERIX_EXECUTION_READY,
    NUMERIX_EXECUTION_RUNNING,
    NUMERIX_EXECUTION_PAUSED,
    NUMERIX_EXECUTION_CANCELLED,
    NUMERIX_EXECUTION_COMPLETED,
    NUMERIX_EXECUTION_FAILED,
    NUMERIX_EXECUTION_LIMIT_REACHED
} numerix_execution_state;

typedef enum numerix_execution_event_type
{
    NUMERIX_EVENT_STARTED = 0,
    NUMERIX_EVENT_PAUSED,
    NUMERIX_EVENT_RESUMED,
    NUMERIX_EVENT_CANCELLED,
    NUMERIX_EVENT_LIMIT_REACHED,
    NUMERIX_EVENT_COMPLETED,
    NUMERIX_EVENT_FAILED
} numerix_execution_event_type;

enum
{
    NUMERIX_STEP_HAS_APPROXIMATION = 1U << 0,
    NUMERIX_STEP_HAS_RESIDUAL = 1U << 1,
    NUMERIX_STEP_HAS_INTERVAL = 1U << 2,
    NUMERIX_STEP_HAS_MIDPOINT = 1U << 3
};

typedef struct numerix_execution_step
{
    size_t number;
    numerix_algorithm algorithm;
    size_t valid_fields;
    size_t iteration;
    size_t function_evaluations;
    double approximation;
    double residual;
    double interval_lower;
    double interval_upper;
    double midpoint;
} numerix_execution_step;

typedef struct numerix_periodic_diagnostics numerix_periodic_diagnostics;

/*
 * Bounded residual-history observer intended for a Phase 5 step callback.
 * It performs no solver calls and is not internally synchronized.
 */
numerix_status numerix_periodic_diagnostics_create(
    size_t history_capacity, size_t every_n_observations,
    numerix_periodic_diagnostics **observer, numerix_error *error);
numerix_status numerix_periodic_diagnostics_observe(
    numerix_periodic_diagnostics *observer,
    const numerix_execution_step *step,
    numerix_convergence_report *report, int *assessment_updated,
    numerix_error *error);
numerix_status numerix_periodic_diagnostics_finish(
    numerix_periodic_diagnostics *observer,
    numerix_convergence_report *report, numerix_error *error);
void numerix_periodic_diagnostics_destroy(
    numerix_periodic_diagnostics *observer);

typedef struct numerix_execution_event
{
    numerix_execution_event_type type;
    numerix_execution_state state;
    numerix_algorithm algorithm;
    numerix_status status;
    size_t iteration;
    size_t function_evaluations;
} numerix_execution_event;

typedef void (*numerix_execution_event_callback)(
    numerix_managed_execution *execution,
    const numerix_execution_event *event, void *user_data);
typedef void (*numerix_execution_step_callback)(
    numerix_managed_execution *execution,
    const numerix_execution_step *step, void *user_data);

typedef struct numerix_managed_execution_options
{
    numerix_root_selection_options root_selection;
    numerix_execution_options execution;
    numerix_execution_event_callback event_callback;
    numerix_execution_step_callback step_callback;
    void *callback_user_data;
} numerix_managed_execution_options;

typedef struct numerix_execution_progress
{
    numerix_execution_state state;
    numerix_problem_category category;
    numerix_algorithm algorithm;
    size_t iterations;
    int iterations_available;
    size_t function_evaluations;
    size_t derivative_evaluations;
    int algorithm_available;
    int has_approximation;
    int has_residual;
    int has_estimated_error;
    double approximation;
    double residual;
    double estimated_error;
    double interval_lower;
    double interval_upper;
    int has_interval;
    double elapsed_seconds;
    int percentage_available;
} numerix_execution_progress;

typedef struct numerix_execution_summary
{
    numerix_execution_state state;
    numerix_status status;
    numerix_problem_category category;
    numerix_algorithm initially_selected;
    numerix_algorithm final_algorithm;
    size_t iterations;
    int iterations_available;
    size_t function_evaluations;
    size_t derivative_evaluations;
    size_t fallback_attempts;
    double elapsed_seconds;
    int has_result;
    numerix_result result;
    numerix_error error;
    numerix_root_selection_report root_history;
} numerix_execution_summary;

/*
 * The request and all input buffers are borrowed until destroy. All
 * implemented problem categories can be dispatched. Callbacks are synchronous;
 * step/event values are borrowed for the callback duration. Cancellation
 * requests are thread safe. Other queries and destruction must not race with
 * run or callbacks.
 */
numerix_status numerix_managed_execution_create(
    const numerix_recommendation_request *request,
    const numerix_managed_execution_options *options,
    numerix_managed_execution **execution, numerix_error *error);
/* Runs once to termination, or until a Bisection step callback requests pause. */
numerix_status numerix_managed_execution_run(
    numerix_managed_execution *execution, numerix_error *error);
/* Thread-safe request; observed at the next managed callback or safe point. */
numerix_status numerix_managed_execution_request_cancel(
    numerix_managed_execution *execution);
/* Pause is supported at Bisection step boundaries only. */
numerix_status numerix_managed_execution_request_pause(
    numerix_managed_execution *execution);
numerix_status numerix_managed_execution_resume(
    numerix_managed_execution *execution, numerix_error *error);
numerix_status numerix_managed_execution_get_progress(
    const numerix_managed_execution *execution,
    numerix_execution_progress *progress, numerix_error *error);
numerix_status numerix_managed_execution_get_summary(
    const numerix_managed_execution *execution,
    numerix_execution_summary *summary, numerix_error *error);
/* Output resources are borrowed from the execution until it is destroyed. */
numerix_status numerix_managed_execution_get_output(
    const numerix_managed_execution *execution,
    const numerix_execution_output **output, numerix_error *error);
void numerix_managed_execution_destroy(
    numerix_managed_execution *execution);

/* Phase 7: bounded numerical discovery on a finite interval. */
typedef enum numerix_root_discovery_kind
{
    NUMERIX_ROOT_SIGN_CHANGING = 0,
    NUMERIX_ROOT_TANGENTIAL,
    NUMERIX_ROOT_SAMPLED_ZERO,
    NUMERIX_ROOT_ENDPOINT,
    NUMERIX_ROOT_PIECEWISE_BOUNDARY,
    NUMERIX_ROOT_REPEATED
} numerix_root_discovery_kind;

typedef enum numerix_root_verification
{
    NUMERIX_ROOT_ACCEPTED_WITH_LIMITATIONS = 0,
    NUMERIX_ROOT_RESIDUAL_VERIFIED
} numerix_root_verification;

typedef struct numerix_discovered_root
{
    numerix_result result;
    numerix_algorithm refinement_algorithm;
    numerix_root_discovery_kind kind;
    numerix_root_verification verification;
    /* Candidate interval from the detector; it is not a formal enclosure. */
    double candidate_lower;
    double candidate_upper;
    size_t merged_detections;
    unsigned int evidence; /* Bit (1U << kind) for each merged detection. */
    int has_multiplicity_estimate; /* Heuristic estimate, never exact proof. */
    unsigned int estimated_multiplicity;
    numerix_diagnostic_confidence multiplicity_confidence;
    numerix_root_condition_report conditioning;
    numerix_diagnostics_report diagnostics;
    numerix_root_selection_report refinement_history;
} numerix_discovered_root;

typedef struct numerix_root_unresolved_interval
{
    double lower;
    double upper;
    numerix_status reason;
} numerix_root_unresolved_interval;

typedef struct numerix_root_search_report
{
    numerix_status status;
    double lower;
    double upper;
    double current_lower;
    double current_upper;
    numerix_discovered_root *roots;
    size_t root_count;
    size_t candidate_count;
    size_t rejected_candidates;
    size_t merged_candidates;
    size_t function_evaluations;
    size_t refinement_iterations;
    size_t subdivisions;
    size_t maximum_depth_reached;
    size_t evaluation_failures;
    size_t known_boundaries;
    size_t suspected_discontinuities;
    numerix_root_unresolved_interval *unresolved;
    size_t unresolved_count; /* Total; storage may be truncated. */
    size_t stored_unresolved_count;
    int unresolved_storage_truncated;
    /* Capacities of the two returned arrays, grown only as results are stored. */
    size_t root_storage_capacity;
    size_t unresolved_storage_capacity;
    /* Retained array bytes; excludes nested diagnostic records and allocator overhead. */
    size_t result_storage_bytes;
    /* Cumulative successful allocations, including arrays later released in the search. */
    size_t root_storage_allocation_count;
    size_t unresolved_storage_allocation_count;
    /* Peak and successful allocation count for direct discovery buffers only. */
    size_t workspace_peak_bytes;
    size_t workspace_allocation_count;
    int completed_normally;
    int resource_limit_reached;
    int search_inconclusive; /* Set for performed searches; completeness is unproved. */
    double elapsed_seconds;
} numerix_root_search_report;

/*
 * Called synchronously at entry/exit and at configured evaluation intervals.
 * The report is borrowed/read-only. SUCCESS continues; any other status stops.
 */
typedef numerix_status (*numerix_root_search_progress_callback)(
    const numerix_root_search_report *report, void *user_data);

typedef struct numerix_root_discovery_options
{
    size_t initial_panels;
    size_t maximum_depth;
    size_t maximum_subdivisions;
    size_t maximum_candidates;
    double residual_tolerance; /* Absolute function units, independent of x. */
    double curvature_threshold; /* Normalized midpoint curvature in (0, 2]. */
    double tangential_reduction; /* Required reduction relative to basin ends. */
    double merge_absolute_tolerance;
    double merge_relative_tolerance;
    int detect_tangential;
    int estimate_multiplicity;
    int estimate_condition;
    numerix_root_selection_options refinement;
    size_t progress_interval; /* Evaluation attempts; start/end also notify. */
    numerix_root_search_progress_callback progress_callback;
    void *progress_user_data;
} numerix_root_discovery_options;

/* Initializes documented defaults for a bounded finite-interval search. */
void numerix_root_discovery_options_init(numerix_root_discovery_options *options);
/*
 * request supplies the existing representation and shared position/resource
 * options. Its category must be ROOT_FINDING. Bounds here override its bracket.
 * Inputs are borrowed during this synchronous call. Destroy every returned
 * report, including partial reports on failure, before reuse. A zero root count
 * means no roots were detected under this strategy; it does not prove none
 * exist. Callbacks should represent a stable function: samples are reused
 * within this call only.
 */
numerix_status numerix_root_find_all(
    const numerix_recommendation_request *request, double lower, double upper,
    const numerix_root_discovery_options *options,
    numerix_root_search_report *report, numerix_error *error);
void numerix_root_search_report_destroy(numerix_root_search_report *report);

/* Returns a static readable name for a status code. */
const char *numerix_status_string(numerix_status status);
/* Clears an error, or records a bounded, copied message and status. */
void numerix_error_clear(numerix_error *error);
numerix_status numerix_error_set(numerix_error *error, numerix_status code,
                                 const char *message);
/* Initializes recommended defaults or validates all shared option fields. */
void numerix_options_init(numerix_options *options);
numerix_status numerix_options_validate(const numerix_options *options,
                                        numerix_error *error);
/* Problem borrows the callback and user_data; neither is freed by NUMERIX. */
numerix_status numerix_scalar_problem_init(numerix_scalar_problem *problem,
                                          numerix_function function,
                                          void *user_data,
                                          numerix_error *error);
numerix_status numerix_scalar_problem_evaluate(
    const numerix_scalar_problem *problem, double x, double *value,
    numerix_error *error);
/* Allocation ownership belongs to the caller; size zero returns NULL. */
void *numerix_allocate(size_t size);
void numerix_deallocate(void *memory);
/* Context is opaque and caller-owned after success. NULL options use defaults. */
numerix_status numerix_context_create(const numerix_options *options,
                                      numerix_context **context,
                                      numerix_error *error);
void numerix_context_destroy(numerix_context *context);
numerix_status numerix_context_record_iteration(numerix_context *context);
numerix_status numerix_context_record_function_evaluation(
    numerix_context *context);
numerix_status numerix_context_reset_counters(numerix_context *context);
size_t numerix_context_iteration_count(const numerix_context *context);
size_t numerix_context_function_evaluation_count(
    const numerix_context *context);
double numerix_context_elapsed_seconds(const numerix_context *context);
numerix_status numerix_initialize(numerix_error *error);

#ifdef __cplusplus
}
#endif
#endif
