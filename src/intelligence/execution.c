#include "intelligence_internal.h"

#include <math.h>
#include <string.h>

static numerix_status execution_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

static numerix_status execute_scalar_callback(
    const numerix_recommendation_request *request,
    numerix_problem_callback_context *context, numerix_error *error)
{
    return numerix_problem_callback_context_init(context, request, error);
}

static numerix_status execute_linear(
    const numerix_recommendation_request *request, numerix_algorithm algorithm,
    numerix_execution_output *output, numerix_error *error)
{
    numerix_status status;
    const numerix_matrix *matrix = request->problem.linear.matrix;
    const numerix_vector *right = request->problem.linear.right_hand_side;
    numerix_lu factorization;
    double residual = NAN;

    if (matrix == NULL || right == NULL)
    {
        return execution_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                               "linear solve input is missing");
    }
    if (algorithm == NUMERIX_ALGORITHM_LU)
    {
        status = numerix_lu_decompose(
            matrix, request->options, &factorization, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return status;
        }
        status = numerix_lu_solve(
            &factorization, right, &output->vector_result, error);
        numerix_lu_destroy(&factorization);
    }
    else if (algorithm == NUMERIX_ALGORITHM_GAUSSIAN_ELIMINATION)
    {
        status = numerix_linear_solve_gaussian(
            matrix, right, request->options, &output->vector_result,
            &residual, error);
    }
    else if (algorithm == NUMERIX_ALGORITHM_GAUSS_JORDAN)
    {
        status = numerix_linear_solve_gauss_jordan(
            matrix, right, request->options, &output->vector_result,
            &residual, error);
    }
    else
    {
        return execution_error(error, NUMERIX_STATUS_UNSUPPORTED_OPERATION,
                               "selected algorithm is not a linear solver");
    }
    output->scalar_result.residual = residual;
    return status;
}

