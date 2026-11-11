#include "numerix_expression_internal.h"

#include <stdio.h>
#include <string.h>

#define NUMERIX_EXPRESSION_MAX_NODE_COUNT 4096U
#define NUMERIX_EXPRESSION_MAX_TREE_DEPTH 256U

typedef struct expression_parser {
    const char *source;
    const numerix_token *tokens;
    size_t token_count;
    size_t cursor;
    size_t node_count;
    numerix_status status;
    numerix_error *error;
} expression_parser;

typedef struct function_description {
    const char *name;
    numerix_expression_function function;
    size_t argument_count;
} function_description;

static const function_description functions[] =
{
    { "sin", NUMERIX_FUNCTION_SIN, 1 },
    { "cos", NUMERIX_FUNCTION_COS, 1 },
    { "tan", NUMERIX_FUNCTION_TAN, 1 },
    { "asin", NUMERIX_FUNCTION_ASIN, 1 },
    { "acos", NUMERIX_FUNCTION_ACOS, 1 },
    { "atan", NUMERIX_FUNCTION_ATAN, 1 },
    { "sinh", NUMERIX_FUNCTION_SINH, 1 },
    { "cosh", NUMERIX_FUNCTION_COSH, 1 },
    { "tanh", NUMERIX_FUNCTION_TANH, 1 },
    { "sqrt", NUMERIX_FUNCTION_SQRT, 1 },
    { "abs", NUMERIX_FUNCTION_ABS, 1 },
    { "exp", NUMERIX_FUNCTION_EXP, 1 },
    { "log", NUMERIX_FUNCTION_LOG, 1 },
    { "log10", NUMERIX_FUNCTION_LOG10, 1 },
    { "floor", NUMERIX_FUNCTION_FLOOR, 1 },
    { "ceil", NUMERIX_FUNCTION_CEIL, 1 },
    { "pow", NUMERIX_FUNCTION_POW, 2 },
    { "min", NUMERIX_FUNCTION_MIN, 2 },
    { "max", NUMERIX_FUNCTION_MAX, 2 }
};

static const numerix_token *current_token(const expression_parser *parser)
{
    if (parser->cursor >= parser->token_count)
    {
        return &parser->tokens[parser->token_count - 1];
    }

    return &parser->tokens[parser->cursor];
}

static const numerix_token *advance_token(expression_parser *parser)
{
    const numerix_token *token = current_token(parser);

    if (token->type != NUMERIX_TOKEN_END)
    {
        ++parser->cursor;
    }

    return token;
}

static int token_matches(
    const expression_parser *parser,
    const numerix_token *token,
    const char *text)
{
    size_t length = strlen(text);

    return token->type == NUMERIX_TOKEN_IDENTIFIER &&
           token->length == length &&
           memcmp(parser->source + token->position, text, length) == 0;
}

static void set_parser_error(
    expression_parser *parser,
    numerix_status status,
    size_t position,
    const char *message)
{
    char formatted[NUMERIX_ERROR_MESSAGE_CAPACITY];

    if (parser->status != NUMERIX_STATUS_SUCCESS)
    {
        return;
    }

    parser->status = status;
    (void)snprintf(
        formatted,
        sizeof(formatted),
        "%s at character %lu",
        message,
        (unsigned long)position);
    numerix_error_set(parser->error, status, formatted);
}

static void set_expected_expression_error(expression_parser *parser)
{
    const numerix_token *token = current_token(parser);

    if (token->type == NUMERIX_TOKEN_END)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "missing operand at end of expression");
    }
    else if (token->type == NUMERIX_TOKEN_RIGHT_PARENTHESIS)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "missing operand before closing parenthesis");
    }
    else
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "expected a number, variable, function, or parenthesized expression");
    }
}

