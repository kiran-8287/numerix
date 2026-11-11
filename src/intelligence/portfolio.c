#include "intelligence_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static numerix_status portfolio_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

static int attempt_is_usable(
    numerix_result *result, numerix_status status,
    const numerix_options *options)
{
    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return 0;
    }
    if (numerix_root_result_is_usable(result, options))
    {
        return 1;
    }
    result->status = NUMERIX_STATUS_NUMERICAL_FAILURE;
    return 0;
}

static void record_attempt(
    numerix_root_portfolio_report *report, numerix_algorithm algorithm,
    numerix_status status, const numerix_result *result,
    const numerix_error *error)
{
    numerix_algorithm_attempt *attempt =
        &report->attempts[report->attempt_count++];

    attempt->algorithm = algorithm;
    attempt->status = status;
    attempt->was_fallback = 0;
    snprintf(attempt->reason, sizeof(attempt->reason),
             "Independent sequential portfolio attempt.");
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

numerix_status numerix_root_solve_portfolio(
    const numerix_recommendation_request *request,
    const numerix_root_portfolio_options *portfolio_options,
    numerix_result *best_result, numerix_root_portfolio_report *report,
    numerix_error *error)
{
    static const numerix_algorithm candidates[] = {
        NUMERIX_ALGORITHM_BRENT,
        NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON,
        NUMERIX_ALGORITHM_BISECTION,
        NUMERIX_ALGORITHM_REGULA_FALSI,
        NUMERIX_ALGORITHM_NEWTON,
        NUMERIX_ALGORITHM_SECANT
    };
    const unsigned int valid_mask =
        (1U << NUMERIX_ALGORITHM_BISECTION) |
        (1U << NUMERIX_ALGORITHM_REGULA_FALSI) |
        (1U << NUMERIX_ALGORITHM_SECANT) |
        (1U << NUMERIX_ALGORITHM_NEWTON) |
        (1U << NUMERIX_ALGORITHM_SAFEGUARDED_NEWTON) |
        (1U << NUMERIX_ALGORITHM_BRENT);
    numerix_recommendation recommendation;
    numerix_options options;
    unsigned int mask = portfolio_options == NULL
                            ? 0U
                            : portfolio_options->algorithm_mask;
    size_t index;
    size_t successful_count = 0;
    size_t best_index = 0;
    numerix_status status;

    if (request == NULL || best_result == NULL || report == NULL ||
        request->category != NUMERIX_PROBLEM_ROOT_FINDING ||
        !numerix_request_has_function(request))
    {
        return portfolio_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                               "root request, callback, result, and report are required");
    }
    memset(report, 0, sizeof(*report));
    memset(best_result, 0, sizeof(*best_result));
    best_result->value = NAN;
    best_result->residual = NAN;
    if ((mask & ~valid_mask) != 0U)
    {
        best_result->status = NUMERIX_STATUS_INVALID_ARGUMENT;
        return portfolio_error(error, best_result->status,
                               "portfolio mask contains a non-root algorithm");
    }

    {
        numerix_error recommendation_error;
        numerix_error_clear(&recommendation_error);
        status = numerix_recommend_algorithm(
            request, &recommendation, &recommendation_error);
        report->recommendation_status = status;
        report->recommendation_error = recommendation_error;
        if (status != NUMERIX_STATUS_SUCCESS && error != NULL)
        {
            *error = recommendation_error;
        }
    }
    report->recommendation_probe_evaluations = recommendation.probe_evaluations;
    report->total_function_evaluations = recommendation.probe_evaluations +
        recommendation.derivative_probe_evaluations;
    if (status != NUMERIX_STATUS_SUCCESS && mask == 0U)
    {
        best_result->status = status;
        return status;
    }
    if (request->options == NULL)
    {
        numerix_options_init(&options);
    }
    else
    {
        options = *request->options;
        status = numerix_options_validate(&options, error);
        if (status != NUMERIX_STATUS_SUCCESS)
        {
            best_result->status = status;
            return status;
        }
    }

    for (index = 0; index < sizeof(candidates) / sizeof(candidates[0]); ++index)
    {
        numerix_algorithm algorithm = candidates[index];
        numerix_result current;
        int requested = mask != 0U &&
                        (mask & (1U << algorithm)) != 0U;
        int applicable = numerix_root_algorithm_applicable(
            algorithm, request, recommendation.sign_changing_bracket);
        int usable;

        if (mask == 0U && !applicable)
        {
            continue;
        }
        if (mask != 0U && !requested)
        {
            continue;
        }
        status = numerix_root_run_algorithm(algorithm, request, &current, error);
        usable = attempt_is_usable(&current, status, &options);
        if (request->representation == NUMERIX_REPRESENTATION_PIECEWISE &&
            usable && fabs(current.residual) > options.absolute_tolerance)
        {
            usable = 0;
            current.status = NUMERIX_STATUS_NUMERICAL_FAILURE;
        }
        if (status == NUMERIX_STATUS_SUCCESS && !usable)
        {
            status = NUMERIX_STATUS_NUMERICAL_FAILURE;
            numerix_error_set(error, status,
                              "solver reported success without a finite accurate root result");
        }
        record_attempt(report, algorithm, status, &current, error);

        if (!usable)
        {
            continue;
        }
        ++successful_count;
        if (!report->has_selected_attempt ||
            fabs(current.residual) <
                fabs(report->attempts[best_index].result.residual) ||
            (fabs(current.residual) ==
                 fabs(report->attempts[best_index].result.residual) &&
             current.iterations < report->attempts[best_index].result.iterations))
        {
            best_index = report->attempt_count - 1;
            report->selected_attempt = best_index;
            report->has_selected_attempt = 1;
            *best_result = current;
        }
    }

    if (report->attempt_count == 0)
    {
        best_result->status = NUMERIX_STATUS_INVALID_INPUT;
        return portfolio_error(error, best_result->status,
                               "no applicable root algorithm was available");
    }
    report->successful_results_agree = 1;
    if (successful_count > 1)
    {
        size_t attempt_index;
        double tolerance = options.absolute_tolerance +
                           options.relative_tolerance * fabs(best_result->value);
        for (attempt_index = 0; attempt_index < report->attempt_count;
             ++attempt_index)
        {
            const numerix_algorithm_attempt *attempt =
                &report->attempts[attempt_index];
            numerix_result checked_result = attempt->result;
            if (attempt_is_usable(&checked_result, attempt->status, &options) &&
                fabs(attempt->result.value - best_result->value) > tolerance)
            {
                report->successful_results_agree = 0;
                break;
            }
        }
    }
    if (!report->has_selected_attempt)
    {
        numerix_status failure_status = NUMERIX_STATUS_CONVERGENCE_FAILURE;
        size_t attempt_index;

        for (attempt_index = 0; attempt_index < report->attempt_count;
             ++attempt_index)
        {
            numerix_status attempt_status =
                report->attempts[attempt_index].status;
            if (attempt_status == NUMERIX_STATUS_INVALID_ARGUMENT ||
                attempt_status == NUMERIX_STATUS_INVALID_INPUT ||
                attempt_status == NUMERIX_STATUS_INVALID_DIMENSIONS)
            {
                failure_status = attempt_status;
                if (error != NULL)
                {
                    *error = report->attempts[attempt_index].error;
                }
                break;
            }
            if (attempt_status == NUMERIX_STATUS_LIMIT_REACHED ||
                attempt_status == NUMERIX_STATUS_NUMERICAL_FAILURE)
            {
                failure_status = attempt_status;
                if (error != NULL)
                {
                    *error = report->attempts[attempt_index].error;
                }
            }
        }
        best_result->status = failure_status;
        return portfolio_error(error, best_result->status,
                               "no portfolio candidate produced a valid converged root");
    }
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