static numerix_status execute_differentiation(
    const numerix_recommendation_request *request, numerix_algorithm algorithm,
    numerix_execution_output *output, numerix_error *error)
{
    numerix_problem_callback_context context;
    numerix_status status;
    numerix_function function;
    void *user_data;
    double x = request->problem.differentiation.x;
    double step = request->problem.differentiation.step;

    if (algorithm == NUMERIX_ALGORITHM_PIECEWISE_DIFF_CENTRAL)
    {
        if (request->piecewise_function == NULL)
        {
            return execution_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                   "piecewise derivative requires a piecewise function");
        }
        return numerix_piecewise_diff_central(
            request->piecewise_function, x, step, request->options,
            &output->scalar_result, error);
    }
    status = execute_scalar_callback(request, &context, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    function = numerix_problem_callback;
    user_data = &context;

    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_DIFF_FORWARD:
            status = numerix_diff_forward(function, user_data, x, step,
                                          request->options,
                                          &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_DIFF_BACKWARD:
            status = numerix_diff_backward(function, user_data, x, step,
                                           request->options,
                                           &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_DIFF_CENTRAL:
            status = numerix_diff_central(function, user_data, x, step,
                                          request->options,
                                          &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_DIFF_FIVE_POINT:
            status = numerix_diff_central_five_point(
                function, user_data, x, step, request->options,
                &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_DIFF_RICHARDSON:
            status = numerix_diff_richardson(
                function, user_data, x, step, request->options,
                &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_DIFF_SECOND:
            status = numerix_diff_second(function, user_data, x, step,
                                         request->options,
                                         &output->scalar_result, error);
            break;
        default:
            return execution_error(error, NUMERIX_STATUS_UNSUPPORTED_OPERATION,
                                   "selected algorithm is not a derivative method");
    }
    if (status != NUMERIX_STATUS_SUCCESS &&
        context.last_status != NUMERIX_STATUS_SUCCESS)
    {
        status = context.last_status;
        output->scalar_result.status = status;
        if (error != NULL)
        {
            *error = context.last_error;
        }
    }
    return status;
}

static numerix_status execute_interpolation(
    const numerix_recommendation_request *request, numerix_algorithm algorithm,
    const numerix_execution_options *options,
    numerix_execution_output *output, numerix_error *error)
{
    const double *x = request->problem.interpolation.x;
    const double *y = request->problem.interpolation.y;
    size_t count = request->problem.interpolation.count;
    double query = request->problem.interpolation.query;
    int allow_extrapolation = request->problem.interpolation.allow_extrapolation;
    (void)options;

    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_INTERP_LINEAR:
            if (count < 2)
            {
                return execution_error(error, NUMERIX_STATUS_INVALID_INPUT,
                                       "linear interpolation needs two points");
            }
            return numerix_interp_linear(
                x[0], y[0], x[1], y[1], query,
                &output->scalar_result, error);
        case NUMERIX_ALGORITHM_INTERP_LAGRANGE:
            return numerix_interp_lagrange(
                x, y, count, query, allow_extrapolation,
                &output->scalar_result, error);
        case NUMERIX_ALGORITHM_INTERP_NEWTON:
        {
            numerix_interpolant interpolant;
            numerix_status status = numerix_interp_newton_create(
                x, y, count, &interpolant, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }
            status = numerix_interp_newton_evaluate(
                &interpolant, query, allow_extrapolation,
                &output->scalar_result, error);
            numerix_interpolant_destroy(&interpolant);
            return status;
        }
        default:
            return execution_error(error, NUMERIX_STATUS_UNSUPPORTED_OPERATION,
                                   "selected algorithm is not an interpolation method");
    }
}

static numerix_status execute_integration(
    const numerix_recommendation_request *request, numerix_algorithm algorithm,
    const numerix_execution_options *execution_options,
    numerix_execution_output *output, numerix_error *error)
{
    numerix_problem_callback_context context;
    numerix_status status;
    double lower = request->problem.integration.lower;
    double upper = request->problem.integration.upper;
    size_t panels = execution_options != NULL && execution_options->panels > 0
                        ? execution_options->panels
                        : request->problem.integration.panels;
    size_t depth = execution_options != NULL && execution_options->adaptive_depth > 0
                       ? execution_options->adaptive_depth
                       : 24;

    if (algorithm == NUMERIX_ALGORITHM_PIECEWISE_INTEGRATE_TRAPEZOID)
    {
        if (request->piecewise_function == NULL)
        {
            return execution_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                   "piecewise integration requires a piecewise function");
        }
        if (panels == 0)
        {
            panels = 100;
        }
        return numerix_piecewise_integrate_trapezoid(
            request->piecewise_function, lower, upper, panels,
            request->options, &output->scalar_result, error);
    }

    status = execute_scalar_callback(request, &context, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }
    if (panels == 0)
    {
        panels = 100;
    }
    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_INTEGRATE_TRAPEZOID:
            status = numerix_integrate_trapezoid(
                numerix_problem_callback, &context, lower, upper, panels,
                request->options, &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_INTEGRATE_SIMPSON:
            if (panels % 2 != 0)
            {
                ++panels;
            }
            status = numerix_integrate_simpson(
                numerix_problem_callback, &context, lower, upper, panels,
                request->options, &output->scalar_result, error);
            break;
        case NUMERIX_ALGORITHM_INTEGRATE_ADAPTIVE_SIMPSON:
            status = numerix_integrate_adaptive_simpson(
                numerix_problem_callback, &context, lower, upper,
                request->options, depth, &output->scalar_result, error);
            break;
        default:
            return execution_error(error, NUMERIX_STATUS_UNSUPPORTED_OPERATION,
                                   "selected algorithm is not an integration method");
    }
    if (status != NUMERIX_STATUS_SUCCESS &&
        context.last_status != NUMERIX_STATUS_SUCCESS)
    {
        status = context.last_status;
        output->scalar_result.status = status;
        if (error != NULL)
        {
            *error = context.last_error;
        }
    }
    return status;
}

static numerix_status execute_optimization(
    const numerix_recommendation_request *request, numerix_algorithm algorithm,
    numerix_execution_output *output, numerix_error *error)
{
    if (algorithm == NUMERIX_ALGORITHM_OPTIMIZE_GOLDEN_SECTION)
    {
        numerix_problem_callback_context context;
        numerix_status status = execute_scalar_callback(request, &context, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            return status;
        }
        status = numerix_optimize_golden_section(
            numerix_problem_callback, &context,
            request->problem.optimization.lower,
            request->problem.optimization.upper, request->options,
            &output->scalar_result, error);
        if (status != NUMERIX_STATUS_SUCCESS &&
            context.last_status != NUMERIX_STATUS_SUCCESS)
        {
            status = context.last_status;
            output->scalar_result.status = status;
            if (error != NULL)
            {
                *error = context.last_error;
            }
        }
        return status;
    }
    if (algorithm == NUMERIX_ALGORITHM_OPTIMIZE_GRADIENT_DESCENT)
    {
        if (request->problem.optimization.objective == NULL ||
            request->problem.optimization.gradient == NULL ||
            request->problem.optimization.initial == NULL)
        {
            return execution_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                                   "gradient descent inputs are incomplete");
        }
        return numerix_optimize_gradient_descent(
            request->problem.optimization.objective,
            request->problem.optimization.gradient, request->user_data,
            request->problem.optimization.initial,
            request->problem.optimization.initial_step, request->options,
            &output->vector_result, &output->scalar_result, error);
    }
    return execution_error(error, NUMERIX_STATUS_UNSUPPORTED_OPERATION,
                           "selected algorithm is not an optimizer");
}

static numerix_status execute_ode(
    const numerix_recommendation_request *request, numerix_algorithm algorithm,
    numerix_execution_output *output, numerix_error *error)
{
    const double *initial = request->problem.ode.initial_state;
    size_t dimension = request->problem.ode.dimension;
    double x0 = request->problem.ode.initial_x;
    double step = request->problem.ode.step;
    size_t steps = request->problem.ode.steps;
    numerix_status status;

    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_ODE_EULER:
            status = numerix_ode_euler(
                request->problem.ode.function, request->user_data, x0,
                initial, dimension, step, steps, &output->ode_result, error);
            break;
        case NUMERIX_ALGORITHM_ODE_HEUN:
            status = numerix_ode_heun(
                request->problem.ode.function, request->user_data, x0,
                initial, dimension, step, steps, &output->ode_result, error);
            break;
        case NUMERIX_ALGORITHM_ODE_RK4:
            status = numerix_ode_rk4(
                request->problem.ode.function, request->user_data, x0,
                initial, dimension, step, steps, &output->ode_result, error);
            break;
        default:
            return execution_error(error, NUMERIX_STATUS_UNSUPPORTED_OPERATION,
                                   "selected algorithm is not an ODE method");
    }
    output->has_ode_result = 1;
    return status;
}