static numerix_expression_node *create_node(
    expression_parser *parser,
    numerix_expression_node_type type,
    size_t position,
    double number,
    numerix_expression_function function,
    numerix_expression_node *left,
    numerix_expression_node *right)
{
    numerix_expression_node *node;
    size_t left_depth = left == NULL ? 0 : left->depth;
    size_t right_depth = right == NULL ? 0 : right->depth;

    if (parser->node_count >= NUMERIX_EXPRESSION_MAX_NODE_COUNT)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            position,
            "expression contains too many syntax nodes");
        return NULL;
    }

    node = numerix_allocate(sizeof(*node));

    if (node == NULL)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_ALLOCATION_FAILURE,
            position,
            "could not allocate an expression syntax node");
        return NULL;
    }

    node->type = type;
    node->function = function;
    node->number = number;
    node->position = position;
    node->depth = (left_depth > right_depth ? left_depth : right_depth) + 1;
    node->left = left;
    node->right = right;
    ++parser->node_count;

    if (node->depth > NUMERIX_EXPRESSION_MAX_TREE_DEPTH)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            position,
            "expression syntax tree is too deeply nested");
        numerix_deallocate(node);
        return NULL;
    }

    return node;
}

void numerix_expression_node_destroy(numerix_expression_node *node)
{
    if (node == NULL)
    {
        return;
    }

    numerix_expression_node_destroy(node->left);
    numerix_expression_node_destroy(node->right);
    numerix_deallocate(node);
}

static const function_description *find_function(
    const expression_parser *parser,
    const numerix_token *token)
{
    size_t i;

    for (i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i)
    {
        if (token_matches(parser, token, functions[i].name))
        {
            return &functions[i];
        }
    }

    return NULL;
}

static numerix_expression_node *parse_additive(expression_parser *parser);
static numerix_expression_node *parse_unary(expression_parser *parser);

static numerix_expression_node *parse_function_call(
    expression_parser *parser,
    const numerix_token *identifier,
    const function_description *description)
{
    numerix_expression_node *first_argument = NULL;
    numerix_expression_node *second_argument = NULL;
    numerix_expression_node *function_node;
    const numerix_token *token;

    advance_token(parser);
    token = current_token(parser);

    if (token->type != NUMERIX_TOKEN_LEFT_PARENTHESIS)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "expected opening parenthesis after function name");
        return NULL;
    }

    advance_token(parser);
    token = current_token(parser);

    if (token->type == NUMERIX_TOKEN_RIGHT_PARENTHESIS)
    {
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "function has too few arguments");
        return NULL;
    }

    first_argument = parse_additive(parser);

    if (first_argument == NULL)
    {
        return NULL;
    }

    token = current_token(parser);

    if (description->argument_count == 2)
    {
        if (token->type != NUMERIX_TOKEN_COMMA)
        {
            numerix_expression_node_destroy(first_argument);
            set_parser_error(
                parser,
                NUMERIX_STATUS_INVALID_INPUT,
                token->position,
                "two-argument function requires a comma and second argument");
            return NULL;
        }

        advance_token(parser);
        second_argument = parse_additive(parser);

        if (second_argument == NULL)
        {
            numerix_expression_node_destroy(first_argument);
            return NULL;
        }

        token = current_token(parser);
    }
    else if (token->type == NUMERIX_TOKEN_COMMA)
    {
        numerix_expression_node_destroy(first_argument);
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "one-argument function does not accept a second argument");
        return NULL;
    }

    if (token->type != NUMERIX_TOKEN_RIGHT_PARENTHESIS)
    {
        numerix_expression_node_destroy(first_argument);
        numerix_expression_node_destroy(second_argument);
        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "expected closing parenthesis after function arguments");
        return NULL;
    }

    advance_token(parser);
    function_node = create_node(
        parser,
        NUMERIX_NODE_FUNCTION,
        identifier->position,
        0.0,
        description->function,
        first_argument,
        second_argument);

    if (function_node == NULL)
    {
        numerix_expression_node_destroy(first_argument);
        numerix_expression_node_destroy(second_argument);
    }

    return function_node;
}

