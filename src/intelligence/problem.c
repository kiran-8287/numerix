#include "intelligence_internal.h"

#include <math.h>

static numerix_status problem_error(
    numerix_error *error, numerix_status status, const char *message)
{
    return numerix_error_set(error, status, message);
}

int numerix_request_has_function(
    const numerix_recommendation_request *request)
{
    if (request == NULL)
    {
        return 0;
    }
    switch (request->representation)
    {
        case NUMERIX_REPRESENTATION_CALLBACK:
            return request->function != NULL;
        case NUMERIX_REPRESENTATION_EXPRESSION:
            return request->expression != NULL;
        case NUMERIX_REPRESENTATION_PIECEWISE:
            return request->piecewise_function != NULL;
    }
    return 0;
}

numerix_status numerix_request_evaluate(
    const numerix_recommendation_request *request, double x,
    double *value, numerix_error *error)
{
    if (request == NULL || value == NULL || !isfinite(x))
    {
        return problem_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "finite evaluation input and output are required");
    }
    switch (request->representation)
    {
        case NUMERIX_REPRESENTATION_CALLBACK:
            if (request->function == NULL)
            {
                break;
            }
            *value = request->function(x, request->user_data);
            if (isfinite(*value))
            {
                numerix_error_clear(error);
                return NUMERIX_STATUS_SUCCESS;
            }
            return problem_error(error, NUMERIX_STATUS_NUMERICAL_FAILURE,
                                 "problem callback returned a non-finite value");
        case NUMERIX_REPRESENTATION_EXPRESSION:
            if (request->expression != NULL)
            {
                return numerix_expression_evaluate(
                    request->expression, x, value, error);
            }
            break;
        case NUMERIX_REPRESENTATION_PIECEWISE:
            if (request->piecewise_function != NULL)
            {
                return numerix_piecewise_evaluate(
                    request->piecewise_function, x, value, error);
            }
            break;
    }
    return problem_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                         "problem representation is not initialized");
}

numerix_status numerix_problem_callback_context_init(
    numerix_problem_callback_context *context,
    const numerix_recommendation_request *request, numerix_error *error)
{
    if (context == NULL || !numerix_request_has_function(request))
    {
        return problem_error(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                             "callback context requires an evaluable problem");
    }
    context->request = request;
    context->last_status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(&context->last_error);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

double numerix_problem_callback(double x, void *user_data)
{
    numerix_problem_callback_context *context = user_data;
    double value = NAN;

    if (context == NULL || context->request == NULL)
    {
        if (context != NULL)
        {
            context->last_status = NUMERIX_STATUS_INVALID_ARGUMENT;
            numerix_error_set(&context->last_error, context->last_status,
                              "problem callback context is not initialized");
        }
        return NAN;
    }
    context->last_status = numerix_request_evaluate(
        context->request, x, &value, &context->last_error);
    return context->last_status == NUMERIX_STATUS_SUCCESS ? value : NAN;
}


double numerix_problem_derivative_callback(double x, void *user_data)
{
    numerix_problem_callback_context *context = user_data;

    if (context == NULL || context->request == NULL ||
        context->request->derivative == NULL)
    {
        return NAN;
    }
    return context->request->derivative(
        x, context->request->user_data);
}
