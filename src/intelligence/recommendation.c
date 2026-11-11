#include "intelligence_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static numerix_status intelligence_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

const char *numerix_algorithm_name(numerix_algorithm algorithm)
{
    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_BISECTION:
            return "bisection";
        case NUMERIX_ALGORITHM_REGULA_FALSI:
            return "regula_falsi";
        case NUMERIX_ALGORITHM_SECANT:
            return "secant";
        case NUMERIX_ALGORITHM_NEWTON:
            return "newton_raphson";
        case NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON:
            return "safeguarded_newton";
        case NUMERIX_ALGORITHM_BRENT:
            return "brent_dekker";
        case NUMERIX_ALGORITHM_GAUSSIAN_ELIMINATION:
            return "gaussian_elimination";
        case NUMERIX_ALGORITHM_GAUSS_JORDAN:
            return "gauss_jordan";
        case NUMERIX_ALGORITHM_LU:
            return "lu_decomposition";
        case NUMERIX_ALGORITHM_DIFF_FORWARD:
            return "forward_difference";
        case NUMERIX_ALGORITHM_DIFF_BACKWARD:
            return "backward_difference";
        case NUMERIX_ALGORITHM_DIFF_CENTRAL:
            return "central_difference";
        case NUMERIX_ALGORITHM_DIFF_FIVE_POINT:
            return "five_point_difference";
        case NUMERIX_ALGORITHM_DIFF_RICHARDSON:
            return "richardson_extrapolation";
        case NUMERIX_ALGORITHM_DIFF_SECOND:
            return "second_difference";
        case NUMERIX_ALGORITHM_PIECEWISE_DIFF_CENTRAL:
            return "piecewise_central_difference";
        case NUMERIX_ALGORITHM_INTERP_LINEAR:
            return "linear_interpolation";
        case NUMERIX_ALGORITHM_INTERP_LAGRANGE:
            return "lagrange_interpolation";
        case NUMERIX_ALGORITHM_INTERP_NEWTON:
            return "newton_interpolation";
        case NUMERIX_ALGORITHM_INTEGRATE_TRAPEZOID:
            return "composite_trapezoid";
        case NUMERIX_ALGORITHM_INTEGRATE_SIMPSON:
            return "composite_simpson";
        case NUMERIX_ALGORITHM_INTEGRATE_ADAPTIVE_SIMPSON:
            return "adaptive_simpson";
        case NUMERIX_ALGORITHM_PIECEWISE_INTEGRATE_TRAPEZOID:
            return "piecewise_composite_trapezoid";
        case NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION:
            return "golden_section";
        case NUMERIX_ALGORITHM_OPTIMIZE_GRADIENT_DESCENT:
            return "gradient_descent";
        case NUMERIX_ALGORITHM_ODE_EULER:
            return "euler";
        case NUMERIX_ALGORITHM_ODE_HEUN:
            return "heun";
        case NUMERIX_ALGORITHM_ODE_RK4:
            return "runge_kutta_4";
        case NUMERIX_ALGORITHM_NONE:
            return "none";
    }
    return "unknown";
}

void numerix_recommendation_add_alternative(
    numerix_recommendation *recommendation,
    numerix_algorithm algorithm, const char *reason)
{
    size_t index = recommendation->alternative_count;

    if (index >= 4)
    {
        return;
    }
    recommendation->alternatives[index] = algorithm;
    snprintf(recommendation->alternative_reason[index],
             sizeof(recommendation->alternative_reason[index]), "%s", reason);
    ++recommendation->alternative_count;
}

void numerix_recommendation_set(
    numerix_recommendation *recommendation,
    numerix_algorithm algorithm, numerix_suitability suitability,
    const char *reason, const char *limitation)
{
    recommendation->recommended_algorithm = algorithm;
    recommendation->suitability = suitability;
    if (reason != NULL)
    {
        snprintf(recommendation->reason[0], sizeof(recommendation->reason[0]),
                 "%s", reason);
    }
    if (limitation != NULL)
    {
        snprintf(recommendation->limitation[0],
                 sizeof(recommendation->limitation[0]), "%s", limitation);
    }
}

static int valid_function_request(
    const numerix_recommendation_request *request)
{
    return numerix_request_has_function(request);
}