static numerix_expression_node *parse_primary(expression_parser *parser)
{
    const numerix_token *token = current_token(parser);
    numerix_expression_node *node;

    if (token->type == NUMERIX_TOKEN_NUMBER)
    {
        advance_token(parser);
        return create_node(
            parser,
            NUMERIX_NODE_NUMBER,
            token->position,
            token->number,
            NUMERIX_FUNCTION_SIN,
            NULL,
            NULL);
    }

    if (token->type == NUMERIX_TOKEN_IDENTIFIER)
    {
        const function_description *description = find_function(parser, token);

        if (description != NULL)
        {
            return parse_function_call(parser, token, description);
        }

        if (token_matches(parser, token, "x"))
        {
            advance_token(parser);
            return create_node(
                parser,
                NUMERIX_NODE_VARIABLE,
                token->position,
                0.0,
                NUMERIX_FUNCTION_SIN,
                NULL,
                NULL);
        }

        if (token_matches(parser, token, "pi"))
        {
            advance_token(parser);
            return create_node(
                parser,
                NUMERIX_NODE_CONSTANT,
                token->position,
                3.14159265358979323846,
                NUMERIX_FUNCTION_SIN,
                NULL,
                NULL);
        }

        if (token_matches(parser, token, "e"))
        {
            advance_token(parser);
            return create_node(
                parser,
                NUMERIX_NODE_CONSTANT,
                token->position,
                2.71828182845904523536,
                NUMERIX_FUNCTION_SIN,
                NULL,
                NULL);
        }

        set_parser_error(
            parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            "unknown variable or function name");
        return NULL;
    }

    if (token->type == NUMERIX_TOKEN_LEFT_PARENTHESIS)
    {
        advance_token(parser);
        node = parse_additive(parser);

        if (node == NULL)
        {
            return NULL;
        }

        token = current_token(parser);

        if (token->type != NUMERIX_TOKEN_RIGHT_PARENTHESIS)
        {
            numerix_expression_node_destroy(node);
            set_parser_error(
                parser,
                NUMERIX_STATUS_INVALID_INPUT,
                token->position,
                "missing closing parenthesis");
            return NULL;
        }

        advance_token(parser);
        return node;
    }

    set_expected_expression_error(parser);
    return NULL;
}

static numerix_expression_node *parse_power(expression_parser *parser)
{
    const numerix_token *token;
    numerix_expression_node *left = parse_primary(parser);
    numerix_expression_node *right;
    numerix_expression_node *node;

    if (left == NULL)
    {
        return NULL;
    }

    token = current_token(parser);

    if (token->type != NUMERIX_TOKEN_POWER)
    {
        return left;
    }

    advance_token(parser);
    right = parse_unary(parser);

    if (right == NULL)
    {
        numerix_expression_node_destroy(left);
        return NULL;
    }

    node = create_node(
        parser,
        NUMERIX_NODE_POWER,
        token->position,
        0.0,
        NUMERIX_FUNCTION_SIN,
        left,
        right);

    if (node == NULL)
    {
        numerix_expression_node_destroy(left);
        numerix_expression_node_destroy(right);
    }

    return node;
}

static numerix_expression_node *parse_unary(expression_parser *parser)
{
    const numerix_token *token = current_token(parser);
    numerix_expression_node_type type;
    numerix_expression_node *child;
    numerix_expression_node *node;

    if (token->type != NUMERIX_TOKEN_PLUS && token->type != NUMERIX_TOKEN_MINUS)
    {
        return parse_power(parser);
    }

    advance_token(parser);
    child = parse_unary(parser);

    if (child == NULL)
    {
        return NULL;
    }

    type = token->type == NUMERIX_TOKEN_PLUS
        ? NUMERIX_NODE_UNARY_PLUS
        : NUMERIX_NODE_UNARY_MINUS;
    node = create_node(
        parser,
        type,
        token->position,
        0.0,
        NUMERIX_FUNCTION_SIN,
        child,
        NULL);

    if (node == NULL)
    {
        numerix_expression_node_destroy(child);
    }

    return node;
}

