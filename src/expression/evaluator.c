#include "numerix_expression_internal.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>

static numerix_status evaluation_error(
    numerix_error *error,
    numerix_status status,
    size_t position,
    const char *message)
{
    char formatted[NUMERIX_ERROR_MESSAGE_CAPACITY];

    (void)snprintf(
        formatted,
        sizeof(formatted),
        "%s at character %lu",
        message,
        (unsigned long)position);

    numerix_error_set(error, status, formatted);
    return status;
}

static numerix_status evaluate_function(
    const numerix_expression_node *node,
    double first,
    double second,
    double *value,
    numerix_error *error)
{
    errno = 0;

    switch (node->function)
    {
        case NUMERIX_FUNCTION_SIN:
            *value = sin(first);
            break;
        case NUMERIX_FUNCTION_COS:
            *value = cos(first);
            break;
        case NUMERIX_FUNCTION_TAN:
            *value = tan(first);
            break;
        case NUMERIX_FUNCTION_ASIN:
            if (first < -1.0 || first > 1.0)
            {
                return evaluation_error(
                    error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    node->position,
                    "asin argument must be between -1 and 1");
            }
            *value = asin(first);
            break;
        case NUMERIX_FUNCTION_ACOS:
            if (first < -1.0 || first > 1.0)
            {
                return evaluation_error(
                    error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    node->position,
                    "acos argument must be between -1 and 1");
            }
            *value = acos(first);
            break;
        case NUMERIX_FUNCTION_ATAN:
            *value = atan(first);
            break;
        case NUMERIX_FUNCTION_SINH:
            *value = sinh(first);
            break;
        case NUMERIX_FUNCTION_COSH:
            *value = cosh(first);
            break;
        case NUMERIX_FUNCTION_TANH:
            *value = tanh(first);
            break;
        case NUMERIX_FUNCTION_SQRT:
            if (first < 0.0)
            {
                return evaluation_error(
                    error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    node->position,
                    "sqrt argument must not be negative");
            }
            *value = sqrt(first);
            break;
        case NUMERIX_FUNCTION_ABS:
            *value = fabs(first);
            break;
        case NUMERIX_FUNCTION_EXP:
            *value = exp(first);
            break;
        case NUMERIX_FUNCTION_LOG:
            if (first <= 0.0)
            {
                return evaluation_error(
                    error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    node->position,
                    "log argument must be positive");
            }
            *value = log(first);
            break;
        case NUMERIX_FUNCTION_LOG10:
            if (first <= 0.0)
            {
                return evaluation_error(
                    error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    node->position,
                    "log10 argument must be positive");
            }
            *value = log10(first);
            break;
        case NUMERIX_FUNCTION_FLOOR:
            *value = floor(first);
            break;
        case NUMERIX_FUNCTION_CEIL:
            *value = ceil(first);
            break;
        case NUMERIX_FUNCTION_POW:
            if (first == 0.0 && second <= 0.0)
            {
                return evaluation_error(
                    error,
                    NUMERIX_STATUS_INVALID_INPUT,
                    node->position,
                    "zero cannot be raised to a non-positive power");
            }

            if (first < 0.0)
            {
                double integer_part;

                if (modf(second, &integer_part) != 0.0)
                {
                    return evaluation_error(
                        error,
                        NUMERIX_STATUS_INVALID_INPUT,
                        node->position,
                        "negative base requires an integer exponent");
                }
            }

            *value = pow(first, second);
            break;
        case NUMERIX_FUNCTION_MIN:
            *value = first < second ? first : second;
            break;
        case NUMERIX_FUNCTION_MAX:
            *value = first > second ? first : second;
            break;
        default:
            return evaluation_error(
                error,
                NUMERIX_STATUS_INTERNAL_ERROR,
                node->position,
                "unknown compiled mathematical function");
    }

    if (errno == EDOM)
    {
        return evaluation_error(
            error,
            NUMERIX_STATUS_INVALID_INPUT,
            node->position,
            "mathematical function domain error");
    }

    if (errno == ERANGE || !isfinite(*value))
    {
        return evaluation_error(
            error,
            NUMERIX_STATUS_NUMERICAL_FAILURE,
            node->position,
            "mathematical function overflow or underflow");
    }

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status evaluate_node(
    const numerix_expression_node *node,
    double x,
    double *value,
    numerix_error *error)
{
    double left_value;
    double right_value;
    numerix_status status;

    if (node == NULL || value == NULL)
    {
        numerix_error_set(error, NUMERIX_STATUS_INVALID_ARGUMENT,
                          "expression node and value output are required");
        return NUMERIX_STATUS_INVALID_ARGUMENT;
    }

    switch (node->type)
    {
        case NUMERIX_NODE_NUMBER:
        case NUMERIX_NODE_CONSTANT:
            *value = node->number;
            return NUMERIX_STATUS_SUCCESS;
        case NUMERIX_NODE_VARIABLE:
            *value = x;
            return NUMERIX_STATUS_SUCCESS;
        case NUMERIX_NODE_UNARY_PLUS:
            status = evaluate_node(node->left, x, value, error);
            return status;
        case NUMERIX_NODE_UNARY_MINUS:
            status = evaluate_node(node->left, x, &left_value, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }
            *value = -left_value;
            break;
        case NUMERIX_NODE_FUNCTION:
            status = evaluate_node(node->left, x, &left_value, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }

            right_value = 0.0;

            if (node->right != NULL)
            {
                status = evaluate_node(node->right, x, &right_value, error);
                if (status != NUMERIX_STATUS_SUCCESS)
                {
                    return status;
                }
            }

            status = evaluate_function(node, left_value, right_value, value, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }

            return NUMERIX_STATUS_SUCCESS;
        case NUMERIX_NODE_ADD:
        case NUMERIX_NODE_SUBTRACT:
        case NUMERIX_NODE_MULTIPLY:
        case NUMERIX_NODE_DIVIDE:
        case NUMERIX_NODE_POWER:
            status = evaluate_node(node->left, x, &left_value, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }

            status = evaluate_node(node->right, x, &right_value, error);
            if (status != NUMERIX_STATUS_SUCCESS)
            {
                return status;
            }

            switch (node->type)
            {
                case NUMERIX_NODE_ADD:
                    *value = left_value + right_value;
                    break;
                case NUMERIX_NODE_SUBTRACT:
                    *value = left_value - right_value;
                    break;
                case NUMERIX_NODE_MULTIPLY:
                    *value = left_value * right_value;
                    break;
                case NUMERIX_NODE_DIVIDE:
                    if (right_value == 0.0)
                    {
                        return evaluation_error(
                            error,
                            NUMERIX_STATUS_INVALID_INPUT,
                            node->position,
                            "division by zero");
                    }
                    *value = left_value / right_value;
                    break;
                case NUMERIX_NODE_POWER:
                    return evaluate_function(
                        &(numerix_expression_node){
                            .function = NUMERIX_FUNCTION_POW,
                            .position = node->position
                        },
                        left_value,
                        right_value,
                        value,
                        error);
                default:
                    return evaluation_error(
                        error,
                        NUMERIX_STATUS_INTERNAL_ERROR,
                        node->position,
                        "unknown binary expression operation");
            }
            break;
        default:
            return evaluation_error(
                error,
                NUMERIX_STATUS_INTERNAL_ERROR,
                node->position,
                "unknown compiled expression node");
    }

    if (!isfinite(*value))
    {
        return evaluation_error(
            error,
            NUMERIX_STATUS_NUMERICAL_FAILURE,
            node->position,
            "expression produced a non-finite result");
    }

    return NUMERIX_STATUS_SUCCESS;
}

numerix_status numerix_expression_evaluate_node(
    const numerix_expression_node *node,
    double x,
    double *value,
    numerix_error *error)
{
    if (!isfinite(x))
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "expression input x must be finite");
    }

    return evaluate_node(node, x, value, error);
}