numerix_status numerix_execute_recommended(
    const numerix_recommendation_request *request,
    const numerix_execution_options *execution_options,
    numerix_execution_output *output, numerix_error *error)
{
    numerix_root_selection_options root_options;
    numerix_status status;
    numerix_algorithm algorithm;
    int automatic;

    if (request == NULL || output == NULL)
    {
        return execution_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                               "execution request and output are required");
    }
    memset(output, 0, sizeof(*output));
    output->category = request->category;
    output->scalar_result.value = NAN;
    output->scalar_result.residual = NAN;
    if (execution_options != NULL &&
        execution_options->enable_root_fallback != 0 &&
        execution_options->enable_root_fallback != 1)
    {
        output->scalar_result.status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return execution_error(error, output->scalar_result.status,
                               "root fallback flag must be zero or one");
    }
    automatic = execution_options == NULL ||
                execution_options->explicit_algorithm == NUMERIX_ALGORITHM_NONE;
    output->selected_automatically = automatic;

    if (request->category == NUMERIX_PROBLEM_ROOT_FINDING)
    {
        root_options.mode = automatic
                                ? NUMERIX_SELECTION_AUTOMATIC
                                : NUMERIX_SELECTION_EXPLICIT;
        root_options.explicit_algorithm = automatic
                                              ? NUMERIX_ALGORITHM_NONE
                                              : execution_options->explicit_algorithm;
        root_options.enable_fallback = execution_options != NULL &&
                                       execution_options->enable_root_fallback;
        status = numerix_root_solve_auto(
            request, &root_options, &output->scalar_result,
            &output->root_selection, error);
        output->recommendation = output->root_selection.recommendation;
        output->algorithm = output->root_selection.final_algorithm;
        return status;
    }

    status = numerix_recommend_algorithm(
        request, &output->recommendation, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        if (automatic)
        {
            output->scalar_result.status = status;
            return status;
        }
        output->recommendation.category = request->category;
        output->recommendation.suitability = NUMERIX_SUITABILITY_UNAVAILABLE;
        output->recommendation.recommended_algorithm = NUMERIX_ALGORITHM_NONE;
    }
    algorithm = automatic
                    ? output->recommendation.recommended_algorithm
                    : execution_options->explicit_algorithm;
    output->algorithm = algorithm;

    switch (request->category)
    {
        case NUMERIX_PROBLEM_LINEAR_ALGEBRA:
            status = execute_linear(request, algorithm, output, error);
            break;
        case NUMERIX_PROBLEM_DIFFERENTIATION:
            status = execute_differentiation(request, algorithm, output, error);
            break;
        case NUMERIX_PROBLEM_INTERPOLATION:
            status = execute_interpolation(
                request, algorithm, execution_options, output, error);
            break;
        case NUMERIX_PROBLEM_INTEGRATION:
            status = execute_integration(
                request, algorithm, execution_options, output, error);
            break;
        case NUMERIX_PROBLEM_OPTIMIZATION:
            status = execute_optimization(request, algorithm, output, error);
            break;
        case NUMERIX_PROBLEM_ODE:
            status = execute_ode(request, algorithm, output, error);
            break;
        case NUMERIX_PROBLEM_ROOT_FINDING:
            status = NUMERIX_STATUS_INTERNAL_ERROR;
            break;
    }
    if (request->category != NUMERIX_PROBLEM_ODE)
    {
        output->scalar_result.status = status;
    }
    return status;
}

void numerix_execution_output_destroy(numerix_execution_output *output)
{
    if (output == NULL)
    {
        return;
    }
    numerix_vector_destroy(output->vector_result);
    if (output->has_ode_result)
    {
        numerix_ode_solution_destroy(&output->ode_result);
    }
    memset(output, 0, sizeof(*output));
}