static int duplicate_coordinates(const double *values, size_t count)
{
    size_t first;

    for (first = 0; first < count; ++first)
    {
        size_t next;
        for (next = first + 1; next < count; ++next)
        {
            if (values[first] == values[next])
            {
                return 1;
            }
        }
    }
    return 0;
}

static numerix_status recommend_root(
    const numerix_recommendation_request *request,
    numerix_recommendation *recommendation, numerix_error *error)
{
    double lower_value;
    double upper_value;
    numerix_status lower_status;
    numerix_status upper_status;
    numerix_status status;
    int bracket = 0;
    size_t piecewise_boundary_count = 0;
    size_t crossing_boundary_count = 0;
    int crosses_piecewise_boundary = 0;
    int derivative_usable = request->derivative != NULL;

    if (!valid_function_request(request))
    {
        return intelligence_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                  "root recommendation requires an evaluable function");
    }
    if ((request->problem.root.has_bracket != 0 &&
         request->problem.root.has_bracket != 1) ||
        (request->problem.root.has_initial != 0 &&
         request->problem.root.has_initial != 1) ||
        (request->problem.root.has_second_initial != 0 &&
         request->problem.root.has_second_initial != 1))
    {
        return intelligence_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                  "root availability flags must be zero or one");
    }
    if (request->problem.root.has_bracket)
    {
        double lower = request->problem.root.lower;
        double upper = request->problem.root.upper;
        if (!isfinite(lower) || !isfinite(upper) || !(lower < upper))
        {
            return intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                      "root interval must be finite and ordered");
        }
        if (request->options != NULL &&
            request->options->maximum_function_evaluations < 2)
        {
            return intelligence_error(error, NUMERIX_STATUS_LIMIT_REACHED,
                                      "root bracket check needs two callback evaluations");
        }
        lower_status = numerix_request_evaluate(
            request, lower, &lower_value, error);
        ++recommendation->probe_evaluations;
        if (lower_status != NUMERIX_STATUS_SUCCESS)
        {
            return lower_status;
        }
        upper_status = numerix_request_evaluate(
            request, upper, &upper_value, error);
        ++recommendation->probe_evaluations;
        if (upper_status != NUMERIX_STATUS_SUCCESS)
        {
            return upper_status;
        }
        bracket = lower_value == 0.0 || upper_value == 0.0 ||
                  ((lower_value < 0.0) != (upper_value < 0.0));

        if (bracket &&
            request->representation == NUMERIX_REPRESENTATION_PIECEWISE &&
            request->piecewise_function != NULL)
        {
            double *boundaries;
            size_t boundary_index;
            status = numerix_piecewise_get_boundaries(
                request->piecewise_function, NULL, 0,
                &piecewise_boundary_count, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }
            if (piecewise_boundary_count > 0)
            {
                boundaries = numerix_allocate(
                    piecewise_boundary_count * sizeof(*boundaries));
                if (boundaries == NULL)
                {
                    return intelligence_error(
                        error, NUMERIX_STATUS_ALLOCATION_FAILURE,
                        "could not allocate piecewise boundary workspace");
                }
                status = numerix_piecewise_get_boundaries(
                    request->piecewise_function, boundaries,
                    piecewise_boundary_count, &piecewise_boundary_count, error);
                if (status != NUMERIX_STATUS_SUCCESS)
                {
                    numerix_deallocate(boundaries);
                    return status;
                }
                for (boundary_index = 0;
                     boundary_index < piecewise_boundary_count;
                     ++boundary_index)
                {
                    if (boundaries[boundary_index] > lower &&
                        boundaries[boundary_index] < upper)
                    {
                        crosses_piecewise_boundary = 1;
                        ++crossing_boundary_count;
                    }
                }
                numerix_deallocate(boundaries);
            }
        }
    }

    if (derivative_usable && request->problem.root.has_initial &&
        isfinite(request->problem.root.initial))
    {
        double derivative_value = request->derivative(
            request->problem.root.initial, request->user_data);
        recommendation->derivative_checked = 1;
        recommendation->derivative_probe_evaluations = 1;
        recommendation->derivative_appears_zero_or_non_finite =
            !isfinite(derivative_value) || derivative_value == 0.0;
        if (recommendation->derivative_appears_zero_or_non_finite)
        {
            derivative_usable = 0;
        }
    }

    recommendation->sign_changing_bracket = bracket;
    recommendation->crosses_piecewise_boundaries = crosses_piecewise_boundary;
    recommendation->piecewise_boundary_count = crossing_boundary_count;
    if (bracket && derivative_usable && request->problem.root.has_initial &&
        request->problem.root.initial >= request->problem.root.lower &&
        request->problem.root.initial <= request->problem.root.upper)
    {
        numerix_recommendation_set(
            recommendation, NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON,
            NUMERIX_SUITABILITY_SUITABLE,
            "A sign-changing bracket and an in-bracket initial estimate are available; the derivative can accelerate progress while the bracket is retained.",
            "Derivative quality is not proven, and the method targets one root in this bracket.");
        numerix_recommendation_add_alternative(
            recommendation, NUMERIX_ALGORITHM_BRENT,
            "Derivative-free bracketed fallback with interpolation and bisection.");
        numerix_recommendation_add_alternative(
            recommendation, NUMERIX_ALGORITHM_BISECTION,
            "Conservative bracket-preserving alternative.");
        if (crosses_piecewise_boundary)
        {
            recommendation->suitability = NUMERIX_SUITABILITY_CONDITIONAL;
            snprintf(recommendation->limitation[1],
                     sizeof(recommendation->limitation[1]),
                     "The bracket crosses known piecewise boundaries; an endpoint sign change may come from a jump rather than a root.");
        }
        return NUMERIX_STATUS_SUCCESS;
    }
    if (bracket)
    {
        numerix_recommendation_set(
            recommendation, NUMERIX_ALGORITHM_BRENT,
            NUMERIX_SUITABILITY_SUITABLE,
            "Endpoint evaluations verify a sign change; Brent-Dekker needs no derivative and preserves a bracket.",
            "The method finds one root in the supplied bracket and does not discover every root.");
        numerix_recommendation_add_alternative(
            recommendation, NUMERIX_ALGORITHM_BISECTION,
            "Use when simpler bracket-preservation behavior is preferred.");
        numerix_recommendation_add_alternative(
            recommendation, NUMERIX_ALGORITHM_REGULA_FALSI,
            "A bracketed chord method; it may converge slowly for unbalanced endpoint values.");
        if (crosses_piecewise_boundary)
        {
            recommendation->suitability = NUMERIX_SUITABILITY_CONDITIONAL;
            snprintf(recommendation->limitation[1],
                     sizeof(recommendation->limitation[1]),
                     "The bracket crosses known piecewise boundaries; an endpoint sign change may come from a jump rather than a root.");
        }
        return NUMERIX_STATUS_SUCCESS;
    }
    if (request->derivative != NULL && request->problem.root.has_initial &&
        isfinite(request->problem.root.initial))
    {
        numerix_recommendation_set(
            recommendation, NUMERIX_ALGORITHM_NEWTON,
            NUMERIX_SUITABILITY_CONDITIONAL,
            "A derivative and finite initial estimate are available, so Newton-Raphson is applicable.",
            "No valid sign-changing bracket was established; convergence depends on the initial estimate and derivative behavior.");
        if (request->problem.root.has_second_initial &&
            isfinite(request->problem.root.second_initial))
        {
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_SECANT,
                "Derivative-free alternative using the two supplied initial estimates.");
        }
        return NUMERIX_STATUS_SUCCESS;
    }
    if (request->problem.root.has_initial &&
        request->problem.root.has_second_initial &&
        isfinite(request->problem.root.initial) &&
        isfinite(request->problem.root.second_initial) &&
        request->problem.root.initial != request->problem.root.second_initial)
    {
        numerix_recommendation_set(
            recommendation, NUMERIX_ALGORITHM_SECANT,
            NUMERIX_SUITABILITY_CONDITIONAL,
            "Two distinct finite initial estimates are available; the secant method does not require a derivative.",
            "Without a bracket, convergence and root location are not guaranteed.");
        return NUMERIX_STATUS_SUCCESS;
    }

    recommendation->suitability = NUMERIX_SUITABILITY_UNAVAILABLE;
    return intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                              "provide a valid sign-changing bracket or usable initial estimates");
}

