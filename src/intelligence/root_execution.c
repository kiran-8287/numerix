#include "intelligence_internal.h"
#include "numerix_execution_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static numerix_status execution_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

static numerix_status get_options(
    const numerix_recommendation_request *request, numerix_options *options,
    numerix_error *error)
{
    numerix_status status;

    if (request->options == NULL)
    {
        numerix_options_init(options);
        return NUMERIX_STATUS_SUCCESS;
    }
    *options = *request->options;
    status = numerix_options_validate(options, error);
    return status;
}

int numerix_root_algorithm_applicable(
    numerix_algorithm algorithm,
    const numerix_recommendation_request *request,
    int sign_changing_bracket)
{
    if (request == NULL || request->category != NUMERIX_PROBLEM_ROOT_FINDING ||
        !numerix_request_has_function(request))
    {
        return 0;
    }
    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_BISECTION:
        case NUMERIX_ALGORITHM_REGULA_FALSI:
        case NUMERIX_ALGORITHM_BRENT:
            return sign_changing_bracket;
        case NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON:
            return sign_changing_bracket && request->derivative != NULL &&
                   request->problem.root.has_initial &&
                   request->problem.root.initial >= request->problem.root.lower &&
                   request->problem.root.initial <= request->problem.root.upper;
        case NUMERIX_ALGORITHM_NEWTON:
            return request->derivative != NULL &&
                   request->problem.root.has_initial &&
                   isfinite(request->problem.root.initial);
        case NUMERIX_ALGORITHM_SECANT:
            return request->problem.root.has_initial &&
                   request->problem.root.has_second_initial &&
                   isfinite(request->problem.root.initial) &&
                   isfinite(request->problem.root.second_initial) &&
                   request->problem.root.initial !=
                       request->problem.root.second_initial;
        default:
            return 0;
    }
}

