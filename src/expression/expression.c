#include "numerix_expression_internal.h"

#include <math.h>
#include <string.h>

numerix_status numerix_expression_compile(
    const char *source,
    numerix_expression **expression_out,
    numerix_error *error)
{
    numerix_token *tokens = NULL;
    size_t token_count = 0;
    numerix_expression_node *root = NULL;
    numerix_expression *expression;
    numerix_status status;

    if (expression_out == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "expression output is required");
    }

    *expression_out = NULL;

    if (source == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "expression source must not be NULL");
    }

    status = numerix_expression_lex(source, &tokens, &token_count, error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    status = numerix_expression_parse(
        source,
        tokens,
        token_count,
        &root,
        error);
    numerix_deallocate(tokens);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    expression = numerix_allocate(sizeof(*expression));

    if (expression == NULL)
    {
        numerix_expression_node_destroy(root);
        return numerix_error_set(
            error,
            NUMERIX_STATUS_ALLOCATION_FAILURE,
            "could not allocate compiled expression");
    }

    expression->root = root;
    *expression_out = expression;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_expression_evaluate(
    const numerix_expression *expression,
    double x,
    double *value_out,
    numerix_error *error)
{
    double value;
    numerix_status status;

    if (expression == NULL || expression->root == NULL || value_out == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "compiled expression and value output are required");
    }

    status = numerix_expression_evaluate_node(
        expression->root,
        x,
        &value,
        error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return status;
    }

    *value_out = value;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

void numerix_expression_destroy(numerix_expression *expression)
{
    if (expression == NULL)
    {
        return;
    }

    numerix_expression_node_destroy(expression->root);
    numerix_deallocate(expression);
}

numerix_status numerix_expression_callback_context_init(
    numerix_expression_callback_context *context,
    const numerix_expression *expression,
    numerix_error *error)
{
    if (context == NULL || expression == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "callback context and compiled expression are required");
    }

    context->expression = expression;
    context->last_status = NUMERIX_STATUS_SUCCESS;
    numerix_error_clear(&context->last_error);
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}

double numerix_expression_callback(double x, void *user_data)
{
    numerix_expression_callback_context *context = user_data;
    double value;
    numerix_status status;

    if (context == NULL || context->expression == NULL)
    {
        if (context != NULL)
        {
            context->last_status = NUMERIX_STATUS_INVALID_ARGUMENT;
            numerix_error_set(
                &context->last_error,
                context->last_status,
                "expression callback context is not initialized");
        }

        return NAN;
    }

    status = numerix_expression_evaluate(
        context->expression,
        x,
        &value,
        &context->last_error);
    context->last_status = status;

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        return NAN;
    }

    return value;
}

numerix_status numerix_expression_callback_status(
    const numerix_expression_callback_context *context)
{
    if (context == NULL)
    {
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }

    return context->last_status;
}

const numerix_error *numerix_expression_callback_error(
    const numerix_expression_callback_context *context)
{
    if (context == NULL)
    {
        return NULL;
    }

    return &context->last_error;
}