numerix_status numerix_recommend_algorithm(
    const numerix_recommendation_request *request,
    numerix_recommendation *recommendation, numerix_error *error)
{
    numerix_options validated_options;
    numerix_status status = NUMERIX_STATUS_SUCCESS;

    if (request == NULL || recommendation == NULL)
    {
        return intelligence_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                  "request and recommendation output are required");
    }
    memset(recommendation, 0, sizeof(*recommendation));
    recommendation->category = request->category;
    recommendation->bracket_was_checked =
        request->category == NUMERIX_PROBLEM_ROOT_FINDING &&
        request->problem.root.has_bracket;
    recommendation->derivative_available = request->derivative != NULL;
    if (request->category < NUMERIX_PROBLEM_ROOT_FINDING ||
        request->category > NUMERIX_PROBLEM_ODE ||
        request->representation < NUMERIX_REPRESENTATION_CALLBACK ||
        request->representation > NUMERIX_REPRESENTATION_PIECEWISE)
    {
        return intelligence_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                  "unknown problem category or representation");
    }
    if (request->options != NULL)
    {
        validated_options = *request->options;
        status = numerix_options_validate(&validated_options, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return status;
        }
    }

    switch (request->category)
    {
        case NUMERIX_PROBLEM_ROOT_FINDING:
            status = recommend_root(request, recommendation, error);
            break;
        case NUMERIX_PROBLEM_LINEAR_ALGEBRA:
        {
            const numerix_matrix *matrix = request->problem.linear.matrix;
            const numerix_vector *right = request->problem.linear.right_hand_side;
            if (matrix == NULL || right == NULL || matrix->data == NULL ||
                right->data == NULL || matrix->rows == 0 ||
                matrix->rows != matrix->columns || right->size != matrix->rows)
            {
                status = intelligence_error(
                    error, NUMERIX_STATUS_INVALID_DIMENSIONS,
                    "linear solve recommendation requires a square matrix and matching right-hand side");
                break;
            }
            {
                numerix_lu factorization;
                status = numerix_lu_decompose(
                    matrix, request->options, &factorization, error);
                if (status != NUMERIX_STATUS_SUCCESS)
                {
                    break;
                }
                numerix_lu_destroy(&factorization);
            }
            numerix_recommendation_set(
                recommendation, NUMERIX_ALGORITHM_LU,
                NUMERIX_SUITABILITY_SUITABLE,
                "A square matrix and matching right-hand side are present, and the existing LU factorization accepts this matrix.",
                "Conditioning is not estimated; pivot acceptance does not guarantee a well-conditioned solution.");
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_GAUSSIAN_ELIMINATION,
                "Direct one-shot solve with pivoting.");
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_GAUSS_JORDAN,
                "Available direct alternative, usually with more elimination work.");
            break;
        }
        case NUMERIX_PROBLEM_DIFFERENTIATION:
        {
            double x = request->problem.differentiation.x;
            double step = request->problem.differentiation.step;
            numerix_algorithm algorithm = NUMERIX_ALGORITHM_DIFF_CENTRAL;
            if (!valid_function_request(request) || !isfinite(x) ||
                !isfinite(step) || step <= 0.0)
            {
                status = intelligence_error(
                    error, NUMERIX_STATUS_INVALID_INPUT,
                    "differentiation requires a callback, finite point, and positive step");
                break;
            }
            if (request->representation == NUMERIX_REPRESENTATION_PIECEWISE &&
                request->piecewise_function != NULL)
            {
                algorithm = NUMERIX_ALGORITHM_PIECEWISE_DIFF_CENTRAL;
                numerix_recommendation_set(
                    recommendation, algorithm, NUMERIX_SUITABILITY_CONDITIONAL,
                    "The piecewise-aware central difference checks known boundaries before evaluating a stencil.",
                    "The estimate is rejected if the stencil crosses a boundary or one-sided slopes disagree.");
            }
            else
            {
                numerix_recommendation_set(
                    recommendation, algorithm, NUMERIX_SUITABILITY_SUITABLE,
                    "Central differences use samples on both sides and have lower leading truncation error than one-sided differences for smooth functions.",
                    "Smoothness near the point is assumed and cannot be proven from the request alone.");
            }
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_DIFF_FIVE_POINT,
                "Higher-order stencil when the function is smooth and extra evaluations are acceptable.");
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_DIFF_FORWARD,
                "One-sided alternative near a known left-domain boundary.");
            break;
        }
        case NUMERIX_PROBLEM_INTERPOLATION:
        {
            const double *x = request->problem.interpolation.x;
            const double *y = request->problem.interpolation.y;
            size_t count = request->problem.interpolation.count;
            size_t index;
            if ((request->problem.interpolation.allow_extrapolation != 0 &&
                 request->problem.interpolation.allow_extrapolation != 1) ||
                x == NULL || y == NULL || count < 2 ||
                !isfinite(request->problem.interpolation.query))
            {
                status = intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                            "interpolation requires at least two data points and a finite query");
                break;
            }
            for (index = 0; index < count; ++index)
            {
                if (!isfinite(x[index]) || !isfinite(y[index]))
                {
                    status = intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                                "interpolation data must be finite");
                    break;
                }
            }
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                break;
            }
            if (duplicate_coordinates(x, count))
            {
                status = intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                            "interpolation data contains duplicate coordinates");
                break;
            }
            {
                double minimum = x[0];
                double maximum = x[0];
                double query = request->problem.interpolation.query;
                for (index = 1; index < count; ++index)
                {
                    minimum = fmin(minimum, x[index]);
                    maximum = fmax(maximum, x[index]);
                }
                if (!request->problem.interpolation.allow_extrapolation &&
                    (query < minimum || query > maximum))
                {
                    status = intelligence_error(
                        error, NUMERIX_STATUS_INVALID_INPUT,
                        "interpolation query is outside the data range and extrapolation is disabled");
                    break;
                }
            }
            recommendation->suitability = NUMERIX_SUITABILITY_SUITABLE;
            if (count == 2)
            {
                numerix_recommendation_set(
                    recommendation, NUMERIX_ALGORITHM_INTERP_LINEAR,
                    NUMERIX_SUITABILITY_SUITABLE,
                    "Two distinct data points define the available linear interpolant directly.",
                    "The result is a straight-line estimate and may be inaccurate for curved data.");
            }
            else
            {
                numerix_recommendation_set(
                    recommendation, NUMERIX_ALGORITHM_INTERP_NEWTON,
                    NUMERIX_SUITABILITY_SUITABLE,
                    "Multiple distinct data points are available; Newton divided differences reuse the supplied nodes.",
                    "High-degree interpolation may be ill-conditioned; extrapolation is less reliable.");
                numerix_recommendation_add_alternative(
                    recommendation, NUMERIX_ALGORITHM_INTERP_LAGRANGE,
                    "Direct polynomial form for a modest number of nodes.");
            }
            break;
        }
        case NUMERIX_PROBLEM_INTEGRATION:
            if ((!valid_function_request(request) &&
                 !(request->representation == NUMERIX_REPRESENTATION_PIECEWISE &&
                   request->piecewise_function != NULL)) ||
                !isfinite(request->problem.integration.lower) ||
                !isfinite(request->problem.integration.upper) ||
                request->problem.integration.lower == request->problem.integration.upper)
            {
                status = intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                            "integration requires a callback and finite nonzero interval");
                break;
            }
            if (request->representation == NUMERIX_REPRESENTATION_PIECEWISE &&
                request->piecewise_function != NULL)
            {
                numerix_recommendation_set(
                    recommendation, NUMERIX_ALGORITHM_PIECEWISE_INTEGRATE_TRAPEZOID,
                    NUMERIX_SUITABILITY_SUITABLE,
                    "The piecewise integrator splits at known segment boundaries and rejects uncovered gaps.",
                    "Open endpoints are approximated from inside each segment; arbitrary interior singularities are not resolved.");
            }
            else
            {
                numerix_recommendation_set(
                    recommendation, NUMERIX_ALGORITHM_INTEGRATE_ADAPTIVE_SIMPSON,
                    NUMERIX_SUITABILITY_SUITABLE,
                    "Adaptive Simpson refines a smooth finite interval according to the requested tolerances.",
                    "Its error estimate is heuristic and discontinuities can invalidate its smoothness assumption.");
                numerix_recommendation_add_alternative(
                    recommendation, NUMERIX_ALGORITHM_INTEGRATE_SIMPSON,
                    "Fixed even-panel Simpson rule when a panel count is specified.");
                numerix_recommendation_add_alternative(
                    recommendation, NUMERIX_ALGORITHM_INTEGRATE_TRAPEZOID,
                    "Simple fixed-panel rule with predictable evaluation locations.");
            }
            break;
        case NUMERIX_PROBLEM_OPTIMIZATION:
        {
            const numerix_vector *initial = request->problem.optimization.initial;
            if (request->problem.optimization.bounded != 0 &&
                request->problem.optimization.bounded != 1)
            {
                status = intelligence_error(
                    error, NUMERIX_STATUS_INVALID_ARGUMENT,
                    "optimization bounded flag must be zero or one");
                break;
            }
            if (request->problem.optimization.gradient != NULL &&
                request->problem.optimization.objective != NULL &&
                initial != NULL && initial->size > 0 && initial->data != NULL &&
                isfinite(request->problem.optimization.initial_step) &&
                request->problem.optimization.initial_step > 0.0)
            {
                numerix_recommendation_set(
                    recommendation, NUMERIX_ALGORITHM_OPTIMIZE_GRADIENT_DESCENT,
                    NUMERIX_SUITABILITY_CONDITIONAL,
                    "An objective, gradient, initial vector, and positive initial step are available.",
                    "The implementation is local gradient descent with backtracking and does not guarantee a global optimum.");
                if (request->problem.optimization.bounded)
                {
                    numerix_recommendation_add_alternative(
                        recommendation, NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION,
                        "Bounded scalar alternative when the objective has one input dimension.");
                }
            }
            else if (request->problem.optimization.bounded &&
                       valid_function_request(request) &&
                       isfinite(request->problem.optimization.lower) &&
                       isfinite(request->problem.optimization.upper) &&
                       request->problem.optimization.lower <
                           request->problem.optimization.upper)
            {
                numerix_recommendation_set(
                    recommendation, NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION,
                    NUMERIX_SUITABILITY_SUITABLE,
                    "A finite bounded scalar interval and objective callback are available.",
                    "Golden-section search is local to the supplied interval and assumes a unimodal objective.");
            }
            else
            {
                status = intelligence_error(
                    error, NUMERIX_STATUS_INVALID_INPUT,
                    "optimization requires a bounded scalar interval or objective-gradient data");
            }
            break;
        }
        case NUMERIX_PROBLEM_ODE:
        {
            const double *initial_state = request->problem.ode.initial_state;
            size_t dimension = request->problem.ode.dimension;
            if (request->problem.ode.function == NULL || initial_state == NULL ||
                dimension == 0 || !isfinite(request->problem.ode.initial_x) ||
                !isfinite(request->problem.ode.step) ||
                request->problem.ode.step == 0.0 ||
                request->problem.ode.steps == 0)
            {
                status = intelligence_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                            "ODE recommendation requires a callback, initial state, dimension, and nonzero step plan");
                break;
            }
            {
                size_t state_index;
                for (state_index = 0; state_index < dimension; ++state_index)
                {
                    if (!isfinite(initial_state[state_index]))
                    {
                        status = intelligence_error(
                            error, NUMERIX_STATUS_INVALID_INPUT,
                            "ODE initial state must contain finite values");
                        break;
                    }
                }
                if (status != NUMERIX_STATUS_SUCCESS)
                {
                    break;
                }
            }
            numerix_recommendation_set(
                recommendation, NUMERIX_ALGORITHM_ODE_RK4,
                NUMERIX_SUITABILITY_SUITABLE,
                "A valid fixed-step initial-value problem is present; RK4 offers higher order than Euler or Heun for smooth systems.",
                "The explicit fixed-step method may be unstable for stiff systems; stiffness is not inferred here.");
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_ODE_HEUN,
                "Lower-cost second-order explicit alternative.");
            numerix_recommendation_add_alternative(
                recommendation, NUMERIX_ALGORITHM_ODE_EULER,
                "Simplest low-cost method, generally less accurate.");
            break;
        }
    }

    if (status == NUMERIX_STATUS_SUCCESS)
    {
        numerix_error_clear(error);
    }
    return status;
}