numerix_status numerix_root_run_algorithm(
    numerix_algorithm algorithm,
    const numerix_recommendation_request *request,
    numerix_result *result, numerix_error *error)
{
    const numerix_options *options = request->options;
    double lower = request->problem.root.lower;
    double upper = request->problem.root.upper;
    double initial = request->problem.root.initial;
    numerix_problem_callback_context callback_context;
    numerix_status status;

    memset(result, 0, sizeof(*result));
    result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
    result->value = NAN;
    result->residual = NAN;
    status = numerix_problem_callback_context_init(
        &callback_context, request, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    switch (algorithm)
    {
        case NUMERIX_ALGORITHM_BISECTION:
            status = numerix_root_bisection(
                numerix_problem_callback, &callback_context, lower, upper,
                options, result, error);
            break;
        case NUMERIX_ALGORITHM_REGULA_FALSI:
            status = numerix_root_regula_falsi(
                numerix_problem_callback, &callback_context, lower, upper,
                options, result, error);
            break;
        case NUMERIX_ALGORITHM_BRENT:
            status = numerix_root_brent(
                numerix_problem_callback, &callback_context, lower, upper,
                options, result, error);
            break;
        case NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON:
            status = numerix_root_safeguarded_newton(
                numerix_problem_callback, numerix_problem_derivative_callback,
                &callback_context, lower, upper, options, result, error);
            break;
        case NUMERIX_ALGORITHM_NEWTON:
            status = numerix_root_newton(
                numerix_problem_callback, numerix_problem_derivative_callback,
                &callback_context, initial, options, result, error);
            break;
        case NUMERIX_ALGORITHM_SECANT:
            status = numerix_root_secant(
                numerix_problem_callback, &callback_context, initial,
                request->problem.root.second_initial, options, result, error);
            break;
        default:
            memset(result, 0, sizeof(*result));
            result->status = NUMERIX_STATUS_UNSUPPORTED_OPERATION;
            result->value = NAN;
            result->residual = NAN;
            result->algorithm = numerix_algorithm_name(algorithm);
            return execution_error(error, result->status,
                                   "algorithm is not a root solver");
    }

    if (status != NUMERIX_STATUS_SUCCESS &&
        callback_context.last_status != NUMERIX_STATUS_SUCCESS)
    {
        status = callback_context.last_status;
        result->status = status;
        if (error != NULL)
        {
            *error = callback_context.last_error;
        }
    }
    return status;
}

int numerix_root_result_is_usable(
    const numerix_result *result, const numerix_options *options)
{
    double root_tolerance;

    if (result == NULL || options == NULL ||
        result->status != NUMERIX_STATUS_SUCCESS ||
        !isfinite(result->value))
    {
        return 0;
    }
    if (isfinite(result->residual) && result->residual == 0.0)
    {
        return 1;
    }
    root_tolerance = options->absolute_tolerance +
                     options->relative_tolerance * fabs(result->value);
    return isfinite(result->estimated_error) &&
           result->estimated_error <= root_tolerance;
}

static void append_attempt(
    numerix_root_selection_report *report, numerix_algorithm algorithm,
    int was_fallback, const char *reason, numerix_status status,
    const numerix_result *result, const numerix_error *error)
{
    numerix_algorithm_attempt *attempt;

    if (report->attempt_count >= NUMERIX_ROOT_SELECTION_MAX_ATTEMPTS)
    {
        return;
    }
    attempt = &report->attempts[report->attempt_count++];
    attempt->algorithm = algorithm;
    attempt->status = status;
    attempt->was_fallback = was_fallback;
    snprintf(attempt->reason, sizeof(attempt->reason), "%s", reason);
    attempt->result = *result;
    if (error != NULL)
    {
        attempt->error = *error;
    }
    else
    {
        numerix_error_clear(&attempt->error);
    }
    report->total_function_evaluations += result->function_evaluations;
    report->total_iterations += result->iterations;
}

static int missing_requirement_can_fallback(
    numerix_algorithm attempted,
    const numerix_recommendation_request *request,
    int has_bracket)
{
    if (request->derivative == NULL &&
        (attempted == NUMERIX_ALGORITHM_NEWTON ||
         attempted == NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON) &&
        (has_bracket || request->problem.root.has_second_initial)) {
        return 1;
    }
    if (attempted == NUMERIX_ALGORITHM_SECANT && has_bracket &&
        (!request->problem.root.has_initial ||
         !request->problem.root.has_second_initial)) {
        return 1;
    }
    return 0;
}

static int algorithm_was_attempted(
    const numerix_root_selection_report *report, numerix_algorithm algorithm)
{
    size_t index;

    for (index = 0; index < report->attempt_count; ++index)
    {
        if (report->attempts[index].algorithm == algorithm)
        {
            return 1;
        }
    }
    return 0;
}

static numerix_algorithm choose_fallback(
    numerix_algorithm attempted, const numerix_recommendation_request *request,
    int has_bracket, size_t fallback_index)
{
    if (fallback_index == 0)
    {
        if (has_bracket && attempted != NUMERIX_ALGORITHM_BRENT)
        {
            return NUMERIX_ALGORITHM_BRENT;
        }
        if (has_bracket && attempted == NUMERIX_ALGORITHM_BRENT)
        {
            return NUMERIX_ALGORITHM_BISECTION;
        }
        if (request->problem.root.has_second_initial &&
            attempted != NUMERIX_ALGORITHM_SECANT)
        {
            return NUMERIX_ALGORITHM_SECANT;
        }
        return NUMERIX_ALGORITHM_NONE;
    }
    if (fallback_index == 1 && has_bracket &&
        attempted != NUMERIX_ALGORITHM_BISECTION)
    {
        return NUMERIX_ALGORITHM_BISECTION;
    }
    return NUMERIX_ALGORITHM_NONE;
}

numerix_status numerix_root_solve_auto_controlled(
    const numerix_recommendation_request *request,
    const numerix_recommendation_request *execution_request,
    const numerix_root_selection_options *selection_options,
    numerix_result *result, numerix_root_selection_report *report,
    numerix_error *error, numerix_execution_stop_check stop_check,
    void *stop_user_data)
{
    numerix_root_selection_options defaults;
    numerix_options options;
    numerix_algorithm algorithm;
    numerix_status status;
    size_t fallback_index = 0;
    int first_attempt = 1;

    if (request == NULL || execution_request == NULL || result == NULL || report == NULL ||
        request->category != NUMERIX_PROBLEM_ROOT_FINDING ||
        !numerix_request_has_function(request))
    {
        return execution_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                               "root request, callback, result, and report are required");
    }
    memset(report, 0, sizeof(*report));
    memset(result, 0, sizeof(*result));
    result->value = NAN;
    result->residual = NAN;
    defaults.mode = NUMERIX_SELECTION_AUTOMATIC;
    defaults.explicit_algorithm = NUMERIX_ALGORITHM_NONE;
    defaults.enable_fallback = 0;
    if (selection_options == NULL)
    {
        selection_options = &defaults;
    }
    if ((selection_options->enable_fallback != 0 &&
         selection_options->enable_fallback != 1) ||
        (selection_options->mode != NUMERIX_SELECTION_AUTOMATIC &&
         selection_options->mode != NUMERIX_SELECTION_EXPLICIT))
    {
        result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        report->final_status = result->status;
        report->original_failure = result->status;
        return execution_error(error, result->status,
                               "unknown root selection mode");
    }
    report->fallback_enabled = selection_options->enable_fallback != 0;
    if (stop_check != NULL)
    {
        status = stop_check(stop_user_data, NULL);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            result->status = status;
            report->original_failure = status;
            report->final_status = status;
            return numerix_error_set(error, status,
                                     "managed execution stopped before recommendation");
        }
    }
    status = get_options(request, &options, error);
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        result->status = status;
        report->final_status = status;
        report->original_failure = status;
        return status;
    }

    {
        numerix_error recommendation_error;
        numerix_error_clear(&recommendation_error);
        status = numerix_recommend_algorithm(
            request, &report->recommendation, &recommendation_error);
        report->recommendation_status = status;
        report->recommendation_error = recommendation_error;
        if (status != NUMERIX_STATUS_SUCCESS && error != NULL)
        {
            *error = recommendation_error;
        }
    }
    if (stop_check != NULL)
    {
        numerix_status stop_status = stop_check(
            stop_user_data, &report->recommendation);
        if (stop_status != NUMERIX_STATUS_SUCCESS)
        {
            result->status = stop_status;
            report->original_failure = stop_status;
            report->final_status = stop_status;
            return numerix_error_set(error, stop_status,
                                     "managed execution stopped during recommendation");
        }
    }
    if (status != NUMERIX_STATUS_SUCCESS &&
        selection_options->mode == NUMERIX_SELECTION_AUTOMATIC)
    {
        result->status = status;
        report->original_failure = status;
        report->final_status = status;
        return status;
    }
    report->total_function_evaluations =
        report->recommendation.probe_evaluations +
        report->recommendation.derivative_probe_evaluations;

    if (selection_options->mode == NUMERIX_SELECTION_AUTOMATIC)
    {
        algorithm = report->recommendation.recommended_algorithm;
    }
    else
    {
        algorithm = selection_options->explicit_algorithm;
    }
    report->initially_selected = algorithm;
    if (algorithm == NUMERIX_ALGORITHM_NONE)
    {
        result->status = NUMERIX_STATUS_INVALID_INPUT;
        report->original_failure = result->status;
        report->final_status = result->status;
        return execution_error(error, result->status,
                               "no root algorithm was selected");
    }

    while (algorithm != NUMERIX_ALGORITHM_NONE &&
           report->attempt_count < NUMERIX_ROOT_SELECTION_MAX_ATTEMPTS)
    {
        numerix_result attempt_result;
        int usable;
        const char *reason;

        status = numerix_root_run_algorithm(
            algorithm, execution_request, &attempt_result, error);
        usable = numerix_root_result_is_usable(&attempt_result, &options);
        if (request->representation == NUMERIX_REPRESENTATION_PIECEWISE &&
            usable && fabs(attempt_result.residual) > options.absolute_tolerance)
        {
            usable = 0;
        }
        if (status == NUMERIX_STATUS_SUCCESS && !usable)
        {
            status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            attempt_result.status = status;
            numerix_error_set(error, status,
                              "solver reported success without a usable root estimate");
        }
        if (first_attempt)
        {
            reason = selection_options->mode == NUMERIX_SELECTION_EXPLICIT
                         ? "Executed the explicitly requested algorithm."
                         : "Executed the algorithm selected by the recommendation rules.";
        }
        else
        {
            reason = "Attempted an alternative after a recoverable convergence or numerical failure.";
        }
        append_attempt(report, algorithm, !first_attempt, reason,
                       status, &attempt_result, error);
        first_attempt = 0;
        report->final_algorithm = algorithm;
        report->final_status = status;
        if (report->attempt_count == 1)
        {
            report->original_failure = status;
        }
        if (stop_check != NULL)
        {
            numerix_status stop_status = stop_check(stop_user_data, NULL);
            if (stop_status != NUMERIX_STATUS_SUCCESS)
            {
                report->final_status = stop_status;
                report->fallback_used = report->attempt_count > 1;
                *result = attempt_result;
                result->status = stop_status;
                return numerix_error_set(error, stop_status,
                                         "managed execution stopped after algorithm attempt");
            }
        }
        if (status == NUMERIX_STATUS_SUCCESS)
        {
            *result = attempt_result;
            report->fallback_used = report->attempt_count > 1;
            numerix_error_clear(error);
            return NUMERIX_STATUS_SUCCESS;
        }
        if (stop_check != NULL &&
            report->total_iterations >= options.maximum_iterations)
        {
            report->final_status = NUMERIX_STATUS_LIMIT_REACHED;
            report->fallback_used = report->attempt_count > 1;
            *result = attempt_result;
            result->status = report->final_status;
            return numerix_error_set(
                error, report->final_status,
                "managed total iteration limit reached");
        }
        if (!selection_options->enable_fallback ||
            (status != NUMERIX_STATUS_CONVERGENCE_FAILURE &&
             status != NUMERIX_STATUS_NUMERICAL_FAILURE &&
             !(status == NUMERIX_STATUS_INVALID_ARGUMENT &&
               missing_requirement_can_fallback(
                   algorithm, request,
                   report->recommendation.sign_changing_bracket))))
        {
            break;
        }
        algorithm = choose_fallback(
            algorithm, request, report->recommendation.sign_changing_bracket,
            fallback_index++);
        if (algorithm_was_attempted(report, algorithm))
        {
            algorithm = NUMERIX_ALGORITHM_NONE;
        }
    }

    report->fallback_used = report->attempt_count > 1;
    if (report->attempt_count > 0)
    {
        const numerix_algorithm_attempt *last =
            &report->attempts[report->attempt_count - 1];
        *result = last->result;
        result->status = report->final_status;
    }
    return report->final_status;
}

numerix_status numerix_root_solve_auto(
    const numerix_recommendation_request *request,
    const numerix_root_selection_options *selection_options,
    numerix_result *result, numerix_root_selection_report *report,
    numerix_error *error)
{
    return numerix_root_solve_auto_controlled(
        request, request, selection_options, result, report, error, NULL, NULL);
}