static numerix_expression_node *parse_multiplicative(expression_parser *parser)
{
    numerix_expression_node *left = parse_unary(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (current_token(parser)->type == NUMERIX_TOKEN_MULTIPLY ||
           current_token(parser)->type == NUMERIX_TOKEN_DIVIDE)
    {
        const numerix_token *operator_token = advance_token(parser);
        numerix_expression_node *right = parse_unary(parser);
        numerix_expression_node *combined;
        numerix_expression_node_type type;

        if (right == NULL)
        {
            numerix_expression_node_destroy(left);
            return NULL;
        }

        type = operator_token->type == NUMERIX_TOKEN_MULTIPLY
            ? NUMERIX_NODE_MULTIPLY
            : NUMERIX_NODE_DIVIDE;
        combined = create_node(
            parser,
            type,
            operator_token->position,
            0.0,
            NUMERIX_FUNCTION_SIN,
            left,
            right);

        if (combined == NULL)
        {
            numerix_expression_node_destroy(left);
            numerix_expression_node_destroy(right);
            return NULL;
        }

        left = combined;
    }

    return left;
}

static numerix_expression_node *parse_additive(expression_parser *parser)
{
    numerix_expression_node *left = parse_multiplicative(parser);

    if (left == NULL)
    {
        return NULL;
    }

    while (current_token(parser)->type == NUMERIX_TOKEN_PLUS ||
           current_token(parser)->type == NUMERIX_TOKEN_MINUS)
    {
        const numerix_token *operator_token = advance_token(parser);
        numerix_expression_node *right = parse_multiplicative(parser);
        numerix_expression_node *combined;
        numerix_expression_node_type type;

        if (right == NULL)
        {
            numerix_expression_node_destroy(left);
            return NULL;
        }

        type = operator_token->type == NUMERIX_TOKEN_PLUS
            ? NUMERIX_NODE_ADD
            : NUMERIX_NODE_SUBTRACT;
        combined = create_node(
            parser,
            type,
            operator_token->position,
            0.0,
            NUMERIX_FUNCTION_SIN,
            left,
            right);

        if (combined == NULL)
        {
            numerix_expression_node_destroy(left);
            numerix_expression_node_destroy(right);
            return NULL;
        }

        left = combined;
    }

    return left;
}

numerix_status numerix_expression_parse(
    const char *source,
    const numerix_token *tokens,
    size_t token_count,
    numerix_expression_node **root,
    numerix_error *error)
{
    expression_parser parser;
    numerix_expression_node *node;

    if (source == NULL || tokens == NULL || token_count == 0 || root == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "source, tokens, token count, and root output are required");
    }

    *root = NULL;
    parser.source = source;
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.cursor = 0;
    parser.node_count = 0;
    parser.status = NUMERIX_STATUS_SUCCESS;
    parser.error = error;

    node = parse_additive(&parser);

    if (node == NULL)
    {
        return parser.status == NUMERIX_STATUS_SUCCESS
            ? numerix_error_set(error, NUMERIX_STATUS_INVALID_INPUT, "invalid expression")
            : parser.status;
    }

    if (current_token(&parser)->type != NUMERIX_TOKEN_END)
    {
        const numerix_token *token = current_token(&parser);

        numerix_expression_node_destroy(node);
        set_parser_error(
            &parser,
            NUMERIX_STATUS_INVALID_INPUT,
            token->position,
            token->type == NUMERIX_TOKEN_RIGHT_PARENTHESIS
                ? "unmatched closing parenthesis"
                : token->type == NUMERIX_TOKEN_COMMA
                    ? "comma is only valid between function arguments"
                    : "unexpected token after expression");
        return parser.status;
    }

    *root = node;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
